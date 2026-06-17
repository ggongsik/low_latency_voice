#include "common/AudioChunk.h"
#include "inference/DummyVoiceConversionBackend.h"
#include "inference/OnnxBackend.h"

#include <cmath>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool nearlyEqual(float lhs, float rhs) { return std::fabs(lhs - rhs) < 0.00001F; }

llvc::common::AudioChunk makeChunk() {
  llvc::common::AudioChunk chunk;
  chunk.sampleRate = 48000.0;
  chunk.channels = 1;
  chunk.frames = 4;
  chunk.samples = {0.25F, -0.5F, 0.75F, -1.0F};
  return chunk;
}

} // namespace

namespace llvc::tests {

void testInferenceBackends() {
  inference::DummyBackendSettings settings;
  settings.gain = 0.5F;
  inference::DummyVoiceConversionBackend dummy(settings);

  auto input = makeChunk();
  common::AudioChunk output;

  auto result = dummy.process(input, output);
  require(!result, "dummy backend should require loadModel before process");

  result = dummy.loadModel("dummy://identity");
  require(result.succeeded(), "dummy backend should load dummy model path");
  result = dummy.warmUp(input, 2);
  require(result.succeeded(), "dummy backend warm-up should succeed");

  result = dummy.process(input, output);
  require(result.succeeded(), "dummy backend process should succeed");
  require(output.channels == input.channels, "dummy output channel mismatch");
  require(output.frames == input.frames, "dummy output frame mismatch");
  require(output.samples.size() == input.samples.size(), "dummy output sample count mismatch");
  for (std::size_t index = 0; index < input.samples.size(); ++index) {
    require(nearlyEqual(output.samples[index], input.samples[index] * 0.5F),
            "dummy output sample mismatch");
  }

  const auto stats = dummy.stats();
  require(stats.modelLoaded, "dummy backend should report loaded model");
  require(stats.warmupRuns == 2, "dummy warm-up count mismatch");
  require(stats.processedChunks >= 3, "dummy process count should include warm-up");

  inference::OnnxBackend onnx;
  result = onnx.loadModel("dummy.onnx");
  require(!result, "ONNX placeholder should fail while runtime is disabled");
  result = onnx.warmUp(input, 1);
  require(!result, "disabled ONNX backend warm-up should fail");
  const auto onnxStats = onnx.stats();
  require(!onnxStats.modelLoaded, "disabled ONNX backend should not report a loaded model");
  require(onnxStats.warmupRuns == 0,
          "disabled ONNX backend should not report warm-up runs");
  require(onnxStats.processedChunks == 0,
          "disabled ONNX backend should not report processed chunks");
}

} // namespace llvc::tests
