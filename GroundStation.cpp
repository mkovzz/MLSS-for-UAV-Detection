#include "GroundStation.h"

#include <limits>

#include "UnitConverter.h"

GroundStation::GroundStation(const std::string& id, const Vector3d& earthFixedPosition_km) : id_(id), earthFixedPosition_km_(earthFixedPosition_km) {}
void GroundStation::update(double time_s) { time_s_ = time_s; state_.position = UnitConverter::ecefToEci(earthFixedPosition_km_, time_s); state_.velocity = {}; }
void GroundStation::recordEphemeris() { ephemeris_.samples[time_s_] = state_; }
const std::string& GroundStation::getId() const { return id_; }
Ephemeris::State GroundStation::getState() const { return state_; }
const Ephemeris& GroundStation::getEphemeris() const { return ephemeris_; }

std::shared_ptr<GroundStation> GroundStation::getClosest(const std::vector<std::shared_ptr<GroundStation>>& stations, const Vector3d& point) {
    std::shared_ptr<GroundStation> closest;
    double bestDistance = std::numeric_limits<double>::max();
    for (const auto& station : stations) {
        const double distance = (station->getState().position - point).Dot(station->getState().position - point);
        if (distance < bestDistance) { bestDistance = distance; closest = station; }
    }
    return closest;
}
