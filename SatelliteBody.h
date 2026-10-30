#pragma once

#include <string>
#include "EciState.h"
#include "Ephemeris.h"
#include "SatelliteInitialConditions.h"
#include "orbital_elements.h"

/// <summary>
/// FBD block "Satellite Body Instance". One instance per satellite (not a singleton); the shared list is owned by
/// ConstellationGenerator.
/// A point mass (position and velocity, no mass variable) with an ID and its own ephemeris. Its orbit is described by six
/// orbital elements fixed at t = 0 (the node angle drifts at the J2 rate); the state at any time t is computed directly from
/// them by OrbitalMathSolver. All states are in ECI.
/// In: D-10 initial conditions (via ConstellationGenerator), time (s) (ProduceEphemerisTable() passes the scheduler's time).
/// Out: D-12 state and rejection reason to HealthMonitor; ephemeris to OutputWriter.
/// </summary>
class SatelliteBody {
public:
    /// <summary>
    /// Converts D-10 to a position and velocity (always the circular-orbit speed, so eccentricity and argument of perigee are 0),
    /// then to the six orbital elements, and calculates the J2 drift rate. Runs the early-rejection checks (orbit dipping below
    /// about 200 km altitude, or intersecting Earth); a rejected body keeps a reason in getRejectionReason() instead of failing.
    /// </summary>
    /// <param name="id">Unique satellite identifier.</param>
    /// <param name="initial">D-10 initial conditions.</param>
    SatelliteBody(const std::string& id, const SatelliteInitialConditions& initial);

    /// <summary>Moves the point mass to time t and reports the resulting D-12 state and rejection reason to HealthMonitor.</summary>
    /// <param name="time_s">Simulation time [s since start].</param>
    void update(double time_s);

    /// <summary>Appends the current state (as of the last update()) to this satellite's ephemeris. Called on storage steps only.</summary>
    void recordEphemeris();

    /// <returns>Satellite identifier.</returns>
    const std::string& getId() const;

    /// <returns>Current position (km) and velocity (km/s), ECI.</returns>
    Ephemeris::State getState() const;

    /// <returns>The stored samples of this satellite.</returns>
    const Ephemeris& getEphemeris() const;

    /// <returns>Reason the satellite was rejected; empty if none.</returns>
    const std::string& getRejectionReason() const;

private:
    std::string id_;
    OrbitalElements elements_;        // at t = 0
    double j2DriftRate_ = 0.0;        // [rad/s]
    double time_s_ = 0.0;
    Ephemeris::State state_;
    Ephemeris ephemeris_;
    std::string rejectionReason_;
};