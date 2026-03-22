# Generated File Layout v0

## Goal

Define a stable, practical generated output layout that separates generated
files from hand-written extension points.

This is about ownership boundaries first. Fancy packaging can come later.

## Output Roots

Given export root `<out_root>`:

- Generated root: `<out_root>/generated/`
- Handwritten root: `<out_root>/` (hooks live here)

## v0 File Set

Generated (always overwritten on export):

- `<out_root>/generated/stygian_editor_scene.generated.h`
- `<out_root>/generated/stygian_editor_scene.generated.c`
- `<out_root>/generated/stygian_editor_manifest.generated.json`

Handwritten (created once, never overwritten by export):

- `<out_root>/stygian_editor_hooks.h`
- `<out_root>/stygian_editor_hooks.c`

## Ownership Rules

Generated files:

- fully owned by exporter
- include a warning header
- can be regenerated at any time

Handwritten hook files:

- owned by user or host app
- exporter may create missing files with starter stubs
- exporter must not overwrite existing hook files

## Partial Regeneration Strategy

Exporter updates only generated files and leaves hooks intact.

This gives us:

- deterministic regen for generated code
- safe extension points for app-specific logic
- clean diffs where generated churn does not clobber manual work

## Integration Notes

- `stygian_editor_scene.generated.c` currently carries the C23 text produced by
  `stygian_editor_build_c23(...)`.
- Header and manifest are emitted alongside it to make integration less ad hoc.
- Future versions can split behavior and scene into separate generated C units
  without changing ownership semantics.
