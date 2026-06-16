# Inference Backend

## Sprint 5 Scope

Sprint 5 introduces the replaceable inference backend path without requiring
ONNX Runtime to be installed.

Current backends:

- `DummyVoiceConversionBackend`: copies an `AudioChunk` to output, with optional
  gain and polarity changes. This verifies model loading, warm-up, process
  timing, tensor-like input/output conversion, and worker-thread integration.
- `OnnxBackend`: placeholder that returns a clear error while ONNX Runtime is not
  enabled in the build.

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

## Next ONNX Work

Actual ONNX Runtime support should add:

- CMake option for ONNX Runtime include/lib paths.
- `OnnxBackend::loadModel` session creation.
- Fixed-shape input/output tensor conversion.
- Execution provider selection.
- Warm-up inference before measurement.
- Dedicated ONNX benchmark CSV rows.
