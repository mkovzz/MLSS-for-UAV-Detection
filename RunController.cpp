#include <fstream>
#include <filesystem>
#include <iostream>
#include <regex>
#include <sstream>

#include "RunContoller.h"
#include "AircraftReplay.h"
#include "OpenSkyClient.h"
#include "ConfigIntake.h"
#include "ConstellationGenerator.h"
#include "Time_and_step_scheduler.h"
#include "physics_engine_main.h"

void RunController::commandLoop()
{
    printHelp();
    std::cout << "> ";

    std::string line;
    while (std::getline(std::cin, line)) {
        if (!handleCommand(line)) {
            break;
        }
        std::cout << "> ";
    }
}
bool RunController::handleCommand(const std::string& line)
{
    std::istringstream in(line);
    std::string command;
    in >> command;

    if (command.empty()) {
        return true;
    }

    if (command == "help") {
        printHelp();
    }
    else if (command == "load") {
        std::string path;
        std::getline(in >> std::ws, path);
        if (path.empty()) {
            std::cout << "usage: load <file>\n";
        }
        else if (path.rfind("flight data ", 0) == 0) {
            if (!loadFlightData(path.substr(12))) std::cout << "flight data load failed\n";
        }
        else if (loadConfig(path)) {
            std::cout << "D-01 loaded (" << config_.json.size() << " bytes)\n";
        }
    }
    else if (command == "check") {
        std::string usage;
        std::getline(in >> std::ws, usage);
        if (usage.rfind("opensky usage ", 0) != 0) {
            std::cout << "usage: check opensky usage <credentials-json>\n";
        }
        else {
            const OpenSkyUsageResult result = OpenSkyClient::checkUsage(usage.substr(15));
            if (!result.error.empty()) std::cout << result.error << "\n";
            std::cout << "OpenSky usage probe: HTTP " << result.httpStatus << " (" << result.endpoint << ")\n";
            if (!result.rateLimitRemaining.empty()) {
                std::cout << "OpenSky API credits remaining: " << result.rateLimitRemaining << "\n";
            }
            if (!result.retryAfterSeconds.empty()) {
                std::cout << "OpenSky retry after: " << result.retryAfterSeconds << " seconds\n";
            }
        }
    }
    else if (command == "settings") {
        RunControls c;
        if (!(in >> c.startTime_s >> c.endTime_s >> c.stepSize_s >> c.storageRate)) {
            std::cout << "usage: settings <start> <end> <step> <rate>   (times in seconds; rate = store every Nth step)\n";
        }
        else if (c.stepSize_s <= 0.0 || c.endTime_s <= c.startTime_s || c.storageRate < 1) {
            std::cout << "invalid: need step > 0, end > start, rate >= 1\n";
        }
        else {
            controls_ = c;
            controlsSet_ = true;
            std::cout << "run controls set\n";
        }
    }
    else if (command == "opensky") {
        std::string arguments;
        std::getline(in >> std::ws, arguments);
        const std::regex limitedPattern(R"(^([0-9]+)\s+([0-9]+(?:\.[0-9]+)?)\s+([0-9]+)\s+(\{.*\})$)");
        const std::regex legacyPattern(R"(^([0-9]+)\s+([0-9]+(?:\.[0-9]+)?)\s+(\{.*\})$)");
        std::smatch match;
        if (std::regex_match(arguments, match, limitedPattern)) {
            openSkySnapshotTime_s_ = std::stoll(match[1].str());
            openSkyDuration_s_ = std::stod(match[2].str());
            openSkyMaxAircraft_ = std::stoi(match[3].str());
            openSkyCredentialsJson_ = match[4].str();
            if (openSkySnapshotTime_s_ <= 0 || openSkySnapshotTime_s_ > 4102444800LL
                || openSkyDuration_s_ <= 0.0 || openSkyMaxAircraft_ < 1) {
                openSkyCredentialsJson_.clear();
                std::cout << "invalid: unix_time must be Unix seconds (not milliseconds or an extra zero), "
                    << "duration must be > 0, and max_aircraft must be >= 1\n";
            }
            else {
                std::cout << "OpenSky import configured for Unix time " << openSkySnapshotTime_s_
                    << " (max " << openSkyMaxAircraft_ << " aircraft)\n";
                const OpenSkyImportResult imported = OpenSkyClient::fetchPaths(
                    openSkyCredentialsJson_, openSkySnapshotTime_s_, openSkyDuration_s_, aircraftAltitude_km_, openSkyMaxAircraft_);
                if (!imported.error.empty()) {
                    std::cout << "OpenSky import failed: " << imported.error << "\n";
                    openSkyCredentialsJson_.clear();
                }
                else {
                    std::filesystem::path cachePath;
                    for (int number = 1;; ++number) {
                        cachePath = "flights" + std::to_string(number) + ".bin";
                        if (!std::filesystem::exists(cachePath)) break;
                    }
                    std::string cacheError;
                    if (OpenSkyClient::savePaths(cachePath.string(), imported.paths, cacheError)) {
                        std::cout << "OpenSky imported " << imported.paths.size()
                            << " aircraft and saved " << cachePath.string() << "\n";
                    }
                    else {
                        std::cout << "OpenSky cache save failed: " << cacheError << "\n";
                    }
                    if (!imported.rateLimitRemaining.empty()) {
                        std::cout << "OpenSky API credits remaining: " << imported.rateLimitRemaining << "\n";
                    }
                }
            }
        }
        else if (std::regex_match(arguments, match, legacyPattern)) {
            openSkySnapshotTime_s_ = std::stoll(match[1].str());
            openSkyDuration_s_ = std::stod(match[2].str());
            openSkyMaxAircraft_ = 0;
            openSkyCredentialsJson_ = match[3].str();
            if (openSkySnapshotTime_s_ <= 0 || openSkySnapshotTime_s_ > 4102444800LL
                || openSkyDuration_s_ <= 0.0) {
                openSkyCredentialsJson_.clear();
                std::cout << "invalid: unix_time must be Unix seconds (not milliseconds or an extra zero) "
                    << "and duration must be > 0 seconds\n";
            }
            else {
                std::cout << "OpenSky import configured for Unix time " << openSkySnapshotTime_s_ << "\n";
                const OpenSkyImportResult imported = OpenSkyClient::fetchPaths(
                    openSkyCredentialsJson_, openSkySnapshotTime_s_, openSkyDuration_s_, aircraftAltitude_km_, openSkyMaxAircraft_);
                if (!imported.error.empty()) {
                    std::cout << "OpenSky import failed: " << imported.error << "\n";
                    openSkyCredentialsJson_.clear();
                }
                else {
                    std::filesystem::path cachePath;
                    for (int number = 1;; ++number) {
                        cachePath = "flights" + std::to_string(number) + ".bin";
                        if (!std::filesystem::exists(cachePath)) break;
                    }
                    std::string cacheError;
                    if (OpenSkyClient::savePaths(cachePath.string(), imported.paths, cacheError)) {
                        std::cout << "OpenSky imported " << imported.paths.size()
                            << " aircraft and saved " << cachePath.string() << "\n";
                    }
                    else {
                        std::cout << "OpenSky cache save failed: " << cacheError << "\n";
                    }
                    if (!imported.rateLimitRemaining.empty()) {
                        std::cout << "OpenSky API credits remaining: " << imported.rateLimitRemaining << "\n";
                    }
                }
            }
        }
        else {
            std::cout << "usage: opensky <unix_time> <duration_s> [max_aircraft] <credentials-json>\n";
        }
    }
    else if (command == "status") {
        printStatus();
    }
    else if (command == "run") {
        const int result = startProcessing();
        switch (result) {
        case 0:  std::cout << "run complete\n"; break;
        case -1: std::cout << "run not started: not ready, or configuration rejected (-1)\n"; break;
        case -2: std::cout << "run aborted by the health monitor (-2)\n"; break;
        default: std::cout << "run ended with code " << result << "\n"; break;
        }
    }
    else if (command == "quit" || command == "exit") {
        return false;
    }
    else {
        std::cout << "unknown command '" << command << "' (type help)\n";
    }

    return true;
}

bool RunController::loadConfig(const std::string& path)
{
    std::string requestedPath = path;
    if (requestedPath.size() >= 2 && requestedPath.front() == '"' && requestedPath.back() == '"') {
        requestedPath = requestedPath.substr(1, requestedPath.size() - 2);
    }

    std::filesystem::path configPath(requestedPath);
    std::ifstream file(configPath);

    if (!file && !configPath.is_absolute()) {
        const std::filesystem::path sourceDirectory = std::filesystem::path(__FILE__).parent_path();
        configPath = sourceDirectory / configPath;
        file.open(configPath);
    }

    if (!file) {
        std::cout << "could not open '" << requestedPath << "'\n";
        return false;
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    config_.json = contents.str();
    configLoaded_ = true;
    return true;
}

int RunController::startProcessing()
{
    if (!configLoaded_ || !controlsSet_) {
        std::cout << "load a D-01 file and set the run settings first (type status)\n";
        return -1;
    }

    int result = -1;
    if (InstantiateSubsystem() == 0) {
        // Send D-01. The return value is ignored on purpose: a rejection is reported to the HealthMonitor,
        // which ProduceEphemerisTable() checks.
        ConfigIntake::Instance->ingest(config_);
        ConstellationGenerator::Instance->generate(config_);
        const int fileAircraft = AircraftReplay::Instance->loadPaths(ConfigIntake::Instance->getAircraftPathFiles());
        if (fileAircraft < 0) {
            std::cout << "aircraft path loading failed\n";
            ShutdownSubsystem();
            return -1;
        }
        if (!cachedFlightPaths_.empty()) {
            const int cachedCount = AircraftReplay::Instance->loadSynthesizedPaths(cachedFlightPaths_);
            std::cout << "loaded " << cachedCount << " cached aircraft into run\n";
        }
        // Send run controls
        TimeStepScheduler::Instance->configure(controls_.startTime_s, controls_.endTime_s,
            controls_.stepSize_s, controls_.storageRate);

        // Fire the start signal
        result = ProduceEphemerisTable();
    }

    // Clean slate for the next run
    ShutdownSubsystem();

    return result;
}

void RunController::stopProcessing()
{
    if (TimeStepScheduler::Instance != nullptr) {
        TimeStepScheduler::Instance->stop();
    }
}

void RunController::printHelp() const
{
    std::cout <<
        "Run Controller (physics engine test harness)\n"
        "  help                                   this list\n"
        "  load <file>                            read D-01 (JSON text) from a file\n"
        "  load flight data <file.bin>             append cached OpenSky aircraft paths\n"
        "  settings <start> <end> <step> <rate>   run controls: times in seconds, rate = store every Nth step\n"
        "  opensky <unix_time> <duration_s> [max_aircraft] <credentials-json>  fetch and save OpenSky tracks\n"
        "  check opensky usage <credentials-json>  show current OpenSky API credit headers\n"
        "  status                                 show what is loaded / set\n"
        "  run                                    fire the start signal\n"
        "  quit                                   exit\n";
}

bool RunController::loadFlightData(const std::string& path)
{
    std::vector<std::vector<Aircraft::PathPoint>> paths;
    std::string error;
    if (!OpenSkyClient::loadPaths(path, paths, error)) {
        std::cout << "could not load '" << path << "': " << error << "\n";
        return false;
    }
    cachedFlightPaths_.insert(cachedFlightPaths_.end(), paths.begin(), paths.end());
    std::cout << "loaded " << paths.size() << " cached aircraft from " << path << "\n";
    return !paths.empty();
}

void RunController::printStatus() const
{
    std::cout << "D-01: " << (configLoaded_ ? "loaded" : "not loaded") << "\n";
    if (controlsSet_) {
        std::cout << "run controls: start " << controls_.startTime_s << " s, end " << controls_.endTime_s
            << " s, step " << controls_.stepSize_s << " s, store every " << controls_.storageRate << " step(s)\n";
    }
    else {
        std::cout << "run controls: not set\n";
    }
    if (!openSkyCredentialsJson_.empty()) {
        std::cout << "OpenSky: configured for Unix time " << openSkySnapshotTime_s_ << " (duration "
            << openSkyDuration_s_ << " s";
        if (openSkyMaxAircraft_ > 0) std::cout << ", max " << openSkyMaxAircraft_ << " aircraft";
        std::cout << ")\n";
    }
    else {
        std::cout << "OpenSky: not configured\n";
    }
    std::cout << "Cached flight paths selected for run: " << cachedFlightPaths_.size() << "\n";
}