#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

namespace llvc::profiler {

struct LatencyStats {
  double averageMs = 0.0;
  double maxMs = 0.0;
  std::uint64_t samples = 0;
};

class LatencyProfiler {
public:
  using Clock = std::chrono::steady_clock;

  void record(std::string stage, double durationMs);
  LatencyStats stats(const std::string& stage) const;
  double totalAverageMs() const;
  std::size_t stageCount() const noexcept;
  void reset();

private:
  std::map<std::string, LatencyStats, std::less<>> stages_;
};

class ScopedStageTimer {
public:
  ScopedStageTimer(LatencyProfiler& profiler, std::string stage);
  ~ScopedStageTimer();

  ScopedStageTimer(const ScopedStageTimer&) = delete;
  ScopedStageTimer& operator=(const ScopedStageTimer&) = delete;

private:
  LatencyProfiler& profiler_;
  std::string stage_;
  LatencyProfiler::Clock::time_point start_;
};

} // namespace llvc::profiler
