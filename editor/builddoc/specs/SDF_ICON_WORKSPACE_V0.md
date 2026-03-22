# SDF Icon Workspace V0

## Purpose

The icon workspace exists so we stop drawing editor icons ad hoc inside app files.

It is an internal authoring surface for:

- silhouette tuning
- stroke weight consistency
- size/readability checks
- hover and active state preview
- copyable C call snippets

## Placement

Runtime pieces:

- app: `D:\Projects\Code\Stygian\editor\apps\stygian_editor_icon_lab.c`
- shared icon source: `D:\Projects\Code\Stygian\editor\src\stygian_editor_icons.c`
- shared icon API: `D:\Projects\Code\Stygian\editor\include\stygian_editor_icons.h`

Build entry:

- `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_icon_lab.bat`

## How It Should Be Used

1. pick an icon from the list
2. inspect it large, then at smaller sizes
3. check idle / hover / active read
4. adjust shared icon geometry in the shared icon source
5. copy a call snippet when needed

The lab is not the final icon editor dream. It is the smallest tool that keeps the editor's icon language from turning into a pile of unrelated doodles.

## Export Rule

By default, the icon workspace does not export project assets.

Icons live as shared C drawing code. That keeps them:

- deterministic
- tiny
- native to Stygian
- reusable across shell, panels, menus, and labs

## Copy/Paste Direction

Short term:

- right click in preview or press the copy control to push a C call snippet to the clipboard

Medium term:

- allow copying icon recipe blocks or shape fragments

Later, if we want:

- paste shared icon recipes into editor UI surfaces
- optionally generate app-local icon helper code from the same source

What we should not do right now:

- build SVG import
- treat icons as random bitmap assets
- fork icon geometry separately per panel
