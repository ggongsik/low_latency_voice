#include "audio/AudioEngine.h"

#include <cmath>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool nearlyEqual(float lhs, float rhs) { return std::fabs(lhs - rhs) < 0.00001F; }
bool nearlyEqual(double lhs, double rhs) { return std::fabs(lhs - rhs) < 0.00001; }

} // namespace

namespace llvc::tests {

void testAudioEngine() {
  audio::AudioEngine engine;
  audio::AudioSettings settings;
  settings.sampleRate = 48000.0;
  settings.blockSize = 128;
  settings.inputChannels = 1;
  settings.outputChannels = 2;
  settings.bypass = true;
  engine.configure(settings);

  require(nearlyEqual(engine.estimatedBlockLatencyMs(), 2.6666666667),
          "estimated block latency mismatch");

  const float inputMono[] = {0.1F, -0.2F, 0.3F, -0.4F};
  const float* inputs[] = {inputMono};
  float outputLeft[] = {0.0F, 0.0F, 0.0F, 0.0F};
  float outputRight[] = {0.0F, 0.0F, 0.0F, 0.0F};
  float* outputs[] = {outputLeft, outputRight};

  engine.processPassThrough(inputs, outputs, 1, 2, 4);

  for (int i = 0; i < 4; ++i) {
    require(nearlyEqual(outputLeft[i], inputMono[i]), "left output should copy mono input");
    require(nearlyEqual(outputRight[i], inputMono[i]), "right output should duplicate mono input");
  }
  require(engine.processedBlocks() == 1, "processed block count mismatch");

  engine.setBypass(false);
  engine.processPassThrough(inputs, outputs, 1, 2, 4);
  for (int i = 0; i < 4; ++i) {
    require(nearlyEqual(outputLeft[i], 0.0F), "left output should be muted when bypass is off");
    require(nearlyEqual(outputRight[i], 0.0F), "right output should be muted when bypass is off");
  }

  engine.noteXrun();
  require(engine.xrunCount() == 1, "xrun count mismatch");
  engine.resetCounters();
  require(engine.processedBlocks() == 0, "reset should clear processed block count");
  require(engine.xrunCount() == 0, "reset should clear xrun count");
}

} // namespace llvc::tests
