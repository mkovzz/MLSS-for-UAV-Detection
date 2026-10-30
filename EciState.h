#pragma once

#include <array>
#include <string>

/// <summary>
/// D-12 ECI state per entity at time t: position (km), velocity (km/s).
/// Produced by SatelliteBody, consumed by OutputWriter and HealthMonitor.
/// </summary>
struct EciState {
    std::string entityId;
    double time_s = 0.0;
    std::array<double, 3> position_km{ 0.0, 0.0, 0.0 };
    std::array<double, 3> velocity_kmps{ 0.0, 0.0, 0.0 };
};