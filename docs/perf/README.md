# Performance Docs

Use this section for measurable performance practice:

- `gates/` for threshold definitions and gate policy
- `profiling/` for harness recipes and metric interpretation
- `hardware-notes/` for backend and device-class caveats
- `benchmark_comparison.md` for the current Stygian baseline and the live C/C++/Rust/Zig comparison matrix
- `benchmarks/comparison/README.md` for the Windows headless comparison lane, harness workflow, scene coverage, and toolchain caveats
- `ui_optimization_research.md` for the current optimization playbook, text-rendering research, and stress-test plan

Current local proof artifacts live in:

- `benchmarks/comparison/latest_results.md` for the committed comparison snapshot
- `screenshots/` for the current external-facing captures

Read these in this order:

- `benchmark_comparison.md` -> `Current Stygian Baseline`
- `benchmarks/comparison/latest_results.md` -> `Stygian Native Modes`
- `benchmarks/comparison/latest_results.md` -> `CPU Builder Rows`

Start with the native sections if you want to understand what Stygian is actually doing on the GPU.
Use the CPU Builder section after that for the cross-library scene-build lane.
