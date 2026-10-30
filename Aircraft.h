#pragma once

#include <string>
#include <vector>
#include "Ephemeris.h"
#include "Vector3d.h"

/// <summary>
/// One aircraft (not a singleton); the shared list is owned by AircraftReplay.
/// Not simulated: it replays a recorded flight path. Has an ID and its own ephemeris, in ECI like every other entity.
/// The path (time and Earth-fixed position per point) is converted from source units and lon/lat/alt once, at load.
/// Outside the path's time window the aircraft is inactive and contributes nothing. Velocity is not tracked (left zero).
/// </summary>
class Aircraft {
public:
    /// <summary>Loads and converts the flight path.</summary>
    /// <param name="id">Unique aircraft identifier.</param>
    /// <param name="flightPathFile">Flight path data file (CSV).</param>
    Aircraft(const std::string& id, const std::string& flightPathFile);

    /// <returns>true if the flight path was read successfully.</returns>
    bool isLoaded() const;

    /// <summary>
    /// If t is inside the active window, interpolates the position between the nearest recorded points before and after t and
    /// rotates it into ECI; otherwise marks the aircraft inactive.
    /// </summary>
    /// <param name="time_s">Simulation time [s since start].</param>
    void update(double time_s);

    /// <returns>true if the last update() fell inside the active window.</returns>
    bool isActive() const;

    /// <summary>Appends the current state to this aircraft's ephemeris if it is active. Called on storage steps only.</summary>
    void recordEphemeris();

    /// <returns>Aircraft identifier.</returns>
    const std::string& getId() const;

    /// <returns>Current position (km), ECI. Velocity is zero. Meaningful only while active.</returns>
    Ephemeris::State getState() const;

    /// <returns>The stored samples of this aircraft.</returns>
    const Ephemeris& getEphemeris() const;

private:
    /// One recorded point of the flight path.
    struct PathPoint {
        double time_s = 0.0;
        Vector3d earthFixedPosition_km;
    };

    std::string id_;
    std::vector<PathPoint> path_;
    bool loaded_ = false;
    bool active_ = false;
    Ephemeris::State state_;
    Ephemeris ephemeris_;
};