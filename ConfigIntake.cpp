#include "ConfigIntake.h"

#include <regex>

#include "HealthMonitor.h"
#include "UnitConverter.h"

namespace {
/// <summary>
/// Extracts an array section from the D-01 text for the current intake layer.
/// </summary>
/// <param name="json">Original D-01 JSON text.</param>
/// <param name="name">Array property name.</param>
/// <returns>The array fragment, or an empty string when absent.</returns>
std::string section(const std::string& json, const std::string& name) {
    const std::size_t start = json.find("\"" + name + "\"");
    if (start == std::string::npos) return {};
    const std::size_t open = json.find('[', start);
    const std::size_t close = json.find(']', open);
    return open == std::string::npos || close == std::string::npos ? std::string{} : json.substr(open + 1, close - open - 1);
}
/// <summary>
/// Reads a numeric property from an object fragment.
/// </summary>
/// <param name="object">JSON object fragment.</param>
/// <param name="name">Property name.</param>
/// <param name="fallback">Value returned when the property is absent.</param>
/// <returns>The parsed value or fallback.</returns>
double number(const std::string& object, const std::string& name, double fallback = 0.0) {
    std::smatch match;
    const std::regex pattern("\\\"" + name + "\\\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    return std::regex_search(object, match, pattern) ? std::stod(match[1].str()) : fallback;
}
}

/// <summary>
/// Reads D-01 and creates the configured ground stations and aircraft paths.
/// </summary>
/// <param name="config">D-01 scenario configuration.</param>
/// <returns>true when intake succeeds; otherwise false.</returns>
bool ConfigIntake::ingest(const ScenarioConfig& config) {
    groundStations_.clear();
    aircraftPathFiles_.clear();
    if (config.json.empty()) {
        if (HealthMonitor::Instance) HealthMonitor::Instance->reportEarlyRejection("empty configuration");
        return false;
    }
    const std::string stations = section(config.json, "groundStations");
    // This lightweight parser intentionally consumes the original JSON text without rewriting it.
    const std::regex object("\\{([^}]*)\\}");
    std::smatch match;
    std::string::const_iterator begin = stations.begin();
    while (std::regex_search(begin, stations.cend(), match, object)) {
        const std::string value = match[1].str();
        std::smatch id;
        const std::regex idPattern("\\\"id\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
        if (std::regex_search(value, id, idPattern)) {
            const Vector3d position = UnitConverter::llaToEcef(number(value, "longitude_rad"), number(value, "latitude_rad"), number(value, "altitude_km"));
            groundStations_.push_back(std::make_shared<GroundStation>(id[1].str(), position));
        }
        begin = match.suffix().first;
    }
    const std::string aircraft = section(config.json, "aircraft");
    begin = aircraft.begin();
    while (std::regex_search(begin, aircraft.cend(), match, object)) {
        const std::string value = match[1].str();
        std::smatch id;
        std::smatch path;
        const std::regex idPattern("\\\"id\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
        const std::regex pathPattern("\\\"(?:path|file)\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
        if (std::regex_search(value, id, idPattern) && std::regex_search(value, path, pathPattern)) aircraftPathFiles_[id[1].str()] = path[1].str();
        begin = match.suffix().first;
    }
    return true;
}

const std::vector<std::shared_ptr<GroundStation>>& ConfigIntake::getGroundStations() const { return groundStations_; }
const std::map<std::string, std::string>& ConfigIntake::getAircraftPathFiles() const { return aircraftPathFiles_; }
