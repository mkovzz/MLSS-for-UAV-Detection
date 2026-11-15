#include "FlightGenerator.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <random>
#include <regex>
#include <sstream>

#include "UnitConverter.h"

namespace {
constexpr double pi = 3.14159265358979323846;
constexpr double earthRadius_km = 6378.137;
constexpr int routeSamples = 24;

std::string jsonValue(const std::string& json, const std::string& name) {
	const std::regex pattern("\\\"" + name + "\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
	std::smatch match;
	return std::regex_search(json, match, pattern) ? match[1].str() : std::string{};
}

double jsonNumber(const std::string& json, const std::string& name, double fallback) {
	const std::regex pattern("\\\"" + name + "\\\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
	std::smatch match;
	return std::regex_search(json, match, pattern) ? std::stod(match[1].str()) : fallback;
}

std::string jsonObject(const std::string& json, const std::string& name) {
	const std::size_t marker = json.find("\"" + name + "\"");
	if (marker == std::string::npos) return {};
	const std::size_t open = json.find('{', marker);
	if (open == std::string::npos) return {};
	int depth = 0;
	bool quoted = false;
	for (std::size_t i = open; i < json.size(); ++i) {
		if (json[i] == '\"' && (i == 0 || json[i - 1] != '\\')) quoted = !quoted;
		if (quoted) continue;
		if (json[i] == '{') ++depth;
		if (json[i] == '}' && --depth == 0) return json.substr(open, i - open + 1);
	}
	return {};
}

bool parseAirportLine(const std::string& line, FlightGenerator::Airport& airport) {
	std::string normalized = line;
	for (char& character : normalized) {
		if (character == ',' || character == '\t') character = ' ';
	}
	std::istringstream input(normalized);
	if (!(input >> airport.latitude_deg >> airport.longitude_deg)) return false;
	return std::isfinite(airport.latitude_deg) && std::isfinite(airport.longitude_deg)
		&& airport.latitude_deg >= -90.0 && airport.latitude_deg <= 90.0
		&& airport.longitude_deg >= -180.0 && airport.longitude_deg <= 180.0
		&& (airport.latitude_deg != -90.0 || airport.longitude_deg != 0.0);
}

Vector3d unit(const Vector3d& value) {
	const double length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
	return length > 0.0 ? value / length : Vector3d{};
}

Vector3d slerp(const Vector3d& a, const Vector3d& b, double fraction) {
	const double dot = std::clamp(a.x * b.x + a.y * b.y + a.z * b.z, -1.0, 1.0);
	const double angle = std::acos(dot);
	if (angle < 1.0e-8) return unit(a + (b - a) * fraction);
	const double sine = std::sin(angle);
	return (a * (std::sin((1.0 - fraction) * angle) / sine))
		+ (b * (std::sin(fraction * angle) / sine));
}
}

std::vector<std::vector<Aircraft::PathPoint>> FlightGenerator::generate(
	const ScenarioConfig& config, std::string& error) {
	const std::string aircraft = jsonObject(config.json, "aircraft");
	if (aircraft.empty()) {
		error = "scenario aircraft must be an object";
		return {};
	}
	const int count = static_cast<int>(jsonNumber(aircraft, "count", 0.0));
	const std::string speed = jsonObject(aircraft, "speed_kmps");
	const std::string altitude = jsonObject(aircraft, "altitude_km");
	const double speedMin = jsonNumber(speed, "min", 0.2);
	const double speedMax = jsonNumber(speed, "max", speedMin);
	const double altitudeMin = jsonNumber(altitude, "min", 8.0);
	const double altitudeMax = jsonNumber(altitude, "max", altitudeMin);
	const std::string databasePath = jsonValue(aircraft, "airport_database").empty()
		? "gadb_declatlon.csv" : jsonValue(aircraft, "airport_database");
	const unsigned int seed = static_cast<unsigned int>(jsonNumber(aircraft, "seed", 0.0));

	if (count < 0 || speedMin <= 0.0 || speedMax < speedMin || altitudeMin < 0.0 || altitudeMax < altitudeMin) {
		error = "invalid aircraft count, speed_kmps range, or altitude_km range";
		return {};
	}

	std::filesystem::path resolvedDatabasePath(databasePath);
	std::ifstream input(resolvedDatabasePath);
	if (!input && !resolvedDatabasePath.is_absolute()) {
		const std::filesystem::path sourceDirectory = std::filesystem::path(__FILE__).parent_path();
		resolvedDatabasePath = sourceDirectory / resolvedDatabasePath;
		input.open(resolvedDatabasePath);
	}
	if (!input) {
		error = "could not open airport database '" + databasePath + "'";
		return {};
	}
	std::vector<Airport> airports;
	std::string line;
	while (std::getline(input, line)) {
		Airport airport;
		if (parseAirportLine(line, airport)) airports.push_back(airport);
	}
	if (airports.size() < 2) {
		error = "airport database must contain at least two valid airports";
		return {};
	}

	std::mt19937 generator(seed == 0 ? std::random_device{}() : seed);
	std::uniform_int_distribution<std::size_t> airportDistribution(0, airports.size() - 1);
	std::uniform_real_distribution<double> speedDistribution(speedMin, speedMax);
	std::uniform_real_distribution<double> altitudeDistribution(altitudeMin, altitudeMax);
	std::vector<std::vector<Aircraft::PathPoint>> paths;
	paths.reserve(count);

	for (int flight = 0; flight < count; ++flight) {
		const Airport& departure = airports[airportDistribution(generator)];
		const Airport* arrival = &airports[airportDistribution(generator)];
		while (arrival == &departure) arrival = &airports[airportDistribution(generator)];
		const double altitude = altitudeDistribution(generator);
		const double speed = speedDistribution(generator);
		const Vector3d departurePosition = UnitConverter::llaToEcef(
			UnitConverter::degToRad(departure.longitude_deg), UnitConverter::degToRad(departure.latitude_deg), altitude);
		const Vector3d arrivalPosition = UnitConverter::llaToEcef(
			UnitConverter::degToRad(arrival->longitude_deg), UnitConverter::degToRad(arrival->latitude_deg), altitude);
		const Vector3d departureUnit = unit(departurePosition);
		const Vector3d arrivalUnit = unit(arrivalPosition);
		const double centralAngle = std::acos(std::clamp(departureUnit.x * arrivalUnit.x
			+ departureUnit.y * arrivalUnit.y + departureUnit.z * arrivalUnit.z, -1.0, 1.0));
		const double distance_km = centralAngle * (earthRadius_km + altitude);
		const double duration_s = distance_km / speed;
		std::vector<Aircraft::PathPoint> path;
		path.reserve(routeSamples + 1);
		for (int sample = 0; sample <= routeSamples; ++sample) {
			const double fraction = static_cast<double>(sample) / routeSamples;
			const Vector3d direction = slerp(departureUnit, arrivalUnit, fraction);
			const double radius = earthRadius_km + altitude;
			path.push_back({fraction * duration_s, direction * radius});
		}
		paths.push_back(std::move(path));
	}
	return paths;
}
