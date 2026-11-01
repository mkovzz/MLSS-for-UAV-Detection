#include "Aircraft.h"

#include <algorithm>
#include <fstream>
#include <sstream>

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

bool Aircraft::isLoaded() const { return loaded_; }

void Aircraft::update(double time_s) {
    time_s_ = time_s;
    active_ = loaded_ && time_s >= path_.front().time_s && time_s <= path_.back().time_s;
    if (!active_) return;

    auto after = std::lower_bound(path_.begin(), path_.end(), time_s, [](const PathPoint& point, double value) { return point.time_s < value; });
    if (after == path_.begin() || after == path_.end()) {
        state_.position = UnitConverter::ecefToEci((after == path_.end() ? path_.back() : path_.front()).earthFixedPosition_km, time_s);
    } else {
        const PathPoint& b = *after;
        const PathPoint& a = *(after - 1);
        const double fraction = (time_s - a.time_s) / (b.time_s - a.time_s);
        const Vector3d ecef = a.earthFixedPosition_km + (b.earthFixedPosition_km - a.earthFixedPosition_km) * fraction;
        state_.position = UnitConverter::ecefToEci(ecef, time_s);
    }
    state_.velocity = {};
}

bool Aircraft::isActive() const { return active_; }
void Aircraft::recordEphemeris() { if (active_) ephemeris_.samples[time_s_] = state_; }
const std::string& Aircraft::getId() const { return id_; }
Ephemeris::State Aircraft::getState() const { return state_; }
const Ephemeris& Aircraft::getEphemeris() const { return ephemeris_; }
