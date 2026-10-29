#pragma once
#include <iostream>
#include <string>
#include "Vector3d.h"
#include "orbital_elements.h"

class Satellite {
public: //separate public and private data using respective keywords
	struct state {

		Vector3d position; //km
		Vector3d velocity; //km/s

	};
	int SatelliteID = 0;

	/**
	* @brief Default Constructor
	* @return new satellite object with zero-initialized values
	*/
	Satellite();

	/**
	* @brief Constructor for satellite generation
	* @return new satellite object with initialized values based on a random seed from the run controller 
	*/
	Satellite(Vector3d start_pos, Vector3d start_vel);

	/**
	* Added: design 4.1 constructor. longitude, latitude, heading in radians; altitude in km.
	* Velocity is the circular-orbit speed at that radius, along the local horizontal heading.
	*/
	Satellite(double longitude, double latitude, double altitude, double heading);

	/**
	* Added: construct from classical orbital elements (used by the constellation batch generator).
	*/
	explicit Satellite(const OrbitalElements& elements);

	/**
	* @brief Gets the position of the satellite
	* @param None
	* @return list of the x, y, and z satellite position components
	*/
	Vector3d getSatellitePosition() const;

	/**
	* @brief Gets the velocity of the satellite
	* @param None
	* @return list of the x, y, and z satellite velocity components
	*/
	Vector3d getSatelliteVelocity() const;

	/**
	* @brief Solves Kepler's equation to get the Eccentric Anomaly, uses a Mclaurin series approximation since equation is transcendental
	* @param mean_anomaly: mean anomaly of the orbiting satellite, e: eccentricity 
	* @return Returns the eccentric anomaly, E
	*/
	double solveKeplerforE(double meanAnomaly, double e);

	/**
	* @brief Gets the state, position and velocity, at time t using the orbital elements
	* @param t: time in orbit
	* @return Returns a state struct containing calculated positions and velocities
	*/
	state getState(double t);

	OrbitalElements getOrbitalElements() const;
	void setOrbitalElements(const OrbitalElements& elements);

	double getSemiMajorAxis() const;
	void setSemiMajorAxis(double a);
	double getEccentricity() const;
	void setEccentricity(double e);
	double getInclination() const;
	void setInclination(double i);
	double getRaan() const;
	void setRaan(double raan);
	double getArgumentOfPerigee() const;
	void setArgumentOfPerigee(double w);
	double getTrueAnomaly() const;
	void setTrueAnomaly(double v);
	double getJ2RaanDrift() const;
	void setJ2RaanDrift(double raan_dot);

	int getSatelliteID() const;
	void setSatelliteID(int id);

	bool isValid() const;
	const std::string& getRejectionReason() const;

	/**
	* Overloaded cout operator that prints the position and velocity of the satellite at a given time to the terminal
	*/
	friend std::ostream& operator<<(std::ostream& os, const Satellite& sat);

private:
	OrbitalElements elements_{};
	double mean_anomaly0_ = 0.0;
	double raan_dot_ = 0.0;
	state epoch_state_{};
	bool valid_ = true;
	std::string rejection_reason_;

	void initializeFromState(const Vector3d& position, const Vector3d& velocity);
	void initializeFromElements(const OrbitalElements& elements);
	void computeJ2Drift();
	void runEarlyRejection();
	static int nextId();
};
