# Self-Host Surface Polish V0

This pass is about production authoring speed, not feature count.
Core engine capability is already broad; the remaining risk is workflow drag.

## Top Friction Points

1. High-value actions are buried in menus.
   - Users should not dig through nested controls for actions they run every few minutes.
2. Dogfood seed workflows are available but slow to trigger.
   - Stage seeding is central to self-host iteration and should be one key chord away.
3. Authoring loop commands are split across file menu and command palette with weak shortcut surfacing.
   - The top strip should explicitly reinforce the shortcut path.

## Initial Fixes (This Pass)

- Add direct shortcut actions:
  - `Ctrl+N` -> reset canvas
  - `Ctrl+1/2/3` -> seed self-host stage 1/2/3
- Add command palette entries for the same actions so keyboard and click paths stay aligned.
- Keep existing full menu actions; this pass only removes friction, not capabilities.

## Acceptance

- Bootstrap compiles and launches.
- New shortcuts execute deterministic existing actions (no new data model branch).
- Command palette contains matching explicit actions for those shortcuts.

## Verification

- `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_bootstrap.bat`
- launch sanity check:
  - `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_bootstrap.exe`

