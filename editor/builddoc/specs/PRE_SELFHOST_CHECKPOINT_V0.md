# Pre-Selfhost Checkpoint V0

## Goal

Define a strict, single-command checkpoint gate that must pass before increasing
self-host migration scope.

## Policy

- Required command:
  - `cmd /c editor\compile\windows\run_stygian_editor_pre_selfhost_checkpoint.bat`
- This command is the only accepted pass signal for pre-selfhost readiness.
- Partial runs do not count.
- Any non-zero exit from build or runtime checks is a hard fail.

## Gate Sequence

1. Build full `editor_release_gate` group into `editor/build/windows`.
2. Execute every runtime binary in the release gate list, including:
   - core unit suite and roundtrip lanes
   - export/runtime parity lanes
   - module ABI and host interop lanes
   - phase 6/7 and self-host shell/migration/escape-hatch lanes
3. Emit `[pre_selfhost_checkpoint] PASS` only when all commands return success.

## Pass Criteria

- `editor_release_gate` group build returns success.
- Every runtime executable in the checkpoint script returns `[PASS]` and zero exit.
- No command is skipped in the scripted sequence.

## Verification

- Script:
  - `editor/compile/windows/run_stygian_editor_pre_selfhost_checkpoint.bat`
