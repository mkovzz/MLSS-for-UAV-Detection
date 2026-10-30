#pragma once

#include <vector>
#include "EarthFixedPosition.h"
#include "EciState.h"
#include "EphemerisTable.h"

/// <summary>
/// FBD block "Output Writer".
/// In: D-12 (SatelliteBody), D-13 (GroundStation, AircraftReplay), Storage interval (s) (TimeStepScheduler),
/// Rotation rate and flattening (Earth). Out: D-02 to the Environment Model and the Run Data Store.
/// Singleton.
/// </summary>
class OutputWriter {
public:
    // Delete copy and assignment to enforce a single instance
    OutputWriter(const OutputWriter&) = delete;
    OutputWriter& operator=(const OutputWriter&) = delete;

    // Public static field for global access
    inline static OutputWriter* Instance = nullptr;

    // Initialization and Cleanup methods
    static void Initialize() {
        if (Instance == nullptr) {
            Instance = new OutputWriter();
        }
    }

    static void Shutdown() {
        if (Instance != nullptr) {
            delete Instance;
            Instance = nullptr;
        }
    }

    /// <param name="storageInterval_s">Storage interval from TimeStepScheduler [s].</param>
    void setStorageInterval(double storageInterval_s);

    /// <summary>Adds one time step of every entity's data to D-02 (D-13 positions are brought into the frame of D-12 using Earth's rotation rate and flattening).</summary>
    /// <param name="time_s">Simulation time [s].</param>
    /// <param name="satellites">D-12 states.</param>
    /// <param name="groundStations">D-13 positions.</param>
    /// <param name="aircraft">D-13 positions.</param>
    void recordStep(double time_s,
        const std::vector<EciState>& satellites,
        const std::vector<EarthFixedPosition>& groundStations,
        const std::vector<EarthFixedPosition>& aircraft);

    /// <returns>D-02 produced so far.</returns>
    const EphemerisTable& getTable() const;

private:
    // Private constructor prevents external instantiation
    OutputWriter() = default;
    ~OutputWriter() = default;

    EphemerisTable table_;
};