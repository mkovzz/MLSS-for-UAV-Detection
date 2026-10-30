#pragma once

#include <memory>
#include <vector>
#include "SatelliteBody.h"
#include "ScenarioConfig.h"

/// <summary>
/// FBD block "Constellation Generator".
/// In: D-01 (ConfigIntake). Out: D-10 to each SatelliteBody it creates.
/// Owns the shared list of satellites. Singleton.
/// </summary>
class ConstellationGenerator {
public:
    // Delete copy and assignment to enforce a single instance
    ConstellationGenerator(const ConstellationGenerator&) = delete;
    ConstellationGenerator& operator=(const ConstellationGenerator&) = delete;

    // Public static field for global access
    inline static ConstellationGenerator* Instance = nullptr;

    // Initialization and Cleanup methods
    static void Initialize() {
        if (Instance == nullptr) {
            Instance = new ConstellationGenerator();
        }
    }

    static void Shutdown() {
        if (Instance != nullptr) {
            delete Instance;
            Instance = nullptr;
        }
    }

    /// <summary>Generates D-10 for every satellite described by D-01 and instantiates a SatelliteBody for each.</summary>
    /// <param name="config">D-01.</param>
    /// <returns>Number of satellites created, -1 on failure.</returns>
    int generate(const ScenarioConfig& config);

    /// <returns>The shared list of satellite bodies.</returns>
    const std::vector<std::shared_ptr<SatelliteBody>>& getSatellites() const;

private:
    // Private constructor prevents external instantiation
    ConstellationGenerator() = default;
    ~ConstellationGenerator() = default;

    std::vector<std::shared_ptr<SatelliteBody>> satellites_;
};