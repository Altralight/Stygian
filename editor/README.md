# Stygian Editor Module

Attachable visual editor module for Stygian that keeps authoring state separate
from generated runtime code.

## What This Module Does

- Provides an IDE-embeddable editor core (`StygianEditor`) with a clean C ABI.
- Provides a versioned module ABI table for host/plugin interop
  (`stygian_editor_module_get_api`).
- Tracks visual scene state (`rect`, `ellipse`, `path`) in an internal model.
- Supports adaptive multi-level grid and sub-snap behavior.
- Supports behavior rules (`trigger -> animate`) for interactive components.
- Renders a 2D viewport preview using Stygian primitives.
- Builds deterministic Stygian C23 source from editor state.

## Core Flow

1. Author in visual/editor state (`StygianEditor` APIs).
2. Use `stygian_editor_render_viewport2d(...)` for preview in host IDE panes.
3. Add behavior rules with `stygian_editor_add_behavior(...)`.
4. Export generated C23 with `stygian_editor_build_c23(...)`.

No core Stygian runtime changes are required for this first slice.

## Key Files

- `D:\Projects\Code\Stygian\include\stygian_editor.h`
- `D:\Projects\Code\Stygian\include\stygian_editor_module.h`
- `D:\Projects\Code\Stygian\editor\stygian_editor.c`
- `D:\Projects\Code\Stygian\editor\stygian_editor_module.c`

## Notes

- `from_value = NAN` in animation rules means "capture current property value
  at trigger time."
- Path shapes compile to deterministic `stygian_line(...)` calls in generated
  output.
- Grid major spacing adapts by zoom using a 1/2/5 progression.
