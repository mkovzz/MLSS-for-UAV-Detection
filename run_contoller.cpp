#include "run_contoller.h"

#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <random>
#include <string>

#include "UnitConverter.h"
#include "main.h"

namespace {
	bool g_running = false;
}

/**
* @brief Starts the simulation, from a set time or time 0 (for milestone 1, we will pass only one satellite to the function)
* @param sat: satellite with it's current position and velocity, earth: earth struct, t: passes the time
* @return void
*/
void play(Satellite& sat, Earth& earth, double t) {
	g_running = true;

	if (!sat.isValid()) {
		std::cout << "Early rejection: " << sat.getRejectionReason() << "\n";
		g_running = false;
		return;
	}

	std::cout << "Press Enter to start the run...\n";
	std::cin.get();

	const double a = sat.getSemiMajorAxis();
	const double period = 2.0 * UnitConverter::pi * std::sqrt(a * a * a / earth.getMu());
	double last_log = -1.0;

	std::cout << sat << "\n";
	while (g_running && t < period) {
		t = step_scheduler(1, t);
		if (t - last_log >= 1.0 - 1e-9) {
			const Satellite::state s = sat.getState(t);
			std::cout << "t=" << t << "s  pos=" << s.position << " km  vel=" << s.velocity << " km/s\n";
			last_log = t;
		}
	}
	g_running = false;
}

/**
* @brief Stops the simulation
* @param None
* @return void
*/
void stop() {
	g_running = false;
}

/**
* @brief Generates a seed or uses the passed seed to assign reasonable and random values to each satellite in a configuration
* For milestone 1, we will only pass one satellite.
* @param sat: the satellite in the configuration, seed: the seed entered by the user (if blank a random seed will be generated)
* @return void
*/
int generateSeed(Satellite& sat, const std::string& seed) {
	std::uint32_t numeric_seed = 0;
	if (seed.empty()) {
		numeric_seed = std::random_device{}();
	}
	else {
		numeric_seed = static_cast<std::uint32_t>(std::hash<std::string>{}(seed));
	}

	std::mt19937 rng(numeric_seed);
	std::uniform_real_distribution<double> lon_dist(-UnitConverter::pi, UnitConverter::pi);
	std::uniform_real_distribution<double> lat_dist(-0.9, 0.9);
	std::uniform_real_distribution<double> alt_dist(400.0, 800.0);
	std::uniform_real_distribution<double> hdg_dist(0.0, 2.0 * UnitConverter::pi);

	sat = Satellite(lon_dist(rng), lat_dist(rng), alt_dist(rng), hdg_dist(rng));
	return static_cast<int>(numeric_seed);
}
