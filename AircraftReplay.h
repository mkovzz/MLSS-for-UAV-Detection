#pragma once

#include <string>
#include <vector>
#include "EarthFixedPosition.h"

/// <summary>
/// FBD block "Aircraft Replay".
/// In: Aircraft paths (ConfigIntake), time (s). Out: D-13 to OutputWriter.
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

    /// <summary>Loads the recorded flight paths.</summary>
    /// <param name="flightPathFiles">Flight path data files (CSV) for the aircraft in D-01.</param>
    /// <returns>Number of aircraft loaded, -1 on failure.</returns>
    int loadPaths(const std::vector<std::string>& flightPathFiles);

    /// <returns>D-13 positions of all aircraft at the given time.</returns>
    /// <param name="time_s">Simulation time [s].</param>
    std::vector<EarthFixedPosition> getPositions(double time_s) const;

private:
    // Private constructor prevents external instantiation
    AircraftReplay() = default;
    ~AircraftReplay() = default;
};