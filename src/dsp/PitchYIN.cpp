#include "dsp/PitchYIN.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace llvc::dsp {

PitchYIN::PitchYIN(double sampleRate) noexcept { setSampleRate(sampleRate); }

PitchYIN::PitchYIN(double sampleRate, PitchYINSettings settings) noexcept
    : PitchYIN(sampleRate) {
  setSettings(settings);
}

void PitchYIN::setSampleRate(double sampleRate) noexcept {
  if (sampleRate > 0.0) {
    sampleRate_ = sampleRate;
  }
}

void PitchYIN::setSettings(PitchYINSettings settings) noexcept {
  if (settings.minFrequencyHz <= 0.0F) {
    settings.minFrequencyHz = 50.0F;
  }
  if (settings.maxFrequencyHz <= settings.minFrequencyHz) {
    settings.maxFrequencyHz = settings.minFrequencyHz * 2.0F;
  }
  if (settings.threshold <= 0.0F || settings.threshold >= 1.0F) {
    settings.threshold = 0.15F;
  }
  if (settings.rmsThreshold < 0.0F) {
    settings.rmsThreshold = 0.0F;
  }
  settings_ = settings;
}

F0Estimate PitchYIN::estimate(const float* samples, std::size_t frameCount) const noexcept {
  if (samples == nullptr || frameCount < 4 || sampleRate_ <= 0.0) {
    return {};
  }

  double energy = 0.0;
  for (std::size_t index = 0; index < frameCount; ++index) {
    const auto sample = static_cast<double>(samples[index]);
    energy += sample * sample;
  }

  F0Estimate estimate;
  estimate.rms = static_cast<float>(std::sqrt(energy / static_cast<double>(frameCount)));
  if (estimate.rms < settings_.rmsThreshold) {
    return estimate;
  }

  const auto minLag = std::max<std::size_t>(
      2, static_cast<std::size_t>(sampleRate_ / static_cast<double>(settings_.maxFrequencyHz)));
  const auto requestedMaxLag =
      static_cast<std::size_t>(sampleRate_ / static_cast<double>(settings_.minFrequencyHz));
  const auto maxLag = std::min<std::size_t>(requestedMaxLag, frameCount - 2);
  if (minLag >= maxLag) {
    return estimate;
  }

  double cumulative = 0.0;
  float bestCmnd = std::numeric_limits<float>::max();
  std::size_t bestLag = 0;
  float candidateCmnd = std::numeric_limits<float>::max();
  std::size_t candidateLag = 0;
  bool foundThreshold = false;

  for (std::size_t lag = 1; lag <= maxLag; ++lag) {
    const auto diff = difference(samples, frameCount, lag);
    cumulative += diff;
    const auto cmnd =
        cumulative > 0.0 ? static_cast<float>((diff * static_cast<double>(lag)) / cumulative)
                         : 1.0F;

    if (lag < minLag) {
      continue;
    }

    if (cmnd < bestCmnd) {
      bestCmnd = cmnd;
      bestLag = lag;
    }

    if (!foundThreshold && cmnd < settings_.threshold) {
      foundThreshold = true;
      candidateLag = lag;
      candidateCmnd = cmnd;
      continue;
    }

    if (foundThreshold) {
      if (cmnd < candidateCmnd) {
        candidateLag = lag;
        candidateCmnd = cmnd;
        continue;
      }
      break;
    }
  }

  const auto selectedLag = foundThreshold ? candidateLag : bestLag;
  const auto selectedCmnd = foundThreshold ? candidateCmnd : bestCmnd;
  if (selectedLag == 0 || selectedCmnd >= 1.0F) {
    return estimate;
  }

  double refinedLag = static_cast<double>(selectedLag);
  if (selectedLag > minLag && selectedLag + 1 <= maxLag) {
    const auto left = cumulativeMeanNormalizedDifference(samples, frameCount, selectedLag - 1);
    const auto center = cumulativeMeanNormalizedDifference(samples, frameCount, selectedLag);
    const auto right = cumulativeMeanNormalizedDifference(samples, frameCount, selectedLag + 1);
    const auto denominator = static_cast<double>(left - (2.0F * center) + right);
    if (std::fabs(denominator) > 0.0000001) {
      const auto offset = 0.5 * static_cast<double>(left - right) / denominator;
      if (offset > -1.0 && offset < 1.0) {
        refinedLag += offset;
      }
    }
  }

  estimate.frequencyHz = static_cast<float>(sampleRate_ / refinedLag);
  estimate.confidence = clampUnit(1.0 - static_cast<double>(selectedCmnd));
  estimate.voiced = foundThreshold && estimate.confidence > 0.0F;
  return estimate;
}

double PitchYIN::difference(const float* samples, std::size_t frameCount,
                            std::size_t lag) noexcept {
  double sum = 0.0;
  const auto limit = frameCount - lag;
  for (std::size_t index = 0; index < limit; ++index) {
    const auto delta = static_cast<double>(samples[index]) -
                       static_cast<double>(samples[index + lag]);
    sum += delta * delta;
  }
  return sum;
}

float PitchYIN::clampUnit(double value) noexcept {
  if (value <= 0.0) {
    return 0.0F;
  }
  if (value >= 1.0) {
    return 1.0F;
  }
  return static_cast<float>(value);
}

float PitchYIN::cumulativeMeanNormalizedDifference(const float* samples,
                                                   std::size_t frameCount,
                                                   std::size_t lag) const noexcept {
  double cumulative = 0.0;
  double current = 0.0;
  for (std::size_t currentLag = 1; currentLag <= lag; ++currentLag) {
    current = difference(samples, frameCount, currentLag);
    cumulative += current;
  }

  if (cumulative <= 0.0) {
    return 1.0F;
  }
  return static_cast<float>((current * static_cast<double>(lag)) / cumulative);
}

} // namespace llvc::dsp
