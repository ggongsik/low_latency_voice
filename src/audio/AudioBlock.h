#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace llvc::audio {

constexpr std::size_t kMaxRealtimeChannels = 2;
constexpr std::size_t kMaxRealtimeFrames = 2048;

struct AudioBlock {
  std::size_t channels = 0;
  std::size_t frames = 0;
  std::uint64_t sequence = 0;
  std::array<float, kMaxRealtimeChannels * kMaxRealtimeFrames> samples{};

  static bool supports(std::size_t channelCount, std::size_t frameCount) noexcept {
    return channelCount > 0 && channelCount <= kMaxRealtimeChannels && frameCount > 0 &&
           frameCount <= kMaxRealtimeFrames;
  }

  void clear() noexcept {
    channels = 0;
    frames = 0;
    sequence = 0;
    std::fill(samples.begin(), samples.end(), 0.0F);
  }

  float* channelData(std::size_t channel) noexcept {
    return samples.data() + channel * kMaxRealtimeFrames;
  }

  const float* channelData(std::size_t channel) const noexcept {
    return samples.data() + channel * kMaxRealtimeFrames;
  }

  bool copyFrom(const float* const* inputs, std::size_t inputChannels, std::size_t frameCount,
                std::uint64_t blockSequence) noexcept {
    if (inputs == nullptr || !supports(inputChannels, frameCount)) {
      return false;
    }

    channels = inputChannels;
    frames = frameCount;
    sequence = blockSequence;

    for (std::size_t channel = 0; channel < channels; ++channel) {
      float* destination = channelData(channel);
      const float* source = inputs[channel];
      if (source == nullptr) {
        std::fill(destination, destination + frames, 0.0F);
      } else {
        std::copy(source, source + frames, destination);
      }
    }

    return true;
  }

  bool copyTo(float* const* outputs, std::size_t outputChannels,
              std::size_t outputFrames) const noexcept {
    if (outputs == nullptr || outputChannels == 0 || outputFrames == 0 || channels == 0 ||
        frames == 0) {
      return false;
    }

    const auto framesToCopy = std::min(frames, outputFrames);
    for (std::size_t channel = 0; channel < outputChannels; ++channel) {
      float* destination = outputs[channel];
      if (destination == nullptr) {
        continue;
      }

      const auto sourceChannel =
          channels == 1 ? std::size_t{0} : std::min(channel, channels - 1);
      const float* source = channelData(sourceChannel);
      std::copy(source, source + framesToCopy, destination);

      if (framesToCopy < outputFrames) {
        std::fill(destination + framesToCopy, destination + outputFrames, 0.0F);
      }
    }

    return true;
  }
};

} // namespace llvc::audio
