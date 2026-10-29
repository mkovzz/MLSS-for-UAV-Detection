#include "GroundStation.h"

#include "UnitConverter.h"

GroundStation::GroundStation() = default;

GroundStation::GroundStation(double longitude, double latitude, double altitude, std::string identifier)
	: position_(UnitConverter::llaToEcef(longitude, latitude, altitude)),
	  id_(std::move(identifier)) {
	registry().push_back(*this);
}

Vector3d GroundStation::getPosition() const {
	return position_;
}

void GroundStation::setPosition(const Vector3d& position) {
	position_ = position;
}

const std::string& GroundStation::getId() const {
	return id_;
}

void GroundStation::setId(const std::string& identifier) {
	id_ = identifier;
}

std::vector<GroundStation>& GroundStation::registry() {
	static std::vector<GroundStation> stations;
	return stations;
}

const std::vector<GroundStation>& GroundStation::getRegistry() {
	return registry();
}

GroundStation GroundStation::GetClosest(Vector3d point) {
	const auto& stations = registry();
	if (stations.empty()) {
		return {};
	}

	std::size_t best = 0;
	double best_d2 = -1.0;
	for (std::size_t i = 0; i < stations.size(); ++i) {
		const Vector3d d = stations[i].getPosition() - point;
		const double d2 = d.Dot(d);
		if (best_d2 < 0.0 || d2 < best_d2) {
			best_d2 = d2;
			best = i;
		}
	}
	return stations[best];
}
