#include "profiler/LatencyProfiler.h"

#include <iostream>

namespace llvc::benchmarks {
void runAudioPipelineBenchmark();
}

int main() {
  std::cout << "Low Latency Voice Changer benchmark CLI\n";
  llvc::benchmarks::runAudioPipelineBenchmark();
  return 0;
}
