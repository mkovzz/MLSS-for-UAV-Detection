#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "Aircraft.h"

/// <summary>
/// FBD block "Aircraft Replay".
/// In: Aircraft paths (ConfigIntake). Creates one Aircraft per path and owns the shared list.
/// Singleton.
/// </summary>
class AircraftReplay {
public:
    // Delete copy and assignment to enforce a single instance
    AircraftReplay(const AircraftReplay&) = delete;
    AircraftReplay& operator=(const AircraftReplay&) = delete;

    // Public static field for global access
    inline static AircraftReplay* Instance = nullptr;

    // Initialization and Cleanup methods
    static void Initialize() {
        if (Instance == nullptr) {
            Instance = new AircraftReplay();
        }
    }

    static void Shutdown() {
        if (Instance != nullptr) {
            delete Instance;
            Instance = nullptr;
        }
    }

    /// <summary>Creates an Aircraft for each path, replacing any previous set.</summary>
    /// <param name="flightPathFilesById">Aircraft id -> flight path data file (CSV), as listed in D-01.</param>
    /// <returns>Number of aircraft loaded, -1 if any file is missing or unparsable.</returns>
    int loadPaths(const std::map<std::string, std::string>& flightPathFilesById);

    /// <returns>The shared list of aircraft.</returns>
    const std::vector<std::shared_ptr<Aircraft>>& getAircraft() const;

private:
    // Private constructor prevents external instantiation
    AircraftReplay() = default;
    ~AircraftReplay() = default;

    std::vector<std::shared_ptr<Aircraft>> aircraft_;
};