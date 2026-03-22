# Stygian Editor Frontend MVP Tasklist

This is the short-path ship list for the editor as an actual tool for mocking
native C23 applications.

The target is not "lots of features on paper."
The target is:

- quickly design polished native app fronts
- animate and wire common UI behavior
- export deterministic Stygian C23 that stays understandable

If a task does not move those three outcomes, it does not belong in the MVP
lane.

## Status Markers

- `[ ]` not started
- `[-]` in progress
- `[x]` finished

## Working Rule

- A lane is only complete when the authored feature is usable in-editor and
  exportable to generated Stygian C23 with sensible diagnostics.

## Current Baseline

These exist already in the current shell and should not be re-planned from
scratch:

- custom chrome shell, workspace switch, docked panels, and Stygian-native modal
  browser
- viewport pan and zoom
- frame placement
- rectangle, rounded-rectangle, ellipse, line, arrow, polygon, star, text, image, and baseline path placement/editing
- single select, marquee select, shift multi-select, group move, group resize,
  and single/group rotation
- smart guides, grid snap toggle, rulers, and origin marker
- inspector editing for transform, radius, text content, text size, and live
  per-node fill / stroke / opacity values
- context menu baseline and keyboard delete / duplicate / nudge
- scene clipboard now supports copy, cut, and paste for authored nodes
- snapshot-based undo and redo for viewport and inspector-settled scene edits
- generated C23 now carries authored node rotation through retained nodes and
  vector draw-path export
- MVP primitive coverage now has a direct smoke spanning frame, rect, rounded
  rect, ellipse, line, arrow, polygon, star, path, text, and image with
  JSON roundtrip plus deterministic C23 export

## MVP Phases

- [x] FM-100 Authoring Core Must Stop Fighting The User
  - why: if selection, placement, and edit history still feel brittle, nothing
    else matters.
  - done when:
    - copy, cut, paste, duplicate, delete, and select-all are reliable
    - undo and redo cover viewport edits and inspector edits
    - rotate is direct and usable for single and multi-selection
    - placement, drag, resize, and snap feedback stay predictable at normal zooms
  - verification:
    - create a small UI surface, duplicate parts, rotate them, undo back to
      clean state, redo to final state
    - confirm exported node transforms match authored state
    - `stygian_editor_rotation_export_smoke`
    - `stygian_editor_transform_commands_smoke`
    - `stygian_editor_undo_redo_smoke`

- [x] FM-200 Content Primitives Reach Real UI Mockup Range
  - why: we cannot mock a file manager, IDE, browser, or engine shell on frames
    and circles alone.
  - done when:
    - text node exists and is editable on-canvas and in inspector
    - primitive set includes frame, rect, rounded rect, ellipse, line, arrow,
      polygon, star, and path
    - path editing supports point placement, point move, point delete, and close
      path
    - image placement works as a first-class scene node
  - verification:
    - build a simple toolbar, tab strip, sidebar, and card layout without using
      fake placeholder shapes
    - export and confirm all node kinds emit deterministic Stygian C23
    - `stygian_editor_content_primitives_mvp_smoke`
    - `stygian_editor_primitive_coverage_smoke`
    - `stygian_editor_rotation_export_smoke`

- [x] FM-300 Visual Styling Covers Real Product Surfaces
  - why: mockups only become convincing once fill, stroke, text, and effects are
    practical instead of theoretical.
  - done when:
    - fill, stroke, opacity, corner model, and per-corner radius are editable
    - gradients exist for supported node types
    - shadows, blur, and blend basics exist with clear ownership
    - images support fit, crop baseline, and opacity
    - text supports font, size, weight, color, letter spacing, alignment,
      wrapping, and overflow
  - verification:
    - recreate a polished desktop panel with depth, text hierarchy, and icon/image
      accents
    - export and confirm appearance stacks round-trip cleanly
    - `stygian_editor_gradient_effect_parity_smoke`
    - `stygian_editor_text_shaping_overflow_parity_smoke`
    - `stygian_editor_text_image_batch_c_smoke`
  - finished with:
    - shell inspector editing for fill mode, gradient second stop, gradient angle,
      stroke color, stroke width, opacity, blend, effects, image fit, text weight,
      line height, tracking, alignment, and per-corner radius
    - canvas rendering that now previews linear gradients, shadow/blur/glow,
      weighted text, aligned text blocks, image fit modes, and per-corner radius
    - generated C23 helpers for fitted image export and plain text block layout,
      plus a targeted diagnostic when rich styled spans still outrun aligned text
      export

- [x] FM-400 Motion And Interaction Reach Mockup-Useful Depth
  - why: "Figma plus animation" means more than raw transforms.
  - done when:
    - timeline can author translation, rotation, scale, opacity, and color changes
    - easing curves are editable and previewable
    - tabs, drawers, modals, hover, and press states can be wired without custom
      code for the common case
    - simple repeat/state patterns are possible for shells like sidebars, tab
      pages, and expanding panels
  - verification:
    - author an animated tab system and an animated sidebar reveal without
      hand-editing generated code
    - export and run a mock interaction sample
  - finished with:
    - shell canvas now has a dedicated Motion tab, clip/track authoring, easing
      preview, direct timeline scrubbing/key dragging, and preset bindings for
      hover, press, and toggle behaviors
    - left rail now seeds tab, drawer, and modal scaffolds instead of leaving
      those patterns as manual node-by-node chores
    - targeted export coverage now proves sidebar reveal, tabs, and timeline
      content survive build/load/build without hand-editing generated C23
  - verification run:
    - `cmd /c editor\compile\windows\build_stygian_editor_shell_canvas.bat`
    - shell runtime sanity launch of
      `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_shell_canvas.exe`
    - `cmd /c "editor\compile\windows\build_stygian_editor_tabs_behavior_smoke.bat && editor\build\windows\stygian_editor_tabs_behavior_smoke.exe"`
    - `cmd /c "editor\compile\windows\build_stygian_editor_animation_transition_timeline_smoke.bat && editor\build\windows\stygian_editor_animation_transition_timeline_smoke.exe"`
    - `powershell -ExecutionPolicy Bypass -File D:\Projects\Code\Stygian\compile\windows\build.ps1 -Target stygian_editor_mock_interaction_export_smoke`
      then `D:\Projects\Code\Stygian\build\stygian_editor_mock_interaction_export_smoke.exe`

- [x] FM-500 Workspace Structure Supports Real App Mocking
  - why: once documents grow, the editor must stay obvious to use.
  - done when:
    - layers panel can manage hierarchy, visibility, lock, naming, and reorder
    - assets/import surface supports images, textures, and fonts cleanly
    - tabs or document pages inside the editor are manageable without chaos
    - inspector sections are compact and clearly grouped by Transform, Layout,
      Appearance, Effects, Text, and Interaction
  - finished with:
    - left workspace panel now has `Pages`, `Layers`, and `Assets` tabs instead of one flat import block
    - scene model now tracks page ownership, active page switching, parent links, visibility, and lock state
    - layers tooling now supports naming, parent cycling, visibility, lock, and reorder from the panel
    - assets/import flow now records imported images, textures, and fonts into an asset library that can be reselected from the panel
    - right inspector design tab is regrouped around Transform, Layout, Appearance, Effects, Text, and Interaction
  - verification:
    - build a multi-panel application shell with at least 20 authored nodes
    - confirm editing remains navigable without viewport-only hacks
    - `cmd /c editor\compile\windows\build_stygian_editor_shell_canvas.bat`
    - shell runtime sanity launch of
      `D:\Projects\Code\Stygian\editor\build\windows\stygian_editor_shell_canvas.exe`
    - `cmd /c "editor\compile\windows\build_stygian_editor_workspace_structure_smoke.bat && editor\build\windows\stygian_editor_workspace_structure_smoke.exe"`

- [x] FM-600 Export Trust Is Strong Enough For Real Use
  - why: the editor only pays off if the generated C23 is stable and usable.
  - done when:
    - every MVP-authored node and effect has deterministic generated output
    - unsupported cases fail loudly with node-local diagnostics
    - generated code ownership versus hook ownership stays explicit
    - tiny scene edits produce tiny textual diffs
  - finished with:
    - ownership markers in generated C23 are now covered directly by a frontend-MVP trust smoke instead of relying only on lower-level layout tests
    - aggregate export trust coverage now checks deterministic repeated export, build/load/build parity, node-local unsupported diagnostics, and tiny-diff behavior on a realistic mixed mockup
    - trust verification now spans both direct export surface and generated-runtime / interaction fixtures, so this lane is closing on evidence rather than a single lucky path
  - verification:
    - export the same project twice and diff outputs
    - edit one small visual property and confirm limited diff churn
    - run generated sample mocks through the current editor build pipeline
    - `cmd /c "editor\compile\windows\build_stygian_editor_frontend_mvp_export_trust_smoke.bat && editor\build\windows\stygian_editor_frontend_mvp_export_trust_smoke.exe"`
    - `cmd /c "editor\compile\windows\build_stygian_editor_export_determinism_smoke.bat && editor\build\windows\stygian_editor_export_determinism_smoke.exe"`
    - `cmd /c "editor\compile\windows\build_stygian_editor_export_diff_quality_smoke.bat && editor\build\windows\stygian_editor_export_diff_quality_smoke.exe"`
    - `cmd /c "editor\compile\windows\build_stygian_editor_export_diagnostics_smoke.bat && editor\build\windows\stygian_editor_export_diagnostics_smoke.exe"`
    - `cmd /c "editor\compile\windows\build_stygian_editor_generated_file_layout_smoke.bat && editor\build\windows\stygian_editor_generated_file_layout_smoke.exe"`
    - `cmd /c "editor\compile\windows\build_stygian_editor_generated_scene_runtime_smoke.bat && editor\build\windows\stygian_editor_generated_scene_runtime_smoke.exe"`
    - `cmd /c "editor\compile\windows\build_stygian_editor_export_c23_golden_smoke.bat && editor\build\windows\stygian_editor_export_c23_golden_smoke.exe"`
    - `powershell -ExecutionPolicy Bypass -File D:\Projects\Code\Stygian\compile\windows\build.ps1 -Target stygian_editor_mock_interaction_export_smoke -OutputDir editor\build\windows`
      then `cmd /c "editor\build\windows\stygian_editor_mock_interaction_export_smoke.exe"`

## Completion Bar

The editor is "frontend MVP complete" only when:

- an attractive file manager mockup is practical to author
- an attractive browser shell mockup is practical to author
- an attractive IDE shell mockup is practical to author
- those mockups can animate and switch state
- those mockups export into readable generated Stygian C23 without hand rescue

Until then, we are still building the tool.
