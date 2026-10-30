#pragma once

#include "EarthFixedPosition.h"

/// <summary>
/// FBD block "Ground Station". One instance per station (not a singleton); the shared list is owned by ConfigIntake.
/// In: Position (ConfigIntake). Out: D-13 to OutputWriter.
/// </summary>
class GroundStation {
public:
    /// <param name="position">Earth-fixed position given by ConfigIntake.</param>
    explicit GroundStation(const EarthFixedPosition& position);

    /// <returns>D-13 position of this station.</returns>
    EarthFixedPosition getPosition() const;

private:
    EarthFixedPosition position_;
};