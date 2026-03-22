# Stygian Editor Interaction Reset Tasklist

This is the reset lane for fixing how the editor actually feels to use.

It supersedes the old assumption that shell cleanup was enough.

## Status Markers

- `[ ]` not started
- `[-]` in progress
- `[x]` finished

## Rules

- Do not mark an item done because a code path exists.
- Mark it done only when direct usage feels correct and verification exists.
- Every finished task must add evidence to:
  - `D:\Projects\Code\Stygian\editor\builddoc\verification\VERIFICATION_LOG.md`
- Prefer viewport-first interaction over side-panel-only editing.
- Visual style work comes after behavior is sane.

## Phase A: Direction Lock

- [x] IR-001 Collect hard references for the reset.
  - Done when:
    - we have references for shell layout, selection/transform behavior,
      timeline behavior, layers, file/project flows, text/image systems,
      effects/shader ownership, and context menus
    - the user confirms the direction feels closer to the target
  - Verification:
    - store reference notes in the rebuild spec
    - `D:\Projects\Code\Stygian\editor\builddoc\specs\EDITOR_CAPABILITY_RESEARCH_MATRIX_V0.md`
      exists and is current

- [x] IR-002 Freeze the new interaction rules.
  - Done when:
    - tool behavior, cancel behavior, and viewport-first rules are written down
    - implementation order is locked
  - Verification:
    - `D:\Projects\Code\Stygian\editor\builddoc\specs\INTERACTION_RESET_REBUILD_V0.md`
      exists and matches the current plan

## Phase B: Tool Model Reset

- [x] IR-100 Replace toggle soup with a real tool state model.
  - Done when:
    - explicit tools exist for Select, Rect/Frame, Ellipse, Path, Pan, Zoom
    - current tool is always visible
    - `Esc` and tool switching behave predictably
  - Verification:
    - build bootstrap
    - manual tool-switch sweep across all core tools

- [x] IR-101 Rebuild primitive placement flow.
  - Done when:
    - choosing a primitive enters a clear placement mode
    - viewport click or drag creates the primitive directly
    - live preview is visible during placement
  - Verification:
    - place at least rect, ellipse, and frame by direct viewport action

- [x] IR-102 Rebuild point/path creation flow.
  - Done when:
    - point placement shows live preview at every step
    - finish/cancel behavior is obvious
    - right-click no longer conflicts with the future context menu path
  - Verification:
    - create, finish, and cancel several paths cleanly

## Phase C: Selection And Transform Reset

- [x] IR-200 Rebuild single-select behavior.
  - Done when:
    - click-to-select is predictable
    - drag-to-move does not fight selection timing
    - blank click clears when expected
  - Verification:
    - single-node selection/move/deselect sweep

- [x] IR-201 Rebuild multi-select behavior as one coherent group.
  - Done when:
    - marquee select, shift-click add/remove, and group bounds work together
    - move/scale/rotate apply to the whole active selection
  - Verification:
    - multi-node move, scale, rotate sweep

- [x] IR-202 Rebuild resize handles and shared bounds.
  - Done when:
    - handles are visible, consistent, and easy to hit
    - multi-select scaling uses shared bounds instead of fake visual selection
  - Verification:
    - resize sweep on single and multi-selection

- [x] IR-203 Add visible rotation workflow.
  - Done when:
    - canvas interaction for rotation exists
    - selection rotation is direct, not hidden behind inspector-only values
  - Verification:
    - rotate single and multi-selection on canvas

## Phase D: Context And Menus

- [x] IR-300 Add a real viewport context menu.
  - Done when:
    - right-click opens a context menu in the viewport
    - selection-aware actions are present
    - blank-canvas actions are present
  - Verification:
    - manual context menu sweep with and without selection

- [x] IR-301 Rebuild the file menu into a standard desktop flow.
  - Done when:
    - file menu focuses on project operations first
    - templates are moved out of the core file path
    - menu behavior is reliable and boring
    - menu actions open proper project panels or dialogs where expected
  - Verification:
    - menu open/close/click sweep

- [x] IR-302 Rebuild edit/view/selection menus around real work.
  - Done when:
    - menu groups match actual editing behavior
    - redundant or filler items are removed
  - Verification:
    - menu-item action sweep

## Phase E: Snapping And Motion

- [x] IR-400 Rebuild grid and snap visibility.
  - Done when:
    - major/minor grid hierarchy reads clearly
    - snap lines and snap reasons are visible during interaction
  - Verification:
    - move/resize snap sweep with visible guides

- [x] IR-401 Add scale and rotation snapping.
  - Done when:
    - scale snapping and angle snapping work predictably
    - modifier override exists
  - Verification:
    - scale/rotate snap sweep

- [x] IR-402 Rebuild the timeline into a direct tool.
  - Done when:
    - time ruler, playhead, lanes, property subtracks, keys, and curve view are
      the main interaction
    - key timing/value edits can be dragged directly
    - transport and zoom controls feel like animation tooling, not a form
  - Verification:
    - create track, add keys, drag keys, edit easing, scrub playhead

## Phase F: Panel Cleanup And Visual Pass

- [x] IR-500 Strip filler from panels and rebuild around active work.
  - Done when:
    - Layers, Inspector, Assets/Insert, Timeline, and Code/Logic each have a
      clean job
    - non-working filler controls are gone
  - Verification:
    - panel walkthrough with real authoring tasks

- [x] IR-501 Apply a serious visual pass after usage is fixed.
  - Done when:
    - the editor stops looking like an old hobby tool
    - typography, spacing, chrome, and color hierarchy feel intentional
  - Verification:
    - screenshot baseline at representative sizes

## Phase G: Core Editor Commands

- [x] IR-600 Add undo/redo and destructive edit history.
  - Done when:
    - undo and redo work across create, delete, move, resize, rotate, and panel
      edits
    - delete and duplicate are first-class commands instead of ad hoc actions
  - Verification:
    - command sweep for create/edit/delete/undo/redo

- [x] IR-601 Add real copy/cut/paste and duplicate semantics.
  - Done when:
    - copy, cut, paste, and duplicate work on the active selection
    - pasted content lands predictably and stays editable
    - multi-node copy/paste preserves grouping and order
  - Verification:
    - clipboard sweep on single and multi-selection

- [x] IR-602 Add keyboard-first selection and command baseline.
  - Done when:
    - `Ctrl+A`, `Delete`, `Backspace`, `Esc`, arrow nudge, and modifier-based
      selection behave predictably
    - keyboard commands agree with pointer commands instead of fighting them
  - Verification:
    - keyboard command sweep on a representative scene

## Phase H: Structure And Content Panels

- [x] IR-700 Build a real Layers panel and shape manager.
  - Done when:
    - layer rows support rename, visibility, lock, reorder, and focus
    - hierarchy reads clearly
    - shape insertion and object management are no longer spread across random
      strips and filler panels
  - Verification:
    - layer panel walkthrough with nested scene content

- [x] IR-701 Rebuild the inspector into a real control panel.
  - Done when:
    - transform, appearance, layout, content, and motion sections are clearly
      separated
    - the panel adapts to shape, page, text, image, and multi-selection states
  - Verification:
    - control-panel walkthrough across several selection types

## Phase I: Text, Image, Appearance, And Effects

- [x] IR-800 Add a usable text system.
  - Done when:
    - text can be inserted, edited in place, selected, moved, and styled
    - text objects participate in selection, layers, and export cleanly
  - Verification:
    - create and edit several text objects in one scene

- [x] IR-801 Add image and texture import/manipulation baseline.
  - Done when:
    - images can be imported, placed, replaced, and resized predictably
    - texture/image ownership is visible in the inspector and layer stack
  - Verification:
    - import and manipulate several image-backed objects

- [x] IR-802 Add real appearance, effects, and shader ownership surfaces.
  - Done when:
    - fill, stroke, effects, and shader attachments are visible and editable
    - ownership of those attachments is clear in panels and export metadata
  - Verification:
    - appearance/effects walkthrough on representative objects

## Phase J: Motion And Project Flow

- [x] IR-900 Add procedural motion/effect controllers beyond raw keyframes.
  - Done when:
    - repeat, stagger, loop, or driver-like systems exist for common motion
      patterns
    - those systems compile into the export model without hand-editing generated
      code
  - Verification:
    - build a representative procedural animation scene

- [x] IR-901 Rebuild project/file flows into real desktop behavior.
  - Done when:
    - New, Open, Save, Save As, Recent, Import, and Export follow a standard
      project flow
    - file actions open proper panels or dialogs
    - templates live in insert/start surfaces instead of hijacking File
  - Verification:
    - project flow sweep across new/open/save/export/reopen

## Progress

- Interaction reset tasks total: `27`
- Done: `27`
- In progress: `0`
- Remaining: `0`
- Completion: `100.0%`
