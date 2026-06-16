#include "inference/DummyVoiceConversionBackend.h"

#include <algorithm>
#include <chrono>

namespace llvc::inference {

DummyVoiceConversionBackend::DummyVoiceConversionBackend(
    DummyBackendSettings settings) noexcept {
  setSettings(settings);
}

void DummyVoiceConversionBackend::setSettings(DummyBackendSettings settings) noexcept {
  gain_.store(settings.gain, std::memory_order_relaxed);
  invertPolarity_.store(settings.invertPolarity, std::memory_order_relaxed);
}

DummyBackendSettings DummyVoiceConversionBackend::settings() const noexcept {
  DummyBackendSettings current;
  current.gain = gain_.load(std::memory_order_relaxed);
  current.invertPolarity = invertPolarity_.load(std::memory_order_relaxed);
  return current;
}

common::Result DummyVoiceConversionBackend::loadModel(const std::string& path) {
  if (!path.empty() && path.find("dummy://") != 0) {
    return common::Result::error("dummy backend only accepts dummy:// model paths");
  }

  modelLoaded_.store(true, std::memory_order_release);
  return common::Result::ok();
}

common::Result DummyVoiceConversionBackend::warmUp(const common::AudioChunk& input,
                                                   std::size_t iterations) {
  const auto runCount = iterations == 0 ? std::size_t{1} : iterations;
  const auto result = IVoiceConversionBackend::warmUp(input, runCount);
  if (result) {
    warmupRuns_.fetch_add(runCount, std::memory_order_relaxed);
  }
  return result;
}

common::Result DummyVoiceConversionBackend::process(const common::AudioChunk& input,
                                                    common::AudioChunk& output) {
  if (!modelLoaded_.load(std::memory_order_acquire)) {
    return common::Result::error("dummy backend model is not loaded");
  }
  if (input.empty() || input.samples.size() < input.sampleCount()) {
    return common::Result::error("dummy backend received an invalid audio chunk");
  }

  const auto started = std::chrono::steady_clock::now();
  output.sampleRate = input.sampleRate;
  output.channels = input.channels;
  output.frames = input.frames;
  output.samples.resize(input.sampleCount());

  const auto settingsSnapshot = settings();
  const auto polarity = settingsSnapshot.invertPolarity ? -1.0F : 1.0F;
  const auto scale = settingsSnapshot.gain * polarity;

  std::transform(input.samples.begin(), input.samples.begin() + input.sampleCount(),
                 output.samples.begin(), [scale](float sample) { return sample * scale; });

  const auto finished = std::chrono::steady_clock::now();
  const auto elapsedMs =
      std::chrono::duration<double, std::milli>(finished - started).count();
  recordProcessTime(elapsedMs);
  return common::Result::ok();
}

BackendStats DummyVoiceConversionBackend::stats() const {
  BackendStats snapshot;
  snapshot.lastProcessMs = lastProcessMs_.load(std::memory_order_relaxed);
  const auto processedChunks = processedChunks_.load(std::memory_order_relaxed);
  snapshot.processedChunks = processedChunks;
  snapshot.averageProcessMs =
      processedChunks > 0
          ? totalProcessMs_.load(std::memory_order_relaxed) /
                static_cast<double>(processedChunks)
          : 0.0;
  snapshot.warmupRuns = warmupRuns_.load(std::memory_order_relaxed);
  snapshot.modelLoaded = modelLoaded_.load(std::memory_order_acquire);
  return snapshot;
}

void DummyVoiceConversionBackend::recordProcessTime(double elapsedMs) noexcept {
  lastProcessMs_.store(elapsedMs, std::memory_order_relaxed);
  totalProcessMs_.store(totalProcessMs_.load(std::memory_order_relaxed) + elapsedMs,
                        std::memory_order_relaxed);
  processedChunks_.fetch_add(1, std::memory_order_relaxed);
}

} // namespace llvc::inference
