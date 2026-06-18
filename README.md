# Low Latency Voice Changer

Ultra-low-latency real-time character voice changer engine.

This project starts from a C++ real-time audio foundation instead of adapting a
Python-first voice changer. Existing projects such as w-okada/voice-changer are
useful baseline references, but this repository focuses on measurable latency,
real-time callback safety, and swappable inference backends.

## Current Milestone

Sprint 9: ONNX Runtime smoke benchmarking.

- C++17 CMake project.
- Core modules for audio, DSP, inference, profiling, and common types.
- JUCE standalone app target with device selector, bypass toggle, and callback counters.
- Worker-thread shadow pipeline with fixed audio blocks and SPSC queues.
- C++ YIN F0 estimator baseline with benchmark coverage.
- Dummy voice conversion backend with worker-thread integration and benchmark stats.
- Optional ONNX Runtime backend with session load, fixed IO inspection, and
  single-input/single-output inference.
- Benchmark CLI support for backend-only ONNX smoke latency runs.
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

After enabling ONNX Runtime, the same CLI can run a backend-only ONNX smoke
benchmark. The shape options must match the model input/output contract.

```powershell
build\onnx-local\llvc_benchmark_cli.exe --iterations 32 `
  --onnx-model models\identity_audio_1x1x128.onnx `
  --onnx-channels 1 `
  --onnx-frames 128 `
  --onnx-warmup 2
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

This is a C++/CMake project, not a .NET project. Do not use
`dotnet add package Microsoft.ML.OnnxRuntime`; that command requires a `.csproj`
file and installs the .NET binding, not the native C++ package used here.

Recommended Windows native install:

```powershell
powershell -ExecutionPolicy Bypass -File tools\install_onnxruntime_windows.ps1
```

The script installs ONNX Runtime under `third_party\onnxruntime`, which is the
default root used by the `onnx-local` CMake preset.

```powershell
cmake --preset onnx-local
cmake --build --preset onnx-local
```

The current ONNX backend expects one float32 input tensor and one float32 output
tensor, both with static shape `[1, channels, frames]`. It is intended as the
CPU baseline before execution-provider-specific tuning.

To generate a tiny identity model for smoke testing:

```powershell
py -m pip install -r tools\requirements-model.txt
py tools\create_identity_onnx.py --output models\identity_audio_1x1x128.onnx `
  --channels 1 --frames 128
build\onnx-local\llvc_benchmark_cli.exe --iterations 32 `
  --onnx-model models\identity_audio_1x1x128.onnx `
  --onnx-channels 1 `
  --onnx-frames 128 `
  --onnx-warmup 2
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
