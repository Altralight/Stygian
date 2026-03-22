# Stygian Editor Tasklist

This is the working ledger for the editor.

We are building the editor first, not polishing screenshots.
The generated Stygian C23 is the runtime product.
The editor project file exists only so the editor can reload and keep authoring
intent.

## Status Markers

- `[ ]` not started
- `[-]` in progress
- `[x]` finished

## Working Rules

- Move only one major task into `[-]` at a time unless parallel work is truly
  independent.
- Do not move a task to `[x]` until its verification steps are run and logged in
  `D:\Projects\Code\Stygian\editor\builddoc\verification\VERIFICATION_LOG.md`.
- When a task is blocked, leave it as `[-]` and record the blocker in the task.
- Favor editor core, persistence, layout, and code generation over visual polish.
- If a feature cannot round-trip through the project file and export pipeline,
  it is not done.

## Already Settled

- [x] T000 Research what makes Figma strong and map it against current Stygian
  runtime and editor reality.
  - Deliverable:
    `D:\Projects\Code\Stygian\editor\builddoc\specs\FIGMA_TO_STYGIAN_RESEARCH.md`
  - Done when: we have a brutally honest inventory of must-have features, hard
    gaps, and export architecture constraints.
  - Verification:
    - confirm the research file exists and is readable
    - confirm local Stygian runtime capability references are captured
    - confirm official Figma sources are listed

- [x] T001 Create a repo-tracked execution ledger for the editor effort.
  - Deliverable:
    `D:\Projects\Code\Stygian\editor\builddoc\plans\TASKLIST.md`
  - Done when: tasks, statuses, and verification rules are written down in a
    form we will actually maintain.
  - Verification:
    - confirm the ledger file exists
    - confirm status markers and working rules are present

- [x] T002 Create a repo-tracked verification log.
  - Deliverable:
    `D:\Projects\Code\Stygian\editor\builddoc\verification\VERIFICATION_LOG.md`
  - Done when: every future finished task has an obvious place to record test
    evidence and pass or fail notes.
  - Verification:
    - confirm the log file exists
    - confirm it includes an entry format and initial records

- [x] T003 Lock the project direction: editor-core first, visual polish later.
  - Deliverable: this rule is documented in this ledger and the editor README.
  - Done when: nobody has to guess whether we are chasing UI chrome or the
    actual authoring engine.
  - Verification:
    - confirm this rule appears in
      `D:\Projects\Code\Stygian\editor\builddoc\plans\TASKLIST.md`
    - confirm planning files are linked from
      `D:\Projects\Code\Stygian\editor\README.md`

- [x] T004 Reorganize the editor subtree before core implementation begins.
  - Deliverable:
    - `D:\Projects\Code\Stygian\editor\builddoc\`
    - `D:\Projects\Code\Stygian\editor\include\`
    - `D:\Projects\Code\Stygian\editor\src\`
  - Done when:
    - editor docs live under `builddoc`
    - editor public headers live under `editor\include`
    - editor implementation lives under `editor\src`
    - build scripts and docs point at the new layout
  - Verification:
    - build the editor bootstrap target on OpenGL
    - build the editor bootstrap target on Vulkan
    - grep the repo for stale flat editor source and header paths

## Phase 1: Project Model And Export Contract

- [x] T100 Define the editor project sidecar schema v0.
  - Why: pure Stygian C23 is the runtime output, but it cannot preserve enough
    authoring intent to reload the editor.
  - Deliverable: a documented schema for scene graph nodes, hierarchy, layout,
    visual stacks, behaviors, variables, and editor-only metadata.
  - Done when:
    - stable top-level schema version exists
    - node identity rules are defined
    - serialization rules for unknown or future fields are defined
    - editor-only data is clearly separated from runtime-facing data
  - Verification:
    - save a minimal project and load it back without losing IDs or hierarchy
    - save a project with editor-only metadata and confirm that metadata round-trips
    - reject or warn cleanly on schema version mismatch
  - Implemented:
    - `D:\Projects\Code\Stygian\editor\include\stygian_editor.h`
    - `D:\Projects\Code\Stygian\editor\include\stygian_editor_module.h`
    - `D:\Projects\Code\Stygian\editor\src\stygian_editor_project_io.inl`
    - `D:\Projects\Code\Stygian\editor\builddoc\specs\EDITOR_PROJECT_SCHEMA_V0.md`
    - `D:\Projects\Code\Stygian\editor\tests\project_roundtrip_smoke.c`
  - Blockers:
    - none yet

- [x] T101 Define stable node IDs, symbol names, and regeneration rules.
  - Why: if generated names drift, merges get ugly and trust drops fast.
  - Deliverable: ID assignment rules, rename behavior, symbol sanitization, and
    deterministic emission order.
  - Done when:
    - IDs survive save and load
    - generated identifiers are reproducible across clean rebuilds
    - renamed display labels do not break persistent IDs
  - Verification:
    - export the same project twice and diff the outputs
    - rename nodes and confirm only expected symbols change

- [x] T102 Define generated file layout and ownership boundaries.
  - Deliverable: exact generated file set for scene, behavior, tokens, and
    optional hooks.
  - Done when:
    - generated files have clear ownership and warning headers
    - hand-written extension points are separate from generated files
    - partial regeneration strategy is documented
  - Verification:
    - generate all declared files for a sample project
    - re-run generation without touching hand-written hook files

- [x] T103 Define unsupported-feature diagnostics.
  - Deliverable: export diagnostics model with warnings, errors, and fallbacks.
  - Done when:
    - unsupported authoring features fail loudly or degrade intentionally
    - diagnostics can point back to node IDs and feature names
  - Verification:
    - export a project with an intentionally unsupported feature
    - confirm the failure path is readable and actionable

- [x] T104 Build a save-load-export round-trip harness.
  - Deliverable: repeatable harness that can load project sidecar, export C23,
    reload, and compare normalized state.
  - Done when:
    - round-trip tests exist for minimal, medium, and nested projects
    - normalized diffs are stable
  - Verification:
    - run harness on at least three fixture projects
    - confirm clean pass and useful failure output

## Phase 2: Scene Graph And Authoring Core

- [x] T200 Expand the editor node model beyond rect, ellipse, and path.
  - Deliverable: internal node kinds for frame, text, image, line, arrow,
    polygon, star, arc, group, component definition, and instance.
  - Done when:
    - node kind registry is explicit
    - each node kind has a property carrier in the authoring model
    - serialization hooks exist even if rendering is not complete yet
  - Verification:
    - create one of each core node kind in memory
    - serialize and reload without kind corruption

- [x] T201 Add a real hierarchy model with parent-child ownership.
  - Deliverable: scene graph parenting, child ordering, reparenting, and root
    frame handling.
  - Done when:
    - nodes can move between parents
    - local and world transforms stay consistent
    - z-order and child order are deterministic
  - Verification:
    - reparent nested nodes and compare world bounds before and after
    - save and reload a deep hierarchy

- [x] T202 Build command-based transforms for move, scale, rotate, and reparent.
  - Deliverable: transform commands that the rest of the editor can reuse.
  - Done when:
    - direct manipulation and property edits both use the same command path
    - multi-select transforms share one command stack model
    - rotate is no longer a second-class citizen
  - Verification:
    - move, scale, and rotate single and multi-selected nodes
    - undo and redo each transform path cleanly

- [x] T203 Build undo and redo on top of explicit editor commands.
  - Deliverable: command journal with inverse operations and transaction support.
  - Done when:
    - all core editing actions can be undone and redone
    - multi-step actions can be grouped into transactions
  - Verification:
    - run scripted edit sequences and confirm state matches after undo and redo
    - confirm no ID churn from undo or redo

- [x] T204 Build a property system that can handle mixed multi-selection.
  - Deliverable: property descriptors, getters, setters, mixed-value handling,
    and validation rules.
  - Done when:
    - numeric, enum, bool, color, and binding properties share one model
    - multi-selection can show mixed values without lying
  - Verification:
    - select heterogeneous nodes and apply shared transform edits
    - confirm invalid values fail cleanly

- [x] T205 Build a layers-tree API before worrying about a nice layers panel.
  - Deliverable: data and commands for hierarchy browsing, locking, hiding,
    naming, and ordering.
  - Done when:
    - the scene graph can be fully managed without canvas-only interaction
    - the API is stable enough for an IDE host to drive
  - Verification:
    - create scripted tree edits without using the viewport
    - export and reload after hide, lock, reorder, and reparent operations

## Phase 3: Layout And Responsiveness

- [x] T300 Add a frame node with clipping and background paint.
  - Deliverable: first-class frame container in authoring model and codegen.
  - Done when:
    - root frame and nested frames exist
    - clip-content behavior is explicit
    - frame background paint exports predictably
  - Verification:
    - build nested frames, toggle clip on and off, export, and inspect output

- [x] T301 Add constraint rules against parent bounds.
  - Deliverable: left, right, top, bottom, center, scale, and stretch behavior.
  - Done when:
    - child layout can respond to parent resizing
    - constraints survive save, load, and export
  - Verification:
    - resize parent frames and compare child layout against expected rules
    - export generated C23 and test runtime resize callbacks or equivalent

- [x] T302 Add auto layout containers.
  - Deliverable: horizontal and vertical auto layout with padding, gap, and
    alignment.
  - Done when:
    - fixed, hug, and fill sizing exist
    - absolute-position children inside auto layout are supported
    - layout can be recomputed deterministically
  - Verification:
    - build sample button rows, card stacks, and nested containers
    - resize parent frames and confirm results stay stable

- [x] T303 Add min or max sizing and overflow behavior.
  - Deliverable: min-width, max-width, min-height, max-height, and overflow or
    scroll policy.
  - Done when:
    - layout solver respects size constraints
    - overflow can be clipped or made scrollable as designed
  - Verification:
    - stress-test containers with too much content
    - confirm emitted runtime code respects min or max logic

- [x] T304 Add guides, rulers, and layout snapping as data first.
  - Deliverable: guide model, ruler units, snap sources, and grid integration.
  - Done when:
    - snapping can use grid, guides, bounds, and parent edges
    - guide data persists in project files
  - Verification:
    - create guides, snap nodes to them, save, reload, and repeat

- [x] T305 Generate layout math instead of flattening everything to pixels.
  - Deliverable: emitted C23 code that computes bounds from parent size and
    runtime values when layout demands it.
  - Done when:
    - fixed nodes stay fixed
    - responsive nodes generate responsive code
  - Verification:
    - export layout-heavy sample scenes
    - run runtime resize scenarios and compare against editor preview expectations

## Phase 4: Paint, Geometry, And Vector Authoring

- [x] T400 Expand primitive coverage.
  - Deliverable: line, arrow, polygon, star, arc, and explicit frame creation.
  - Done when:
    - each primitive has authoring data, serialization, and export support
  - Verification:
    - create one of each primitive and export a sample scene

- [x] T401 Build a fill stack model.
  - Deliverable: multiple fills per node with visibility, opacity, and ordering.
  - Done when:
    - solid, gradient, and image fills can coexist in authoring data
    - export either maps them directly or fails clearly when unsupported
  - Verification:
    - save and export nodes with multiple fills
    - compare emitted output against expected fill order

- [x] T402 Build a real stroke model.
  - Deliverable: stroke stack, width, caps, joins, alignment, miter, and dash.
  - Done when:
    - strokes are no longer a special case bolted onto paths only
  - Verification:
    - export varied stroke fixtures including dashed and corner-heavy cases

- [x] T403 Build gradient authoring and export.
  - Deliverable: linear, radial, and angular gradients if runtime mapping holds.
  - Done when:
    - gradient stops and transforms survive round-trip
    - export maps into real Stygian gradient calls where possible
  - Verification:
    - save and reload gradient-heavy sample nodes
    - diff exported code for stable stop ordering

- [x] T404 Build effect stacks.
  - Deliverable: shadow, inner shadow if supported, blur, glow, and later noise.
  - Done when:
    - effect ordering is preserved
    - export path is deterministic
  - Verification:
    - export nodes with multiple effects and compare rendered results on fixtures

- [x] T405 Build clip and mask authoring.
  - Deliverable: clipping groups and mask relationships.
  - Done when:
    - masks are explicit in authoring model
    - export either emits runtime clip structures or fails loudly
  - Verification:
    - build sample mask scenes and confirm save-load-export behavior

- [x] T406 Upgrade path editing from point placing to real vector editing.
  - Deliverable: bezier handles, open or closed paths, point insertion, point
    deletion, segment conversion, and tangent handling.
  - Done when:
    - paths can cover real icon work instead of rough polyline drawing
  - Verification:
    - create icon-like fixtures and compare path data before and after reload

- [x] T407 Add boolean operations and flattening.
  - Deliverable: union, subtract, intersect, exclude, and flatten to path when
    requested.
  - Done when:
    - non-destructive boolean intent can be saved
    - flattening produces stable geometry
  - Verification:
    - build boolean fixtures, save and reload them, then flatten and compare

## Phase 5: Text And Image Content

- [x] T500 Add text nodes to the authoring model.
  - Deliverable: text node kind, content storage, alignment, box modes, and
    sizing behavior.
  - Done when:
    - point text and area text both exist
    - text nodes can export through real Stygian text calls
  - Verification:
    - create text fixtures with multiple alignments and sizing modes
    - export and render them in runtime smoke scenes

- [x] T501 Add typography and mixed-style spans.
  - Deliverable: font family, weight, size, line height, letter spacing, and
    rich text span support.
  - Done when:
    - one text node can carry mixed styles without losing data on reload
  - Verification:
    - save, reload, and export mixed-style text fixtures

- [x] T502 Add text measurement and auto-size behavior.
  - Deliverable: measurement hooks tied into layout and export.
  - Done when:
    - hug-content text sizing is stable in editor and generated runtime
  - Verification:
    - compare editor size and runtime size for the same text fixture

- [x] T503 Add image nodes and image fills.
  - Deliverable: image asset reference model, crop or fit or fill modes, and
    export path.
  - Done when:
    - image data references survive save and load
    - emitted output uses real Stygian texture APIs
  - Verification:
    - load image fixtures, export, and render in smoke scenes

## Phase 6: Components, Styles, And Variables

- [x] T600 Add component definitions and instances.
  - Deliverable: component authoring model, instance linkage, and detach flow.
  - Done when:
    - changes to a component can propagate to instances
    - instance identity survives save and reload
  - Verification:
    - build a component with several instances and compare propagation behavior

- [x] T601 Add instance overrides.
  - Deliverable: override storage for text, visibility, swaps, transforms where
    legal, and style bindings.
  - Done when:
    - instance overrides survive regeneration
    - reset override works cleanly
  - Verification:
    - create override-heavy fixtures and round-trip them

- [x] T602 Add variants and component properties.
  - Deliverable: variant groups, enum or bool properties, and swap properties.
  - Done when:
    - a single component family can represent common UI states without duplication
  - Verification:
    - build a button family with size and state variants and export it

- [x] T603 Add styles.
  - Deliverable: color, text, effect, and layout style definitions with usage tracking.
  - Done when:
    - style updates can propagate or stay detached intentionally
  - Verification:
    - change a shared style and confirm affected nodes update correctly

- [x] T604 Add variables and modes.
  - Deliverable: variable collections, modes, bindings, and runtime emission plan.
  - Done when:
    - color and numeric variables can drive layout, paint, and behavior
    - theme modes can switch deterministically
  - Verification:
    - build light and dark theme fixtures and export both mode-aware outputs

- [x] T605 Generate token or variable support in C23 output.
  - Deliverable: emitted token tables or generated variable access helpers.
  - Done when:
    - variable-bound scenes no longer collapse into random literals at export time
  - Verification:
    - export token-heavy fixtures and diff the outputs across multiple runs

## Phase 7: Behavior And Runtime Bridge

- [x] T700 Expand the interaction model beyond raw property animation.
  - Deliverable: event graph for press, release, hover, drag, scroll, value
    changed, and focus-like states if needed.
  - Done when:
    - interaction model can describe common UI controls without hand-wiring everything
  - Verification:
    - serialize and reload behavior-heavy sample scenes

- [x] T701 Add common behavior actions.
  - Deliverable: set property, toggle visibility, set variable, navigate, and
    animate property.
  - Done when:
    - common control logic can be expressed without custom code for every case
  - Verification:
    - build sample button, toggle, tab, and accordion behaviors

- [x] T702 Generate runtime dispatch code from behavior graphs.
  - Deliverable: generated runtime event tables and dispatch helpers.
  - Done when:
    - behavior is not editor-preview-only anymore
  - Verification:
    - export behavior fixtures and run runtime interaction smoke tests

- [x] T703 Add generated hook boundaries for custom logic.
  - Deliverable: stable hook declarations and user-code separation rules.
  - Done when:
    - hand-written logic can survive regeneration without patching generated files
  - Verification:
    - add a hand-written hook, regenerate, and confirm it still builds

- [x] T704 Build first-class control recipes.
  - Deliverable: generated patterns for button, slider, toggle, scroll region,
    tabs, and accordion.
  - Done when:
    - common interactive controls stop being one-off experiments
  - Verification:
    - build runtime smoke scenes for each control recipe and test behavior

## Phase 7.5: Variant Hardening (Figma-Grade Gap Closure)

- [x] T705 Build a deterministic variant resolution engine.
  - Deliverable: explicit matching and fallback rules for multi-property variant
    selection.
  - Done when:
    - same inputs always resolve to the same variant ID
    - fallback behavior is ordered, documented, and test-covered
  - Verification:
    - run fixture matrix with missing/partial property sets and compare chosen variants

- [x] T706 Add subtree-aware instance overrides.
  - Deliverable: override model for nested child content/properties inside an
    instance, not only top-level host fields.
  - Done when:
    - nested text/visibility/style/property overrides can be applied and reset
    - override scope rules are deterministic and round-trip safe
  - Verification:
    - create nested override fixtures, save/load/export, and diff normalized state

- [x] T707 Add component-set inheritance and conflict semantics.
  - Deliverable: clear precedence between base component defaults, variant
    deltas, and instance overrides.
  - Done when:
    - precedence rules are enforced uniformly in authoring, reload, and export
    - conflict cases produce readable diagnostics instead of silent drift
  - Verification:
    - run precedence/conflict fixtures and verify deterministic outcomes

- [x] T708 Add variant swap graph constraints and compatibility checks.
  - Deliverable: legal/illegal swap validation using property contracts and set
    membership.
  - Done when:
    - incompatible swaps fail with actionable diagnostics
    - legal swaps preserve expected overrides where possible
  - Verification:
    - run swap fixtures across compatible and incompatible sets

- [x] T709 Add golden tests for variant resolution and override stability.
  - Deliverable: fixture + golden suite focused on variant matching, inheritance,
    and override reset behavior.
  - Done when:
    - variant engine regressions are caught before merge
    - intended behavior changes are explicit in snapshot diffs
  - Verification:
    - regenerate goldens and compare diff budget for variant fixtures

## Phase 8: Self-Hosting And Dogfooding

- [x] T800 Make the editor able to save and reload its own project files.
  - Deliverable: editor app can save its state to sidecar and reload it cleanly.
  - Done when:
    - current editing session can be fully restored
  - Verification:
    - save, close, reopen, and confirm full scene restoration

- [x] T801 Make the editor export its own layout shell as generated C23.
  - Deliverable: the editor can describe part of its own UI through exported
    code instead of only handwritten bootstrap code.
  - Done when:
    - a meaningful slice of the editor UI can be generated and run
  - Verification:
    - export a shell scene, build it, and compare against the editor-authored source state

- [x] T802 Move internal editor surfaces onto its own authoring stack in stages.
  - Deliverable: staged dogfooding plan for toolbars, panels, controls, and
    layout shells.
  - Done when:
    - each migrated slice can still be edited, saved, exported, and rebuilt
  - Verification:
    - migrate one surface at a time and run smoke tests after each move

- [x] T803 Keep a handwritten escape hatch while dogfooding.
  - Deliverable: clear boundary between generated editor UI and handwritten host
    or bootstrap code.
  - Done when:
    - we can keep shipping while migrating the editor onto itself
  - Verification:
    - confirm mixed generated and handwritten slices still build together

## Phase 9: Hardening, Regression, And Scale

- [x] T900 Add unit tests for serialization, scene graph, layout, and property rules.
  - Deliverable: focused non-UI tests for core editor behavior.
  - Done when:
    - core state logic has automated coverage
  - Verification:
    - run the unit suite and keep it green in CI or local repeatable scripts

- [x] T901 Add golden tests for exported C23.
  - Deliverable: snapshot fixtures for generated output.
  - Done when:
    - intended export changes are explicit in diffs
  - Verification:
    - regenerate fixtures and compare against committed golden outputs

- [x] T902 Add runtime smoke tests for generated scenes.
  - Deliverable: small executable scenes that exercise layout, paint, text, and behavior.
  - Done when:
    - exported code is tested as runtime product, not just as strings
  - Verification:
    - build and run smoke scenes on supported backends

- [x] T903 Add fuzz or hostile-load tests for project files.
  - Deliverable: invalid-input coverage for sidecar parsing and migration.
  - Done when:
    - bad files do not crash the editor or corrupt state silently
  - Verification:
    - run malformed project fixtures through load paths

- [x] T904 Add module ABI compatibility tests.
  - Deliverable: checks that host and editor module API stay aligned.
  - Done when:
    - ABI changes are deliberate and visible
  - Verification:
    - load the module through version checks and confirm graceful failure on mismatch

- [x] T905 Add performance budgets for scene load, layout solve, export, and viewport interaction.
  - Deliverable: repeatable metrics and budget thresholds.
  - Done when:
    - regressions can be caught before the editor feels sluggish
  - Verification:
    - run perf fixtures at agreed scene sizes and compare against budgets

## Phase 10: Full Figma-Grade Pass (E.5)

- [x] T950 Add explicit editor release-gate groups in shared build targets.
  - Deliverable:
    - `editor_hardening_core` and `editor_release_gate` groups in
      `D:\Projects\Code\Stygian\compile\targets.json`
    - gate usage docs in `D:\Projects\Code\Stygian\compile\README.md` and
      `D:\Projects\Code\Stygian\editor\README.md`
  - Done when:
    - we can compile all editor hardening targets with one group command
    - gate commands are documented in both compile and editor docs
  - Verification:
    - run `powershell -File D:\Projects\Code\Stygian\compile\run.ps1 -Group editor_hardening_core`
    - run key gate executables and confirm pass

- [x] T951 Add gradient and effect transform controls to authoring + export parity.
  - Deliverable: editable gradient transform handles and deterministic effect-space mapping.
  - Done when:
    - gradient origin/angle/scale are editable and round-trip safe
    - effects keep stable ordering and geometry mapping across save/load/export
  - Verification:
    - add dedicated gradient-transform fixtures and runtime smoke checks

- [x] T952 Add true runtime mask semantics parity for nested mask stacks.
  - Deliverable: mask-chain authoring model + generated runtime mask graph.
  - Done when:
    - nested masks render in the same order/behavior between preview and runtime
  - Verification:
    - add nested-mask fixtures and runtime diffs

- [x] T953 Add geometric boolean solver parity (non-destructive + flatten output quality).
  - Deliverable: deterministic boolean geometry generation and flatten stability.
  - Done when:
    - union/subtract/intersect/exclude produce stable geometry under export
  - Verification:
    - add boolean stress fixtures and flatten golden checks

- [x] T954 Add text shaping and overflow parity pass for mixed-style blocks.
  - Deliverable: deterministic shaping/measurement pipeline for mixed spans and overflow.
  - Done when:
    - editor preview and generated runtime measurements agree within tolerance
  - Verification:
    - run mixed-script/mixed-style text fixture comparisons

- [x] T955 Add variable-driven behavior binding parity (layout, paint, interaction).
  - Deliverable: variable bindings across behavior dispatch and generated runtime hooks.
  - Done when:
    - variable mode switches and event-driven updates match editor intent
  - Verification:
    - run mode-switch + interaction fixtures and output goldens

- [x] T956 Add component-instance detachment and repair semantics.
  - Deliverable: explicit detach/reattach workflows with deterministic override migration.
  - Done when:
    - detaching or reconnecting instances does not corrupt subtree overrides
  - Verification:
    - run detach/repair fixture matrix through roundtrip + export checks

- [x] T957 Add advanced control-recipe authoring templates (form + data-display set).
  - Deliverable: checkbox, radio, input field, dropdown, and table/card recipe scaffolds.
  - Done when:
    - generated runtime controls work without hand wiring for common states
  - Verification:
    - runtime smoke scenes per recipe family

- [x] T958 Add IDE-host interop contract test lane.
  - Deliverable: host/plugin ABI interaction tests for module loading and lifecycle.
  - Done when:
    - API negotiation, lifecycle calls, and failure paths are validated from host side
  - Verification:
    - run host-driven module load/unload/version mismatch scenarios

- [x] T959 Add final pre-selfhost checkpoint with strict gate policy.
  - Deliverable: checklist and automated gate sequence required before selfhost migration increases.
  - Done when:
    - release gate sequence is one command + documented pass criteria
  - Verification:
    - run full `editor_release_gate` and capture pass evidence in verification log

- [x] T960 Add self-host daily-flow fixture gate.
  - Deliverable: realistic fixture lane (`dashboard shell`, `tabs settings`,
    `modal flow`) with strict open/edit/export/reopen checks and diff budget.
  - Done when:
    - fixtures persist under `editor/fixtures`
    - dedicated smoke target validates deterministic export and tiny-edit diff bounds
    - pre-selfhost checkpoint runs the new lane
  - Verification:
    - run `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_self_host_daily_flow_smoke.bat`
    - run `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_self_host_daily_flow_smoke.exe`
    - run `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_pre_selfhost_checkpoint.bat`

- [x] T961 Add a fast pre-selfhost checkpoint lane for iteration speed.
  - Deliverable: dedicated quick build group and runtime checkpoint script that
    keep core confidence while cutting local loop time.
  - Done when:
    - quick checkpoint group exists in `compile/targets.json`
    - editor-local fast build + fast run wrappers exist
    - docs clearly separate fast checkpoint vs full checkpoint usage
  - Verification:
    - run `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\run_stygian_editor_pre_selfhost_checkpoint_fast.bat`
    - confirm all fast-lane runtime executables return `[PASS]`

## Phase 11: Self-Host Surface Polish

- [x] T962 Ship shortcut-first workflow actions for production authoring loops.
  - Deliverable:
    - shortcut actions for reset + dogfood stage seed paths
    - command palette parity entries for those actions
    - friction inventory/spec for this phase
  - Done when:
    - `Ctrl+N` resets canvas
    - `Ctrl+1`, `Ctrl+2`, `Ctrl+3` seed self-host stages
    - command palette exposes matching actions
  - Verification:
    - run `cmd /c D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_bootstrap.bat`
    - launch `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_bootstrap.exe`
  - Spec:
    - `D:\Projects\Code\Stygian\editor\builddoc\specs\SELF_HOST_SURFACE_POLISH_V0.md`

- [x] T963 Build self-host quick-action strip for template and scaffold creation.
  - Deliverable: explicit top-strip action group for `Tabs`, `Sidebar`, `Modal`,
    and `Settings` templates without opening the file menu.
  - Done when:
    - one-click actions exist in top strip
    - status feed shows deterministic result for each action
  - Verification:
    - build bootstrap target and run manual interaction sweep

- [x] T964 Harden top-chrome layout for narrow widths (no clipping/overlap).
  - Deliverable: responsive top-chrome layout behavior with graceful collapse
    rules for badges, hints, and command controls.
  - Done when:
    - no text/button overlap across representative widths
    - shortcut help and command palette remain readable under compression
  - Verification:
    - bootstrap run at narrow and wide window sizes
    - capture before/after screenshots for regression reference

## Phase 12: Creative Tool Shell Redesign

- [x] T970 Write the redesign brief before touching the shell.
  - Deliverable:
    - `D:\Projects\Code\Stygian\editor\builddoc\specs\CREATIVE_TOOL_SHELL_REDESIGN_V0.md`
  - Done when:
    - research references are captured
    - shell rules are explicit
    - acceptance targets for the redesign lane are written down
  - Verification:
    - confirm the spec exists and references external design/tool guidance
    - confirm the spec maps research into concrete editor shell rules

- [x] T971 Rebuild the global editor shell so it reads like a real creative app.
  - Deliverable: new app bar, grouped authoring strip, clearer state badges, and
    a working file menu with sensible grouping.
  - Done when:
    - the top chrome no longer looks like per-word debug buttons
    - file actions are grouped and readable
    - command / mode / ownership status stays visible without collisions
  - Verification:
    - build bootstrap target
    - launch and manually exercise menu + top-strip actions

- [x] T972 Rebuild left and right rails around actual workflow.
  - Deliverable: cleaner Scene/Layers, better Inspector grouping, clearer Logic
    surface, and a code panel that reads like a tool instead of an afterthought.
  - Done when:
    - scene/layers exposes meaningful hierarchy state
    - inspector is grouped by task
    - code panel embeds readable generated / hook source
  - Verification:
    - build bootstrap target
    - launch and manually exercise selection, logic, and code-panel workflows

- [x] T973 Rebuild the timeline into a visual motion workspace.
  - Deliverable: time ruler, lane treatment, selected-track emphasis, clip
    occupancy, and curve/value preview integrated into the panel.
  - Done when:
    - the timeline reads as animation editing at a glance
    - track/key/clip operations still work after the layout change
  - Verification:
    - build bootstrap target
    - launch and create/edit/delete timeline tracks, keys, and clips

- [x] T974 Run a control reliability pass on the redesigned shell.
  - Deliverable: hitbox, clipping, and overlap fixes across top chrome and dock
    panels touched by this redesign.
  - Done when:
    - key buttons visibly and reliably click
    - no obvious clipping/overlap remains in touched panels
  - Verification:
    - build bootstrap target
    - run a manual click sweep across top chrome, file menu, panels, and code view

- [x] T975 Capture the redesigned shell as a regression baseline.
  - Deliverable: screenshot set and verification record for representative
    window widths after the redesign lands.
  - Done when:
    - before/after or fixed-width captures exist for the new shell
    - verification log records what was checked and what still looks rough
  - Verification:
    - capture representative widths and attach artifacts in verification log

## Current Focus

- Completion: `81 / 81` tasks done (`100.0%`).
- In progress: `none`.
- Next up: `none in the scoped plan`.
- Remaining after current: `0` tasks.
- This phase is allowed to focus on shell quality because the editor core,
  persistence, and export lanes are already in place.

