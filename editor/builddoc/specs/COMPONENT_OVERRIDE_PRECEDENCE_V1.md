# Component Override Precedence v1

This spec defines precedence for component-set inheritance, variant selection,
and instance overrides.

## Scope

- authoring mutation APIs
- sidecar reload validation
- export diagnostics

## Resolution Order

For one component instance, resolve in this order:

1. Base definition: linked `component_def_id`.
2. Swap override: if `COMPONENT_OVERRIDE_SWAP` is set, use swap target only when
   it is legal for the same component set and schema.
3. Variant resolution: run deterministic variant matching using top-level
   property overrides (scoped subtree overrides are excluded from variant
   matching).
4. Field overrides: apply explicit instance mask overrides for visibility and
   geometry after resolved definition values.
5. Scoped subtree overrides: keep and validate scoped override paths against
   descendants of the resolved component definition tree.

## Conflict Semantics

- Illegal swap target: reject mutation and surface diagnostics.
- Invalid top-level property override: reject mutation.
- Invalid scoped subtree override path/value: reject mutation.
- Variant ties: resolve with deterministic ranking policy from
  `COMPONENT_VARIANT_RESOLUTION_V1.md`.

## Reload Contract

Sidecar load must reject component instances whose swap or override state cannot
be validated against these rules.

## Export Contract

Export diagnostics must report actionable errors/warnings for swap and override
conflicts rather than silently drifting component state.
