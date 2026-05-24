#pragma once
#include <vector>
#include <QString>
#include <QJsonObject>

struct Sample {
    double timestamp;
    double rpm;
    double speed;
    double maf;
    double accelPositionD;
    double accelPositionE;
};

struct Event {
    QString type;
    double start_time = 0.0;
    double end_time = 0.0;
    double score = 0.0;
    QJsonObject toJson() const;
};

struct TripSummary {
    QString trip_id;
    double start_time = 0.0;
    double end_time = 0.0;
    double fuel_est_l = 0.0;
    double avg_fuel_rate_lph = 0.0;
    double avg_l_per_100km = 0.0;
    double idle_seconds = 0.0;
    double high_rpm_seconds = 0.0;
    int high_rpm_count = 0;
    int harsh_accel_count = 0;
    int harsh_brake_count = 0;
    int throttle_spike_count = 0;
    QString fuel_est_method;
    std::vector<Event> events;

    QJsonObject toJson() const;
};

class CsvTripParser {
public:
    std::vector<Sample> parseFile(const QString& path);
    TripSummary analyseTrip(const std::vector<Sample>& samples);
};
