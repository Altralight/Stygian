# stygian_cmd.h Reference

This page documents the command-buffer API in `include/stygian_cmd.h`.

## Purpose

Command buffers let multiple producers stage retained mutations without writing directly into the live SoA state. The runtime applies them during the deterministic commit boundary.

## Buffer Lifecycle

```c
StygianCmdBuffer *stygian_cmd_begin(StygianContext *ctx, uint32_t source_tag);
void stygian_cmd_discard(StygianCmdBuffer *buffer);
bool stygian_cmd_submit(StygianContext *ctx, StygianCmdBuffer *buffer);
```

`stygian_cmd_begin`

- Starts a command buffer for one producer.
- `source_tag` is carried into runtime accounting and diagnostics.
- Returns `NULL` on allocation or state failure.

`stygian_cmd_discard`

- Drops a buffer without committing it.
- Safe way to abandon a producer path after partial emission.

`stygian_cmd_submit`

- Freezes and submits the buffer to the runtime queue.
- Returns `false` if submission is rejected.
- Notes:
  - Submitted buffers must not be mutated afterward.
  - Submit during an invalid runtime state is rejected rather than silently applied.

## Property Emitters

```c
bool stygian_cmd_set_bounds(StygianCmdBuffer *buffer, StygianElement element,
                            float x, float y, float w, float h);
bool stygian_cmd_set_color(StygianCmdBuffer *buffer, StygianElement element,
                           float r, float g, float b, float a);
bool stygian_cmd_set_border(StygianCmdBuffer *buffer, StygianElement element,
                            float r, float g, float b, float a);
bool stygian_cmd_set_radius(StygianCmdBuffer *buffer, StygianElement element,
                            float tl, float tr, float br, float bl);
bool stygian_cmd_set_type(StygianCmdBuffer *buffer, StygianElement element,
                          StygianType type);
bool stygian_cmd_set_visible(StygianCmdBuffer *buffer, StygianElement element,
                             bool visible);
bool stygian_cmd_set_z(StygianCmdBuffer *buffer, StygianElement element,
                       float z);
bool stygian_cmd_set_texture(StygianCmdBuffer *buffer, StygianElement element,
                             StygianTexture texture, float u0, float v0,
                             float u1, float v1);
bool stygian_cmd_set_shadow(StygianCmdBuffer *buffer, StygianElement element,
                            float offset_x, float offset_y, float blur,
                            float spread, float r, float g, float b, float a);
bool stygian_cmd_set_gradient(StygianCmdBuffer *buffer, StygianElement element,
                              float angle, float r1, float g1, float b1,
                              float a1, float r2, float g2, float b2,
                              float a2);
bool stygian_cmd_set_hover(StygianCmdBuffer *buffer, StygianElement element,
                           float hover);
bool stygian_cmd_set_blend(StygianCmdBuffer *buffer, StygianElement element,
                           float blend);
bool stygian_cmd_set_blur(StygianCmdBuffer *buffer, StygianElement element,
                          float blur_radius);
bool stygian_cmd_set_glow(StygianCmdBuffer *buffer, StygianElement element,
                          float intensity);
```

All setters:

- Return `true` when the command was recorded.
- Return `false` when the buffer is invalid, full, or the element/property write is rejected.
- Preserve runtime safety: invalid handles are rejected instead of corrupting SoA state.

## Notes

- `StygianCmdPropertyId` exists so the runtime can merge and resolve property conflicts deterministically.
- Queue overflow and invalid writes are runtime errors, but they are handled as recoverable misuse rather than process-aborting faults.
- The command-buffer path is the right place for producer-thread work. Direct SoA mutation should stay on the core commit path.
