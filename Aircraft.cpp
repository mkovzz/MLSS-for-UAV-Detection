#include "Aircraft.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "Earth.h"
#include "UnitConverter.h"

Aircraft::Aircraft(const std::string& id, const std::string& file) : id_(id) {
    std::ifstream input(file);
    std::string line;
    while (std::getline(input, line)) {
        std::stringstream stream(line);
        PathPoint point;
        char comma;
        if (stream >> point.time_s >> comma >> point.earthFixedPosition_km.x >> comma >> point.earthFixedPosition_km.y >> comma >> point.earthFixedPosition_km.z) path_.push_back(point);
    }
    std::sort(path_.begin(), path_.end(), [](const PathPoint& a, const PathPoint& b) { return a.time_s < b.time_s; });
    loaded_ = !path_.empty();
}

Aircraft::Aircraft(const std::string& id, const std::vector<PathPoint>& path) : id_(id), path_(path) {
    std::sort(path_.begin(), path_.end(), [](const PathPoint& a, const PathPoint& b) { return a.time_s < b.time_s; });
    loaded_ = !path_.empty();
}

bool Aircraft::isLoaded() const { return loaded_; }

void Aircraft::update(double time_s) {
    time_s_ = time_s;
    active_ = loaded_ && time_s >= path_.front().time_s && time_s <= path_.back().time_s;
    if (!active_) {
        state_ = {};
        return;
    }

    auto after = std::lower_bound(path_.begin(), path_.end(), time_s,
        [](const PathPoint& point, double value) { return point.time_s < value; });
    const PathPoint* a = nullptr;
    const PathPoint* b = nullptr;
    if (after == path_.begin()) {
        a = &path_.front();
        b = path_.size() > 1 ? &path_[1] : a;
    }
    else if (after == path_.end()) {
        b = &path_.back();
        a = path_.size() > 1 ? &path_[path_.size() - 2] : b;
    }
    else {
        a = &*(after - 1);
        b = &*after;
    }

    Vector3d ecef = a->earthFixedPosition_km;
    Vector3d ecefVelocity{};
    const double interval = b->time_s - a->time_s;
    if (interval > 0.0) {
        const double fraction = std::clamp((time_s - a->time_s) / interval, 0.0, 1.0);
        ecef = a->earthFixedPosition_km + (b->earthFixedPosition_km - a->earthFixedPosition_km) * fraction;
        ecefVelocity = (b->earthFixedPosition_km - a->earthFixedPosition_km) / interval;
    }

    state_.position = UnitConverter::ecefToEci(ecef, time_s);
    const Vector3d eciPathVelocity = UnitConverter::ecefToEci(ecefVelocity, time_s);
    const Vector3d earthRotationVelocity{
        -Earth::rotation_rate * state_.position.y,
        Earth::rotation_rate * state_.position.x,
        0.0};
    state_.velocity = eciPathVelocity + earthRotationVelocity;
}

bool Aircraft::isActive() const { return active_; }
void Aircraft::recordEphemeris() { ephemeris_.samples[time_s_] = state_; }
const std::string& Aircraft::getId() const { return id_; }
Ephemeris::State Aircraft::getState() const { return state_; }
const Ephemeris& Aircraft::getEphemeris() const { return ephemeris_; }
