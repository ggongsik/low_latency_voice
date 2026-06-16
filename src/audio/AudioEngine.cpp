#include "audio/AudioEngine.h"

#include <algorithm>

namespace llvc::audio {

void AudioEngine::configure(const AudioSettings& settings) noexcept { settings_ = settings; }

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
  const auto channelsToCopy = std::min(inputChannels, outputChannels);

  for (std::size_t channel = 0; channel < channelsToCopy; ++channel) {
    const float* input = inputs != nullptr ? inputs[channel] : nullptr;
    float* output = outputs != nullptr ? outputs[channel] : nullptr;
    if (output == nullptr) {
      continue;
    }

    if (settings_.bypass && input != nullptr) {
      std::copy(input, input + frames, output);
    } else {
      std::fill(output, output + frames, 0.0F);
    }
  }

  for (std::size_t channel = channelsToCopy; channel < outputChannels; ++channel) {
    float* output = outputs != nullptr ? outputs[channel] : nullptr;
    if (output != nullptr) {
      std::fill(output, output + frames, 0.0F);
    }
  }

  processedBlocks_.fetch_add(1, std::memory_order_relaxed);
}

} // namespace llvc::audio
