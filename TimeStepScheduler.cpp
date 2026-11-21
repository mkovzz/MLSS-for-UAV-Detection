#include "Time_and_step_scheduler.h"

/// <summary>
/// Applies run timing and storage controls, clamping invalid step/rate values.
/// </summary>
/// <param name="start">Start time in seconds.</param>
/// <param name="end">End time in seconds.</param>
/// <param name="step">Step size in seconds.</param>
/// <param name="rate">Number of steps between stored samples.</param>
void TimeStepScheduler::configure(double start, double end, double step, int rate) {
    startTime_s_ = start;
    endTime_s_ = end;
    stepSize_s_ = step > 0.0 ? step : 1.0;
    storageRate_ = rate < 1 ? 1 : rate;
    currentTime_s_ = start;
    paused_ = false;
    stopped_ = false;
}

/// <summary>
/// Advances one simulation step and reports whether the run should continue.
/// </summary>
/// <returns>false when the run has reached its end or was stopped.</returns>
bool TimeStepScheduler::advance() {
    if (stopped_) return false;
    if (paused_) return true;
    currentTime_s_ += stepSize_s_;
    return currentTime_s_ < endTime_s_;
}

/// <summary>
/// Pauses time advancement without ending the run.
/// </summary>
void TimeStepScheduler::pause() { paused_ = true; }
/// <summary>
/// Resumes time advancement.
/// </summary>
void TimeStepScheduler::resume() { paused_ = false; }
/// <summary>
/// Permanently stops time advancement.
/// </summary>
void TimeStepScheduler::stop() { stopped_ = true; }
/// <summary>
/// Returns the current simulation time.
/// </summary>
/// <returns>Current simulation time in seconds.</returns>
double TimeStepScheduler::getTime() const { return currentTime_s_; }
/// <summary>
/// Returns the configured start time.
/// </summary>
/// <returns>Start time in seconds.</returns>
double TimeStepScheduler::getStartTime() const { return startTime_s_; }
/// <summary>
/// Returns the configured end time.
/// </summary>
/// <returns>End time in seconds.</returns>
double TimeStepScheduler::getEndTime() const { return endTime_s_; }
/// <summary>
/// Returns the configured step size.
/// </summary>
/// <returns>Step size in seconds.</returns>
double TimeStepScheduler::getStepSize() const { return stepSize_s_; }
/// <summary>
/// Returns the configured storage rate.
/// </summary>
/// <returns>Number of steps between stored samples.</returns>
int TimeStepScheduler::getStorageRate() const { return storageRate_; }
