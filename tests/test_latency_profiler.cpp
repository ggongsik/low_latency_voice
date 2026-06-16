#include "profiler/LatencyProfiler.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool nearlyEqual(double lhs, double rhs) {
  const auto delta = lhs - rhs;
  return delta < 0.00001 && delta > -0.00001;
}

} // namespace

namespace llvc::tests {

void testLatencyProfiler() {
  profiler::LatencyProfiler profiler(2);
  profiler.record("input", 1.0);
  profiler.record("input", 3.0);
  profiler.record("input", 7.0);
  profiler.record("inference", 10.0);

  const auto input = profiler.stats("input");
  require(input.samples == 3, "input sample count mismatch");
  require(nearlyEqual(input.lastMs, 7.0), "input last mismatch");
  require(nearlyEqual(input.averageMs, 11.0 / 3.0), "input average mismatch");
  require(nearlyEqual(input.movingAverageMs, 5.0), "input moving average mismatch");
  require(nearlyEqual(input.p95Ms, 6.6), "input p95 mismatch");
  require(nearlyEqual(input.minMs, 1.0), "input min mismatch");
  require(nearlyEqual(input.maxMs, 7.0), "input max mismatch");
  require(profiler.stageCount() == 2, "stage count mismatch");
  require(nearlyEqual(profiler.totalAverageMs(), (11.0 / 3.0) + 10.0),
          "total average mismatch");
  require(!profiler.stageNames().empty(), "stage names should not be empty");

  const auto report = profiler.formatReport();
  require(report.find("Latency Report") != std::string::npos, "report title missing");
  require(report.find("input") != std::string::npos, "report stage missing");

  const char* csvPath = "latency_profiler_test.csv";
  const auto csvResult = profiler.writeCsv(csvPath);
  require(csvResult.succeeded(), "CSV export should succeed");

  std::ifstream csv(csvPath);
  std::string csvBody((std::istreambuf_iterator<char>(csv)),
                      std::istreambuf_iterator<char>());
  require(csvBody.find("stage,samples") != std::string::npos, "CSV header missing");
  require(csvBody.find("input,3") != std::string::npos, "CSV input row missing");
  std::remove(csvPath);

  profiler.reset();
  require(profiler.stageCount() == 0, "reset should clear stages");
}

} // namespace llvc::tests
