#include "inference/AudioTensorAdapter.h"

#include <cmath>
#include <stdexcept>
#include <vector>

namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool nearlyEqual(float lhs, float rhs) { return std::fabs(lhs - rhs) < 0.00001F; }

llvc::common::AudioChunk makeStereoChunk() {
  llvc::common::AudioChunk chunk;
  chunk.sampleRate = 48000.0;
  chunk.channels = 2;
  chunk.frames = 3;
  chunk.samples = {0.1F, 0.2F, 0.3F, -0.1F, -0.2F, -0.3F};
  return chunk;
}

} // namespace

namespace llvc::tests {

void testAudioTensorAdapter() {
  const auto input = makeStereoChunk();
  const inference::AudioTensorShape shape{1, 2, 3};
  const auto shapeVector = shape.toInt64Vector();
  require(shape.valid(), "shape should be valid");
  require(shape.sampleCount() == input.sampleCount(), "shape sample count mismatch");
  require(shapeVector.size() == 3, "shape vector rank mismatch");
  require(shapeVector[0] == 1 && shapeVector[1] == 2 && shapeVector[2] == 3,
          "shape vector value mismatch");

  std::vector<float> tensor;
  auto result = inference::copyAudioChunkToTensor(input, shape, tensor);
  require(result.succeeded(), "chunk to tensor copy should succeed");
  require(tensor.size() == input.samples.size(), "tensor sample count mismatch");
  for (std::size_t index = 0; index < tensor.size(); ++index) {
    require(nearlyEqual(tensor[index], input.samples[index]), "tensor sample mismatch");
  }

  common::AudioChunk output;
  result =
      inference::copyTensorToAudioChunk(tensor.data(), tensor.size(), shape, 44100.0, output);
  require(result.succeeded(), "tensor to chunk copy should succeed");
  require(output.sampleRate == 44100.0, "output sample rate mismatch");
  require(output.channels == input.channels, "output channel mismatch");
  require(output.frames == input.frames, "output frame mismatch");
  for (std::size_t index = 0; index < output.samples.size(); ++index) {
    require(nearlyEqual(output.samples[index], input.samples[index]),
            "output sample mismatch");
  }

  const inference::AudioTensorShape badShape{2, 2, 3};
  result = inference::copyAudioChunkToTensor(input, badShape, tensor);
  require(!result, "non-batch-1 shape should fail");

  const inference::AudioTensorShape wrongFrames{1, 2, 4};
  result = inference::copyAudioChunkToTensor(input, wrongFrames, tensor);
  require(!result, "mismatched frame count should fail");
}

} // namespace llvc::tests
