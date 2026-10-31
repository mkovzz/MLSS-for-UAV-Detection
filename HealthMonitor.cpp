#include "HealthMonitor.h"

void HealthMonitor::reportEarlyRejection(const std::string& reason) { abort_ = true; status_ = reason; }
void HealthMonitor::submitSatelliteState(const EciState&, const std::string& reason) { if (!reason.empty()) { abort_ = true; status_ = reason; } }
bool HealthMonitor::isAbortRequested() const { return abort_; }
const std::string& HealthMonitor::getStatus() const { return status_; }
