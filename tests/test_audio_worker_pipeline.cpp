#include "audio/AudioWorkerPipeline.h"
#include "common/Threading.h"

#include <chrono>
#include <cmath>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool nearlyEqual(float lhs, float rhs) { return std::fabs(lhs - rhs) < 0.00001F; }

template <typename Predicate> bool waitUntil(Predicate predicate) {
  for (int attempt = 0; attempt < 200; ++attempt) {
    if (predicate()) {
      return true;
    }
    llvc::common::sleepFor(std::chrono::milliseconds(1));
  }
  return false;
}

} // namespace

namespace llvc::tests {

void testAudioWorkerPipeline() {
  audio::AudioWorkerPipeline pipeline;

  const float firstInput[] = {0.1F, -0.2F, 0.3F, -0.4F};
  const float secondInput[] = {0.8F, 0.7F, 0.6F, 0.5F};
  const float* firstInputs[] = {firstInput};
  const float* secondInputs[] = {secondInput};
  float outputLeft[] = {0.0F, 0.0F, 0.0F, 0.0F};
  float outputRight[] = {0.0F, 0.0F, 0.0F, 0.0F};
  float* outputs[] = {outputLeft, outputRight};

  const auto usedWorkerWhileStopped =
      pipeline.processReplacingFromAudioThread(firstInputs, outputs, 1, 2, 4);
  require(!usedWorkerWhileStopped, "stopped worker should use dry fallback");
  for (int i = 0; i < 4; ++i) {
    require(nearlyEqual(outputLeft[i], firstInput[i]), "fallback left output mismatch");
    require(nearlyEqual(outputRight[i], firstInput[i]), "fallback right output mismatch");
  }

  auto stats = pipeline.stats();
  require(stats.submittedBlocks == 1, "submitted count mismatch");
  require(stats.lateOutputBlocks == 1, "late output should be counted");

  pipeline.clearQueuesWhenStopped();
  pipeline.resetCounters();
  pipeline.start();
  require(pipeline.running(), "pipeline should be running");

  const auto firstPopUsedWorker =
      pipeline.processReplacingFromAudioThread(firstInputs, outputs, 1, 2, 4);
  require(!firstPopUsedWorker, "first callback should fall back until worker output exists");

  require(waitUntil([&pipeline] {
            return pipeline.stats().workerProcessedBlocks >= 1 &&
                   pipeline.stats().outputQueueSize >= 1;
          }),
          "worker did not process first block");

  const auto secondPopUsedWorker =
      pipeline.processReplacingFromAudioThread(secondInputs, outputs, 1, 2, 4);
  require(secondPopUsedWorker, "second callback should consume processed worker output");

  for (int i = 0; i < 4; ++i) {
    require(nearlyEqual(outputLeft[i], firstInput[i]), "worker left output mismatch");
    require(nearlyEqual(outputRight[i], firstInput[i]), "worker right output mismatch");
  }

  pipeline.stop();
  require(!pipeline.running(), "pipeline should stop");

  stats = pipeline.stats();
  require(stats.consumedOutputBlocks >= 1, "consumed output should be counted");
  require(stats.droppedInputBlocks == 0, "input should not be dropped in nominal path");
  require(stats.lastWorkerProcessUs >= 0, "last worker process time should be available");
  require(stats.averageWorkerProcessUs >= 0, "average worker process time should be available");
  require(stats.maxWorkerProcessUs >= stats.lastWorkerProcessUs,
          "max worker process time should cover last time");
}

} // namespace llvc::tests
