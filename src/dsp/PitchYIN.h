#pragma once

#include <cstddef>

namespace llvc::dsp {

struct F0Estimate {
  float frequencyHz = 0.0F;
  float confidence = 0.0F;
  bool voiced = false;
};

class PitchYIN {
public:
  explicit PitchYIN(double sampleRate = 48000.0) noexcept;

  void setSampleRate(double sampleRate) noexcept;
  double sampleRate() const noexcept { return sampleRate_; }

  F0Estimate estimate(const float* samples, std::size_t frameCount) const noexcept;

private:
  double sampleRate_ = 48000.0;
};

} // namespace llvc::dsp
