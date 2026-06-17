# Architecture

## Pipeline

```text
Microphone / Audio Interface
  -> JUCE audio callback
  -> lock-free input ring buffer
  -> preprocess worker
  -> F0 estimator
  -> voice conversion backend
  -> vocoder
  -> output ring buffer
  -> JUCE audio callback
  -> speaker / audio interface
```

## Realtime Boundary

The audio callback is a hard realtime boundary. It may copy samples between
preallocated buffers and update atomic counters, but it must not allocate memory,
lock a mutex, wait on I/O, run model inference, or write logs.

## Core Modules

- `src/audio`: device-facing audio logic and realtime-safe queues.
- `src/dsp`: pitch and signal processing components.
- `src/inference`: replaceable model backend interfaces.
- `src/profiler`: latency measurements and benchmark reporting.
- `src/common`: shared data structures and result types.

## Sprint 1 Pass-Through

The first realtime app uses JUCE's `AudioAppComponent` to open an audio device
and routes input samples directly to output samples through `AudioEngine`.
It also mirrors input blocks into `AudioWorkerPipeline` in shadow mode so the
worker queue can be measured without changing audible pass-through behavior.
Device selection and status rendering happen on the UI thread; the callback only
copies samples, clears samples, pushes/pops SPSC queues, and touches atomic
flags/counters.

See `docs/audio_callback_flow.md` and `docs/worker_pipeline.md` for the callback
boundary.

## Sprint 4 F0 Baseline

`PitchYIN` provides the first C++ F0 estimator. It is benchmarked independently
and is not yet wired into the realtime worker path. See `docs/f0_estimation.md`.

## Sprint 5 Inference Backend

`AudioWorkerPipeline` can now call an `IVoiceConversionBackend` on the worker
thread. Sprint 5 uses `DummyVoiceConversionBackend` for pipeline verification.
Sprint 6 adds optional ONNX Runtime session loading behind
`LLVC_ENABLE_ONNXRUNTIME`. See `docs/inference_backend.md`.

## Sprint 7/8 Tensor Adapter And ONNX Path

`AudioTensorAdapter` defines the current fixed tensor boundary between audio
chunks and ONNX-style backends: `float32[1, channels, frames]`. This keeps model
I/O conversion testable before real model selection.

When `LLVC_ENABLE_ONNXRUNTIME=ON`, `OnnxBackend` now loads a model, inspects its
single audio input/output names and fixed tensor shapes, then calls ONNX Runtime
from the worker-side backend process path. This does not move model inference
into the realtime audio callback.

## Initial Backend Choice

Inference is intentionally absent from Sprint 0. The first backend will be a
dummy ONNX-compatible path used to verify scheduling, chunking, and profiling
before any real voice conversion model is selected.
