# stygian_ap.h Reference

This page documents the backend access point contract in `backends/stygian_ap.h`.

## Purpose

The AP layer is the only part of Stygian that talks directly to GPU APIs. Core runtime, layout, widgets, and command processing stay above this boundary.

## Core Types

- `StygianAP`
- `StygianAPSurface`
- `StygianAPConfig`
- `StygianAPType`
- `StygianAPAdapterClass`

## Lifecycle

```c
StygianAP *stygian_ap_create(const StygianAPConfig *config);
void stygian_ap_destroy(StygianAP *ap);
```

`stygian_ap_create`

- Creates the backend device/context path for the configured API.
- `config->window` is required for the main surface.
- Returns `NULL` on backend setup failure.

`stygian_ap_destroy`

- Releases backend-owned GPU resources and contexts.

## Frame Submission

```c
void stygian_ap_begin_frame(StygianAP *ap, int width, int height);
void stygian_ap_submit_soa(StygianAP *ap, const StygianSoAHot *hot,
                           const StygianSoAAppearance *appearance,
                           const StygianSoAEffects *effects,
                           uint32_t element_count,
                           const StygianBufferChunk *chunks,
                           uint32_t chunk_count, uint32_t chunk_size);
void stygian_ap_draw(StygianAP *ap);
void stygian_ap_end_frame(StygianAP *ap);
void stygian_ap_swap(StygianAP *ap);
```

These functions drive the backend frame:

- begin frame and viewport setup
- upload dirty SoA ranges
- draw issued batch
- finalize frame
- present the main surface

## Multi-Surface

```c
StygianAPSurface *stygian_ap_surface_create(StygianAP *ap,
                                            StygianWindow *window);
void stygian_ap_surface_destroy(StygianAP *ap, StygianAPSurface *surface);
void stygian_ap_surface_begin(StygianAP *ap, StygianAPSurface *surface,
                              int width, int height);
void stygian_ap_surface_submit(StygianAP *ap, StygianAPSurface *surface,
                               const StygianSoAHot *soa_hot, uint32_t count);
void stygian_ap_surface_end(StygianAP *ap, StygianAPSurface *surface);
void stygian_ap_surface_swap(StygianAP *ap, StygianAPSurface *surface);
```

These APIs support additional render surfaces such as floating windows or extra viewports sharing the same backend/device path.

## Textures and Output State

Important APIs:

- `stygian_ap_texture_create`
- `stygian_ap_texture_update`
- `stygian_ap_texture_destroy`
- `stygian_ap_set_font_texture`
- `stygian_ap_set_output_color_transform`
- `stygian_ap_set_clips`

These cover texture residency, font atlas binding, output transform state, and clip-region upload.

## Metrics

Important queries:

- `stygian_ap_get_adapter_class`
- `stygian_ap_get_last_upload_bytes`
- `stygian_ap_get_last_upload_ranges`
- `stygian_ap_get_last_gpu_ms`

These are the bridge between backend work and runtime diagnostics.
