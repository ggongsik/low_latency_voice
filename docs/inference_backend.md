# Inference Backend

## Sprint 6 Scope

Sprint 5 introduced the replaceable inference backend path without requiring
ONNX Runtime to be installed. Sprint 6 adds build wiring for optional ONNX
Runtime session loading.

Current backends:

- `DummyVoiceConversionBackend`: copies an `AudioChunk` to output, with optional
  gain and polarity changes. This verifies model loading, warm-up, process
  timing, tensor-like input/output conversion, and worker-thread integration.
- `OnnxBackend`: dependency-free placeholder by default. When
  `LLVC_ENABLE_ONNXRUNTIME=ON`, it creates an ONNX Runtime session in
  `loadModel`. Tensor input/output conversion is intentionally still pending.

## Worker Integration

`AudioWorkerPipeline` accepts a non-owning `IVoiceConversionBackend*` while the
pipeline is stopped. The realtime callback still only pushes fixed `AudioBlock`
items into an SPSC queue. The worker thread converts those blocks into
channel-major `AudioChunk` data and calls the backend outside the callback.

If backend processing fails, the worker falls back to the dry input block and
increments `backendErrorBlocks`.

## Warm-Up

`IVoiceConversionBackend::warmUp` runs one or more process calls before benchmark
measurement. The dummy backend reports warm-up counts through `BackendStats`.

## Benchmark Stage

The benchmark CLI records `dummy_backend_process` from backend stats and keeps
`worker_process_observed` as the full worker-thread observation.

```powershell
build\manual\llvc_benchmark_cli.exe --iterations 32 --dummy-delay-us 1000 --csv build\manual\latency_report.csv
```

## ONNX Runtime Build

Configure with either `LLVC_ONNXRUNTIME_ROOT`, or both
`LLVC_ONNXRUNTIME_INCLUDE_DIR` and `LLVC_ONNXRUNTIME_LIBRARY`.

```powershell
cmake -S . -B build/onnx-local -G Ninja `
  -DLLVC_ENABLE_ONNXRUNTIME=ON `
  -DLLVC_ONNXRUNTIME_ROOT=C:\path\to\onnxruntime
cmake --build build/onnx-local
```

## Next ONNX Work

Actual ONNX inference should add:

- Fixed-shape input/output tensor conversion.
- Execution provider selection.
- Warm-up inference before measurement.
- Dedicated ONNX benchmark CSV rows.
