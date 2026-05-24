#include "CsvTripParser.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <cmath>
#include <QJsonArray>
#include <algorithm>

// Event constants
static const double RPM_THRESHOLD = 2900.0;
static const double RPM_MIN_DURATION = 0.5;
static const double HARSH_ACCEL_THRESHOLD = 1.1;
static const double HARSH_BRAKE_THRESHOLD = -1.1;
static const double ACCEL_MIN_DURATION = 0.3;
static const double THROTTLE_THRESHOLD = 4.0;
static const double AFR = 14.7;
static const double FUEL_DENSITY_G_PER_L = 745.0;
static const int FREQUENT_HIGH_RPM_COUNT = 3;

QJsonObject Event::toJson() const {
    QJsonObject obj;
    obj["type"] = type;
    obj["start_time"] = start_time;
    obj["end_time"] = end_time;
    obj["score"] = score;
    return obj;
}

QJsonObject TripSummary::toJson() const {
    QJsonObject obj;
    obj["trip_id"] = trip_id;
    obj["start_time"] = start_time;
    obj["end_time"] = end_time;
    obj["fuel_est_l"] = fuel_est_l;
    obj["avg_fuel_rate_lph"] = avg_fuel_rate_lph;
    obj["avg_l_per_100km"] = avg_l_per_100km;
    obj["idle_seconds"] = idle_seconds;
    obj["high_rpm_seconds"] = high_rpm_seconds;
    obj["high_rpm_count"] = high_rpm_count;
    obj["harsh_accel_count"] = harsh_accel_count;
    obj["harsh_brake_count"] = harsh_brake_count;
    obj["throttle_spike_count"] = throttle_spike_count;
    obj["fuel_est_method"] = fuel_est_method;

    QJsonArray eventArray;
    for (const auto& e : events) {
        eventArray.append(e.toJson());
    }
    obj["events"] = eventArray;

    return obj;
}

std::vector<Sample> CsvTripParser::parseFile(const QString& path) {
    std::vector<Sample> samples;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return samples;
    }

    QTextStream in(&file);

    if (!in.atEnd()) {
        in.readLine(); // skip header
    }

    QTime baseTime;
    bool haveBaseTime = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }

        QStringList parts = line.split(",");

        if (parts.size() < 11) {
            continue;
        }

        for (QString &p : parts) {
            p = p.trimmed();
            if (p.startsWith('"') && p.endsWith('"') && p.size() >= 2) {
                p = p.mid(1, p.size() - 2);
            }
        }

        Sample s{};

        bool okTs = false, okRpm = false, okSpeed = false;
        bool okMaf = false, okApd = false, okApe = false;

        QTime t = QTime::fromString(parts[0], "HH:mm:ss.zzz");
        if (t.isValid()) {
            if (!haveBaseTime) {
                baseTime = t;
                haveBaseTime = true;
            }
            s.timestamp = baseTime.msecsTo(t) / 1000.0;
            okTs = true;
        }

        s.rpm = parts[3].toDouble(&okRpm);
        s.speed = parts[4].toDouble(&okSpeed);
        s.maf = parts[6].toDouble(&okMaf);
        s.accelPositionD = parts[9].toDouble(&okApd);
        s.accelPositionE = parts[10].toDouble(&okApe);

        if (!(okTs && okRpm && okSpeed)) {
            continue;
        }

        if (!okMaf) s.maf = 0.0;
        if (!okApd) s.accelPositionD = 0.0;
        if (!okApe) s.accelPositionE = 0.0;

        samples.push_back(s);
    }

    return samples;
}

TripSummary CsvTripParser::analyseTrip(const std::vector<Sample>& samples) {
    TripSummary summary;

    if (samples.empty()) {
        return summary;
    }

    summary.start_time = samples.front().timestamp;
    summary.end_time = samples.back().timestamp;
    summary.fuel_est_method = "MAF/AFR(14.7)/Density(745g/L)";

    std::vector<Event> events;

    // high rpm state
    bool rpmActive = false;
    double rpmStart = -1.0;
    int rpmSampleCount = 0;

    // harsh accel/decel state
    bool accelActive = false;
    double accelStart = -1.0;
    int accelSampleCount = 0;

    bool brakeActive = false;
    double brakeStart = -1.0;
    int brakeSampleCount = 0;

    // previous sample state
    bool hasPrev = false;
    double prevSpeedMps = NAN;
    double prevTime = NAN;

    // summary accumulators
    double totalFuelL = 0.0;
    double totalTimeS = 0.0;
    double distanceKm = 0.0;

    double accelPeak = 0.0;
    double brakePeak = 0.0;

    for (const auto& s : samples) {
        double dt = 0.0;

        if (hasPrev) {
            dt = s.timestamp - prevTime;
            if (dt <= 0.0) {
                prevTime = s.timestamp;
                prevSpeedMps = s.speed / 3.6;
                continue;
            }
        }

        if (hasPrev && dt > 0.0) {
            totalTimeS += dt;

            double speedMpsNow = s.speed / 3.6;
            distanceKm += (speedMpsNow * dt) / 1000.0;

            if (speedMpsNow < 0.5) {
                summary.idle_seconds += dt;
            }

            if (s.maf > 0.0) {
                double fuelFlowLps = s.maf / (AFR * FUEL_DENSITY_G_PER_L);
                totalFuelL += fuelFlowLps * dt;
            }
        }

        // --- HIGH RPM ---
        if (s.rpm > RPM_THRESHOLD) {
            rpmSampleCount++;
            if (!rpmActive) {
                if (rpmStart < 0.0) rpmStart = s.timestamp - dt * (rpmSampleCount - 1);
            }
        } else {
            if (rpmStart >= 0.0) {
                double eventEnd = prevTime;
                double dur = eventEnd - rpmStart;
                if (dur >= RPM_MIN_DURATION) {
                    Event e;
                    e.type = "HighRPM_Event";
                    e.start_time = rpmStart;
                    e.end_time = eventEnd;
                    e.score = dur;
                    events.push_back(e);
                    summary.high_rpm_count++;
                    summary.high_rpm_seconds += dur;
                }
            }
            rpmStart = -1.0;
            rpmSampleCount = 0;
            rpmActive = false;
        }

        if (s.rpm > RPM_THRESHOLD && rpmStart >= 0.0) {
            double dur = s.timestamp - rpmStart;
            if (!rpmActive && dur >= RPM_MIN_DURATION) {
                rpmActive = true;
            }
        }

        // --- HARSH ACCEL / BRAKE detection using speed derivative ---
        double speedMps = s.speed / 3.6;

        if (hasPrev && dt > 0.0) {
            double accel = (speedMps - prevSpeedMps) / dt;

            // harsh acceleration
            if (accel > HARSH_ACCEL_THRESHOLD && s.accelPositionD > THROTTLE_THRESHOLD) {
                accelSampleCount++;
                if (!accelActive) {
                    if (accelStart < 0.0) accelStart = prevTime;
                    accelPeak = accel;
                    double dur = s.timestamp - accelStart;
                    if (dur >= ACCEL_MIN_DURATION && !accelActive) {
                        accelActive = true;
                    }
                } else {
                    accelPeak = std::max(accelPeak, accel);
                }
            } else {
                if (accelStart >= 0.0) {
                    double eventEnd = prevTime;
                    double dur = eventEnd - accelStart;
                    if (dur >= ACCEL_MIN_DURATION) {
                        Event e;
                        e.type = "HarshAccel";
                        e.start_time = accelStart;
                        e.end_time = eventEnd;
                        e.score = accelPeak;
                        events.push_back(e);
                        summary.harsh_accel_count++;
                    }
                }
                accelStart = -1.0;
                accelSampleCount = 0;
                accelActive = false;
                accelPeak = 0.0;
            }

            // harsh braking
            if (accel < HARSH_BRAKE_THRESHOLD) {
                brakeSampleCount++;
                if (!brakeActive) {
                    if (brakeStart < 0.0) brakeStart = prevTime;
                    brakePeak = accel;
                    double dur = s.timestamp - brakeStart;
                    if (dur >= ACCEL_MIN_DURATION && !brakeActive) {
                        brakeActive = true;
                    }
                } else {
                    brakePeak = std::min(brakePeak, accel);
                }
            } else {
                if (brakeStart >= 0.0) {
                    double eventEnd = prevTime;
                    double dur = eventEnd - brakeStart;
                    if (dur >= ACCEL_MIN_DURATION) {
                        Event e;
                        e.type = "HarshBrake";
                        e.start_time = brakeStart;
                        e.end_time = eventEnd;
                        e.score = std::abs(brakePeak);
                        events.push_back(e);
                        summary.harsh_brake_count++;
                    }
                }
                brakeStart = -1.0;
                brakeSampleCount = 0;
                brakeActive = false;
                brakePeak = 0.0;
            }
        }

        prevTime = s.timestamp;
        prevSpeedMps = speedMps;
        hasPrev = true;
    }

    // close open events at EOF

    if (rpmStart >= 0.0) {
        double dur = samples.back().timestamp - rpmStart;
        if (dur >= RPM_MIN_DURATION) {
            Event e;
            e.type = "HighRPM_Event";
            e.start_time = rpmStart;
            e.end_time = samples.back().timestamp;
            e.score = dur;
            events.push_back(e);
            summary.high_rpm_count++;
            summary.high_rpm_seconds += dur;
        }
    }

    if (accelStart >= 0.0) {
        double dur = samples.back().timestamp - accelStart;
        if (dur >= ACCEL_MIN_DURATION) {
            Event e;
            e.type = "HarshAccel";
            e.start_time = accelStart;
            e.end_time = samples.back().timestamp;
            e.score = accelPeak;
            events.push_back(e);
            summary.harsh_accel_count++;
        }
    }

    if (brakeStart >= 0.0) {
        double dur = samples.back().timestamp - brakeStart;
        if (dur >= ACCEL_MIN_DURATION) {
            Event e;
            e.type = "HarshBrake";
            e.start_time = brakeStart;
            e.end_time = samples.back().timestamp;
            e.score = std::abs(brakePeak);
            events.push_back(e);
            summary.harsh_brake_count++;
        }
    }

    summary.fuel_est_l = totalFuelL;

    if (totalTimeS > 0.0) {
        summary.avg_fuel_rate_lph = (totalFuelL / totalTimeS) * 3600.0;
    }

    if (distanceKm > 0.0) {
        summary.avg_l_per_100km = (totalFuelL / distanceKm) * 100.0;
    }

    if (summary.high_rpm_count >= FREQUENT_HIGH_RPM_COUNT) {
        summary.throttle_spike_count = 1;
    }

    summary.events = events;
    return summary;
}
