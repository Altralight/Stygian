# Batch 5 Data + Advanced Engine V0

## Scope

This batch closes `V2-014`, `V2-016`, `V2-017`, and `V2-018` in bootstrap.
The goal is practical authoring parity, not a full solver rewrite.

## Delivered

### V2-014 Repeater Primitive

- Added a selection-driven repeater surface in properties panel.
- Inputs:
  - copy count
  - step X
  - step Y
- Deterministic naming contract:
  - new instances are named `<source_name>_rep_<index>` with zero padding.
- Supported source kinds in v0:
  - rectangle
  - ellipse
- Unsupported source kinds fail loudly with explicit status.

### V2-016 Advanced Boolean/Merge Authoring UX

- Added boolean authoring block in properties panel with:
  - union
  - subtract
  - intersect
  - exclude
- Added optional flatten-on-compose flow.
- Added smooth/metaball intent toggle with explicit deterministic fallback status:
  - author intent is visible
  - export path remains deterministic boolean output in v0

### V2-017 Smart Guides + Snap Hierarchy

- Added snap hierarchy controls (runtime-wired):
  - grid
  - guides
  - bounds
  - parent edges
- Added smart guide overlays during move/resize drag:
  - vertical/horizontal guide lines render when snapping resolves to adjusted coordinates.

### V2-018 Performance Transparency Surface

- Added performance surface readout in properties panel.
- Surface shows latest measured values and deltas vs budgets for:
  - load
  - layout
  - export
  - interaction
- Metrics are sourced from real editor operations:
  - project load
  - geometry resize/layout operations
  - runtime bundle export/save
  - viewport event handling

## Known Limits

- Repeater currently clones only rectangle/ellipse nodes.
- Smooth/metaball merge is still a deterministic fallback marker path in UX, not a dedicated geometric solver.
- Performance values are latest-sample visibility, not historical profiling series.

## Verification Targets

- `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_generated_scene_runtime_smoke.bat`
- `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_boolean_solver_parity_smoke.bat`
- `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_guides_snap_smoke.bat`
- `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_performance_budget_smoke.bat`
