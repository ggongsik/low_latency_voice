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
