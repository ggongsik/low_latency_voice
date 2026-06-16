#include "audio/AudioEngine.h"

#include <algorithm>

namespace llvc::audio {

void AudioEngine::configure(const AudioSettings& settings) noexcept {
  sampleRate_.store(settings.sampleRate, std::memory_order_relaxed);
  blockSize_.store(settings.blockSize, std::memory_order_relaxed);
  inputChannels_.store(settings.inputChannels, std::memory_order_relaxed);
  outputChannels_.store(settings.outputChannels, std::memory_order_relaxed);
  bypassEnabled_.store(settings.bypass, std::memory_order_relaxed);
}

void AudioEngine::setBypass(bool enabled) noexcept {
  bypassEnabled_.store(enabled, std::memory_order_relaxed);
}

void AudioEngine::resetCounters() noexcept {
  processedBlocks_.store(0, std::memory_order_relaxed);
  xrunCount_.store(0, std::memory_order_relaxed);
}

AudioSettings AudioEngine::settings() const noexcept {
  AudioSettings current;
  current.sampleRate = sampleRate();
  current.blockSize = blockSize();
  current.inputChannels = inputChannels_.load(std::memory_order_relaxed);
  current.outputChannels = outputChannels_.load(std::memory_order_relaxed);
  current.bypass = bypassEnabled();
  return current;
}

bool AudioEngine::bypassEnabled() const noexcept {
  return bypassEnabled_.load(std::memory_order_relaxed);
}

double AudioEngine::sampleRate() const noexcept {
  return sampleRate_.load(std::memory_order_relaxed);
}

std::size_t AudioEngine::blockSize() const noexcept {
  return blockSize_.load(std::memory_order_relaxed);
}

double AudioEngine::estimatedBlockLatencyMs() const noexcept {
  const auto rate = sampleRate();
  if (rate <= 0.0) {
    return 0.0;
  }
  return 1000.0 * static_cast<double>(blockSize()) / rate;
}

std::uint64_t AudioEngine::processedBlocks() const noexcept {
  return processedBlocks_.load(std::memory_order_relaxed);
}

std::uint64_t AudioEngine::xrunCount() const noexcept {
  return xrunCount_.load(std::memory_order_relaxed);
}

void AudioEngine::noteXrun() noexcept { xrunCount_.fetch_add(1, std::memory_order_relaxed); }

void AudioEngine::processPassThrough(const float* const* inputs, float* const* outputs,
                                     std::size_t inputChannels, std::size_t outputChannels,
                                     std::size_t frames) noexcept {
  if (outputs == nullptr) {
    noteXrun();
    return;
  }

  const auto shouldBypass = bypassEnabled();
  const auto hasInput = inputs != nullptr && inputChannels > 0;

  for (std::size_t channel = 0; channel < outputChannels; ++channel) {
    float* output = outputs[channel];
    if (output == nullptr) {
      noteXrun();
      continue;
    }

    if (shouldBypass && hasInput) {
      const auto sourceChannel =
          inputChannels == 1 ? std::size_t{0} : std::min(channel, inputChannels - 1);
      const float* input = inputs[sourceChannel];
      if (input == nullptr) {
        std::fill(output, output + frames, 0.0F);
        noteXrun();
        continue;
      }

      std::copy(input, input + frames, output);
    } else {
      std::fill(output, output + frames, 0.0F);
    }
  }

  processedBlocks_.fetch_add(1, std::memory_order_relaxed);
}

} // namespace llvc::audio
