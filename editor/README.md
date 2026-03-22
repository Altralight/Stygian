# Stygian Editor Module

Attachable visual editor module for Stygian that keeps authoring state separate
from generated runtime code.

## What This Module Does

- Provides an IDE-embeddable editor core (`StygianEditor`) with a clean C ABI.
- Provides a versioned module ABI table for host/plugin interop
  (`stygian_editor_module_get_api`).
- Tracks visual scene state (`rect`, `ellipse`, `path`) in an internal model.
- Supports adaptive multi-level grid and sub-snap behavior.
- Supports behavior rules with animate/set-property/toggle/set-variable/navigate
  actions and generated runtime dispatch hooks.
- Supports component overrides, variant/property metadata, reusable styles, and
  variable modes with generated token tables.
- Renders a 2D viewport preview using Stygian primitives.
- Builds deterministic Stygian C23 source from editor state.

## Core Flow

1. Author in visual/editor state (`StygianEditor` APIs).
2. Use `stygian_editor_render_viewport2d(...)` for preview in host IDE panes.
3. Add behavior rules with `stygian_editor_add_behavior(...)`.
4. Export generated C23 with `stygian_editor_build_c23(...)`.

No core Stygian runtime changes are required for this first slice.

## Key Files

- `D:\Projects\Code\Stygian\editor\include\stygian_editor.h`
- `D:\Projects\Code\Stygian\editor\include\stygian_editor_module.h`
- `D:\Projects\Code\Stygian\editor\src\stygian_editor.c`
- `D:\Projects\Code\Stygian\editor\src\stygian_editor_module.c`
- `D:\Projects\Code\Stygian\editor\apps\stygian_editor_bootstrap.c`
- `D:\Projects\Code\Stygian\editor\apps\stygian_editor_proto.c`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_bootstrap.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_bootstrap_vk.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_project_roundtrip_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_project_roundtrip_harness.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_export_determinism_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_generated_file_layout_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_export_diagnostics_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_node_kinds_roundtrip_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_hierarchy_reparent_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_transform_commands_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_property_system_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_layers_tree_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_frame_clip_export_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_constraints_resize_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_constraints_runtime_equivalent_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_auto_layout_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_auto_layout_runtime_equivalent_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_overflow_sizing_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_guides_snap_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_primitive_coverage_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_fill_stack_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_paint_geometry_batch_b_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_text_image_batch_c_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_component_linkage_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_phase6_components_styles_variables_smoke.bat`
- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_phase7_behavior_bridge_smoke.bat`

## Layout

- `D:\Projects\Code\Stygian\editor\include\` - public editor headers
- `D:\Projects\Code\Stygian\editor\src\` - editor implementation
- `D:\Projects\Code\Stygian\editor\apps\` - thin bootstrap and prototype entrypoints
- `D:\Projects\Code\Stygian\editor\compile\` - editor-local build wrappers
- `D:\Projects\Code\Stygian\editor\build\` - disposable editor build output
- `D:\Projects\Code\Stygian\editor\builddoc\` - private editor planning and build context
- `D:\Projects\Code\Stygian\editor\tests\` - editor test surfaces
- `D:\Projects\Code\Stygian\editor\fixtures\` - editor fixtures and goldens

Windows wrappers compile through the shared target pipeline and stage editor
artifacts into `D:\Projects\Code\Stygian\editor\build\windows\`.

## Gate Runs

- Fast local checkpoint (short loop):
  `D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_pre_selfhost_checkpoint_fast.bat`
- Daily hardening build gate:
  `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_hardening_core.bat`
- Milestone release build gate:
  `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_release_gate.bat`
- Full strict checkpoint:
  `D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_pre_selfhost_checkpoint.bat`

Fallback through shared runner:

- `powershell -File D:\Projects\Code\Stygian\compile\run.ps1 -Group editor_quick_checkpoint`
- `powershell -File D:\Projects\Code\Stygian\compile\run.ps1 -Group editor_hardening_core`
- `powershell -File D:\Projects\Code\Stygian\compile\run.ps1 -Group editor_release_gate`

## Planning Files

- `D:\Projects\Code\Stygian\editor\builddoc\specs\FIGMA_TO_STYGIAN_RESEARCH.md`
- `D:\Projects\Code\Stygian\editor\builddoc\specs\EDITOR_PROJECT_SCHEMA_V0.md`
- `D:\Projects\Code\Stygian\editor\builddoc\specs\GENERATED_FILE_LAYOUT_V0.md`
- `D:\Projects\Code\Stygian\editor\builddoc\specs\EXPORT_DIAGNOSTICS_V0.md`
- `D:\Projects\Code\Stygian\editor\builddoc\specs\GRADIENT_EFFECT_PARITY_V0.md`
- `D:\Projects\Code\Stygian\editor\builddoc\specs\MASK_RUNTIME_PARITY_V0.md`
- `D:\Projects\Code\Stygian\editor\builddoc\plans\TASKLIST.md`
- `D:\Projects\Code\Stygian\editor\builddoc\verification\VERIFICATION_LOG.md`

`TASKLIST.md` is the working ledger.
`VERIFICATION_LOG.md` records the test or validation run for every item that
gets moved to done.

## Notes

- `from_value = NAN` in animation rules means "capture current property value
  at trigger time."
- Path shapes compile to deterministic `stygian_line(...)` calls in generated
  output.
- Grid major spacing adapts by zoom using a 1/2/5 progression.
