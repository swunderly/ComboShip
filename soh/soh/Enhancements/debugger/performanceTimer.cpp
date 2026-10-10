#include "performanceTimer.h"
#include <spdlog/spdlog.h>

void StartPerformanceTimer(TimerID timer) {
    // These measurements are only consumed by debug logs. Avoid a clock read for
    // every logic edge/check when that output is disabled (especially costly
    // through Android's ARM translation). Keep timing available at debug/trace.
    if (!spdlog::should_log(spdlog::level::debug)) {
        timeStarted[timer] = {};
        return;
    }
    timeStarted[timer] = std::chrono::high_resolution_clock::now();
}

void StopPerformanceTimer(TimerID timer) {
    if (timeStarted[timer] == std::chrono::high_resolution_clock::time_point{}) {
        return;
    }
    totalTimes[timer] += (std::chrono::high_resolution_clock::now() - timeStarted[timer]);
}

std::chrono::duration<double, std::milli> GetPerformanceTimer(TimerID timer) {
    return totalTimes[timer];
}

void ResetPerformanceTimer(TimerID timer) {
    totalTimes[timer] = {};
}

void ResetPerformanceTimers() {
    totalTimes = {};
}
