# UX Depth Batch 4 V0

## Goal

Deliver UX-depth features that improve inspection, onboarding, and trust without
changing runtime ownership boundaries.

## Delivered

- Logic panel contract in inspector:
  - selected-node logic summary with supported events
  - behavior connectivity status (`connected`, `missing-node`, `missing-hook`)
  - ownership hinting for binding-related actions
- Opt-in code insight:
  - toggleable from top chrome
  - selected node insight shows generated C23 location and hook usage counts
  - refresh action available in inspector
- Template onboarding actions in File menu:
  - tabs scaffold
  - sidebar app shell
  - modal flow
  - settings panel
- Progressive complexity modes:
  - `Beginner` mode suppresses low-level runtime wiring surface
  - `Systems` mode exposes explicit IDs/contracts and code insight flow
- Runtime contract surfacing in systems mode:
  - schema version + generated/hook ownership contract text in inspector

## Scope Notes

- Batch 4 focuses on authoring UX clarity and onboarding velocity.
- Runtime export model remains unchanged: generated files are exporter-owned and
  hook files remain user-owned.
