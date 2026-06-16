#pragma once

#include "common/AudioChunk.h"
#include "common/Result.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace llvc::inference {

struct BackendStats {
  double lastProcessMs = 0.0;
  double averageProcessMs = 0.0;
  std::uint64_t processedChunks = 0;
  std::uint64_t warmupRuns = 0;
  bool modelLoaded = false;
};

class IVoiceConversionBackend {
public:
  virtual ~IVoiceConversionBackend() = default;

  virtual common::Result loadModel(const std::string& path) = 0;
  virtual common::Result warmUp(const common::AudioChunk& input, std::size_t iterations);
  virtual common::Result process(const common::AudioChunk& input,
                                 common::AudioChunk& output) = 0;
  virtual BackendStats stats() const = 0;
};

} // namespace llvc::inference
