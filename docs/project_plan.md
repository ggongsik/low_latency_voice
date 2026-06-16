# Project Plan

## Objective

Build an ultra-low-latency real-time character voice changer engine with a C++
audio pipeline, measurable latency reporting, and replaceable AI inference
backends.

## Strategy

The project will start as a new C++ engine. Python-first projects such as
w-okada/voice-changer remain useful baseline references, but directly modifying
one of them would keep the realtime path tied to Python process boundaries,
larger buffering assumptions, and UI/backend coupling.

## Milestones

1. Sprint 0: repository structure, CMake, JUCE target preparation, core interfaces.
2. Sprint 1: JUCE audio pass-through prototype.
3. Sprint 2: SPSC ring buffer and worker thread split.
4. Sprint 3: latency profiler with terminal report and CSV export.
5. Sprint 4: YIN F0 baseline.
6. Sprint 5: dummy ONNX Runtime backend.
7. Sprint 6: real voice conversion model experiment.
8. Sprint 7: TensorRT/FP16 optimization.
9. Sprint 8: demo hardening and portfolio report.

## Push Policy

Push after each large milestone is complete and verified. Keep each milestone
small enough to review independently.
