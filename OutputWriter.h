#pragma once

#include <memory>
#include <ostream>
#include <vector>
#include "Aircraft.h"
#include "EphemerisTable.h"
#include "GroundStation.h"
#include "SatelliteBody.h"

/// <summary>
/// FBD block "Output Writer".
/// In: the entities (satellites, ground stations, aircraft) each step, and the storage rate (TimeStepScheduler).
/// Out: D-02 to the Environment Model and the Run Data Store, and a printout.
/// The current ephemeris records every simulation step. The storage rate is retained for future Run Data Store output.
/// Every ephemeris is in ECI; conversion to longitude / latitude / altitude happens only here, on output.
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

    /// <summary>Sets the future Run Data Store storage rate.</summary>
    /// <param name="stepsPerSample">Reserved storage rate: store every Nth step. Must be at least 1.</param>
    void setStorageRate(int stepsPerSample);

    /// <summary>
    /// Called once per time step, after every entity has been updated. Every call appends the current state to the
    /// current ephemeris; storage-rate filtering is not applied to D-02.
    /// </summary>
    /// <param name="satellites">Shared list from ConstellationGenerator.</param>
    /// <param name="groundStations">Shared list from ConfigIntake.</param>
    /// <param name="aircraft">Shared list from AircraftReplay.</param>
    void recordStep(const std::vector<std::shared_ptr<SatelliteBody>>& satellites,
        const std::vector<std::shared_ptr<GroundStation>>& groundStations,
        const std::vector<std::shared_ptr<Aircraft>>& aircraft);

    /// <summary>Assembles D-02 (entity id -> ephemeris, ECI) from the entities' own ephemerides.</summary>
    /// <returns>A copy of the table.</returns>
    EphemerisTable buildTable(const std::vector<std::shared_ptr<SatelliteBody>>& satellites,
        const std::vector<std::shared_ptr<GroundStation>>& groundStations,
        const std::vector<std::shared_ptr<Aircraft>>& aircraft) const;

    /// <summary>
    /// Prints D-02, converted for reading: for each entity, one line per stored time with the time (s since start) and the
    /// position as longitude (deg), latitude (deg) and altitude (km). Velocity is not printed.
    /// </summary>
    /// <param name="out">Stream to print to.</param>
    void print(std::ostream& out,
        const std::vector<std::shared_ptr<SatelliteBody>>& satellites,
        const std::vector<std::shared_ptr<GroundStation>>& groundStations,
        const std::vector<std::shared_ptr<Aircraft>>& aircraft) const;

private:
    // Private constructor prevents external instantiation
    OutputWriter() = default;
    ~OutputWriter() = default;

    int storageRate_ = 1;
};
