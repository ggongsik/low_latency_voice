#pragma once

#include "audio/AudioSettings.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace llvc::audio {

class AudioEngine {
public:
  void configure(const AudioSettings& settings) noexcept;
  void setBypass(bool enabled) noexcept;
  void resetCounters() noexcept;

  AudioSettings settings() const noexcept;
  bool bypassEnabled() const noexcept;
  double sampleRate() const noexcept;
  std::size_t blockSize() const noexcept;
  double estimatedBlockLatencyMs() const noexcept;
  std::uint64_t processedBlocks() const noexcept;
  std::uint64_t xrunCount() const noexcept;

  void noteXrun() noexcept;

  void processPassThrough(const float* const* inputs, float* const* outputs,
                          std::size_t inputChannels, std::size_t outputChannels,
                          std::size_t frames) noexcept;

private:
  std::atomic<double> sampleRate_{48000.0};
  std::atomic<std::size_t> blockSize_{128};
  std::atomic<std::size_t> inputChannels_{1};
  std::atomic<std::size_t> outputChannels_{2};
  std::atomic<bool> bypassEnabled_{true};
  std::atomic<std::uint64_t> processedBlocks_{0};
  std::atomic<std::uint64_t> xrunCount_{0};
};

} // namespace llvc::audio
