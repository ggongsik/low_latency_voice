# AGENTS.md

## Project Goal

This repository implements an ultra-low-latency real-time character voice changer
engine using C++, JUCE, and optimized AI inference backends.

The primary goal is measurable latency reduction across the entire real-time
audio pipeline, not only voice conversion quality.

## Core Rules

1. Never perform heavy work inside the audio callback.
2. Do not allocate memory inside the audio callback.
3. Do not use mutex locks inside the audio callback.
4. Do not perform file I/O or logging inside the audio callback.
5. Every optimization must include before/after benchmark numbers.
6. Keep modules small and testable.
7. Prefer C++17 or later.
8. Prefer CMake-based builds.
9. Python may be used for model export or offline analysis, but not in the real-time audio path.
10. Do not introduce large architecture changes without updating `docs/architecture.md`.

## Required Checks

Before finishing a task, run or update:

- Relevant unit tests.
- Build check.
- Benchmark if performance-related.
- Documentation if architecture changed.

## Coding Style

- Keep audio, DSP, inference, and profiling modules separate.
- Use clear interfaces.
- Avoid global state.
- Prefer RAII.
- Prefer fixed-size buffers in real-time paths.
- Use explicit error handling.

## Pull Request Format

Each PR should include:

1. What changed.
2. Why it changed.
3. How it was tested.
4. Latency or performance impact, if relevant.
5. Known limitations.
