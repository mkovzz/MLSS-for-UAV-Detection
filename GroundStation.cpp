#include "GroundStation.h"

#include <limits>

#include "UnitConverter.h"

/// <summary>
/// Creates a ground station at a fixed ECEF position.
/// </summary>
/// <param name="id">Unique ground-station identifier.</param>
/// <param name="earthFixedPosition_km">ECEF position in kilometers.</param>
GroundStation::GroundStation(const std::string& id, const Vector3d& earthFixedPosition_km) : id_(id), earthFixedPosition_km_(earthFixedPosition_km) {}
/// <summary>
/// Updates the station's inertial position; a fixed station has zero local velocity here.
/// </summary>
/// <param name="time_s">Simulation time in seconds.</param>
void GroundStation::update(double time_s) { time_s_ = time_s; state_.position = UnitConverter::ecefToEci(earthFixedPosition_km_, time_s); state_.velocity = {}; }
/// <summary>
/// Stores the current station state.
/// </summary>
void GroundStation::recordEphemeris() { ephemeris_.samples[time_s_] = state_; }
/// <summary>
/// Returns the station identifier.
/// </summary>
/// <returns>The station identifier.</returns>
const std::string& GroundStation::getId() const { return id_; }
/// <summary>
/// Returns the current station state.
/// </summary>
/// <returns>The current position and velocity.</returns>
Ephemeris::State GroundStation::getState() const { return state_; }
/// <summary>
/// Returns all recorded station samples.
/// </summary>
/// <returns>The station ephemeris.</returns>
const Ephemeris& GroundStation::getEphemeris() const { return ephemeris_; }

/// <summary>
/// Finds the station with the smallest squared distance from a point.
/// </summary>
/// <param name="stations">Stations to search.</param>
/// <param name="point">Point used for the distance comparison.</param>
/// <returns>The closest station, or an empty pointer when the collection is empty.</returns>
std::shared_ptr<GroundStation> GroundStation::getClosest(const std::vector<std::shared_ptr<GroundStation>>& stations, const Vector3d& point) {
    std::shared_ptr<GroundStation> closest;
    double bestDistance = std::numeric_limits<double>::max();
    for (const auto& station : stations) {
        const double distance = (station->getState().position - point).Dot(station->getState().position - point);
        if (distance < bestDistance) { bestDistance = distance; closest = station; }
    }
    return closest;
}
