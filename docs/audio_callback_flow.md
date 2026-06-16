# Audio Callback Flow

## Sprint 1 Pass-Through Path

```text
JUCE device callback
  -> MainComponent::getNextAudioBlock
  -> collect preallocated channel pointers
  -> AudioWorkerPipeline::processShadowFromAudioThread
  -> submit fixed block to worker queue and consume completed output for stats
  -> AudioEngine::processPassThrough
  -> copy input samples to output samples
```

The audible path is intentionally direct. It proves the device setup, callback
shape, bypass flag, basic counters, and worker queue plumbing before any AI
inference is introduced.

## Realtime Safety Rules

`getNextAudioBlock` and functions called from it must not:

- Allocate heap memory.
- Lock a mutex.
- Write logs or files.
- Call model inference.
- Wait for UI, device, network, or disk work.

Allowed operations in the callback:

- Stack-local fixed-size arrays.
- Copying or clearing samples.
- Reading atomic flags.
- Incrementing atomic counters.

## Current Counters

- `processedBlocks`: increments after each non-null output callback.
- `xrunCount`: increments when the callback receives missing output buffers or
  missing channel pointers.
- `lateOutputBlocks`: increments when the worker output queue has no processed
  block ready for the current callback.
- `estimatedBlockLatencyMs`: reports one hardware block duration as
  `1000 * blockSize / sampleRate`.

The estimate is not end-to-end latency. It is a first hook that will later be
combined with ring buffer depth, worker queue time, inference time, vocoder
time, and output queue time.

## UI Thread

The JUCE device selector, bypass toggle, reset button, and status label all run
outside the audio callback. The bypass toggle writes an atomic flag that the
callback reads without blocking.
