#pragma once

#include <string>
#include <vector>
#include "Vector3d.h"

/// <summary>
/// D-02 Ephemeris table: per entity and per stored time, position (x,y,z) plus velocity (vx,vy,vz). UTC is derived at output.
/// Produced by OutputWriter, consumed by the Environment Model and the Run Data Store.
/// </summary>
struct EphemerisTable {
    struct Row {
        double time_s = 0.0;
        std::string entityId;
        Vector3d position;
        Vector3d velocity;
    };
    std::vector<Row> rows;
};