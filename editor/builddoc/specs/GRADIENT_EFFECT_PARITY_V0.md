# Gradient And Effect Parity V0

This is the implementation contract for `T951`.

Goal: gradient/effect authoring data must survive save/load/export and produce
runtime output that matches editor intent in deterministic ways.

## Problem Snapshot

Current gradient support stores basic parameters (`stops`, angle, radial
center/radius), but does not expose a full editable transform model.

Effects are stack-based and deterministic in order, but effect-space mapping
under transformed nodes needs explicit parity checks.

## Required Additions

1. Gradient transform carrier:
   - Add explicit transform fields for linear/radial gradient space.
   - Keep backward compatibility with existing angle/radial fields during
     migration.
2. API surface:
   - Add get/set APIs for gradient transform on node fills.
   - Reject invalid transform payloads with clear failure.
3. Persistence:
   - Sidecar save/load round-trips transform values without drift.
4. Export:
   - Generated C23 emits deterministic gradient transform representation.
5. Effect-space parity:
   - Document and enforce mapping rules for blur/shadow/glow under node
     translation/scale/rotation.

## Verification Plan

- New smoke fixture:
  - gradient transform round-trip with multiple node types
  - effect stack under transformed nodes (position, scale, rotation)
- Determinism:
  - export same fixture twice and compare stable output
- Runtime parity:
  - generated runtime smoke checks expected transformed gradient/effect values

## Non-Goals (V0)

- UI polish for gradient handles
- full visual editing UX for every effect type
- nonlinear color-space authoring controls
