# Advanced Control Recipes V0

## Goal

Add first-party scaffolds for common form and data-display controls so authors can
drop usable baseline structures without manual subtree wiring.

## Delivered

- Added control recipe enum kinds:
  - `STYGIAN_EDITOR_RECIPE_CHECKBOX`
  - `STYGIAN_EDITOR_RECIPE_RADIO_GROUP`
  - `STYGIAN_EDITOR_RECIPE_INPUT_FIELD`
  - `STYGIAN_EDITOR_RECIPE_DROPDOWN`
  - `STYGIAN_EDITOR_RECIPE_DATA_TABLE`
  - `STYGIAN_EDITOR_RECIPE_DATA_CARD`
- Extended `stygian_editor_add_control_recipe(...)` to emit deterministic node
  scaffolds for each new recipe family.
- Checkbox scaffold includes press toggle behavior for check indicator visibility.
- Dropdown scaffold emits label + chevron marker.
- Data table/card scaffolds emit multi-node layout shells suitable for override
  and behavior binding layers.
- Added smoke test target:
  - `editor/tests/advanced_control_recipes_smoke.c`
  - build target: `stygian_editor_advanced_control_recipes_smoke`
- Added the new smoke target to:
  - `editor_hardening_core`
  - `editor_release_gate`

## Scope Notes

- V0 recipes are structure-first baselines, not full interaction-complete widgets.
- Geometry and behavior defaults are deterministic and export-safe.

## Verification

- `editor/tests/advanced_control_recipes_smoke.c`
- target: `stygian_editor_advanced_control_recipes_smoke`
