#include "profiler/LatencyProfiler.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace llvc::benchmarks {
void runAudioPipelineBenchmark(std::size_t iterations, long dummyDelayUs,
                               const std::string& csvPath,
                               const std::string& onnxModelPath,
                               std::size_t onnxChannels,
                               std::size_t onnxFrames,
                               std::size_t onnxWarmupRuns);
}

namespace {

std::size_t parsePositiveSize(const char* value, std::size_t fallback) {
  const auto parsed = static_cast<std::size_t>(std::strtoul(value, nullptr, 10));
  return parsed == 0 ? fallback : parsed;
}

} // namespace

int main(int argc, char* argv[]) {
  std::size_t iterations = 128;
  long dummyDelayUs = 1000;
  std::string csvPath;
  std::string onnxModelPath;
  std::size_t onnxChannels = 1;
  std::size_t onnxFrames = 128;
  std::size_t onnxWarmupRuns = 2;

  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--help") {
      std::cout << "Usage: llvc_benchmark_cli [--iterations N] "
                   "[--dummy-delay-us N] [--csv path] "
                   "[--onnx-model path] [--onnx-channels N] "
                   "[--onnx-frames N] [--onnx-warmup N]\n";
      return 0;
    }

    if (argument == "--iterations" && index + 1 < argc) {
      iterations = parsePositiveSize(argv[++index], 1);
      continue;
    }

    if (argument == "--dummy-delay-us" && index + 1 < argc) {
      dummyDelayUs = std::strtol(argv[++index], nullptr, 10);
      if (dummyDelayUs < 0) {
        dummyDelayUs = 0;
      }
      continue;
    }

    if (argument == "--csv" && index + 1 < argc) {
      csvPath = argv[++index];
      continue;
    }

    if (argument == "--onnx-model" && index + 1 < argc) {
      onnxModelPath = argv[++index];
      continue;
    }

    if (argument == "--onnx-channels" && index + 1 < argc) {
      onnxChannels = parsePositiveSize(argv[++index], 1);
      continue;
    }

    if (argument == "--onnx-frames" && index + 1 < argc) {
      onnxFrames = parsePositiveSize(argv[++index], 128);
      continue;
    }

    if (argument == "--onnx-warmup" && index + 1 < argc) {
      onnxWarmupRuns = parsePositiveSize(argv[++index], 1);
      continue;
    }

    std::cerr << "Unknown argument: " << argument << '\n';
    return 1;
  }

  std::cout << "Low Latency Voice Changer benchmark CLI\n";
  llvc::benchmarks::runAudioPipelineBenchmark(iterations, dummyDelayUs, csvPath,
                                              onnxModelPath, onnxChannels,
                                              onnxFrames, onnxWarmupRuns);
  return 0;
}
