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

## Initial Backend Choice

Inference is intentionally absent from Sprint 0. The first backend will be a
dummy ONNX-compatible path used to verify scheduling, chunking, and profiling
before any real voice conversion model is selected.
