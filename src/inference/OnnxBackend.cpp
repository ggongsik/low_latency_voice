#include "inference/OnnxBackend.h"

#include <exception>
#include <string>

#if LLVC_ENABLE_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

namespace llvc::inference {

#if LLVC_ENABLE_ONNXRUNTIME
struct OnnxBackend::Impl {
  Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "llvc"};
  Ort::SessionOptions sessionOptions;
  std::unique_ptr<Ort::Session> session;
  BackendStats stats;

  Impl() {
    sessionOptions.SetIntraOpNumThreads(1);
    sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);
  }
};
#else
struct OnnxBackend::Impl {
  BackendStats stats;
};
#endif

OnnxBackend::OnnxBackend() : impl_(new Impl()) {}

OnnxBackend::~OnnxBackend() = default;

common::Result OnnxBackend::loadModel(const std::string& path) {
#if LLVC_ENABLE_ONNXRUNTIME
  if (path.empty()) {
    return common::Result::error("ONNX model path is empty");
  }

  try {
#if defined(_WIN32)
    const std::wstring widePath(path.begin(), path.end());
    impl_->session.reset(
        new Ort::Session(impl_->env, widePath.c_str(), impl_->sessionOptions));
#else
    impl_->session.reset(new Ort::Session(impl_->env, path.c_str(), impl_->sessionOptions));
#endif
    impl_->stats.modelLoaded = true;
    return common::Result::ok();
  } catch (const Ort::Exception& error) {
    impl_->stats.modelLoaded = false;
    return common::Result::error(std::string("ONNX Runtime load failed: ") + error.what());
  } catch (const std::exception& error) {
    impl_->stats.modelLoaded = false;
    return common::Result::error(std::string("ONNX backend load failed: ") + error.what());
  }
#else
  (void)path;
  return common::Result::error(
      "ONNX Runtime backend is not enabled in this build; use DummyVoiceConversionBackend "
      "for pipeline tests");
#endif
}

common::Result OnnxBackend::process(const common::AudioChunk& input,
                                    common::AudioChunk& output) {
#if LLVC_ENABLE_ONNXRUNTIME
  (void)input;
  (void)output;
  if (!impl_->session) {
    return common::Result::error("ONNX Runtime session is not loaded");
  }

  return common::Result::error(
      "ONNX Runtime session loading is enabled, and AudioTensorAdapter is available, "
      "but OnnxBackend::process has not wired model input/output names yet");
#else
  (void)input;
  (void)output;
  return common::Result::error("ONNX Runtime backend is not enabled in this build");
#endif
}

BackendStats OnnxBackend::stats() const { return impl_->stats; }

} // namespace llvc::inference
