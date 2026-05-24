#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <vector>
#include <QVariantList>
#include "CsvTripParser.h"
#include "AIController.h"

class TripController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList tripHistory READ tripHistory NOTIFY tripHistoryChanged)
    Q_PROPERTY(bool tripLoaded READ tripLoaded NOTIFY tripChanged)
    Q_PROPERTY(QString tripFileName READ tripFileName NOTIFY tripChanged)

    Q_PROPERTY(double journeyProgress READ journeyProgress NOTIFY tripChanged)
    Q_PROPERTY(double distanceTravelledKm READ distanceTravelledKm NOTIFY tripChanged)
    Q_PROPERTY(double timeTravelledSeconds READ timeTravelledSeconds NOTIFY tripChanged)
    Q_PROPERTY(double liveSpeedKmh READ liveSpeedKmh NOTIFY tripChanged)
    Q_PROPERTY(double avgSpeedKmh READ avgSpeedKmh NOTIFY tripChanged)
    Q_PROPERTY(bool journeyFinished READ journeyFinished NOTIFY tripChanged)

    Q_PROPERTY(double fuelUsedL READ fuelUsedL NOTIFY tripChanged)
    Q_PROPERTY(double avgFuelRateLph READ avgFuelRateLph NOTIFY tripChanged)
    Q_PROPERTY(double avgFuelEconomyLPer100Km READ avgFuelEconomyLPer100Km NOTIFY tripChanged)
    Q_PROPERTY(double idleSeconds READ idleSeconds NOTIFY tripChanged)
    Q_PROPERTY(int highRpmCount READ highRpmCount NOTIFY tripChanged)
    Q_PROPERTY(int harshAccelCount READ harshAccelCount NOTIFY tripChanged)
    Q_PROPERTY(int harshBrakeCount READ harshBrakeCount NOTIFY tripChanged)

    Q_PROPERTY(double liveFuelUsedL READ liveFuelUsedL NOTIFY tripChanged)
    Q_PROPERTY(double liveFuelEconomyLPer100Km READ liveFuelEconomyLPer100Km NOTIFY tripChanged)
    Q_PROPERTY(double liveFuelEfficiencyPercent READ liveFuelEfficiencyPercent NOTIFY tripChanged)
    Q_PROPERTY(double benchmarkFuelEconomyLPer100Km READ benchmarkFuelEconomyLPer100Km WRITE setBenchmarkFuelEconomyLPer100Km NOTIFY tripChanged)

    Q_PROPERTY(bool aiLoaded READ aiLoaded NOTIFY tripChanged)
    Q_PROPERTY(QString aiGuidance1 READ aiGuidance1 NOTIFY tripChanged)
    Q_PROPERTY(QString aiGuidance2 READ aiGuidance2 NOTIFY tripChanged)
    Q_PROPERTY(QString aiGuidance3 READ aiGuidance3 NOTIFY tripChanged)
    Q_PROPERTY(QVariantList fuelEconomySeries READ fuelEconomySeries NOTIFY fuelEconomySeriesChanged)
    Q_PROPERTY(double driveScore READ driveScore NOTIFY tripChanged)

public:
    QVariantList fuelEconomySeries() const;
    Q_INVOKABLE void rebuildFuelEconomySeries(double windowSeconds = 1.0);

signals:
    void fuelEconomySeriesChanged();

public:
    explicit TripController(QObject *parent = nullptr);

    QVariantList tripHistory() const;

    bool tripLoaded() const;
    QString tripFileName() const;

    double driveScore() const;
    double journeyProgress() const;
    double distanceTravelledKm() const;
    double timeTravelledSeconds() const;
    double liveSpeedKmh() const;
    double avgSpeedKmh() const;
    bool journeyFinished() const;

    double fuelUsedL() const;
    double avgFuelRateLph() const;
    double avgFuelEconomyLPer100Km() const;
    double idleSeconds() const;
    int highRpmCount() const;
    int harshAccelCount() const;
    int harshBrakeCount() const;

    double liveFuelUsedL() const;
    double liveFuelEconomyLPer100Km() const;
    double liveFuelEfficiencyPercent() const;
    double benchmarkFuelEconomyLPer100Km() const;
    void setBenchmarkFuelEconomyLPer100Km(double value);
    void setAIController(AIController *controller);

    bool aiLoaded() const;
    QString aiGuidance1() const;
    QString aiGuidance2() const;
    QString aiGuidance3() const;

    void setAILoaded(bool state) { m_aiLoaded = state; }
    void setAIGuidance1(std::string guidance) { m_aiGuidance1 = QString::fromStdString(guidance); }
    void setAIGuidance2(std::string guidance) { m_aiGuidance2 = QString::fromStdString(guidance); }
    void setAIGuidance3(std::string guidance) { m_aiGuidance3 = QString::fromStdString(guidance); }

    Q_INVOKABLE void loadTrip(const QString &path);
    Q_INVOKABLE void startReplay();
    Q_INVOKABLE void resetReplay();
    Q_INVOKABLE void setSortMode(int mode);
    Q_INVOKABLE bool saveCurrentTripToHistory();
    Q_INVOKABLE void loadTripHistory();
    QString extractTrafficConditions(const QString &path);


signals:
    void tripChanged();
    void errorOccurred(const QString &message);
    void tripHistoryChanged();

private slots:
    void advanceReplay();

private:
    void applySorting();
    QVariantList m_filteredTripHistory;
    int m_sortMode = 0;
    void resetLiveState();
    QVariantList m_fuelEconomySeries;
    QVariantList m_tripHistory;
    QString tripHistoryDirectory() const;

    bool m_tripLoaded = false;
    QString m_tripFileName;
    TripSummary m_summary {};
    std::vector<Sample> m_samples;

    QTimer m_replayTimer;
    int m_currentIndex = 0;
    bool m_journeyFinished = false;

    double m_distanceTravelledKm = 0.0;
    double m_timeTravelledSeconds = 0.0;
    double m_liveSpeedKmh = 0.0;
    double m_avgSpeedKmh = 0.0;

    double m_liveFuelUsedL = 0.0;
    double m_benchmarkFuelEconomyLPer100Km = 6.5;

    bool m_aiLoaded = false;
    QString m_aiGuidance1 = "Keep a safe distance to the car in front to prevent harsh braking.";
    QString m_aiGuidance2 = "Avoid idling to reduce uneccessary fuel usage.";
    QString m_aiGuidance3 = "Change to a higher gear where possible.";

    AIController *m_aiController = nullptr;

    static constexpr double kPlaybackRate =  15.0;
};
