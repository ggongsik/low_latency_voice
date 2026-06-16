#include "profiler/LatencyProfiler.h"

#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

} // namespace

namespace llvc::tests {

void testLatencyProfiler() {
  profiler::LatencyProfiler profiler;
  profiler.record("input", 1.0);
  profiler.record("input", 3.0);
  profiler.record("inference", 10.0);

  const auto input = profiler.stats("input");
  require(input.samples == 2, "input sample count mismatch");
  require(input.averageMs == 2.0, "input average mismatch");
  require(input.maxMs == 3.0, "input max mismatch");
  require(profiler.stageCount() == 2, "stage count mismatch");
  require(profiler.totalAverageMs() == 12.0, "total average mismatch");

  profiler.reset();
  require(profiler.stageCount() == 0, "reset should clear stages");
}

} // namespace llvc::tests
