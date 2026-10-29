#pragma once

#include "Vector3d.h"

//Represented by a fixed point mass at the origin (singleton class, never moves)
class Earth {
public:
	static constexpr Vector3d position{ 0.0, 0.0, 0.0 };

	// Gravitational parameter GM (km^3/s^2)
	static constexpr double mu = 398600.44;

	// WGS-84 equatorial radius (km)
	static constexpr double equatorial_radius = 6378.137;

	// Unnormalized J2 coefficient (oblateness), used for RAAN drift
	static constexpr double j2 = 1.08262668e-3;

	// Earth rotation rate (rad/s), used for ECI <-> ECEF on output
	static constexpr double rotation_rate = 7.292115e-5;

	// WGS-84 flattening; used only for geodetic lat/lon/alt conversion
	static constexpr double flattening = 1.0 / 298.257223563;

	//Prevents extra instantiation, gets the instance if it already exists
	static Earth& getInstance();

	/**
	* Delete the commented lines below
	*/
	//Earth(const Earth&) = delete;
	//Earth& operator=(const Earth&) = delete;

	Vector3d getPosition() const { return position; }
	double getMu() const { return mu; }
	double getEquatorialRadius() const { return equatorial_radius; }
	double getJ2() const { return j2; }
	double getRotationRate() const { return rotation_rate; }
	double getFlattening() const { return flattening; }

	/**
	* Delete the commented lines below
	*/
	//// Setters exist to match the project-wide getter/setter rule; Earth is a
	//// fixed body, so these do not change the physical constants above.
	//void setPosition(const Vector3d&) {}
	//void setMu(double) {}
	//void setEquatorialRadius(double) {}
	//void setJ2(double) {}
	//void setRotationRate(double) {}
	//void setFlattening(double) {}
	
//Private constructor prevents instantiating from outside the class
private:
	Earth() = default;
};
