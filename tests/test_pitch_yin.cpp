#include "dsp/PitchYIN.h"

#include <array>
#include <cmath>
#include <stdexcept>

namespace {

constexpr double kPi = 3.14159265358979323846;

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool within(float value, float expected, float tolerance) {
  return std::fabs(value - expected) <= tolerance;
}

template <std::size_t Size>
void fillSine(std::array<float, Size>& samples, double sampleRate, double frequencyHz,
              float amplitude) {
  for (std::size_t index = 0; index < samples.size(); ++index) {
    const auto phase =
        2.0 * kPi * frequencyHz * static_cast<double>(index) / sampleRate;
    samples[index] = amplitude * static_cast<float>(std::sin(phase));
  }
}

} // namespace

namespace llvc::tests {

void testPitchYIN() {
  constexpr double sampleRate = 48000.0;
  std::array<float, 2048> frame{};
  dsp::PitchYIN estimator(sampleRate);

  fillSine(frame, sampleRate, 440.0, 0.8F);
  auto estimate = estimator.estimate(frame.data(), frame.size());
  require(estimate.voiced, "440 Hz sine should be voiced");
  require(within(estimate.frequencyHz, 440.0F, 2.0F), "440 Hz estimate out of range");
  require(estimate.confidence > 0.80F, "440 Hz confidence too low");
  require(estimate.rms > 0.1F, "440 Hz RMS should be nonzero");

  fillSine(frame, sampleRate, 220.0, 0.8F);
  estimate = estimator.estimate(frame.data(), frame.size());
  require(estimate.voiced, "220 Hz sine should be voiced");
  require(within(estimate.frequencyHz, 220.0F, 2.0F), "220 Hz estimate out of range");

  frame.fill(0.0F);
  estimate = estimator.estimate(frame.data(), frame.size());
  require(!estimate.voiced, "silence should be unvoiced");
  require(estimate.frequencyHz == 0.0F, "silence should not report frequency");

  dsp::PitchYINSettings settings;
  settings.minFrequencyHz = 300.0F;
  settings.maxFrequencyHz = 600.0F;
  estimator.setSettings(settings);
  fillSine(frame, sampleRate, 440.0, 0.8F);
  estimate = estimator.estimate(frame.data(), frame.size());
  require(estimate.voiced, "440 Hz sine should be voiced with narrowed settings");
  require(within(estimate.frequencyHz, 440.0F, 2.0F),
          "narrowed settings estimate out of range");

  settings.rmsThreshold = 0.9F;
  estimator.setSettings(settings);
  estimate = estimator.estimate(frame.data(), frame.size());
  require(!estimate.voiced, "high RMS threshold should gate the estimate");
}

} // namespace llvc::tests
