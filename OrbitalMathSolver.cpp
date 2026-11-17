#include "OrbitalMathSolver.h"

#include <cmath>

#include "Earth.h"

namespace {
/// <summary>Convergence threshold used by orbital calculations.</summary>
constexpr double kTolerance = 1e-12;
/// <summary>Minimum supported operating altitude used by health checks.</summary>
constexpr double kMinOperatingAltitudeKm = 200.0;
}

/// <summary>
/// Computes mean motion for a circular-equivalent semi-major axis.
/// </summary>
/// <param name="semiMajorAxis_km">Semi-major axis in kilometers.</param>
/// <returns>Mean motion in radians per second.</returns>
double OrbitalMathSolver::meanMotion(double semiMajorAxis_km) {
	if (semiMajorAxis_km <= 0.0) {
		return 0.0;
	}
	return std::sqrt(Earth::mu / (semiMajorAxis_km * semiMajorAxis_km * semiMajorAxis_km));
}

/// <summary>
/// Solves Kepler's equation with a bounded Newton iteration.
/// </summary>
/// <param name="meanAnomaly">Mean anomaly in radians.</param>
/// <param name="eccentricity">Orbital eccentricity.</param>
/// <returns>Eccentric anomaly in radians.</returns>
double OrbitalMathSolver::eccentricAnomalyFromMean(double meanAnomaly, double eccentricity) {
	if (eccentricity < kTolerance) {
		return meanAnomaly;
	}

	double eccentricAnomaly = meanAnomaly;
	for (int i = 0; i < 12; ++i) {
		const double residual = eccentricAnomaly - eccentricity * std::sin(eccentricAnomaly) - meanAnomaly;
		const double derivative = 1.0 - eccentricity * std::cos(eccentricAnomaly);
		if (std::abs(derivative) < kTolerance) {
			break;
		}
		eccentricAnomaly -= residual / derivative;
	}
	return eccentricAnomaly;
}

/// <summary>
/// Computes the secular J2 nodal drift rate.
/// </summary>
/// <param name="elements">Orbital elements.</param>
/// <returns>Nodal drift rate in radians per second.</returns>
double OrbitalMathSolver::j2NodalDriftRate(const OrbitalElements& elements) {
	if (elements.a <= 0.0 || elements.e >= 1.0) {
		return 0.0;
	}
	const double p = elements.a * (1.0 - elements.e * elements.e);
	const double n = meanMotion(elements.a);
	const double ratio = Earth::equatorial_radius / p;
	return -1.5 * n * Earth::j2 * ratio * ratio * std::cos(elements.i);
}

/// <summary>
/// Propagates orbital elements to a body state at a requested time.
/// </summary>
/// <param name="elementsAtStart">Orbital elements at the start time.</param>
/// <param name="nodalDriftRate_radps">Nodal drift rate in radians per second.</param>
/// <param name="time_s">Elapsed simulation time in seconds.</param>
/// <returns>Position and velocity state at the requested time.</returns>
BodyState OrbitalMathSolver::stateAtTime(const OrbitalElements& elementsAtStart,
	double nodalDriftRate_radps, double time_s) {
	OrbitalElements elements = elementsAtStart;
	elements.big_omega += nodalDriftRate_radps * time_s;
	const double n = meanMotion(elements.a);
	const double meanAnomaly = UnitConverter::meanAnomalyFromTrue(elementsAtStart.v, elementsAtStart.e) + n * time_s;
	const double eccentricAnomaly = eccentricAnomalyFromMean(meanAnomaly, elements.e);
	elements.v = UnitConverter::trueAnomalyFromEccentric(eccentricAnomaly, elements.e);
	return UnitConverter::orbitalElementsToState(elements);
}

/// <summary>
/// Computes periapsis altitude above the equatorial radius.
/// </summary>
/// <param name="elements">Orbital elements.</param>
/// <returns>Periapsis altitude in kilometers.</returns>
double OrbitalMathSolver::periapsisAltitude(const OrbitalElements& elements) {
	return elements.a * (1.0 - elements.e) - Earth::equatorial_radius;
}
