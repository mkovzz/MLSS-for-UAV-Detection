#pragma once

#include <array>
#include <string>
#include <vector>

/// <summary>
/// D-02 Ephemeris table: per entity and per stored time, position (x,y,z) plus velocity (vx,vy,vz). UTC is derived at output.
/// Produced by OutputWriter, consumed by the Environment Model and the Run Data Store.
/// </summary>
struct EphemerisTable {
    struct Row {
        double time_s = 0.0;
        std::string entityId;
        std::array<double, 3> position{ 0.0, 0.0, 0.0 };
        std::array<double, 3> velocity{ 0.0, 0.0, 0.0 };
    };
    std::vector<Row> rows;
};