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
  backendErrorBlocks_.store(0, std::memory_order_relaxed);
  lastWorkerProcessUs_.store(0, std::memory_order_relaxed);
  totalWorkerProcessUs_.store(0, std::memory_order_relaxed);
  averageWorkerProcessUs_.store(0, std::memory_order_relaxed);
  maxWorkerProcessUs_.store(0, std::memory_order_relaxed);
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

void AudioWorkerPipeline::setSampleRate(double sampleRate) noexcept {
  if (sampleRate > 0.0) {
    sampleRate_.store(sampleRate, std::memory_order_relaxed);
  }
}

void AudioWorkerPipeline::setDummyProcessingDelay(std::chrono::microseconds delay) noexcept {
  dummyProcessingDelayUs_.store(delay.count(), std::memory_order_relaxed);
}

bool AudioWorkerPipeline::setInferenceBackend(
    inference::IVoiceConversionBackend* backend) noexcept {
  if (running()) {
    return false;
  }

  backend_ = backend;
  return true;
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
  snapshot.lastWorkerProcessUs = lastWorkerProcessUs_.load(std::memory_order_relaxed);
  snapshot.averageWorkerProcessUs = averageWorkerProcessUs_.load(std::memory_order_relaxed);
  snapshot.maxWorkerProcessUs = maxWorkerProcessUs_.load(std::memory_order_relaxed);
  snapshot.backendErrorBlocks = backendErrorBlocks_.load(std::memory_order_relaxed);
  snapshot.backendAttached = backend_ != nullptr;
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
  AudioBlock output;
  while (running_.load(std::memory_order_acquire)) {
    if (inputQueue_.empty()) {
      common::sleepFor(std::chrono::microseconds(250));
      continue;
    }

    if (!inputQueue_.tryPop(input)) {
      continue;
    }

    const auto started = std::chrono::steady_clock::now();
    const auto delayUs = dummyProcessingDelayUs_.load(std::memory_order_relaxed);
    if (delayUs > 0) {
      common::sleepFor(std::chrono::microseconds(delayUs));
    }

    if (backend_ != nullptr) {
      if (!processWithBackend(input, output)) {
        output = input;
      }
    } else {
      output = input;
    }

    if (!outputQueue_.tryPush(output)) {
      droppedOutputBlocks_.fetch_add(1, std::memory_order_relaxed);
    }

    const auto finished = std::chrono::steady_clock::now();
    const auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(
                               finished - started)
                               .count();
    lastWorkerProcessUs_.store(elapsedUs, std::memory_order_relaxed);
    const auto totalUs =
        totalWorkerProcessUs_.fetch_add(elapsedUs, std::memory_order_relaxed) + elapsedUs;
    const auto processedBlocks =
        workerProcessedBlocks_.fetch_add(1, std::memory_order_relaxed) + 1;
    averageWorkerProcessUs_.store(totalUs / static_cast<std::int64_t>(processedBlocks),
                                  std::memory_order_relaxed);

    auto previousMax = maxWorkerProcessUs_.load(std::memory_order_relaxed);
    while (elapsedUs > previousMax &&
           !maxWorkerProcessUs_.compare_exchange_weak(previousMax, elapsedUs,
                                                      std::memory_order_relaxed)) {
    }
  }
}

bool AudioWorkerPipeline::processWithBackend(const AudioBlock& input, AudioBlock& output) {
  auto* backend = backend_;
  if (backend == nullptr) {
    return false;
  }

  copyBlockToChunk(input, backendInputScratch_);
  const auto result = backend->process(backendInputScratch_, backendOutputScratch_);
  if (!result) {
    backendErrorBlocks_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  if (!copyChunkToBlock(backendOutputScratch_, input.sequence, output)) {
    backendErrorBlocks_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  return true;
}

bool AudioWorkerPipeline::tryConsumeOutputFromAudioThread(AudioBlock& block) noexcept {
  if (!outputQueue_.tryPop(block)) {
    lateOutputBlocks_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  consumedOutputBlocks_.fetch_add(1, std::memory_order_relaxed);
  return true;
}

void AudioWorkerPipeline::copyBlockToChunk(const AudioBlock& block,
                                           common::AudioChunk& chunk) const {
  chunk.sampleRate = sampleRate_.load(std::memory_order_relaxed);
  chunk.channels = block.channels;
  chunk.frames = block.frames;
  chunk.samples.resize(block.channels * block.frames);

  for (std::size_t channel = 0; channel < block.channels; ++channel) {
    const float* source = block.channelData(channel);
    auto* destination = chunk.samples.data() + (channel * block.frames);
    std::copy(source, source + block.frames, destination);
  }
}

bool AudioWorkerPipeline::copyChunkToBlock(const common::AudioChunk& chunk,
                                           std::uint64_t sequence,
                                           AudioBlock& block) noexcept {
  if (!AudioBlock::supports(chunk.channels, chunk.frames) ||
      chunk.samples.size() < chunk.sampleCount()) {
    return false;
  }

  block.channels = chunk.channels;
  block.frames = chunk.frames;
  block.sequence = sequence;

  for (std::size_t channel = 0; channel < block.channels; ++channel) {
    const auto* source = chunk.samples.data() + (channel * block.frames);
    float* destination = block.channelData(channel);
    std::copy(source, source + block.frames, destination);
  }

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
