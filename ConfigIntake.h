#pragma once

#include <memory>
#include <vector>
#include "GroundStation.h"
#include "ScenarioConfig.h"

/// <summary>
/// FBD block "Config Intake".
/// In: D-01 (Backend Controls; RunController stands in for now).
/// Out: Position to GroundStation, D-01 to ConstellationGenerator, Aircraft paths to AircraftReplay, Early rejection to HealthMonitor.
/// Instantiates the ground stations and owns their shared list. Singleton.
/// </summary>
class ConfigIntake {
public:
    // Delete copy and assignment to enforce a single instance
    ConfigIntake(const ConfigIntake&) = delete;
    ConfigIntake& operator=(const ConfigIntake&) = delete;

    // Public static field for global access
    inline static ConfigIntake* Instance = nullptr;

    // Initialization and Cleanup methods
    static void Initialize() {
        if (Instance == nullptr) {
            Instance = new ConfigIntake();
        }
    }

    static void Shutdown() {
        if (Instance != nullptr) {
            delete Instance;
            Instance = nullptr;
        }
    }

    /// <summary>Reads D-01 and distributes it. Sends an early rejection to HealthMonitor if the configuration cannot be run.</summary>
    /// <param name="config">D-01.</param>
    /// <returns>true if accepted, false if rejected.</returns>
    bool ingest(const ScenarioConfig& config);

    /// <returns>The shared list of ground stations.</returns>
    const std::vector<std::shared_ptr<GroundStation>>& getGroundStations() const;

private:
    // Private constructor prevents external instantiation
    ConfigIntake() = default;
    ~ConfigIntake() = default;

    std::vector<std::shared_ptr<GroundStation>> groundStations_;
};