#pragma once

#include "common/AudioChunk.h"
#include "common/Result.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace llvc::inference {

struct AudioTensorShape {
  std::size_t batch = 1;
  std::size_t channels = 0;
  std::size_t frames = 0;

  std::size_t sampleCount() const noexcept { return batch * channels * frames; }
  bool valid() const noexcept { return batch == 1 && channels > 0 && frames > 0; }
  std::vector<std::int64_t> toInt64Vector() const;
};

common::Result copyAudioChunkToTensor(const common::AudioChunk& input,
                                      const AudioTensorShape& shape,
                                      std::vector<float>& tensor);

common::Result copyTensorToAudioChunk(const float* tensorData, std::size_t tensorSamples,
                                      const AudioTensorShape& shape, double sampleRate,
                                      common::AudioChunk& output);

} // namespace llvc::inference
