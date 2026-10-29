#pragma once

#include <iostream>
#include <string>
#include <random>
#include "Satellite.h"
#include "Earth.h"

/**
* @brief Starts the simulation, from a set time or time 0 (for milestone 1, we will pass only one satellite to the function)
* @param sat: satellite with it's current position and velocity, earth: earth struct, t: passes the time
* @return void
*/
void play(Satellite& sat, Earth& earth, double t);

/**
* @brief Stops the simulation
* @param None
* @return void
*/
void stop();

/**
* @brief Generates a seed or uses the passed seed to assign reasonable and random values to each satellite in a configuration
* For milestone 1, we will only pass one satellite.
* @param sat: the satellite in the configuration, seed: the seed entered by the user (if blank a random seed will be generated)
* @return void
*/
int generateSeed(Satellite& sat, const std::string& seed);