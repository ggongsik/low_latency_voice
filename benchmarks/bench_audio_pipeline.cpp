#include "audio/SpscRingBuffer.h"
#include "audio/AudioWorkerPipeline.h"
#include "common/Threading.h"
#include "profiler/LatencyProfiler.h"

#include <array>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>

namespace llvc::benchmarks {

namespace {

bool waitForWorker(audio::AudioWorkerPipeline& pipeline, std::uint64_t submittedBlocks) {
  for (int attempt = 0; attempt < 200; ++attempt) {
    if (pipeline.stats().workerProcessedBlocks >= submittedBlocks) {
      return true;
    }
    common::sleepFor(std::chrono::microseconds(500));
  }
  return false;
}

} // namespace

void runAudioPipelineBenchmark(std::size_t iterations, long dummyDelayUs,
                               const std::string& csvPath) {
  audio::SpscRingBuffer<float, 128> buffer;
  audio::AudioWorkerPipeline workerPipeline;
  profiler::LatencyProfiler profiler(64);

  for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
    profiler::ScopedStageTimer timer(profiler, "ring_push_pop");
    for (int i = 0; i < 128; ++i) {
      const auto sample = static_cast<float>(i);
      (void)buffer.tryPush(sample);
    }

    float sample = 0.0F;
    while (buffer.tryPop(sample)) {
    }
  }

  std::array<float, 128> monoInput{};
  for (std::size_t index = 0; index < monoInput.size(); ++index) {
    monoInput[index] = static_cast<float>(index) / static_cast<float>(monoInput.size());
  }

  const float* inputs[] = {monoInput.data()};
  workerPipeline.setDummyProcessingDelay(std::chrono::microseconds(dummyDelayUs));
  workerPipeline.start();

  for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
    {
      profiler::ScopedStageTimer timer(profiler, "worker_shadow_callback");
      (void)workerPipeline.processShadowFromAudioThread(inputs, 1, monoInput.size());
    }

    common::sleepFor(std::chrono::microseconds(std::max<long>(100, dummyDelayUs)));
  }

  (void)waitForWorker(workerPipeline, iterations);
  const auto workerStats = workerPipeline.stats();
  workerPipeline.stop();

  if (workerStats.workerProcessedBlocks > 0) {
    profiler.record("worker_process_observed",
                    static_cast<double>(workerStats.averageWorkerProcessUs) / 1000.0);
  }

  std::cout << profiler.formatReport("Audio Pipeline Benchmark");
  std::cout << "Worker submitted=" << workerStats.submittedBlocks
            << " processed=" << workerStats.workerProcessedBlocks
            << " consumed=" << workerStats.consumedOutputBlocks
            << " late=" << workerStats.lateOutputBlocks
            << " dummy_delay_us=" << workerStats.dummyProcessingDelayUs
            << " dropped_input=" << workerStats.droppedInputBlocks
            << " dropped_output=" << workerStats.droppedOutputBlocks << '\n';

  if (!csvPath.empty()) {
    const auto result = profiler.writeCsv(csvPath);
    if (result) {
      std::cout << "CSV written: " << csvPath << '\n';
    } else {
      std::cout << "CSV export failed: " << result.message() << '\n';
    }
  }
}

} // namespace llvc::benchmarks
