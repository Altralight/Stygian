# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- Invalidation-driven DDI frame model with render and eval-only paths
- Triple-buffer SoA layout split into Hot, Appearance, and Effects buffers
- Multi-producer command buffer system with deterministic single-thread commit
- Repaint scheduler with reason flags and source tags
- Scope replay and dirty-range upload accounting
- OpenGL 4.3 access point backend
- Vulkan access point backend
- Win32 platform layer with borderless window handling, custom titlebar hooks, snap actions, fullscreen control, and ICC-aware monitor binding
- X11 platform layer scaffolding and build integration work
- Cocoa platform layer scaffolding and build integration work
- MTSDF text rendering pipeline
- Triad glyph system with BC4, R8, and MTSDF fallback chain plus iGPU/dGPU profile selection
- ICC color management with per-monitor auto-binding on Win32
- Arena-allocated flexbox layout engine
- Tab and dock system
- Node graph editor path with spatial JIT culling helpers
- Tiered test suite:
  - Tier1 safety and handle validity checks
  - Tier2 runtime checks for scopes, borderless behavior, ICC, and runtime invariants
  - Tier3 misuse fuzz coverage
- Mini apps:
  - `calculator_mini`
  - `calendar_mini`
  - `text_editor_mini`
  - `perf_pathological_suite`
- Manifest-driven cross-platform build system via `compile/targets.json`, `compile/run.ps1`, and `compile/run.sh`
- GitHub Actions CI for Windows, Linux, and macOS

### Widgets

- Shipped widget baseline including button, slider, checkbox, radio button, text input, text area, vertical scrollbar, tooltip, context menu, modal, panel, perf widget, file explorer, breadcrumb, output panel, problems panel, debug toolbar, call stack, coordinate input, snap settings, CAD gizmo controls, layer manager, scene viewport, scene hierarchy, inspector, asset browser, console log, split panel, menu bar, toolbar, and node graph helpers
