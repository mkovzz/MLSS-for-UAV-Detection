#include "OrbitalMathSolver.h"

#include <cmath>

#include "Earth.h"

namespace {
constexpr double kTolerance = 1e-12;
constexpr double kMinOperatingAltitudeKm = 200.0;
}

double OrbitalMathSolver::meanMotion(double semiMajorAxis_km) {
	if (semiMajorAxis_km <= 0.0) {
		return 0.0;
	}
	return std::sqrt(Earth::mu / (semiMajorAxis_km * semiMajorAxis_km * semiMajorAxis_km));
}

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

double OrbitalMathSolver::j2NodalDriftRate(const OrbitalElements& elements) {
	if (elements.a <= 0.0 || elements.e >= 1.0) {
		return 0.0;
	}
	const double p = elements.a * (1.0 - elements.e * elements.e);
	const double n = meanMotion(elements.a);
	const double ratio = Earth::equatorial_radius / p;
	return -1.5 * n * Earth::j2 * ratio * ratio * std::cos(elements.i);
}

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

double OrbitalMathSolver::periapsisAltitude(const OrbitalElements& elements) {
	return elements.a * (1.0 - elements.e) - Earth::equatorial_radius;
}
