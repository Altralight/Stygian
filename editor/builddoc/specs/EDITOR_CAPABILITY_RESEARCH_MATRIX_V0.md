# Stygian Editor Capability Research Matrix V0

## Why This Exists

The first Figma screenshots were useful, but they only covered the easy front
surface.

That is nowhere near enough.

The current editor is missing a lot of the machinery that makes a creative tool
feel complete:

- history and destructive-edit recovery
- clipboard and paste semantics
- layer and shape management
- text authoring
- image, texture, and import handling
- effects and shader ownership
- proper file and project flows
- timeline, curves, and procedural motion systems
- keyboard-first editing and selection

This matrix expands the research scope so we stop pretending the rebuild is just
selection handles and nicer chrome.

## Status Markers

- `[ ]` not yet researched enough
- `[-]` active capture
- `[x]` baseline sources captured

## Research Tracks

### [x] RM-001 Core Shape Authoring

What to study:

- primitive insertion flow
- drag-place vs click-place behavior
- modifier behavior for centered and constrained placement
- per-corner radius editing
- boolean and vector-network escalation path

Why it matters:

- our editor currently fails at the easiest possible job

Baseline sources:

- Figma shape tools: [Shape tools](https://help.figma.com/hc/en-us/articles/360040450133-Basic-shape-tools-in-Figma-design)
- local screenshot board: `D:\Projects\Code\Stygian\editor\builddoc\references\figma-phase1\index.html`

Stygian translation questions:

- which shapes stay first-class authored nodes instead of falling into generic
  path data too early?
- how do we keep authored primitives lossless until export?

### [x] RM-002 Selection, Transform, And On-Canvas Feedback

What to study:

- single select, multi select, marquee, shift-add/remove
- direct move, scale, rotate
- measurement chips, snap lines, spacing hints
- mixed-value inspector states

Why it matters:

- the user currently does not feel in control of the scene

Baseline sources:

- Figma selection guide: [Select layers and objects](https://help.figma.com/hc/en-us/articles/360040449873-Select-layers-and-objects)
- local screenshots in `D:\Projects\Code\Stygian\editor\builddoc\references\figma-phase1\assets`

Stygian translation questions:

- how do shared bounds stay stable across mixed primitive types?
- which transform affordances belong on canvas vs inspector?

### [x] RM-003 Menus, Context Menus, And Desktop Command Model

What to study:

- file menu anatomy
- edit/view/selection menus
- right-click context actions
- command palette / quick insert interplay

Why it matters:

- the current shell has menu-looking controls without dependable desktop
  behavior

Baseline sources:

- Figma UI shell and workspace navigation:
  [Navigating UI3](https://help.figma.com/hc/en-us/articles/23954856027159-Navigating-UI3)
- Figma quick insert / insert flow: [Create and insert component instances](https://help.figma.com/hc/en-us/articles/360039150173-Create-and-insert-component-instances)
- Figma search and quick actions:
  [Use search to find menu actions](https://help.figma.com/hc/en-us/articles/360040328653-Use-search-to-find-menu-actions)
- local workspace screenshots in `D:\Projects\Code\Stygian\editor\builddoc\references\figma-phase1\assets`

Stygian translation questions:

- which actions belong in a standard menu, in a context menu, and in viewport
  chrome?
- where do project panels open without cluttering the dock stack?

Batch 1 conclusion:

- top menus must handle standard document and view commands
- right-click context should be selection-aware and viewport-local
- insert/quick actions should not replace menus; they should complement them

### [x] RM-004 Layers, Shape Manager, And Scene Hierarchy

What to study:

- layer rows, lock, visibility, rename, reorder
- nested effect ownership
- locate-selection in hierarchy
- filtering and search

Why it matters:

- a real editor needs structure management, not just canvas poking

Baseline sources:

- Figma layer basics: [Layers 101](https://help.figma.com/hc/en-us/articles/26584819173271-Layers-101-Get-started-with-layers)
- Figma layer types:
  [Layers 101: Explore layer types](https://help.figma.com/hc/en-us/articles/26620239826199-Layers-101-Explore-layer-types)
- Figma visibility behavior: [Toggle visibility to hide layers](https://help.figma.com/hc/en-us/articles/360041112614-Toggle-visibility-to-hide-layers)
- Figma file exploration:
  [Explore design files](https://help.figma.com/hc/en-us/articles/15297425105303-Explore-design-files)
- Photoshop layer references in local assets

Stygian translation questions:

- how do scene nodes, authored helpers, guides, and generated-only helper nodes
  differ in the hierarchy?
- how should layer state map to exported code ownership?

Batch 1 conclusion:

- the left rail needs to merge document structure and object management cleanly
- the shape manager should be a real hierarchy surface, not a separate gimmick
- visibility, lock, rename, reorder, and focus must live directly on rows

### [x] RM-005 Inspector And Control Panel Architecture

What to study:

- contextual inspector layouts
- compact numeric editing patterns
- mixed-value presentation
- page-level vs object-level property surfaces

Why it matters:

- the current inspector is still a grab bag of controls instead of a reliable
  work surface

Baseline sources:

- local right-rail screenshots in `D:\Projects\Code\Stygian\editor\builddoc\references\figma-phase1\assets`
- Figma UI shell and right-rail context:
  [Navigating UI3](https://help.figma.com/hc/en-us/articles/23954856027159-Navigating-UI3)
- Figma fill and property behavior: [Add fills to text and shape layers](https://help.figma.com/hc/en-us/articles/360040623954-Add-fills-to-text-and-shape-layers)
- Figma auto layout basics:
  [Toggle on auto layout in designs](https://help.figma.com/hc/en-us/articles/5731482952599-Toggle-auto-layout-on-designs)

Stygian translation questions:

- which sections are always present?
- which sections appear only for shape, page, text, image, or animation
  selections?

Batch 1 conclusion:

- the inspector must be contextual, not a constant wall of controls
- transform and appearance stay near the top because they are edited constantly
- page, shape, text, image, and animation selections need different panel
  compositions

### [x] RM-006 History, Undo/Redo, Clipboard, And Paste Semantics

What to study:

- undo/redo expectations
- duplicate behavior
- copy/paste placement rules
- paste-to-replace and repeat-duplicate behavior
- version history vs local command history

Why it matters:

- without this, the app is not trustworthy enough for real work

Baseline sources:

- Figma copy/paste: [Copy and paste objects](https://help.figma.com/hc/en-us/articles/4409078832791-Copy-and-paste-objects)
- Figma version history: [View a file's version history](https://help.figma.com/hc/en-us/articles/360038006754-View-a-file-s-version-history)

Stygian translation questions:

- what belongs in transient edit history vs durable project checkpoints?
- how do we keep paste stable when the project exports to generated C23 later?

### [x] RM-007 Text System And Typography Editing

What to study:

- text insertion and in-place editing
- typography sections in inspector
- text layer selection, resize, and auto-size modes
- outline/flatten escape hatches

Why it matters:

- even simple UI authoring is crippled without a text system

Baseline sources:

- Figma layer types, including text and images:
  [Layers 101: Explore layer types](https://help.figma.com/hc/en-us/articles/26620239826199-Layers-101-Explore-layer-types)
- Figma text conversion edge case:
  [Convert text to vector paths](https://help.figma.com/hc/en-us/articles/360047239073-Convert-text-to-vector-paths)
- Photoshop text authoring:
  [Add text](https://helpx.adobe.com/photoshop/using/add-edit-text.html)
- Photoshop type layers:
  [Create type in Photoshop](https://helpx.adobe.com/photoshop/using/creating-type.html)

Stygian translation questions:

- how do text metrics and wrapping map into exported layout code?
- what can stay authored declaratively vs requiring custom code?

### [x] RM-008 Images, Textures, Assets, And Import Flow

What to study:

- image import and replacement
- fills using images and gradients
- asset browser patterns
- texture ownership and previewing

Why it matters:

- authored scenes are dead on arrival without image and texture handling

Baseline sources:

- Figma export/import basics: [Export from Figma Design](https://help.figma.com/hc/en-us/articles/360040028114-Export-from-Figma-Design)
- Figma paint/image fills: [Add fills to text and shape layers](https://help.figma.com/hc/en-us/articles/360040623954-Add-fills-to-text-and-shape-layers)

Stygian translation questions:

- when is an image a fill, a texture slot, or a dedicated image node?
- how should imported assets live alongside generated runtime code?

### [x] RM-009 Effects, Filters, Masks, And Shader Ownership

What to study:

- effect stacks
- non-destructive filters
- masks and clipping ownership
- layer-attached vs object-attached effects
- custom shader attachment model

Why it matters:

- Stygian should be stronger than browser design tools here, but only if the
  ownership model stays sane

Baseline sources:

- Photoshop filter overview: [Filters overview](https://helpx.adobe.com/photoshop/desktop/effects-filters/get-started-with-filters/filters-overview.html)
- Photoshop smart filters: [Apply Smart Filters in Photoshop](https://helpx.adobe.com/photoshop/using/applying-smart-filters.html)

Stygian translation questions:

- which effects become authored parameters vs custom shader hooks?
- how are masks represented in scene data and in generated C23?

### [x] RM-010 Timeline, Dope Sheet, And Graph Editor

What to study:

- timeline lane view
- property subtracks
- keyframe editing
- graph editor curves
- transport, markers, and zoom behavior

Why it matters:

- the current timeline is not an animation tool yet

Baseline sources:

- Blender timeline: [Timeline](https://docs.blender.org/manual/en/dev/editors/timeline.html)
- Blender keyframe editing: [Editing Keyframes](https://docs.blender.org/manual/en/5.0/animation/keyframes/editing.html)
- After Effects animation basics: [Animation basics in Adobe After Effects](https://helpx.adobe.com/after-effects/using/animation-basics.html/animation-basics.html)
- After Effects keyframe editing: [Editing, moving, and copying keyframes](https://helpx.adobe.com/after-effects/using/editing-moving-copying-keyframes.html)
- Photoshop tweening: [Create frames using tweening](https://helpx.adobe.com/photoshop/desktop/add-video-and-animation/create-animation-frames/create-frames-using-tweening.html)

Stygian translation questions:

- when do we use lane view vs graph view?
- how should authored curves compile into runtime property drivers?

### [x] RM-011 Procedural Motion And Effect Systems

What to study:

- repeaters, stagger, loops, and driven properties
- expression-like systems vs preset operators
- reusable motion templates

Why it matters:

- users will want more than hand-keyed transforms very quickly

Baseline sources:

- After Effects expressions:
  [Editing expressions](https://helpx.adobe.com/after-effects/using/edit-expressions.html)
- Blender drivers introduction:
  [Introduction](https://docs.blender.org/manual/en/3.6/animation/drivers/introduction.html)
- Blender drivers panel:
  [Drivers Panel](https://docs.blender.org/manual/en/5.0/animation/drivers/drivers_panel.html)
- Blender driver usage:
  [Usage](https://docs.blender.org/manual/en/4.0/animation/drivers/usage.html)

Stygian translation questions:

- what procedural systems can compile directly into generated Stygian code?
- where do we draw the line between declarative behavior and hook code?

Research conclusion:

- the first procedural lane should be native operators, not freeform scripting
- the right starting set is: loop, ping-pong, stagger, repeater offsets, and
  property-to-property drivers with named inputs
- expression systems should come later and stay constrained, because they are a
  trust and performance trap if they become the default authoring path

### [x] RM-012 Layout Systems, Constraints, And Responsive Structure

What to study:

- auto layout
- constraints
- alignment and distribution
- resize behavior for text, images, and frames

Why it matters:

- a visual UI editor without a strong layout model becomes a manual pixel farm

Baseline sources:

- Figma auto layout: [Toggle on auto layout in designs](https://help.figma.com/hc/en-us/articles/5731482952599-Toggle-auto-layout-on-designs)
- Figma auto layout properties:
  [Guide to auto layout](https://help.figma.com/hc/en-us/articles/360040451373-Explore-auto-layout-properties)
- Figma layer exploration: [Explore design files](https://help.figma.com/hc/en-us/articles/15297425105303-Explore-design-files)

Stygian translation questions:

- what authored layout rules compile into explicit C23 layout code cleanly?
- where does SDF-native behavior help instead of complicate things?

Research conclusion:

- layout must stay authored as explicit structure, not inferred from loose pixel
  positions
- frames need a real layout mode with direction, padding, gap, alignment,
  clipping, child sizing, and constraint escape hatches
- responsive behavior should compile into plain generated layout code, not a DSL
  trapped behind the editor

### [x] RM-013 Export, Generated Code Ownership, And Hook Boundaries

What to study:

- generated vs user-owned file boundaries
- code preview UX
- export diff stability
- rebuild/import loops

Why it matters:

- this is the Stygian-specific part that can make the product genuinely better
  than trapped-in-a-DSl editors

Existing local sources:

- `D:\Projects\Code\Stygian\editor\builddoc\plans\TASKLIST_V2.md`
- `D:\Projects\Code\Stygian\editor\builddoc\specs\SELF_HOST_DAILY_FLOW_CONTRACT_V0.md`
- `D:\Projects\Code\Stygian\editor\builddoc\specs\INTERACTION_RESET_REBUILD_V0.md`

Stygian translation questions:

- how do authored editor objects stay editable while exported code stays clean?
- how do we expose generated code without making the UI feel like an IDE first?

Research conclusion:

- generated/runtime code, user hooks, and authored project state must stay three
  visibly different ownership zones
- export diff quality is product trust, not a nice-to-have
- the editor should show ownership and file targets constantly, but code editing
  should stay secondary to visual authoring

### [x] RM-014 Components, Variants, Templates, And Stateful UI Parts

What to study:

- reusable component insertion
- variants and property-driven UI states
- interactive component patterns for tabs, buttons, toggles, and disclosures
- starter templates vs reusable library parts

Why it matters:

- the user keeps asking the right question here: should a tab system be drawn
  once and then expanded by code, or authored as explicit stateful variants?

Baseline sources:

- Figma variants:
  [Create and use variants](https://help.figma.com/hc/en-us/articles/360056440594-Create-and-use-variants)
- Figma interactive components:
  [Create interactive components with variants](https://help.figma.com/hc/en-us/articles/360061175334-Create-interactive-components-with-variants)
- Figma component insertion:
  [Create and insert component instances](https://help.figma.com/hc/en-us/articles/360039150173-Create-and-insert-component-instances)
- Figma component properties:
  [Explore component properties](https://help.figma.com/hc/en-us/articles/5579474826519-Explore-component-properties)

Stygian translation questions:

- which reusable UI parts become authored templates?
- which states stay declarative in the editor?
- when does the exporter duplicate structure vs emit state-driven runtime code?

Research conclusion:

- components need explicit instance override surfaces, not just copy-and-paste
  templates
- variants should model state families like tabs, toggles, and disclosure shells
- component properties are the bridge between visual reuse and generated runtime
  parameters, and they should stay inspectable instead of magical

## Immediate Next Research Pass

The next useful pass is:

1. convert RM-011 through RM-014 into implementation blocks with explicit done
   conditions
2. keep attaching local screenshot evidence to
   `D:\Projects\Code\Stygian\editor\builddoc\references\figma-phase1\index.html`
3. keep shader attachment depth called out separately so we do not pretend the
   current ABI already solves it

## Direction Lock Snapshot

This lane now has enough reference coverage to stop guessing.

Baseline coverage locked for the reset:

- shell and desktop command model: RM-003
- selection and on-canvas transform feedback: RM-002
- layers and object hierarchy: RM-004
- inspector/control panel composition: RM-005
- history / clipboard / paste expectations: RM-006
- text authoring surface: RM-007
- image and texture handling: RM-008
- effects, masks, and shader ownership: RM-009
- timeline / dope sheet / curve editing: RM-010
- procedural systems: RM-011
- layout and responsive structure: RM-012
- export/code ownership depth: RM-013
- components, variants, and stateful parts: RM-014
- local visual references: `D:\Projects\Code\Stygian\editor\builddoc\references\figma-phase1\assets`

What this means:

- the current reset can proceed on a locked direction
- deeper research still continues for later phases
- we do not need to pretend the shell, layers, inspector, or timeline are
  "good enough" while the reference target is still fuzzy
