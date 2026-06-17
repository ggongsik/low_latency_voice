#include "inference/OnnxBackend.h"

#include "inference/AudioTensorAdapter.h"

#include <chrono>
#include <exception>
#include <string>
#include <vector>

#if LLVC_ENABLE_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

namespace llvc::inference {

#if LLVC_ENABLE_ONNXRUNTIME
namespace {

common::Result readStaticAudioTensorShape(const Ort::TensorTypeAndShapeInfo& tensorInfo,
                                          const char* role,
                                          AudioTensorShape& shape) {
  if (tensorInfo.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
    return common::Result::error(std::string("ONNX ") + role +
                                 " tensor must use float32 samples");
  }

  const auto dims = tensorInfo.GetShape();
  if (dims.size() != 3) {
    return common::Result::error(std::string("ONNX ") + role +
                                 " tensor must have shape [1, channels, frames]");
  }
  if (dims[0] != 1) {
    return common::Result::error(std::string("ONNX ") + role +
                                 " tensor batch dimension must be 1");
  }
  if (dims[1] <= 0 || dims[2] <= 0) {
    return common::Result::error(std::string("ONNX ") + role +
                                 " tensor channel/frame dimensions must be static");
  }

  shape.batch = 1;
  shape.channels = static_cast<std::size_t>(dims[1]);
  shape.frames = static_cast<std::size_t>(dims[2]);
  return common::Result::ok();
}

} // namespace

struct OnnxBackend::Impl {
  Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "llvc"};
  Ort::SessionOptions sessionOptions;
  std::unique_ptr<Ort::Session> session;
  Ort::AllocatorWithDefaultOptions allocator;
  std::vector<Ort::AllocatedStringPtr> inputNameStorage;
  std::vector<Ort::AllocatedStringPtr> outputNameStorage;
  std::vector<const char*> inputNames;
  std::vector<const char*> outputNames;
  AudioTensorShape inputShape;
  AudioTensorShape outputShape;
  std::vector<float> inputTensor;
  BackendStats stats;

  Impl() {
    sessionOptions.SetIntraOpNumThreads(1);
    sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);
  }

  void resetMetadata() {
    inputNameStorage.clear();
    outputNameStorage.clear();
    inputNames.clear();
    outputNames.clear();
    inputShape = {};
    outputShape = {};
    inputTensor.clear();
  }

  common::Result inspectModel() {
    if (!session) {
      return common::Result::error("ONNX Runtime session is not loaded");
    }

    const auto inputCount = session->GetInputCount();
    const auto outputCount = session->GetOutputCount();
    if (inputCount != 1 || outputCount != 1) {
      return common::Result::error(
          "ONNX backend currently supports exactly one audio input and one audio output");
    }

    inputNameStorage.reserve(inputCount);
    outputNameStorage.reserve(outputCount);
    inputNames.reserve(inputCount);
    outputNames.reserve(outputCount);

    inputNameStorage.push_back(session->GetInputNameAllocated(0, allocator));
    outputNameStorage.push_back(session->GetOutputNameAllocated(0, allocator));
    inputNames.push_back(inputNameStorage.back().get());
    outputNames.push_back(outputNameStorage.back().get());

    const auto inputTypeInfo = session->GetInputTypeInfo(0);
    const auto outputTypeInfo = session->GetOutputTypeInfo(0);
    const auto inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
    const auto outputTensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();

    auto result = readStaticAudioTensorShape(inputTensorInfo, "input", inputShape);
    if (!result) {
      return result;
    }
    result = readStaticAudioTensorShape(outputTensorInfo, "output", outputShape);
    if (!result) {
      return result;
    }

    inputTensor.resize(inputShape.sampleCount());
    return common::Result::ok();
  }

  void recordProcessTime(double elapsedMs) noexcept {
    stats.lastProcessMs = elapsedMs;
    const auto previousChunks = stats.processedChunks;
    ++stats.processedChunks;
    const auto previousTotal = stats.averageProcessMs * static_cast<double>(previousChunks);
    stats.averageProcessMs =
        (previousTotal + elapsedMs) / static_cast<double>(stats.processedChunks);
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
    impl_->stats = {};
    impl_->resetMetadata();
#if defined(_WIN32)
    const std::wstring widePath(path.begin(), path.end());
    impl_->session.reset(
        new Ort::Session(impl_->env, widePath.c_str(), impl_->sessionOptions));
#else
    impl_->session.reset(new Ort::Session(impl_->env, path.c_str(), impl_->sessionOptions));
#endif
    const auto inspection = impl_->inspectModel();
    if (!inspection) {
      impl_->session.reset();
      impl_->resetMetadata();
      impl_->stats.modelLoaded = false;
      return inspection;
    }

    impl_->stats.modelLoaded = true;
    return common::Result::ok();
  } catch (const Ort::Exception& error) {
    impl_->session.reset();
    impl_->resetMetadata();
    impl_->stats.modelLoaded = false;
    return common::Result::error(std::string("ONNX Runtime load failed: ") + error.what());
  } catch (const std::exception& error) {
    impl_->session.reset();
    impl_->resetMetadata();
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

common::Result OnnxBackend::warmUp(const common::AudioChunk& input,
                                   std::size_t iterations) {
  const auto runCount = iterations == 0 ? std::size_t{1} : iterations;
  const auto result = IVoiceConversionBackend::warmUp(input, runCount);
  if (result) {
    impl_->stats.warmupRuns += runCount;
  }
  return result;
}

common::Result OnnxBackend::process(const common::AudioChunk& input,
                                    common::AudioChunk& output) {
#if LLVC_ENABLE_ONNXRUNTIME
  if (!impl_->session) {
    return common::Result::error("ONNX Runtime session is not loaded");
  }

  auto result = copyAudioChunkToTensor(input, impl_->inputShape, impl_->inputTensor);
  if (!result) {
    return result;
  }

  try {
    const auto started = std::chrono::steady_clock::now();
    const auto inputDims = impl_->inputShape.toInt64Vector();
    const auto memoryInfo =
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    auto inputTensor = Ort::Value::CreateTensor<float>(
        memoryInfo, impl_->inputTensor.data(), impl_->inputTensor.size(), inputDims.data(),
        inputDims.size());

    auto outputs = impl_->session->Run(Ort::RunOptions{nullptr}, impl_->inputNames.data(),
                                       &inputTensor, impl_->inputNames.size(),
                                       impl_->outputNames.data(), impl_->outputNames.size());
    if (outputs.size() != 1 || !outputs.front().IsTensor()) {
      return common::Result::error("ONNX Runtime did not return one output tensor");
    }

    const auto outputInfo = outputs.front().GetTensorTypeAndShapeInfo();
    const auto outputShape = outputInfo.GetShape();
    const auto expectedOutputDims = impl_->outputShape.toInt64Vector();
    if (outputShape != expectedOutputDims) {
      return common::Result::error("ONNX output tensor shape does not match model metadata");
    }

    const auto* outputData = outputs.front().GetTensorData<float>();
    result = copyTensorToAudioChunk(outputData, outputInfo.GetElementCount(),
                                    impl_->outputShape, input.sampleRate, output);
    if (!result) {
      return result;
    }

    const auto finished = std::chrono::steady_clock::now();
    const auto elapsedMs =
        std::chrono::duration<double, std::milli>(finished - started).count();
    impl_->recordProcessTime(elapsedMs);
    return common::Result::ok();
  } catch (const Ort::Exception& error) {
    return common::Result::error(std::string("ONNX Runtime process failed: ") + error.what());
  } catch (const std::exception& error) {
    return common::Result::error(std::string("ONNX backend process failed: ") + error.what());
  }
#else
  (void)input;
  (void)output;
  return common::Result::error("ONNX Runtime backend is not enabled in this build");
#endif
}

BackendStats OnnxBackend::stats() const { return impl_->stats; }

} // namespace llvc::inference
