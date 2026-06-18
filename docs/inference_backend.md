# Inference Backend

## Sprint 8 Scope

Sprint 5 introduced the replaceable inference backend path without requiring
ONNX Runtime to be installed. Sprint 6 added optional ONNX Runtime session
loading. Sprint 8 wires the fixed audio tensor adapter into
`OnnxBackend::process` for a first real ONNX Runtime execution path.

Current backends:

- `DummyVoiceConversionBackend`: copies an `AudioChunk` to output, with optional
  gain and polarity changes. This verifies model loading, warm-up, process
  timing, tensor-like input/output conversion, and worker-thread integration.
- `OnnxBackend`: dependency-free placeholder by default. When
  `LLVC_ENABLE_ONNXRUNTIME=ON`, it creates an ONNX Runtime session in
  `loadModel`, inspects one input and one output name, validates fixed
  `float32[1, channels, frames]` tensors, and calls `Ort::Session::Run` in
  `process`.

## Worker Integration

`AudioWorkerPipeline` accepts a non-owning `IVoiceConversionBackend*` while the
pipeline is stopped. The realtime callback still only pushes fixed `AudioBlock`
items into an SPSC queue. The worker thread converts those blocks into
channel-major `AudioChunk` data and calls the backend outside the callback.

If backend processing fails, the worker falls back to the dry input block and
increments `backendErrorBlocks`.

## Fixed Tensor Adapter

`AudioTensorAdapter` converts between `AudioChunk` and a fixed ONNX-style
`[1, channels, frames]` float tensor. The adapter is independent of ONNX Runtime
so the shape contract can be tested before a real model is connected.

Current assumptions:

- batch size is always 1
- channel-major sample order
- fixed channel count and frame count per backend configuration
- float32 input/output tensors
- exactly one ONNX input tensor and one ONNX output tensor

## Warm-Up

`IVoiceConversionBackend::warmUp` runs one or more process calls before benchmark
measurement. The dummy backend reports warm-up counts through `BackendStats`.

## Benchmark Stage

The benchmark CLI records `dummy_backend_process` from backend stats and keeps
`worker_process_observed` as the full worker-thread observation.

```powershell
build\manual\llvc_benchmark_cli.exe --iterations 32 --dummy-delay-us 1000 --csv build\manual\latency_report.csv
```

With ONNX Runtime enabled, pass `--onnx-model` to run a backend-only ONNX smoke
benchmark and emit `onnx_backend_process`. The identity model below is only a
pipeline smoke test; it is not a voice conversion model.

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

## ONNX Runtime Build

Configure with either `LLVC_ONNXRUNTIME_ROOT`, or both
`LLVC_ONNXRUNTIME_INCLUDE_DIR` and `LLVC_ONNXRUNTIME_LIBRARY`.

```powershell
powershell -ExecutionPolicy Bypass -File tools\install_onnxruntime_windows.ps1
cmake --preset onnx-local
cmake --build --preset onnx-local
```

## Next ONNX Work

The next ONNX steps should add:

- Execution provider selection.
- Model-specific adapters for feature tensors that are not raw audio chunks.
- Local ONNX Runtime smoke data once the runtime package and a fixed-shape test
  model are installed.
