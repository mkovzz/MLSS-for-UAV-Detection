#pragma once

#include <string>
#include "Vector3d.h"

/// <summary>
/// D-12 ECI state per entity at time t: position (km), velocity (km/s).
/// Produced by SatelliteBody, consumed by OutputWriter and HealthMonitor.
/// </summary>
struct EciState {
    std::string entityId;
    double time_s = 0.0;
    Vector3d position_km;
    Vector3d velocity_kmps;
};