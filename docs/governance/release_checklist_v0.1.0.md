# Stygian v0.1.0 Release Checklist

## Scope
- Standalone Stygian repository only.
- OpenGL and Vulkan build parity via manifest runners.
- DDI/perf gates validated on current baseline.

## Release Gates
- [x] `compile/windows/build_mini_apps_all.bat` succeeds.
- [x] `compile/windows/run_perf_gates.bat -Backend both -Seconds 6 -Profile aggressive` passes.
- [x] `compile/run.ps1 -Target text_editor_mini` succeeds.
- [x] `compile/run.ps1 -Target text_editor_mini_vk` succeeds.
- [ ] `git status --short` is clean before tagging.

## Repo Hygiene
- [x] No tracked `deprecated/`, `examples/deprecated/`, or `docs/context/`.
- [x] No tracked binaries/build outputs (`.exe`, `build/`).
- [x] Stygian-local `.gitignore` enforces standalone policy.

## Docs Sanity
- [x] `README.md` points to `compile/run.ps1` and `compile/run.sh`.
- [x] Integration quickstart uses unified compile entrypoints.
- [x] Perf gate policy points to `compile/windows/run_perf_gates.bat`.

## Linux / macOS Gate (CI)
- [x] `linux-build` CI job passes on `ubuntu-latest`
- [x] `macos-build` CI job passes on `macos-latest`
- [x] `tier1_safety` compiles and passes headless on Linux (xvfb)

## Sponsor / Funding Readiness
- [x] README has no "infancy" language
- [x] CI badges visible on README
- [x] CONTRIBUTING.md exists
- [x] CHANGELOG.md exists
- [x] docs/perf/benchmark_comparison.md has real measurements filled in
- [x] docs/architecture/pipeline_diagram.md exists
- [x] docs/reference/core/ has stygian.h API reference
- [x] node_graph_demo compiles and runs
- [x] Widget taxonomy clearly separates Shipped / In Progress / Planned
- [x] At least one screenshot in repo (screenshots/ directory or assets/)

## First Release Tag
- [ ] Tag created: `v0.1.0`
- [ ] Tag annotation includes summary of:
  - DDI/eval-only frame split
  - SoA triple-buffer layout
  - multi-platform CI
  - shipped widget baseline

## Publish
- [x] Push `main` to private GitHub repository.
- [ ] Push tags.
