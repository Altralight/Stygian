# Stygian Editor Tasklist V2

This is the compressed execution plan.
We are not replacing `TASKLIST.md`; this file is the practical ship list.

## Status Markers

- `[ ]` not started
- `[-]` in progress
- `[x]` finished

## Ground Rules

- Runtime output is generated Stygian C23.
- Authoring intent lives in the project sidecar.
- User logic lives in hook files and must never be overwritten.
- Every finished task must include verification evidence in:
  - `D:\Projects\Code\Stygian\editor\builddoc\verification\VERIFICATION_LOG.md`

## Must Ship

- [x] V2-001 Ownership Model In UI (Generated/User/Binding)
  - why: users need zero confusion about what is safe to edit.
  - done when:
    - generated assets are visibly marked read-only in editor surfaces
    - hook-owned surfaces are visibly marked user-owned
    - binding status is shown per interactive node
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_release_gate.bat`

- [x] V2-002 Export Regeneration Report
  - why: trust dies when export side effects are hidden.
  - done when:
    - export shows overwritten generated files
    - export shows preserved user files
    - export reports missing/stale hooks and fallback diagnostics
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_pre_selfhost_checkpoint.bat`

- [x] V2-003 Hook Stub Generation And Protection
  - why: custom logic needs a reliable and safe insertion point.
  - done when:
    - connecting a custom event action generates missing stub once
    - existing stubs are never overwritten
    - missing required hooks produce clear diagnostics
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_pre_selfhost_checkpoint.bat`

- [x] V2-004 Deterministic Node Naming Contract
  - why: symbol churn breaks merges and destroys confidence.
  - done when:
    - stable node IDs remain machine identity
    - deterministic exported symbol names are generated from stable naming rules
    - rename behavior does not break bound logic unexpectedly
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_export_determinism_smoke.bat`

- [x] V2-005 Binding Integrity Doctor
  - why: teams need one command to find broken graphs fast.
  - done when:
    - command reports missing handlers, stale node refs, unsupported feature fallbacks
    - output maps back to node ID + display path
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_pre_selfhost_checkpoint.bat`

- [x] V2-006 Declarative Interaction First-Class Set
  - why: common interactions should not require writing C.
  - done when:
    - built-in actions cover show/hide, state switch, property animate, tab switch, expand/collapse
    - these actions are editable in logic UI and export deterministically
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_phase7_behavior_bridge_smoke.bat`

- [x] V2-007 Tabs As Visual Component + Optional Data Path
  - why: tabs are core UX and should be designer-friendly.
  - done when:
    - designer can create tabs visually and wire state transitions without code
    - optional code/data path exists for dynamic tab counts
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_pre_selfhost_checkpoint.bat`

- [x] V2-008 Animation V1 (State Transitions + Curves)
  - why: translation/rotation/scale/opacity animation is baseline capability.
  - done when:
    - timeline data exists for transition clips with easing curves
    - state-driven transitions export to deterministic runtime code
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_phase7_behavior_bridge_smoke.bat`

- [x] V2-009 Schema Evolution And Migration Policy
  - why: sidecar upgrades need predictable behavior across versions.
  - done when:
    - migration rules are versioned and documented
    - loader reports incompatible schema with clear action
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_project_hostile_load_smoke.bat`

## Should Ship

- [x] V2-010 Logic Panel Contract (Per Node)
  - why: users need one place to inspect and wire interactivity.
  - done when:
    - panel shows supported events, connected handler, existence state, source ownership
    - stale/missing bindings are visible without reading code
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_host_interop_contract_smoke.bat`

- [x] V2-011 Code Insight View (Opt-In)
  - why: side-by-side generated code builds trust.
  - done when:
    - selected node can reveal exported code region and hook linkage
    - view is opt-in and does not block normal design flow
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_export_c23_golden_smoke.bat`

- [x] V2-012 Templates For Onboarding
  - why: examples teach boundaries faster than docs.
  - done when:
    - starter templates exist for tabs, sidebar shell, modal flow, settings panel
    - each template includes visual, declarative, and hook examples
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_pre_selfhost_checkpoint.bat`

- [x] V2-013 Progressive Complexity Modes
  - why: beginners and systems users need different noise levels.
  - done when:
    - beginner mode reduces exposed low-level terms
    - systems mode shows explicit runtime/export contracts
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_pre_selfhost_checkpoint.bat`

- [x] V2-014 Dynamic Repeater/Data Source Primitive
  - why: data-driven duplication should not require manual scene cloning.
  - done when:
    - nodes can be repeated from bound data with deterministic IDs/naming policy
    - unsupported runtime cases fail loudly
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_generated_scene_runtime_smoke.bat`

## Later

- [x] V2-015 Full Timeline Editor (Multi-Track Authoring)
  - why: complex choreography is useful but not a day-one blocker.
  - done when:
    - keyframe tracks, curve editor, and layered clips are authorable in editor
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_pre_selfhost_checkpoint.bat`

- [x] V2-016 Advanced SDF Boolean/Merge Authoring UX
  - why: SDF is a core differentiator beyond typical UI tools.
  - done when:
    - smooth union/subtract/intersect workflows are interactive and export-safe
    - metaball-like merges are available with deterministic fallback diagnostics
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_boolean_solver_parity_smoke.bat`

- [x] V2-017 Snap And Smart Guide Upgrade
  - why: speed and precision editing need better guidance than plain grid snap.
  - done when:
    - smart alignment guides appear during move/resize
    - snapping hierarchy is configurable and predictable
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_guides_snap_smoke.bat`

- [x] V2-018 Performance Transparency Surface
  - why: strong runtime performance is a product promise.
  - done when:
    - editor exposes performance budgets and regression deltas for generated output
  - verification command:
    - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_performance_budget_smoke.bat`

## Execution Focus

- Suggested immediate start order:
  - `V2-001` -> `V2-003` -> `V2-006` -> `V2-007` -> `V2-008`
- Why this order:
  - it locks trust boundaries first, then interaction power, then animation depth.

## Batch Status

- Batch 1 (Trust Boundary): done
- Batch 2 (Interaction Core): done
- Batch 3 (Motion Core): done
- Batch 4 (UX Depth): done
- Batch 5 (Data + Advanced Engine): done
- Next active batch: UI Rebuild (`TASKLIST_UI_REBUILD.md`)
- Progress: `18 / 18` items done (`100.0%`)
