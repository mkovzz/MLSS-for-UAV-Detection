#pragma once

#include <string>
#include <vector>

#include "Aircraft.h"
#include "ScenarioConfig.h"

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

	struct Airport {
		double latitude_deg = 0.0;
		double longitude_deg = 0.0;
	};

	std::vector<std::vector<Aircraft::PathPoint>> generate(const ScenarioConfig& config, std::string& error);

private:
	FlightGenerator() = default;
	~FlightGenerator() = default;

};
