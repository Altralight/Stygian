# Component Variant Resolution v1

This spec defines deterministic variant selection for component instances.

## Scope

- API: `stygian_editor_component_resolve_variant(...)`
- Inputs: seed component definition + requested component properties.
- Output: chosen compatible component definition ID.

## Normalization Rules

1. Ignore scoped override keys during resolution (`name` containing `.`).
2. For duplicate top-level property keys, the last value wins.
3. Keep only top-level keys in the normalized request set.

## Candidate Set

A candidate is considered only when:

- it is a component definition node
- it is compatible with the seed definition according to
  `stygian_editor_component_defs_compatible(...)`

## Per-Candidate Counters

For each normalized request key/value:

- `match`: candidate has the property and requested value equals that candidate
  property default (variant value)
- `mismatch`: candidate has the property but requested value differs from default
  or is invalid for that property schema
- `missing`: candidate does not define that property key

`exact` is true only when request count > 0 and `mismatch == 0` and `missing == 0`.

## Deterministic Ranking (Highest Priority First)

1. Higher `match` count
2. Lower `mismatch` count
3. Lower `missing` count
4. `exact=true` over `exact=false`
5. Lower component definition node ID (stable tiebreak)

This ordering guarantees stable resolution for full, partial, and under-specified
property sets.

## Notes

- This intentionally resolves against variant defaults instead of instance
  mutable state.
- Scoped subtree overrides are stored and round-tripped separately; they do not
  participate in variant selection.
