# Model Notes

Model integration is intentionally deferred until the audio pipeline is stable.

## Requirements

- Batch size 1.
- Fixed-shape streaming chunks where possible.
- Explicit warm-up before measurement.
- Replaceable backend interface.
- CPU baseline before GPU-specific optimization.

## Candidates

Track candidates here once the ONNX dummy backend is working.

## Sprint 5 Backend Status

The current inference path uses `DummyVoiceConversionBackend` to verify load,
warm-up, process timing, worker integration, and benchmark reporting. The
`OnnxBackend` class exists as a placeholder and intentionally reports that ONNX
Runtime is not enabled.
