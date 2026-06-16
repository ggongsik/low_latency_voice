# Low Latency Voice Changer

Ultra-low-latency real-time character voice changer engine.

This project starts from a C++ real-time audio foundation instead of adapting a
Python-first voice changer. Existing projects such as w-okada/voice-changer are
useful baseline references, but this repository focuses on measurable latency,
real-time callback safety, and swappable inference backends.

## Current Milestone

Sprint 7: Requirements and fixed tensor conversion.

- C++17 CMake project.
- Core modules for audio, DSP, inference, profiling, and common types.
- JUCE standalone app target with device selector, bypass toggle, and callback counters.
- Worker-thread shadow pipeline with fixed audio blocks and SPSC queues.
- C++ YIN F0 estimator baseline with benchmark coverage.
- Dummy voice conversion backend with worker-thread integration and benchmark stats.
- Optional ONNX Runtime build wiring for future real model loading.
- Fixed `[1, channels, frames]` audio tensor adapter for ONNX-style backends.
- Unit-test and benchmark CLI targets.
- Real-time audio coding rules in `AGENTS.md`.

## Build

The default build does not require JUCE and is intended for core library tests.

See [REQUIREMENTS.md](REQUIREMENTS.md) for the full dependency list, official
download locations, and CMake variables.

Dependency summary:

| Requirement | Needed for | Where to get it | Configure |
| --- | --- | --- | --- |
| MSVC 2019+ / C++17 compiler | Core build, JUCE, ONNX Runtime | https://visualstudio.microsoft.com/visual-cpp-build-tools/ | Install "Desktop development with C++". |
| CMake 3.22+ | All CMake builds | https://cmake.org/download/ | Make sure `cmake` is on `PATH`. |
| Ninja | Provided presets | https://ninja-build.org/ | Make sure `ninja` is on `PATH`, or use another CMake generator. |
| Git | Source checkout | https://git-scm.com/downloads | Needed to clone JUCE and manage this repo. |
| JUCE | Realtime standalone app | https://github.com/juce-framework/JUCE or https://juce.com/get-juce/download/ | `-DLLVC_BUILD_JUCE_APP=ON -DLLVC_JUCE_DIR=C:\path\to\JUCE` |
| ONNX Runtime | Optional ONNX backend | https://onnxruntime.ai/docs/get-started/with-cpp.html or https://github.com/microsoft/onnxruntime/releases | `-DLLVC_ENABLE_ONNXRUNTIME=ON -DLLVC_ONNXRUNTIME_ROOT=C:\path\to\onnxruntime` |
| ASIO SDK | Optional low-latency Windows audio experiments | https://www.steinberg.net/developers/ | Later sprint; licensing/setup is separate. |

```powershell
cmake --preset default
cmake --build --preset default
ctest --preset default
```

The benchmark CLI supports terminal and CSV reports:

```powershell
build\manual\llvc_benchmark_cli.exe --iterations 128 --dummy-delay-us 1000 --csv build\manual\latency_report.csv
```

To build the JUCE app, install CMake and Ninja, clone JUCE locally from
https://github.com/juce-framework/JUCE or download it from
https://juce.com/get-juce/download/, then configure with `LLVC_BUILD_JUCE_APP=ON`
and `LLVC_JUCE_DIR` pointing at the JUCE checkout.

```powershell
cmake -S . -B build/juce-local -G Ninja `
  -DLLVC_BUILD_JUCE_APP=ON `
  -DLLVC_JUCE_DIR=C:\path\to\JUCE
cmake --build build/juce-local
```

To enable ONNX Runtime session loading, install ONNX Runtime from
https://onnxruntime.ai/docs/get-started/with-cpp.html or
https://github.com/microsoft/onnxruntime/releases, then point
`LLVC_ONNXRUNTIME_ROOT` at that installation:

```powershell
cmake -S . -B build/onnx-local -G Ninja `
  -DLLVC_ENABLE_ONNXRUNTIME=ON `
  -DLLVC_ONNXRUNTIME_ROOT=C:\path\to\onnxruntime
cmake --build build/onnx-local
```

## Roadmap

1. Baseline latency measurement.
2. C++ audio pass-through app.
3. SPSC ring buffer and worker thread split.
4. Latency profiler and CSV export.
5. Dummy ONNX inference backend.
6. F0 estimation baseline.
7. Real voice conversion model experiment.
8. TensorRT/FP16 optimization.
