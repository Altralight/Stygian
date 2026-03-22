# Variable Binding Parity V0

## Goal

Make generated C23 honor editor variable bindings for paint/layout/interaction so
runtime behavior can apply variable modes and variable assignments directly.

## Delivered

- Generated output now includes node-variable binding records.
- Added generated runtime helpers:
  - `stygian_editor_generated_apply_variable_assignment(...)`
  - `stygian_editor_generated_apply_variable_mode(...)`
  - `stygian_editor_generated_set_active_variable_mode(...)`
  - `stygian_editor_generated_get_active_variable_mode(...)`
- Build scene now applies the authored active variable mode to exported elements.
- Behavior dispatch `set_variable` action now applies assignment to bound runtime
  elements before invoking host hook callback.

## Scope Notes

- V0 uses existing editor binding model (one color variable + one number variable
  binding per node).
- Host hooks remain available; runtime parity is additive, not a replacement.

## Verification

- `editor/tests/variable_binding_parity_smoke.c`
- target: `stygian_editor_variable_binding_parity_smoke`
