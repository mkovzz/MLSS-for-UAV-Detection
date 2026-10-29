#pragma once

#include "Vector3d.h"
#include "orbital_elements.h"

// Added: kinematic state returned by element <-> cartesian conversions (design 4.4)
struct BodyState {
	Vector3d position;
	Vector3d velocity;
};

// Static helper for all unit and frame conversions the engine needs (design 4.4)
class UnitConverter {
public:
	static constexpr double pi = 3.14159265358979323846;

	static double degToRad(double degrees);
	static double radToDeg(double radians);
	static double wrapTwoPi(double angle);

	// Geodetic lon/lat (rad) and alt (km) -> Earth-fixed x, y, z (km)
	static Vector3d llaToEcef(double longitude, double latitude, double altitude);

	// Non-rotating (ECI) <-> Earth-fixed (ECEF). Frames are aligned at t = 0.
	static Vector3d eciToEcef(const Vector3d& eci, double t_seconds);
	static Vector3d ecefToEci(const Vector3d& ecef, double t_seconds);
	static BodyState eciToEcef(const BodyState& eci, double t_seconds);

	static BodyState orbitalElementsToState(const OrbitalElements& elements);
	static OrbitalElements stateToOrbitalElements(const Vector3d& position, const Vector3d& velocity);

	// Heading: 0 = north, pi/2 = east. Speed in km/s. Position used to form local ENU.
	static Vector3d headingToVelocity(const Vector3d& position, double heading, double speed);

	static double circularOrbitSpeed(double radius_km);

	static double meanAnomalyFromTrue(double true_anomaly, double eccentricity);
	static double trueAnomalyFromEccentric(double eccentric_anomaly, double eccentricity);

	// Source-data helpers (flight-path files). Identity until a format is chosen.
	static double sourceLengthToKm(double value, double meters_per_unit = 1000.0);
	static double sourceSpeedToKmPerS(double value, double meters_per_second_per_unit = 1000.0);
};
