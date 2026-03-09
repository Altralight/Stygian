# Stygian vs Contemporary Native UI Libraries - Render Performance Comparison

## Methodology

- Harness: `examples/perf_pathological_suite.c`
- Platform:
  - CPU: `Intel(R) Core(TM) i7-4710HQ CPU @ 2.50GHz`
  - GPU: `Intel(R) HD Graphics 4600`
  - OS: `Microsoft Windows 11 Pro 10.0.26100`
  - Driver: `20.19.15.4624`
- Backend: OpenGL 4.3
- Frame budget: 16.67ms (60 FPS target)
- Metrics:
  - CPU build time (ms)
  - CPU submit time (ms)
  - draw calls per frame
  - upload bytes per frame
  - wall FPS for raw/no-present runs

Run each case long enough to discard startup noise, then capture steady-state averages plus p95 if available. All libraries should render equivalent scene structure and text density where practical.

For the local Stygian baselines below, raw mode runs with present disabled, GPU timer queries disabled, and ICC auto-probing disabled. That is intentional. On this HD 4600, GL timer queries were distorting max-throughput measurements badly enough to make the benchmark lie.

## Hardware Context Matters

These local numbers were captured on Intel HD Graphics 4600, not on a modern desktop dGPU. This is old Haswell iGPU hardware with shared system memory, not an RTX-class desktop card with modern bandwidth. If Stygian is moving real SDF scenes well here, that matters more than a flattering number on much stronger hardware.

Read the results this way:

- lead with Stygian's native GPU-path results
- use the CPU-builder lane as a secondary authoring-cost comparison
- remember that a stronger dGPU will likely help Stygian more than it helps a CPU-only builder, but the exact gain still has to be measured

There is now a separate Windows comparison lane in [benchmarks/comparison/README.md](/D:/Projects/Code/Stygian/benchmarks/comparison/README.md). That lane is useful, but it is still mixed-mode by design:

- `dvui`, `egui`, `imgui`, `clay`, and `nuklear` are measured as `headless-primitive` CPU command builders
- `stygian` is measured as `raw-gpu-resident` and `eval-only-replay`
- those rows are comparable as workload context, not as a final "one number wins everything" verdict
- `dvui` is pinned to Zig `0.14.1` in this lane because the current harness does not build cleanly on Zig `0.15.2`

## Current Stygian Baseline (Local, Raw, No-Present)

| Case | Count | Dirty | Wall FPS | Render ms | Build ms | Submit ms | Draw Calls | Upload Bytes |
|------|-------|-------|----------|-----------|----------|-----------|------------|--------------|
| Static scene (clean replay) | 10,000 | 0 | 457.66 | 1.8602 | 1.5752 | 0.2850 | 1 | 0 |
| Sparse dirty scene | 10,000 | 100 | 418.44 | 2.0395 | 1.6789 | 0.3606 | 1 | 30,784 |
| Full-hot scene | 10,000 | all | 185.58 | 5.0920 | 3.9017 | 1.1903 | 1 | 2,087,072 |
| Static scene (clean replay) | 100,000 | 0 | 60.41 | 14.2607 | 11.1202 | 3.1405 | 1 | 0 |
| Sparse dirty scene | 100,000 | 1,000 | 75.64 | 11.3359 | 9.0200 | 2.3159 | 1 | 247,936 |
| Full-hot scene | 100,000 | all | 20.33 | 47.9315 | 36.5910 | 11.3405 | 1 | 20,807,072 |
| Text editor static scene | 122 visible glyph elements | 0 | 579.77 | 1.5522 | 1.4728 | 0.0794 | 1 | 0 |
| Node graph showcase (pretty) | 64 nodes, labeled | scene redraw | 512.50 | 1.7715 | 1.6773 | 0.0942 | 1 | scene-dependent |
| Node graph benchmark mode | 64 nodes, stripped | scene redraw | 509.00 | 1.7863 | 1.6920 | 0.0943 | 1 | scene-dependent |

Notes:

- `100k @ 500 FPS` is not real for a dense visible scene on this machine.
- The invalidation/replay and SoA data-model story is still real: static frames do reach zero upload after warmup.
- Sparse dirty cases scale much better than full-hot cases, which is exactly what the architecture is supposed to buy.
- The node/text showcase numbers were previously underreported because the benchmark path was paying for low-grade Windows timers and GL timer-query overhead.

## Current Windows Headless Comparison Lane (10k, Local)

Artifacts:

- [latest summary.md](/D:/Projects/Code/Stygian/build/comparison/latest/summary.md)
- [latest summary.csv](/D:/Projects/Code/Stygian/build/comparison/latest/summary.csv)
- [latest summary.log](/D:/Projects/Code/Stygian/build/comparison/latest/summary.log)

These rows come from a `0.5s` capture after a `0.25s` warmup. That is enough for a live matrix. It is not enough for the final long-form appendix.

Important caveats:

- Older pre-`20260309` comparison artifacts overstated some Stygian static rows because the comparison harness had allocator/replay accounting bugs. Ignore those older Stygian comparison numbers.
- This lane is only fair when you compare like with like.

### Reading Order

- Start with the `Current Stygian Baseline` section in this document.
- Then read `Stygian Native Modes` in [latest summary.md](/D:/Projects/Code/Stygian/build/comparison/latest/summary.md).
- Use `CPU Builder Rows` in that same file for the cross-library CPU authoring lane.
- Do not flatten the whole file into one leaderboard. It is not that kind of benchmark.

### CPU Builder Snapshot (Static)

| Library | Mode | Cards FPS | Text Wall FPS | Graph FPS | Inspector FPS | Hierarchy FPS |
|---------|------|-----------|---------------|-----------|---------------|---------------|
| Stygian | cpu-authoring-full-build | 27.78 | 33.30 | 21.11 | 17.90 | 18.59 |
| egui | headless-primitive | 50.89 | 60.04 | 26.39 | 26.34 | 32.10 |
| Dear ImGui | headless-primitive | 30.31 | 56.79 | 20.64 | 18.20 | 15.66 |
| Clay | headless-primitive | 3.32 | 2.84 | 3.25 | 0.88 | 0.22 |
| Nuklear | headless-primitive | 431.38 | 1221.52 | 201.71 | 141.80 | 200.37 |

Interpretation:

- This is the fair CPU churn comparison in the current harness.
- Nuklear is still the fastest lightweight CPU-builder in this matrix.
- Stygian is not winning raw CPU command churn here, especially in text-heavy and editor-heavy scenes.
- That does not mean Nuklear is beating Stygian at the same end-to-end job. It means Nuklear is paying a smaller bill in this lane.

### Stygian Native Snapshot (Static)

| Mode | Cards FPS | Text Wall FPS | Graph FPS | Inspector FPS | Hierarchy FPS |
|------|-----------|---------------|-----------|---------------|---------------|
| raw-gpu-resident | 37.88 | 39.60 | 32.21 | 20.46 | 24.87 |
| eval-only-replay | 39.74 | 39.04 | 30.46 | 21.71 | 15.23 |

Interpretation:

- This is where Stygian's architecture actually shows up: replay, zero-upload static frames, and a real renderer path.
- This is the section that best represents Stygian as a GPU-native SDF framework rather than as a CPU widget builder.
- These rows are not fair CPU-builder comparisons, but they are the direct Stygian-native measurements.

### Stygian Scene Cost Snapshot

These are the current real scene sizes and upload costs from the same latest artifact:

| Scene | Static Elements | Static Upload | Sparse Upload | Fullhot Upload |
|------|-----------------|---------------|---------------|----------------|
| cards | 110,000 | 0 | 57,408 | 2,116,608 |
| textwall | 100,000 | 0 | 241,280 | 20,833,280 |
| graph | 187,437 | 0 | 4,510,288 | 43,321,632 |
| inspector | 183,301 | 0 | 430,643 | 37,873,888 |
| hierarchy | 182,500 | 0 | 427,024 | 38,007,424 |

Scene characteristics:

- `cards` and `textwall` are not “10,000 elements” in Stygian once text and decoration are fully counted.
- `graph`, `inspector`, and `hierarchy` are heavy scenes. They are the kind of editor workloads that expose real renderer cost instead of letting the engine coast on a toy case.
- The static rows are still zero-upload after warmup, which is the structural proof Stygian is supposed to deliver.

### Summary

- The Stygian Native section is the main lane for understanding Stygian as a GPU-native SDF system.
- The CPU Builder section is a secondary apples-to-apples lane for Stygian authoring cost versus the other headless builders.
- Nuklear remains the fastest lightweight CPU-builder lane in this matrix.
- Stygian's differentiator is not "highest static FPS in every harness." It is the combination of data-driven immediate authoring, persistent scene/state backing, replay/skip paths, dirty upload accounting, GPU-native SDF rendering, and a real renderer path with one draw call.

### Latest Sanity Measurement

- After the recent OpenGL state-cleanup pass, `node_graph_demo` pretty raw now lands around `598 FPS` with `render_ms=1.49` and `submit_ms=0.135` on this machine.
- Treat that as a showcase sanity check, not as a replacement for the fuller matrix above.

## Test Cases

### Case 1: Static UI (N=10,000 elements, no mutations)

| Library | Build ms | Submit ms | Draw Calls | Upload Bytes |
|---------|----------|-----------|------------|--------------|
| Stygian | 1.5752 | 0.2850 | 1 | 0 |
| Dear ImGui | [FILL] | [FILL] | [FILL] | [FILL] |
| Clay | [FILL] | [FILL] | [FILL] | [FILL] |

Notes:

- Stygian achieves zero upload on fully static frames via scope replay.
- Dear ImGui rebuilds vertex buffers every frame by design.
- Clay is renderer-agnostic CPU layout, not a GPU-resident scene/state-backed renderer.

### Case 2: Partial mutation (N=10,000, 100 elements changed per frame)

| Library | Build ms | Submit ms | Draw Calls | Upload Bytes |
|---------|----------|-----------|------------|--------------|
| Stygian | 1.6789 | 0.3606 | 1 | 30,784 |
| Dear ImGui | [FILL] | [FILL] | [FILL] | [FILL] |
| Clay | [FILL] | [FILL] | [FILL] | [FILL] |

Notes:

- Stygian uploads only dirty SoA chunks.
- Upload bytes should scale with dirty element count and chunk coverage, not total scene size.

### Case 3: Full stress (N=10,000, all elements mutated every frame)

| Library | Build ms | Submit ms | Draw Calls | Upload Bytes |
|---------|----------|-----------|------------|--------------|
| Stygian | 3.9017 | 1.1903 | 1 | 2,087,072 |
| Dear ImGui | [FILL] | [FILL] | [FILL] | [FILL] |
| Clay | [FILL] | [FILL] | [FILL] | [FILL] |

Notes:

- This is the regime where Stygian's structural advantages narrow and raw backend quality matters more.
- Even here, the data-driven immediate SoA path should still avoid needless CPU-side rebuild patterns outside actual mutation.

## Structural Differences (Not Measurable By Frame Time Alone)

| Feature | Stygian | Dear ImGui | Clay |
|---------|---------|------------|------|
| GPU-resident elements | Yes | No (CPU vertex rebuild) | No |
| Invalidation-driven render | Yes | No | No |
| Eval-only frames (no GPU) | Yes | No | N/A |
| SDF rendering | Yes | No (triangles) | No |
| ICC color management | Yes (Win32) | No | No |
| Single draw call | Yes | No | N/A |
| MTSDF text | Yes | No | No |

## Measurement Gaps

Replace every remaining `[FILL]` slot with real measurements taken from the same machine, same backend settings, and comparable scene content.

Recommended additions:

- p95 frame time
- CPU package and GPU utilization snapshots
- upload bytes over time graph
- screenshots of each test case using identical scene layout
