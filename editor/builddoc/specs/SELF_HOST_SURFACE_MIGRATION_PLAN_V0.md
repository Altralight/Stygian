# Self-Host Surface Migration Plan v0

This plan defines staged migration of editor-owned surfaces onto the editor
authoring stack.

## Stage 1

- root shell frames (`editor_root`, `top_chrome`, `viewport_shell`, `inspector_shell`)
- top chrome slot placeholders (`file_menu_slot`, `place_button_slot`)

## Stage 2

- inspector-hosted surfaces (`behavior_graph_surface`, `console_surface`)
- retained stage 1 shell hierarchy

## Stage 3

- viewport-hosted tools surface (`tools_surface`)
- minimal control placeholders (`tool_place_shape`, `tool_point_place`)
- retained stage 1+2 hierarchy

## Runtime Hook

Bootstrap file menu exposes:

- `Dogfood Stage 1`
- `Dogfood Stage 2`
- `Dogfood Stage 3`

Each action rebuilds the current editor scene to that migration stage so the
surface can be edited, saved, exported, and rebuilt repeatedly.

## Validation

`stygian_editor_self_host_stage_migration_smoke` verifies per stage:

- stage scene can be rebuilt from clean state
- sidecar save/load round-trip succeeds
- exported C23 is deterministic and stable through round-trip
- stage complexity increases monotonically (node count ordering)
