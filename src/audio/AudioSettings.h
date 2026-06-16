#pragma once

#include <cstddef>

namespace llvc::audio {

struct AudioSettings {
  double sampleRate = 48000.0;
  std::size_t blockSize = 128;
  std::size_t inputChannels = 1;
  std::size_t outputChannels = 2;
  bool bypass = true;
};

} // namespace llvc::audio
