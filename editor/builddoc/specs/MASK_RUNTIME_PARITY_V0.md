# Mask Runtime Parity V0

This locks `T952` behavior to deterministic runtime semantics.

## Runtime Contract

- Mask relationships are exported as chain-aware clip rectangles.
- For node `N` masked by node `M`, runtime clip is `bounds(M)`.
- If `M` is itself masked, runtime clip becomes the intersection of
  `bounds(M)` and its ancestor mask chain.
- This continues recursively until the chain ends.
- Cycles are rejected at authoring time.

## What Is Supported

- Nested mask stacks via clip-rect intersection.
- Element node masking in generated scene build.
- Vector primitive masking in generated path draw pass.
- Sidecar save/load round-trip of mask links and flags.

## Current Limits (Explicit)

- `invert` and non-alpha mask modes stay as authoring metadata and currently map
  to the same clip-chain behavior at runtime.
- Clip behavior is bounds-based, not arbitrary alpha/luminance compositing.

## Diagnostics

- `mask_export_fallback` is removed for normal alpha mask chains.
- `mask_mode_partial_export` is emitted when mode is non-alpha or invert is set.

## Verification

- Smoke target:
  - `D:\Projects\Code\Stygian\editor\tests\mask_runtime_parity_smoke.c`
- Build wrapper:
  - `D:\Projects\Code\Stygian\editor\compile\windows\build_stygian_editor_mask_runtime_parity_smoke.bat`
