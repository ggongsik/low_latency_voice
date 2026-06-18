# Benchmark Protocol

## Principles

- Measure before optimizing.
- Keep input sample rate, block size, chunk size, and backend fixed for each run.
- Report moving average, p95, maximum latency, underruns, and dropouts.
- Record hardware, driver, OS, and build configuration.

## Baseline Comparisons

1. Existing voice changer end-to-end latency.
2. This project's C++ pass-through latency.
3. Worker-thread dummy processing latency.
4. ONNX dummy inference latency.
5. Real model latency by chunk size.

## Output Files

Benchmark CSV files should be generated under `build/` and should not be
committed.

## Sprint 3 CLI

The benchmark CLI prints a terminal latency report and can optionally export CSV:

```powershell
build\manual\llvc_benchmark_cli.exe --iterations 128 --dummy-delay-us 1000 --csv build\manual\latency_report.csv
```

Optional ONNX backend smoke benchmark:

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

The generated identity model verifies ONNX Runtime load/run overhead for the
current tensor contract. Do not use it as a quality benchmark.

Current stages:

- `ring_push_pop`: SPSC queue push/pop loop cost.
- `worker_shadow_callback`: callback-side worker queue submit/consume cost.
- `worker_process_observed`: worker-thread dummy processing time observed through
  atomic stats.
- `dummy_backend_process`: average process time reported by
  `DummyVoiceConversionBackend`.
- `onnx_backend_process`: average process time reported by `OnnxBackend` when
  `--onnx-model` is provided and ONNX Runtime is enabled.
- `pitch_yin_2048`: C++ YIN F0 estimation on a 2048-sample frame.

CSV columns:

- `stage`
- `samples`
- `last_ms`
- `average_ms`
- `moving_average_ms`
- `p95_ms`
- `min_ms`
- `max_ms`
