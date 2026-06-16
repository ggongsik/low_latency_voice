#include "profiler/LatencyProfiler.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>

namespace llvc::profiler {

LatencyProfiler::LatencyProfiler(std::size_t movingWindowSize)
    : movingWindowSize_(movingWindowSize == 0 ? 1 : movingWindowSize) {}

void LatencyProfiler::record(std::string stage, double durationMs) {
  const auto clampedDurationMs = std::max(0.0, durationMs);
  auto& entry = stages_[std::move(stage)];
  auto& stats = entry.stats;

  const auto previousTotal = stats.averageMs * static_cast<double>(stats.samples);
  stats.samples += 1;
  stats.lastMs = clampedDurationMs;
  stats.averageMs =
      (previousTotal + clampedDurationMs) / static_cast<double>(stats.samples);

  if (stats.samples == 1) {
    stats.minMs = clampedDurationMs;
    stats.maxMs = clampedDurationMs;
  } else {
    stats.minMs = std::min(stats.minMs, clampedDurationMs);
    stats.maxMs = std::max(stats.maxMs, clampedDurationMs);
  }

  entry.samples.push_back(clampedDurationMs);
  refreshDerivedStats(entry);
}

LatencyStats LatencyProfiler::stats(const std::string& stage) const {
  const auto found = stages_.find(stage);
  if (found == stages_.end()) {
    return {};
  }
  return found->second.stats;
}

std::vector<std::string> LatencyProfiler::stageNames() const {
  std::vector<std::string> names;
  names.reserve(stages_.size());
  for (const auto& stage : stages_) {
    names.push_back(stage.first);
  }
  return names;
}

double LatencyProfiler::totalAverageMs() const {
  double total = 0.0;
  for (const auto& stage : stages_) {
    total += stage.second.stats.averageMs;
  }
  return total;
}

double LatencyProfiler::totalMovingAverageMs() const {
  double total = 0.0;
  for (const auto& stage : stages_) {
    total += stage.second.stats.movingAverageMs;
  }
  return total;
}

std::size_t LatencyProfiler::stageCount() const noexcept { return stages_.size(); }

std::string LatencyProfiler::formatReport(const std::string& title) const {
  std::ostringstream report;
  report << '[' << title << "]\n";
  report << std::fixed << std::setprecision(3);

  for (const auto& stage : stages_) {
    const auto& stats = stage.second.stats;
    report << std::left << std::setw(24) << stage.first << " avg=" << std::setw(8)
           << stats.averageMs << " mov=" << std::setw(8) << stats.movingAverageMs
           << " p95=" << std::setw(8) << stats.p95Ms << " max=" << std::setw(8)
           << stats.maxMs << " samples=" << stats.samples << '\n';
  }

  report << std::left << std::setw(24) << "Estimated Total" << " avg=" << std::setw(8)
         << totalAverageMs() << " mov=" << std::setw(8) << totalMovingAverageMs()
         << '\n';
  return report.str();
}

common::Result LatencyProfiler::writeCsv(const std::string& path) const {
  std::ofstream output(path.c_str(), std::ios::out | std::ios::trunc);
  if (!output) {
    return common::Result::error("failed to open CSV file: " + path);
  }

  output << "stage,samples,last_ms,average_ms,moving_average_ms,p95_ms,min_ms,max_ms\n";
  output << std::fixed << std::setprecision(6);

  for (const auto& stage : stages_) {
    const auto& stats = stage.second.stats;
    output << stage.first << ',' << stats.samples << ',' << stats.lastMs << ','
           << stats.averageMs << ',' << stats.movingAverageMs << ',' << stats.p95Ms
           << ',' << stats.minMs << ',' << stats.maxMs << '\n';
  }

  if (!output) {
    return common::Result::error("failed to write CSV file: " + path);
  }

  return common::Result::ok();
}

void LatencyProfiler::reset() { stages_.clear(); }

double LatencyProfiler::percentile(std::vector<double> samples, double percentileValue) {
  if (samples.empty()) {
    return 0.0;
  }

  std::sort(samples.begin(), samples.end());
  const auto clampedPercentile = std::max(0.0, std::min(100.0, percentileValue));
  const auto position =
      (clampedPercentile / 100.0) * static_cast<double>(samples.size() - 1);
  const auto lowerIndex = static_cast<std::size_t>(position);
  const auto upperIndex = std::min(lowerIndex + 1, samples.size() - 1);
  const auto fraction = position - static_cast<double>(lowerIndex);
  return samples[lowerIndex] * (1.0 - fraction) + samples[upperIndex] * fraction;
}

void LatencyProfiler::refreshDerivedStats(StageData& data) const {
  const auto& samples = data.samples;
  if (samples.empty()) {
    data.stats.movingAverageMs = 0.0;
    data.stats.p95Ms = 0.0;
    return;
  }

  const auto sampleCount = samples.size();
  const auto window = std::min(movingWindowSize_, sampleCount);
  const auto begin = samples.end() - static_cast<std::ptrdiff_t>(window);
  const auto movingTotal = std::accumulate(begin, samples.end(), 0.0);
  data.stats.movingAverageMs = movingTotal / static_cast<double>(window);
  data.stats.p95Ms = percentile(samples, 95.0);
}

ScopedStageTimer::ScopedStageTimer(LatencyProfiler& profiler, std::string stage)
    : profiler_(profiler), stage_(std::move(stage)), start_(LatencyProfiler::Clock::now()) {}

ScopedStageTimer::~ScopedStageTimer() {
  const auto end = LatencyProfiler::Clock::now();
  const auto elapsed = std::chrono::duration<double, std::milli>(end - start_).count();
  profiler_.record(std::move(stage_), elapsed);
}

} // namespace llvc::profiler
