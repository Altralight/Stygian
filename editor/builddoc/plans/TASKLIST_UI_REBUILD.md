# Stygian Editor UI Rebuild Tasklist

This is the explicit visual/product rebuild lane for bootstrap UX.
It exists because core capabilities now exceed what the current panels can reliably expose.

## Status Markers

- `[ ]` not started
- `[-]` in progress
- `[x]` finished

## Rules

- Every finished item must include evidence in:
  - `D:\Projects\Code\Stygian\editor\builddoc\verification\VERIFICATION_LOG.md`
- Prefer deterministic behavior over flashy UI.
- Any new UX affordance must map cleanly to generated Stygian C23 behavior.

## Immediate Trust-Critical

- [x] UIR-001 Exported Code Ownership Markers
  - why: ownership has to be visible in files, not only in UI badges.
  - done when:
    - generated files contain explicit exporter-owned warnings
    - generated files point to exact hook file locations
    - hook files contain explicit user-owned/non-overwritten notes
  - verification:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_generated_file_layout_smoke.bat`
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_export_diagnostics_smoke.bat`

- [x] UIR-002 Hook Discovery UX
  - why: a safe hook model is useless if users can’t discover and jump to hooks fast.
  - done when:
    - interactive node selection shows exact hook symbol(s)
    - action can open hook stub/source location directly
    - missing hook state can create stub from logic panel
  - verification:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_host_interop_contract_smoke.bat`
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_phase7_behavior_bridge_smoke.bat`

- [x] UIR-003 Export Diff Quality Gate
  - why: tiny edits must produce tiny diffs or trust dies.
  - done when:
    - tiny scene edit yields small textual diff
    - unchanged nodes preserve ordering and symbol names
    - rename-only edits do not churn unrelated output
  - verification:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_export_determinism_smoke.bat`
    - add dedicated diff-quality smoke fixture and target

## Core Shell Rebuild

- [x] UIR-010 Panel Architecture Reset
  - done when:
    - panels are split into: Scene/Layers, Viewport, Inspector, Timeline, Logic, Assets, Diagnostics, Code Insight
    - panel responsibilities do not overlap

- [x] UIR-011 Scene/Layers Panel
  - done when:
    - hierarchy tree supports lock/hide/select/reorder/reparent
    - component and instance states are visible

- [x] UIR-012 Inspector V2
  - done when:
    - sections are explicit: Transform, Layout, Appearance, Effects, Variables, Component
    - no mixed unrelated controls in one block

- [x] UIR-013 Selection + Transform UX
  - done when:
    - stable gizmos/anchors/pivot behavior for single and multi-select
    - numeric edits and drag edits stay synchronized

## Timeline UX

- [x] UIR-020 Timeline Panel CRUD
  - depends on: `V2-015` core timeline data model
  - done when:
    - track and clip list are editable from UI
    - track target/property/layer assignment is exposed

- [x] UIR-021 Keyframe Authoring
  - done when:
    - click/drag keyframe editing works for time and value
    - easing assignment is editable per keyframe

- [x] UIR-022 Layered Clip Semantics Surface
  - done when:
    - layer conflict rule is visible in UI
    - overlapping clip diagnostics are shown when needed

## Logic + Code Insight

- [x] UIR-030 Logic Panel Graph Cleanup
  - done when:
    - event/action graph is readable at a glance
    - missing node/hook/binding states are obvious

- [x] UIR-031 Code Insight V2
  - done when:
    - selected node shows generated region + hook linkage + ownership state
    - jump-to-generated and jump-to-hook actions exist

## Diagnostics + UX Quality

- [x] UIR-040 Diagnostics Panel
  - done when:
    - export/runtime warnings are visible without reading log files
    - each issue points to node ID and display path

- [x] UIR-041 Command UX Baseline
  - done when:
    - shortcut help, command palette, and status feed are consistent

- [x] UIR-042 Visual Consistency Pass
  - done when:
    - text clipping/overlap issues are gone
    - control sizing, spacing, and states are consistent

## Execution Order

1. `UIR-001`
2. `UIR-003`
3. `UIR-002`
4. `UIR-010` -> `UIR-013`
5. `UIR-020` -> `UIR-022`
6. `UIR-030` -> `UIR-042`

## Batch Plan

- `Batch A` Trust + determinism foundation
  - `UIR-001`
  - `UIR-003`
  - `UIR-002`
- `Batch B` Core shell usability
  - `UIR-010`
  - `UIR-011`
  - `UIR-012`
  - `UIR-013`
- `Batch C` Timeline authoring UX
  - `UIR-020`
  - `UIR-021`
  - `UIR-022`
- `Batch D` Logic + code insight
  - `UIR-030`
  - `UIR-031`
- `Batch E` Diagnostics + final consistency
  - `UIR-040`
  - `UIR-041`
  - `UIR-042`

## Progress

- UI Rebuild tasks total: `14`
- Done: `14`
- In progress: `0`
- Remaining: `0`
- Completion: `100.0%`

