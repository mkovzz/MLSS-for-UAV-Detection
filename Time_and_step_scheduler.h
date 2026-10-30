#pragma once

/// <summary>
/// FBD block "Time and step scheduler".
/// In: Run controls (Backend Controls; RunController stands in for now).
/// Out: time (s) to SatelliteBody and AircraftReplay (via ProduceEphemerisTable), Storage interval (s) to OutputWriter.
/// Singleton.
/// </summary>
class TimeStepScheduler {
public:
    // Delete copy and assignment to enforce a single instance
    TimeStepScheduler(const TimeStepScheduler&) = delete;
    TimeStepScheduler& operator=(const TimeStepScheduler&) = delete;

    // Public static field for global access
    inline static TimeStepScheduler* Instance = nullptr;

    // Initialization and Cleanup methods
    static void Initialize() {
        if (Instance == nullptr) {
            Instance = new TimeStepScheduler();
        }
    }

    static void Shutdown() {
        if (Instance != nullptr) {
            delete Instance;
            Instance = nullptr;
        }
    }

    /// <summary>Applies the run controls: start time, end time, step size and storage interval.</summary>
    void configure(double startTime_s, double endTime_s, double stepSize_s, double storageInterval_s);

    /// <summary>Moves to the next step.</summary>
    /// <returns>false once the end time is reached or the run is stopped.</returns>
    bool advance();

    void pause();
    void resume();
    void stop();

    /// <returns>Current simulation time [s].</returns>
    double getTime() const;

    /// <returns>Storage interval [s].</returns>
    double getStorageInterval() const;

private:
    // Private constructor prevents external instantiation
    TimeStepScheduler() = default;
    ~TimeStepScheduler() = default;
};