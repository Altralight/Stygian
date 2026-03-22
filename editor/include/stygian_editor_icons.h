#ifndef STYGIAN_EDITOR_ICONS_H
#define STYGIAN_EDITOR_ICONS_H

#include "../../include/stygian.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum StygianEditorIconKind {
  STYGIAN_EDITOR_ICON_SELECT = 0,
  STYGIAN_EDITOR_ICON_FRAME = 1,
  STYGIAN_EDITOR_ICON_LINE = 2,
  STYGIAN_EDITOR_ICON_PEN = 3,
  STYGIAN_EDITOR_ICON_TEXT = 4,
  STYGIAN_EDITOR_ICON_ELLIPSE = 5,
  STYGIAN_EDITOR_ICON_COMPONENT_ADD = 6,
  STYGIAN_EDITOR_ICON_WAVE = 7,
  STYGIAN_EDITOR_ICON_INSPECT = 8,
  STYGIAN_EDITOR_ICON_CODE = 9,
  STYGIAN_EDITOR_ICON_MOVE = 10,
  STYGIAN_EDITOR_ICON_RECTANGLE = 11,
  STYGIAN_EDITOR_ICON_GRID = 12,
  STYGIAN_EDITOR_ICON_PENCIL = 13,
  STYGIAN_EDITOR_ICON_BEND = 14,
  STYGIAN_EDITOR_ICON_SET = 15,
  STYGIAN_EDITOR_ICON_INSTANCE = 16,
  STYGIAN_EDITOR_ICON_LASSO = 17,
  STYGIAN_EDITOR_ICON_COUNT = 18
} StygianEditorIconKind;

void stygian_editor_draw_icon(StygianContext *ctx, StygianEditorIconKind kind,
                              float x, float y, float w, float h, float r,
                              float g, float b, float a);

const char *stygian_editor_icon_name(StygianEditorIconKind kind);
const char *stygian_editor_icon_token(StygianEditorIconKind kind);

#ifdef __cplusplus
}
#endif

#endif
