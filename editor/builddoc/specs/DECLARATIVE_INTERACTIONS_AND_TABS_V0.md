# Declarative Interactions + Tabs V0

## Goal

Ship Batch 2 interaction core so common behavior authoring is fully declarative
in the editor and tabs can be authored visually without mandatory code.

## Delivered

- Function Editor now supports declarative behavior actions:
  - `Animate`
  - `Set Property`
  - `Toggle Visibility`
  - `Set Variable`
  - `Navigate`
- Action-specific inputs are shown conditionally in the panel instead of forcing
  one animate-only flow.
- Added tabs scaffold action in File menu:
  - `Add Tabs Scaffold`
  - creates a visual tabs bar and three content surfaces
  - wires press-driven declarative switching behavior for header/content states
- Added optional dynamic-tab hook stubs in export bundle hooks:
  - `stygian_editor_hooks_tab_count(...)`
  - `stygian_editor_hooks_tab_label(...)`

## Test Coverage

- Added smoke:
  - `editor/tests/tabs_behavior_smoke.c`
  - target: `stygian_editor_tabs_behavior_smoke`
- Integrated target in:
  - `editor_hardening_core`
  - `editor_release_gate`
  - pre-selfhost checkpoint runtime sequence

## Scope Notes

- V0 uses deterministic opacity switching for content-state transitions in tabs.
- Dynamic tab generation remains optional via hook pathway and is not required for
  static visual tab authoring.
