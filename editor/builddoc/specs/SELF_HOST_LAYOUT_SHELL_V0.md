# Self-Host Layout Shell v0

This document defines the first editor self-hosting slice exported as generated
Stygian C23.

## Scope

The shell scene includes:

- `editor_root` frame
- `top_chrome` frame
- `viewport_shell` frame
- `inspector_shell` frame
- minimal top-bar placeholders (`file_menu_slot`, `place_button_slot`)

## Goal

Prove the editor can author and export a meaningful piece of its own UI shell as
runtime C23 output, with deterministic parent/child + constraint mapping.

## Non-Goals

- full parity with handwritten bootstrap behavior and docking
- complete panel internals
- full self-hosted interaction graph

## Validation

`stygian_editor_self_host_layout_shell_smoke` verifies:

- authored parent relationships for shell frames
- generated C23 contains expected parent/child constraint records
- deterministic repeated C23 generation
- shell artifact is written to
  `editor/build/windows/stygian_editor_layout_shell.generated.c`
