#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>

namespace llvc::audio {

template <typename T, std::size_t Capacity> class SpscRingBuffer {
  static_assert(Capacity > 0, "SpscRingBuffer capacity must be greater than zero.");
  static_assert(std::is_copy_assignable<T>::value,
                "SpscRingBuffer requires copy-assignable items.");

public:
  SpscRingBuffer() = default;

  SpscRingBuffer(const SpscRingBuffer&) = delete;
  SpscRingBuffer& operator=(const SpscRingBuffer&) = delete;

  constexpr std::size_t capacity() const noexcept { return Capacity; }

  bool empty() const noexcept {
    return readIndex_.load(std::memory_order_acquire) ==
           writeIndex_.load(std::memory_order_acquire);
  }

  bool full() const noexcept {
    const auto next = increment(writeIndex_.load(std::memory_order_relaxed));
    return next == readIndex_.load(std::memory_order_acquire);
  }

  std::size_t size() const noexcept {
    const auto read = readIndex_.load(std::memory_order_acquire);
    const auto write = writeIndex_.load(std::memory_order_acquire);
    if (write >= read) {
      return write - read;
    }
    return storage_.size() - read + write;
  }

  bool tryPush(const T& item) noexcept(std::is_nothrow_copy_assignable<T>::value) {
    const auto write = writeIndex_.load(std::memory_order_relaxed);
    const auto next = increment(write);
    if (next == readIndex_.load(std::memory_order_acquire)) {
      overflowCount_.fetch_add(1, std::memory_order_relaxed);
      return false;
    }

    storage_[write] = item;
    writeIndex_.store(next, std::memory_order_release);
    return true;
  }

  bool tryPop(T& item) noexcept(std::is_nothrow_copy_assignable<T>::value) {
    const auto read = readIndex_.load(std::memory_order_relaxed);
    if (read == writeIndex_.load(std::memory_order_acquire)) {
      underflowCount_.fetch_add(1, std::memory_order_relaxed);
      return false;
    }

    item = storage_[read];
    readIndex_.store(increment(read), std::memory_order_release);
    return true;
  }

  void reset() noexcept {
    readIndex_.store(0, std::memory_order_release);
    writeIndex_.store(0, std::memory_order_release);
    overflowCount_.store(0, std::memory_order_relaxed);
    underflowCount_.store(0, std::memory_order_relaxed);
  }

  std::size_t overflowCount() const noexcept {
    return overflowCount_.load(std::memory_order_relaxed);
  }

  std::size_t underflowCount() const noexcept {
    return underflowCount_.load(std::memory_order_relaxed);
  }

private:
  static constexpr std::size_t increment(std::size_t index) noexcept {
    return (index + 1) % (Capacity + 1);
  }

  std::array<T, Capacity + 1> storage_{};
  std::atomic<std::size_t> readIndex_{0};
  std::atomic<std::size_t> writeIndex_{0};
  std::atomic<std::size_t> overflowCount_{0};
  std::atomic<std::size_t> underflowCount_{0};
};

} // namespace llvc::audio
