# Headless UI Comparison Harnesses

This directory contains the current Windows comparison harnesses for Stygian and several native UI libraries.

Scope:
- Windows-only build lane for now
- Headless CPU-side scene generation for external libraries
- CPU-authoring-full-build lane for Stygian
- Raw offscreen GPU-resident mode for Stygian
- Eval-only replay mode for Stygian
- Shared scenes:
  - fixed-position card grid with a short text label
  - text wall with per-line dynamic hot spans
  - graph canvas with labeled nodes and orthogonal wire segments
  - inspector/property-grid rows with labels and value pills
  - hierarchy/tree pane rows with indent depth, markers, and type chips
- Shared scenarios: `static`, `sparse`, `fullhot`

What this harness is good for:
- comparing scene build cost without platform backends muddying the water
- checking command growth and raw CPU overhead across libraries
- getting repeatable CSV output we can stick into the perf docs later

What this harness is not:
- a GPU renderer benchmark
- a compositor/present benchmark
- proof that one library's renderer is faster than another when they all use different backends

Important:
- `dvui`, `egui`, `imgui`, `clay`, and `nuklear` are measured in `headless-primitive` mode
- `stygian` is measured in `cpu-authoring-full-build`, `raw-gpu-resident`, and `eval-only-replay` modes
- `cpu-authoring-full-build` is the CPU authoring comparison lane for Stygian in this harness
- `raw-gpu-resident` and `eval-only-replay` are useful Stygian-native context, not pure CPU-builder rows
- Clay's graph lane is node-heavy but wire-light in this headless setup. That's a harness limitation, not a serious renderer claim.
- `dvui` is currently pinned to Zig `0.14.1` in this lane. The harness does not build cleanly on Zig `0.15.2` yet, so the build script falls back to an official `0.14.1` toolchain for that target.
- The `dvui` harness forces the fetched source onto its stb text path for this headless lane. That keeps the benchmark about scene churn instead of freetype toolchain plumbing.

Pinned upstreams:
- dvui: `654ee61a8e55ebac9386c75724ccfbf8ffc0c6da`
- egui: `0.31.1` via Cargo
- Dear ImGui: `41765fbda723d23e04e98afec40447d149d02ec8`
- Clay: `76ec3632d80c145158136fd44db501448e7b17c4`
- Nuklear: `4aff9c7c8a5b56b9cbdd7245e5cbec0db5da6d94`

Current harnesses:
- `stygian_headless_bench.c`
- `dvui_headless_bench/`
- `egui_headless_bench/`
- `imgui_headless_bench.cpp`
- `clay_headless_bench.c`
- `nuklear_headless_bench.c`

Workflow:
1. `powershell -NoProfile -ExecutionPolicy Bypass -File benchmarks\comparison\fetch_deps.ps1`
2. `powershell -NoProfile -ExecutionPolicy Bypass -File benchmarks\comparison\build_windows.ps1`
3. `powershell -NoProfile -ExecutionPolicy Bypass -File benchmarks\comparison\run_windows.ps1`
4. `powershell -NoProfile -ExecutionPolicy Bypass -File benchmarks\comparison\run_windows.ps1 -Scenes graph`
5. `powershell -NoProfile -ExecutionPolicy Bypass -File benchmarks\comparison\run_windows.ps1 -NoBuild`
6. `powershell -NoProfile -ExecutionPolicy Bypass -File benchmarks\comparison\run_windows.ps1 -NoBuild -Libraries stygian,egui,imgui,clay,nuklear -Scenes inspector`
7. `powershell -NoProfile -ExecutionPolicy Bypass -File benchmarks\comparison\run_windows.ps1 -NoBuild -Libraries stygian,egui,imgui,clay,nuklear -Scenes hierarchy`

Notes:
- `build_windows.ps1` uses your installed Zig if it is already `0.14.x`.
- If your Zig is newer, the script pulls in a local official `0.14.1` toolchain just for the `dvui` lane.
- `run_windows.ps1 -NoBuild` reruns measurements against the current binaries without rebuilding.
- `run_windows.ps1 -Libraries ...` lets you isolate the targets you actually care about.
- `build_windows.ps1 -SkipDvui` is useful in environments where Zig sandboxing gets in the way of the `dvui` lane.
- The Stygian harness sizes its element pool from the scene budget. That avoids undersized allocations on hierarchy and inspector scenes, where a bad cap can quietly poison the numbers.

Output:
- executables: `build/comparison/`
- run artifacts: `build/comparison/runs/<timestamp>/`
- stable latest snapshot: `build/comparison/latest/`
- committed repo snapshot: `benchmarks/comparison/latest_results.md`
- `summary.log` also includes `STYGIANDETAIL` lines with scene, average element count, and upload bytes for the Stygian runs

Next steps:
- add Linux/macOS build runners once the base lane is stable
- add more Rust/Zig harnesses after the methodology settles down
