# Text Shaping + Overflow Parity V0

## Goal

Close the mixed-span text export gap so authoring text style runs and area overflow
behavior map deterministically into generated C23 output.

## Delivered

- Export path now emits mixed-style text as multiple `stygian_text(...)` runs per
  text node based on persisted span ranges.
- Text area mode (`box_mode=area`) now emits clip push/pop guards for exported text.
- Shared span-style resolution logic is used by editor text measurement and export
  run generation.
- Legacy `text_span_partial_export` warning was removed for this V0 parity path.

## Notes

- V0 is deterministic and run-based; it does not attempt full script shaping/kerning.
- Export line-wrap policy remains explicit authoring line-break driven.

## Verification

- `editor/tests/text_shaping_overflow_parity_smoke.c`
- target: `stygian_editor_text_shaping_overflow_parity_smoke`
