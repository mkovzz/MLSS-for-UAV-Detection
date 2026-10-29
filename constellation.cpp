#include "constellation.h"

#include "Earth.h"
#include "UnitConverter.h"
#include "orbital_elements.h"

/**
* @brief Default constructor
* @param None
* @return Constellation object with a default constructed satellite
*/
Constellation::Constellation() {
	satellites.push_back(Satellite());
}

/**
* @brief Constructor for a single satellite
* @param sat: an instantiated satellite object
* @return Constellation containing the passed satellite
*/
Constellation::Constellation(Satellite& sat) {
	satellites.push_back(sat);
}

Constellation::Constellation(double altitude_km, double inclination_rad, int satellite_count, int plane_count) {
	if (plane_count < 1) {
		plane_count = 1;
	}
	if (satellite_count < 1) {
		satellite_count = 1;
	}

	const int per_plane = satellite_count / plane_count;
	const int extra = satellite_count % plane_count;

	for (int p = 0; p < plane_count; ++p) {
		const int count = per_plane + (p < extra ? 1 : 0);
		if (count < 1) {
			continue;
		}

		OrbitalElements el;
		el.a = Earth::equatorial_radius + altitude_km;
		el.e = 0.0;
		el.i = inclination_rad;
		el.big_omega = 2.0 * UnitConverter::pi * static_cast<double>(p) / static_cast<double>(plane_count);
		el.w = 0.0;

		for (int j = 0; j < count; ++j) {
			el.v = 2.0 * UnitConverter::pi * static_cast<double>(j) / static_cast<double>(count);
			satellites.push_back(Satellite(el));
		}
	}
}

std::vector<Satellite>& Constellation::getSatellites() {
	return satellites;
}

const std::vector<Satellite>& Constellation::getSatellites() const {
	return satellites;
}

void Constellation::setSatellites(const std::vector<Satellite>& sats) {
	satellites = sats;
}
