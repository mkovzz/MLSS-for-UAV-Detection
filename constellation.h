#pragma once
#include <vector>
#include "Satellite.h"

class Constellation {
public:
	
	//Array of satellites within the constellation
	//Should only be one for milestone 1
	std::vector<Satellite> satellites;

	/**
	* @brief Default constructor
	* @param None
	* @return Constellation object with a default constructed satellite
	*/
	Constellation();

	/**
	* @brief Constructor for a single satellite
	* @param sat: an instantiated satellite object
	* @return Constellation containing the passed satellite
	*/
	Constellation(Satellite& sat);

	/**
	* AI Added: design 4.1 preset batch. altitude_km is height above Earth, inclination_rad is tilt.
	* Satellites are placed on evenly spaced planes with evenly spaced true anomalies.
	*/
	//Constellation(double altitude_km, double inclination_rad, int satellite_count, int plane_count);

	//std::vector<Satellite>& getSatellites();
	//const std::vector<Satellite>& getSatellites() const;
	//void setSatellites(const std::vector<Satellite>& sats);
};
