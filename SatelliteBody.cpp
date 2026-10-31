#include "SatelliteBody.h"

#include "HealthMonitor.h"
#include "OrbitalMathSolver.h"
#include "UnitConverter.h"

SatelliteBody::SatelliteBody(const std::string& id, const SatelliteInitialConditions& initial) : id_(id) {
    const Vector3d position = UnitConverter::llaToEcef(initial.longitude_rad, initial.latitude_rad, initial.altitude_km);
    const Vector3d velocity = UnitConverter::headingToVelocity(position, initial.heading_rad, UnitConverter::circularOrbitSpeed(position.Magnitude()));
    elements_ = UnitConverter::stateToOrbitalElements(position, velocity);
    j2DriftRate_ = OrbitalMathSolver::j2NodalDriftRate(elements_);
    if (OrbitalMathSolver::periapsisAltitude(elements_) < 0.0) rejectionReason_ = "orbit intersects Earth";
    else if (OrbitalMathSolver::periapsisAltitude(elements_) < 200.0) rejectionReason_ = "orbit below minimum operating altitude";
}

void SatelliteBody::update(double time_s) {
    time_s_ = time_s;
    const BodyState state = OrbitalMathSolver::stateAtTime(elements_, j2DriftRate_, time_s);
    state_ = {state.position, state.velocity};
    if (HealthMonitor::Instance != nullptr) HealthMonitor::Instance->submitSatelliteState({id_, time_s, state.position, state.velocity}, rejectionReason_);
}

void SatelliteBody::recordEphemeris() { ephemeris_.samples[time_s_] = state_; }
const std::string& SatelliteBody::getId() const { return id_; }
Ephemeris::State SatelliteBody::getState() const { return state_; }
const Ephemeris& SatelliteBody::getEphemeris() const { return ephemeris_; }
const std::string& SatelliteBody::getRejectionReason() const { return rejectionReason_; }
