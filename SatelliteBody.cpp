#include "SatelliteBody.h"

#include "HealthMonitor.h"
#include "OrbitalMathSolver.h"
#include "UnitConverter.h"

/// <summary>
/// Builds an orbital state from the satellite's initial geographic conditions.
/// </summary>
/// <param name="id">Unique satellite identifier.</param>
/// <param name="initial">Initial satellite conditions.</param>
SatelliteBody::SatelliteBody(const std::string& id, const SatelliteInitialConditions& initial) : id_(id) {
    const Vector3d position = UnitConverter::llaToEcef(initial.longitude_rad, initial.latitude_rad, initial.altitude_km);
    const Vector3d velocity = UnitConverter::headingToVelocity(position, initial.heading_rad, UnitConverter::circularOrbitSpeed(position.Magnitude()));
    elements_ = UnitConverter::stateToOrbitalElements(position, velocity);
    j2DriftRate_ = OrbitalMathSolver::j2NodalDriftRate(elements_);
    if (OrbitalMathSolver::periapsisAltitude(elements_) < 0.0) rejectionReason_ = "orbit intersects Earth";
    else if (OrbitalMathSolver::periapsisAltitude(elements_) < 200.0) rejectionReason_ = "orbit below minimum operating altitude";
}

/// <summary>
/// Propagates the orbit and reports any invalid-orbit rejection to HealthMonitor.
/// </summary>
/// <param name="time_s">Simulation time in seconds.</param>
void SatelliteBody::update(double time_s) {
    time_s_ = time_s;
    const BodyState state = OrbitalMathSolver::stateAtTime(elements_, j2DriftRate_, time_s);
    state_ = {state.position, state.velocity};
    if (HealthMonitor::Instance != nullptr) HealthMonitor::Instance->submitSatelliteState({id_, time_s, state.position, state.velocity}, rejectionReason_);
}

/// <summary>
/// Stores the current satellite state at the current simulation time.
/// </summary>
void SatelliteBody::recordEphemeris() { ephemeris_.samples[time_s_] = state_; }
/// <summary>
/// Returns the satellite identifier.
/// </summary>
/// <returns>The satellite identifier.</returns>
const std::string& SatelliteBody::getId() const { return id_; }
/// <summary>
/// Returns the current propagated state.
/// </summary>
/// <returns>The current position and velocity.</returns>
Ephemeris::State SatelliteBody::getState() const { return state_; }
/// <summary>
/// Returns all recorded satellite samples.
/// </summary>
/// <returns>The satellite ephemeris.</returns>
const Ephemeris& SatelliteBody::getEphemeris() const { return ephemeris_; }
/// <summary>
/// Returns the reason the initial orbit was rejected, if any.
/// </summary>
/// <returns>The rejection reason, or an empty string.</returns>
const std::string& SatelliteBody::getRejectionReason() const { return rejectionReason_; }
