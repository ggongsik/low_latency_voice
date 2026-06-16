#pragma once

#include "inference/IVoiceConversionBackend.h"

namespace llvc::inference {

class OnnxBackend final : public IVoiceConversionBackend {
public:
  common::Result loadModel(const std::string& path) override;
  common::Result process(const common::AudioChunk& input,
                         common::AudioChunk& output) override;
  BackendStats stats() const override;
};

} // namespace llvc::inference
