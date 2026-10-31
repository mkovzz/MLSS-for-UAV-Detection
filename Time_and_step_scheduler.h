#pragma once

/// <summary>
/// FBD block "Time and step scheduler".
/// In: Run controls (Backend Controls; RunController stands in for now).
/// Out: time (s) to the entities (via ProduceEphemerisTable), storage rate to OutputWriter (via ProduceEphemerisTable).
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

    /// <summary>Applies the run controls: start time, end time, step size and storage rate.</summary>
    /// <param name="storageRate">Store every Nth step (1 = every step); a multiple of the step size, so storage is counted in whole steps.</param>
    void configure(double startTime_s, double endTime_s, double stepSize_s, int storageRate);

    /// <summary>Moves to the next step.</summary>
    /// <returns>false once the end time is reached or the run is stopped.</returns>
    bool advance();

    void pause();
    void resume();
    void stop();

    /// <returns>Current simulation time [s].</returns>
    double getTime() const;

    /// <returns>Storage rate: store every Nth step.</returns>
    int getStorageRate() const;

private:
    // Private constructor prevents external instantiation
    TimeStepScheduler() = default;
    ~TimeStepScheduler() = default;

    double startTime_s_ = 0.0;
    double endTime_s_ = 0.0;
    double stepSize_s_ = 1.0;
    double currentTime_s_ = 0.0;
    int storageRate_ = 1;
    bool paused_ = false;
    bool stopped_ = false;
};