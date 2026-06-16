#include "audio/SpscRingBuffer.h"

#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

} // namespace

namespace llvc::tests {

void testSpscRingBuffer() {
  audio::SpscRingBuffer<int, 3> buffer;

  require(buffer.empty(), "new buffer should be empty");
  require(buffer.capacity() == 3, "capacity mismatch");

  int value = 0;
  require(!buffer.tryPop(value), "empty pop should fail");
  require(buffer.underflowCount() == 1, "underflow should be counted");

  require(buffer.tryPush(1), "push 1 failed");
  require(buffer.tryPush(2), "push 2 failed");
  require(buffer.tryPush(3), "push 3 failed");
  require(buffer.full(), "buffer should be full");
  require(!buffer.tryPush(4), "overflow push should fail");
  require(buffer.overflowCount() == 1, "overflow should be counted");

  require(buffer.tryPop(value) && value == 1, "pop 1 failed");
  require(buffer.tryPop(value) && value == 2, "pop 2 failed");
  require(buffer.tryPush(4), "wraparound push failed");
  require(buffer.tryPop(value) && value == 3, "pop 3 failed");
  require(buffer.tryPop(value) && value == 4, "pop 4 failed");
  require(buffer.empty(), "buffer should be empty after pops");

  buffer.reset();
  require(buffer.empty(), "reset should empty buffer");
  require(buffer.overflowCount() == 0, "reset should clear overflow count");
  require(buffer.underflowCount() == 0, "reset should clear underflow count");
}

} // namespace llvc::tests
