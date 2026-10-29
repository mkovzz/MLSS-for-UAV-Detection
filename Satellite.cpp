#include <cmath>
#include <iostream>
#include <string>

#include "Earth.h"
#include "Satellite.h"
#include "UnitConverter.h"
#include "Vector3d.h"

namespace {
	constexpr double kMinOperatingAltitudeKm = 200.0;
}

int Satellite::nextId() {
	static int id = 1;
	return id++;
}

void Satellite::computeJ2Drift() {
	const double a = elements_.a;
	const double e = elements_.e;
	const double n = std::sqrt(Earth::mu / (a * a * a));
	const double p = a * (1.0 - e * e);
	const double re_over_p = Earth::equatorial_radius / p;
	raan_dot_ = -1.5 * n * Earth::j2 * re_over_p * re_over_p * std::cos(elements_.i);
}

void Satellite::runEarlyRejection() {
	valid_ = true;
	rejection_reason_.clear();
	const double periapsis = elements_.a * (1.0 - elements_.e);
	if (periapsis < Earth::equatorial_radius) {
		valid_ = false;
		rejection_reason_ = "orbit intersects Earth";
		return;
	}
	if (periapsis < Earth::equatorial_radius + kMinOperatingAltitudeKm) {
		valid_ = false;
		rejection_reason_ = "orbit dips below about 200 km altitude";
	}
}

void Satellite::initializeFromElements(const OrbitalElements& elements) {
	elements_ = elements;
	elements_.e = 0.0;
	elements_.w = 0.0;
	mean_anomaly0_ = UnitConverter::meanAnomalyFromTrue(elements_.v, elements_.e);
	const BodyState s = UnitConverter::orbitalElementsToState(elements_);
	epoch_state_.position = s.position;
	epoch_state_.velocity = s.velocity;
	computeJ2Drift();
	runEarlyRejection();
}

void Satellite::initializeFromState(const Vector3d& position, const Vector3d& velocity) {
	elements_ = UnitConverter::stateToOrbitalElements(position, velocity);
	initializeFromElements(elements_);
}

/**
* @brief Default Constructor
* @return new satellite object with zero-initialized values
*/
Satellite::Satellite() : SatelliteID(nextId()) {
}

/**
	* @brief Constructor for satellite generation
	* @return new satellite object with initialized values based on a random seed from the run controller
	*/
Satellite::Satellite(Vector3d start_pos, Vector3d start_vel) : SatelliteID(nextId()) {
	initializeFromState(start_pos, start_vel);
}

Satellite::Satellite(double longitude, double latitude, double altitude, double heading)
	: SatelliteID(nextId()) {
	const Vector3d position = UnitConverter::llaToEcef(longitude, latitude, altitude);
	const double speed = UnitConverter::circularOrbitSpeed(position.Magnitude());
	const Vector3d velocity = UnitConverter::headingToVelocity(position, heading, speed);
	initializeFromState(position, velocity);
}

Satellite::Satellite(const OrbitalElements& elements) : SatelliteID(nextId()) {
	initializeFromElements(elements);
}

/**
* @brief gets the position of the satellite
* @param sat: passes the satellite object by reference
* @return list of the x, y, and z satellite position components
*/
Vector3d Satellite::getSatellitePosition() const {
	return epoch_state_.position;
}

/**
* @brief gets the velocity of the satellite
* @param sat: passes the satellite object by reference
* @return list of the x, y, and z satellite velocity components
*/
Vector3d Satellite::getSatelliteVelocity() const {
	return epoch_state_.velocity;
}

double Satellite::solveKeplerforE(double meanAnomaly, double e) {
	const double M = UnitConverter::wrapTwoPi(meanAnomaly);
	if (e < 1e-12) {
		return M;
	}

	// Maclaurin / Fourier series solution of Kepler's equation
	const double e2 = e * e;
	const double e3 = e2 * e;
	double E = M
		+ e * std::sin(M)
		+ (e2 / 2.0) * std::sin(2.0 * M)
		+ (e3 / 8.0) * (3.0 * std::sin(3.0 * M) - std::sin(M));

	// Added: a few Newton steps so the residual stays small if e is not tiny
	for (int i = 0; i < 8; ++i) {
		const double f = E - e * std::sin(E) - M;
		const double fp = 1.0 - e * std::cos(E);
		E -= f / fp;
	}
	return E;
}

Satellite::state Satellite::getState(double t) {
	OrbitalElements el = elements_;
	el.big_omega = UnitConverter::wrapTwoPi(elements_.big_omega + raan_dot_ * t);

	const double n = std::sqrt(Earth::mu / (el.a * el.a * el.a));
	const double M = mean_anomaly0_ + n * t;
	const double E = solveKeplerforE(M, el.e);
	el.v = UnitConverter::trueAnomalyFromEccentric(E, el.e);

	const BodyState body = UnitConverter::orbitalElementsToState(el);
	return state{ body.position, body.velocity };
}

OrbitalElements Satellite::getOrbitalElements() const {
	return elements_;
}

void Satellite::setOrbitalElements(const OrbitalElements& elements) {
	initializeFromElements(elements);
}

double Satellite::getSemiMajorAxis() const { return elements_.a; }
void Satellite::setSemiMajorAxis(double a) {
	elements_.a = a;
	initializeFromElements(elements_);
}
double Satellite::getEccentricity() const { return elements_.e; }
void Satellite::setEccentricity(double e) {
	elements_.e = e;
	initializeFromElements(elements_);
}
double Satellite::getInclination() const { return elements_.i; }
void Satellite::setInclination(double i) {
	elements_.i = i;
	initializeFromElements(elements_);
}
double Satellite::getRaan() const { return elements_.big_omega; }
void Satellite::setRaan(double raan) {
	elements_.big_omega = raan;
	initializeFromElements(elements_);
}
double Satellite::getArgumentOfPerigee() const { return elements_.w; }
void Satellite::setArgumentOfPerigee(double w) {
	elements_.w = w;
	initializeFromElements(elements_);
}
double Satellite::getTrueAnomaly() const { return elements_.v; }
void Satellite::setTrueAnomaly(double v) {
	elements_.v = v;
	initializeFromElements(elements_);
}
double Satellite::getJ2RaanDrift() const { return raan_dot_; }
void Satellite::setJ2RaanDrift(double raan_dot) { raan_dot_ = raan_dot; }

int Satellite::getSatelliteID() const { return SatelliteID; }
void Satellite::setSatelliteID(int id) { SatelliteID = id; }

bool Satellite::isValid() const { return valid_; }
const std::string& Satellite::getRejectionReason() const { return rejection_reason_; }

std::ostream& operator<<(std::ostream& os, const Satellite& sat) {
	os << "Satellite " << sat.SatelliteID
		<< " Position: " << sat.getSatellitePosition()
		<< ", Velocity: " << sat.getSatelliteVelocity();
	return os;
}
