# Export Diagnostics v0

## Purpose

Export must never fail silently.

If a feature is not supported yet, export must either:

- degrade intentionally with a warning, or
- fail with an error and a clear reason.

## Data Model

`StygianEditorExportDiagnostic`:

- `severity`: info | warning | error
- `node_id`: offending or related node (or `0` for global issues)
- `feature`: stable short key
- `message`: human-readable actionable detail

## API

- `stygian_editor_collect_export_diagnostics(...)`
- `stygian_editor_build_c23_with_diagnostics(...)`

`build_c23_with_diagnostics(...)` behavior:

- fills diagnostics
- returns `0` if hard errors exist
- still allows output when diagnostics are warning-only

## v0 Rules

Warning (degrade intentionally):

- `closed_path_fill`:
  closed paths currently export as stroke segments only (no fill primitive yet).

Error (fail loudly):

- `path_point_count`:
  path has fewer than 2 points.
- `behavior_missing_node`:
  behavior references a trigger or target node that does not exist.

## Host Guidance

- Show warning diagnostics inline but allow export artifact usage.
- Block build pipeline usage on any error diagnostics.
- Render diagnostics with node IDs so users can jump to offending items.
