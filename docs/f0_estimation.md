# F0 Estimation

## Sprint 4 Baseline

The first F0 estimator is a C++ YIN baseline in `src/dsp/PitchYIN`.

It is designed for early latency and correctness measurements:

- no heap allocation inside `PitchYIN::estimate`
- configurable min/max F0 range
- configurable YIN threshold
- RMS gate for silence and very low-level input
- voiced/unvoiced output
- confidence score derived from cumulative mean normalized difference

## Defaults

| Setting | Value |
| --- | ---: |
| Min frequency | 50 Hz |
| Max frequency | 1000 Hz |
| YIN threshold | 0.15 |
| RMS threshold | 0.005 |

## Benchmark Stage

The benchmark CLI records `pitch_yin_2048`, using a 2048-sample 220 Hz sine
frame at 48 kHz.

```powershell
build\manual\llvc_benchmark_cli.exe --iterations 32 --dummy-delay-us 1000 --csv build\manual\latency_report.csv
```

## Known Limits

This is a baseline estimator, not the final pitch path. It is expected to work
well on clean monophonic frames, while noisy speech, breath, consonants, and
rapid pitch transitions will need smoothing, frame skipping, and possibly an
ONNX/RMVPE-style estimator later.
