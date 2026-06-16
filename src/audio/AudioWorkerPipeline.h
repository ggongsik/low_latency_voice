#pragma once

#include "audio/AudioBlock.h"
#include "audio/SpscRingBuffer.h"
#include "common/Threading.h"
#include "inference/IVoiceConversionBackend.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace llvc::audio {

struct AudioWorkerPipelineStats {
  bool running = false;
  std::uint64_t submittedBlocks = 0;
  std::uint64_t workerProcessedBlocks = 0;
  std::uint64_t consumedOutputBlocks = 0;
  std::uint64_t lateOutputBlocks = 0;
  std::uint64_t droppedInputBlocks = 0;
  std::uint64_t droppedOutputBlocks = 0;
  std::uint64_t unsupportedInputBlocks = 0;
  std::size_t inputQueueSize = 0;
  std::size_t outputQueueSize = 0;
  std::size_t inputQueueOverflows = 0;
  std::size_t outputQueueOverflows = 0;
  std::int64_t dummyProcessingDelayUs = 0;
  std::int64_t lastWorkerProcessUs = 0;
  std::int64_t averageWorkerProcessUs = 0;
  std::int64_t maxWorkerProcessUs = 0;
  std::uint64_t backendErrorBlocks = 0;
  bool backendAttached = false;
};

class AudioWorkerPipeline {
public:
  static constexpr std::size_t queueCapacity = 16;

  AudioWorkerPipeline() = default;
  ~AudioWorkerPipeline();

  AudioWorkerPipeline(const AudioWorkerPipeline&) = delete;
  AudioWorkerPipeline& operator=(const AudioWorkerPipeline&) = delete;

  void start();
  void stop();
  void resetCounters() noexcept;
  void clearQueuesWhenStopped() noexcept;
  void setSampleRate(double sampleRate) noexcept;
  void setDummyProcessingDelay(std::chrono::microseconds delay) noexcept;
  bool setInferenceBackend(inference::IVoiceConversionBackend* backend) noexcept;

  bool running() const noexcept;
  AudioWorkerPipelineStats stats() const noexcept;

  bool submitInputFromAudioThread(const float* const* inputs, std::size_t inputChannels,
                                  std::size_t frames) noexcept;

  bool processShadowFromAudioThread(const float* const* inputs, std::size_t inputChannels,
                                    std::size_t frames) noexcept;

  bool processReplacingFromAudioThread(const float* const* inputs, float* const* outputs,
                                       std::size_t inputChannels, std::size_t outputChannels,
                                       std::size_t frames) noexcept;

private:
  using BlockQueue = SpscRingBuffer<AudioBlock, queueCapacity>;

  void workerLoop();
  bool processWithBackend(const AudioBlock& input, AudioBlock& output);
  bool tryConsumeOutputFromAudioThread(AudioBlock& block) noexcept;
  void copyBlockToChunk(const AudioBlock& block, common::AudioChunk& chunk) const;
  static bool copyChunkToBlock(const common::AudioChunk& chunk, std::uint64_t sequence,
                               AudioBlock& block) noexcept;
  static void copyDryToOutput(const float* const* inputs, float* const* outputs,
                              std::size_t inputChannels, std::size_t outputChannels,
                              std::size_t frames) noexcept;

  BlockQueue inputQueue_;
  BlockQueue outputQueue_;
  AudioBlock audioThreadInputScratch_{};
  AudioBlock audioThreadOutputScratch_{};

  common::JoinableThread workerThread_;
  std::atomic<bool> running_{false};
  std::atomic<double> sampleRate_{48000.0};
  std::atomic<std::int64_t> dummyProcessingDelayUs_{0};
  std::atomic<std::uint64_t> nextSequence_{1};
  std::atomic<std::uint64_t> submittedBlocks_{0};
  std::atomic<std::uint64_t> workerProcessedBlocks_{0};
  std::atomic<std::uint64_t> consumedOutputBlocks_{0};
  std::atomic<std::uint64_t> lateOutputBlocks_{0};
  std::atomic<std::uint64_t> droppedInputBlocks_{0};
  std::atomic<std::uint64_t> droppedOutputBlocks_{0};
  std::atomic<std::uint64_t> unsupportedInputBlocks_{0};
  std::atomic<std::uint64_t> backendErrorBlocks_{0};
  std::atomic<std::int64_t> lastWorkerProcessUs_{0};
  std::atomic<std::int64_t> totalWorkerProcessUs_{0};
  std::atomic<std::int64_t> averageWorkerProcessUs_{0};
  std::atomic<std::int64_t> maxWorkerProcessUs_{0};
  inference::IVoiceConversionBackend* backend_ = nullptr;
  common::AudioChunk backendInputScratch_{};
  common::AudioChunk backendOutputScratch_{};
};

} // namespace llvc::audio
