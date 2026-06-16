#include "profiler/LatencyProfiler.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace llvc::benchmarks {
void runAudioPipelineBenchmark(std::size_t iterations, long dummyDelayUs,
                               const std::string& csvPath);
}

int main(int argc, char* argv[]) {
  std::size_t iterations = 128;
  long dummyDelayUs = 1000;
  std::string csvPath;

  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--help") {
      std::cout << "Usage: llvc_benchmark_cli [--iterations N] "
                   "[--dummy-delay-us N] [--csv path]\n";
      return 0;
    }

    if (argument == "--iterations" && index + 1 < argc) {
      iterations = static_cast<std::size_t>(std::strtoul(argv[++index], nullptr, 10));
      if (iterations == 0) {
        iterations = 1;
      }
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

    std::cerr << "Unknown argument: " << argument << '\n';
    return 1;
  }

  std::cout << "Low Latency Voice Changer benchmark CLI\n";
  llvc::benchmarks::runAudioPipelineBenchmark(iterations, dummyDelayUs, csvPath);
  return 0;
}
