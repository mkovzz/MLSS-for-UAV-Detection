#pragma once

#include <string>
#include <vector>

#include "Aircraft.h"
#include "ScenarioConfig.h"

/// <summary>
/// Generates synthetic aircraft paths from the D-01 aircraft object.
/// </summary>
class FlightGenerator {
public:
	inline static FlightGenerator* Instance = nullptr;

	FlightGenerator(const FlightGenerator&) = delete;
	FlightGenerator& operator=(const FlightGenerator&) = delete;

	static void Initialize() {
		if (Instance == nullptr) Instance = new FlightGenerator();
	}

	static void Shutdown() {
		if (Instance != nullptr) {
			delete Instance;
			Instance = nullptr;
		}
	}

	/// <summary>
	/// Geographic airport location read from the airport database.
	/// </summary>
	struct Airport {
		double latitude_deg = 0.0;
		double longitude_deg = 0.0;
	};

	/// <summary>
	/// Generates one sampled route for each requested aircraft.
	/// </summary>
	/// <param name="config">D-01 scenario configuration.</param>
	/// <param name="error">Receives an error message when generation fails.</param>
	/// <returns>Generated aircraft paths.</returns>
	std::vector<std::vector<Aircraft::PathPoint>> generate(const ScenarioConfig& config, std::string& error);

private:
	FlightGenerator() = default;
	~FlightGenerator() = default;

};
