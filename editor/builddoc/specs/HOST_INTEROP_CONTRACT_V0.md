# Host Interop Contract V0

## Goal

Add a host-driven contract lane that validates editor module ABI negotiation and
lifecycle behavior from the host/plugin boundary, not just direct editor calls.

## Delivered

- Added host-side smoke target:
  - source: `editor/tests/host_interop_contract_smoke.c`
  - target: `stygian_editor_host_interop_contract_smoke`
- Contract coverage in the smoke:
  - ABI negotiation acceptance/rejection
    - exact ABI accepted
    - major mismatch rejected
    - newer minor rejected
  - Module API table sanity for host-required entry points
  - Lifecycle path through module table
    - `create` -> `set_host` -> `tick` -> graph mutation -> save -> reset -> load
    - `destroy` cleanup
  - Failure path validation
    - malformed JSON rejected through module load API
- Wired into quality gates:
  - `editor_hardening_core`
  - `editor_release_gate`

## Scope Notes

- V0 exercises host-contract behavior with static module linking and API table
  dispatch, which is enough to validate ABI and lifecycle semantics.
- V0 does not add OS dynamic-loader integration coverage yet.

## Verification

- `editor/tests/host_interop_contract_smoke.c`
- target: `stygian_editor_host_interop_contract_smoke`
