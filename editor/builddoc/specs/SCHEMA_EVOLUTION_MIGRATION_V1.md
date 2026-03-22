# Schema Evolution + Migration V1

## Goal

Define and enforce explicit sidecar schema migration policy for backward
compatibility and forward incompatibility handling.

## Delivered

- Project schema minor advanced to `1`.
- Loader policy now enforces:
  - backward-compatible load for older minor versions
  - hard failure for newer unsupported minor versions with actionable message
- Added migration telemetry log on older-minor load path.
- Added transition-clip parsing path for schema v1 sidecar payloads.
- Updated hostile-load coverage to verify:
  - older minor (`0`) is accepted
  - newer minor (`2`) is rejected

## Policy

- `schema_major` mismatch: reject
- `schema_minor` newer than loader: reject and require editor upgrade
- `schema_minor` older than loader: accept and migrate with defaults

## Verification

- `editor/tests/project_hostile_load_smoke.c`
- `editor/tests/project_roundtrip_smoke.c`
