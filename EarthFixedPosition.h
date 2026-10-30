#pragma once

#include <array>
#include <string>

/// <summary>
/// D-13 Aircraft and ground-station positions in the Earth-fixed frame (km).
/// Produced by GroundStation and AircraftReplay, consumed by OutputWriter.
/// </summary>
struct EarthFixedPosition {
    std::string entityId;
    double time_s = 0.0;
    std::array<double, 3> position_km{ 0.0, 0.0, 0.0 };
};