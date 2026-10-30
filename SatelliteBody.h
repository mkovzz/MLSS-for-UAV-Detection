#pragma once

#include <string>
#include "EciState.h"
#include "SatelliteInitialConditions.h"

/// <summary>
/// FBD block "Satellite Body Instance". One instance per satellite (not a singleton); the shared list is owned by
/// ConstellationGenerator.
/// In: D-10 initial conditions (ConstellationGenerator), time (s) (called by ProduceEphemerisTable with the scheduler's time).
/// Out: D-12 state to OutputWriter; D-12 state and rejection reason to HealthMonitor.
/// </summary>
class SatelliteBody {
public:
    /// <param name="initial">D-10 initial conditions.</param>
    explicit SatelliteBody(const SatelliteInitialConditions& initial);

    /// <summary>Advances the satellite to the given time and sends its D-12 state and rejection reason to HealthMonitor.</summary>
    /// <param name="time_s">Simulation time [s].</param>
    void update(double time_s);

    /// <returns>D-12 state at the last update.</returns>
    EciState getState() const;

    /// <returns>Reason the satellite was rejected; empty if none.</returns>
    const std::string& getRejectionReason() const;

private:
    SatelliteInitialConditions initial_;
    EciState state_;
    std::string rejectionReason_;
};