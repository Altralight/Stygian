# Self-Host Daily Flow Contract V0

This contract defines the baseline "can we actually use this every day?" loop.
It is intentionally strict and boring. If this fails, self-host confidence is fake.

## Scope

We validate three realistic editor projects:

- `dashboard_shell`
- `tabs_settings`
- `modal_flow`

Each fixture must round-trip through file persistence and keep deterministic C23 export.

## Required Loop (Per Fixture)

1. Seed a realistic scene in editor state.
2. Save to `editor/fixtures/self_host_daily_<name>.project.json`.
3. Export C23.
4. Reload from saved file.
5. Re-export C23 and confirm exact match.
6. Apply a tiny edit (single-node 1px move).
7. Confirm diff size stays local (bounded changed-line budget).
8. Save edited project to build output.
9. Reload edited file and confirm normalized project JSON matches.

## Required Guarantees

- Save/load must not lose node identity or behavior graph.
- Export must stay deterministic across reload.
- Tiny edits must produce tiny textual diffs.
- Generated code must still expose runtime scene build symbol.
- Fixture complexity is non-trivial:
  - dashboard: frame hierarchy + behavior + timeline track
  - tabs: multi-rule visible-state switching
  - modal: layered show/hide plus animated entry

## Verification

- Build:
  - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_self_host_daily_flow_smoke.bat`
- Run:
  - `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_self_host_daily_flow_smoke.exe`
- Gate inclusion:
  - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_pre_selfhost_checkpoint.bat`

