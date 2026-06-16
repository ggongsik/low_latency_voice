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

Current stages:

- `ring_push_pop`: SPSC queue push/pop loop cost.
- `worker_shadow_callback`: callback-side worker queue submit/consume cost.
- `worker_process_observed`: worker-thread dummy processing time observed through
  atomic stats.
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
