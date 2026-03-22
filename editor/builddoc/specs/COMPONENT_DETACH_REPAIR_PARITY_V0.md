# Component Detach + Repair Parity V0

## Goal

Add explicit component-instance detach/repair semantics so instance workflows can
temporarily break linkage and later reconnect without override-state corruption.

## Delivered

- New editor APIs:
  - `stygian_editor_component_instance_detach(...)`
  - `stygian_editor_component_instance_is_detached(...)`
  - `stygian_editor_component_instance_repair(...)`
- Detached instances are ignored by definition-apply propagation.
- Repair supports preferred definition target with optional override preservation.
- Detached state is persisted through project save/load (`idet` record field).
- Export diagnostics no longer treat detached instances as hard missing-definition
  errors.
- Module ABI updated to minor `24` with appended detach/repair API table entries.

## Scope Notes

- V0 preserves instance-node identity and spatial data while detached.
- V0 repair is deterministic and conservative: when `preserve_overrides=true`, it
  validates overrides against the selected target definition.

## Verification

- `editor/tests/component_detach_repair_parity_smoke.c`
- target: `stygian_editor_component_detach_repair_parity_smoke`
