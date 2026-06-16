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
warm-up, process timing, worker integration, and benchmark reporting.
`OnnxBackend` can now be compiled with ONNX Runtime and can create a session, but
tensor input/output conversion is still pending until a target model format is
chosen.

## Tensor Contract

The current backend-facing audio tensor contract is:

```text
float32[1, channels, frames]
```

`AudioTensorAdapter` validates and copies channel-major `AudioChunk` data into
that shape. Real VC model integration should either match this contract or
document a different model-specific adapter.
