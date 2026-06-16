#include <exception>
#include <iostream>
#include <string>

namespace llvc::tests {
void testLatencyProfiler();
void testSpscRingBuffer();
}

int main() {
  try {
    llvc::tests::testSpscRingBuffer();
    llvc::tests::testLatencyProfiler();
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
