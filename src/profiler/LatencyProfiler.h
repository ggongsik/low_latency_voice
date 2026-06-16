#pragma once

#include "common/Result.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace llvc::profiler {

struct LatencyStats {
  double lastMs = 0.0;
  double averageMs = 0.0;
  double movingAverageMs = 0.0;
  double p95Ms = 0.0;
  double minMs = 0.0;
  double maxMs = 0.0;
  std::uint64_t samples = 0;
};

class LatencyProfiler {
public:
  using Clock = std::chrono::steady_clock;

  explicit LatencyProfiler(std::size_t movingWindowSize = 64);

  void record(std::string stage, double durationMs);
  LatencyStats stats(const std::string& stage) const;
  std::vector<std::string> stageNames() const;
  double totalAverageMs() const;
  double totalMovingAverageMs() const;
  std::size_t stageCount() const noexcept;
  std::string formatReport(const std::string& title = "Latency Report") const;
  common::Result writeCsv(const std::string& path) const;
  void reset();

private:
  struct StageData {
    LatencyStats stats;
    std::vector<double> samples;
  };

  static double percentile(std::vector<double> samples, double percentileValue);
  void refreshDerivedStats(StageData& data) const;

  std::size_t movingWindowSize_ = 64;
  std::map<std::string, StageData, std::less<>> stages_;
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
