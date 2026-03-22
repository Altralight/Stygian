# Component Swap Constraints v1

This spec defines legal swap targets for component instances.

## Legal Swap Rules

A swap from definition `A` to definition `B` is legal only when:

1. `A` and `B` are component definition nodes.
2. `A == B` is always legal.
3. Otherwise, both definitions belong to the same non-empty variant group.
4. Property contracts are compatible:
   - same property set
   - matching property types
   - for enum properties, same option list in the same order

## Rationale

This blocks cross-family swaps that look compatible by accident but break
variant/override expectations at runtime.

## Behavior Requirement

- Illegal swap must fail mutation.
- Diagnostics must report swap failures with a component-set/schema message.
- Legal swaps must preserve existing valid overrides and continue through the
  variant resolver.
