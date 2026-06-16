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
