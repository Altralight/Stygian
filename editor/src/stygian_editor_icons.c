#include "../include/stygian_editor_icons.h"

#include <math.h>

static void editor_icon_rect_outline(StygianContext *ctx, float x, float y,
                                     float w, float h, float thickness, float r,
                                     float g, float b, float a) {
  stygian_line(ctx, x, y, x + w, y, thickness, r, g, b, a);
  stygian_line(ctx, x + w, y, x + w, y + h, thickness, r, g, b, a);
  stygian_line(ctx, x + w, y + h, x, y + h, thickness, r, g, b, a);
  stygian_line(ctx, x, y + h, x, y, thickness, r, g, b, a);
}

static void editor_icon_plus(StygianContext *ctx, float cx, float cy,
                             float size, float thickness, float r, float g,
                             float b, float a) {
  stygian_line(ctx, cx - size, cy, cx + size, cy, thickness, r, g, b, a);
  stygian_line(ctx, cx, cy - size, cx, cy + size, thickness, r, g, b, a);
}

static void editor_icon_dot(StygianContext *ctx, float cx, float cy, float size,
                            float r, float g, float b, float a) {
  stygian_rect_rounded(ctx, cx - size, cy - size, size * 2.0f, size * 2.0f, r,
                       g, b, a, size);
}

static void editor_icon_diamond_outline(StygianContext *ctx, float cx, float cy,
                                        float rx, float ry, float thickness,
                                        float r, float g, float b, float a) {
  stygian_line(ctx, cx, cy - ry, cx + rx, cy, thickness, r, g, b, a);
  stygian_line(ctx, cx + rx, cy, cx, cy + ry, thickness, r, g, b, a);
  stygian_line(ctx, cx, cy + ry, cx - rx, cy, thickness, r, g, b, a);
  stygian_line(ctx, cx - rx, cy, cx, cy - ry, thickness, r, g, b, a);
}

static void editor_icon_ellipse_outline(StygianContext *ctx, float cx, float cy,
                                        float rx, float ry, float thickness,
                                        float r, float g, float b, float a) {
  int i;
  const int segs = 18;
  const float pi = 3.14159265358979323846f;
  for (i = 0; i < segs; ++i) {
    float t0 = ((float)i / (float)segs) * 2.0f * pi;
    float t1 = ((float)(i + 1) / (float)segs) * 2.0f * pi;
    float x0 = cx + cosf(t0) * rx;
    float y0 = cy + sinf(t0) * ry;
    float x1 = cx + cosf(t1) * rx;
    float y1 = cy + sinf(t1) * ry;
    stygian_line(ctx, x0, y0, x1, y1, thickness, r, g, b, a);
  }
}

void stygian_editor_draw_icon(StygianContext *ctx, StygianEditorIconKind kind,
                              float x, float y, float w, float h, float r,
                              float g, float b, float a) {
  float cx = x + w * 0.5f;
  float cy = y + h * 0.5f;
  float t = 1.6f;
  float thin = 1.25f;
  if (!ctx)
    return;
  switch (kind) {
  case STYGIAN_EDITOR_ICON_SELECT:
    stygian_line(ctx, cx - 6.8f, cy - 7.8f, cx - 6.8f, cy + 6.4f, t, r, g, b,
                 a);
    stygian_line(ctx, cx - 6.8f, cy - 7.8f, cx + 3.6f, cy + 0.9f, t, r, g, b,
                 a);
    stygian_line(ctx, cx - 6.8f, cy + 6.4f, cx - 0.7f, cy + 2.1f, t, r, g, b,
                 a);
    stygian_line(ctx, cx - 0.1f, cy + 2.4f, cx + 4.4f, cy + 10.0f, t, r, g, b,
                 a);
    stygian_line(ctx, cx + 4.4f, cy + 10.0f, cx + 6.8f, cy + 8.5f, t, r, g, b,
                 a);
    break;
  case STYGIAN_EDITOR_ICON_FRAME: {
    float sx = cx - 6.3f;
    float sy = cy - 6.3f;
    editor_icon_rect_outline(ctx, sx, sy, 12.6f, 12.6f, thin, r, g, b, a);
    stygian_line(ctx, sx + 6.3f, sy, sx + 6.3f, sy + 12.6f, thin, r, g, b, a);
    stygian_line(ctx, sx, sy + 6.3f, sx + 12.6f, sy + 6.3f, thin, r, g, b, a);
  } break;
  case STYGIAN_EDITOR_ICON_LINE:
    stygian_line(ctx, cx - 5.8f, cy + 5.8f, cx + 5.8f, cy - 5.8f, 1.9f, r, g,
                 b, a);
    break;
  case STYGIAN_EDITOR_ICON_PEN:
    editor_icon_diamond_outline(ctx, cx - 0.2f, cy + 0.2f, 5.0f, 6.2f, thin,
                                r, g, b, a);
    stygian_line(ctx, cx + 1.8f, cy - 4.4f, cx + 5.6f, cy - 7.9f, thin, r, g,
                 b, a);
    stygian_line(ctx, cx - 1.7f, cy + 1.8f, cx + 1.2f, cy - 0.9f, thin, r, g,
                 b, a);
    editor_icon_dot(ctx, cx - 0.2f, cy + 0.2f, 0.75f, r, g, b, a);
    break;
  case STYGIAN_EDITOR_ICON_TEXT:
    stygian_line(ctx, cx - 6.2f, cy - 6.5f, cx + 6.2f, cy - 6.5f, 1.9f, r, g,
                 b, a);
    stygian_line(ctx, cx, cy - 6.5f, cx, cy + 6.8f, 1.9f, r, g, b, a);
    break;
  case STYGIAN_EDITOR_ICON_ELLIPSE:
    editor_icon_ellipse_outline(ctx, cx, cy, 6.8f, 6.0f, thin, r, g, b, a);
    break;
  case STYGIAN_EDITOR_ICON_COMPONENT_ADD:
    editor_icon_diamond_outline(ctx, cx - 4.5f, cy - 3.7f, 2.2f, 2.7f, thin,
                                r, g, b, a);
    editor_icon_diamond_outline(ctx, cx + 1.4f, cy - 3.7f, 2.2f, 2.7f, thin,
                                r, g, b, a);
    editor_icon_diamond_outline(ctx, cx - 4.5f, cy + 2.2f, 2.2f, 2.7f, thin,
                                r, g, b, a);
    editor_icon_diamond_outline(ctx, cx + 1.4f, cy + 2.2f, 2.2f, 2.7f, thin,
                                r, g, b, a * 0.6f);
    editor_icon_plus(ctx, cx + 7.0f, cy + 5.6f, 2.7f, thin, r, g, b, a);
    break;
  case STYGIAN_EDITOR_ICON_WAVE:
    stygian_wire(ctx, cx - 7.2f, cy + 1.8f, cx - 4.8f, cy - 3.8f, cx - 2.0f,
                 cy + 1.9f, cx + 0.8f, cy - 1.6f, 1.75f, r, g, b, a);
    stygian_wire(ctx, cx + 0.8f, cy - 1.6f, cx + 3.8f, cy - 5.3f, cx + 5.8f,
                 cy + 0.6f, cx + 7.2f, cy - 0.8f, 1.75f, r, g, b, a);
    break;
  case STYGIAN_EDITOR_ICON_INSPECT: {
    float px = cx - 6.0f;
    float py = cy - 6.0f;
    editor_icon_rect_outline(ctx, px, py, 11.0f, 11.0f, thin, r, g, b, a * 0.75f);
    stygian_line(ctx, cx - 3.4f, cy - 3.4f, cx - 3.4f, cy + 5.0f, t, r, g, b,
                 a);
    stygian_line(ctx, cx - 3.4f, cy - 3.4f, cx + 2.8f, cy + 1.8f, t, r, g, b,
                 a);
    stygian_line(ctx, cx - 3.4f, cy + 5.0f, cx + 0.2f, cy + 2.4f, t, r, g, b,
                 a);
    stygian_line(ctx, cx + 0.2f, cy + 2.4f, cx + 2.8f, cy + 6.8f, t, r, g, b,
                 a);
  } break;
  case STYGIAN_EDITOR_ICON_CODE:
    stygian_line(ctx, cx - 7.2f, cy, cx - 4.2f, cy - 3.4f, thin, r, g, b, a);
    stygian_line(ctx, cx - 7.2f, cy, cx - 4.2f, cy + 3.4f, thin, r, g, b, a);
    stygian_line(ctx, cx - 0.7f, cy + 5.0f, cx + 2.2f, cy - 5.0f, thin, r, g,
                 b, a);
    stygian_line(ctx, cx + 7.2f, cy, cx + 4.2f, cy - 3.4f, thin, r, g, b, a);
    stygian_line(ctx, cx + 7.2f, cy, cx + 4.2f, cy + 3.4f, thin, r, g, b, a);
    break;
  case STYGIAN_EDITOR_ICON_MOVE:
    stygian_line(ctx, cx - 6.0f, cy, cx + 6.0f, cy, thin, r, g, b, a);
    stygian_line(ctx, cx, cy - 6.0f, cx, cy + 6.0f, thin, r, g, b, a);
    stygian_line(ctx, cx - 6.0f, cy, cx - 3.2f, cy - 2.8f, thin, r, g, b, a);
    stygian_line(ctx, cx - 6.0f, cy, cx - 3.2f, cy + 2.8f, thin, r, g, b, a);
    stygian_line(ctx, cx + 6.0f, cy, cx + 3.2f, cy - 2.8f, thin, r, g, b, a);
    stygian_line(ctx, cx + 6.0f, cy, cx + 3.2f, cy + 2.8f, thin, r, g, b, a);
    stygian_line(ctx, cx, cy - 6.0f, cx - 2.8f, cy - 3.2f, thin, r, g, b, a);
    stygian_line(ctx, cx, cy - 6.0f, cx + 2.8f, cy - 3.2f, thin, r, g, b, a);
    stygian_line(ctx, cx, cy + 6.0f, cx - 2.8f, cy + 3.2f, thin, r, g, b, a);
    stygian_line(ctx, cx, cy + 6.0f, cx + 2.8f, cy + 3.2f, thin, r, g, b, a);
    break;
  case STYGIAN_EDITOR_ICON_RECTANGLE:
    editor_icon_rect_outline(ctx, cx - 6.4f, cy - 5.4f, 12.8f, 10.8f, thin, r,
                             g, b, a);
    break;
  case STYGIAN_EDITOR_ICON_GRID:
    editor_icon_rect_outline(ctx, cx - 6.4f, cy - 6.4f, 5.0f, 5.0f, thin, r,
                             g, b, a);
    editor_icon_rect_outline(ctx, cx + 1.4f, cy - 6.4f, 5.0f, 5.0f, thin, r,
                             g, b, a);
    editor_icon_rect_outline(ctx, cx - 6.4f, cy + 1.4f, 5.0f, 5.0f, thin, r,
                             g, b, a);
    editor_icon_rect_outline(ctx, cx + 1.4f, cy + 1.4f, 5.0f, 5.0f, thin, r,
                             g, b, a);
    break;
  case STYGIAN_EDITOR_ICON_PENCIL:
    stygian_line(ctx, cx - 4.8f, cy + 4.8f, cx + 4.8f, cy - 4.8f, 2.0f, r, g,
                 b, a);
    stygian_line(ctx, cx + 4.8f, cy - 4.8f, cx + 7.0f, cy - 7.0f, 1.5f, r, g,
                 b, a);
    stygian_line(ctx, cx - 4.8f, cy + 4.8f, cx - 6.4f, cy + 6.4f, 1.5f, r, g,
                 b, a);
    break;
  case STYGIAN_EDITOR_ICON_BEND:
    stygian_wire(ctx, cx - 7.0f, cy + 3.0f, cx - 3.0f, cy - 4.6f, cx + 1.8f,
                 cy + 5.0f, cx + 7.0f, cy - 1.0f, 1.75f, r, g, b, a);
    editor_icon_dot(ctx, cx - 7.0f, cy + 3.0f, 1.0f, r, g, b, a);
    editor_icon_dot(ctx, cx + 7.0f, cy - 1.0f, 1.0f, r, g, b, a);
    break;
  case STYGIAN_EDITOR_ICON_SET:
    editor_icon_diamond_outline(ctx, cx - 4.5f, cy - 3.7f, 2.2f, 2.7f, thin,
                                r, g, b, a);
    editor_icon_diamond_outline(ctx, cx + 1.4f, cy - 3.7f, 2.2f, 2.7f, thin,
                                r, g, b, a);
    editor_icon_diamond_outline(ctx, cx - 4.5f, cy + 2.2f, 2.2f, 2.7f, thin,
                                r, g, b, a);
    editor_icon_diamond_outline(ctx, cx + 1.4f, cy + 2.2f, 2.2f, 2.7f, thin,
                                r, g, b, a);
    break;
  case STYGIAN_EDITOR_ICON_INSTANCE:
    editor_icon_rect_outline(ctx, cx - 5.4f, cy - 2.5f, 9.2f, 8.6f, thin, r, g,
                             b, a * 0.7f);
    editor_icon_rect_outline(ctx, cx - 2.6f, cy - 5.8f, 9.2f, 8.6f, thin, r, g,
                             b, a);
    break;
  case STYGIAN_EDITOR_ICON_LASSO:
    stygian_wire(ctx, cx - 6.6f, cy + 2.0f, cx - 2.8f, cy - 5.8f, cx + 3.2f,
                 cy + 3.6f, cx + 6.0f, cy - 0.4f, 1.65f, r, g, b, a);
    editor_icon_dot(ctx, cx + 6.5f, cy - 0.2f, 1.0f, r, g, b, a);
    break;
  default:
    editor_icon_plus(ctx, cx, cy, 5.0f, t, r, g, b, a);
    break;
  }
}

const char *stygian_editor_icon_name(StygianEditorIconKind kind) {
  switch (kind) {
  case STYGIAN_EDITOR_ICON_SELECT:
    return "Select";
  case STYGIAN_EDITOR_ICON_FRAME:
    return "Frame";
  case STYGIAN_EDITOR_ICON_LINE:
    return "Line";
  case STYGIAN_EDITOR_ICON_PEN:
    return "Pen";
  case STYGIAN_EDITOR_ICON_TEXT:
    return "Text";
  case STYGIAN_EDITOR_ICON_ELLIPSE:
    return "Ellipse";
  case STYGIAN_EDITOR_ICON_COMPONENT_ADD:
    return "Component Add";
  case STYGIAN_EDITOR_ICON_WAVE:
    return "Wave";
  case STYGIAN_EDITOR_ICON_INSPECT:
    return "Inspect";
  case STYGIAN_EDITOR_ICON_CODE:
    return "Code";
  case STYGIAN_EDITOR_ICON_MOVE:
    return "Move";
  case STYGIAN_EDITOR_ICON_RECTANGLE:
    return "Rectangle";
  case STYGIAN_EDITOR_ICON_GRID:
    return "Grid";
  case STYGIAN_EDITOR_ICON_PENCIL:
    return "Pencil";
  case STYGIAN_EDITOR_ICON_BEND:
    return "Bend";
  case STYGIAN_EDITOR_ICON_SET:
    return "Set";
  case STYGIAN_EDITOR_ICON_INSTANCE:
    return "Instance";
  case STYGIAN_EDITOR_ICON_LASSO:
    return "Lasso";
  default:
    return "Unknown";
  }
}

const char *stygian_editor_icon_token(StygianEditorIconKind kind) {
  switch (kind) {
  case STYGIAN_EDITOR_ICON_SELECT:
    return "STYGIAN_EDITOR_ICON_SELECT";
  case STYGIAN_EDITOR_ICON_FRAME:
    return "STYGIAN_EDITOR_ICON_FRAME";
  case STYGIAN_EDITOR_ICON_LINE:
    return "STYGIAN_EDITOR_ICON_LINE";
  case STYGIAN_EDITOR_ICON_PEN:
    return "STYGIAN_EDITOR_ICON_PEN";
  case STYGIAN_EDITOR_ICON_TEXT:
    return "STYGIAN_EDITOR_ICON_TEXT";
  case STYGIAN_EDITOR_ICON_ELLIPSE:
    return "STYGIAN_EDITOR_ICON_ELLIPSE";
  case STYGIAN_EDITOR_ICON_COMPONENT_ADD:
    return "STYGIAN_EDITOR_ICON_COMPONENT_ADD";
  case STYGIAN_EDITOR_ICON_WAVE:
    return "STYGIAN_EDITOR_ICON_WAVE";
  case STYGIAN_EDITOR_ICON_INSPECT:
    return "STYGIAN_EDITOR_ICON_INSPECT";
  case STYGIAN_EDITOR_ICON_CODE:
    return "STYGIAN_EDITOR_ICON_CODE";
  case STYGIAN_EDITOR_ICON_MOVE:
    return "STYGIAN_EDITOR_ICON_MOVE";
  case STYGIAN_EDITOR_ICON_RECTANGLE:
    return "STYGIAN_EDITOR_ICON_RECTANGLE";
  case STYGIAN_EDITOR_ICON_GRID:
    return "STYGIAN_EDITOR_ICON_GRID";
  case STYGIAN_EDITOR_ICON_PENCIL:
    return "STYGIAN_EDITOR_ICON_PENCIL";
  case STYGIAN_EDITOR_ICON_BEND:
    return "STYGIAN_EDITOR_ICON_BEND";
  case STYGIAN_EDITOR_ICON_SET:
    return "STYGIAN_EDITOR_ICON_SET";
  case STYGIAN_EDITOR_ICON_INSTANCE:
    return "STYGIAN_EDITOR_ICON_INSTANCE";
  case STYGIAN_EDITOR_ICON_LASSO:
    return "STYGIAN_EDITOR_ICON_LASSO";
  default:
    return "STYGIAN_EDITOR_ICON_SELECT";
  }
}
