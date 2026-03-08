# Stygian
[CI badge: windows] [CI badge: linux] [CI badge: macos] [License: MIT]

One-line: GPU-accelerated SDF UI library for C23. Single draw call. Invalidation-driven. Cross-platform.

## What It Is
2-3 paragraph description covering: DDI model, SoA GPU residency, single draw call per frame, 
MTSDF text with Triad glyph compression, ICC color management, multi-backend (GL/Vulkan).
Do NOT use "infancy". Do NOT hedge. State what is implemented.

## Platform Support
Table:
| Platform | Backend | Status |
|----------|---------|--------|
| Windows (Win32) | OpenGL 4.3 | ✅ Full |
| Windows (Win32) | Vulkan | ✅ Full |
| Linux (X11) | OpenGL 4.3 | 🔨 Building |
| Linux (X11) | Vulkan | 🔨 Building |
| macOS (Cocoa) | OpenGL | 🔨 Building |

## Why Not Dear ImGui / Clay?
One paragraph: DDI invalidation model (not input-driven redraw), SoA GPU residency, 
generational element handles, ICC color management, Triad glyph compression. 
These don't exist in ImGui or Clay.

## Quick Start
[keep existing quickwindow.c example, unchanged]

## Architecture
Frame pipeline: Collect → Commit → Evaluate → Render/Skip
Link to docs/architecture/runtime_model.md

## Widgets
List only SHIPPED widgets with checkmarks.
Link to widgets/WIDGET_TAXONOMY.md for roadmap.

## Build
[keep existing build instructions, cleaned up]

## License
MIT. Note SGC emoji assets third-party licenses.
