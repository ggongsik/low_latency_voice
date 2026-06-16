#pragma once

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <utility>

#if defined(_WIN32) && defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
#define LLVC_USE_WIN32_THREAD_FALLBACK 1
#include <process.h>
#include <windows.h>
#else
#include <thread>
#endif

namespace llvc::common {

inline void sleepFor(std::chrono::microseconds duration) {
#if defined(LLVC_USE_WIN32_THREAD_FALLBACK)
  auto milliseconds = static_cast<std::int64_t>((duration.count() + 999) / 1000);
  if (milliseconds <= 0) {
    milliseconds = 1;
  }
  ::Sleep(static_cast<DWORD>(milliseconds));
#else
  std::this_thread::sleep_for(duration);
#endif
}

class JoinableThread {
public:
  JoinableThread() = default;
  ~JoinableThread() = default;

  JoinableThread(const JoinableThread&) = delete;
  JoinableThread& operator=(const JoinableThread&) = delete;

  template <typename Function> void start(Function&& function) {
#if defined(LLVC_USE_WIN32_THREAD_FALLBACK)
    using State = ThreadState<typename std::decay<Function>::type>;
    auto* state = new State(std::forward<Function>(function));
    const auto rawHandle = _beginthreadex(nullptr, 0, &State::run, state, 0, nullptr);
    if (rawHandle == 0) {
      delete state;
      throw std::runtime_error("failed to start worker thread");
    }
    handle_ = reinterpret_cast<HANDLE>(rawHandle);
#else
    thread_ = std::thread(std::forward<Function>(function));
#endif
  }

  bool joinable() const noexcept {
#if defined(LLVC_USE_WIN32_THREAD_FALLBACK)
    return handle_ != nullptr;
#else
    return thread_.joinable();
#endif
  }

  void join() {
#if defined(LLVC_USE_WIN32_THREAD_FALLBACK)
    if (handle_ == nullptr) {
      return;
    }
    ::WaitForSingleObject(handle_, INFINITE);
    ::CloseHandle(handle_);
    handle_ = nullptr;
#else
    if (thread_.joinable()) {
      thread_.join();
    }
#endif
  }

private:
#if defined(LLVC_USE_WIN32_THREAD_FALLBACK)
  template <typename Function> struct ThreadState {
    explicit ThreadState(Function&& threadFunction)
        : function(std::move(threadFunction)) {}

    static unsigned __stdcall run(void* context) {
      auto* state = static_cast<ThreadState*>(context);
      state->function();
      delete state;
      return 0;
    }

    Function function;
  };

  HANDLE handle_ = nullptr;
#else
  std::thread thread_;
#endif
};

} // namespace llvc::common
