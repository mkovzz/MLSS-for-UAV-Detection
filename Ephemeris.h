#pragma once

#include <map>
#include "Vector3d.h"

/// <summary>
/// The ephemeris of ONE entity: time (s) -> position and velocity, one sample per stored time step.
/// A building block of D-02 (see EphemerisTable). Time is in seconds since the scenario start, and samples are ordered by time.
/// </summary>
struct Ephemeris {
    /// Position (km) and velocity (km/s) of the entity at one stored time.
    struct State {
        Vector3d position;
        Vector3d velocity;
    };

    /// time (s) -> state at that time
    std::map<double, State> samples;
};