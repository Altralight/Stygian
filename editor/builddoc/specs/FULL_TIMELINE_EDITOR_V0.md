# Full Timeline Editor V0

## Goal

Move timeline authoring from clip-only transition metadata to explicit multi-track
keyframe data that is first-class in project state and editable in bootstrap UI.

## Delivered

- Core timeline model in editor state:
  - track IDs and clip IDs
  - track-level property binding (`target_node`, `property`, `layer`, `name`)
  - keyframe arrays with timestamp sorting and easing
  - clip-level track references with `start`, `duration`, `layer`
- Timeline core API surface:
  - add/remove/get track
  - set track keyframes
  - add/remove/get clip
  - set clip tracks
- Sidecar persistence for timeline data:
  - `timeline_tracks`
  - `timeline_clips`
  - timeline next-id fields in `next_ids`
- Link validation and cleanup behavior:
  - track target node must exist
  - clip track references must resolve
  - node deletion prunes bound timeline tracks and stale clip references
- Bootstrap authoring UI (properties panel timeline section):
  - track creation/deletion
  - target/property selection
  - keyframe add/update/delete
  - easing selection per keyframe
  - curve view (track keyframe polyline)
  - clip creation/deletion
  - clip layer/start/duration authoring
  - clip track assignment/unassignment

## Schema / Migration

- Project schema minor advanced to `2`.
- Hostile-load smoke updated to perform schema-minor mutation by key rewrite,
  avoiding brittle hardcoded string assumptions.

## Verification

- `stygian_editor_timeline_multitrack_smoke`: PASS
- `stygian_editor_animation_transition_timeline_smoke`: PASS
- `stygian_editor_project_roundtrip_smoke`: PASS
- `stygian_editor_project_hostile_load_smoke`: PASS
- `stygian_editor_bootstrap` build: SUCCESS
- `run_stygian_editor_pre_selfhost_checkpoint.bat`: PASS

## Known Limits

- Curve editing is preset/easing-driven with keyframe value/time controls,
  not a full bezier tangent manipulation system yet.
- Layered clip runtime blend/export semantics are deterministic but still v0 in UX,
  with deeper polish scheduled in UI rebuild lane.
