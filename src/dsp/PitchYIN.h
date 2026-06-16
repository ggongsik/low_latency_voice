#pragma once

#include <cstddef>

namespace llvc::dsp {

struct PitchYINSettings {
  float minFrequencyHz = 50.0F;
  float maxFrequencyHz = 1000.0F;
  float threshold = 0.15F;
  float rmsThreshold = 0.005F;
};

struct F0Estimate {
  float frequencyHz = 0.0F;
  float confidence = 0.0F;
  float rms = 0.0F;
  bool voiced = false;
};

class PitchYIN {
public:
  explicit PitchYIN(double sampleRate = 48000.0) noexcept;
  PitchYIN(double sampleRate, PitchYINSettings settings) noexcept;

  void setSampleRate(double sampleRate) noexcept;
  double sampleRate() const noexcept { return sampleRate_; }

  void setSettings(PitchYINSettings settings) noexcept;
  const PitchYINSettings& settings() const noexcept { return settings_; }

  F0Estimate estimate(const float* samples, std::size_t frameCount) const noexcept;

private:
  static double difference(const float* samples, std::size_t frameCount,
                           std::size_t lag) noexcept;
  static float clampUnit(double value) noexcept;
  float cumulativeMeanNormalizedDifference(const float* samples, std::size_t frameCount,
                                           std::size_t lag) const noexcept;

  double sampleRate_ = 48000.0;
  PitchYINSettings settings_{};
};

} // namespace llvc::dsp
