#pragma once

#include "orbital_elements.h"
#include "UnitConverter.h"

/// <summary>
/// Static helper for the orbital math that entities share, so no entity repeats it. Not a component of the engine.
/// Everything here is a pure function of its arguments: positions at any time t are computed directly from the orbital
/// elements (Kepler's equation), never by stepping from a previous position.
/// Units: km, km/s, radians, seconds. Frame: ECI.
/// Used by SatelliteBody.
/// </summary>
class OrbitalMathSolver {
public:
	/// <summary>Mean motion n = sqrt(mu / a^3).</summary>
	/// <param name="semiMajorAxis_km">Semi-major axis a [km].</param>
	/// <returns>Mean motion [rad/s].</returns>
	static double meanMotion(double semiMajorAxis_km);

	/// <summary>Solves Kepler's equation M = E - e sin(E) for the eccentric anomaly E (Newton iteration; returns M when e = 0).</summary>
	/// <param name="meanAnomaly">Mean anomaly M [rad].</param>
	/// <param name="eccentricity">Eccentricity e (0 <= e < 1).</param>
	/// <returns>Eccentric anomaly E [rad].</returns>
	static double eccentricAnomalyFromMean(double meanAnomaly, double eccentricity);

	/// <summary>Rate at which J2 rotates the orbital plane about Earth's axis (drift of the right ascension of the ascending node). Calculated once per satellite.</summary>
	/// <param name="elements">The satellite's orbital elements.</param>
	/// <returns>Nodal drift rate [rad/s].</returns>
	static double j2NodalDriftRate(const OrbitalElements& elements);

	/// <summary>
	/// State at time t, computed directly from the elements at t = 0: advances the mean anomaly by n*t, solves Kepler's equation,
	/// applies the J2 drift to the ascending node, and converts to position and velocity.
	/// </summary>
	/// <param name="elementsAtStart">Orbital elements at t = 0.</param>
	/// <param name="nodalDriftRate_radps">From j2NodalDriftRate() [rad/s].</param>
	/// <param name="time_s">Seconds since the start of the run.</param>
	/// <returns>Position [km] and velocity [km/s] in ECI.</returns>
	static BodyState stateAtTime(const OrbitalElements& elementsAtStart, double nodalDriftRate_radps, double time_s);

	/// <summary>Altitude of the closest point of the orbit above Earth's equatorial radius, a(1 - e) - R. Used by SatelliteBody's early-rejection check.</summary>
	/// <param name="elements">The satellite's orbital elements.</param>
	/// <returns>Periapsis altitude [km]; negative if the orbit intersects Earth.</returns>
	static double periapsisAltitude(const OrbitalElements& elements);

	// Static-only class: no instances, copies or assignment.
	OrbitalMathSolver() = delete;
};