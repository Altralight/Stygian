# Figma-to-Stygian Editor Research

## Goal

Build an attachable visual editor for Stygian that feels strong in the same places
Figma feels strong, while exporting pure Stygian C23 code instead of a runtime
DSL.

The editor is the authoring surface.
The generated C23 is the runtime product.
The app built with the editor should run the generated Stygian code, not the
editor itself.

## Brutal Reality

This is possible.

This is also a lot bigger than "make a canvas with shapes."

What makes Figma valuable is not the rectangle tool. It is the combination of:

- fast direct manipulation on a big canvas
- responsive layout with frames, constraints, and auto layout
- vector editing that can handle real illustration work
- reusable components, variants, and instance overrides
- styles and variables that keep large files sane
- interaction authoring that stays close to the visual model

Right now, Stygian already has a better runtime ceiling than the current editor
surface suggests. The runtime exposes rectangles, circles, text, textures,
bezier-style wire primitives, rounded corners, gradients, shadows, blur, glow,
blend, and clipping in `D:\Projects\Code\Stygian\include\stygian.h:68`,
`D:\Projects\Code\Stygian\include\stygian.h:74`,
`D:\Projects\Code\Stygian\include\stygian.h:78`,
`D:\Projects\Code\Stygian\include\stygian.h:86`,
`D:\Projects\Code\Stygian\include\stygian.h:345`,
`D:\Projects\Code\Stygian\include\stygian.h:357`,
`D:\Projects\Code\Stygian\include\stygian.h:362`,
`D:\Projects\Code\Stygian\include\stygian.h:369`,
`D:\Projects\Code\Stygian\include\stygian.h:370`,
`D:\Projects\Code\Stygian\include\stygian.h:373`,
`D:\Projects\Code\Stygian\include\stygian.h:397`,
`D:\Projects\Code\Stygian\include\stygian.h:414`,
`D:\Projects\Code\Stygian\include\stygian.h:417`,
`D:\Projects\Code\Stygian\include\stygian.h:425`, and
`D:\Projects\Code\Stygian\include\stygian.h:144`.

The current editor model is nowhere near that broad yet. It still authors only
rectangles, ellipses, and paths in
`D:\Projects\Code\Stygian\editor\include\stygian_editor.h:27` and only generates a
narrow slice of scene code in
`D:\Projects\Code\Stygian\editor\src\stygian_editor.c:2025`,
`D:\Projects\Code\Stygian\editor\src\stygian_editor.c:2154`,
`D:\Projects\Code\Stygian\editor\src\stygian_editor.c:2187`, and
`D:\Projects\Code\Stygian\editor\src\stygian_editor.c:2220`.

So the truth is simple:

- Stygian runtime capability is not the blocker.
- Editor data model depth is the blocker.
- Code generation is feasible, but only if the editor owns a serious authoring
  graph and exports deterministically.
- If we skip the authoring graph and jump straight to ad hoc viewport widgets,
  we will build a toy and then spend months crawling out of that hole.

## What Makes Figma Figma

### 1. Direct manipulation that feels immediate

Figma is fast to operate because the common actions are cheap:

- pan, zoom, frame-fit, and selection are always available
- multi-select, marquee, shift-select, and nested selection are predictable
- transforms are visible and reversible
- snapping, guides, rulers, and pixel alignment reduce manual cleanup
- layers, grouping, locking, and z-order stay understandable

If the editor feels vague while dragging or resizing, it already lost.

### 2. Frames and layout, not just freeform shapes

Figma is not just "draw shapes anywhere." Frames, constraints, and auto layout
are what make UI authoring survive resizing and real screen sizes.

That means Stygian Editor needs:

- frames as first-class containers
- parent-child transforms
- clipping and scroll regions
- constraints against parent bounds
- horizontal, vertical, and eventually grid auto layout
- padding, gap, alignment, wrapping, and min/max sizing
- absolute positioning inside layout containers

Without this, the output will look fine in one viewport and fall apart in any
real app.

### 3. Real vector authoring

Figma can do icons, illustrations, logos, and weird geometry because it has
more than primitive shapes:

- vector networks and point editing
- bezier handles
- open and closed paths
- boolean operations
- masks
- stroke controls
- corner controls
- shape conversion and flattening

If Stygian Editor only handles rectangles, circles, and polyline-ish paths, it
cannot claim "draw just about anything" yet.

### 4. Reuse through components

Figma stops large files from becoming garbage by turning repeated UI into
components, variants, and instance overrides.

That means we need:

- components with local definitions
- instances that stay linked
- variants for state and size families
- component properties for text, visibility, swaps, and enums
- override tracking that survives regeneration

Without components, the first decent-sized app becomes copy-paste soup.

### 5. Styles and variables

Figma works at scale because color, type, effects, spacing, and state can be
named and reused.

That means Stygian Editor needs:

- color styles and tokens
- typography styles
- effect styles
- spacing and size variables
- mode support for themes and states
- variable binding into layout and behavior

Without variables, "frontend that generates backend code" turns into a brittle
pile of hardcoded numbers.

### 6. Interaction authoring that stays visual

Figma prototypes are not production apps, but they matter because designers can
express state, flow, hover, press, drag, scroll, and overlays without dropping
out of the editor.

For Stygian, we should go one step further:

- simple visual interactions for common UI behavior
- generated C23 behavior code for runtime use
- clean extension points for hand-written logic

That is the bridge between visual authoring and real apps.

## Official Figma Capabilities Worth Copying

The sources at the end of this document are official Figma docs. The short
version is:

- Auto layout is a full layout system, not a gimmick.
- Constraints are essential for resizing behavior.
- Vector networks are a real differentiator.
- Boolean groups and masks are non-destructive.
- Fills can stack and can be solid, gradient, pattern, image, or video.
- Effects include shadows, blur, texture, noise, and glass.
- Blend modes can apply at the layer, fill, and effect levels.
- Text is richer than a plain label: area text, point text, styling ranges, text
  on a path, alignment, spacing, and links.
- Components and variants are core, not optional.
- Variables and styles make design systems usable.
- Prototype triggers and actions cover click, drag, hover, press, scrolling,
  variable changes, and transitions.

## What Stygian Can Already Support

Stygian is not a bad backend for this. It is a promising one.

### Runtime strengths already present

- basic primitive scene building
- rounded rectangles
- circles
- text drawing
- image drawing
- SDF wire or curve-oriented primitives
- gradients
- shadows
- blur
- glow
- clipping
- blend control

This gives us a serious base for UI, illustration, and effects.

### Current editor and exporter limits

- no frame node
- no text node
- no image node
- no container hierarchy worth calling layout
- no component or instance system
- no variable or style system
- no boolean or mask authoring
- no multiple fills or strokes
- no rich stroke model
- no rotation-centric authoring model
- no deterministic reusable component emission yet
- no proper reloadable project format for editor-only metadata

The exporter currently proves the pipeline, not the full product.

## Non-Negotiable Architecture

If we want pure Stygian C23 output without painting ourselves into a corner, the
architecture has to be split cleanly.

### 1. Authoring model

The editor keeps a canonical authoring graph in memory.

This graph contains:

- node hierarchy
- transforms
- constraints and auto layout settings
- path and vector data
- fills, strokes, effects, clips, masks, and blend settings
- component definitions and instances
- style and variable bindings
- interaction graph
- editor-only metadata such as selection state, guides, locked handles, and
  uniform-scale preferences

This is the source of truth while editing.

### 2. Generated runtime code

The build step exports pure C23 using real Stygian API calls.

That means generated output should look like this kind of thing:

```c
StygianElement button_bg = stygian_rect(ctx, x, y, w, h, r, g, b, a);
stygian_set_radius(ctx, button_bg, tl, tr, br, bl);
stygian_set_shadow(ctx, button_bg, ox, oy, blur, spread, sr, sg, sb, sa);
stygian_set_gradient(ctx, button_bg, angle, ...);
```

Not this:

```c
run_editor_dsl(json_blob);
```

### 3. Sidecar project data

This is the part people try to skip and regret later.

The runtime app should not consume a DSL.
The editor still needs a persistent project format.

Those are different problems.

A sidecar project file is still required for reloadable editing because pure
runtime calls do not preserve enough authoring intent on their own. You cannot
reliably recover:

- auto layout rules
- component definitions and instance override intent
- variable bindings
- guide data
- editor-only handles and tool state
- non-destructive boolean history
- rich text spans
- shape construction history

So the sane setup is:

- project sidecar for editor reload
- generated C23 for runtime build

The sidecar is not the app runtime format. It is editor persistence.

## Required Feature Inventory

This is the feature list that matters if the goal is "draw just about anything"
and ship real UI from it.

### P0: Foundation We Cannot Skip

| Area | Required |
| --- | --- |
| Canvas | pan, zoom, fit, infinite canvas, smooth input |
| Selection | single, marquee, shift-add, shift-toggle, nested select, lock-aware select |
| Transform | move, rotate, resize, scale, numeric properties, pivot, snapping |
| Structure | layers tree, parent-child hierarchy, grouping, ungroup, reorder, hide, lock |
| History | undo, redo, deterministic commands |
| Persistence | save and load editor project sidecar |
| Export | deterministic C23 scene export with stable IDs and names |
| Embedding | attachable module API for IDE integration |

Without P0, everything else is sand.

### P1: Drawing and Styling Core

| Area | Required |
| --- | --- |
| Primitives | rect, rounded rect, ellipse, line, arrow, polygon, star, arc, frame |
| Paths | open path, closed path, bezier handles, point edit, insert/delete points |
| Paint | solid fill, multiple fills, opacity, stroke, multiple strokes |
| Stroke | width, inside/outside/center, caps, joins, miter, dash |
| Color | RGBA, hex, HSL/HSV view, eyedropper, palette swatches |
| Gradient | linear, radial, angular, diamond or equivalent |
| Image | image fill, image node, crop/fit/fill modes |
| Effects | shadow, inner shadow, blur, glow, texture/noise later |
| Blend | per layer and per paint where backend allows |
| Radius | uniform radius and per-corner radius |
| Clip | clipping and mask basics |

This is the minimum where illustration and UI start to feel real.

### P2: Layout and Responsiveness

| Area | Required |
| --- | --- |
| Frames | root frames, nested frames, clip content, background paint |
| Constraints | left, right, center, top, bottom, scale, stretch |
| Auto layout | horizontal, vertical, padding, gap, alignment, hug/fill/fixed sizing |
| Grid layout | later in this phase, not day one |
| Min/max | per node size constraints |
| Scroll | overflow direction, fixed elements, sticky behavior later |
| Guides | rulers, guides, layout grids or guides, snapping to them |

If the editor cannot express layout, the generated code will be static screenshots
pretending to be UI.

### P3: Components and Design System Features

| Area | Required |
| --- | --- |
| Components | create component, edit main, use instances |
| Instances | linked overrides, reset overrides, detach |
| Variants | size, state, tone, icon-on or off, etc. |
| Component props | text, bool visibility, enum, instance swap |
| Styles | color, text, effect, layout styles |
| Variables | color, number, string, bool, collection, mode |
| Theming | theme mode switch, dark/light or brand modes |

This is where the editor stops being a canvas toy and becomes a system tool.

### P4: Text That Does Not Suck

| Area | Required |
| --- | --- |
| Text nodes | point text, area text, auto-size modes |
| Typography | font family, weight, size, line height, letter spacing, alignment |
| Rich text | mixed styles in a single node |
| Paragraph | spacing, list handling later, wrapping behavior |
| Path text | text on a path |
| Variables | text bound to variables and component props |

If text is weak, UI authoring is weak. No way around it.

### P5: Vector and Shape Power

| Area | Required |
| --- | --- |
| Boolean ops | union, subtract, intersect, exclude |
| Masks | mask groups and editable mask source |
| Flattening | convert complex result to path when needed |
| Shape builder | nice to have after booleans |
| Outline stroke | convert stroke to path later |
| Node ops | join, split, corner mode control, tangent handling |

This is what moves the tool from UI boxes into real icon and illustration work.

### P6: Behavior and App Logic Bridge

| Area | Required |
| --- | --- |
| Triggers | click, press, release, hover enter or leave, drag, scroll, value changed |
| Actions | set property, animate property, navigate, toggle visibility, set variable |
| Animations | duration, delay, easing, spring later |
| State | component state, selection state, hover or press state, variable state |
| Hooks | generated callback stubs for custom code |
| Runtime | generated dispatcher code, not editor-only playback |

This is the part that lets an app use generated code and still behave like an app.

## Feature List We Should Not Fake Yet

These are real Figma features or adjacent features, but they are not first-wave
work for Stygian Editor:

- real-time multiplayer editing
- comments and review workflows
- branching and file merge UX
- plugin ecosystem
- community publishing
- Dev Mode parity
- AI generation features
- video fills on day one
- advanced texture or glass parity on day one

If we fake these too early, we burn time and still fail the core editor job.

## Recommended Stygian Code Generation Contract

The generated output should be boring, stable, and easy to merge.

### Output files

- `scene.generated.h`
- `scene.generated.c`
- `scene.behavior.generated.c`
- optional `scene.tokens.generated.c`

### Optional editor-only file

- `scene.editor.json` or a compact binary equivalent

The runtime uses the generated C files.
The editor uses the sidecar and can regenerate the C files.

### Stable naming rules

- every node gets a persistent numeric ID
- every named node gets a sanitized symbol name
- generation order is deterministic
- instance and component names are stable across rebuilds

If IDs drift every export, LLM merge quality and user trust both fall off a cliff.

### Codegen passes

1. Validate the authoring graph.
2. Resolve variables, styles, and inherited values.
3. Solve layout rules into emitted layout code.
4. Expand or emit components and instances.
5. Emit geometry and visual properties.
6. Emit interaction tables and behavior code.
7. Emit user hook declarations.
8. Emit diagnostics for unsupported features.

### Layout emission rule

Do not flatten everything to absolute pixels unless it is truly fixed.

If a node is constrained or auto-laid out, generate C23 that computes bounds from
parent size and runtime state. That still counts as pure Stygian C23. It is
generated code, not a DSL.

### Behavior emission rule

Common interactions should generate real runtime code:

- button press
- hover state
- slider drag
- toggle switch
- scroll region
- accordion
- tab selection

More complex app logic should bind to user code through generated hooks.

### User-code boundary

Do not invite users or an LLM to edit generated files by hand.

Instead:

- generate stable hook declarations
- keep user logic in hand-authored files
- regenerate visuals and interaction tables freely

That boundary is what makes LLM-assisted merging sane instead of cursed.

## Suggested Runtime Shape Model

The current editor shape model is too thin. We need a richer internal model, even
if multiple node kinds emit down to the same low-level Stygian primitives.

Recommended authoring node kinds:

- frame
- rect
- ellipse
- line
- arrow
- polygon
- star
- arc
- path
- text
- image
- group
- boolean group
- component definition
- component instance
- slot or placeholder for swaps

Recommended visual property model:

- transform
- opacity
- visibility
- clip or mask
- blend mode
- fill stack
- stroke stack
- effect stack
- corner model
- text model
- layout model
- behavior bindings

## Honest Risks

### Risk 1: pretending runtime calls are enough to round-trip editing

They are not.

If we refuse a sidecar, we will lose authoring intent and hate our own exporter.

### Risk 2: treating layout as an optional later feature

That kills UI usefulness fast.

Primitive drawing can ship early, but frame and layout work must begin early in
the architecture, not as a bolt-on.

### Risk 3: building behavior as a fake preview only

If interactions only exist inside the editor preview, then the generated code is
not the product. That breaks the stated goal.

### Risk 4: skipping components and variables too long

That creates copy-paste design files and ugly generated code.

### Risk 5: chasing Figma surface area before nailing determinism

A smaller editor with stable export beats a flashy editor with drifting output.

## Recommended Implementation Order

### Phase 1: Project model and export contract

- define editor sidecar schema
- define stable node ID rules
- define generated file layout
- define user hook boundary
- define frame node and hierarchy

### Phase 2: Real authoring core

- layers tree
- grouping
- transforms
- snapping and guides
- property system
- undo and redo

### Phase 3: Drawing that can cover real work

- primitives
- paths with bezier editing
- fills and strokes
- text nodes
- image nodes
- effects

### Phase 4: Responsive UI authoring

- frames
- constraints
- auto layout
- scroll regions
- clipping

### Phase 5: System authoring

- components
- instances
- variants
- styles
- variables

### Phase 6: Behavior generation

- interaction graph
- runtime dispatcher generation
- generated component state changes
- user hooks

### Phase 7: Power features

- booleans
- masks
- flatten and outline operations
- richer effects
- better shape builders

## Bottom Line

This is more than possible.

But it only works if we accept the real shape of the problem:

- the editor is a serious frontend authoring system
- the export is serious Stygian C23 code
- the sidecar is editor persistence, not app runtime
- layout, components, variables, and behavior are first-class citizens
- determinism matters as much as visuals

If we do that, Stygian Editor can become something Figma cannot be by default:

- native
- backed by a real rendering runtime
- deployable as generated C23
- friendly to both human code and LLM-assisted code merging

If we do not do that, we will build a shiny canvas that dies the moment someone
tries to ship a real app with it.

## Sources

Official Figma documentation used for this research:

- [Guide to auto layout](https://help.figma.com/hc/en-us/articles/360040451373-Guide-to-auto-layout)
- [Use the grid auto layout flow](https://help.figma.com/hc/en-us/articles/31289469907863-Use-the-grid-auto-layout-flow)
- [Apply constraints to define how layers resize](https://help.figma.com/hc/en-us/articles/360039957734-Apply-Constraints-to-define-how-layers-resize)
- [Combine layout guides and constraints](https://help.figma.com/hc/en-us/articles/360039957934-Combine-Layout-Grids-and-Constraints-for-flexible-layouts)
- [Frames in Figma Design](https://help.figma.com/hc/en-us/articles/360041539473-Frames-in-Figma-Design)
- [Add guides to the canvas or frames](https://help.figma.com/hc/en-us/articles/360040449713-Add-Guides-to-the-Canvas-or-a-Frame)
- [Shape tools](https://help.figma.com/hc/en-us/articles/360040450133-Basic-shape-tools-in-Figma-design)
- [Arc tool](https://help.figma.com/hc/en-us/articles/360040450173-Arc-tool-create-arcs-semi-circles-and-rings)
- [Vector networks](https://help.figma.com/hc/en-us/articles/360040450213-Vector-networks)
- [Boolean operations](https://help.figma.com/hc/en-us/articles/360039957534-Boolean-operations)
- [Masks](https://help.figma.com/hc/en-us/articles/360040450253-Masks)
- [Guide to text in Figma Design](https://help.figma.com/hc/en-us/articles/360039956434-Guide-to-text-in-Figma-Design)
- [Guide to fills](https://help.figma.com/hc/en-us/articles/360041003694-Guide-to-fills)
- [Add fills to text and shape layers](https://help.figma.com/hc/en-us/articles/360040623954-Add-fills-to-text-and-shape-layers)
- [Apply effects to layers](https://help.figma.com/hc/en-us/articles/360041488473-Apply-effects-to-layers)
- [Apply blend modes](https://help.figma.com/hc/en-us/articles/360040667874-Use-blend-modes-to-create-unique-effects)
- [Sample colors with the eyedropper tool](https://help.figma.com/hc/en-us/articles/27643269375767-Sample-colors-with-the-eyedropper-tool)
- [Guide to components in Figma](https://help.figma.com/hc/en-us/articles/360038662654)
- [Create and use variants](https://help.figma.com/hc/en-us/articles/360056440594-Create-and-use-variants)
- [Explore component properties](https://help.figma.com/hc/en-us/articles/5579474826519-Explore-component-properties)
- [Styles in Figma Design](https://help.figma.com/hc/en-us/articles/360039238753-Styles-in-Figma-Design)
- [Guide to variables in Figma](https://help.figma.com/hc/en-us/articles/15339657135383-Guide-to-variables-in-Figma)
- [Apply variables to designs](https://help.figma.com/hc/en-us/articles/15343107263511-Apply-variables-to-designs)
- [Guide to prototyping in Figma](https://help.figma.com/hc/en-us/articles/360040314193-Getting-Started-with-Prototyping)
- [Prototype triggers](https://help.figma.com/hc/en-us/articles/360040035834-Prototype-triggers)
- [Prototype actions](https://help.figma.com/hc/en-us/articles/360040035874-Prototype-actions)
- [Prototype animations](https://help.figma.com/hc/en-us/articles/360040522373-Prototype-animations)
- [Prototype scroll and overflow behavior](https://help.figma.com/hc/en-us/articles/360039818734-Advanced-Prototyping-with-Scrolling-Overflow)
- [Scale layers while maintaining proportions](https://help.figma.com/hc/en-us/articles/360040451453-Scale-layers-while-maintaining-proportions)
- [Adjust alignment, rotation, position, and dimensions](https://help.figma.com/hc/en-us/articles/360039956914-Adjust-alignment-rotation-and-position)
