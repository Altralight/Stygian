# Stygian UI Optimization Research And Stress-Test Plan

## Purpose

This document records the optimization and stress-test work needed to turn Stygian's architecture into publishable performance results.

The goals are:

- identify where persistent scene/state, SoA, and DDI actually win
- identify where fragment cost, upload bandwidth, and text layout dominate the frame
- build a benchmark plan that proves Stygian's real advantages against other UI stacks
- turn Stygian text into a serious renderer instead of a pretty demo path

## Executive Summary

The short version:

- Stygian's invalidation/replay model with persistent scene/state backing is a real advantage, but it helps most on static and sparse-dirty scenes, not on fully hot scenes where every element mutates every frame.
- A single draw call is not the same thing as cheap rendering. Fill rate, overdraw, blend cost, `discard`, and expensive fragment math still matter.
- MTSDF is the right scalable text path for zoomable UI, but it is not automatically the cheapest path for ordinary body text. A serious UI library usually needs multiple text paths.
- "100,000 elements at 500 FPS" is only a reasonable headline if the workload is defined clearly. It is plausible for tiny, low-overdraw, mostly opaque scenes with persistent state. It is not plausible for "100,000 pretty nodes with labels, wires, and full-screen blending" on an old iGPU.
- Serious text means shaping, grapheme segmentation, line breaking, caret mapping, visible-line caching, and dirty-span updates. Pretty glyph sampling is only one piece of the job.

## Current Stygian Findings

These are the important realities from the current codebase and recent local profiling:

- The current node showcase is not a clean "64 nodes" test. It is a fragment-heavy scene with:
  - a background grid that can emit up to 400 rects per frame in [widgets/stygian_widgets.c](D:/Projects/Code/Stygian/widgets/stygian_widgets.c)
  - around 88 smooth cubic wires in [examples/node_graph_demo.c](D:/Projects/Code/Stygian/examples/node_graph_demo.c)
  - padded wire bounds in [src/stygian.c](D:/Projects/Code/Stygian/src/stygian.c), where `stygian_wire()` uses `pad = thickness + 32.0f`
  - rounded node bodies, headers, text, blending, and clip discard
- The current graph demo is therefore a "fragment stress pretty scene," not a headline benchmark for raw replay-driven throughput.
- Stygian's text widget was wasting work by calling `stygian_text()` once per visible character. That has now been improved to line-span emission in [widgets/stygian_widgets.c](D:/Projects/Code/Stygian/widgets/stygian_widgets.c), but the engine still expands text into one glyph quad per glyph in [src/stygian.c](D:/Projects/Code/Stygian/src/stygian.c).
- That means the editor path is better than it was, but it is still not a serious large-document renderer.
- The current GL submit path uses dirty-range `glBufferSubData` uploads per SoA chunk in [backends/stygian_ap_gl.c](D:/Projects/Code/Stygian/backends/stygian_ap_gl.c). This is solid for sparse dirty updates, but it is still a streaming path and not the endgame for fully hot cases.
- Stygian's three SoA buffers currently cost `48 + 64 + 96 = 208 bytes` per element if hot, appearance, and effects all change. That means a worst-case full-hot `100,000`-element frame is about `20.8 MB` of upload before shading cost.
- At `500 FPS`, that worst case would imply about `10.4 GB/s` of upload bandwidth just for SoA payloads. That is not a realistic full-hot target for an old shared-memory iGPU.

## Measured Baseline On The Current Windows Test Box

Machine:

- CPU: `Intel(R) Core(TM) i7-4710HQ CPU @ 2.50GHz`
- GPU: `Intel(R) HD Graphics 4600`
- OS: `Microsoft Windows 11 Pro 10.0.26100`
- Driver: `20.19.15.4624`

Important correction:

- Earlier local benchmark numbers for the node and text showcase were too low because the measurement path was inaccurate.
- Two issues were responsible:
  - the Windows-side wall clock in the benchmark apps was using `timespec_get`, which was too coarse for this max-throughput path on this machine
  - raw mode was still paying GL timer-query overhead, which distorted the path being measured
- After switching the benchmark clocks to `QueryPerformanceCounter`, disabling GPU timer queries in raw mode, and routing no-present GL frames offscreen, the measured throughput increased and the numbers lined up with the actual renderer path.

Current raw/no-present results:

| Scene | Wall FPS | Render ms | Build ms | Submit ms | Upload Bytes |
|------|----------|-----------|----------|-----------|--------------|
| Static scene (clean replay), 10k | 457.66 | 1.8602 | 1.5752 | 0.2850 | 0 |
| Sparse dirty scene, 10k with 100 dirty | 418.44 | 2.0395 | 1.6789 | 0.3606 | 30,784 |
| Full-hot scene, 10k | 185.58 | 5.0920 | 3.9017 | 1.1903 | 2,087,072 |
| Static scene (clean replay), 100k | 60.41 | 14.2607 | 11.1202 | 3.1405 | 0 |
| Sparse dirty scene, 100k with 1k dirty | 75.64 | 11.3359 | 9.0200 | 2.3159 | 247,936 |
| Full-hot scene, 100k | 20.33 | 47.9315 | 36.5910 | 11.3405 | 20,807,072 |
| Text editor static scene | 579.77 | 1.5522 | 1.4728 | 0.0794 | 0 |
| Node graph showcase, pretty | 512.50 | 1.7715 | 1.6773 | 0.0942 | scene-dependent |
| Node graph benchmark mode | 509.00 | 1.7863 | 1.6920 | 0.0943 | scene-dependent |

What these numbers mean:

- The architecture story is real. Static clean-replay scenes really do collapse to zero upload after warmup.
- Stygian is now in the `~500 FPS` ballpark for the node/text showcase paths on this HD 4600 after the timing-path fixes.
- Dense visible `100k` scenes with persistent state are still nowhere near `500 FPS` on this hardware.
- Sparse dirty `100k` beats static `100k` here because the measured scene classes are not identical in CPU work; that is a benchmark-design smell worth cleaning up before using those numbers as marketing ammunition.
- Full-hot `100k` is the regime where upload bandwidth and CPU build cost dominate again.

## Source-Backed Findings

### 1. Retained display lists really do win on sparse mutation

Mozilla's retained display list work is the cleanest external proof that retaining scene structure matters. Their blog explains that rebuilding a full display list every update wastes time on repeated work, and their `displaylist_mutate` stress test with over 10,000 items and one-at-a-time mutation saw a drop of more than 30% in run time after retaining partial state instead of rebuilding everything.

Why it matters for Stygian:

- This is the exact regime Stygian should dominate:
  - static UI
  - sparse dirty updates
  - overlays changing while the base scene stays clean
- This is also a warning: retained pipelines still lose their edge when "everything changed" and the analysis cost becomes wasted work.

Source:

- Mozilla Gfx Team, "Retained Display Lists"  
  [https://mozillagfx.wordpress.com/2018/01/09/retained-display-lists/](https://mozillagfx.wordpress.com/2018/01/09/retained-display-lists/)

### 2. Single draw call does not beat fill rate

The current Stygian node showcase demonstrates this directly. One draw call is great for CPU submission overhead, but the fragment shader still pays for:

- SDF distance evaluation
- MTSDF median decode
- anti-aliasing derivatives
- clipping
- blending
- overdraw from fat bounds

Khronos' early fragment test docs make the important point: if the fragment shader uses `discard`, some hardware will lose early depth/fragment rejection opportunities. PowerVR's guidance is even blunter: avoid `discard` and alpha testing when possible because they can undercut early rejection behavior.

Why it matters for Stygian:

- The current clip path in [shaders/stygian.frag](D:/Projects/Code/Stygian/shaders/stygian.frag) uses per-fragment clip rejection.
- For rectangular UI clips, Stygian should prefer hardware scissor where possible.
- Nested clips can still fall back to stencil or mask paths when necessary.
- The graph grid should not be hundreds of rects. It should be one procedural background element.

Sources:

- Khronos OpenGL Wiki, "Early Fragment Test"  
  [https://wikis.khronos.org/opengl/Early_Fragment_Test](https://wikis.khronos.org/opengl/Early_Fragment_Test)
- Imagination / PowerVR, "Do Not Use Discard"  
  [https://docs.imgtec.com/starter-guides/powervr-architecture/html/topics/rules/do-not-use-discard.html](https://docs.imgtec.com/starter-guides/powervr-architecture/html/topics/rules/do-not-use-discard.html)

### 3. MTSDF is strong, but it has rules

Viktor Chlumsky's `msdfgen` and `msdf-atlas-gen` docs are the best direct sources here.

Important takeaways:

- MSDF/MTSDF must be sampled in linear space, not sRGB.
- For 2D rendering, `screenPxRange()` can generally be replaced by a precomputed uniform value instead of per-fragment derivatives.
- `screenPxRange()` must not drop below `1`, and values below `2` are likely to anti-alias badly.
- `-autoframe` is explicitly not for character maps.
- `mtsdf` is MSDF plus true SDF in alpha, which is useful for soft effects.
- Atlas generation has knobs that matter for real fonts:
  - corner angle
  - edge coloring strategy
  - error correction
  - overlap handling
  - scanline sign fixes
  - glyph pixel alignment
  - padding and miter limits

Why it matters for Stygian:

- Stygian's current text shader in [shaders/text.glsl](D:/Projects/Code/Stygian/shaders/text.glsl) uses `fwidth(texCoord)` every fragment. That is acceptable, but for 2D UI it is leaving performance on the table.
- The current text path should move to a precomputed per-run or per-instance `screenPxRange` / `unitRange` path.
- Stygian should keep the atlas in linear space and be paranoid about not letting a PNG loader or sampler state sneak in sRGB treatment.
- Stygian should stop assuming one text path is enough. Small fixed-size UI labels do not need the full MTSDF cost if a cheaper coverage path looks identical at target size.

Sources:

- `msdfgen` README  
  [https://github.com/Chlumsky/msdfgen](https://github.com/Chlumsky/msdfgen)
- `msdf-atlas-gen` README  
  [https://github.com/Chlumsky/msdf-atlas-gen](https://github.com/Chlumsky/msdf-atlas-gen)
- Valve, "Improved Alpha-Tested Magnification for Vector Textures and Special Effects"  
  [https://steamcdn-a.akamaihd.net/apps/valve/2007/SIGGRAPH2007_AlphaTestedMagnification.pdf](https://steamcdn-a.akamaihd.net/apps/valve/2007/SIGGRAPH2007_AlphaTestedMagnification.pdf)

### 4. Serious text rendering is mostly shaping and layout

Skia's text docs make the point clearly: text has a "processing" step that is surprisingly complex and expensive, and shaping should be treated as a separate subsystem. HarfBuzz documents the same thing from the shaping side: feed it Unicode codepoints, get glyph IDs and positions back, and cache shape plans as appropriate.

Unicode also matters:

- grapheme clusters are the correct unit for "user-perceived characters"
- line breaking has its own algorithm and data tables

Why it matters for Stygian:

- A serious editor cannot treat bytes as editing units forever.
- Caret motion, backspace, selection, hit-testing, and line wrapping need grapheme-aware logic.
- Complex scripts, ligatures, marks, bidi, and "unsafe to break" runs are shaping problems, not texture problems.
- Stygian should split text into:
  - shaping/layout
  - line/run cache
  - draw submission

Sources:

- Skia, "Text API Overview"  
  [https://docs.skia.org/docs/dev/design/text_overview/](https://docs.skia.org/docs/dev/design/text_overview/)
- Skia, "Shaped Text"  
  [https://skia.org/docs/dev/design/text_shaper/](https://skia.org/docs/dev/design/text_shaper/)
- HarfBuzz, "What does HarfBuzz do?"  
  [https://harfbuzz.github.io/what-does-harfbuzz-do.html](https://harfbuzz.github.io/what-does-harfbuzz-do.html)
- HarfBuzz, "Getting started"  
  [https://harfbuzz.github.io/getting-started.html](https://harfbuzz.github.io/getting-started.html)
- HarfBuzz, "Shaping and shape plans"  
  [https://harfbuzz.github.io/shaping-and-shape-plans.html](https://harfbuzz.github.io/shaping-and-shape-plans.html)
- Unicode UAX #29, "Unicode Text Segmentation"  
  [https://www.unicode.org/reports/tr29/](https://www.unicode.org/reports/tr29/)
- Unicode UAX #14, "Unicode Line Breaking Algorithm"  
  [https://www.unicode.org/reports/tr14/](https://www.unicode.org/reports/tr14/)

### 5. Buffer streaming is still a synchronization problem

Khronos' buffer streaming guide is worth following. The enemy is implicit synchronization. Dirty-range `glBufferSubData` is fine and simple, but if Stygian wants to push much harder on fully hot scenes, persistent mapping plus fenced ring sections is the next obvious OpenGL path.

Why it matters for Stygian:

- The current upload path is good enough for sparse dirty scenes.
- If Stygian wants better worst-case behavior, it should consider:
  - persistent-mapped upload rings
  - buffer orphaning fallback
  - explicit fences instead of trusting the driver to hide sync pain

Source:

- Khronos OpenGL Wiki, "Buffer Object Streaming"  
  [https://wikis.khronos.org/opengl/Buffer_Object_Streaming](https://wikis.khronos.org/opengl/Buffer_Object_Streaming)

### 6. Hardware class still matters

Intel's Gen7.5 paper is a useful sanity check for Haswell iGPUs. HD 4600 is not a joke, but it is still a 20-EU shared-memory part with limited cache and bandwidth compared to modern dGPUs.

Why it matters for Stygian:

- A claim that depends on old iGPU hardware doing modern dGPU work is a bad claim.
- Stygian should benchmark across device classes and publish that directly:
  - old iGPU
  - modern iGPU
  - mid-range dGPU

Source:

- Intel, "The Compute Architecture of Intel Processor Graphics Gen7.5"  
  [https://www.intel.com/content/dam/develop/external/us/en/documents/compute-architecture-of-intel-processor-graphics-gen7dot5-aug4-2014-166010.pdf](https://www.intel.com/content/dam/develop/external/us/en/documents/compute-architecture-of-intel-processor-graphics-gen7dot5-aug4-2014-166010.pdf)

## Optimization Roadmap For Stygian

## P0: Fix the benchmark story first

Do this before chasing random optimizations.

- Add a no-present benchmark mode.
  - The benchmark should be able to run without swap/present pacing.
  - Report:
    - build ms
    - submit ms
    - GPU ms
    - upload bytes
    - upload ranges
    - draw calls
    - visible element count
- Keep a second mode with normal present so real desktop behavior is still visible.
- Disable VSync in perf targets by default.
- Save results to CSV and JSON, not just human-readable console lines.
- Add warmup and measurement phases:
  - warmup: 5 seconds
  - capture: 15 seconds
  - report: avg, p50, p95, p99

Why first:

- If the harness is inaccurate, every optimization discussion starts from the wrong baseline.

## P0: Separate headline benchmarks from pretty demos

Stygian needs two different demo classes:

- Showcase demos
  - pretty
  - screenshot-friendly
  - allowed to spend GPU on style
- Benchmark demos
  - ugly on purpose
  - minimal overdraw
  - minimal UI chrome
  - built to isolate architecture wins

Never use the pretty node editor as the headline `100k` number.

## P0: Replace the graph grid with one procedural element

Current problem:

- `stygian_graph_draw_grid()` can emit hundreds of rects per frame.

Fix:

- Add a `STYGIAN_GRID` element type, or reuse a background element with world/grid parameters in SoA appearance or effects.
- Compute major/minor lines procedurally in the fragment shader.
- Make the grid LOD-aware:
  - when zoomed far out, skip minor lines
  - when zoomed way out, blend into a sparse grid or crosshatch

Expected gain:

- lower element count
- lower CPU build cost
- lower upload churn
- better showcase FPS for node scenes

## P0: Add a scissor fast-path for rectangular clips

Current problem:

- Many Stygian clips are plain rectangles.
- Per-fragment clip reject is convenient but not free.

Fix:

- If clip stack depth is 0 or 1 and the effective clip is axis-aligned, use native scissor.
- Keep the shader clip path for nested or irregular cases.
- If nested clipping becomes common, investigate a stencil-based clip mask path.

Expected gain:

- less fragment waste
- fewer `discard`-related penalties
- better graph/editor throughput

## P0: Stop using derivative-based MTSDF AA for ordinary 2D UI text

Current problem:

- The text shader computes AA scale from derivatives in every fragment.

Fix:

- Precompute `screenPxRange` or `unitRange` per text run or per glyph instance for 2D UI.
- Pass the result through instance data or a compact text parameter buffer.
- Keep derivative mode only for perspective or highly transformed paths.

Expected gain:

- lower fragment ALU cost
- more predictable text cost
- cleaner separation between 2D UI and "fancy transformed text"

## P1: Build a real text pipeline

The current and future text work should be split into layers.

### Layer 1: shaping and segmentation

- integrate HarfBuzz
- use grapheme clusters for caret/selection/backspace
- use Unicode line breaking data
- keep bidi support on the roadmap even if initial support is narrow

### Layer 2: line and run cache

- store shaped lines as runs
- cache:
  - glyph IDs
  - advances
  - offsets
  - cluster boundaries
  - line metrics
  - wrap opportunities
- invalidate only the touched line range on edits
- keep prefix widths so hit-testing does not rescan every glyph

### Layer 3: draw submission

- one `stygian_text()` call per visible line/run is the bare minimum
- next step: internal text-run submission path that packs glyph instances in a dedicated text lane
- do not keep paying one public logical element per glyph forever if the goal is editor-class scale

### Layer 4: specialized renderer paths

Use more than one text path:

- Small UI labels:
  - cheap grayscale or softmask atlas
  - snapped to pixel grid
  - prioritize speed
- Normal zoomable editor text:
  - MSDF or MTSDF
  - run-level caching
- Large zoom / outline / glow:
  - MTSDF path with alpha-channel SDF effects

That is not cheating. It is sane.

## P1: Add a dual-path or tri-path text system

`msdf-atlas-gen` makes the tradeoffs pretty obvious:

- `softmask` is anti-aliased but not scale-flexible
- `msdf` and `mtsdf` scale better and preserve corners
- `mtsdf` adds soft-effect headroom

The right Stygian move is likely:

- body text and tiny UI labels use a cheap mask path
- scalable editor text uses MSDF/MTSDF
- extreme zoom/effects use MTSDF

This is how Stygian gets both:

- very high FPS where visuals do not need the expensive path
- good zoom behavior where they do

## P1: Fix glyph generation quality at the source

Generation-side work:

- audit atlas generation flags
- avoid `-autoframe` for character maps
- validate:
  - `-pxrange`
  - `-angle`
  - `-coloringstrategy`
  - `-errorcorrection`
  - `-overlap`
  - `-scanline`
  - `-pxalign`
  - padding and miter settings
- keep atlas channels linear, never sRGB
- keep a test that zooms text until AA failure is obvious

Add a zoom torture test:

- render same sample text at:
  - 0.75x
  - 1x
  - 2x
  - 4x
  - 8x
  - 16x
- report both:
  - visual artifacts
  - frame cost

## P1: Add text LOD and snapping

Practical rules:

- snap baseline and glyph origin to pixel grid for small text
- quantize scale/origin enough to improve cache reuse
- collapse tiny text to a cheaper path before MTSDF starts burning ALU for no visual gain
- avoid glow, blur, shadow, or stroke paths on text unless explicitly needed

This section is partly inference from the source material and partly normal renderer common sense, but it is the right direction.

## P1: Improve the GL upload path for worst-case scenes

The current chunked dirty-range upload path is not wrong. It is just not the final answer for fully hot loads.

Likely next OpenGL step:

- persistent mapped ring buffers for SoA staging
- fence each segment
- memcpy into the next free segment instead of repeated `glBufferSubData`
- keep `glBufferSubData` as the simpler fallback path

Also worth testing:

- upload only the SoA lanes that actually matter for a benchmark
- skip effects buffer upload entirely when effects are disabled
- maintain smaller benchmark-oriented element types with compact payloads

## P1: Add render-path LOD for wires and graph chrome

For graph workloads:

- smooth cubic wire only when:
  - selected
  - hovered
  - zoomed in enough to justify it
- distant wires should degrade to:
  - quadratic
  - segmented polyline
  - or even a simple line path
- node body style should also LOD:
  - distant nodes: flat rect
  - medium: rounded rect
  - near: pretty header/body treatment

This is not giving up quality. It is spending quality where it is visible.

## P2: Add a dedicated text-run element lane

This is the bigger architectural move.

The idea:

- keep public text API at string/run granularity
- internally store shaped glyph runs in a dedicated buffer
- dirty runs update their glyph payloads
- unchanged runs stay resident
- main scene still uses Stygian's invalidation and replay rules

There are several ways to implement this:

- keep one glyph quad per glyph internally, but move glyph data into a dedicated compact lane
- or use compute/prepass expansion into a transient glyph instance buffer on dirty runs only

The exact method is less important than the principle:

- stop making the editor pay full public-element semantics per glyph

## P2: Add occlusion and coverage-aware metrics

For serious optimization, Stygian's perf HUD should also know:

- approximate pixel coverage
- average overdraw factor for key demo modes
- per-element-type cost buckets:
  - rects
  - text
  - wires
  - textures
  - effects

If a "1000 element" scene is really a "2 million expensive pixels plus blending" scene, the tooling should say that explicitly.

## Stress-Test Matrix

This is the benchmark suite Stygian actually needs.

### Class A: Static scene (clean replay)

- `10k`, `50k`, `100k`, `250k` opaque rects
- no mutations
- no overlay
- no present pacing in raw mode

Headline:

- scope replay hit rate
- upload bytes should fall to zero after warmup

### Class B: Sparse dirty scene

- same sizes as above
- mutate `1%` of elements per frame
- variants:
  - clustered dirty
  - scattered dirty

Headline:

- upload bytes should scale with dirty coverage, not total element count

### Class C: Full-hot scene

- same sizes
- every element changes every frame

Purpose:

- measure worst-case ceiling
- not the main architecture flex

### Class D: Text wall

- large document
- visible-line virtualization enabled
- variants:
  - idle caret blink
  - typing near end of line
  - mid-buffer insertion
  - scroll only
  - zoom only
  - syntax color churn

Headline:

- visible runs dirtied
- glyphs uploaded
- caret hit-test cost

### Class E: Graph canvas

Use three graph modes:

- benchmark graph:
  - no grid
  - flat nodes
  - no labels
  - optional wires off
- mixed graph:
  - wires on
  - labels off
  - cheap node bodies
- pretty graph:
  - screenshot mode
  - not the headline perf mode

This is how Stygian avoids lying to itself.

### Class F: Clip stack and overlay

- nested panels
- split panes
- overlay changes while base stays clean

Purpose:

- prove DDI and scoped invalidation behavior under real tool UI

### Class G: Zoom torture

- text zoom
- graph zoom
- mixed zoom with pan

Purpose:

- prove MTSDF quality and expose fragment bottlenecks

## Benchmark Rules Against Other Libraries

If Stygian wants to beat everyone, the comparison has to be fair.

### Candidate libraries

Start with:

- Dear ImGui
- Nuklear
- Clay
- egui
- Slint
- dvui

Possible later additions:

- iced
- imgui-node-editor style graph add-ons where relevant

### Fairness rules

- same machine
- same GPU
- same monitor mode
- same window size
- VSync off for raw renderer tests
- same font size and approximate scene density
- warm caches before timing
- separate:
  - raw renderer mode
  - desktop present mode
- compare scene classes, not marketing screenshots

### Metrics to publish

- average frame ms
- p95 and p99 frame ms
- build ms
- submit ms
- GPU ms
- present ms
- upload bytes
- draw calls
- visible items
- memory footprint if obtainable

### What Stygian should try to win

- static scenes
- sparse dirty scenes
- overlay churn over stable base scenes
- zoomed scalable text if text path is fixed

### What Stygian should not lead with unless it actually wins

- fully hot pretty scenes with huge blended overdraw
- old iGPU pretty-node FPS claims
- generic "single draw call therefore fastest" claims

## What A Real 100,000-Element Headline Should Look Like

If Stygian wants a `100,000` headline, do it like this:

- test name:
  - `rect_static_100k`
  - `rect_sparse_100k`
  - `text_idle_100k_glyphs`
- conditions:
  - VSync off
  - no perf overlay
  - raw renderer mode and present mode both reported
- output:
  - CSV
  - screenshot
  - hardware info
  - scene description

Good headline:

- "100,000 GPU-resident UI elements, zero upload after warmup, single draw call, X render FPS"

Bad headline:

- "100,000 pretty graph nodes at 500 FPS" when that is not remotely what was measured

## Recommended Implementation Order

1. Fix the benchmark harness.
2. Replace graph grid emission with a procedural grid path.
3. Add scissor fast-path clips.
4. Switch MTSDF AA scale from fragment derivatives to precomputed 2D values.
5. Integrate shaping/segmentation groundwork:
   - HarfBuzz
   - grapheme clusters
   - line breaks
6. Build line/run caches for text.
7. Add dual-path text rendering.
8. Test persistent mapped upload rings on GL.
9. Add benchmark-only `100k` targets.
10. Only then publish "wins" against other stacks.

## References

- Valve, "Improved Alpha-Tested Magnification for Vector Textures and Special Effects"  
  [https://steamcdn-a.akamaihd.net/apps/valve/2007/SIGGRAPH2007_AlphaTestedMagnification.pdf](https://steamcdn-a.akamaihd.net/apps/valve/2007/SIGGRAPH2007_AlphaTestedMagnification.pdf)
- Viktor Chlumsky, `msdfgen`  
  [https://github.com/Chlumsky/msdfgen](https://github.com/Chlumsky/msdfgen)
- Viktor Chlumsky, `msdf-atlas-gen`  
  [https://github.com/Chlumsky/msdf-atlas-gen](https://github.com/Chlumsky/msdf-atlas-gen)
- HarfBuzz Manual  
  [https://harfbuzz.github.io/](https://harfbuzz.github.io/)
- Unicode UAX #29, "Unicode Text Segmentation"  
  [https://www.unicode.org/reports/tr29/](https://www.unicode.org/reports/tr29/)
- Unicode UAX #14, "Unicode Line Breaking Algorithm"  
  [https://www.unicode.org/reports/tr14/](https://www.unicode.org/reports/tr14/)
- Skia text docs  
  [https://docs.skia.org/docs/dev/design/text_overview/](https://docs.skia.org/docs/dev/design/text_overview/)  
  [https://skia.org/docs/dev/design/text_shaper/](https://skia.org/docs/dev/design/text_shaper/)
- Khronos OpenGL Wiki, "Buffer Object Streaming"  
  [https://wikis.khronos.org/opengl/Buffer_Object_Streaming](https://wikis.khronos.org/opengl/Buffer_Object_Streaming)
- Khronos OpenGL Wiki, "Early Fragment Test"  
  [https://wikis.khronos.org/opengl/Early_Fragment_Test](https://wikis.khronos.org/opengl/Early_Fragment_Test)
- Mozilla Gfx Team, "Retained Display Lists"  
  [https://mozillagfx.wordpress.com/2018/01/09/retained-display-lists/](https://mozillagfx.wordpress.com/2018/01/09/retained-display-lists/)
- Intel, "The Compute Architecture of Intel Processor Graphics Gen7.5"  
  [https://www.intel.com/content/dam/develop/external/us/en/documents/compute-architecture-of-intel-processor-graphics-gen7dot5-aug4-2014-166010.pdf](https://www.intel.com/content/dam/develop/external/us/en/documents/compute-architecture-of-intel-processor-graphics-gen7dot5-aug4-2014-166010.pdf)
- Imagination / PowerVR, "Do Not Use Discard"  
  [https://docs.imgtec.com/starter-guides/powervr-architecture/html/topics/rules/do-not-use-discard.html](https://docs.imgtec.com/starter-guides/powervr-architecture/html/topics/rules/do-not-use-discard.html)
