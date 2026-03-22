# Animation Transitions V1

## Goal

Add transition-clip capable animation authoring for state-driven UI changes with
deterministic easing/duration semantics that export to stable runtime code.

## Delivered

- Transition clip data is now serialized in project sidecar:
  - top-level `transition_clips` array
  - records include trigger event, target node, property, from/to, duration, easing
- Transition clips are derived from animate behavior rules during project export,
  so timeline/state transition data exists without introducing a second runtime
  behavior execution model.
- Added transition/timeline runtime smoke:
  - `editor/tests/animation_transition_timeline_smoke.c`
  - target: `stygian_editor_animation_transition_timeline_smoke`
  - validates hover enter/leave state transitions with easing over time.
- Integrated transition smoke target into:
  - `editor_hardening_core`
  - `editor_release_gate`
  - pre-selfhost checkpoint runtime sequence

## Scope Notes

- V1 timeline data model is clip-based and event-driven; it does not yet include
  multi-track freeform keyframe authoring.
- Runtime export remains deterministic because behavior dispatch and codegen are
  unchanged in structure and still table-driven.
