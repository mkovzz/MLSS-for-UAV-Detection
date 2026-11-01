#include "Time_and_step_scheduler.h"

void TimeStepScheduler::configure(double start, double end, double step, int rate) {
    startTime_s_ = start;
    endTime_s_ = end;
    stepSize_s_ = step > 0.0 ? step : 1.0;
    storageRate_ = rate < 1 ? 1 : rate;
    currentTime_s_ = start;
    paused_ = false;
    stopped_ = false;
}

bool TimeStepScheduler::advance() {
    if (stopped_) return false;
    if (paused_) return true;
    currentTime_s_ += stepSize_s_;
    return currentTime_s_ < endTime_s_;
}

void TimeStepScheduler::pause() { paused_ = true; }
void TimeStepScheduler::resume() { paused_ = false; }
void TimeStepScheduler::stop() { stopped_ = true; }
double TimeStepScheduler::getTime() const { return currentTime_s_; }
int TimeStepScheduler::getStorageRate() const { return storageRate_; }
