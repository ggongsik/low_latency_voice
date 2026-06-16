#pragma once

#include "inference/IVoiceConversionBackend.h"

#include <memory>

namespace llvc::inference {

class OnnxBackend final : public IVoiceConversionBackend {
public:
  OnnxBackend();
  ~OnnxBackend() override;

  OnnxBackend(const OnnxBackend&) = delete;
  OnnxBackend& operator=(const OnnxBackend&) = delete;

  common::Result loadModel(const std::string& path) override;
  common::Result process(const common::AudioChunk& input,
                         common::AudioChunk& output) override;
  BackendStats stats() const override;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace llvc::inference
