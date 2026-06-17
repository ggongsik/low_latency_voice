# Requirements

This project is split into a required core build and optional feature builds.

## Required For Core Tests And Benchmarks

| Requirement | Recommended | Where to get it | Notes |
| --- | --- | --- | --- |
| C++ compiler | MSVC 2019 or newer on Windows | https://visualstudio.microsoft.com/visual-cpp-build-tools/ | Install "Desktop development with C++". MinGW can compile the current core tests, but MSVC is the recommended Windows toolchain for JUCE and ONNX Runtime work. |
| CMake | 3.22 or newer | https://cmake.org/download/ | The repository uses CMake presets and requires CMake 3.22+. |
| Ninja | Latest stable | https://ninja-build.org/ | Used by the provided CMake presets. Visual Studio generators are also possible if preferred. |
| Git | Latest stable | https://git-scm.com/downloads | Needed for cloning JUCE and normal source control work. |

Default build:

```powershell
cmake --preset default
cmake --build --preset default
ctest --preset default
```

## Optional: JUCE Realtime App

| Requirement | Where to get it | Configure |
| --- | --- | --- |
| JUCE | https://github.com/juce-framework/JUCE or https://juce.com/get-juce/download/ | Set `LLVC_BUILD_JUCE_APP=ON` and `LLVC_JUCE_DIR=C:\path\to\JUCE`. |

Example:

```powershell
git clone https://github.com/juce-framework/JUCE.git C:\dev\JUCE
cmake -S . -B build\juce-local -G Ninja `
  -DLLVC_BUILD_JUCE_APP=ON `
  -DLLVC_JUCE_DIR=C:\dev\JUCE
cmake --build build\juce-local
```

## Optional: ONNX Runtime Backend

| Requirement | Where to get it | Configure |
| --- | --- | --- |
| ONNX Runtime C/C++ package | https://onnxruntime.ai/docs/get-started/with-cpp.html and https://github.com/microsoft/onnxruntime/releases | Set `LLVC_ENABLE_ONNXRUNTIME=ON` and either `LLVC_ONNXRUNTIME_ROOT`, or both `LLVC_ONNXRUNTIME_INCLUDE_DIR` and `LLVC_ONNXRUNTIME_LIBRARY`. |

CPU example:

```powershell
cmake -S . -B build\onnx-local -G Ninja `
  -DLLVC_ENABLE_ONNXRUNTIME=ON `
  -DLLVC_ONNXRUNTIME_ROOT=C:\dev\onnxruntime
cmake --build build\onnx-local
```

If using a manually unpacked release, the root should contain an `include`
directory with `onnxruntime_cxx_api.h` and a `lib` directory containing the ONNX
Runtime library. On Windows, make sure the ONNX Runtime `.dll` is available next
to the executable or on `PATH` before running.

Current backend contract:

- Exactly one model input and one model output.
- Both tensors must be `float32`.
- Both tensors must use static shape `[1, channels, frames]`.
- Dynamic channel/frame dimensions and model-specific feature tensors need a
  future adapter before they can run in the realtime worker path.

## Optional Later Work

| Requirement | Purpose | Notes |
| --- | --- | --- |
| ASIO SDK | Lower-latency Windows audio backend experiments | JUCE can use ASIO, but Steinberg's SDK has separate licensing and setup. See https://www.steinberg.net/developers/. |
| CUDA-enabled ONNX Runtime | GPU inference experiments | Use the ONNX Runtime install matrix to match CUDA/cuDNN versions. |
| TensorRT | Later FP16/TensorRT optimization | Not required until the TensorRT sprint. |

## Current Local Caveats

- The current development machine does not expose `cmake` on `PATH`, so local
  verification has been done with direct MinGW `g++` commands.
- ONNX Runtime is not installed locally yet, so `LLVC_ENABLE_ONNXRUNTIME=ON` has
  not been linked on this machine.
- The default build path intentionally does not require JUCE or ONNX Runtime.
