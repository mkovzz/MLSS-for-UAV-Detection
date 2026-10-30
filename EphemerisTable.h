#pragma once

#include <map>
#include <string>
#include "Ephemeris.h"

/// <summary>
/// D-02 Ephemeris table: every entity's ephemeris.
/// EphemerisTable: entity -> Ephemeris
/// Ephemeris: time -> position and velocity
/// UTC is derived at output.
/// Produced by OutputWriter, consumed by the Environment Model and the Run Data Store.
/// </summary>
struct EphemerisTable {
    /// entity id -> that entity's ephemeris
    std::map<std::string, Ephemeris> entities;
};