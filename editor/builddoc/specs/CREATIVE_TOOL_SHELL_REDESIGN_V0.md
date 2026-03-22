# Creative Tool Shell Redesign V0

## Why This Exists

The current bootstrap app has real editor capability under it, but the shell
still looks and behaves like a test harness.

That mismatch is the problem now.

We do not need a prettier fake editor.
We need a shell that makes existing authoring systems readable, discoverable,
and trustworthy, while giving us room to grow into a real creative tool.

## Research Snapshot

These references were used to pin down the shell direction before touching the
bootstrap UI:

- Figma UI3 navigation and placement model:
  - [Figma Help: Navigating UI3](https://help.figma.com/hc/en-us/articles/23954856027159-Navigating-UI3)
- Blender editor model:
  - [Blender Manual: Graph Editor](https://docs.blender.org/manual/en/latest/editors/graph_editor/introduction.html)
  - [Blender Manual: Timeline](https://docs.blender.org/manual/en/latest/editors/timeline.html)
- Adobe workspace patterns:
  - [Photoshop Workspace Overview](https://helpx.adobe.com/photoshop/desktop/get-started/learn-the-basics/workspace-overview.html)
  - [Photoshop Layers Panel](https://helpx.adobe.com/photoshop/using/layers-panel.html)
  - [After Effects Workspaces](https://helpx.adobe.com/after-effects/using/workspaces.html)
- Interaction / information architecture references:
  - [Nielsen Norman Group: Progressive Disclosure](https://www.nngroup.com/articles/progressive-disclosure/)
  - [Cooper: About Face](https://www.wiley.com/en-us/About+Face%3A+The+Essentials+of+Interaction+Design%2C+4th+Edition-p-9781118766576)
  - [O'Reilly: Designing Interfaces](https://www.oreilly.com/library/view/designing-interfaces-3rd/9781492051954/)

## Brutal Read Of Those Tools

The good tools do not win because they are dark gray.

They win because:

- the canvas stays primary
- tools are grouped by task, not by implementation detail
- panels are contextual instead of dumping everything at once
- the timeline reads as time and motion, not as a row of form fields
- code or metadata views are attached to selection and action, not floating in a
  vague side room
- menus are boring on purpose and work every time

What we should copy is discipline, not cosmetics.

## Shell Rules

### 1. Canvas-first

The viewport stays dominant.
The rest of the shell exists to support authoring on the canvas, not compete
with it.

### 2. Three interaction bands

- Application bar:
  - file/edit/view/selection/help
  - project status
  - command search / action access
- Authoring strip:
  - place tools
  - primitive picker
  - templates
  - mode toggles
  - ownership/code state badges
- Workspace:
  - left rail for scene/assets/tools
  - center viewport
  - right contextual stack for inspector/logic/code/diagnostics
  - bottom animation workspace

### 3. Context over clutter

Panels should show:

- what is selected
- what matters next
- what file or hook owns the logic
- what can be edited safely

Panels should stop showing:

- duplicate low-value labels
- giant flat lists of unrelated controls
- fake controls that exist only to fill space

### 4. Timeline must read as motion

The animation panel needs four visible ideas:

- transport / current time
- time ruler
- track lanes
- curve or value preview for selected track

If it only looks like text inputs and buttons, it is not doing its job.

### 5. Code view is part of trust

The code view must work in-app:

- generated file readable in a scrollable text view
- hook file readable in a scrollable text view
- ownership visible in the panel
- quick open-to-external-editor path still available

### 6. Templates must feel like scaffolds, not hidden cheats

Template actions should be:

- visible
- named clearly
- placed near the main authoring controls
- predictable about where they drop content

### 7. Hit targets must stop fighting the user

Any redesign pass must include a control reliability sweep:

- no overlapping clickable regions
- no tabs sitting on top of content
- no labels visually clipped by fixed geometry
- no controls that look active but ignore input

## Visual Direction

Do not copy the current washed-out gray bars.

Target direction:

- darker neutral base with cooler steel-blue accents
- clear surface hierarchy between app chrome, dock chrome, and content
- less raw flat gray, more separation through value and edge treatment
- restrained accent usage for selection, playback, active tabs, and code state
- compact typography with tighter spacing, closer to pro creative tools than toy
  demo apps

## First Implementation Pass

### Global shell

- rebuild the menu strip into one merged application bar
- rebuild the authoring strip with grouped tool clusters
- expose project, mode, and command state without eating the whole row

### Left rail

- make scene/layers read like a tree and selection summary, not button spam
- keep assets honest; if it is still limited, say so cleanly
- add useful quick context like selection count and parent focus

### Right rail

- inspector gets grouped cards
- logic panel gets ownership, hook, and behavior visibility
- code insight becomes an actual embedded source viewer
- diagnostics remains a narrow support surface, not the main attraction

### Timeline

- add ruler + lane treatment
- add clear selected-track emphasis
- show clip occupancy across time
- keep add/update/delete operations, but place them under visual context

## Acceptance For This Redesign Lane

- bootstrap looks like a competent editor shell instead of a debug harness
- file menu is clear, grouped, and works reliably
- key top-strip actions are obvious and clickable
- inspector, logic, and code panels carry useful information
- embedded code view works without leaving the app
- timeline reads as animation authoring, even if bezier tangent editing stays a
  later pass
- bootstrap still builds and launches

## Explicit Non-Goals For V0

- full Blender-grade curve tangent editing
- full Photoshop-grade asset browser
- final taste-level art direction
- complete design-system polish

This pass is about professional structure and believable workflow, not the last
word on style.
