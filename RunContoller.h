#pragma once

#include <iostream>
#include <string>
#include <random>
#include "SatelliteBody.h"
#include "Earth.h"

/// <summary>
/// Will eventually interface with its main subsystem "Backend controls" but as of now is a unit tester for D-01 and 
/// run controls. 
/// </summary>
class RunController {
public:
    // Delete copy and assignment to enforce a single instance
    RunController(const RunController&) = delete;
    RunController& operator=(const RunController&) = delete;

    // Public static field for global access
    inline static RunController* Instance = nullptr;

    // Initialization and Cleanup methods
    static void Initialize() {
        if (Instance == nullptr) {
            Instance = new RunController();
        }
    }

    static void Shutdown() {
        if (Instance != nullptr) {
            delete Instance;
            Instance = nullptr;
        }
    }

    /**
    * @brief Starts the simulation, from a set time or time 0 (for milestone 1, we will pass only one satellite to the function)
    * @param sat: satellite with it's current position and velocity, earth: earth struct, t: passes the time
    * @return void
    */
    void startProcessing(SatelliteBody& sat, Earth& earth, double t);

    /**
    * @brief Stops the preprocessing
    * @param None
    * @return void
    */
    void stopProcessing();

    /**
    * @brief Generates a seed or uses the passed seed to assign reasonable and random values to each satellite in a configuration
    * For milestone 1, we will only pass one satellite.
    * @param sat: the satellite in the configuration, seed: the seed entered by the user (if blank a random seed will be generated)
    * @return void
    */
    int generateSeed(SatelliteBody& sat, const std::string& seed);

private:
    // Private constructor prevents external instantiation
    RunController() = default;
    ~RunController() = default;
};