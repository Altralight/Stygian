# Stygian Editor Next Systems Tasklist

This phase starts after the interaction reset closed.

The reset made the bootstrap editor less confused.
This phase is what decides whether the editor becomes a serious authoring tool.

## Status Markers

- `[ ]` not started
- `[-]` in progress
- `[x]` finished

## Rules

- Do not mark a system done because a demo path exists.
- Mark it done only when the editor surface, export model, and verification all agree.
- Every finished task must add evidence to:
  - `D:\Projects\Code\Stygian\editor\builddoc\verification\VERIFICATION_LOG.md`
- Keep generated C23 output as the target. Do not hide unfinished logic behind a new DSL.
- Keep shader-slot depth explicit when the editor ABI does not expose enough yet.
- Keep generic runtime property polling as a later advanced runtime service.
  - Do not make polling the default authoring path.
  - Default path stays authored drivers plus explicit generated helpers.
  - Add polling later only when we want richer reactive/runtime systems without
    muddying ownership or deterministic export.

## Batch A: Procedural Motion Systems

- [x] NS-100 Build operator-based motion controllers.
  - Done when:
    - loop, ping-pong, stagger, repeater-offset, and delay controllers exist
    - controllers bind to selected tracks or selections without manual track surgery
    - controller intent survives save/load/export cleanly
  - Verification:
    - build three representative motion scenes without hand-editing generated code

- [x] NS-101 Build property driver surfaces.
  - Done when:
    - one property can drive another through a visible controller surface
    - named inputs and ranges are inspectable
    - driver evaluation compiles into deterministic runtime code
  - Verification:
    - create a scroll/slider-driven animation scene and re-open it cleanly

- [x] NS-102 Separate presets from authored controllers.
  - Done when:
    - quick presets remain available
    - promoted controllers become editable systems instead of opaque baked keys
  - Verification:
    - apply preset, promote it, edit it, export it, re-open it

## Batch B: Layout And Constraints

- [x] NS-200 Build real frame layout modes.
  - Done when:
    - frames support direction, padding, gap, alignment, wrap policy, and clipping
    - child sizing rules are visible and editable
  - Verification:
    - recreate representative sidebar, modal, and settings layouts without manual pixel pushing

- [x] NS-201 Build child constraint and pin behavior.
  - Done when:
    - pinning, fill, hug, fixed, and mixed sizing states exist
    - resize behavior is predictable across text, image, and frame children
  - Verification:
    - resize sweep across nested layouts with text and image content

- [x] NS-202 Build responsive layout export confidence.
  - Done when:
    - layout authoring compiles into readable generated C23 layout code
    - re-export diff quality stays sane for small layout edits
  - Verification:
    - small layout edits cause small runtime diffs across at least two fixtures

## Batch C: Export Ownership And Code Insight

- [x] NS-300 Deepen generated-code ownership surfaces.
  - Done when:
    - editor panels show generated zone, hook zone, and authored project zone clearly
    - exported files carry visible ownership markers and hook guidance
  - Verification:
    - export review sweep across generated files, hook files, and code insight panel

- [x] NS-301 Build hook discovery and binding UX.
  - Done when:
    - interactive nodes show bound hook symbol, source location, and missing-hook state
    - missing hooks can be stubbed from the editor surface
  - Verification:
    - bind, generate, edit, and re-export several hook-backed interactions

- [x] NS-302 Add export diff quality gates.
  - Done when:
    - tiny scene edits cause tiny runtime diffs
    - rename-only edits do not scramble unrelated output
    - stable symbol naming is enforced by tests
  - Verification:
    - diff-quality fixture sweep

## Batch D: Components, Variants, And Stateful UI

- [x] NS-400 Build component definitions and instance overrides.
  - Done when:
    - reusable components can be defined, inserted, and overridden safely
    - overrides are visible in Layers and Inspector
  - Verification:
    - component instance sweep with override add/remove and export roundtrip

- [x] NS-401 Build variant families and state switching.
  - Done when:
    - variants model tabs, toggles, disclosure states, and similar UI families
    - state names and transitions stay inspectable
  - Verification:
    - build at least one tab system and one toggle family without copy-paste hacks

- [x] NS-402 Build component properties as runtime-facing parameters.
  - Done when:
    - component properties feed generated runtime code cleanly
    - the mapping from editor property to exported symbol is visible
  - Verification:
    - property-driven component export sweep

## Batch E: Shader And Effects Depth

- [x] NS-500 Expand effect stack editing.
  - Done when:
    - multi-effect stacks are editable, reorderable, and inspectable
    - masks and clip ownership read clearly in panel surfaces
  - Verification:
    - effect-stack walkthrough on representative scenes

- [x] NS-501 Add shader attachment editing once the ABI surface exists.
  - Done when:
    - shader slots can be attached, configured, and owned from the editor
    - shader attachments export with clear ownership and diagnostics
  - Verification:
    - attach shader to scene content, export, reload, and confirm roundtrip stability

## Batch F: Integration And Confidence

- [x] NS-600 Build fixture-grade self-host scenes for the new systems.
  - Done when:
    - procedural motion, layout, components, and ownership surfaces all appear in fixtures
  - Verification:
    - fixture save/load/export/reopen sweep

- [x] NS-601 Build a next-systems checkpoint gate.
  - Done when:
    - one command proves the new systems without the full slow kitchen sink
  - Verification:
    - checkpoint pass from a clean build

## Progress

- Next systems tasks total: `16`
- Done: `16`
- In progress: `0`
- Remaining: `0`
- Completion: `100.0%`
