# Stygian Editor Performance Budgets V0

This defines the first hard performance guardrails for the editor core.

The point of these numbers is regression detection, not absolute benchmark
truth. If we intentionally change workload scale or machine class assumptions,
we should update this doc and the smoke test in the same change.

## Scope

The v0 budget test covers these core paths:

- project load from sidecar JSON
- repeated frame resize + layout recompute
- C23 export generation
- viewport interaction primitives (hit-test + marquee select)

## Fixture Profile

- Scene:
  - 1 frame root
  - 80 child rectangles
  - frame auto-layout enabled (horizontal)
- Runtime:
  - load one serialized sidecar payload
  - 4 layout recompute passes under small size deltas
  - one C23 export-size build pass
  - 500 interaction operations (hit-test + select-in-rect)

## Budgets (milliseconds)

- load: `<= 80.0 ms`
- layout: `<= 250.0 ms`
- export: `<= 80.0 ms`
- interaction: `<= 80.0 ms`

## Source of Truth

- Test:
  - `D:\Projects\Code\Stygian\editor\tests\performance_budget_smoke.c`
- Build wrapper:
  - `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_performance_budget_smoke.bat`
- Target:
  - `stygian_editor_performance_budget_smoke` in
    `D:\Projects\Code\Stygian\compile\targets.json`
