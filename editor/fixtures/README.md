# Editor Fixtures

This folder is reserved for editor-specific fixtures.

Planned buckets:

- `projects/` for sidecar project samples
- `export_golden/` for expected generated C23 output
- `runtime_scenes/` for smoke scene inputs and snapshots

Current fixture files:

- `roundtrip_harness_minimal.project.json`
- `roundtrip_harness_medium.project.json`
- `roundtrip_harness_deep_nested.project.json`
- `self_host_daily_dashboard_shell.project.json`
- `self_host_daily_tabs_settings.project.json`
- `self_host_daily_modal_flow.project.json`
- `next_systems_motion_layout_driver.project.json`
- `next_systems_component_states.project.json`
- `next_systems_ownership_effects.project.json`
- `export_c23_golden_signature.txt`
- `variant_resolution_golden.txt`
- `runtime_generated_scene_fixture.c`

Checkpoint commands:

- `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_pre_selfhost_checkpoint_fast.bat`
- `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_next_systems_checkpoint.bat`
- `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_pre_selfhost_checkpoint.bat`
