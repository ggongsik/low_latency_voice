#include "audio/SpscRingBuffer.h"
#include "profiler/LatencyProfiler.h"

#include <iostream>

namespace llvc::benchmarks {

void runAudioPipelineBenchmark() {
  audio::SpscRingBuffer<float, 128> buffer;
  profiler::LatencyProfiler profiler;

  {
    profiler::ScopedStageTimer timer(profiler, "ring_push_pop");
    for (int i = 0; i < 128; ++i) {
      const auto sample = static_cast<float>(i);
      (void)buffer.tryPush(sample);
    }

    float sample = 0.0F;
    while (buffer.tryPop(sample)) {
    }
  }

  const auto stats = profiler.stats("ring_push_pop");
  std::cout << "ring_push_pop avg_ms=" << stats.averageMs << " max_ms=" << stats.maxMs
            << " samples=" << stats.samples << '\n';
}

} // namespace llvc::benchmarks
