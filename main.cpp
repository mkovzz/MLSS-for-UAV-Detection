#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

#include "AircraftReplay.h"
#include "ConfigIntake.h"
#include "ConstellationGenerator.h"
#include "FlightGenerator.h"
#include "ScenarioConfig.h"
#include "Time_and_step_scheduler.h"
#include "physics_engine_main.h"

namespace {
/// <summary>Parses the physics-engine run command and its numeric controls.</summary>
/// <param name="argc">Argument count.</param>
/// <param name="argv">Argument values.</param>
/// <param name="startTime">Receives the run start time.</param>
/// <param name="endTime">Receives the run end time.</param>
/// <param name="stepSize">Receives the simulation step size.</param>
/// <param name="storageRate">Receives the storage interval.</param>
/// <returns>true when all arguments are valid.</returns>
bool parseArguments(int argc, char* argv[], double& startTime, double& endTime,
    double& stepSize, int& storageRate)
{
    if (argc != 6 || std::string(argv[1]) != "--run") {
        return false;
    }

    try {
        startTime = std::stod(argv[2]);
        endTime = std::stod(argv[3]);
        stepSize = std::stod(argv[4]);
        storageRate = std::stoi(argv[5]);
    }
    catch (const std::exception&) {
        return false;
    }

    return stepSize > 0.0 && endTime > startTime && storageRate >= 1;
}

/// <summary>Locates and launches the Python subsystem when no engine arguments are supplied.</summary>
/// <param name="executablePath">Path of the current executable.</param>
/// <returns>The Python subsystem process result.</returns>
int launchPythonSubsystems(const char* executablePath)
{
    namespace fs = std::filesystem;
    const fs::path executable = fs::absolute(executablePath);
    fs::path directory = executable.parent_path();
    fs::path controller;

    // Search upward so the executable can be run from either a build or source directory.
    for (int i = 0; i < 5 && !directory.empty(); ++i) {
        const fs::path candidate = directory / "PythonLauncher.py";
        if (fs::exists(candidate)) {
            controller = candidate;
            break;
        }
        directory = directory.parent_path();
    }

    if (controller.empty()) {
        std::cerr << "could not find PythonLauncher.py\n";
        return 1;
    }

    std::cout << "[C++ launcher] starting PythonLauncher.py\n" << std::flush;
    const std::string command = "python \"" + controller.string() + "\"";
    return std::system(command.c_str());
}
}

/// <summary>Runs either the Python launcher or one physics-engine ephemeris run.</summary>
/// <param name="argc">Argument count.</param>
/// <param name="argv">Argument values.</param>
/// <returns>Process result code.</returns>
int main(int argc, char* argv[])
{
    if (argc == 1) {
        return launchPythonSubsystems(argv[0]);
    }

    double startTime = 0.0;
    double endTime = 0.0;
    double stepSize = 0.0;
    int storageRate = 1;

    if (!parseArguments(argc, argv, startTime, endTime, stepSize, storageRate)) {
        std::cerr << "usage: physics-engine --run <start> <end> <step> <rate>\n";
        std::cout << "RUN_RESULT -1\n";
        return 0;
    }

    std::ostringstream contents;
    contents << std::cin.rdbuf();
    ScenarioConfig config;
    config.json = contents.str();

    int result = -1;
    if (InstantiateSubsystem() == 0) {
        // Components consume the same unchanged scenario JSON and publish one result code.
        ConfigIntake::Instance->ingest(config);
        ConstellationGenerator::Instance->generate(config);

        const int fileAircraft = AircraftReplay::Instance->loadPaths(
            ConfigIntake::Instance->getAircraftPathFiles());
        if (fileAircraft >= 0) {
            std::string generationError;
            const auto generatedPaths = FlightGenerator::Instance->generate(config, generationError);
            if (generationError.empty()) {
                if (!generatedPaths.empty()) {
                    const int generatedCount = AircraftReplay::Instance->loadSynthesizedPaths(generatedPaths);
                    std::cout << "generated " << generatedCount << " aircraft\n";
                }

                TimeStepScheduler::Instance->configure(startTime, endTime, stepSize, storageRate);
                result = ProduceEphemerisTable();
            }
            else {
                std::cout << "flight generation failed: " << generationError << "\n";
            }
        }
        else {
            std::cout << "aircraft path loading failed\n";
        }
    }

    ShutdownSubsystem();
    std::cout << "RUN_RESULT " << result << "\n";
    return 0;
}
