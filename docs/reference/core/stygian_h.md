# stygian.h Reference

This page documents the public runtime surface in `include/stygian.h`.

## Lifecycle

```c
StygianContext *stygian_create(const StygianConfig *config);
void stygian_destroy(StygianContext *ctx);
```

`stygian_create`

- Creates the runtime context, backend access point, internal SoA state, scope cache, and scheduler state.
- `config->window` is required.
- Returns `NULL` on failure.
- Notes:
  - Backend selection comes from `config->backend`.
  - The returned context owns runtime-side allocations and backend objects.

`stygian_destroy`

- Releases context-owned runtime and backend resources.
- Accepts `NULL`.

## Frame Control

```c
void stygian_begin_frame(StygianContext *ctx, int width, int height);
void stygian_begin_frame_intent(StygianContext *ctx, int width, int height,
                                StygianFrameIntent intent);
void stygian_end_frame(StygianContext *ctx);
```

`stygian_begin_frame`

- Starts a render-capable frame using the normal render intent.
- `width` and `height` should match the current framebuffer or window size.

`stygian_begin_frame_intent`

- Starts a frame with explicit render intent.
- Parameters:
  - `intent`: `STYGIAN_FRAME_RENDER` or `STYGIAN_FRAME_EVAL_ONLY`
- Notes:
  - Eval-only frames run logic and stats updates without backend submit or swap.

`stygian_end_frame`

- Finalizes the frame.
- In a render frame, this is where upload, draw, and present/submit complete.
- In an eval-only frame, it closes the frame without GPU work.

## Scope Invalidation

```c
void stygian_scope_begin(StygianContext *ctx, StygianScopeId id);
void stygian_scope_end(StygianContext *ctx);
void stygian_scope_invalidate_now(StygianContext *ctx, StygianScopeId id);
void stygian_scope_invalidate_next(StygianContext *ctx, StygianScopeId id);
```

`stygian_scope_begin` / `stygian_scope_end`

- Mark the retained scope currently being built.
- Scope replay uses these boundaries to decide whether content can be skipped or reused.

`stygian_scope_invalidate_now`

- Marks a scope dirty for the current frame.
- Use this when the frame already needs to rebuild the scope immediately.

`stygian_scope_invalidate_next`

- Schedules a scope to rebuild on the next frame boundary.
- This is the safer default when state changed after the current scope already emitted content.

## Element Allocation and Lifetime

```c
StygianElement stygian_element(StygianContext *ctx);
uint32_t stygian_element_batch(StygianContext *ctx, uint32_t count,
                               StygianElement *out_ids);
void stygian_element_free(StygianContext *ctx, StygianElement e);
```

`stygian_element`

- Allocates one retained element handle.
- Returns an invalid handle on exhaustion or invalid context.

`stygian_element_batch`

- Allocates up to `count` handles into `out_ids`.
- Returns the number of handles actually allocated.

`stygian_element_free`

- Frees a previously allocated element handle.
- Stale or invalid handles are ignored or rejected by the safety path rather than crashing the process.

## Core Property Setters

```c
void stygian_set_bounds(StygianContext *ctx, StygianElement e, float x, float y,
                        float w, float h);
void stygian_set_color(StygianContext *ctx, StygianElement e, float r, float g,
                       float b, float a);
void stygian_set_radius(StygianContext *ctx, StygianElement e, float tl,
                        float tr, float br, float bl);
void stygian_set_shadow(StygianContext *ctx, StygianElement e, float offset_x,
                        float offset_y, float blur, float spread, float r,
                        float g, float b, float a);
void stygian_set_gradient(StygianContext *ctx, StygianElement e, float angle,
                          float r1, float g1, float b1, float a1, float r2,
                          float g2, float b2, float a2);
```

These setters mutate the retained SoA model for one element.

- `stygian_set_bounds`: position and size
- `stygian_set_color`: base RGBA fill color
- `stygian_set_radius`: per-corner rounded rectangle radii
- `stygian_set_shadow`: shadow offset, blur, spread, and color
- `stygian_set_gradient`: angle and endpoint RGBA colors

Notes:

- Invalid handles are rejected by runtime safety checks.
- Mutation should only happen inside a valid frame lifecycle.

## Clip Stack

```c
uint8_t stygian_clip_push(StygianContext *ctx, float x, float y, float w,
                          float h);
void stygian_clip_pop(StygianContext *ctx);
```

`stygian_clip_push`

- Pushes a clip rect and returns the clip id assigned to that stack entry.
- Returns a bounded clip id suitable for later element binding.

`stygian_clip_pop`

- Pops the most recent clip stack entry.
- Notes:
  - Push/pop discipline must stay balanced within the frame.

## Text and Fonts

```c
StygianFont stygian_font_load(StygianContext *ctx, const char *atlas_png,
                              const char *atlas_json);
StygianElement stygian_text(StygianContext *ctx, StygianFont font,
                            const char *str, float x, float y, float size,
                            float r, float g, float b, float a);
```

`stygian_font_load`

- Loads a font atlas and metadata into the runtime.
- Returns an invalid font handle on failure.

`stygian_text`

- Emits a retained text run using the provided font.
- Returns the first element handle of the generated run.
- Notes:
  - Invalid or stale font handles are rejected by safety checks.

## Repaint Scheduler

```c
void stygian_request_repaint_after_ms(StygianContext *ctx, uint32_t ms);
uint32_t stygian_next_repaint_wait_ms(const StygianContext *ctx,
                                      uint32_t idle_wait_ms);
void stygian_set_repaint_source(StygianContext *ctx, const char *source);
uint32_t stygian_get_repaint_reason_flags(const StygianContext *ctx);
```

`stygian_request_repaint_after_ms`

- Schedules repaint work after the requested delay.
- `ms == 0` requests immediate repaint on the next frame opportunity.

`stygian_next_repaint_wait_ms`

- Returns the recommended deep-wait timeout for event-driven loops.
- Use this to avoid busy-spin during idle.

`stygian_set_repaint_source`

- Tags the current repaint request with a human-readable source string for diagnostics.

`stygian_get_repaint_reason_flags`

- Returns the reason flags recorded for the last frame.
- Useful for perf and debugging overlays.

## Notes

- The DDI contract is documented in `docs/governance/dod_ddi_nonnegotiables.md`.
- The runtime model is documented in `docs/architecture/runtime_model.md`.
- `stygian.h` also includes texture APIs, convenience draw helpers, triad/glyph controls, metrics, and error-channel APIs that should be expanded in later reference passes.
