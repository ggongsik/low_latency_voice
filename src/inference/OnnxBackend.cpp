#include "inference/OnnxBackend.h"

namespace llvc::inference {

common::Result OnnxBackend::loadModel(const std::string&) {
  return common::Result::error(
      "ONNX Runtime backend is not enabled in this build; use DummyVoiceConversionBackend "
      "for pipeline tests");
}

common::Result OnnxBackend::process(const common::AudioChunk&, common::AudioChunk&) {
  return common::Result::error("ONNX Runtime backend is not enabled in this build");
}

BackendStats OnnxBackend::stats() const { return {}; }

} // namespace llvc::inference
