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

There is now a separate Windows comparison lane in [benchmarks/comparison/README.md](../../benchmarks/comparison/README.md). That lane is useful, but it is still mixed-mode by design:

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

- [latest_results.md](../../benchmarks/comparison/latest_results.md)
- `build/comparison/latest/summary.csv` and `summary.log` are local/generated artifacts from the harness and are not committed to the repo

These rows come from a `0.5s` capture after a `0.25s` warmup. That is enough for a live matrix. It is not enough for the final long-form appendix.

Important caveats:

- Older pre-`20260309` comparison artifacts overstated some Stygian static rows because the comparison harness had allocator/replay accounting bugs. Ignore those older Stygian comparison numbers.
- This lane is only fair when you compare like with like.

### Reading Order

- Start with the `Current Stygian Baseline` section in this document.
- Then read `Stygian Native Modes` in [latest_results.md](../../benchmarks/comparison/latest_results.md).
- Use `CPU Builder Rows` in that same file for the cross-library CPU authoring lane.
- Do not flatten the whole file into one leaderboard. It is not that kind of benchmark.

### CPU Builder Snapshot (Static)

| Library | Mode | Cards FPS | Text Wall FPS | Graph FPS | Inspector FPS | Hierarchy FPS |
|---------|------|-----------|---------------|-----------|---------------|---------------|
| Stygian | cpu-authoring-full-build | 27.40 | 27.26 | 19.18 | 14.83 | 17.16 |
| egui | headless-primitive | 44.39 | 63.45 | 24.58 | 23.07 | 27.23 |
| Dear ImGui | headless-primitive | 32.21 | 49.18 | 16.81 | 15.65 | 13.43 |
| Clay | headless-primitive | 3.24 | 3.26 | 3.05 | 0.83 | 0.19 |
| Nuklear | headless-primitive | 337.90 | 715.13 | 161.50 | 93.15 | 226.81 |

Interpretation:

- This is the fair CPU churn comparison in the current harness.
- Nuklear is still the fastest lightweight CPU-builder in this matrix.
- Stygian is not winning raw CPU command churn here, especially in text-heavy and editor-heavy scenes.
- That does not mean Nuklear is beating Stygian at the same end-to-end job. It means Nuklear is paying a smaller bill in this lane.

### Stygian Native Snapshot (Static)

| Mode | Cards FPS | Text Wall FPS | Graph FPS | Inspector FPS | Hierarchy FPS |
|------|-----------|---------------|-----------|---------------|---------------|
| raw-gpu-resident | 35.41 | 39.33 | 19.76 | 20.30 | 23.93 |
| eval-only-replay | 35.85 | 38.04 | 23.28 | 14.86 | 22.14 |

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
| graph | 187,437 | 0 | 4,510,288 | 50,252,064 |
| inspector | 183,301 | 0 | 430,612 | 37,873,888 |
| hierarchy | 182,500 | 0 | 427,024 | 38,007,424 |

Scene characteristics:

- `cards` and `textwall` are not "10,000 elements" in Stygian once text and decoration are fully counted.
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
| Dear ImGui | 31.0492 | N/A (headless) | N/A | N/A |
| Clay | 308.8195 | N/A (headless) | N/A | N/A |

Notes:

- Stygian achieves zero upload on fully static frames via scope replay.
- Dear ImGui and Clay rows here come from the headless CPU-builder lane, so submit, draw-call, and upload fields are not applicable.
- Dear ImGui rebuilds vertex buffers every frame by design.
- Clay is renderer-agnostic CPU layout, not a GPU-resident scene/state-backed renderer.

### Case 2: Partial mutation (N=10,000, 100 elements changed per frame)

| Library | Build ms | Submit ms | Draw Calls | Upload Bytes |
|---------|----------|-----------|------------|--------------|
| Stygian | 1.6789 | 0.3606 | 1 | 30,784 |
| Dear ImGui | 34.5171 | N/A (headless) | N/A | N/A |
| Clay | 311.1197 | N/A (headless) | N/A | N/A |

Notes:

- Stygian uploads only dirty SoA chunks.
- Upload bytes should scale with dirty element count and chunk coverage, not total scene size.
- Dear ImGui and Clay rows here come from the headless CPU-builder lane, so submit, draw-call, and upload fields are not applicable.

### Case 3: Full stress (N=10,000, all elements mutated every frame)

| Library | Build ms | Submit ms | Draw Calls | Upload Bytes |
|---------|----------|-----------|------------|--------------|
| Stygian | 3.9017 | 1.1903 | 1 | 2,087,072 |
| Dear ImGui | 36.6071 | N/A (headless) | N/A | N/A |
| Clay | 298.4653 | N/A (headless) | N/A | N/A |

Notes:

- This is the regime where Stygian's structural advantages narrow and raw backend quality matters more.
- Even here, the data-driven immediate SoA path should still avoid needless CPU-side rebuild patterns outside actual mutation.
- Dear ImGui and Clay rows here come from the headless CPU-builder lane, so submit, draw-call, and upload fields are not applicable.

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

The remaining gap in this document is end-to-end renderer data for the non-Stygian rows. The current Dear ImGui and Clay values come from the headless CPU-builder lane, so submit, draw-call, and upload columns are not applicable there yet.

Recommended additions:

- p95 frame time
- CPU package and GPU utilization snapshots
- upload bytes over time graph
- screenshots of each test case using identical scene layout
