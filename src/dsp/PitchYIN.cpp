#include "dsp/PitchYIN.h"

namespace llvc::dsp {

PitchYIN::PitchYIN(double sampleRate) noexcept : sampleRate_(sampleRate) {}

void PitchYIN::setSampleRate(double sampleRate) noexcept { sampleRate_ = sampleRate; }

F0Estimate PitchYIN::estimate(const float* samples, std::size_t frameCount) const noexcept {
  if (samples == nullptr || frameCount == 0) {
    return {};
  }

  return {};
}

} // namespace llvc::dsp
