# Worker Pipeline

## Sprint 2 Scope

Sprint 2 introduces a worker-thread pipeline without changing audible
pass-through behavior.

The realtime callback still writes dry input directly to output through
`AudioEngine::processPassThrough`. In parallel, it submits the same input block
to `AudioWorkerPipeline` in shadow mode. The worker copies input blocks to an
output queue as dummy processing, and the callback consumes processed blocks only
for queue and late-frame accounting.

## Flow

```text
Audio callback
  -> dry pass-through output
  -> fixed AudioBlock copy
  -> input SPSC queue

Worker thread
  -> pop input block
  -> optional dummy delay
  -> push processed block to output SPSC queue

Audio callback
  -> consume output queue in shadow mode
  -> count late output when no processed block is ready
```

## Realtime Safety

`AudioWorkerPipeline` uses fixed-size `AudioBlock` storage:

- Maximum channels: 2
- Maximum frames: 2048
- No heap allocation in the callback
- No locks in the callback
- No blocking waits in the callback

The worker thread may sleep and do heavier work. Future inference modules should
run there, not inside the audio callback.

## Fallback Policy

The future replacing path is already implemented for tests:

- If processed output is available, copy it to the output buffers.
- If processed output is late, copy dry input to output.
- If input cannot fit in the fixed block, count it as unsupported and dropped.

The app currently keeps this replacing path disabled so Sprint 2 cannot add
audible latency.

## Thread Priority

Thread priority is not modified yet. Priority experiments should be measured in
a later sprint after baseline queue behavior and latency reporting are stable.

## Timing Stats

The worker thread records lightweight atomic timing snapshots:

- last worker processing duration
- average worker processing duration
- max worker processing duration

These values are for status reporting and coarse benchmark correlation. Detailed
stage timing belongs in `LatencyProfiler` and benchmark CSV output.
