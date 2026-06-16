# Latency Budget

## Phase Targets

| Phase | Target | Notes |
| --- | ---: | --- |
| 0 | Baseline only | Measure existing tools and local pass-through. |
| 1 | < 100 ms | Stable MVP, quality is secondary. |
| 2 | < 50 ms | Better conversational and streaming feel. |
| 3 | < 30 ms | Requires aggressive chunk and model optimization. |
| 4 | < 20 ms | Research goal; likely needs streamable models. |

## Initial Report Shape

```text
[Latency Report]
Input Buffer:       5.3 ms
Preprocess:         0.8 ms
F0 Estimation:      4.1 ms
VC Inference:       12.7 ms
Vocoder:            18.9 ms
Output Buffer:      5.3 ms
Estimated Total:    47.1 ms
XRuns:              0
```
