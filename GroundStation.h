#pragma once

#include <string>
#include <vector>

#include "Vector3d.h"

// Ground station: a fixed ECEF position converted at load (design 4.3)
class GroundStation {
public:

	/**
	* @brief Default constructor for ground
	* @param None
	* @return Instantiated ground station object
	*/
	GroundStation();

	/**
	* @brief Constructor for 
	* @param None
	* @return Instantiated ground station object
	*/
	GroundStation(double longitude, double latitude, double altitude, std::string identifier = "");

	//Getters and Setters
	Vector3d getPosition() const;
	void setPosition(const Vector3d& position);

	const std::string& getId() const;
	void setId(const std::string& identifier);

	// Returns the registered ground station closest to the given ECEF point.
	// If none are registered, returns a default-constructed station.
	static GroundStation GetClosest(Vector3d point);

	static const std::vector<GroundStation>& getRegistry();

private:
	Vector3d position_{};
	std::string id_{};
	static std::vector<GroundStation>& registry();
};
