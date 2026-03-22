# Boolean Solver Parity V0

## Goal

Replace legacy boolean host-bounds fallback with deterministic geometry solving
for editor booleans so non-destructive booleans and flatten output are stable
across mutate/save/load/export.

## Scope

- Boolean node stores solved polygon points (`boolean_solved_points`).
- Solve pass runs on:
  - boolean compose
  - project load link validation
  - mutation finalize refresh
- Flatten prefers solved polygon points and only falls back to resolved bounds
  when no surface exists.
- Export diagnostics no longer emit `boolean_export_fallback`; partial warning is
  only raised when no polygon surface is available.

## Solver Model (V0)

- Operand geometry uses world-space bounds for referenced operand nodes.
- A deterministic orthogonal-grid solve is built from unique operand bound
  coordinates.
- Cell occupancy is evaluated by boolean op:
  - union: any operand contains cell center
  - intersect: all operands contain cell center
  - subtract: operand0 minus union(operand1..n)
  - exclude: xor parity across operands
- If multiple disconnected components exist, V0 keeps the largest-area component.
- Boundary extraction produces a closed orthogonal polygon.
- Collinear simplification runs before point-cap validation.
- If polygon cannot be emitted within the current point cap, V0 falls back to
  resolved component bounds.

## Constraints

- V0 is deterministic and stable, but intentionally not full multi-component /
  hole-preserving boolean topology.
- This is an explicit quality step for flatten/runtime parity, not the final
  geometric boolean engine.

## Verification

- `editor/tests/boolean_solver_parity_smoke.c`
- Build target: `stygian_editor_boolean_solver_parity_smoke`
- Included in:
  - `editor_hardening_core`
  - `editor_release_gate`
