#include "audio/AudioWorkerPipeline.h"

#include <algorithm>

namespace llvc::audio {

AudioWorkerPipeline::~AudioWorkerPipeline() { stop(); }

void AudioWorkerPipeline::start() {
  bool expected = false;
  if (!running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
    return;
  }

  workerThread_.start([this] { workerLoop(); });
}

void AudioWorkerPipeline::stop() {
  const auto wasRunning = running_.exchange(false, std::memory_order_acq_rel);
  if ((wasRunning || workerThread_.joinable()) && workerThread_.joinable()) {
    workerThread_.join();
  }
}

void AudioWorkerPipeline::resetCounters() noexcept {
  submittedBlocks_.store(0, std::memory_order_relaxed);
  workerProcessedBlocks_.store(0, std::memory_order_relaxed);
  consumedOutputBlocks_.store(0, std::memory_order_relaxed);
  lateOutputBlocks_.store(0, std::memory_order_relaxed);
  droppedInputBlocks_.store(0, std::memory_order_relaxed);
  droppedOutputBlocks_.store(0, std::memory_order_relaxed);
  unsupportedInputBlocks_.store(0, std::memory_order_relaxed);
  nextSequence_.store(1, std::memory_order_relaxed);
}

void AudioWorkerPipeline::clearQueuesWhenStopped() noexcept {
  if (running()) {
    return;
  }

  inputQueue_.reset();
  outputQueue_.reset();
  audioThreadInputScratch_.clear();
  audioThreadOutputScratch_.clear();
}

void AudioWorkerPipeline::setDummyProcessingDelay(std::chrono::microseconds delay) noexcept {
  dummyProcessingDelayUs_.store(delay.count(), std::memory_order_relaxed);
}

bool AudioWorkerPipeline::running() const noexcept {
  return running_.load(std::memory_order_acquire);
}

AudioWorkerPipelineStats AudioWorkerPipeline::stats() const noexcept {
  AudioWorkerPipelineStats snapshot;
  snapshot.running = running();
  snapshot.submittedBlocks = submittedBlocks_.load(std::memory_order_relaxed);
  snapshot.workerProcessedBlocks = workerProcessedBlocks_.load(std::memory_order_relaxed);
  snapshot.consumedOutputBlocks = consumedOutputBlocks_.load(std::memory_order_relaxed);
  snapshot.lateOutputBlocks = lateOutputBlocks_.load(std::memory_order_relaxed);
  snapshot.droppedInputBlocks = droppedInputBlocks_.load(std::memory_order_relaxed);
  snapshot.droppedOutputBlocks = droppedOutputBlocks_.load(std::memory_order_relaxed);
  snapshot.unsupportedInputBlocks = unsupportedInputBlocks_.load(std::memory_order_relaxed);
  snapshot.inputQueueSize = inputQueue_.size();
  snapshot.outputQueueSize = outputQueue_.size();
  snapshot.inputQueueOverflows = inputQueue_.overflowCount();
  snapshot.outputQueueOverflows = outputQueue_.overflowCount();
  snapshot.dummyProcessingDelayUs = dummyProcessingDelayUs_.load(std::memory_order_relaxed);
  return snapshot;
}

bool AudioWorkerPipeline::submitInputFromAudioThread(const float* const* inputs,
                                                     std::size_t inputChannels,
                                                     std::size_t frames) noexcept {
  const auto sequence = nextSequence_.fetch_add(1, std::memory_order_relaxed);
  if (!audioThreadInputScratch_.copyFrom(inputs, inputChannels, frames, sequence)) {
    unsupportedInputBlocks_.fetch_add(1, std::memory_order_relaxed);
    droppedInputBlocks_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  submittedBlocks_.fetch_add(1, std::memory_order_relaxed);
  if (!inputQueue_.tryPush(audioThreadInputScratch_)) {
    droppedInputBlocks_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  return true;
}

bool AudioWorkerPipeline::processShadowFromAudioThread(const float* const* inputs,
                                                       std::size_t inputChannels,
                                                       std::size_t frames) noexcept {
  (void)submitInputFromAudioThread(inputs, inputChannels, frames);
  return tryConsumeOutputFromAudioThread(audioThreadOutputScratch_);
}

bool AudioWorkerPipeline::processReplacingFromAudioThread(const float* const* inputs,
                                                          float* const* outputs,
                                                          std::size_t inputChannels,
                                                          std::size_t outputChannels,
                                                          std::size_t frames) noexcept {
  (void)submitInputFromAudioThread(inputs, inputChannels, frames);

  if (tryConsumeOutputFromAudioThread(audioThreadOutputScratch_) &&
      audioThreadOutputScratch_.copyTo(outputs, outputChannels, frames)) {
    return true;
  }

  copyDryToOutput(inputs, outputs, inputChannels, outputChannels, frames);
  return false;
}

void AudioWorkerPipeline::workerLoop() {
  AudioBlock input;
  while (running_.load(std::memory_order_acquire)) {
    if (inputQueue_.empty()) {
      common::sleepFor(std::chrono::microseconds(250));
      continue;
    }

    if (!inputQueue_.tryPop(input)) {
      continue;
    }

    const auto delayUs = dummyProcessingDelayUs_.load(std::memory_order_relaxed);
    if (delayUs > 0) {
      common::sleepFor(std::chrono::microseconds(delayUs));
    }

    workerProcessedBlocks_.fetch_add(1, std::memory_order_relaxed);
    if (!outputQueue_.tryPush(input)) {
      droppedOutputBlocks_.fetch_add(1, std::memory_order_relaxed);
    }
  }
}

bool AudioWorkerPipeline::tryConsumeOutputFromAudioThread(AudioBlock& block) noexcept {
  if (!outputQueue_.tryPop(block)) {
    lateOutputBlocks_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  consumedOutputBlocks_.fetch_add(1, std::memory_order_relaxed);
  return true;
}

void AudioWorkerPipeline::copyDryToOutput(const float* const* inputs, float* const* outputs,
                                          std::size_t inputChannels,
                                          std::size_t outputChannels,
                                          std::size_t frames) noexcept {
  if (outputs == nullptr) {
    return;
  }

  const auto hasInput = inputs != nullptr && inputChannels > 0;
  for (std::size_t channel = 0; channel < outputChannels; ++channel) {
    float* output = outputs[channel];
    if (output == nullptr) {
      continue;
    }

    if (hasInput) {
      const auto sourceChannel =
          inputChannels == 1 ? std::size_t{0} : std::min(channel, inputChannels - 1);
      const float* input = inputs[sourceChannel];
      if (input != nullptr) {
        std::copy(input, input + frames, output);
        continue;
      }
    }

    std::fill(output, output + frames, 0.0F);
  }
}

} // namespace llvc::audio
