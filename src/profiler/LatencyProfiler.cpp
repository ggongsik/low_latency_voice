#include "profiler/LatencyProfiler.h"

#include <algorithm>

namespace llvc::profiler {

void LatencyProfiler::record(std::string stage, double durationMs) {
  auto& entry = stages_[std::move(stage)];
  const auto previousTotal = entry.averageMs * static_cast<double>(entry.samples);
  entry.samples += 1;
  entry.averageMs = (previousTotal + durationMs) / static_cast<double>(entry.samples);
  entry.maxMs = std::max(entry.maxMs, durationMs);
}

LatencyStats LatencyProfiler::stats(const std::string& stage) const {
  const auto found = stages_.find(stage);
  if (found == stages_.end()) {
    return {};
  }
  return found->second;
}

double LatencyProfiler::totalAverageMs() const {
  double total = 0.0;
  for (const auto& stage : stages_) {
    total += stage.second.averageMs;
  }
  return total;
}

std::size_t LatencyProfiler::stageCount() const noexcept { return stages_.size(); }

void LatencyProfiler::reset() { stages_.clear(); }

ScopedStageTimer::ScopedStageTimer(LatencyProfiler& profiler, std::string stage)
    : profiler_(profiler), stage_(std::move(stage)), start_(LatencyProfiler::Clock::now()) {}

ScopedStageTimer::~ScopedStageTimer() {
  const auto end = LatencyProfiler::Clock::now();
  const auto elapsed = std::chrono::duration<double, std::milli>(end - start_).count();
  profiler_.record(std::move(stage_), elapsed);
}

} // namespace llvc::profiler
