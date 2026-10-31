#include "ConstellationGenerator.h"

#include <regex>

namespace {
std::string section(const std::string& json) {
    const std::size_t start = json.find("\"satellites\"");
    if (start == std::string::npos) return {};
    const std::size_t open = json.find('[', start);
    const std::size_t close = json.find(']', open);
    return open == std::string::npos || close == std::string::npos ? std::string{} : json.substr(open + 1, close - open - 1);
}
double number(const std::string& object, const std::string& name, double fallback) {
    std::smatch match;
    const std::regex pattern("\\\"" + name + "\\\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    return std::regex_search(object, match, pattern) ? std::stod(match[1].str()) : fallback;
}
}

int ConstellationGenerator::generate(const ScenarioConfig& config) {
    satellites_.clear();
    const std::string satellites = section(config.json);
    const std::regex object("\\{([^}]*)\\}");
    std::smatch match;
    std::string::const_iterator begin = satellites.begin();
    int index = 1;
    while (std::regex_search(begin, satellites.cend(), match, object)) {
        const std::string value = match[1].str();
        SatelliteInitialConditions initial;
        initial.longitude_rad = number(value, "longitude_rad", 0.0);
        initial.latitude_rad = number(value, "latitude_rad", 0.0);
        initial.altitude_km = number(value, "altitude_km", 400.0);
        initial.heading_rad = number(value, "heading_rad", 0.0);
        satellites_.push_back(std::make_shared<SatelliteBody>("SAT-" + std::to_string(index++), initial));
        begin = match.suffix().first;
    }
    if (satellites_.empty() && !config.json.empty()) {
        SatelliteInitialConditions initial;
        initial.altitude_km = 400.0;
        satellites_.push_back(std::make_shared<SatelliteBody>("SAT-1", initial));
    }
    return config.json.empty() ? -1 : static_cast<int>(satellites_.size());
}

const std::vector<std::shared_ptr<SatelliteBody>>& ConstellationGenerator::getSatellites() const { return satellites_; }
