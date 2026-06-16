# Low Latency Voice Changer

Ultra-low-latency real-time character voice changer engine.

This project starts from a C++ real-time audio foundation instead of adapting a
Python-first voice changer. Existing projects such as w-okada/voice-changer are
useful baseline references, but this repository focuses on measurable latency,
real-time callback safety, and swappable inference backends.

## Current Milestone

Sprint 5: Dummy inference backend pipeline.

- C++17 CMake project.
- Core modules for audio, DSP, inference, profiling, and common types.
- JUCE standalone app target with device selector, bypass toggle, and callback counters.
- Worker-thread shadow pipeline with fixed audio blocks and SPSC queues.
- C++ YIN F0 estimator baseline with benchmark coverage.
- Dummy voice conversion backend with worker-thread integration and benchmark stats.
- Unit-test and benchmark CLI targets.
- Real-time audio coding rules in `AGENTS.md`.

## Build

The default build does not require JUCE and is intended for core library tests.

```powershell
cmake --preset default
cmake --build --preset default
ctest --preset default
```

The benchmark CLI supports terminal and CSV reports:

```powershell
build\manual\llvc_benchmark_cli.exe --iterations 128 --dummy-delay-us 1000 --csv build\manual\latency_report.csv
```

To build the JUCE app, install CMake and Ninja, clone JUCE locally, then configure
with `LLVC_BUILD_JUCE_APP=ON` and `LLVC_JUCE_DIR` pointing at the JUCE checkout.

```powershell
cmake -S . -B build/juce-local -G Ninja `
  -DLLVC_BUILD_JUCE_APP=ON `
  -DLLVC_JUCE_DIR=C:\path\to\JUCE
cmake --build build/juce-local
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
