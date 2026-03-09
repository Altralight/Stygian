# Stygian Editor Module Integration

## Goal

Embed a visual editor into an IDE while preserving exact Stygian C23 output.

## Module Boundary

- Host IDE owns windowing, file system hooks, and panel embedding.
- `StygianEditor` owns visual/behavior authoring state.
- Generated code is exported by `stygian_editor_build_c23(...)`.
- The attachable module ABI is exported by `stygian_editor_module_get_api(...)`.

## Current ABI Surface

Public header:

- `D:\Projects\Code\Stygian\include\stygian_editor.h`
- `D:\Projects\Code\Stygian\include\stygian_editor_module.h`

Implementation:

- `D:\Projects\Code\Stygian\editor\stygian_editor.c`
- `D:\Projects\Code\Stygian\editor\stygian_editor_module.c`

## Module ABI (Attachable Host Contract)

Entry points:

- `stygian_editor_module_abi_version()`
- `stygian_editor_module_get_info(...)`
- `stygian_editor_module_get_api(requested_abi_version)`

Compatibility rule:

- Host must request the same ABI major version.
- Host can request an older/equal minor ABI version.
- If incompatible, `stygian_editor_module_get_api(...)` returns `NULL`.

The returned `StygianEditorModuleApi` table provides stable function pointers
for editor creation, viewport/grid ops, scene edits, behavior wiring, preview
rendering, and deterministic C23 generation.

## Host Boot Example (C)

```c
#include "stygian_editor_module.h"

const StygianEditorModuleApi *api =
    stygian_editor_module_get_api(STYGIAN_EDITOR_MODULE_ABI_VERSION);
if (!api) {
    return false; /* incompatible module */
}

StygianEditorConfig cfg = api->config_default();
StygianEditor *editor = api->create(&cfg, &host_callbacks);
if (!editor) {
    return false;
}
```

## First-Slice Capabilities

- 2D viewport transform helpers (pan/zoom world-view conversion).
- Adaptive grid with major/sub levels and snapping.
- Scene model:
  - `rect`
  - `ellipse`
  - point-authored `path`
- Selection and shape mutation APIs.
- Behavior rules for event-driven animation:
  - `trigger -> animate(property)`
- Preview rendering with Stygian primitives.
- Deterministic C23 build/export:
  - scene builder (`stygian_set_*` calls)
  - path draw calls (`stygian_line`)
  - behavior rule table export

## Recommended Host Loop

1. Feed input to your own editor tools.
2. Convert pointer to world space with `stygian_editor_view_to_world`.
3. Apply mutations on editor model (`stygian_editor_node_set_*`, path APIs).
4. Trigger behavior events with `stygian_editor_trigger_event`.
5. Tick animations with `stygian_editor_tick`.
6. Draw viewport with `stygian_editor_render_viewport2d`.
7. Export C23 using `stygian_editor_build_c23` on build.

## Practical Note

This is intentionally modular and non-invasive: no changes to Stygian core
runtime were required for this phase.
