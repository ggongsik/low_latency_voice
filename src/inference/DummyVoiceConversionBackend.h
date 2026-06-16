#pragma once

#include "inference/IVoiceConversionBackend.h"

#include <atomic>
#include <cstdint>
#include <string>

namespace llvc::inference {

struct DummyBackendSettings {
  float gain = 1.0F;
  bool invertPolarity = false;
};

class DummyVoiceConversionBackend final : public IVoiceConversionBackend {
public:
  explicit DummyVoiceConversionBackend(DummyBackendSettings settings = {}) noexcept;

  void setSettings(DummyBackendSettings settings) noexcept;
  DummyBackendSettings settings() const noexcept;

  common::Result loadModel(const std::string& path) override;
  common::Result warmUp(const common::AudioChunk& input, std::size_t iterations) override;
  common::Result process(const common::AudioChunk& input, common::AudioChunk& output) override;
  BackendStats stats() const override;

private:
  void recordProcessTime(double elapsedMs) noexcept;

  std::atomic<float> gain_{1.0F};
  std::atomic<bool> invertPolarity_{false};
  std::atomic<bool> modelLoaded_{false};
  std::atomic<double> lastProcessMs_{0.0};
  std::atomic<double> totalProcessMs_{0.0};
  std::atomic<std::uint64_t> processedChunks_{0};
  std::atomic<std::uint64_t> warmupRuns_{0};
};

} // namespace llvc::inference
