#pragma once

/// <summary>
/// D-10 Satellite initial conditions: lon, lat (rad), alt (km), heading (rad).
/// Produced by ConstellationGenerator, consumed by SatelliteBody.
/// </summary>
struct SatelliteInitialConditions {
    double longitude_rad = 0.0;
    double latitude_rad = 0.0;
    double altitude_km = 0.0;
    double heading_rad = 0.0;
};