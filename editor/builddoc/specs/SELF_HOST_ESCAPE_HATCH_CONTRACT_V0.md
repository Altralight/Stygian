# Self-Host Escape Hatch Contract v0

This contract keeps dogfooding practical while migration is in progress.

## Boundary Rules

Generated slice:

- `stygian_editor_layout_shell.generated.h`
- `stygian_editor_layout_shell.generated.c`

Handwritten slice:

- `stygian_editor_layout_shell_host.h`
- `stygian_editor_layout_shell_host.c`

## Regeneration Policy

- generated files are rewritten every export
- handwritten host files are created only if missing
- handwritten host files are never overwritten by regeneration

## Why this matters

This allows mixed-mode shipping:

- migrate surfaces onto authoring stack incrementally
- keep app-owned logic and temporary glue in handwritten files
- regenerate visual shell safely without nuking host code

## Validation

`stygian_editor_self_host_escape_hatch_smoke` verifies handwritten host file
survives repeated regeneration unchanged.
