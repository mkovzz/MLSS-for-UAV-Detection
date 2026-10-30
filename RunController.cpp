#include <fstream>
#include <iostream>
#include <sstream>

#include "run_controller.h"
#include "ConfigIntake.h"
#include "TimeStepScheduler.h"
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
        else if (loadConfig(path)) {
            std::cout << "D-01 loaded (" << config_.json.size() << " bytes)\n";
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
    std::ifstream file(path);
    if (!file) {
        std::cout << "could not open '" << path << "'\n";
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
        "  settings <start> <end> <step> <rate>   run controls: times in seconds, rate = store every Nth step\n"
        "  status                                 show what is loaded / set\n"
        "  run                                    fire the start signal\n"
        "  quit                                   exit\n";
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
}