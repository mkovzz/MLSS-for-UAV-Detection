#pragma once

#include <string>
#include "ScenarioConfig.h"

/// <summary>
/// Stand-in for the full "Backend Controls" subsystem. As of now it is only a unit tester for the physics engine, driven by commands
/// typed in the terminal. It gives the engine the two things Backend Controls will eventually send in (D-01 and the run controls)
/// and fires the start signal. It has no validation, run registry or status handling.
///
/// Commands (one per line):
///   help                                   list commands
///   load <file>                           read D-01 (JSON text) from a file
///   settings <start> <end> <step> <rate>   run controls: times in seconds, rate = store every Nth step
///   status                                 show what has been loaded / set
///   run                                    fire the start signal (blocks until the run ends)
///   quit                                   exit
///
/// Singleton.
/// </summary>
class RunController {
public:
    // Delete copy and assignment to enforce a single instance
    RunController(const RunController&) = delete;
    RunController& operator=(const RunController&) = delete;

    // Public static field for global access
    inline static RunController* Instance = nullptr;

    // Initialization and Cleanup methods
    static void Initialize() {
        if (Instance == nullptr) {
            Instance = new RunController();
        }
    }

    static void Shutdown() {
        if (Instance != nullptr) {
            delete Instance;
            Instance = nullptr;
        }
    }

    /// The "Run controls" Backend Controls sends to the TimeStepScheduler.
    struct RunControls {
        double startTime_s = 0.0;
        double endTime_s = 0.0;
        double stepSize_s = 0.0;
        int storageRate = 1;    // store every Nth step
    };

    /// <summary>Reads commands from the terminal until "quit" or end of input.</summary>
    void commandLoop();

    /// <summary>
    /// Fires the start signal: creates the physics engine, gives it the loaded D-01 and run controls, then runs
    /// ProduceEphemerisTable(). The engine is shut down again afterwards so the next run starts clean.
    /// </summary>
    /// <returns>ProduceEphemerisTable()'s result (0 success, -1 not ready / rejected, -2 aborted), or -1 if nothing is loaded or the engine could not be instantiated.</returns>
    int startProcessing();

    /// <summary>
    /// Sends the stop run control to the TimeStepScheduler. Not reachable from the terminal yet, because startProcessing()
    /// blocks until the run is over.
    /// </summary>
    void stopProcessing();

private:
    // Private constructor prevents external instantiation
    RunController() = default;
    ~RunController() = default;

    /// <summary>Handles one line of terminal input.</summary>
    /// <returns>false if the loop should end.</returns>
    bool handleCommand(const std::string& line);

    /// <summary>Reads the file at path into config_.</summary>
    /// <returns>true on success.</returns>
    bool loadConfig(const std::string& path);

    void printHelp() const;
    void printStatus() const;

    ScenarioConfig config_;
    RunControls controls_;
    bool configLoaded_ = false;
    bool controlsSet_ = false;
};