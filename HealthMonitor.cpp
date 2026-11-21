#include "HealthMonitor.h"

/// <summary>
/// Requests an abort before the simulation starts.
/// </summary>
/// <param name="reason">Explanation for rejecting the run.</param>
void HealthMonitor::reportEarlyRejection(const std::string& reason) { abort_ = true; status_ = reason; }
/// <summary>
/// Records a satellite health failure and requests an abort when needed.
/// </summary>
/// <param name="reason">Health failure explanation.</param>
void HealthMonitor::submitSatelliteState(const EciState&, const std::string& reason) { if (!reason.empty()) { abort_ = true; status_ = reason; } }
/// <summary>
/// Returns whether a component has requested an abort.
/// </summary>
/// <returns>true when the simulation should abort.</returns>
bool HealthMonitor::isAbortRequested() const { return abort_; }
/// <summary>
/// Returns the most recent health status text.
/// </summary>
/// <returns>The current health status.</returns>
const std::string& HealthMonitor::getStatus() const { return status_; }
