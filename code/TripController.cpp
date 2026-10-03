#include "TripController.h"

#include <QFileInfo>
#include <QUrl>
#include <algorithm>
#include <exception>
#include <QVariantMap>
#include <cmath>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QVariantList>

TripController::TripController(QObject *parent)
    : QObject(parent)
{
    connect(&m_replayTimer, &QTimer::timeout, this, &TripController::advanceReplay);
    m_replayTimer.setInterval(200);
    loadTripHistory();
}

bool TripController::tripLoaded() const { return m_tripLoaded; }
QString TripController::tripFileName() const { return m_tripFileName; }

double TripController::journeyProgress() const
{
    if (!m_tripLoaded || m_samples.size() < 2) return 0.0;
    return std::clamp((100.0 * m_currentIndex) / double(m_samples.size() - 1), 0.0, 100.0);
}

double TripController::distanceTravelledKm() const { return m_distanceTravelledKm; }
double TripController::timeTravelledSeconds() const { return m_timeTravelledSeconds; }
double TripController::liveSpeedKmh() const { return m_liveSpeedKmh; }
double TripController::avgSpeedKmh() const { return m_avgSpeedKmh; }
bool TripController::journeyFinished() const { return m_journeyFinished; }

double TripController::fuelUsedL() const { return m_tripLoaded ? m_summary.fuel_est_l : 0.0; }
double TripController::avgFuelRateLph() const { return m_tripLoaded ? m_summary.avg_fuel_rate_lph : 0.0; }
double TripController::avgFuelEconomyLPer100Km() const { return m_tripLoaded ? m_summary.avg_l_per_100km : 0.0; }
double TripController::idleSeconds() const { return m_tripLoaded ? m_summary.idle_seconds : 0.0; }
int TripController::highRpmCount() const { return m_tripLoaded ? m_summary.high_rpm_count : 0; }
int TripController::harshAccelCount() const { return m_tripLoaded ? m_summary.harsh_accel_count : 0; }
int TripController::harshBrakeCount() const { return m_tripLoaded ? m_summary.harsh_brake_count : 0; }

double TripController::liveFuelUsedL() const
{
    return m_liveFuelUsedL;
}

double TripController::liveFuelEconomyLPer100Km() const
{
    if (m_distanceTravelledKm <= 0.0001)
        return 0.0;

    return (m_liveFuelUsedL / m_distanceTravelledKm) * 100.0;
}

double TripController::liveFuelEfficiencyPercent() const
{
    const double liveEconomy = liveFuelEconomyLPer100Km();
    if (liveEconomy <= 0.0001 || m_benchmarkFuelEconomyLPer100Km <= 0.0001)
        return 0.0;

    return (m_benchmarkFuelEconomyLPer100Km / liveEconomy) * 100.0;
}

double TripController::benchmarkFuelEconomyLPer100Km() const
{
    return m_benchmarkFuelEconomyLPer100Km;
}

bool TripController::aiLoaded() const
{
    return m_aiLoaded;
}

QString TripController::aiGuidance1() const {
    return m_aiGuidance1;
}

QString TripController::aiGuidance2() const {
    qDebug() << "polling...";
    return m_aiGuidance2;
}

QString TripController::aiGuidance3() const {
    return m_aiGuidance3;
}

void TripController::setBenchmarkFuelEconomyLPer100Km(double value)
{
    if (value <= 0.0)
        return;

    if (qFuzzyCompare(m_benchmarkFuelEconomyLPer100Km, value))
        return;

    m_benchmarkFuelEconomyLPer100Km = value;
    emit tripChanged();
}

void TripController::resetLiveState()
{
    m_replayTimer.stop();
    m_currentIndex = 0;
    m_journeyFinished = false;
    m_distanceTravelledKm = 0.0;
    m_timeTravelledSeconds = 0.0;
    m_liveSpeedKmh = 0.0;
    m_avgSpeedKmh = 0.0;
    m_liveFuelUsedL = 0.0;
}

void TripController::resetReplay()
{
    if (!m_tripLoaded || m_samples.empty())
        return;

    resetLiveState();
    if (!m_samples.empty())
        m_liveSpeedKmh = m_samples.front().speed;

    emit tripChanged();
}

void TripController::startReplay()
{
    if (!m_tripLoaded || m_samples.size() < 2)
        return;

    resetLiveState();
    m_liveSpeedKmh = m_samples.front().speed;
    m_replayTimer.start();
    emit tripChanged();
}

void TripController::loadTrip(const QString &path)
{
    m_replayTimer.stop();
    resetLiveState();
    m_samples.clear();
    m_tripLoaded = false;

    QString localPath = path.trimmed();
    if (localPath.isEmpty()) {
        emit errorOccurred("No CSV file path was provided.");
        return;
    }

    const QUrl url(localPath);
    if (url.isValid() && url.isLocalFile())
        localPath = url.toLocalFile();

    QFileInfo fileInfo(localPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        emit errorOccurred(QString("Selected file does not exist: %1").arg(localPath));
        return;
    }

    if (fileInfo.suffix().compare("csv", Qt::CaseInsensitive) != 0) {
        emit errorOccurred(QString("Selected file is not a CSV file: %1").arg(localPath));
        return;
    }

    try {
        CsvTripParser parser;
        m_samples = parser.parseFile(localPath);

        if (m_samples.empty()) {
            emit errorOccurred("The selected CSV file contains no usable samples.");
            return;
        }

        m_summary = parser.analyseTrip(m_samples);
        m_tripFileName = fileInfo.fileName();
        QString trafficCondition = extractTrafficConditions(localPath);
        m_tripLoaded = true;
        rebuildFuelEconomySeries(1.0);

        resetLiveState();
        m_liveSpeedKmh = m_samples.front().speed;

        emit tripChanged();

        QString modelDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/models";

        QString modelPath = modelDir + "/granite-4.0-micro-Q4_K_M.gguf";

        std::string prompt =
            "You are an AI that gives eco-driving tips.\n"
            "\n"
            "Rules:\n"
            "1. Output only driving tips.\n"
            "2. Each tip MUST start with a verb.\n"
            "3. Separate tips with a semicolon (;).\n"
            "4. Do NOT use quotes, markdown, bullet points, or numbering.\n"
            "5. Do NOT over-explain anything. Make sure the tips are precise.\n"
            "6. Write between 3 and 5 tips.\n"
            "7. Each tip must start with a verb, contain 8 to 14 words and describe a specific driving action\n"
            "8. Take the traffic condition listed below when giving advice.\n"
            "\n"
            "Correct examples:\n"
            "Look ahead to avoid harsh braking; Maintain a safe following distance to reduce braking; Avoid idling to save fuel\n"
            "\n"
            "Driving data:\n"
            "Harsh braking instances: " + std::to_string(m_summary.harsh_brake_count) + "\n"
                                                            "Fuel usage: " + std::to_string(m_summary.avg_l_per_100km) + " L/100km (benchmark 6.5)\n"
                                                          "High RPM events: " + std::to_string(m_summary.high_rpm_count) + "\n"
                                                         "Idle seconds: " + std::to_string(m_summary.idle_seconds) + "\n"
                                                        "Harsh acceleration events: " + std::to_string(m_summary.harsh_accel_count) + "\n"
                                                        "Harsh breaking events: " + std::to_string(m_summary.harsh_brake_count) + "\n"
                                                        "Trip Duration: " + std::to_string((m_summary.end_time-m_summary.start_time)) + " seconds" + "\n"
                                                        "Traffic condition: " + trafficCondition.toStdString() + "\n"
                                                       "\n"
                                                       "Output:";

        std::string output = m_aiController->query_granite(modelPath, prompt).toStdString();

        std::string temp;
        std::stringstream stringstream { output };
        std::vector<std::string> result;

        while (std::getline(stringstream, temp, ';')) {
            result.push_back(temp);
        }

        if (result.size() > 0) setAIGuidance1(result.at(0));
        if (result.size() > 1) setAIGuidance2(result.at(1));
        if (result.size() > 2) setAIGuidance3(result.at(2));
        setAILoaded(true);

        for (std::string advice : result) {
            qDebug() << result.size();
        }

        startReplay();
    } catch (const std::exception &ex) {
        emit errorOccurred(QString("Failed to load trip: %1").arg(ex.what()));
    } catch (...) {
        emit errorOccurred("Failed to load trip due to an unknown error.");
    }
}

void TripController::advanceReplay()
{
    if (m_samples.size() < 2 || m_currentIndex >= int(m_samples.size()) - 1) {
        m_replayTimer.stop();
        m_journeyFinished = true;
        emit tripChanged();
        return;
    }

    double consumed = 0.0;

    while (m_currentIndex < int(m_samples.size()) - 1 && consumed < kPlaybackRate) {
        const Sample &prev = m_samples[m_currentIndex];
        const Sample &next = m_samples[m_currentIndex + 1];

        const double rawDt = next.timestamp - prev.timestamp;
        const double dt = rawDt > 0.0 ? rawDt : 0.0;

        if (consumed + dt > kPlaybackRate)
            break;

        m_timeTravelledSeconds += dt;
        m_liveSpeedKmh = next.speed;
        m_distanceTravelledKm += (next.speed / 3.6) * dt / 1000.0;

        const double fuelLitresThisStep =
            (next.maf > 0.0) ? (next.maf / (14.7 * 745.0)) * dt : 0.0;
        m_liveFuelUsedL += fuelLitresThisStep;

        m_avgSpeedKmh = m_timeTravelledSeconds > 0.0
                            ? (m_distanceTravelledKm / (m_timeTravelledSeconds / 3600.0))
                            : 0.0;

        consumed += dt;
        ++m_currentIndex;
    }

    if (m_currentIndex >= int(m_samples.size()) - 1) {
        m_replayTimer.stop();
        m_journeyFinished = true;

        saveCurrentTripToHistory();
    }

    emit tripChanged();
}

void TripController::setAIController(AIController *controller) {
    m_aiController = controller;
}

QVariantList TripController::fuelEconomySeries() const
{
    return m_fuelEconomySeries;
}

void TripController::rebuildFuelEconomySeries(double windowSeconds)
{
    m_fuelEconomySeries.clear();

    if (!m_tripLoaded || m_samples.size() < 2 || windowSeconds <= 0.0) {
        emit fuelEconomySeriesChanged();
        return;
    }

    constexpr double kAfr = 14.7;
    constexpr double kFuelDensity = 745.0;
    constexpr double kMinCumulativeDistanceKm = 0.05; // 50 m before plotting first point

    double nextPointTime = m_samples.front().timestamp + windowSeconds;
    double totalFuelL = 0.0;
    double totalDistanceKm = 0.0;

    for (int i = 0; i < int(m_samples.size()) - 1; ++i) {
        const Sample &a = m_samples[i];
        const Sample &b = m_samples[i + 1];

        const double dt = b.timestamp - a.timestamp;
        if (dt <= 0.0)
            continue;

        const double speedMps = b.speed / 3.6;
        const double fuelFlowLps = (b.maf > 0.0) ? (b.maf / (kAfr * kFuelDensity)) : 0.0;

        totalDistanceKm += (speedMps * dt) / 1000.0;
        totalFuelL += fuelFlowLps * dt;

        if (b.timestamp >= nextPointTime) {
            nextPointTime += windowSeconds;

            // skip the unstable startup region
            if (totalDistanceKm < kMinCumulativeDistanceKm)
                continue;

            QVariantMap point;
            point["x"] = b.timestamp / 60.0; // minutes
            point["y"] = (totalFuelL / totalDistanceKm) * 100.0;
            m_fuelEconomySeries.append(point);
        }
    }

    emit fuelEconomySeriesChanged();
}

double TripController::driveScore() const
{
    if (!m_tripLoaded || m_samples.size() < 2)
        return 0.0;

    const double durationSeconds =
        std::max(1.0, m_summary.end_time - m_summary.start_time);
    const double tripMinutes = durationSeconds / 60.0;

    auto clamp100 = [](double v) {
        return std::max(0.0, std::min(100.0, v));
    };

    // 1) Fuel efficiency score
    // Lower L/100km is better. Benchmark already exists in the controller.
    const double actualEconomy = std::max(0.1, m_summary.avg_l_per_100km);
    const double benchmarkEconomy = std::max(0.1, m_benchmarkFuelEconomyLPer100Km);
    const double feScore = clamp100(100.0 * benchmarkEconomy / actualEconomy);

    // 2) Idle score
    // 0% idle => 100, 15%+ idle => 0
    const double idleRatio = m_summary.idle_seconds / durationSeconds;
    const double idleScore = clamp100(100.0 * (1.0 - idleRatio / 0.15));

    // 3) Harsh braking score
    // Normalize by trip length: events per 10 minutes
    const double brakesPer10Min =
        m_summary.harsh_brake_count / std::max(0.1, tripMinutes / 10.0);
    const double brakeScore = clamp100(100.0 * (1.0 - brakesPer10Min / 8.0));

    // 4) Harsh acceleration score
    const double accelsPer10Min =
        m_summary.harsh_accel_count / std::max(0.1, tripMinutes / 10.0);
    const double accelScore = clamp100(100.0 * (1.0 - accelsPer10Min / 8.0));

    // 5) Speed score
    // Penalize time above 105 km/h (~65 mph)
    double secondsAboveThreshold = 0.0;
    constexpr double speedThresholdKmh = 105.0;

    for (int i = 0; i < int(m_samples.size()) - 1; ++i) {
        const Sample &a = m_samples[i];
        const Sample &b = m_samples[i + 1];

        const double dt = b.timestamp - a.timestamp;
        if (dt <= 0.0)
            continue;

        if (b.speed > speedThresholdKmh)
            secondsAboveThreshold += dt;
    }

    const double speedRatio = secondsAboveThreshold / durationSeconds;
    const double speedScore = clamp100(100.0 * (1.0 - speedRatio / 0.20));

    const double finalScore =
        0.40 * feScore +
        0.20 * idleScore +
        0.15 * brakeScore +
        0.15 * accelScore +
        0.10 * speedScore;

    return std::round(finalScore);
}

QString TripController::tripHistoryDirectory() const
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath() + "/.FuelEfficiencyCoach";
    }

    return QDir(baseDir).filePath("TripHistory");
}

QVariantList TripController::tripHistory() const
{
    return m_filteredTripHistory;
}

bool TripController::saveCurrentTripToHistory()
{
    if (!m_tripLoaded) {
        emit errorOccurred("No trip is loaded, so there is nothing to save.");
        return false;
    }

    const QString historyDir = tripHistoryDirectory();
    if (!QDir().mkpath(historyDir)) {
        emit errorOccurred("Could not create the trip history directory.");
        return false;
    }

    const QString id = QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH-mm-ss-zzz");
    const QString filePath = QDir(historyDir).filePath(id + ".json");

    QJsonObject root;
    root["id"] = id;
    root["savedAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    root["tripFileName"] = m_tripFileName;
    root["benchmarkFuelEconomyLPer100Km"] = m_benchmarkFuelEconomyLPer100Km;
    root["score"] = driveScore();

    double efficiencyPercent = 0.0;
    if (m_summary.avg_l_per_100km > 0.0 && m_benchmarkFuelEconomyLPer100Km > 0.0) {
        efficiencyPercent = (m_benchmarkFuelEconomyLPer100Km / m_summary.avg_l_per_100km) * 100.0;
    }
    root["fuelEfficiencyPercent"] = efficiencyPercent;

    // Reuse the summary object you already have
    root["summary"] = m_summary.toJson();

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred(QString("Could not open trip history file for writing: %1").arg(filePath));
        return false;
    }

    const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(json) != json.size()) {
        emit errorOccurred("Failed while writing trip history file.");
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        emit errorOccurred("Failed to save trip history file.");
        return false;
    }

    loadTripHistory();
    return true;
}

void TripController::loadTripHistory()
{
    m_tripHistory.clear();

    const QString historyDir = tripHistoryDirectory();
    QDir dir(historyDir);

    if (!dir.exists()) {
        emit tripHistoryChanged();
        return;
    }

    const QFileInfoList files = dir.entryInfoList(
        QStringList() << "*.json",
        QDir::Files,
        QDir::Time // newest first
        );

    for (const QFileInfo &info : files) {
        QFile file(info.filePath());
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }

        const QByteArray data = file.readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(data);

        if (!doc.isObject()) {
            continue;
        }

        const QJsonObject root = doc.object();

        QVariantMap item;
        item["id"] = root.value("id").toString();
        item["savedAt"] = root.value("savedAt").toString();
        item["tripFileName"] = root.value("tripFileName").toString();
        item["benchmarkFuelEconomyLPer100Km"] = root.value("benchmarkFuelEconomyLPer100Km").toDouble();
        item["distanceKm"] = root.value("distanceKm").toDouble();
        item["score"] = root.value("score").toDouble();
        item["fuelEfficiencyPercent"] = root.value("fuelEfficiencyPercent").toDouble();

        const QJsonObject summary = root.value("summary").toObject();
        item["startTime"] = summary.value("start_time").toDouble();
        item["endTime"] = summary.value("end_time").toDouble();
        item["fuelUsedL"] = summary.value("fuel_est_l").toDouble();
        item["avgFuelRateLph"] = summary.value("avg_fuel_rate_lph").toDouble();
        item["avgFuelEconomyLPer100Km"] = summary.value("avg_l_per_100km").toDouble();
        item["idleSeconds"] = summary.value("idle_seconds").toDouble();
        item["highRpmSeconds"] = summary.value("high_rpm_seconds").toDouble();
        item["highRpmCount"] = summary.value("high_rpm_count").toInt();
        item["harshAccelCount"] = summary.value("harsh_accel_count").toInt();
        item["harshBrakeCount"] = summary.value("harsh_brake_count").toInt();
        item["throttleSpikeCount"] = summary.value("throttle_spike_count").toInt();
        item["fuelEstMethod"] = summary.value("fuel_est_method").toString();

        // handy display fields
        const double durationSeconds =
            summary.value("end_time").toDouble() - summary.value("start_time").toDouble();
        item["durationSeconds"] = durationSeconds;
        item["durationMinutes"] = durationSeconds / 60.0;

        m_tripHistory.append(item);
    }

    applySorting();
}

void TripController::applySorting()
{
    m_filteredTripHistory.clear();

    QList<QVariantMap> sortedTrips;

    for (const QVariant &tripVariant : std::as_const(m_tripHistory)) {
        sortedTrips.append(tripVariant.toMap());
    }

    std::sort(sortedTrips.begin(), sortedTrips.end(),
              [&](const QVariantMap &leftTrip, const QVariantMap &rightTrip) {

                  QDateTime leftDate =
                      QDateTime::fromString(leftTrip["savedAt"].toString(), Qt::ISODate);

                  QDateTime rightDate =
                      QDateTime::fromString(rightTrip["savedAt"].toString(), Qt::ISODate);

                  if (m_sortMode == 0) return leftDate > rightDate; // newest first
                  if (m_sortMode == 1) return leftDate < rightDate; // oldest first

                  return false;
              });

    for (const QVariantMap &trip : sortedTrips)
        m_filteredTripHistory.append(trip);

    emit tripHistoryChanged();
}

void TripController::setSortMode(int mode)
{
    m_sortMode = mode;
    applySorting();
}

QString TripController::extractTrafficConditions(const QString &path)
{
    QFileInfo fileInfo(path);
    QString baseName = fileInfo.completeBaseName();

    QStringList parts = baseName.split("_");

    if (parts.size() < 6)
        return "unknown";

    QString condition = parts[5].trimmed().toLower();

    if (condition == "stau")
        condition = "heavy traffic";
    else if (condition == "frei")
        condition = "free flowing traffic";
    else if (condition == "normal")
        condition = "normal traffic";

    return condition;
}
