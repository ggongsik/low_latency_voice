#include <exception>
#include <iostream>
#include <string>

namespace llvc::tests {
void testAudioEngine();
void testAudioWorkerPipeline();
void testInferenceBackends();
void testLatencyProfiler();
void testPitchYIN();
void testSpscRingBuffer();
}

int main() {
  try {
    llvc::tests::testAudioEngine();
    llvc::tests::testAudioWorkerPipeline();
    llvc::tests::testInferenceBackends();
    llvc::tests::testSpscRingBuffer();
    llvc::tests::testLatencyProfiler();
    llvc::tests::testPitchYIN();
  } catch (const std::exception& error) {
    std::cerr << "Test failure: " << error.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "Unknown test failure\n";
    return 1;
  }

  std::cout << "All tests passed\n";
  return 0;
}
