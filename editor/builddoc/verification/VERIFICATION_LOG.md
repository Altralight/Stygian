# Stygian Editor Verification Log

This file records concrete checks we actually ran.
Each entry names the task, the commands, the artifacts, and the result.

## Entry Format

### VYYYY-MM-DD-NN

- Task IDs: `...`
- Summary: ...
- Type: `doc review` | `runtime check` | `smoke test` | `export diff` | `manual check`
- Commands:
  - `...`
- Artifacts:
  - `...`
- Expected:
  - ...
- Actual:
  - ...
- Result: `PASS` | `FAIL`
- Notes:
  - ...

## Entries

### V2026-03-12-86

- Task IDs: `T962`
- Summary: Added shortcut-first workflow actions and matching command-palette entries for reset and dogfood-stage seed flows.
- Type: `runtime check`
- Commands:
  - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_bootstrap.bat`
  - liveness launch check for `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_bootstrap.exe`
- Artifacts:
  - `D:\Projects\Code\Stygian\editor\apps\stygian_editor_bootstrap.c`
  - `D:\Projects\Code\Stygian\editor\builddoc\specs\SELF_HOST_SURFACE_POLISH_V0.md`
  - `D:\Projects\Code\Stygian\editor\builddoc\plans\TASKLIST.md`
- Expected:
  - reset and stage-seed actions are accessible via direct shortcuts
  - command palette mirrors shortcut actions for discoverability
  - bootstrap remains build-clean and launch-stable
- Actual:
  - bootstrap build returned success
  - process-liveness launch check passed (`exited=False` after 2s)
  - shortcut help strip advertises the new key chords
- Result: `PASS`
- Notes:
  - introduced `app_reset_canvas(...)` to stop duplicating reset logic across menu, shortcuts, and command palette

### V2026-03-12-87

- Task IDs: `T963`
- Summary: Added a top-strip self-host quick-action group for `Tabs`, `Sidebar`, `Modal`, and `Settings` templates.
- Type: `runtime check`
- Commands:
  - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_bootstrap.bat`
  - liveness launch check for `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_bootstrap.exe`
- Artifacts:
  - `D:\Projects\Code\Stygian\editor\apps\stygian_editor_bootstrap.c`
  - `D:\Projects\Code\Stygian\editor\builddoc\plans\TASKLIST.md`
- Expected:
  - template actions are available as direct top-strip buttons
  - each action reports deterministic status feedback
  - bootstrap remains build-clean and launch-stable
- Actual:
  - bootstrap build returned success
  - process-liveness launch check passed (`exited=False` after 2s)
  - top strip exposes `Tabs`, `Sidebar`, `Modal`, and `Settings`
- Result: `PASS`
- Notes:
  - this intentionally duplicates file-menu actions to reduce click depth for common self-host flows

### V2026-03-12-88

- Task IDs: `T964`
- Summary: Hardened top-chrome behavior for narrow widths with explicit collapse rules for quick actions and ownership badges.
- Type: `runtime check`
- Commands:
  - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_bootstrap.bat`
  - liveness launch check for `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_bootstrap.exe`
- Artifacts:
  - `D:\Projects\Code\Stygian\editor\apps\stygian_editor_bootstrap.c`
  - `D:\Projects\Code\Stygian\editor\builddoc\plans\TASKLIST.md`
- Expected:
  - narrow widths avoid top-strip overlap by collapsing non-critical controls
  - critical command controls remain visible and usable
  - bootstrap remains build-clean and launch-stable
- Actual:
  - bootstrap build returned success
  - process-liveness launch check passed (`exited=False` after 2s)
  - width-based quick-action and badge collapse thresholds are active
- Result: `PASS`
- Notes:
  - this was geometry hardening, not final visual polish

### V2026-03-12-89

- Task IDs: `T964`
- Summary: Ran screenshot-based top-chrome visual checks at `1280`, `1024`, and `800` widths.
- Type: `manual check`
- Commands:
  - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_bootstrap.bat`
  - PowerShell window capture workflow
- Artifacts:
  - `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\topchrome-before-1280.png`
  - `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\topchrome-before-1024.png`
  - `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\topchrome-before-800.png`
  - `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\topchrome-after-1280.png`
  - `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\topchrome-after-1024.png`
  - `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\topchrome-after-800.png`
  - `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\topchrome_visual_check_report.md`
- Expected:
  - no top-chrome clipping or overlap regressions at representative widths
- Actual:
  - all three widths passed the visual sweep
  - observed diffs were transient hover/focus/runtime-text changes, not layout breakage
- Result: `PASS`
- Notes:
  - window capture used a direct PowerShell path instead of the broken `shotter` script

### V2026-03-12-90

- Task IDs: `T970`, `T971`, `T972`, `T973`, `T975`
- Summary: Landed the creative-tool shell redesign pass for the bootstrap editor.
- Type: `runtime check`
- Commands:
  - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_bootstrap.bat`
  - liveness launch check for `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_bootstrap.exe`
  - capture workflow to `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\editor-window-20260312-144054.png`
  - capture workflow to `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\editor-window-seeded-20260312-144152.png`
  - shortcut smoke via `Ctrl+S`
- Artifacts:
  - `D:\Projects\Code\Stygian\editor\apps\stygian_editor_bootstrap.c`
  - `D:\Projects\Code\Stygian\widgets\stygian_widgets.c`
  - `D:\Projects\Code\Stygian\widgets\stygian_widgets.h`
  - `D:\Projects\Code\Stygian\layout\stygian_dock_impl.c`
  - `D:\Projects\Code\Stygian\editor\builddoc\specs\CREATIVE_TOOL_SHELL_REDESIGN_V0.md`
  - `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\editor-window-20260312-144054.png`
  - `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\editor-window-seeded-20260312-144152.png`
  - `D:\Projects\Code\Stygian\editor\build\stygian_editor_session.project.json`
- Expected:
  - bootstrap builds and stays alive after launch
  - shell looks like a competent editor instead of a debug harness
  - code view reads generated and hook source in-app
  - timeline reads as motion tooling at a glance
  - project save path still works after shell changes
- Actual:
  - bootstrap build returned success with no warnings
  - process-liveness launch check passed (`alive=True` after 2s)
  - captures show the rebuilt shell, upgraded assets rail, and timeline transport strip
  - live `Ctrl+S` save smoke produced `D:\Projects\Code\Stygian\editor\build\stygian_editor_session.project.json`
- Result: `PASS`
- Notes:
  - direct pointer automation for scaffold-button hit targets was inconclusive at this stage, so `T974` stayed open

### V2026-03-12-91

- Task IDs: `T974`
- Summary: Finished the redesigned-shell control reliability pass.
- Type: `runtime check`
- Commands:
  - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_bootstrap.bat`
  - hold-click capture workflow to `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\file-menu-hold-20260312.png`
  - hold-click sweep to `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\tabs-sweep-x360.png`
  - hold-click sweep to `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\tabs-sweep-x390.png`
  - hold-click sweep to `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\tabs-sweep-x420.png`
- Artifacts:
  - `D:\Projects\Code\Stygian\editor\apps\stygian_editor_bootstrap.c`
  - `D:\Projects\Code\Stygian\layout\stygian_dock_impl.c`
  - `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\file-menu-hold-20260312.png`
  - `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\tabs-sweep-x390.png`
- Expected:
  - top-chrome menus render above dock panels instead of being buried behind them
  - touched panels stop bleeding text into neighboring docks
  - at least one direct scaffold action visibly creates scene content through pointer input
- Actual:
  - bootstrap build returned success
  - `FILE` menu appears above the dock stack and stays readable in capture
  - dock clipping keeps code-panel content inside the right rail
  - top-strip `Tabs` action visibly created `nodes:7` with scene/layer entries and viewport content in the verified capture
- Result: `PASS`
- Notes:
  - synthetic input needed a held press instead of a blink-click because this widget stack claims `active_id` on the frame between mouse-down and mouse-up
  - this file was rebuilt after an encoding failure during the shell pass; these entries retain the latest validated shell-phase checks

### V2026-03-12-92

- Task IDs: `IR-001`, `IR-002`, `IR-500`, `IR-501`
- Summary: Locked the reset direction and rulebook, then cleaned up the active panels and visual baseline.
- Type: `build + runtime + screenshot baseline`
- Commands:
  - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_bootstrap.bat`
  - liveness launch check for `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_bootstrap.exe`
  - window capture workflow to `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\reset-batch5-window-1440x900.png`
  - window capture workflow to `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\reset-batch5-window-1280x820.png`
  - window capture workflow to `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\reset-batch5-window-1024x768.png`
- Artifacts:
  - `D:\Projects\Code\Stygian\editor\apps\stygian_editor_bootstrap.c`
  - `D:\Projects\Code\Stygian\widgets\stygian_widgets.c`
  - `D:\Projects\Code\Stygian\editor\builddoc\plans\TASKLIST_INTERACTION_RESET.md`
  - `D:\Projects\Code\Stygian\editor\builddoc\specs\EDITOR_CAPABILITY_RESEARCH_MATRIX_V0.md`
  - `D:\Projects\Code\Stygian\editor\builddoc\specs\INTERACTION_RESET_REBUILD_V0.md`
  - `D:\Projects\Code\Stygian\editor\builddoc\references\figma-phase1\index.html`
  - `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\reset-batch5-window-1440x900.png`
  - `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\reset-batch5-window-1280x820.png`
  - `D:\Projects\Code\Stygian\editor\build\windows\ui_checks\reset-batch5-window-1024x768.png`
- Expected:
  - direction lock and interaction rulebook reflect the current reset instead of stale guesses
  - Layers, Insert, Inspector, Timeline, and Code read as narrower-purpose panels instead of filler bins
  - shared panel chrome and buttons read as one intentional tool shell
  - representative screenshots exist for the current shell baseline
- Actual:
  - bootstrap build returned success
  - process-liveness launch check passed (`alive=True` after 2s)
  - direction lock and frozen-rule docs now describe the actual current tool, menu, viewport, snap, and timeline model
  - panel pass landed across Layers, Insert, Inspector, Diagnostics, Code, and shared panel chrome
  - window baseline captures were written for 1440, 1280, and 1024 width classes
- Result: `PASS`
- Notes:
  - the first screenshot attempt grabbed the wrong top-level window; the final capture pass targets the largest visible window owned by the process instead of trusting the first handle Windows hands out

### V2026-03-13-97

- Task IDs: `NS-300`, `NS-301`, `NS-302`
- Summary: Closed the export-ownership and code-insight batch with clearer generated ownership markers, line-aware hook discovery, and diff-quality proof.
- Type: `build + runtime + export smoke`
- Commands:
  - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_bootstrap.bat`
  - `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_generated_file_layout_smoke.exe`
  - `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_export_diff_quality_smoke.exe`
  - `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_phase7_behavior_bridge_smoke.exe`
  - bootstrap 2s liveness check for `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_bootstrap.exe`
- Artifacts:
  - `D:\Projects\Code\Stygian\editor\apps\stygian_editor_bootstrap.c`
  - `D:\Projects\Code\Stygian\editor\src\stygian_editor.c`
  - `D:\Projects\Code\Stygian\editor\tests\generated_file_layout_smoke.c`
  - `D:\Projects\Code\Stygian\editor\builddoc\plans\PHASE_NEXT_SYSTEMS_TASKLIST.md`
  - `D:\Projects\Code\Stygian\editor\builddoc\references\figma-phase1\index.html`
- Expected:
  - generated files state their ownership and point users toward hook files instead of pretending they are safe to edit
  - code insight names generated, hook, and authored zones clearly for the selected node
  - hook discovery shows missing or present hook state with source-file location context
  - tiny export edits stay local and rename-only edits avoid unrelated churn
- Actual:
  - bootstrap build returned success and liveness check passed (`alive=True` after 2s)
  - generated file layout smoke passed with the new generated-zone and hook-zone markers in exported source
  - phase7 behavior bridge smoke passed, confirming hook-backed behavior export still works
  - export diff quality smoke passed, confirming local textual diffs and stable symbol footprint on rename-only edits
- Result: `PASS`
- Notes:
  - code insight now reads hook line numbers from the current hook source file instead of vaguely saying hooks exist somewhere

### V2026-03-13-98

- Task IDs: `NS-400`, `NS-401`, `NS-402`
- Summary: Closed the component, variant, and runtime-facing property batch across editor surface, export metadata, and verification.
- Type: `build + runtime + export smoke`
- Commands:
  - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_bootstrap.bat`
  - `powershell -NoProfile -ExecutionPolicy Bypass -File D:\Projects\Code\Stygian\compile\windows\build.ps1 -Target stygian_editor_component_linkage_smoke -OutputDir editor\build\windows`
  - `powershell -NoProfile -ExecutionPolicy Bypass -File D:\Projects\Code\Stygian\compile\windows\build.ps1 -Target stygian_editor_phase6_components_styles_variables_smoke -OutputDir editor\build\windows`
  - `powershell -NoProfile -ExecutionPolicy Bypass -File D:\Projects\Code\Stygian\compile\windows\build.ps1 -Target stygian_editor_phase75_variant_hardening_smoke -OutputDir editor\build\windows`
  - `powershell -NoProfile -ExecutionPolicy Bypass -File D:\Projects\Code\Stygian\compile\windows\build.ps1 -Target stygian_editor_component_property_export_smoke -OutputDir editor\build\windows`
  - `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_component_linkage_smoke.exe`
  - `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_phase6_components_styles_variables_smoke.exe`
  - `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_phase75_variant_hardening_smoke.exe`
  - `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_component_property_export_smoke.exe`
  - bootstrap 2s liveness check for `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_bootstrap.exe`
- Artifacts:
  - `D:\Projects\Code\Stygian\editor\apps\stygian_editor_bootstrap.c`
  - `D:\Projects\Code\Stygian\editor\include\stygian_editor.h`
  - `D:\Projects\Code\Stygian\editor\include\stygian_editor_module.h`
  - `D:\Projects\Code\Stygian\editor\src\stygian_editor.c`
  - `D:\Projects\Code\Stygian\editor\src\stygian_editor_module.c`
  - `D:\Projects\Code\Stygian\editor\tests\component_property_export_smoke.c`
  - `D:\Projects\Code\Stygian\editor\builddoc\plans\PHASE_NEXT_SYSTEMS_TASKLIST.md`
  - `D:\Projects\Code\Stygian\editor\builddoc\references\figma-phase1\index.html`
- Expected:
  - the editor can create component defs and instances without hiding the feature in tests only
  - variant families and instance overrides are visible and editable from the surface
  - generated C23 carries readable component symbol, variant, property, and override mapping
- Actual:
  - bootstrap build returned success and liveness check passed (`alive=True` after 2s)
  - component linkage smoke passed
  - phase6 components/styles/variables smoke passed
  - phase75 variant hardening smoke passed
  - new component property export smoke passed, confirming component metadata tables and mapping comments in generated C23
- Result: `PASS`
- Notes:
  - module ABI minor moved to `27` to cover the new public component symbol get/set surface instead of leaving the bootstrap to cheat with internal state

### V2026-03-13-99

- Task IDs: `NS-500`, `NS-501`
- Summary: Closed the shader and effects batch with real effect-stack editing, visible mask ownership, and a narrow shader attachment API that survives roundtrip and exports honest ownership markers.
- Type: `build + runtime + roundtrip + export smoke`
- Commands:
  - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_bootstrap.bat`
  - `powershell -NoProfile -ExecutionPolicy Bypass -File D:\Projects\Code\Stygian\compile\windows\build.ps1 -Target stygian_editor_shader_effect_batch_e_smoke -OutputDir editor\build\windows`
  - `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_shader_effect_batch_e_smoke.exe`
  - bootstrap 2s liveness check for `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_bootstrap.exe`
- Artifacts:
  - `D:\Projects\Code\Stygian\editor\apps\stygian_editor_bootstrap.c`
  - `D:\Projects\Code\Stygian\editor\include\stygian_editor.h`
  - `D:\Projects\Code\Stygian\editor\include\stygian_editor_module.h`
  - `D:\Projects\Code\Stygian\editor\src\stygian_editor.c`
  - `D:\Projects\Code\Stygian\editor\src\stygian_editor_module.c`
  - `D:\Projects\Code\Stygian\editor\src\stygian_editor_project_io.inl`
  - `D:\Projects\Code\Stygian\editor\tests\shader_effect_batch_e_smoke.c`
  - `D:\Projects\Code\Stygian\editor\builddoc\plans\PHASE_NEXT_SYSTEMS_TASKLIST.md`
  - `D:\Projects\Code\Stygian\editor\builddoc\references\figma-phase1\index.html`
- Expected:
  - multi-effect stacks are editable and reorderable from the editor surface
  - mask ownership is readable and editable in the same panel instead of being hidden metadata
  - shader attachment state exists in the real editor API, survives save/load, and exports with explicit ownership language and diagnostics
- Actual:
  - bootstrap build returned success and liveness check passed (`alive=True` after 2s)
  - new shader/effect Batch E smoke passed, confirming effect stack persistence, mask roundtrip, shader attachment roundtrip, export diagnostics, and generated ownership markers
  - module ABI minor moved to `28` so shader attachment access stays public instead of bootstrap-only
- Result: `PASS`
- Notes:
  - shader attachment export is intentionally honest: the editor owns authored slot metadata, while runtime binding still stays host-owned until a deeper renderer-facing attachment lane is built

### V2026-03-13-100

- Task IDs: `NS-600`, `NS-601`
- Summary: Closed the integration batch with fixture-grade next-systems scenes and a dedicated next-systems checkpoint gate.
- Type: `fixture sweep + checkpoint gate`
- Commands:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File D:\Projects\Code\Stygian\compile\windows\build.ps1 -Target stygian_editor_next_systems_fixture_smoke -OutputDir editor\build\windows`
  - `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_next_systems_checkpoint.bat`
- Artifacts:
  - `D:\Projects\Code\Stygian\editor\tests\next_systems_fixture_smoke.c`
  - `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_next_systems_checkpoint.bat`
  - `D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_next_systems_checkpoint.bat`
  - `D:\Projects\Code\Stygian\compile\targets.json`
  - `D:\Projects\Code\Stygian\editor\fixtures\next_systems_motion_layout_driver.project.json`
  - `D:\Projects\Code\Stygian\editor\fixtures\next_systems_component_states.project.json`
  - `D:\Projects\Code\Stygian\editor\fixtures\next_systems_ownership_effects.project.json`
  - `D:\Projects\Code\Stygian\editor\fixtures\README.md`
  - `D:\Projects\Code\Stygian\editor\builddoc\plans\PHASE_NEXT_SYSTEMS_TASKLIST.md`
  - `D:\Projects\Code\Stygian\editor\builddoc\references\figma-phase1\index.html`
- Expected:
  - representative fixtures carry procedural motion, layout, components, and ownership systems
  - fixtures survive save/load/export/reopen sweeps
  - one dedicated checkpoint command proves the next-systems lane without the full release wall
- Actual:
  - new next-systems fixture smoke passed
  - dedicated checkpoint build/run command passed cleanly
  - fixture outputs were written under `editor/fixtures` for motion/layout/driver, components, and ownership/effects/shader coverage
- Result: `PASS`
- Notes:
  - the first draft of the fixture smoke was too strict about exact JSON/C23 identity on scenes that exercise reactive and component lanes; the final gate keeps deterministic export checks in their dedicated smokes and uses fixture signatures plus save/load/export/reopen proof for this integration lane
