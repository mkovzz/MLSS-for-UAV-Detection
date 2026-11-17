#include "AircraftReplay.h"

/// <summary>
/// Loads file-backed aircraft paths and returns the number loaded.
/// </summary>
/// <param name="files">Map of aircraft identifiers to path files.</param>
/// <returns>The number of loaded aircraft, or -1 on failure.</returns>
int AircraftReplay::loadPaths(const std::map<std::string, std::string>& files) {
    aircraft_.clear();
    for (const auto& pair : files) {
        auto aircraft = std::make_shared<Aircraft>(pair.first, pair.second);
        if (!aircraft->isLoaded()) return -1;
        aircraft_.push_back(aircraft);
    }
    return static_cast<int>(aircraft_.size());
}

/// <summary>
/// Loads generated aircraft paths and skips paths with no samples.
/// </summary>
/// <param name="paths">Generated path samples grouped by aircraft.</param>
/// <returns>The number of generated aircraft identifiers processed.</returns>
int AircraftReplay::loadSynthesizedPaths(const std::vector<std::vector<Aircraft::PathPoint>>& paths) {
    int number = 1;
    for (const auto& path : paths) {
        auto aircraft = std::make_shared<Aircraft>("A" + std::to_string(number++), path);
        if (!aircraft->isLoaded()) continue;
        aircraft_.push_back(aircraft);
    }
    return static_cast<int>(number - 1);
}

/// <summary>
/// Returns the aircraft managed by the replay component.
/// </summary>
/// <returns>The loaded aircraft collection.</returns>
const std::vector<std::shared_ptr<Aircraft>>& AircraftReplay::getAircraft() const { return aircraft_; }
