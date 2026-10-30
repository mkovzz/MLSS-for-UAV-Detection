#pragma once

#include "Vector3d.h"

/// <summary>
/// FBD block "Earth". A fully static informational hub: a fixed point mass at the origin that never moves.
/// Never instantiated; read the constants directly, e.g. Earth::mu.
/// Consumers: SatelliteBody (mu, j2), OutputWriter (rotation_rate, flattening for ECI <-> ECEF), GroundStation / ConfigIntake (flattening, equatorial_radius for geodetic conversion).
/// </summary>
class Earth {
public:
	/// Position of Earth's centre (km), the origin of the ECI frame.
	static constexpr Vector3d position{ 0.0, 0.0, 0.0 };

	/// Gravitational parameter GM (km^3/s^2)
	static constexpr double mu = 398600.44;

	/// WGS-84 equatorial radius (km)
	static constexpr double equatorial_radius = 6378.137;

	/// Unnormalized J2 coefficient (oblateness), used for RAAN drift
	static constexpr double j2 = 1.08262668e-3;

	/// Earth rotation rate (rad/s), used for ECI <-> ECEF on output
	static constexpr double rotation_rate = 7.292115e-5;

	/// WGS-84 flattening; used only for geodetic lat/lon/alt conversion
	static constexpr double flattening = 1.0 / 298.257223563;

	// Static-only class: no instances, copies or assignment.
	Earth() = delete;
};