#include "AircraftReplay.h"

int AircraftReplay::loadPaths(const std::map<std::string, std::string>& files) {
    aircraft_.clear();
    for (const auto& pair : files) {
        auto aircraft = std::make_shared<Aircraft>(pair.first, pair.second);
        if (!aircraft->isLoaded()) return -1;
        aircraft_.push_back(aircraft);
    }
    return static_cast<int>(aircraft_.size());
}

int AircraftReplay::loadSynthesizedPaths(const std::vector<std::vector<Aircraft::PathPoint>>& paths) {
    int number = 1;
    for (const auto& path : paths) {
        auto aircraft = std::make_shared<Aircraft>("A" + std::to_string(number++), path);
        if (!aircraft->isLoaded()) continue;
        aircraft_.push_back(aircraft);
    }
    return static_cast<int>(number - 1);
}

const std::vector<std::shared_ptr<Aircraft>>& AircraftReplay::getAircraft() const { return aircraft_; }
