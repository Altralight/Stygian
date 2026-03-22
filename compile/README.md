# Compile Runners

`targets.json` is the single source of truth for Stygian mini-app targets.

## Runners

- Unified dispatcher:
  - Windows: `compile/run.ps1`
  - Linux/macOS: `compile/run.sh`
- Windows: `compile/windows/build.ps1`
- Linux: `compile/linux/build.sh`
- macOS: `compile/macos/build.sh`

## First Build (Windows)

1. Build shader outputs:
   - `compile\windows\build_shaders.bat`
2. Build quick-start target (default wrapper):
   - `compile\windows\build.bat`
3. Run:
   - `build\quickwindow.exe`

Optional quick tests:

- Borderless: `compile\windows\build_quickwindow_borderless.bat`
- Custom titlebar: `compile\windows\build_quickwindow_custom_titlebar.bat`
- Custom titlebar (Vulkan):
  `compile\windows\build_quickwindow_custom_titlebar_vk.bat`
- Capture smoke (local capture workload):
  `compile\windows\build_quickwindow_capture_smoke.bat`
- Capture smoke Vulkan fallback path:
  `compile\windows\build_quickwindow_capture_smoke_vk.bat`
  (v1 records video via Vulkan readback; currently synchronous and lower-throughput)
- Editor bootstrap (OpenGL):
  `editor\compile\windows\build_stygian_editor_bootstrap.bat`
- Editor bootstrap (Vulkan):
  `editor\compile\windows\build_stygian_editor_bootstrap_vk.bat`
- All quick smoke targets: `compile\windows\build_quick_smoke.bat`

Borderless maximize on Windows is work-area maximize (taskbar remains visible),
not true fullscreen.

If you want fullscreen-style monitor coverage, that is a different mode and not
the default maximize behavior.

On Windows, OpenGL borderless main windows keep strict `WS_POPUP` semantics
with manual maximize/restore sizing in core Win32 code (`window/platform/stygian_win32.c`).

Vulkan borderless path remains unchanged in this iteration.

`quickwindow_custom_titlebar` uses an event-paced loop to avoid maximize hitch
behavior in unstable OpenGL vsync environments.

Custom-titlebar behavior is now exposed through core APIs:

- `stygian_window_get_titlebar_hints`
- `stygian_window_set_titlebar_behavior`
- `stygian_window_begin_system_move`
- `stygian_window_titlebar_double_click`
- `stygian_window_get_titlebar_menu_actions`
- `stygian_window_apply_titlebar_menu_action`
- `stygian_window_set_fullscreen` / `stygian_window_is_fullscreen`

Default titlebar double-click is maximize/restore. Optional fullscreen toggle
policy is configurable per window.

Win32 implements these behaviors; non-Windows backends currently expose
deterministic fallback stubs while native implementations are in progress.

## Backend Rules (Strict)

Backend setup must match across all layers:

1. Source backend (`StygianConfig.backend`)
2. Source window render flag (`STYGIAN_WINDOW_OPENGL` or `STYGIAN_WINDOW_VULKAN`)
3. Target backend in `compile/targets.json` (`"gl"` or `"vk"`)

For quickwindow:

- OpenGL: `powershell -File compile/run.ps1 -Target quickwindow`
- Vulkan: `powershell -File compile/run.ps1 -Target quickwindow_vk`

## Examples

- Quick-start target: `powershell -File compile/run.ps1 -Target quickwindow`
- Quick-start Vulkan target: `powershell -File compile/run.ps1 -Target quickwindow_vk`
- Custom titlebar OpenGL target:
  `powershell -File compile/run.ps1 -Target quickwindow_custom_titlebar`
- Custom titlebar Vulkan target:
  `powershell -File compile/run.ps1 -Target quickwindow_custom_titlebar_vk`
- Capture smoke target:
  `powershell -File compile/run.ps1 -Target quickwindow_capture_smoke -EnableCapture`
- Capture smoke Vulkan target:
  `powershell -File compile/run.ps1 -Target quickwindow_capture_smoke_vk -EnableCapture`
  (v1 records video via Vulkan readback; currently synchronous and lower-throughput)
- Editor bootstrap target:
  `powershell -File compile/run.ps1 -Target stygian_editor_bootstrap`
- Editor bootstrap Vulkan target:
  `powershell -File compile/run.ps1 -Target stygian_editor_bootstrap_vk`
- Capture MP4 path tags BT.709 primaries/matrix + sRGB/709 transfer metadata
  and uses BT.709 full-range RGB->NV12 conversion in local capture sources.
- Capture MF writer init now retries with hardware transforms on/off and null
  attributes for broader machine compatibility.
- Quick smoke group: `powershell -File compile/run.ps1 -Group quick_smoke`
- Editor hardening core group:
  `powershell -File compile/run.ps1 -Group editor_hardening_core`
- Editor quick checkpoint group:
  `powershell -File compile/run.ps1 -Group editor_quick_checkpoint`
- Editor release gate group:
  `powershell -File compile/run.ps1 -Group editor_release_gate`

Editor-local wrappers (outputs stay under `editor/build/windows/`):

- `editor\compile\windows\build_stygian_editor_hardening_core.bat`
- `editor\compile\windows\build_stygian_editor_quick_checkpoint.bat`
- `editor\compile\windows\build_stygian_editor_release_gate.bat`
- Unified Windows target: `powershell -File compile/run.ps1 -Target text_editor_mini`
- Unified Windows group: `powershell -File compile/run.ps1 -Group mini_apps_all`
- Linux/macOS target: `compile/run.sh --target text_editor_mini`
- Linux/macOS group: `compile/run.sh --group mini_apps_all`
- Direct Windows runner target: `powershell -File compile/windows/build.ps1 -Target text_editor_mini`
- Direct Windows runner group: `powershell -File compile/windows/build.ps1 -Group mini_apps_all`
- Linux target: `compile/linux/build.sh text_editor_mini`
- macOS target: `compile/macos/build.sh text_editor_mini`

Editor app entry sources can also live outside `examples/`; the Stygian editor
bootstrap now builds from `editor/apps/`.

Editor hardening note:

- `editor_quick_checkpoint` is the shortest useful editor checkpoint for frequent local loops.
- `editor_hardening_core` is the fast editor gate for daily iteration.
- `editor_release_gate` is the heavier gate before declaring a milestone stable.

## Add Your Own Example

1. Create your file in `examples/` (for example, `examples/my_app.c`).
2. Add a target entry in `compile/targets.json` under `targets`:
   - `backend`: `gl` or `vk`
   - `entry_source`: example source path
   - `output_stem`: output exe name
3. Build with:
   - `powershell -File compile/run.ps1 -Target <your_target_name>`

## Notes

- Windows runner validates shader outputs unless `-NoShaderCheck` is passed.
- Linux/macOS runners require `jq` to parse `targets.json`.
- CI workflow builds the tier test targets on Windows hosted runners using
  `-NoShaderCheck`.
- Runtime tier execution (`tests/run_all.ps1`) is available as a manual
  workflow dispatch path (`run_runtime=true`) and runs best-effort.
- `capture/` remains ignored/local until MP4 stability is verified on target
  machines.
