#pragma once

#include <cstddef>
#include <vector>

namespace llvc::common {

struct AudioChunk {
  double sampleRate = 48000.0;
  std::size_t channels = 0;
  std::size_t frames = 0;
  std::vector<float> samples;

  std::size_t sampleCount() const noexcept { return channels * frames; }
  bool empty() const noexcept { return sampleCount() == 0; }
};

} // namespace llvc::common
