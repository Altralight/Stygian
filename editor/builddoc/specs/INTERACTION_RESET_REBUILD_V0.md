# Interaction Reset Rebuild V0

## Why This Exists

The current bootstrap editor is not failing because it lacks polish.
It is failing because the interaction model is wrong.

We spent too much effort making the shell look more "editor-like" while the
actual usage stayed prototype-grade.

That was the wrong order.

This document resets the work around one rule:

The editor must become usable before it becomes stylish.

## Brutal Read Of The Current State

The user is right.

The current bootstrap editor still has these problems:

- selection feels unstable and hard to predict
- multi-select exists, but does not feel like one coherent transform group
- clicking is unreliable because too many actions depend on tiny timing or odd
  mode state
- placement workflow is confused and too button-heavy
- file menu is not trustworthy enough to feel native
- there is no proper context menu workflow
- timeline is form-heavy instead of direct
- snapping is not readable or direct enough
- rotation exists in code paths, but not as a first-class canvas interaction
- panels still carry too much filler and not enough direct utility
- the visual language is still closer to a dev harness than a serious creative
  tool
- there is no real undo/redo and destructive-edit recovery path
- there is no trustworthy clipboard and paste model
- there is no real layers panel or shape manager
- there is no text system worth calling a text system
- there is no image/texture workflow worth using
- there is no real effects or shader ownership model yet
- file and project flows still do not feel like a native desktop tool

So the real problem is not "make it prettier."

The real problem is:

The user does not feel in control.

## What Must Change

### 1. Replace the current tool model

Right now the editor behaves like a stack of toggles.

That needs to become an explicit mode system with a small number of real tools:

- Select
- Frame / Rectangle
- Ellipse
- Path / Pen
- Pan
- Zoom

Done right, each tool has:

- one obvious cursor behavior
- one obvious click behavior
- one obvious drag behavior
- one obvious cancel path

No hidden "am I placing or selecting or finishing points" ambiguity.

### 2. Make viewport interaction primary

The viewport should do most of the real work:

- click to select
- shift-click to add/remove from selection
- drag selected items to move
- drag blank space to marquee select
- drag handles to resize
- rotate through visible canvas controls, not hidden side-panel-only controls
- right-click for context actions

Panels should support the viewport, not explain away its confusion.

### 3. Rebuild selection around a single mental model

Selection must behave like one thing:

- one item selected: direct manipulation
- many items selected: shared bounds + shared move/scale/rotate
- panel values reflect mixed/shared state honestly
- commands apply to the active selection as a unit

If multi-select only "looks selected" but acts per-node, it is broken.

### 4. Add a real context menu

The editor needs a viewport context menu with fast, obvious actions:

- Cut / Copy / Paste
- Duplicate
- Delete
- Group / Ungroup
- Bring Forward / Send Back
- Frame Selection / Reset Zoom
- Add primitive at cursor
- Convert selection actions where valid

This is not optional. Creative tools live or die by right-click flow.

### 5. Make placement direct

Primitive placement must stop being a confusing strip of mode buttons.

Preferred behavior:

- choose a tool once
- click or drag in the viewport to place
- see immediate preview
- finish with predictable behavior

Point/path creation must show live feedback while drawing, not after a later
click or state change.

### 6. Rebuild the timeline around direct manipulation

The timeline must read like time, not like a spreadsheet.

Needed:

- clear playhead
- visible tracks
- visible keyframes
- drag keys in time
- drag value handles in a curve view
- clips read as layered spans
- current selection reflected immediately

The current form-heavy track/key editing should become a support layer, not the
main experience.

### 7. Rebuild snapping so the user can trust it

Snapping must answer three questions clearly:

- what am I snapping to
- why did it snap there
- how do I temporarily override it

Need:

- visible snap lines
- clear grid hierarchy
- consistent move/scale/rotate snap behavior
- modifier override
- sane defaults

### 8. File menu must become boring and reliable

That is a compliment.

The file menu should feel standard:

- New
- Open
- Save
- Save As
- Export
- Recent
- Quit

No strange template-heavy first impression.
Templates belong in a starter or insert workflow, not as the identity of File.

### 9. Panel architecture must be rebuilt around real work

The panel set should help the active task:

- Layers: scene hierarchy and object visibility/lock/order
- Inspector: transform, layout, appearance, behavior, component data
- Assets/Insert: templates, primitives, assets
- Timeline: motion
- Code/Logic: only where ownership and generated/hook mapping matters

No panel should exist just to prove a subsystem exists.

### 10. Missing foundation systems need first-class scope

The reset is not done once selection feels less bad.

We also need:

- history, undo, redo, delete, duplicate
- copy, cut, paste, and predictable paste placement
- a real layers panel and shape manager
- a text tool with direct editing
- image, texture, and asset handling
- effects and shader ownership surfaces
- proper file/project panels and reliable desktop command flow
- keyboard-first selection and command paths

That is not optional scope creep.

That is the difference between a shell demo and an editor.

## New Product Rules

### Rule A

Every major interaction must have a viewport-first path.

### Rule B

Every tool must have visible state and a predictable cancel path.

### Rule C

Every menu must open above surrounding panels and respect clipping.

### Rule D

Every selection operation must be testable on:

- single node
- multi node
- mixed node kinds

### Rule E

Visual redesign is allowed only when it makes interaction clearer.

## Frozen Interaction Rules

These are the rules the bootstrap editor should now obey until a later spec
revision says otherwise.

### Tool Rules

- `V` Select
- `R` Rect / Frame
- `O` Ellipse
- `P` Path
- `H` Pan
- `Z` Zoom
- primitive choice selects the tool first, then the viewport placement action
- rect and ellipse place by click or drag and fall back to Select after commit
- path stays in path mode until commit or cancel

### Cancel Rules

- `Esc` exits the current non-select tool when that makes sense
- `Esc` cancels active path drawing
- releasing a placement drag commits the shape and leaves the viewport in a
  known state
- right-click is reserved for the viewport context path, not path completion

### Viewport Rules

- blank drag does marquee selection
- shift-click adds or removes from the active selection
- selection transforms operate on shared bounds, not fake per-node visuals
- rotation is a first-class canvas action
- right-click opens viewport-local actions

### Snap Rules

- move, resize, and rotate all expose visible snap feedback
- `Alt` temporarily bypasses snapping for resize and rotate
- grid hierarchy must stay readable before any extra chrome is added

### Timeline Rules

- the lane and curve surface are the main editing path
- playhead scrub is direct
- keyframe time edits are direct
- curve value edits are direct
- forms support the interaction, not the other way around

### Panel Rules

- Layers owns hierarchy, focus, visibility, lock, and reorder
- Insert owns scaffolds and project/export entry points
- Inspector owns transform, appearance, primitive rules, and ownership notes
- Timeline owns direct motion editing
- Code owns read-only generated / hook visibility and ownership context
- Diagnostics owns export trouble and binding health

### Menu Rules

- top menus stay boring and desktop-like
- templates do not hijack File
- selection-aware commands live in the viewport context menu and Selection menu
- code ownership actions stay discoverable from Help / Code / Diagnostics, not
  hidden in random panels

## Current Phase Lock

Completed in this reset:

1. tool model reset
2. selection / transform reset
3. context and menu baseline
4. snapping and timeline direct manipulation
5. panel cleanup and first serious visual pass
6. real layers and control-panel depth
7. text system
8. image / texture / asset handling
9. appearance / effects ownership with explicit export surfaces
10. procedural motion presets
11. desktop-style project flow with Save As, Recent, and import entry points

Still open after this lock:

1. deeper shader attachment editing once the editor ABI exposes a real shader-slot surface
2. richer procedural systems beyond the first preset lane
3. a second-pass visual redesign once the bootstrap surface is replaced by the real self-hosted shell

## Implementation Order

This order matters.

1. input/tool model reset
2. selection/transform reset
3. history, delete, clipboard, and keyboard command baseline
4. context menu + viewport actions
5. file/edit/view menu cleanup and real project panels
6. snapping + rotate workflow
7. timeline direct-manipulation rewrite
8. layers/control-panel simplification
9. text, image, effects, and shader systems
10. visual pass once the usage is no longer embarrassing

## What We Are Not Doing

We are not doing another fake "professional shell" pass while the editor still
fights the user.

We are not treating side-panel controls as a substitute for direct canvas
interaction.

We are not calling a feature done because the code path technically exists.

## Acceptance Standard

This rebuild is only succeeding when:

- the user can select, move, scale, rotate, and multi-edit without confusion
- the user can place primitives and paths without mode ambiguity
- the user can undo, redo, delete, copy, and paste without distrust
- right-click context actions feel natural
- the file menu feels standard and safe
- the timeline reads as animation tooling, not a debug form
- snapping helps instead of surprising
- layers, text, images, and appearance systems feel like real editor surfaces
- the app finally looks like a competent modern native tool instead of a retro
  experiment with dock tabs
