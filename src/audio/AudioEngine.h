#pragma once

#include "audio/AudioSettings.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace llvc::audio {

class AudioEngine {
public:
  void configure(const AudioSettings& settings) noexcept;

  const AudioSettings& settings() const noexcept { return settings_; }
  std::uint64_t processedBlocks() const noexcept;
  std::uint64_t xrunCount() const noexcept;

  void noteXrun() noexcept;

  void processPassThrough(const float* const* inputs, float* const* outputs,
                          std::size_t inputChannels, std::size_t outputChannels,
                          std::size_t frames) noexcept;

private:
  AudioSettings settings_{};
  std::atomic<std::uint64_t> processedBlocks_{0};
  std::atomic<std::uint64_t> xrunCount_{0};
};

} // namespace llvc::audio
