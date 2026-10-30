#pragma once

#include <string>
#include "Ephemeris.h"
#include "Vector3d.h"

/// <summary>
/// FBD block "Ground Station". One instance per station (not a singleton); the shared list is owned by ConfigIntake.
/// Fixed to the Earth, with an ID and its own ephemeris. Its Earth-fixed position is given once (converted from lon/lat/alt at load),
/// and rotated into ECI at each time t so every ephemeris in the engine shares one frame. Velocity is not tracked (left zero).
/// In: Position (ConfigIntake), time (s). Out: ephemeris to OutputWriter.
/// </summary>
class GroundStation {
public:
    /// <param name="id">Unique station identifier.</param>
    /// <param name="earthFixedPosition_km">Position in the Earth-fixed frame [km].</param>
    GroundStation(const std::string& id, const Vector3d& earthFixedPosition_km);

    /// <summary>Computes the station's ECI position at time t.</summary>
    /// <param name="time_s">Simulation time [s since start].</param>
    void update(double time_s);

    /// <summary>Appends the current state (as of the last update()) to this station's ephemeris. Called on storage steps only.</summary>
    void recordEphemeris();

    /// <returns>Station identifier.</returns>
    const std::string& getId() const;

    /// <returns>Current position (km), ECI. Velocity is zero.</returns>
    Ephemeris::State getState() const;

    /// <returns>The stored samples of this station.</returns>
    const Ephemeris& getEphemeris() const;

private:
    std::string id_;
    Vector3d earthFixedPosition_km_;
    double time_s_ = 0.0;
    Ephemeris::State state_;
    Ephemeris ephemeris_;
};