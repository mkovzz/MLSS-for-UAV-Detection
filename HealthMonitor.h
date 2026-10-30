#pragma once

#include <string>
#include "EciState.h"

/// <summary>
/// FBD block "Health Monitor".
/// In: Early rejection (ConfigIntake), D-12 state and rejection reason (SatelliteBody).
/// Out: Status and abort to Backend Controls (RunController stands in for now).
/// Singleton.
/// </summary>
class HealthMonitor {
public:
    // Delete copy and assignment to enforce a single instance
    HealthMonitor(const HealthMonitor&) = delete;
    HealthMonitor& operator=(const HealthMonitor&) = delete;

    // Public static field for global access
    inline static HealthMonitor* Instance = nullptr;

    // Initialization and Cleanup methods
    static void Initialize() {
        if (Instance == nullptr) {
            Instance = new HealthMonitor();
        }
    }

    static void Shutdown() {
        if (Instance != nullptr) {
            delete Instance;
            Instance = nullptr;
        }
    }

    /// <summary>ConfigIntake rejected the configuration.</summary>
    /// <param name="reason">Why it was rejected.</param>
    void reportEarlyRejection(const std::string& reason);

    /// <summary>A satellite reports its D-12 state and its rejection reason.</summary>
    /// <param name="state">D-12.</param>
    /// <param name="rejectionReason">Empty if the satellite was not rejected.</param>
    void submitSatelliteState(const EciState& state, const std::string& rejectionReason);

    /// <returns>true if the run should be aborted.</returns>
    bool isAbortRequested() const;

    /// <returns>Status text for Backend Controls.</returns>
    const std::string& getStatus() const;

private:
    // Private constructor prevents external instantiation
    HealthMonitor() = default;
    ~HealthMonitor() = default;

    std::string status_;
    bool abort_ = false;
};