#include "inference/AudioTensorAdapter.h"

#include <algorithm>
#include <string>

namespace llvc::inference {

namespace {

common::Result validateShape(const AudioTensorShape& shape) {
  if (!shape.valid()) {
    return common::Result::error("audio tensor shape must be [1, channels, frames]");
  }
  return common::Result::ok();
}

common::Result validateChunkMatchesShape(const common::AudioChunk& chunk,
                                         const AudioTensorShape& shape) {
  const auto shapeResult = validateShape(shape);
  if (!shapeResult) {
    return shapeResult;
  }

  if (chunk.channels != shape.channels || chunk.frames != shape.frames) {
    return common::Result::error("audio chunk shape does not match fixed tensor shape");
  }
  if (chunk.samples.size() < chunk.sampleCount()) {
    return common::Result::error("audio chunk sample buffer is smaller than channels * frames");
  }
  return common::Result::ok();
}

} // namespace

std::vector<std::int64_t> AudioTensorShape::toInt64Vector() const {
  return {static_cast<std::int64_t>(batch), static_cast<std::int64_t>(channels),
          static_cast<std::int64_t>(frames)};
}

common::Result copyAudioChunkToTensor(const common::AudioChunk& input,
                                      const AudioTensorShape& shape,
                                      std::vector<float>& tensor) {
  const auto validation = validateChunkMatchesShape(input, shape);
  if (!validation) {
    return validation;
  }

  tensor.resize(shape.sampleCount());
  std::copy(input.samples.begin(), input.samples.begin() + input.sampleCount(), tensor.begin());
  return common::Result::ok();
}

common::Result copyTensorToAudioChunk(const float* tensorData, std::size_t tensorSamples,
                                      const AudioTensorShape& shape, double sampleRate,
                                      common::AudioChunk& output) {
  const auto shapeResult = validateShape(shape);
  if (!shapeResult) {
    return shapeResult;
  }
  if (tensorData == nullptr) {
    return common::Result::error("tensor data pointer is null");
  }
  if (tensorSamples < shape.sampleCount()) {
    return common::Result::error("tensor sample count is smaller than fixed tensor shape");
  }

  output.sampleRate = sampleRate;
  output.channels = shape.channels;
  output.frames = shape.frames;
  output.samples.assign(tensorData, tensorData + shape.sampleCount());
  return common::Result::ok();
}

} // namespace llvc::inference
