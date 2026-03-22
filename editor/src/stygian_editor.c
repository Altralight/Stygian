#include "../include/stygian_editor.h"

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#include <float.h>
#ifndef isnan
#define isnan _isnan
#endif
#endif

#define STYGIAN_EDITOR_BOOLEAN_SOLVED_POINT_CAP 12u
#define STYGIAN_EDITOR_BOOLEAN_COORD_CAP 64u
#define STYGIAN_EDITOR_BOOLEAN_EDGE_CAP 4096u

typedef struct StygianEditorNodeRect {
  float x;
  float y;
  float w;
  float h;
  float radius[4];
  StygianEditorColor fill;
} StygianEditorNodeRect;

typedef struct StygianEditorNodeEllipse {
  float x;
  float y;
  float w;
  float h;
  StygianEditorColor fill;
} StygianEditorNodeEllipse;

typedef struct StygianEditorNodePath {
  uint32_t first_point;
  uint32_t point_count;
  bool closed;
  float thickness;
  StygianEditorColor stroke;
  float min_x;
  float min_y;
  float max_x;
  float max_y;
} StygianEditorNodePath;

typedef struct StygianEditorNodeFrame {
  float x;
  float y;
  float w;
  float h;
  bool clip_content;
  StygianEditorColor fill;
  StygianEditorAutoLayoutMode layout_mode;
  StygianEditorAutoLayoutWrap layout_wrap;
  float layout_padding_left;
  float layout_padding_right;
  float layout_padding_top;
  float layout_padding_bottom;
  float layout_gap;
  StygianEditorAutoLayoutAlign layout_primary_align;
  StygianEditorAutoLayoutAlign layout_cross_align;
  StygianEditorFrameOverflowPolicy overflow_policy;
  float scroll_x;
  float scroll_y;
} StygianEditorNodeFrame;

typedef struct StygianEditorNodeText {
  float x;
  float y;
  float w;
  float h;
  float font_size;
  float line_height;
  float letter_spacing;
  uint32_t font_weight;
  StygianEditorTextBoxMode box_mode;
  StygianEditorTextHAlign align_h;
  StygianEditorTextVAlign align_v;
  StygianEditorTextAutoSize auto_size;
  StygianEditorColor fill;
  char text[96];
  uint32_t span_count;
  StygianEditorTextStyleSpan spans[STYGIAN_EDITOR_TEXT_SPAN_CAP];
} StygianEditorNodeText;

typedef struct StygianEditorNodeImage {
  float x;
  float y;
  float w;
  float h;
  uint32_t fit_mode;
  char source[128];
} StygianEditorNodeImage;

typedef struct StygianEditorNodeLine {
  float x1;
  float y1;
  float x2;
  float y2;
  float thickness;
  StygianEditorColor stroke;
} StygianEditorNodeLine;

typedef struct StygianEditorNodeArrow {
  float x1;
  float y1;
  float x2;
  float y2;
  float thickness;
  float head_size;
  StygianEditorColor stroke;
} StygianEditorNodeArrow;

typedef struct StygianEditorNodePolygon {
  float x;
  float y;
  float w;
  float h;
  uint32_t sides;
  float corner_radius;
  StygianEditorColor fill;
} StygianEditorNodePolygon;

typedef struct StygianEditorNodeStar {
  float x;
  float y;
  float w;
  float h;
  uint32_t points;
  float inner_ratio;
  StygianEditorColor fill;
} StygianEditorNodeStar;

typedef struct StygianEditorNodeArc {
  float x;
  float y;
  float w;
  float h;
  float start_angle;
  float sweep_angle;
  float thickness;
  StygianEditorColor stroke;
} StygianEditorNodeArc;

typedef struct StygianEditorNodeGroup {
  float x;
  float y;
  float w;
  float h;
  bool clip_content;
} StygianEditorNodeGroup;

typedef struct StygianEditorNodeComponentDef {
  float x;
  float y;
  float w;
  float h;
  char symbol[64];
  char variant_group[32];
  char variant_name[32];
  uint32_t property_count;
  StygianEditorComponentPropertyDef
      properties[STYGIAN_EDITOR_COMPONENT_PROPERTY_CAP];
} StygianEditorNodeComponentDef;

typedef struct StygianEditorNodeComponentInstance {
  float x;
  float y;
  float w;
  float h;
  StygianEditorNodeId component_def_id;
  char symbol_ref[64];
  StygianEditorComponentOverrideState overrides;
} StygianEditorNodeComponentInstance;

typedef struct StygianEditorNode {
  StygianEditorNodeId id;
  StygianEditorNodeId parent_id;
  StygianEditorShapeKind kind;
  bool locked;
  bool visible;
  bool selected;
  float z;
  float value;
  float rotation_deg;
  char name[64];
  char color_token[32];
  uint32_t fill_count;
  StygianEditorNodeFill fills[STYGIAN_EDITOR_NODE_FILL_CAP];
  StygianEditorGradientTransform
      fill_gradient_xform[STYGIAN_EDITOR_NODE_FILL_CAP];
  uint32_t stroke_count;
  StygianEditorNodeStroke strokes[STYGIAN_EDITOR_NODE_STROKE_CAP];
  uint32_t effect_count;
  StygianEditorNodeEffect effects[STYGIAN_EDITOR_NODE_EFFECT_CAP];
  StygianEditorEffectTransform
      effect_xform[STYGIAN_EDITOR_NODE_EFFECT_CAP];
  StygianEditorNodeId mask_node_id;
  StygianEditorMaskMode mask_mode;
  bool mask_invert;
  StygianEditorShaderAttachment shader_attachment;
  bool boolean_valid;
  StygianEditorBooleanOp boolean_op;
  uint32_t boolean_operand_count;
  StygianEditorNodeId
      boolean_operands[STYGIAN_EDITOR_NODE_BOOLEAN_OPERAND_CAP];
  uint32_t boolean_solved_point_count;
  StygianEditorPoint
      boolean_solved_points[STYGIAN_EDITOR_BOOLEAN_SOLVED_POINT_CAP];
  StygianEditorConstraintH constraint_h;
  StygianEditorConstraintV constraint_v;
  float constraint_left;
  float constraint_right;
  float constraint_top;
  float constraint_bottom;
  float constraint_center_dx;
  float constraint_center_dy;
  float constraint_x_ratio;
  float constraint_y_ratio;
  float constraint_w_ratio;
  float constraint_h_ratio;
  bool layout_absolute;
  StygianEditorAutoLayoutSizing layout_sizing_h;
  StygianEditorAutoLayoutSizing layout_sizing_v;
  float size_min_w;
  float size_max_w;
  float size_min_h;
  float size_max_h;
  char text_style[32];
  char effect_style[32];
  char layout_style[32];
  char color_variable[32];
  char number_variable[32];
  StygianEditorPropertyKind number_variable_property;
  bool component_detached;
  union {
    StygianEditorNodeRect rect;
    StygianEditorNodeEllipse ellipse;
    StygianEditorNodePath path;
    StygianEditorNodeFrame frame;
    StygianEditorNodeText text;
    StygianEditorNodeImage image;
    StygianEditorNodeLine line;
    StygianEditorNodeArrow arrow;
    StygianEditorNodePolygon polygon;
    StygianEditorNodeStar star;
    StygianEditorNodeArc arc;
    StygianEditorNodeGroup group;
    StygianEditorNodeComponentDef component_def;
    StygianEditorNodeComponentInstance component_instance;
  } as;
} StygianEditorNode;

typedef struct StygianEditorBehaviorSlot {
  StygianEditorBehaviorId id;
  StygianEditorBehaviorRule rule;
} StygianEditorBehaviorSlot;

typedef struct StygianEditorTimelineTrackSlot {
  StygianEditorTimelineTrackId id;
  StygianEditorTimelineTrack track;
} StygianEditorTimelineTrackSlot;

typedef struct StygianEditorTimelineClipSlot {
  StygianEditorTimelineClipId id;
  StygianEditorTimelineClip clip;
} StygianEditorTimelineClipSlot;

typedef struct StygianEditorDriverSlot {
  StygianEditorDriverId id;
  StygianEditorDriverRule rule;
} StygianEditorDriverSlot;

typedef struct StygianEditorActiveAnimation {
  bool active;
  StygianEditorNodeId target_node;
  StygianEditorPropertyKind property;
  float from_value;
  float to_value;
  uint32_t duration_ms;
  StygianEditorEasing easing;
  uint64_t started_ms;
} StygianEditorActiveAnimation;

typedef struct StygianEditorColorToken {
  char name[32];
  StygianEditorColor color;
} StygianEditorColorToken;

typedef struct StygianEditorTextStyleSlot {
  StygianEditorTextStyleDef def;
} StygianEditorTextStyleSlot;

typedef struct StygianEditorEffectStyleSlot {
  StygianEditorEffectStyleDef def;
} StygianEditorEffectStyleSlot;

typedef struct StygianEditorLayoutStyleSlot {
  StygianEditorLayoutStyleDef def;
} StygianEditorLayoutStyleSlot;

typedef struct StygianEditorVariableSlot {
  StygianEditorVariableDef def;
} StygianEditorVariableSlot;

typedef struct StygianEditorGuideSlot {
  uint32_t id;
  StygianEditorGuideAxis axis;
  float position;
  StygianEditorNodeId parent_id;
} StygianEditorGuideSlot;

typedef struct StygianEditorPathBuilder {
  bool active;
  StygianEditorPathId id;
  uint32_t first_point;
  uint32_t point_count;
  StygianEditorPathDesc desc;
} StygianEditorPathBuilder;

typedef struct StygianEditorStringBuilder {
  char *dst;
  size_t cap;
  size_t len;
} StygianEditorStringBuilder;

#define STYGIAN_EDITOR_HISTORY_CAPACITY 128u

typedef struct StygianEditorHistoryEntry {
  char *before_json;
  char *after_json;
} StygianEditorHistoryEntry;

struct StygianEditor {
  StygianEditorHost host;
  StygianEditorViewport2D viewport;
  StygianEditorGridConfig grid;

  StygianEditorNode *nodes;
  uint32_t node_count;
  uint32_t max_nodes;

  StygianEditorPoint *path_points;
  uint32_t point_count;
  uint32_t max_path_points;

  StygianEditorBehaviorSlot *behaviors;
  StygianEditorActiveAnimation *active_anims;
  uint32_t behavior_count;
  uint32_t max_behaviors;
  StygianEditorTimelineTrackSlot *timeline_tracks;
  uint32_t timeline_track_count;
  uint32_t max_timeline_tracks;
  StygianEditorTimelineClipSlot *timeline_clips;
  uint32_t timeline_clip_count;
  uint32_t max_timeline_clips;
  StygianEditorDriverSlot *drivers;
  uint32_t driver_count;
  uint32_t max_drivers;

  StygianEditorColorToken *color_tokens;
  uint32_t color_token_count;
  uint32_t max_color_tokens;

  StygianEditorGuideSlot *guides;
  uint32_t guide_count;
  uint32_t max_guides;
  uint32_t next_guide_id;
  StygianEditorRulerUnit ruler_unit;
  StygianEditorSnapSources snap_sources;
  StygianEditorTextStyleSlot text_styles[STYGIAN_EDITOR_STYLE_CAP];
  uint32_t text_style_count;
  StygianEditorEffectStyleSlot effect_styles[STYGIAN_EDITOR_STYLE_CAP];
  uint32_t effect_style_count;
  StygianEditorLayoutStyleSlot layout_styles[STYGIAN_EDITOR_STYLE_CAP];
  uint32_t layout_style_count;
  StygianEditorVariableSlot variables[STYGIAN_EDITOR_VARIABLE_CAP];
  uint32_t variable_count;
  char variable_modes[STYGIAN_EDITOR_VARIABLE_MODE_CAP][32];
  uint32_t variable_mode_count;
  uint32_t active_variable_mode;

  StygianEditorPathBuilder path_builder;
  StygianEditorNodeId selected_node;

  StygianEditorNodeId next_node_id;
  StygianEditorPathId next_path_id;
  StygianEditorBehaviorId next_behavior_id;
  StygianEditorTimelineTrackId next_timeline_track_id;
  StygianEditorTimelineClipId next_timeline_clip_id;
  StygianEditorDriverId next_driver_id;

  StygianEditorHistoryEntry *history_entries;
  uint32_t history_count;
  uint32_t history_cursor;
  uint32_t history_capacity;
  bool history_replaying;
  bool history_suspended;
  bool transaction_active;
  bool transaction_dirty;
  char *transaction_before_json;
};

static bool editor_boolean_solve_node(StygianEditor *editor,
                                      StygianEditorNode *node);
static void editor_boolean_refresh_all(StygianEditor *editor);

static float editor_clampf(float v, float lo, float hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static float editor_absf(float v) { return v < 0.0f ? -v : v; }

static bool editor_overflow_scroll_x(StygianEditorFrameOverflowPolicy policy) {
  return policy == STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_X ||
         policy == STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_BOTH;
}

static bool editor_overflow_scroll_y(StygianEditorFrameOverflowPolicy policy) {
  return policy == STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_Y ||
         policy == STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_BOTH;
}

static float editor_safe_zoom(const StygianEditor *editor) {
  if (!editor)
    return 1.0f;
  if (editor->viewport.zoom < 0.0001f)
    return 0.0001f;
  return editor->viewport.zoom;
}

static float editor_safe_clip_z(float z) {
  if (z < -0.98f)
    return -0.98f;
  if (z > 0.98f)
    return 0.98f;
  return z;
}

static bool editor_float_isnan(float v) { return isnan(v) ? true : false; }

static float editor_text_span_line_height(float font_size, float line_height) {
  if (line_height > 0.0f)
    return line_height;
  return font_size * 1.2f;
}

static void editor_text_style_for_byte(const StygianEditorNodeText *text,
                                       size_t pos, float *out_font_size,
                                       float *out_line_height,
                                       float *out_letter_spacing,
                                       StygianEditorColor *out_color,
                                       uint32_t *out_weight) {
  float fs = 16.0f;
  float ls = 0.0f;
  float lh = 0.0f;
  uint32_t weight = 400u;
  StygianEditorColor color = {1.0f, 1.0f, 1.0f, 1.0f};
  uint32_t i;
  if (!text)
    return;
  fs = text->font_size > 0.0f ? text->font_size : 16.0f;
  ls = text->letter_spacing;
  lh = text->line_height;
  weight = text->font_weight;
  color = text->fill;
  for (i = 0u; i < text->span_count && i < STYGIAN_EDITOR_TEXT_SPAN_CAP; ++i) {
    const StygianEditorTextStyleSpan *span = &text->spans[i];
    if (pos < span->start || pos >= (span->start + span->length))
      continue;
    fs = span->font_size > 0.0f ? span->font_size : fs;
    lh = span->line_height > 0.0f ? span->line_height : lh;
    ls = span->letter_spacing;
    weight = span->weight > 0u ? span->weight : weight;
    color = span->color;
    break;
  }
  if (out_font_size)
    *out_font_size = fs;
  if (out_line_height)
    *out_line_height = editor_text_span_line_height(fs, lh);
  if (out_letter_spacing)
    *out_letter_spacing = ls;
  if (out_color)
    *out_color = color;
  if (out_weight)
    *out_weight = weight;
}

static void editor_measure_text_block(const StygianEditorNodeText *text,
                                      float *out_w, float *out_h) {
  const char *s;
  float max_w = 0.0f;
  float line_w = 0.0f;
  float line_h = 0.0f;
  float total_h = 0.0f;
  if (!text) {
    if (out_w)
      *out_w = 0.0f;
    if (out_h)
      *out_h = 0.0f;
    return;
  }
  s = text->text;
  while (*s) {
    unsigned char ch = (unsigned char)*s++;
    float fs = 16.0f;
    float lh = 0.0f;
    float ls = 0.0f;
    float advance = 0.0f;
    size_t pos = (size_t)(s - text->text - 1);
    editor_text_style_for_byte(text, pos, &fs, &lh, &ls, NULL, NULL);
    advance = fs * 0.6f + ls;
    if (ch == '\n') {
      if (line_w > max_w)
        max_w = line_w;
      total_h += (line_h > 0.0f ? line_h : lh);
      line_w = 0.0f;
      line_h = 0.0f;
      continue;
    }
    line_w += advance;
    if (lh > line_h)
      line_h = lh;
  }
  if (line_w > max_w)
    max_w = line_w;
  if (line_h <= 0.0f)
    line_h = editor_text_span_line_height(
        text->font_size > 0.0f ? text->font_size : 16.0f, text->line_height);
  total_h += line_h;
  if (out_w)
    *out_w = max_w;
  if (out_h)
    *out_h = total_h;
}

static void editor_text_apply_auto_size(StygianEditorNode *node) {
  float w = 0.0f;
  float h = 0.0f;
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_TEXT)
    return;
  editor_measure_text_block(&node->as.text, &w, &h);
  if (node->as.text.auto_size == STYGIAN_EDITOR_TEXT_AUTOSIZE_WIDTH ||
      node->as.text.auto_size == STYGIAN_EDITOR_TEXT_AUTOSIZE_BOTH) {
    node->as.text.w = w;
  }
  if (node->as.text.auto_size == STYGIAN_EDITOR_TEXT_AUTOSIZE_HEIGHT ||
      node->as.text.auto_size == STYGIAN_EDITOR_TEXT_AUTOSIZE_BOTH) {
    node->as.text.h = h;
  }
}

static bool editor_node_supports_fill(StygianEditorShapeKind kind) {
  return kind == STYGIAN_EDITOR_SHAPE_RECT ||
         kind == STYGIAN_EDITOR_SHAPE_ELLIPSE ||
         kind == STYGIAN_EDITOR_SHAPE_FRAME ||
         kind == STYGIAN_EDITOR_SHAPE_TEXT ||
         kind == STYGIAN_EDITOR_SHAPE_POLYGON ||
         kind == STYGIAN_EDITOR_SHAPE_STAR;
}

static bool editor_property_allows_number_variable(
    StygianEditorPropertyKind property) {
  return property == STYGIAN_EDITOR_PROP_X || property == STYGIAN_EDITOR_PROP_Y ||
         property == STYGIAN_EDITOR_PROP_WIDTH ||
         property == STYGIAN_EDITOR_PROP_HEIGHT ||
         property == STYGIAN_EDITOR_PROP_VALUE ||
         property == STYGIAN_EDITOR_PROP_OPACITY;
}

static StygianEditorColor editor_node_get_legacy_fill_color(
    const StygianEditorNode *node) {
  StygianEditorColor c = {0};
  if (!node)
    return c;
  switch (node->kind) {
  case STYGIAN_EDITOR_SHAPE_RECT:
    return node->as.rect.fill;
  case STYGIAN_EDITOR_SHAPE_ELLIPSE:
    return node->as.ellipse.fill;
  case STYGIAN_EDITOR_SHAPE_FRAME:
    return node->as.frame.fill;
  case STYGIAN_EDITOR_SHAPE_TEXT:
    return node->as.text.fill;
  case STYGIAN_EDITOR_SHAPE_POLYGON:
    return node->as.polygon.fill;
  case STYGIAN_EDITOR_SHAPE_STAR:
    return node->as.star.fill;
  default:
    return c;
  }
}

static void editor_node_set_legacy_fill_color(StygianEditorNode *node,
                                              StygianEditorColor color) {
  if (!node)
    return;
  switch (node->kind) {
  case STYGIAN_EDITOR_SHAPE_RECT:
    node->as.rect.fill = color;
    break;
  case STYGIAN_EDITOR_SHAPE_ELLIPSE:
    node->as.ellipse.fill = color;
    break;
  case STYGIAN_EDITOR_SHAPE_FRAME:
    node->as.frame.fill = color;
    break;
  case STYGIAN_EDITOR_SHAPE_TEXT:
    node->as.text.fill = color;
    break;
  case STYGIAN_EDITOR_SHAPE_POLYGON:
    node->as.polygon.fill = color;
    break;
  case STYGIAN_EDITOR_SHAPE_STAR:
    node->as.star.fill = color;
    break;
  default:
    break;
  }
}

static StygianEditorColor editor_node_primary_fill_color(
    const StygianEditorNode *node) {
  uint32_t i;
  if (!node)
    return editor_node_get_legacy_fill_color(node);
  for (i = 0u; i < node->fill_count && i < STYGIAN_EDITOR_NODE_FILL_CAP; ++i) {
    const StygianEditorNodeFill *fill = &node->fills[i];
    if (!fill->visible)
      continue;
    if (fill->kind == STYGIAN_EDITOR_FILL_SOLID) {
      StygianEditorColor c = fill->solid;
      c.a *= fill->opacity;
      return c;
    }
  }
  return editor_node_get_legacy_fill_color(node);
}

static const StygianEditorNodeFill *
editor_node_primary_visible_fill(const StygianEditorNode *node) {
  uint32_t i;
  if (!node)
    return NULL;
  for (i = 0u; i < node->fill_count && i < STYGIAN_EDITOR_NODE_FILL_CAP; ++i) {
    if (node->fills[i].visible)
      return &node->fills[i];
  }
  return NULL;
}

static int editor_node_primary_visible_fill_index(const StygianEditorNode *node) {
  uint32_t i;
  if (!node)
    return -1;
  for (i = 0u; i < node->fill_count && i < STYGIAN_EDITOR_NODE_FILL_CAP; ++i) {
    if (node->fills[i].visible)
      return (int)i;
  }
  return -1;
}

static StygianEditorGradientTransform editor_gradient_xform_default(void) {
  StygianEditorGradientTransform out;
  out.origin_x = 0.5f;
  out.origin_y = 0.5f;
  out.scale_x = 1.0f;
  out.scale_y = 1.0f;
  out.rotation_deg = 0.0f;
  return out;
}

static StygianEditorEffectTransform editor_effect_xform_default(void) {
  StygianEditorEffectTransform out;
  out.scale_x = 1.0f;
  out.scale_y = 1.0f;
  out.rotation_deg = 0.0f;
  return out;
}

static bool editor_is_finite(float v) { return !isnan(v) && isfinite(v); }

static void editor_gradient_xform_sanitize(StygianEditorGradientTransform *xform) {
  if (!xform)
    return;
  if (!editor_is_finite(xform->origin_x))
    xform->origin_x = 0.5f;
  if (!editor_is_finite(xform->origin_y))
    xform->origin_y = 0.5f;
  if (!editor_is_finite(xform->scale_x) || fabsf(xform->scale_x) < 0.0001f)
    xform->scale_x = 1.0f;
  if (!editor_is_finite(xform->scale_y) || fabsf(xform->scale_y) < 0.0001f)
    xform->scale_y = 1.0f;
  if (!editor_is_finite(xform->rotation_deg))
    xform->rotation_deg = 0.0f;
}

static void editor_effect_xform_sanitize(StygianEditorEffectTransform *xform) {
  if (!xform)
    return;
  if (!editor_is_finite(xform->scale_x) || fabsf(xform->scale_x) < 0.0001f)
    xform->scale_x = 1.0f;
  if (!editor_is_finite(xform->scale_y) || fabsf(xform->scale_y) < 0.0001f)
    xform->scale_y = 1.0f;
  if (!editor_is_finite(xform->rotation_deg))
    xform->rotation_deg = 0.0f;
}

static void editor_shader_attachment_sanitize(
    StygianEditorShaderAttachment *attachment) {
  if (!attachment)
    return;
  attachment->enabled = attachment->enabled ? true : false;
  attachment->slot_name[sizeof(attachment->slot_name) - 1u] = '\0';
  attachment->asset_path[sizeof(attachment->asset_path) - 1u] = '\0';
  attachment->entry_point[sizeof(attachment->entry_point) - 1u] = '\0';
  if (!attachment->slot_name[0]) {
    snprintf(attachment->slot_name, sizeof(attachment->slot_name), "%s",
             "surface");
  }
  if (!attachment->entry_point[0]) {
    snprintf(attachment->entry_point, sizeof(attachment->entry_point), "%s",
             "main");
  }
}

static bool editor_gradient_xform_is_default(
    const StygianEditorGradientTransform *xform) {
  if (!xform)
    return true;
  return fabsf(xform->origin_x - 0.5f) < 0.0001f &&
         fabsf(xform->origin_y - 0.5f) < 0.0001f &&
         fabsf(xform->scale_x - 1.0f) < 0.0001f &&
         fabsf(xform->scale_y - 1.0f) < 0.0001f &&
         fabsf(xform->rotation_deg) < 0.0001f;
}

static void editor_node_fill_sync_from_legacy(StygianEditorNode *node) {
  StygianEditorNodeFill fill;
  if (!node || !editor_node_supports_fill(node->kind))
    return;
  if (node->fill_count > 0u)
    return;
  memset(&fill, 0, sizeof(fill));
  fill.kind = STYGIAN_EDITOR_FILL_SOLID;
  fill.visible = true;
  fill.opacity = 1.0f;
  fill.solid = editor_node_get_legacy_fill_color(node);
  node->fills[0] = fill;
  node->fill_gradient_xform[0] = editor_gradient_xform_default();
  node->fill_count = 1u;
}

static bool editor_node_supports_stroke(StygianEditorShapeKind kind) {
  return kind == STYGIAN_EDITOR_SHAPE_RECT ||
         kind == STYGIAN_EDITOR_SHAPE_ELLIPSE ||
         kind == STYGIAN_EDITOR_SHAPE_PATH ||
         kind == STYGIAN_EDITOR_SHAPE_FRAME ||
         kind == STYGIAN_EDITOR_SHAPE_LINE ||
         kind == STYGIAN_EDITOR_SHAPE_ARROW ||
         kind == STYGIAN_EDITOR_SHAPE_POLYGON ||
         kind == STYGIAN_EDITOR_SHAPE_STAR ||
         kind == STYGIAN_EDITOR_SHAPE_ARC;
}

static bool editor_node_supports_effects(StygianEditorShapeKind kind) {
  return kind != STYGIAN_EDITOR_SHAPE_COMPONENT_DEF &&
         kind != STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE;
}

static void editor_stroke_set_default(StygianEditorNodeStroke *stroke) {
  if (!stroke)
    return;
  memset(stroke, 0, sizeof(*stroke));
  stroke->visible = true;
  stroke->opacity = 1.0f;
  stroke->thickness = 1.0f;
  stroke->color = stygian_editor_color_rgba(1.0f, 1.0f, 1.0f, 1.0f);
  stroke->cap = STYGIAN_EDITOR_STROKE_CAP_BUTT;
  stroke->join = STYGIAN_EDITOR_STROKE_JOIN_MITER;
  stroke->align = STYGIAN_EDITOR_STROKE_ALIGN_CENTER;
  stroke->miter_limit = 4.0f;
}

static void editor_node_seed_legacy_stroke(StygianEditorNode *node) {
  if (!node || !editor_node_supports_stroke(node->kind))
    return;
  memset(node->strokes, 0, sizeof(node->strokes));
  node->stroke_count = 1u;
  editor_stroke_set_default(&node->strokes[0]);
  switch (node->kind) {
  case STYGIAN_EDITOR_SHAPE_PATH:
    node->strokes[0].thickness = node->as.path.thickness;
    node->strokes[0].color = node->as.path.stroke;
    break;
  case STYGIAN_EDITOR_SHAPE_LINE:
    node->strokes[0].thickness = node->as.line.thickness;
    node->strokes[0].color = node->as.line.stroke;
    break;
  case STYGIAN_EDITOR_SHAPE_ARROW:
    node->strokes[0].thickness = node->as.arrow.thickness;
    node->strokes[0].color = node->as.arrow.stroke;
    break;
  case STYGIAN_EDITOR_SHAPE_ARC:
    node->strokes[0].thickness = node->as.arc.thickness;
    node->strokes[0].color = node->as.arc.stroke;
    break;
  default:
    node->stroke_count = 0u;
    memset(node->strokes, 0, sizeof(node->strokes));
    break;
  }
}

static void editor_recompute_path_bounds(StygianEditor *editor,
                                         StygianEditorNode *node);

static bool editor_node_path_replace_points(
    StygianEditor *editor, StygianEditorNode *node, const StygianEditorPoint *pts,
    uint32_t count) {
  uint32_t first = 0u;
  if (!editor || !node || node->kind != STYGIAN_EDITOR_SHAPE_PATH || !pts ||
      count < 2u)
    return false;
  if (editor->point_count + count > editor->max_path_points)
    return false;
  first = editor->point_count;
  memcpy(&editor->path_points[first], pts, (size_t)count * sizeof(*pts));
  editor->point_count += count;
  node->as.path.first_point = first;
  node->as.path.point_count = count;
  editor_recompute_path_bounds(editor, node);
  return true;
}

static uint64_t editor_now_ms(const StygianEditor *editor) {
  if (!editor || !editor->host.now_ms)
    return 0u;
  return editor->host.now_ms(editor->host.user_data);
}

static void editor_request_repaint(StygianEditor *editor, uint32_t hz) {
  if (!editor || !editor->host.request_repaint_hz)
    return;
  if (hz == 0u)
    hz = 1u;
  editor->host.request_repaint_hz(editor->host.user_data, hz);
}

static void editor_logf(StygianEditor *editor, StygianEditorLogLevel level,
                        const char *fmt, ...) {
  char message[256];
  va_list args;
  if (!editor || !editor->host.log || !fmt)
    return;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);
  editor->host.log(editor->host.user_data, level, message);
}

static const char *editor_shape_kind_label(StygianEditorShapeKind kind) {
  switch (kind) {
  case STYGIAN_EDITOR_SHAPE_RECT:
    return "Rect";
  case STYGIAN_EDITOR_SHAPE_ELLIPSE:
    return "Ellipse";
  case STYGIAN_EDITOR_SHAPE_PATH:
    return "Path";
  case STYGIAN_EDITOR_SHAPE_FRAME:
    return "Frame";
  case STYGIAN_EDITOR_SHAPE_TEXT:
    return "Text";
  case STYGIAN_EDITOR_SHAPE_IMAGE:
    return "Image";
  case STYGIAN_EDITOR_SHAPE_LINE:
    return "Line";
  case STYGIAN_EDITOR_SHAPE_ARROW:
    return "Arrow";
  case STYGIAN_EDITOR_SHAPE_POLYGON:
    return "Polygon";
  case STYGIAN_EDITOR_SHAPE_STAR:
    return "Star";
  case STYGIAN_EDITOR_SHAPE_ARC:
    return "Arc";
  case STYGIAN_EDITOR_SHAPE_GROUP:
    return "Group";
  case STYGIAN_EDITOR_SHAPE_COMPONENT_DEF:
    return "Component";
  case STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE:
    return "Instance";
  default:
    return "Node";
  }
}

static void editor_set_default_node_name(StygianEditorNode *node) {
  if (!node)
    return;
  snprintf(node->name, sizeof(node->name), "%s %u",
           editor_shape_kind_label(node->kind), node->id);
}

static void editor_history_entry_free(StygianEditorHistoryEntry *entry) {
  if (!entry)
    return;
  free(entry->before_json);
  free(entry->after_json);
  entry->before_json = NULL;
  entry->after_json = NULL;
}

static void editor_history_clear(StygianEditor *editor) {
  uint32_t i;
  if (!editor || !editor->history_entries)
    return;
  for (i = 0u; i < editor->history_count; ++i)
    editor_history_entry_free(&editor->history_entries[i]);
  editor->history_count = 0u;
  editor->history_cursor = 0u;
}

static char *editor_capture_project_json(const StygianEditor *editor) {
  size_t needed;
  char *buffer;
  if (!editor)
    return NULL;
  needed = stygian_editor_build_project_json(editor, NULL, 0u);
  if (needed == 0u)
    return NULL;
  buffer = (char *)malloc(needed);
  if (!buffer)
    return NULL;
  if (stygian_editor_build_project_json(editor, buffer, needed) != needed) {
    free(buffer);
    return NULL;
  }
  return buffer;
}

static void editor_history_drop_redo(StygianEditor *editor) {
  uint32_t i;
  if (!editor || editor->history_cursor >= editor->history_count)
    return;
  for (i = editor->history_cursor; i < editor->history_count; ++i)
    editor_history_entry_free(&editor->history_entries[i]);
  editor->history_count = editor->history_cursor;
}

static bool editor_history_push(StygianEditor *editor, char *before_json,
                                char *after_json) {
  StygianEditorHistoryEntry *entry = NULL;
  uint32_t i;
  if (!editor || !before_json || !after_json)
    return false;

  editor_history_drop_redo(editor);
  if (editor->history_count >= editor->history_capacity) {
    editor_history_entry_free(&editor->history_entries[0]);
    for (i = 0u; i + 1u < editor->history_count; ++i)
      editor->history_entries[i] = editor->history_entries[i + 1u];
    editor->history_count -= 1u;
    if (editor->history_cursor > 0u)
      editor->history_cursor -= 1u;
  }

  entry = &editor->history_entries[editor->history_count++];
  entry->before_json = before_json;
  entry->after_json = after_json;
  editor->history_cursor = editor->history_count;
  return true;
}

static void editor_history_mark_dirty(StygianEditor *editor) {
  if (editor && editor->transaction_active)
    editor->transaction_dirty = true;
}

static void editor_history_drop_transaction(StygianEditor *editor) {
  if (!editor)
    return;
  free(editor->transaction_before_json);
  editor->transaction_before_json = NULL;
  editor->transaction_active = false;
  editor->transaction_dirty = false;
}

static bool editor_history_record_mutation(StygianEditor *editor,
                                           const char *before_json) {
  char *after_json = NULL;
  char *before_copy = NULL;
  size_t before_len = 0u;
  if (!editor || !before_json || editor->history_suspended ||
      editor->history_replaying)
    return true;
  before_len = strlen(before_json);
  before_copy = (char *)malloc(before_len + 1u);
  if (!before_copy)
    return false;
  memcpy(before_copy, before_json, before_len + 1u);
  after_json = editor_capture_project_json(editor);
  if (!after_json) {
    free(before_copy);
    return false;
  }
  if (!editor_history_push(editor, before_copy, after_json)) {
    free(before_copy);
    free(after_json);
    return false;
  }
  return true;
}

static bool editor_begin_mutation(StygianEditor *editor, char **out_before_json) {
  if (!out_before_json)
    return false;
  *out_before_json = NULL;
  if (!editor)
    return false;
  if (editor->history_suspended || editor->history_replaying ||
      editor->transaction_active) {
    return true;
  }
  *out_before_json = editor_capture_project_json(editor);
  return *out_before_json != NULL;
}

static bool editor_finish_mutation(StygianEditor *editor, char *before_json,
                                   bool success) {
  if (before_json) {
    if (!success) {
      if (editor) {
        bool was_suspended = editor->history_suspended;
        editor->history_suspended = true;
        (void)stygian_editor_load_project_json(editor, before_json);
        editor->history_suspended = was_suspended;
      }
      free(before_json);
      return false;
    }
    editor_boolean_refresh_all(editor);
    if (!editor_history_record_mutation(editor, before_json)) {
      free(before_json);
      return false;
    }
    free(before_json);
    return true;
  }

  if (!editor || !success)
    return success;
  editor_boolean_refresh_all(editor);
  if (editor->transaction_active)
    editor_history_mark_dirty(editor);
  return true;
}

static void editor_grid_steps(const StygianEditor *editor,
                              float *out_major_world, float *out_minor_world,
                              bool *out_minor_visible) {
  float zoom = editor_safe_zoom(editor);
  float target_px;
  float raw_world;
  float exponent;
  float scale;
  float normalized;
  float quantized;
  float major_world;
  float minor_world = 0.0f;
  bool minor_visible = false;
  float minor_px = 0.0f;

  target_px = editor->grid.major_step_px;
  if (target_px < 8.0f)
    target_px = 96.0f;

  raw_world = target_px / zoom;
  if (raw_world <= 0.000001f)
    raw_world = 0.000001f;

  exponent = floorf(log10f(raw_world));
  scale = powf(10.0f, exponent);
  if (scale <= 0.0f)
    scale = 1.0f;

  normalized = raw_world / scale;
  if (normalized < 1.5f)
    quantized = 1.0f;
  else if (normalized < 3.5f)
    quantized = 2.0f;
  else if (normalized < 7.5f)
    quantized = 5.0f;
  else
    quantized = 10.0f;

  major_world = quantized * scale;
  if (major_world <= 0.000001f)
    major_world = raw_world;

  if (editor->grid.sub_snap_enabled && editor->grid.sub_divisions > 1u) {
    minor_world = major_world / (float)editor->grid.sub_divisions;
    minor_px = minor_world * zoom;
    if (minor_px >= editor_clampf(editor->grid.min_minor_px, 1.0f, 64.0f))
      minor_visible = true;
  }

  if (out_major_world)
    *out_major_world = major_world;
  if (out_minor_world)
    *out_minor_world = minor_world;
  if (out_minor_visible)
    *out_minor_visible = minor_visible;
}

static int editor_find_node_index(const StygianEditor *editor,
                                  StygianEditorNodeId id) {
  uint32_t i;
  if (!editor || id == STYGIAN_EDITOR_INVALID_ID)
    return -1;
  for (i = 0; i < editor->node_count; ++i) {
    if (editor->nodes[i].id == id)
      return (int)i;
  }
  return -1;
}

static StygianEditorNode *editor_find_node(StygianEditor *editor,
                                           StygianEditorNodeId id) {
  int idx = editor_find_node_index(editor, id);
  if (idx < 0)
    return NULL;
  return &editor->nodes[idx];
}

static const StygianEditorNode *editor_find_node_const(const StygianEditor *editor,
                                                        StygianEditorNodeId id) {
  int idx = editor_find_node_index(editor, id);
  if (idx < 0)
    return NULL;
  return &editor->nodes[idx];
}

static bool editor_mask_would_cycle(const StygianEditor *editor,
                                    StygianEditorNodeId node_id,
                                    StygianEditorNodeId candidate_mask_id) {
  uint32_t guard = 0u;
  StygianEditorNodeId cursor = candidate_mask_id;
  if (!editor || node_id == STYGIAN_EDITOR_INVALID_ID ||
      candidate_mask_id == STYGIAN_EDITOR_INVALID_ID) {
    return false;
  }
  while (cursor != STYGIAN_EDITOR_INVALID_ID && guard < editor->node_count + 1u) {
    const StygianEditorNode *n = editor_find_node_const(editor, cursor);
    if (!n)
      break;
    if (cursor == node_id)
      return true;
    cursor = n->mask_node_id;
    guard += 1u;
  }
  return false;
}

static bool editor_is_component_def(const StygianEditorNode *node) {
  return node && node->kind == STYGIAN_EDITOR_SHAPE_COMPONENT_DEF;
}

static bool editor_is_component_instance(const StygianEditorNode *node) {
  return node && node->kind == STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE;
}

static bool editor_component_symbol_copy(char *dst, size_t dst_cap,
                                         const char *symbol) {
  size_t len = 0u;
  if (!dst || dst_cap == 0u)
    return false;
  if (!symbol || !symbol[0]) {
    dst[0] = '\0';
    return true;
  }
  len = strlen(symbol);
  if (len >= dst_cap)
    return false;
  memcpy(dst, symbol, len + 1u);
  return true;
}

static const StygianEditorNode *editor_component_def_for_instance(
    const StygianEditor *editor, const StygianEditorNode *instance) {
  if (!editor || !editor_is_component_instance(instance))
    return NULL;
  if (instance->component_detached)
    return NULL;
  if (instance->as.component_instance.component_def_id != STYGIAN_EDITOR_INVALID_ID) {
    const StygianEditorNode *def = editor_find_node_const(
        editor, instance->as.component_instance.component_def_id);
    if (editor_is_component_def(def))
      return def;
  }
  if (instance->as.component_instance.symbol_ref[0]) {
    uint32_t i;
    for (i = 0u; i < editor->node_count; ++i) {
      const StygianEditorNode *node = &editor->nodes[i];
      if (!editor_is_component_def(node))
        continue;
      if (strncmp(node->as.component_def.symbol,
                  instance->as.component_instance.symbol_ref,
                  sizeof(node->as.component_def.symbol)) == 0) {
        return node;
      }
    }
  }
  return NULL;
}

static bool editor_component_property_value_valid_for_instance_context(
    const StygianEditor *editor, const StygianEditorNode *instance,
    const StygianEditorNode *instance_def,
    const StygianEditorComponentPropertyValue *value);

static bool editor_component_resolve_instance_definition(
    const StygianEditor *editor, const StygianEditorNode *instance,
    StygianEditorNodeId base_def_id, const StygianEditorComponentOverrideState *state,
    StygianEditorNodeId *out_def_id);

static void editor_component_apply_definition_internal(StygianEditor *editor,
                                                       StygianEditorNodeId def_id) {
  const StygianEditorNode *base_def = editor_find_node_const(editor, def_id);
  uint32_t i;
  if (!editor_is_component_def(base_def))
    return;
  for (i = 0u; i < editor->node_count; ++i) {
    StygianEditorNode *node = &editor->nodes[i];
    const StygianEditorNode *resolved_def = base_def;
    if (!editor_is_component_instance(node))
      continue;
    if (node->component_detached)
      continue;
    if (node->as.component_instance.component_def_id != def_id &&
        strncmp(node->as.component_instance.symbol_ref,
                base_def->as.component_def.symbol,
                sizeof(node->as.component_instance.symbol_ref)) != 0) {
      continue;
    }
    {
      StygianEditorNodeId resolved_id = STYGIAN_EDITOR_INVALID_ID;
      if (editor_component_resolve_instance_definition(
              editor, node, def_id, &node->as.component_instance.overrides,
              &resolved_id)) {
        const StygianEditorNode *candidate =
            editor_find_node_const(editor, resolved_id);
        if (editor_is_component_def(candidate))
          resolved_def = candidate;
      }
    }
    node->as.component_instance.component_def_id = resolved_def->id;
    (void)editor_component_symbol_copy(node->as.component_instance.symbol_ref,
                                       sizeof(node->as.component_instance.symbol_ref),
                                       resolved_def->as.component_def.symbol);
    if ((node->as.component_instance.overrides.mask &
         STYGIAN_EDITOR_COMPONENT_OVERRIDE_W) == 0u) {
      node->as.component_instance.w = resolved_def->as.component_def.w;
    }
    if ((node->as.component_instance.overrides.mask &
         STYGIAN_EDITOR_COMPONENT_OVERRIDE_H) == 0u) {
      node->as.component_instance.h = resolved_def->as.component_def.h;
    }
    if ((node->as.component_instance.overrides.mask &
         STYGIAN_EDITOR_COMPONENT_OVERRIDE_VISIBLE) == 0u) {
      node->visible = resolved_def->visible;
    }
  }
}

static int editor_component_def_property_index(
    const StygianEditorNode *def, const char *name) {
  uint32_t i;
  if (!editor_is_component_def(def) || !name || !name[0])
    return -1;
  for (i = 0u; i < def->as.component_def.property_count &&
              i < STYGIAN_EDITOR_COMPONENT_PROPERTY_CAP;
       ++i) {
    if (strncmp(def->as.component_def.properties[i].name, name,
                sizeof(def->as.component_def.properties[i].name)) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static const char *editor_component_property_base_name(const char *name) {
  const char *dot = NULL;
  const char *at = name;
  if (!name)
    return NULL;
  while (*at) {
    if (*at == '.')
      dot = at;
    at++;
  }
  return dot ? (dot + 1) : name;
}

static bool editor_component_property_value_matches_default(
    const StygianEditorComponentPropertyDef *def,
    const StygianEditorComponentPropertyValue *value) {
  if (!def || !value)
    return false;
  if (def->type != value->type)
    return false;
  if (def->type == STYGIAN_EDITOR_COMPONENT_PROPERTY_BOOL) {
    return def->default_bool == value->bool_value;
  }
  return strncmp(def->default_enum, value->enum_value,
                 sizeof(def->default_enum)) == 0;
}

static bool editor_component_property_value_valid_for_def(
    const StygianEditorNode *def,
    const StygianEditorComponentPropertyValue *value) {
  const char *base_name = editor_component_property_base_name(value ? value->name : NULL);
  int idx = editor_component_def_property_index(def, base_name);
  const StygianEditorComponentPropertyDef *prop = NULL;
  uint32_t i;
  if (!editor_is_component_def(def) || !value || !base_name || !base_name[0])
    return false;
  if (idx < 0) {
    /* Scoped subtree overrides intentionally allow keys outside component set props. */
    return strchr(value->name, '.') != NULL;
  }
  prop = &def->as.component_def.properties[idx];
  if (prop->type != value->type)
    return false;
  if (prop->type == STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM) {
    if (!value->enum_value[0])
      return false;
    for (i = 0u; i < prop->enum_option_count &&
                i < STYGIAN_EDITOR_COMPONENT_ENUM_OPTION_CAP;
         ++i) {
      if (strncmp(prop->enum_options[i], value->enum_value,
                  sizeof(prop->enum_options[i])) == 0) {
        return true;
      }
    }
    return false;
  }
  return true;
}

static bool editor_node_get_local_xy(const StygianEditorNode *node, float *out_x,
                                     float *out_y) {
  if (!node || !out_x || !out_y)
    return false;
  switch (node->kind) {
  case STYGIAN_EDITOR_SHAPE_RECT:
    *out_x = node->as.rect.x;
    *out_y = node->as.rect.y;
    return true;
  case STYGIAN_EDITOR_SHAPE_ELLIPSE:
    *out_x = node->as.ellipse.x;
    *out_y = node->as.ellipse.y;
    return true;
  case STYGIAN_EDITOR_SHAPE_PATH:
    *out_x = node->as.path.min_x;
    *out_y = node->as.path.min_y;
    return true;
  case STYGIAN_EDITOR_SHAPE_FRAME:
    *out_x = node->as.frame.x;
    *out_y = node->as.frame.y;
    return true;
  case STYGIAN_EDITOR_SHAPE_TEXT:
    *out_x = node->as.text.x;
    *out_y = node->as.text.y;
    return true;
  case STYGIAN_EDITOR_SHAPE_IMAGE:
    *out_x = node->as.image.x;
    *out_y = node->as.image.y;
    return true;
  case STYGIAN_EDITOR_SHAPE_LINE:
    *out_x = node->as.line.x1;
    *out_y = node->as.line.y1;
    return true;
  case STYGIAN_EDITOR_SHAPE_ARROW:
    *out_x = node->as.arrow.x1;
    *out_y = node->as.arrow.y1;
    return true;
  case STYGIAN_EDITOR_SHAPE_POLYGON:
    *out_x = node->as.polygon.x;
    *out_y = node->as.polygon.y;
    return true;
  case STYGIAN_EDITOR_SHAPE_STAR:
    *out_x = node->as.star.x;
    *out_y = node->as.star.y;
    return true;
  case STYGIAN_EDITOR_SHAPE_ARC:
    *out_x = node->as.arc.x;
    *out_y = node->as.arc.y;
    return true;
  case STYGIAN_EDITOR_SHAPE_GROUP:
    *out_x = node->as.group.x;
    *out_y = node->as.group.y;
    return true;
  case STYGIAN_EDITOR_SHAPE_COMPONENT_DEF:
    *out_x = node->as.component_def.x;
    *out_y = node->as.component_def.y;
    return true;
  case STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE:
    *out_x = node->as.component_instance.x;
    *out_y = node->as.component_instance.y;
    return true;
  default:
    return false;
  }
}

static bool editor_node_get_local_wh(const StygianEditorNode *node, float *out_w,
                                     float *out_h) {
  if (!node || !out_w || !out_h)
    return false;
  switch (node->kind) {
  case STYGIAN_EDITOR_SHAPE_RECT:
    *out_w = node->as.rect.w;
    *out_h = node->as.rect.h;
    return true;
  case STYGIAN_EDITOR_SHAPE_ELLIPSE:
    *out_w = node->as.ellipse.w;
    *out_h = node->as.ellipse.h;
    return true;
  case STYGIAN_EDITOR_SHAPE_PATH:
    *out_w = node->as.path.max_x - node->as.path.min_x;
    *out_h = node->as.path.max_y - node->as.path.min_y;
    return true;
  case STYGIAN_EDITOR_SHAPE_FRAME:
    *out_w = node->as.frame.w;
    *out_h = node->as.frame.h;
    return true;
  case STYGIAN_EDITOR_SHAPE_TEXT:
    *out_w = node->as.text.w;
    *out_h = node->as.text.h;
    return true;
  case STYGIAN_EDITOR_SHAPE_IMAGE:
    *out_w = node->as.image.w;
    *out_h = node->as.image.h;
    return true;
  case STYGIAN_EDITOR_SHAPE_LINE:
    *out_w = node->as.line.x2 - node->as.line.x1;
    *out_h = node->as.line.y2 - node->as.line.y1;
    return true;
  case STYGIAN_EDITOR_SHAPE_ARROW:
    *out_w = node->as.arrow.x2 - node->as.arrow.x1;
    *out_h = node->as.arrow.y2 - node->as.arrow.y1;
    return true;
  case STYGIAN_EDITOR_SHAPE_POLYGON:
    *out_w = node->as.polygon.w;
    *out_h = node->as.polygon.h;
    return true;
  case STYGIAN_EDITOR_SHAPE_STAR:
    *out_w = node->as.star.w;
    *out_h = node->as.star.h;
    return true;
  case STYGIAN_EDITOR_SHAPE_ARC:
    *out_w = node->as.arc.w;
    *out_h = node->as.arc.h;
    return true;
  case STYGIAN_EDITOR_SHAPE_GROUP:
    *out_w = node->as.group.w;
    *out_h = node->as.group.h;
    return true;
  case STYGIAN_EDITOR_SHAPE_COMPONENT_DEF:
    *out_w = node->as.component_def.w;
    *out_h = node->as.component_def.h;
    return true;
  case STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE:
    *out_w = node->as.component_instance.w;
    *out_h = node->as.component_instance.h;
    return true;
  default:
    return false;
  }
}

static bool editor_node_get_parent_size(const StygianEditor *editor,
                                        const StygianEditorNode *node,
                                        float *out_w, float *out_h) {
  const StygianEditorNode *parent = NULL;
  if (!editor || !node || !out_w || !out_h)
    return false;
  if (node->parent_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  parent = editor_find_node_const(editor, node->parent_id);
  if (!parent)
    return false;
  return editor_node_get_local_wh(parent, out_w, out_h);
}

static float editor_node_clamp_width(const StygianEditorNode *node, float width) {
  if (!node)
    return width;
  if (width < node->size_min_w)
    width = node->size_min_w;
  if (node->size_max_w > 0.0f && width > node->size_max_w)
    width = node->size_max_w;
  if (width < 0.0f)
    width = 0.0f;
  return width;
}

static float editor_node_clamp_height(const StygianEditorNode *node,
                                      float height) {
  if (!node)
    return height;
  if (height < node->size_min_h)
    height = node->size_min_h;
  if (node->size_max_h > 0.0f && height > node->size_max_h)
    height = node->size_max_h;
  if (height < 0.0f)
    height = 0.0f;
  return height;
}

static bool editor_node_apply_property(StygianEditor *editor,
                                       StygianEditorNode *node,
                                       StygianEditorPropertyKind property,
                                       float value);

static void editor_constraints_capture(StygianEditor *editor,
                                       StygianEditorNode *node) {
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
  float parent_w = 0.0f, parent_h = 0.0f;
  if (!editor || !node)
    return;
  if (!editor_node_get_local_xy(node, &x, &y) || !editor_node_get_local_wh(node, &w, &h))
    return;
  if (!editor_node_get_parent_size(editor, node, &parent_w, &parent_h)) {
    node->constraint_left = x;
    node->constraint_top = y;
    node->constraint_right = 0.0f;
    node->constraint_bottom = 0.0f;
    node->constraint_center_dx = 0.0f;
    node->constraint_center_dy = 0.0f;
    node->constraint_x_ratio = 0.0f;
    node->constraint_y_ratio = 0.0f;
    node->constraint_w_ratio = 0.0f;
    node->constraint_h_ratio = 0.0f;
    return;
  }
  node->constraint_left = x;
  node->constraint_top = y;
  node->constraint_right = parent_w - (x + w);
  node->constraint_bottom = parent_h - (y + h);
  node->constraint_center_dx = x - (parent_w - w) * 0.5f;
  node->constraint_center_dy = y - (parent_h - h) * 0.5f;
  if (parent_w > 0.0001f) {
    node->constraint_x_ratio = x / parent_w;
    node->constraint_w_ratio = w / parent_w;
  }
  if (parent_h > 0.0001f) {
    node->constraint_y_ratio = y / parent_h;
    node->constraint_h_ratio = h / parent_h;
  }
}

static bool editor_constraints_apply_to_children(StygianEditor *editor,
                                                 StygianEditorNodeId parent_id,
                                                 float parent_w, float parent_h);
static bool editor_auto_layout_recompute_frame_node(StygianEditor *editor,
                                                     StygianEditorNode *frame);

static bool editor_constraints_apply_to_child(StygianEditor *editor,
                                              StygianEditorNode *node,
                                              float parent_w, float parent_h) {
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
  float new_x, new_y, new_w, new_h;
  float old_w = 0.0f, old_h = 0.0f;
  if (!editor || !node)
    return false;
  if (!editor_node_get_local_xy(node, &x, &y) || !editor_node_get_local_wh(node, &w, &h))
    return false;
  new_x = x;
  new_y = y;
  new_w = w;
  new_h = h;
  old_w = w;
  old_h = h;

  switch (node->constraint_h) {
  case STYGIAN_EDITOR_CONSTRAINT_H_RIGHT:
    new_x = parent_w - node->constraint_right - w;
    break;
  case STYGIAN_EDITOR_CONSTRAINT_H_LEFT_RIGHT:
    new_x = node->constraint_left;
    new_w = parent_w - node->constraint_left - node->constraint_right;
    if (new_w < 0.0f)
      new_w = 0.0f;
    break;
  case STYGIAN_EDITOR_CONSTRAINT_H_CENTER:
    new_x = (parent_w - w) * 0.5f + node->constraint_center_dx;
    break;
  case STYGIAN_EDITOR_CONSTRAINT_H_SCALE:
    new_x = parent_w * node->constraint_x_ratio;
    new_w = parent_w * node->constraint_w_ratio;
    if (new_w < 0.0f)
      new_w = 0.0f;
    break;
  case STYGIAN_EDITOR_CONSTRAINT_H_LEFT:
  default:
    new_x = node->constraint_left;
    break;
  }

  switch (node->constraint_v) {
  case STYGIAN_EDITOR_CONSTRAINT_V_BOTTOM:
    new_y = parent_h - node->constraint_bottom - h;
    break;
  case STYGIAN_EDITOR_CONSTRAINT_V_TOP_BOTTOM:
    new_y = node->constraint_top;
    new_h = parent_h - node->constraint_top - node->constraint_bottom;
    if (new_h < 0.0f)
      new_h = 0.0f;
    break;
  case STYGIAN_EDITOR_CONSTRAINT_V_CENTER:
    new_y = (parent_h - h) * 0.5f + node->constraint_center_dy;
    break;
  case STYGIAN_EDITOR_CONSTRAINT_V_SCALE:
    new_y = parent_h * node->constraint_y_ratio;
    new_h = parent_h * node->constraint_h_ratio;
    if (new_h < 0.0f)
      new_h = 0.0f;
    break;
  case STYGIAN_EDITOR_CONSTRAINT_V_TOP:
  default:
    new_y = node->constraint_top;
    break;
  }

  if (!editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_X, new_x) ||
      !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_Y, new_y) ||
      !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_WIDTH, new_w) ||
      !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_HEIGHT, new_h)) {
    return false;
  }

  if (node->kind == STYGIAN_EDITOR_SHAPE_FRAME &&
      (fabsf(old_w - new_w) > 0.0001f || fabsf(old_h - new_h) > 0.0001f)) {
    if (!editor_constraints_apply_to_children(editor, node->id, new_w, new_h))
      return false;
  }
  return true;
}

static bool editor_constraints_apply_to_children(StygianEditor *editor,
                                                 StygianEditorNodeId parent_id,
                                                 float parent_w, float parent_h) {
  uint32_t i;
  if (!editor)
    return false;
  for (i = 0u; i < editor->node_count; ++i) {
    StygianEditorNode *node = &editor->nodes[i];
    if (node->parent_id != parent_id)
      continue;
    if (!editor_constraints_apply_to_child(editor, node, parent_w, parent_h))
      return false;
  }
  return true;
}

typedef struct EditorAutoLayoutChild {
  StygianEditorNode *node;
  float width;
  float height;
  bool fill_primary;
  bool fill_cross;
} EditorAutoLayoutChild;

typedef struct EditorAutoLayoutLine {
  uint32_t start;
  uint32_t count;
  uint32_t fill_primary_count;
  float fixed_primary;
  float cross_size;
} EditorAutoLayoutLine;

static bool editor_auto_layout_recompute_frame_node(StygianEditor *editor,
                                                     StygianEditorNode *frame) {
  uint32_t i;
  uint32_t flow_count = 0u;
  uint32_t at = 0u;
  EditorAutoLayoutChild *children = NULL;
  EditorAutoLayoutLine *lines = NULL;
  float inner_w = 0.0f;
  float inner_h = 0.0f;
  float gap = 0.0f;
  float fixed_primary = 0.0f;
  uint32_t fill_primary_count = 0u;
  float available_primary = 0.0f;
  float fill_each = 0.0f;
  float total_primary = 0.0f;
  float cursor = 0.0f;
  float content_primary = 0.0f;
  float max_scroll_x = 0.0f;
  float max_scroll_y = 0.0f;
  float scroll_x = 0.0f;
  float scroll_y = 0.0f;
  bool horizontal = false;
  bool wrap = false;
  if (!editor || !frame || frame->kind != STYGIAN_EDITOR_SHAPE_FRAME)
    return false;
  if (frame->as.frame.layout_mode == STYGIAN_EDITOR_AUTO_LAYOUT_OFF)
    return true;

  horizontal =
      frame->as.frame.layout_mode == STYGIAN_EDITOR_AUTO_LAYOUT_HORIZONTAL;
  wrap = frame->as.frame.layout_wrap == STYGIAN_EDITOR_AUTO_LAYOUT_WRAP;
  inner_w = frame->as.frame.w - frame->as.frame.layout_padding_left -
            frame->as.frame.layout_padding_right;
  inner_h = frame->as.frame.h - frame->as.frame.layout_padding_top -
            frame->as.frame.layout_padding_bottom;
  if (inner_w < 0.0f)
    inner_w = 0.0f;
  if (inner_h < 0.0f)
    inner_h = 0.0f;
  gap = frame->as.frame.layout_gap;
  if (gap < 0.0f)
    gap = 0.0f;

  for (i = 0u; i < editor->node_count; ++i) {
    const StygianEditorNode *node = &editor->nodes[i];
    if (node->parent_id != frame->id || node->layout_absolute)
      continue;
    flow_count += 1u;
  }
  if (flow_count == 0u)
    return true;

  children = (EditorAutoLayoutChild *)malloc(sizeof(EditorAutoLayoutChild) *
                                             (size_t)flow_count);
  if (!children)
    return false;
  memset(children, 0, sizeof(EditorAutoLayoutChild) * (size_t)flow_count);

  for (i = 0u; i < editor->node_count; ++i) {
    StygianEditorNode *node = &editor->nodes[i];
    if (node->parent_id != frame->id || node->layout_absolute)
      continue;
    children[at].node = node;
    if (!editor_node_get_local_wh(node, &children[at].width, &children[at].height)) {
      free(children);
      return false;
    }
    children[at].width = editor_node_clamp_width(node, children[at].width);
    children[at].height = editor_node_clamp_height(node, children[at].height);
    if (horizontal) {
      children[at].fill_primary =
          node->layout_sizing_h == STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FILL;
      children[at].fill_cross =
          node->layout_sizing_v == STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FILL;
      if (children[at].fill_primary)
        fill_primary_count += 1u;
      else
        fixed_primary += children[at].width;
    } else {
      children[at].fill_primary =
          node->layout_sizing_v == STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FILL;
      children[at].fill_cross =
          node->layout_sizing_h == STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FILL;
      if (children[at].fill_primary)
        fill_primary_count += 1u;
      else
        fixed_primary += children[at].height;
    }
    at += 1u;
  }

  if (wrap) {
    uint32_t line_count = 0u;
    float primary_limit = horizontal ? inner_w : inner_h;
    float cross_cursor = horizontal ? frame->as.frame.layout_padding_top
                                    : frame->as.frame.layout_padding_left;
    float content_cross = 0.0f;
    lines = (EditorAutoLayoutLine *)calloc((size_t)flow_count, sizeof(*lines));
    if (!lines) {
      free(children);
      return false;
    }
    for (i = 0u; i < flow_count; ++i) {
      float child_primary = horizontal ? children[i].width : children[i].height;
      float child_cross = horizontal ? children[i].height : children[i].width;
      EditorAutoLayoutLine *line;
      float used = 0.0f;
      if (line_count == 0u) {
        lines[0].start = 0u;
        line_count = 1u;
      }
      line = &lines[line_count - 1u];
      if (line->count > 0u) {
        uint32_t j;
        used = gap * (float)line->count;
        for (j = 0u; j < line->count; ++j) {
          uint32_t idx = line->start + j;
          used += horizontal ? children[idx].width : children[idx].height;
        }
      }
      if (line->count > 0u && primary_limit > 0.0f &&
          used + child_primary > primary_limit + 0.001f) {
        lines[line_count].start = i;
        line_count += 1u;
        line = &lines[line_count - 1u];
      }
      line->count += 1u;
      if (children[i].fill_primary)
        line->fill_primary_count += 1u;
      else
        line->fixed_primary += child_primary;
      if (child_cross > line->cross_size)
        line->cross_size = child_cross;
    }
    for (i = 0u; i < line_count; ++i) {
      if (i > 0u)
        content_cross += gap;
      content_cross += lines[i].cross_size;
    }
    if (horizontal) {
      max_scroll_y = content_cross - inner_h;
      if (max_scroll_y < 0.0f)
        max_scroll_y = 0.0f;
      if (editor_overflow_scroll_y(frame->as.frame.overflow_policy))
        scroll_y = editor_clampf(frame->as.frame.scroll_y, 0.0f, max_scroll_y);
      frame->as.frame.scroll_x = 0.0f;
      frame->as.frame.scroll_y = scroll_y;
    } else {
      max_scroll_x = content_cross - inner_w;
      if (max_scroll_x < 0.0f)
        max_scroll_x = 0.0f;
      if (editor_overflow_scroll_x(frame->as.frame.overflow_policy))
        scroll_x = editor_clampf(frame->as.frame.scroll_x, 0.0f, max_scroll_x);
      frame->as.frame.scroll_x = scroll_x;
      frame->as.frame.scroll_y = 0.0f;
    }
    for (i = 0u; i < line_count; ++i) {
      EditorAutoLayoutLine *line = &lines[i];
      float available = (horizontal ? inner_w : inner_h) - line->fixed_primary;
      float fill_each_line = 0.0f;
      float total_primary_line = 0.0f;
      float line_cursor;
      uint32_t j;
      if (line->count > 1u)
        available -= gap * (float)(line->count - 1u);
      if (available < 0.0f)
        available = 0.0f;
      if (line->fill_primary_count > 0u)
        fill_each_line = available / (float)line->fill_primary_count;
      for (j = 0u; j < line->count; ++j) {
        uint32_t idx = line->start + j;
        float primary = children[idx].fill_primary
                            ? fill_each_line
                            : (horizontal ? children[idx].width : children[idx].height);
        if (horizontal)
          primary = editor_node_clamp_width(children[idx].node, primary);
        else
          primary = editor_node_clamp_height(children[idx].node, primary);
        total_primary_line += primary;
      }
      if (line->count > 1u)
        total_primary_line += gap * (float)(line->count - 1u);
      line_cursor = horizontal ? frame->as.frame.layout_padding_left
                               : frame->as.frame.layout_padding_top;
      if (frame->as.frame.layout_primary_align ==
          STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_CENTER) {
        line_cursor += ((horizontal ? inner_w : inner_h) - total_primary_line) * 0.5f;
      } else if (frame->as.frame.layout_primary_align ==
                 STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_END) {
        line_cursor += (horizontal ? inner_w : inner_h) - total_primary_line;
      }
      for (j = 0u; j < line->count; ++j) {
        uint32_t idx = line->start + j;
        StygianEditorNode *node = children[idx].node;
        float x = 0.0f;
        float y = 0.0f;
        float w = children[idx].width;
        float h = children[idx].height;
        float cross = 0.0f;
        float cross_offset = 0.0f;
        if (horizontal) {
          if (children[idx].fill_primary)
            w = fill_each_line;
          if (children[idx].fill_cross ||
              frame->as.frame.layout_cross_align ==
                  STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_STRETCH) {
            h = line->cross_size;
          }
          w = editor_node_clamp_width(node, w);
          h = editor_node_clamp_height(node, h);
          cross = line->cross_size - h;
          if (cross < 0.0f)
            cross = 0.0f;
          if (frame->as.frame.layout_cross_align ==
              STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_CENTER)
            cross_offset = cross * 0.5f;
          else if (frame->as.frame.layout_cross_align ==
                   STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_END)
            cross_offset = cross;
          x = line_cursor;
          y = cross_cursor - scroll_y + cross_offset;
          line_cursor += w + gap;
        } else {
          if (children[idx].fill_primary)
            h = fill_each_line;
          if (children[idx].fill_cross ||
              frame->as.frame.layout_cross_align ==
                  STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_STRETCH) {
            w = line->cross_size;
          }
          w = editor_node_clamp_width(node, w);
          h = editor_node_clamp_height(node, h);
          cross = line->cross_size - w;
          if (cross < 0.0f)
            cross = 0.0f;
          if (frame->as.frame.layout_cross_align ==
              STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_CENTER)
            cross_offset = cross * 0.5f;
          else if (frame->as.frame.layout_cross_align ==
                   STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_END)
            cross_offset = cross;
          x = cross_cursor - scroll_x + cross_offset;
          y = line_cursor;
          line_cursor += h + gap;
        }
        if (!editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_X, x) ||
            !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_Y, y) ||
            !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_WIDTH, w) ||
            !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_HEIGHT, h)) {
          free(lines);
          free(children);
          return false;
        }
        editor_constraints_capture(editor, node);
        if (node->kind == STYGIAN_EDITOR_SHAPE_FRAME &&
            !editor_auto_layout_recompute_frame_node(editor, node)) {
          free(lines);
          free(children);
          return false;
        }
      }
      cross_cursor += line->cross_size + gap;
    }
    free(lines);
    free(children);
    return true;
  }

  if (flow_count > 1u)
    fixed_primary += gap * (float)(flow_count - 1u);
  available_primary = (horizontal ? inner_w : inner_h) - fixed_primary;
  if (available_primary < 0.0f)
    available_primary = 0.0f;
  if (fill_primary_count > 0u)
    fill_each = available_primary / (float)fill_primary_count;
  total_primary = 0.0f;
  for (i = 0u; i < flow_count; ++i) {
    StygianEditorNode *node = children[i].node;
    float primary =
        children[i].fill_primary ? fill_each : (horizontal ? children[i].width
                                                           : children[i].height);
    if (horizontal)
      primary = editor_node_clamp_width(node, primary);
    else
      primary = editor_node_clamp_height(node, primary);
    total_primary += primary;
  }
  if (flow_count > 1u)
    total_primary += gap * (float)(flow_count - 1u);
  content_primary = total_primary;

  cursor = horizontal ? frame->as.frame.layout_padding_left
                      : frame->as.frame.layout_padding_top;
  if (frame->as.frame.layout_primary_align == STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_CENTER)
    cursor += ((horizontal ? inner_w : inner_h) - total_primary) * 0.5f;
  else if (frame->as.frame.layout_primary_align ==
           STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_END)
    cursor += (horizontal ? inner_w : inner_h) - total_primary;

  if (horizontal) {
    max_scroll_x = content_primary - inner_w;
    if (max_scroll_x < 0.0f)
      max_scroll_x = 0.0f;
    if (editor_overflow_scroll_x(frame->as.frame.overflow_policy))
      scroll_x = editor_clampf(frame->as.frame.scroll_x, 0.0f, max_scroll_x);
    frame->as.frame.scroll_x = scroll_x;
    frame->as.frame.scroll_y = 0.0f;
  } else {
    max_scroll_y = content_primary - inner_h;
    if (max_scroll_y < 0.0f)
      max_scroll_y = 0.0f;
    if (editor_overflow_scroll_y(frame->as.frame.overflow_policy))
      scroll_y = editor_clampf(frame->as.frame.scroll_y, 0.0f, max_scroll_y);
    frame->as.frame.scroll_x = 0.0f;
    frame->as.frame.scroll_y = scroll_y;
  }

  for (i = 0u; i < flow_count; ++i) {
    StygianEditorNode *node = children[i].node;
    float x = 0.0f;
    float y = 0.0f;
    float w = children[i].width;
    float h = children[i].height;
    float cross = 0.0f;
    float cross_offset = 0.0f;
    if (horizontal) {
      if (children[i].fill_primary)
        w = fill_each;
      if (children[i].fill_cross ||
          frame->as.frame.layout_cross_align ==
              STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_STRETCH) {
        h = inner_h;
      }
      w = editor_node_clamp_width(node, w);
      h = editor_node_clamp_height(node, h);
      cross = inner_h - h;
      if (cross < 0.0f)
        cross = 0.0f;
      if (frame->as.frame.layout_cross_align ==
          STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_CENTER)
        cross_offset = cross * 0.5f;
      else if (frame->as.frame.layout_cross_align ==
               STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_END)
        cross_offset = cross;
      x = cursor - scroll_x;
      y = frame->as.frame.layout_padding_top + cross_offset;
      cursor += w + gap;
    } else {
      if (children[i].fill_primary)
        h = fill_each;
      if (children[i].fill_cross ||
          frame->as.frame.layout_cross_align ==
              STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_STRETCH) {
        w = inner_w;
      }
      w = editor_node_clamp_width(node, w);
      h = editor_node_clamp_height(node, h);
      cross = inner_w - w;
      if (cross < 0.0f)
        cross = 0.0f;
      if (frame->as.frame.layout_cross_align ==
          STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_CENTER)
        cross_offset = cross * 0.5f;
      else if (frame->as.frame.layout_cross_align ==
               STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_END)
        cross_offset = cross;
      x = frame->as.frame.layout_padding_left + cross_offset;
      y = cursor - scroll_y;
      cursor += h + gap;
    }
    if (!editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_X, x) ||
        !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_Y, y) ||
        !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_WIDTH, w) ||
        !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_HEIGHT, h)) {
      free(children);
      return false;
    }
    editor_constraints_capture(editor, node);
    if (node->kind == STYGIAN_EDITOR_SHAPE_FRAME &&
        !editor_auto_layout_recompute_frame_node(editor, node)) {
      free(children);
      return false;
    }
  }
  free(children);
  return true;
}

static void editor_node_world_offset(const StygianEditor *editor,
                                     const StygianEditorNode *node,
                                     float *out_x, float *out_y) {
  float ox = 0.0f;
  float oy = 0.0f;
  const StygianEditorNode *it = node;
  uint32_t guard = 0u;
  if (!editor || !node) {
    if (out_x)
      *out_x = 0.0f;
    if (out_y)
      *out_y = 0.0f;
    return;
  }
  while (it && it->parent_id != STYGIAN_EDITOR_INVALID_ID &&
         guard++ < editor->max_nodes) {
    const StygianEditorNode *parent =
        editor_find_node_const(editor, it->parent_id);
    float px = 0.0f;
    float py = 0.0f;
    if (!parent)
      break;
    if (editor_node_get_local_xy(parent, &px, &py)) {
      ox += px;
      oy += py;
    }
    it = parent;
  }
  if (out_x)
    *out_x = ox;
  if (out_y)
    *out_y = oy;
}

static bool editor_node_is_ancestor(const StygianEditor *editor,
                                    StygianEditorNodeId ancestor_id,
                                    StygianEditorNodeId node_id) {
  const StygianEditorNode *it = NULL;
  uint32_t guard = 0u;
  if (!editor || ancestor_id == STYGIAN_EDITOR_INVALID_ID ||
      node_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  it = editor_find_node_const(editor, node_id);
  while (it && it->parent_id != STYGIAN_EDITOR_INVALID_ID &&
         guard++ < editor->max_nodes) {
    if (it->parent_id == ancestor_id)
      return true;
    it = editor_find_node_const(editor, it->parent_id);
  }
  return false;
}

static bool editor_node_bounds(const StygianEditorNode *node, float *out_x,
                               float *out_y, float *out_w, float *out_h);
static bool editor_node_bounds_world(const StygianEditor *editor,
                                     const StygianEditorNode *node, float *out_x,
                                     float *out_y, float *out_w, float *out_h);
static bool editor_node_hit_test(const StygianEditor *editor,
                                 const StygianEditorNode *node, float x,
                                 float y);
static bool editor_component_property_value_valid_for_instance_context(
    const StygianEditor *editor, const StygianEditorNode *instance,
    const StygianEditorNode *instance_def,
    const StygianEditorComponentPropertyValue *value);
static bool editor_component_resolve_instance_definition(
    const StygianEditor *editor, const StygianEditorNode *instance,
    StygianEditorNodeId base_def_id, const StygianEditorComponentOverrideState *state,
    StygianEditorNodeId *out_def_id);

static void editor_clear_selection(StygianEditor *editor) {
  uint32_t i;
  if (!editor)
    return;
  for (i = 0u; i < editor->node_count; ++i)
    editor->nodes[i].selected = false;
  editor->selected_node = STYGIAN_EDITOR_INVALID_ID;
}

static StygianEditorNodeId
editor_pick_primary_selected(const StygianEditor *editor) {
  uint32_t i;
  int best_i = -1;
  float best_z = -1.0e30f;
  if (!editor)
    return STYGIAN_EDITOR_INVALID_ID;
  for (i = 0u; i < editor->node_count; ++i) {
    const StygianEditorNode *node = &editor->nodes[i];
    if (!node->selected)
      continue;
    if (best_i < 0 || node->z > best_z ||
        (node->z == best_z && (int)i > best_i)) {
      best_i = (int)i;
      best_z = node->z;
    }
  }
  if (best_i < 0)
    return STYGIAN_EDITOR_INVALID_ID;
  return editor->nodes[best_i].id;
}

static void editor_refresh_primary_selected(StygianEditor *editor) {
  if (!editor)
    return;
  editor->selected_node = editor_pick_primary_selected(editor);
}

static uint32_t editor_count_selected_nodes(const StygianEditor *editor) {
  uint32_t i;
  uint32_t count = 0u;
  if (!editor)
    return 0u;
  for (i = 0u; i < editor->node_count; ++i) {
    if (editor->nodes[i].selected)
      ++count;
  }
  return count;
}

static int editor_hit_test_top_node_index(const StygianEditor *editor, float x,
                                          float y) {
  uint32_t i;
  int best_index = -1;
  float best_z = -1.0e30f;
  if (!editor)
    return -1;
  for (i = 0u; i < editor->node_count; ++i) {
    const StygianEditorNode *node = &editor->nodes[i];
    if (!editor_node_hit_test(editor, node, x, y))
      continue;
    if (best_index < 0 || node->z > best_z ||
        (node->z == best_z && (int)i > best_index)) {
      best_index = (int)i;
      best_z = node->z;
    }
  }
  return best_index;
}

static bool editor_rects_intersect(float ax, float ay, float aw, float ah,
                                   float bx, float by, float bw, float bh) {
  return (ax <= bx + bw) && (ax + aw >= bx) && (ay <= by + bh) &&
         (ay + ah >= by);
}

typedef struct EditorBooleanRect {
  float x0;
  float y0;
  float x1;
  float y1;
} EditorBooleanRect;

typedef struct EditorBooleanEdge {
  uint16_t x0;
  uint16_t y0;
  uint16_t x1;
  uint16_t y1;
  bool used;
} EditorBooleanEdge;

static bool editor_boolean_rect_contains(const EditorBooleanRect *r, float x,
                                         float y) {
  if (!r)
    return false;
  return x >= r->x0 && x < r->x1 && y >= r->y0 && y < r->y1;
}

static int editor_boolean_float_compare(const void *a, const void *b) {
  const float fa = *(const float *)a;
  const float fb = *(const float *)b;
  if (fa < fb)
    return -1;
  if (fa > fb)
    return 1;
  return 0;
}

static bool editor_boolean_coord_push_unique(float *values, uint32_t *count,
                                             float value) {
  uint32_t i;
  if (!values || !count)
    return false;
  for (i = 0u; i < *count; ++i) {
    if (editor_absf(values[i] - value) <= 0.0001f)
      return true;
  }
  if (*count >= STYGIAN_EDITOR_BOOLEAN_COORD_CAP)
    return false;
  values[*count] = value;
  *count += 1u;
  return true;
}

static bool editor_boolean_rect_from_node(const StygianEditor *editor,
                                          StygianEditorNodeId node_id,
                                          EditorBooleanRect *out_rect) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
  if (!node || !out_rect)
    return false;
  if (!editor_node_bounds_world(editor, node, &x, &y, &w, &h))
    return false;
  if (w <= 0.0001f || h <= 0.0001f)
    return false;
  out_rect->x0 = x;
  out_rect->y0 = y;
  out_rect->x1 = x + w;
  out_rect->y1 = y + h;
  return true;
}

static bool editor_boolean_node_result_for_cell(StygianEditorBooleanOp op,
                                                const bool *inside,
                                                uint32_t count) {
  uint32_t i;
  switch (op) {
  case STYGIAN_EDITOR_BOOLEAN_UNION:
    for (i = 0u; i < count; ++i) {
      if (inside[i])
        return true;
    }
    return false;
  case STYGIAN_EDITOR_BOOLEAN_INTERSECT:
    for (i = 0u; i < count; ++i) {
      if (!inside[i])
        return false;
    }
    return count > 0u;
  case STYGIAN_EDITOR_BOOLEAN_SUBTRACT:
    if (count == 0u || !inside[0])
      return false;
    for (i = 1u; i < count; ++i) {
      if (inside[i])
        return false;
    }
    return true;
  case STYGIAN_EDITOR_BOOLEAN_EXCLUDE: {
    bool value = false;
    for (i = 0u; i < count; ++i)
      value = value ^ inside[i];
    return value;
  }
  default:
    return false;
  }
}

static void editor_boolean_write_rect_points(StygianEditorNode *node, float x0,
                                             float y0, float x1, float y1) {
  if (!node)
    return;
  node->boolean_solved_point_count = 4u;
  node->boolean_solved_points[0].x = x0;
  node->boolean_solved_points[0].y = y0;
  node->boolean_solved_points[1].x = x1;
  node->boolean_solved_points[1].y = y0;
  node->boolean_solved_points[2].x = x1;
  node->boolean_solved_points[2].y = y1;
  node->boolean_solved_points[3].x = x0;
  node->boolean_solved_points[3].y = y1;
  for (uint32_t i = 0u; i < 4u; ++i) {
    node->boolean_solved_points[i].kind = STYGIAN_EDITOR_PATH_POINT_CORNER;
    node->boolean_solved_points[i].in_x = node->boolean_solved_points[i].x;
    node->boolean_solved_points[i].in_y = node->boolean_solved_points[i].y;
    node->boolean_solved_points[i].out_x = node->boolean_solved_points[i].x;
    node->boolean_solved_points[i].out_y = node->boolean_solved_points[i].y;
  }
}

static bool editor_boolean_solve_node(StygianEditor *editor,
                                      StygianEditorNode *node) {
  EditorBooleanRect rects[STYGIAN_EDITOR_NODE_BOOLEAN_OPERAND_CAP];
  float xs[STYGIAN_EDITOR_BOOLEAN_COORD_CAP];
  float ys[STYGIAN_EDITOR_BOOLEAN_COORD_CAP];
  bool inside[STYGIAN_EDITOR_NODE_BOOLEAN_OPERAND_CAP];
  bool occupied[(STYGIAN_EDITOR_BOOLEAN_COORD_CAP - 1u) *
                (STYGIAN_EDITOR_BOOLEAN_COORD_CAP - 1u)];
  int16_t component[(STYGIAN_EDITOR_BOOLEAN_COORD_CAP - 1u) *
                    (STYGIAN_EDITOR_BOOLEAN_COORD_CAP - 1u)];
  uint32_t queue[(STYGIAN_EDITOR_BOOLEAN_COORD_CAP - 1u) *
                 (STYGIAN_EDITOR_BOOLEAN_COORD_CAP - 1u)];
  float component_area[(STYGIAN_EDITOR_BOOLEAN_COORD_CAP - 1u) *
                       (STYGIAN_EDITOR_BOOLEAN_COORD_CAP - 1u)];
  EditorBooleanEdge edges[STYGIAN_EDITOR_BOOLEAN_EDGE_CAP];
  uint16_t best_loop_x[STYGIAN_EDITOR_BOOLEAN_EDGE_CAP];
  uint16_t best_loop_y[STYGIAN_EDITOR_BOOLEAN_EDGE_CAP];
  uint32_t x_count = 0u;
  uint32_t y_count = 0u;
  uint32_t rect_count = 0u;
  float union_min_x = 0.0f;
  float union_min_y = 0.0f;
  float union_max_x = 0.0f;
  float union_max_y = 0.0f;
  uint32_t cell_w;
  uint32_t cell_h;
  uint32_t cell_count;
  uint32_t component_count = 0u;
  int16_t best_component = -1;
  float best_component_area = -1.0f;
  uint32_t edge_count = 0u;
  uint32_t best_loop_count = 0u;
  float best_loop_area = -1.0f;
  float out_min_x = 0.0f;
  float out_min_y = 0.0f;
  float out_max_x = 0.0f;
  float out_max_y = 0.0f;
  bool out_bounds_set = false;
  uint32_t i;

  if (!editor || !node || !node->boolean_valid ||
      node->boolean_operand_count < 1u) {
    return false;
  }

  memset(occupied, 0, sizeof(occupied));
  for (i = 0u; i < (uint32_t)(sizeof(component) / sizeof(component[0])); ++i)
    component[i] = -1;
  memset(component_area, 0, sizeof(component_area));

  for (i = 0u; i < node->boolean_operand_count &&
                i < STYGIAN_EDITOR_NODE_BOOLEAN_OPERAND_CAP;
       ++i) {
    if (!editor_boolean_rect_from_node(editor, node->boolean_operands[i],
                                       &rects[rect_count])) {
      return false;
    }
    if (rect_count == 0u) {
      union_min_x = rects[0].x0;
      union_min_y = rects[0].y0;
      union_max_x = rects[0].x1;
      union_max_y = rects[0].y1;
    } else {
      if (rects[rect_count].x0 < union_min_x)
        union_min_x = rects[rect_count].x0;
      if (rects[rect_count].y0 < union_min_y)
        union_min_y = rects[rect_count].y0;
      if (rects[rect_count].x1 > union_max_x)
        union_max_x = rects[rect_count].x1;
      if (rects[rect_count].y1 > union_max_y)
        union_max_y = rects[rect_count].y1;
    }
    if (!editor_boolean_coord_push_unique(xs, &x_count, rects[rect_count].x0) ||
        !editor_boolean_coord_push_unique(xs, &x_count, rects[rect_count].x1) ||
        !editor_boolean_coord_push_unique(ys, &y_count, rects[rect_count].y0) ||
        !editor_boolean_coord_push_unique(ys, &y_count, rects[rect_count].y1)) {
      return false;
    }
    rect_count += 1u;
  }

  if (rect_count == 0u)
    return false;
  if (x_count < 2u || y_count < 2u)
    return false;

  qsort(xs, x_count, sizeof(float), editor_boolean_float_compare);
  qsort(ys, y_count, sizeof(float), editor_boolean_float_compare);
  cell_w = x_count - 1u;
  cell_h = y_count - 1u;
  cell_count = cell_w * cell_h;
  if (cell_count == 0u ||
      cell_count > (uint32_t)(sizeof(occupied) / sizeof(occupied[0])))
    return false;

  for (uint32_t yi = 0u; yi < cell_h; ++yi) {
    for (uint32_t xi = 0u; xi < cell_w; ++xi) {
      const uint32_t idx = yi * cell_w + xi;
      const float cx = (xs[xi] + xs[xi + 1u]) * 0.5f;
      const float cy = (ys[yi] + ys[yi + 1u]) * 0.5f;
      bool take = false;
      for (i = 0u; i < rect_count; ++i)
        inside[i] = editor_boolean_rect_contains(&rects[i], cx, cy);
      take = editor_boolean_node_result_for_cell(node->boolean_op, inside,
                                                 rect_count);
      occupied[idx] = take;
    }
  }

  for (uint32_t idx = 0u; idx < cell_count; ++idx) {
    uint32_t q_head = 0u;
    uint32_t q_tail = 0u;
    if (!occupied[idx] || component[idx] >= 0)
      continue;
    if (component_count >= cell_count)
      return false;
    component_area[component_count] = 0.0f;
    component[idx] = (int16_t)component_count;
    queue[q_tail++] = idx;
    while (q_head < q_tail) {
      const uint32_t cur = queue[q_head++];
      const uint32_t cx = cur % cell_w;
      const uint32_t cy = cur / cell_w;
      const float cw = xs[cx + 1u] - xs[cx];
      const float ch = ys[cy + 1u] - ys[cy];
      component_area[component_count] += cw * ch;
      if (cx > 0u) {
        const uint32_t n = cur - 1u;
        if (occupied[n] && component[n] < 0) {
          component[n] = (int16_t)component_count;
          queue[q_tail++] = n;
        }
      }
      if (cx + 1u < cell_w) {
        const uint32_t n = cur + 1u;
        if (occupied[n] && component[n] < 0) {
          component[n] = (int16_t)component_count;
          queue[q_tail++] = n;
        }
      }
      if (cy > 0u) {
        const uint32_t n = cur - cell_w;
        if (occupied[n] && component[n] < 0) {
          component[n] = (int16_t)component_count;
          queue[q_tail++] = n;
        }
      }
      if (cy + 1u < cell_h) {
        const uint32_t n = cur + cell_w;
        if (occupied[n] && component[n] < 0) {
          component[n] = (int16_t)component_count;
          queue[q_tail++] = n;
        }
      }
    }
    if (component_area[component_count] > best_component_area) {
      best_component_area = component_area[component_count];
      best_component = (int16_t)component_count;
    }
    component_count += 1u;
  }

  if (best_component < 0 || best_component_area <= 0.0001f) {
    node->as.group.x = union_min_x;
    node->as.group.y = union_min_y;
    node->as.group.w = 0.0f;
    node->as.group.h = 0.0f;
    node->boolean_solved_point_count = 0u;
    return true;
  }

  for (uint32_t yi = 0u; yi < cell_h; ++yi) {
    for (uint32_t xi = 0u; xi < cell_w; ++xi) {
      const uint32_t idx = yi * cell_w + xi;
      bool top_open = false;
      bool right_open = false;
      bool bottom_open = false;
      bool left_open = false;
      if (component[idx] != best_component)
        continue;

      if (!out_bounds_set) {
        out_min_x = xs[xi];
        out_min_y = ys[yi];
        out_max_x = xs[xi + 1u];
        out_max_y = ys[yi + 1u];
        out_bounds_set = true;
      } else {
        if (xs[xi] < out_min_x)
          out_min_x = xs[xi];
        if (ys[yi] < out_min_y)
          out_min_y = ys[yi];
        if (xs[xi + 1u] > out_max_x)
          out_max_x = xs[xi + 1u];
        if (ys[yi + 1u] > out_max_y)
          out_max_y = ys[yi + 1u];
      }

      top_open = (yi == 0u) || (component[idx - cell_w] != best_component);
      right_open =
          (xi + 1u >= cell_w) || (component[idx + 1u] != best_component);
      bottom_open =
          (yi + 1u >= cell_h) || (component[idx + cell_w] != best_component);
      left_open = (xi == 0u) || (component[idx - 1u] != best_component);

      if (top_open) {
        if (edge_count >= STYGIAN_EDITOR_BOOLEAN_EDGE_CAP)
          return false;
        edges[edge_count++] =
            (EditorBooleanEdge){(uint16_t)xi, (uint16_t)yi, (uint16_t)(xi + 1u),
                                (uint16_t)yi, false};
      }
      if (right_open) {
        if (edge_count >= STYGIAN_EDITOR_BOOLEAN_EDGE_CAP)
          return false;
        edges[edge_count++] = (EditorBooleanEdge){(uint16_t)(xi + 1u),
                                                  (uint16_t)yi,
                                                  (uint16_t)(xi + 1u),
                                                  (uint16_t)(yi + 1u), false};
      }
      if (bottom_open) {
        if (edge_count >= STYGIAN_EDITOR_BOOLEAN_EDGE_CAP)
          return false;
        edges[edge_count++] = (EditorBooleanEdge){
            (uint16_t)(xi + 1u), (uint16_t)(yi + 1u), (uint16_t)xi,
            (uint16_t)(yi + 1u), false};
      }
      if (left_open) {
        if (edge_count >= STYGIAN_EDITOR_BOOLEAN_EDGE_CAP)
          return false;
        edges[edge_count++] =
            (EditorBooleanEdge){(uint16_t)xi, (uint16_t)(yi + 1u),
                                (uint16_t)xi, (uint16_t)yi, false};
      }
    }
  }

  for (i = 0u; i < edge_count; ++i) {
    uint16_t loop_x[STYGIAN_EDITOR_BOOLEAN_EDGE_CAP];
    uint16_t loop_y[STYGIAN_EDITOR_BOOLEAN_EDGE_CAP];
    uint32_t loop_count = 0u;
    uint32_t cursor = i;
    uint16_t start_x;
    uint16_t start_y;
    uint16_t cur_x;
    uint16_t cur_y;
    uint32_t guard = 0u;
    float area2 = 0.0f;

    if (edges[i].used)
      continue;

    start_x = edges[cursor].x0;
    start_y = edges[cursor].y0;
    cur_x = edges[cursor].x1;
    cur_y = edges[cursor].y1;
    edges[cursor].used = true;
    loop_x[loop_count] = start_x;
    loop_y[loop_count] = start_y;
    loop_count += 1u;

    while (!(cur_x == start_x && cur_y == start_y)) {
      bool found = false;
      if (loop_count >= STYGIAN_EDITOR_BOOLEAN_EDGE_CAP)
        break;
      loop_x[loop_count] = cur_x;
      loop_y[loop_count] = cur_y;
      loop_count += 1u;
      for (uint32_t e = 0u; e < edge_count; ++e) {
        if (!edges[e].used && edges[e].x0 == cur_x && edges[e].y0 == cur_y) {
          edges[e].used = true;
          cur_x = edges[e].x1;
          cur_y = edges[e].y1;
          found = true;
          break;
        }
      }
      if (!found)
        break;
      guard += 1u;
      if (guard > edge_count + 2u)
        break;
    }

    if (!(cur_x == start_x && cur_y == start_y))
      continue;
    if (loop_count < 3u)
      continue;

    for (uint32_t p = 0u; p < loop_count; ++p) {
      const uint32_t n = (p + 1u) % loop_count;
      const float x0 = xs[loop_x[p]];
      const float y0 = ys[loop_y[p]];
      const float x1 = xs[loop_x[n]];
      const float y1 = ys[loop_y[n]];
      area2 += x0 * y1 - x1 * y0;
    }
    area2 = editor_absf(area2);
    if (area2 > best_loop_area) {
      best_loop_area = area2;
      best_loop_count = loop_count;
      memcpy(best_loop_x, loop_x, sizeof(uint16_t) * loop_count);
      memcpy(best_loop_y, loop_y, sizeof(uint16_t) * loop_count);
    }
  }

  if (!out_bounds_set)
    return false;

  node->as.group.x = out_min_x;
  node->as.group.y = out_min_y;
  node->as.group.w = out_max_x - out_min_x;
  node->as.group.h = out_max_y - out_min_y;

  if (best_loop_count >= 3u) {
    bool changed = true;
    while (changed && best_loop_count > 3u) {
      changed = false;
      for (i = 0u; i < best_loop_count; ++i) {
        const uint32_t a = (i + best_loop_count - 1u) % best_loop_count;
        const uint32_t b = i;
        const uint32_t c = (i + 1u) % best_loop_count;
        const bool same_x =
            best_loop_x[a] == best_loop_x[b] && best_loop_x[b] == best_loop_x[c];
        const bool same_y =
            best_loop_y[a] == best_loop_y[b] && best_loop_y[b] == best_loop_y[c];
        if (same_x || same_y) {
          if (i + 1u < best_loop_count) {
            memmove(&best_loop_x[i], &best_loop_x[i + 1u],
                    sizeof(uint16_t) * (best_loop_count - i - 1u));
            memmove(&best_loop_y[i], &best_loop_y[i + 1u],
                    sizeof(uint16_t) * (best_loop_count - i - 1u));
          }
          best_loop_count -= 1u;
          changed = true;
          break;
        }
      }
    }
  }

  if (best_loop_count >= 3u &&
      best_loop_count <= STYGIAN_EDITOR_BOOLEAN_SOLVED_POINT_CAP) {
    node->boolean_solved_point_count = best_loop_count;
    for (i = 0u; i < best_loop_count; ++i) {
      node->boolean_solved_points[i].x = xs[best_loop_x[i]];
      node->boolean_solved_points[i].y = ys[best_loop_y[i]];
      node->boolean_solved_points[i].kind = STYGIAN_EDITOR_PATH_POINT_CORNER;
      node->boolean_solved_points[i].in_x = node->boolean_solved_points[i].x;
      node->boolean_solved_points[i].in_y = node->boolean_solved_points[i].y;
      node->boolean_solved_points[i].out_x = node->boolean_solved_points[i].x;
      node->boolean_solved_points[i].out_y = node->boolean_solved_points[i].y;
    }
  } else {
    editor_boolean_write_rect_points(node, out_min_x, out_min_y, out_max_x,
                                     out_max_y);
  }
  return true;
}

static void editor_boolean_refresh_all(StygianEditor *editor) {
  uint32_t i;
  if (!editor)
    return;
  for (i = 0u; i < editor->node_count; ++i) {
    StygianEditorNode *node = &editor->nodes[i];
    if (!node->boolean_valid)
      continue;
    (void)editor_boolean_solve_node(editor, node);
  }
}

static bool editor_node_intersects_rect(const StygianEditor *editor,
                                        const StygianEditorNode *node, float x,
                                        float y, float w, float h) {
  float nx = 0.0f;
  float ny = 0.0f;
  float nw = 0.0f;
  float nh = 0.0f;
  if (!node)
    return false;
  if (!editor_node_bounds_world(editor, node, &nx, &ny, &nw, &nh))
    return false;
  return editor_rects_intersect(nx, ny, nw, nh, x, y, w, h);
}

static bool editor_node_bounds(const StygianEditorNode *node, float *out_x,
                               float *out_y, float *out_w, float *out_h) {
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;

  if (!node)
    return false;

  switch (node->kind) {
  case STYGIAN_EDITOR_SHAPE_RECT:
    x = node->as.rect.x;
    y = node->as.rect.y;
    w = node->as.rect.w;
    h = node->as.rect.h;
    break;
  case STYGIAN_EDITOR_SHAPE_ELLIPSE:
    x = node->as.ellipse.x;
    y = node->as.ellipse.y;
    w = node->as.ellipse.w;
    h = node->as.ellipse.h;
    break;
  case STYGIAN_EDITOR_SHAPE_PATH:
    x = node->as.path.min_x;
    y = node->as.path.min_y;
    w = node->as.path.max_x - node->as.path.min_x;
    h = node->as.path.max_y - node->as.path.min_y;
    break;
  case STYGIAN_EDITOR_SHAPE_FRAME:
    x = node->as.frame.x;
    y = node->as.frame.y;
    w = node->as.frame.w;
    h = node->as.frame.h;
    break;
  case STYGIAN_EDITOR_SHAPE_TEXT:
    x = node->as.text.x;
    y = node->as.text.y;
    w = node->as.text.w;
    h = node->as.text.h;
    break;
  case STYGIAN_EDITOR_SHAPE_IMAGE:
    x = node->as.image.x;
    y = node->as.image.y;
    w = node->as.image.w;
    h = node->as.image.h;
    break;
  case STYGIAN_EDITOR_SHAPE_LINE:
    x = node->as.line.x1 < node->as.line.x2 ? node->as.line.x1 : node->as.line.x2;
    y = node->as.line.y1 < node->as.line.y2 ? node->as.line.y1 : node->as.line.y2;
    w = editor_absf(node->as.line.x2 - node->as.line.x1);
    h = editor_absf(node->as.line.y2 - node->as.line.y1);
    break;
  case STYGIAN_EDITOR_SHAPE_ARROW:
    x = node->as.arrow.x1 < node->as.arrow.x2 ? node->as.arrow.x1
                                               : node->as.arrow.x2;
    y = node->as.arrow.y1 < node->as.arrow.y2 ? node->as.arrow.y1
                                               : node->as.arrow.y2;
    w = editor_absf(node->as.arrow.x2 - node->as.arrow.x1);
    h = editor_absf(node->as.arrow.y2 - node->as.arrow.y1);
    break;
  case STYGIAN_EDITOR_SHAPE_POLYGON:
    x = node->as.polygon.x;
    y = node->as.polygon.y;
    w = node->as.polygon.w;
    h = node->as.polygon.h;
    break;
  case STYGIAN_EDITOR_SHAPE_STAR:
    x = node->as.star.x;
    y = node->as.star.y;
    w = node->as.star.w;
    h = node->as.star.h;
    break;
  case STYGIAN_EDITOR_SHAPE_ARC:
    x = node->as.arc.x;
    y = node->as.arc.y;
    w = node->as.arc.w;
    h = node->as.arc.h;
    break;
  case STYGIAN_EDITOR_SHAPE_GROUP:
    x = node->as.group.x;
    y = node->as.group.y;
    w = node->as.group.w;
    h = node->as.group.h;
    break;
  case STYGIAN_EDITOR_SHAPE_COMPONENT_DEF:
    x = node->as.component_def.x;
    y = node->as.component_def.y;
    w = node->as.component_def.w;
    h = node->as.component_def.h;
    break;
  case STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE:
    x = node->as.component_instance.x;
    y = node->as.component_instance.y;
    w = node->as.component_instance.w;
    h = node->as.component_instance.h;
    break;
  default:
    return false;
  }

  if (w < 0.0f) {
    x += w;
    w = -w;
  }
  if (h < 0.0f) {
    y += h;
    h = -h;
  }

  if (out_x)
    *out_x = x;
  if (out_y)
    *out_y = y;
  if (out_w)
    *out_w = w;
  if (out_h)
    *out_h = h;
  return true;
}

static bool editor_node_bounds_world(const StygianEditor *editor,
                                     const StygianEditorNode *node, float *out_x,
                                     float *out_y, float *out_w, float *out_h) {
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
  float ox = 0.0f;
  float oy = 0.0f;
  if (!editor_node_bounds(node, &x, &y, &w, &h))
    return false;
  editor_node_world_offset(editor, node, &ox, &oy);
  if (out_x)
    *out_x = x + ox;
  if (out_y)
    *out_y = y + oy;
  if (out_w)
    *out_w = w;
  if (out_h)
    *out_h = h;
  return true;
}

static float editor_dist_sq_point_segment(float px, float py, float ax, float ay,
                                          float bx, float by) {
  float abx = bx - ax;
  float aby = by - ay;
  float apx = px - ax;
  float apy = py - ay;
  float denom = abx * abx + aby * aby;
  float t = 0.0f;
  float cx;
  float cy;
  float dx;
  float dy;

  if (denom > 0.0f)
    t = (apx * abx + apy * aby) / denom;
  t = editor_clampf(t, 0.0f, 1.0f);

  cx = ax + abx * t;
  cy = ay + aby * t;
  dx = px - cx;
  dy = py - cy;
  return dx * dx + dy * dy;
}

static bool editor_node_hit_test(const StygianEditor *editor,
                                 const StygianEditorNode *node, float x,
                                 float y) {
  float local_x = x;
  float local_y = y;
  float ox = 0.0f;
  float oy = 0.0f;
  float bx;
  float by;
  float bw;
  float bh;

  if (!editor || !node || !node->visible)
    return false;
  editor_node_world_offset(editor, node, &ox, &oy);
  local_x -= ox;
  local_y -= oy;

  switch (node->kind) {
  case STYGIAN_EDITOR_SHAPE_RECT:
    return editor_node_bounds(node, &bx, &by, &bw, &bh) &&
           local_x >= bx && local_x <= (bx + bw) && local_y >= by &&
           local_y <= (by + bh);

  case STYGIAN_EDITOR_SHAPE_ELLIPSE: {
    float cx;
    float cy;
    float rx;
    float ry;
    float nx;
    float ny;
    if (!editor_node_bounds(node, &bx, &by, &bw, &bh))
      return false;
    cx = bx + bw * 0.5f;
    cy = by + bh * 0.5f;
    rx = bw * 0.5f;
    ry = bh * 0.5f;
    if (rx <= 0.0f || ry <= 0.0f)
      return false;
    nx = (local_x - cx) / rx;
    ny = (local_y - cy) / ry;
    return nx * nx + ny * ny <= 1.0f;
  }

  case STYGIAN_EDITOR_SHAPE_PATH: {
    uint32_t i;
    float tolerance_world;
    float best_dist_sq;
    if (node->as.path.point_count < 2u)
      return false;

    tolerance_world =
        (node->as.path.thickness * 0.5f) + (3.0f / editor_safe_zoom(editor));
    best_dist_sq = tolerance_world * tolerance_world;

    for (i = 1; i < node->as.path.point_count; ++i) {
      StygianEditorPoint a =
          editor->path_points[node->as.path.first_point + i - 1u];
      StygianEditorPoint b = editor->path_points[node->as.path.first_point + i];
      float d = editor_dist_sq_point_segment(local_x, local_y, a.x, a.y, b.x, b.y);
      if (d <= best_dist_sq)
        return true;
    }

    if (node->as.path.closed && node->as.path.point_count > 2u) {
      StygianEditorPoint a =
          editor->path_points[node->as.path.first_point + node->as.path.point_count -
                              1u];
      StygianEditorPoint b = editor->path_points[node->as.path.first_point];
      float d = editor_dist_sq_point_segment(local_x, local_y, a.x, a.y, b.x, b.y);
      if (d <= best_dist_sq)
        return true;
    }
    return false;
  }

  case STYGIAN_EDITOR_SHAPE_LINE: {
    float thickness = node->as.line.thickness;
    float d = editor_dist_sq_point_segment(local_x, local_y, node->as.line.x1, node->as.line.y1,
                                           node->as.line.x2, node->as.line.y2);
    float r = (thickness * 0.5f) + (3.0f / editor_safe_zoom(editor));
    return d <= (r * r);
  }

  case STYGIAN_EDITOR_SHAPE_ARROW: {
    float thickness = node->as.arrow.thickness;
    float d =
        editor_dist_sq_point_segment(local_x, local_y, node->as.arrow.x1, node->as.arrow.y1,
                                     node->as.arrow.x2, node->as.arrow.y2);
    float r = (thickness * 0.5f) + (3.0f / editor_safe_zoom(editor));
    return d <= (r * r);
  }

  default:
    return editor_node_bounds(node, &bx, &by, &bw, &bh) && local_x >= bx &&
           local_x <= (bx + bw) && local_y >= by && local_y <= (by + bh);
  }
}

static void editor_recompute_path_bounds(StygianEditor *editor,
                                         StygianEditorNode *node) {
  uint32_t i;
  float min_x;
  float min_y;
  float max_x;
  float max_y;

  if (!editor || !node || node->kind != STYGIAN_EDITOR_SHAPE_PATH ||
      node->as.path.point_count == 0u)
    return;

  min_x = editor->path_points[node->as.path.first_point].x;
  min_y = editor->path_points[node->as.path.first_point].y;
  max_x = min_x;
  max_y = min_y;

  for (i = 1; i < node->as.path.point_count; ++i) {
    StygianEditorPoint p = editor->path_points[node->as.path.first_point + i];
    if (p.x < min_x)
      min_x = p.x;
    if (p.y < min_y)
      min_y = p.y;
    if (p.x > max_x)
      max_x = p.x;
    if (p.y > max_y)
      max_y = p.y;
  }

  node->as.path.min_x = min_x;
  node->as.path.min_y = min_y;
  node->as.path.max_x = max_x;
  node->as.path.max_y = max_y;
}

static bool editor_node_get_property(const StygianEditor *editor,
                                     const StygianEditorNode *node,
                                     StygianEditorPropertyKind property,
                                     float *out_value) {
  if (!editor || !node || !out_value)
    return false;

  switch (property) {
  case STYGIAN_EDITOR_PROP_X:
    switch (node->kind) {
    case STYGIAN_EDITOR_SHAPE_RECT:
      *out_value = node->as.rect.x;
      break;
    case STYGIAN_EDITOR_SHAPE_ELLIPSE:
      *out_value = node->as.ellipse.x;
      break;
    case STYGIAN_EDITOR_SHAPE_PATH:
      *out_value = node->as.path.min_x;
      break;
    case STYGIAN_EDITOR_SHAPE_FRAME:
      *out_value = node->as.frame.x;
      break;
    case STYGIAN_EDITOR_SHAPE_TEXT:
      *out_value = node->as.text.x;
      break;
    case STYGIAN_EDITOR_SHAPE_IMAGE:
      *out_value = node->as.image.x;
      break;
    case STYGIAN_EDITOR_SHAPE_LINE:
      *out_value = node->as.line.x1;
      break;
    case STYGIAN_EDITOR_SHAPE_ARROW:
      *out_value = node->as.arrow.x1;
      break;
    case STYGIAN_EDITOR_SHAPE_POLYGON:
      *out_value = node->as.polygon.x;
      break;
    case STYGIAN_EDITOR_SHAPE_STAR:
      *out_value = node->as.star.x;
      break;
    case STYGIAN_EDITOR_SHAPE_ARC:
      *out_value = node->as.arc.x;
      break;
    case STYGIAN_EDITOR_SHAPE_GROUP:
      *out_value = node->as.group.x;
      break;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_DEF:
      *out_value = node->as.component_def.x;
      break;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE:
      *out_value = node->as.component_instance.x;
      break;
    default:
      return false;
    }
    return true;

  case STYGIAN_EDITOR_PROP_Y:
    switch (node->kind) {
    case STYGIAN_EDITOR_SHAPE_RECT:
      *out_value = node->as.rect.y;
      break;
    case STYGIAN_EDITOR_SHAPE_ELLIPSE:
      *out_value = node->as.ellipse.y;
      break;
    case STYGIAN_EDITOR_SHAPE_PATH:
      *out_value = node->as.path.min_y;
      break;
    case STYGIAN_EDITOR_SHAPE_FRAME:
      *out_value = node->as.frame.y;
      break;
    case STYGIAN_EDITOR_SHAPE_TEXT:
      *out_value = node->as.text.y;
      break;
    case STYGIAN_EDITOR_SHAPE_IMAGE:
      *out_value = node->as.image.y;
      break;
    case STYGIAN_EDITOR_SHAPE_LINE:
      *out_value = node->as.line.y1;
      break;
    case STYGIAN_EDITOR_SHAPE_ARROW:
      *out_value = node->as.arrow.y1;
      break;
    case STYGIAN_EDITOR_SHAPE_POLYGON:
      *out_value = node->as.polygon.y;
      break;
    case STYGIAN_EDITOR_SHAPE_STAR:
      *out_value = node->as.star.y;
      break;
    case STYGIAN_EDITOR_SHAPE_ARC:
      *out_value = node->as.arc.y;
      break;
    case STYGIAN_EDITOR_SHAPE_GROUP:
      *out_value = node->as.group.y;
      break;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_DEF:
      *out_value = node->as.component_def.y;
      break;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE:
      *out_value = node->as.component_instance.y;
      break;
    default:
      return false;
    }
    return true;

  case STYGIAN_EDITOR_PROP_WIDTH:
    switch (node->kind) {
    case STYGIAN_EDITOR_SHAPE_RECT:
      *out_value = node->as.rect.w;
      break;
    case STYGIAN_EDITOR_SHAPE_ELLIPSE:
      *out_value = node->as.ellipse.w;
      break;
    case STYGIAN_EDITOR_SHAPE_PATH:
      *out_value = node->as.path.max_x - node->as.path.min_x;
      break;
    case STYGIAN_EDITOR_SHAPE_FRAME:
      *out_value = node->as.frame.w;
      break;
    case STYGIAN_EDITOR_SHAPE_TEXT:
      *out_value = node->as.text.w;
      break;
    case STYGIAN_EDITOR_SHAPE_IMAGE:
      *out_value = node->as.image.w;
      break;
    case STYGIAN_EDITOR_SHAPE_LINE:
      *out_value = node->as.line.x2 - node->as.line.x1;
      break;
    case STYGIAN_EDITOR_SHAPE_ARROW:
      *out_value = node->as.arrow.x2 - node->as.arrow.x1;
      break;
    case STYGIAN_EDITOR_SHAPE_POLYGON:
      *out_value = node->as.polygon.w;
      break;
    case STYGIAN_EDITOR_SHAPE_STAR:
      *out_value = node->as.star.w;
      break;
    case STYGIAN_EDITOR_SHAPE_ARC:
      *out_value = node->as.arc.w;
      break;
    case STYGIAN_EDITOR_SHAPE_GROUP:
      *out_value = node->as.group.w;
      break;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_DEF:
      *out_value = node->as.component_def.w;
      break;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE:
      *out_value = node->as.component_instance.w;
      break;
    default:
      return false;
    }
    return true;

  case STYGIAN_EDITOR_PROP_HEIGHT:
    switch (node->kind) {
    case STYGIAN_EDITOR_SHAPE_RECT:
      *out_value = node->as.rect.h;
      break;
    case STYGIAN_EDITOR_SHAPE_ELLIPSE:
      *out_value = node->as.ellipse.h;
      break;
    case STYGIAN_EDITOR_SHAPE_PATH:
      *out_value = node->as.path.max_y - node->as.path.min_y;
      break;
    case STYGIAN_EDITOR_SHAPE_FRAME:
      *out_value = node->as.frame.h;
      break;
    case STYGIAN_EDITOR_SHAPE_TEXT:
      *out_value = node->as.text.h;
      break;
    case STYGIAN_EDITOR_SHAPE_IMAGE:
      *out_value = node->as.image.h;
      break;
    case STYGIAN_EDITOR_SHAPE_LINE:
      *out_value = node->as.line.y2 - node->as.line.y1;
      break;
    case STYGIAN_EDITOR_SHAPE_ARROW:
      *out_value = node->as.arrow.y2 - node->as.arrow.y1;
      break;
    case STYGIAN_EDITOR_SHAPE_POLYGON:
      *out_value = node->as.polygon.h;
      break;
    case STYGIAN_EDITOR_SHAPE_STAR:
      *out_value = node->as.star.h;
      break;
    case STYGIAN_EDITOR_SHAPE_ARC:
      *out_value = node->as.arc.h;
      break;
    case STYGIAN_EDITOR_SHAPE_GROUP:
      *out_value = node->as.group.h;
      break;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_DEF:
      *out_value = node->as.component_def.h;
      break;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE:
      *out_value = node->as.component_instance.h;
      break;
    default:
      return false;
    }
    return true;

  case STYGIAN_EDITOR_PROP_OPACITY:
    if (editor_node_supports_fill(node->kind)) {
      uint32_t i;
      for (i = 0u; i < node->fill_count && i < STYGIAN_EDITOR_NODE_FILL_CAP; ++i) {
        if (node->fills[i].visible) {
          *out_value = node->fills[i].opacity;
          return true;
        }
      }
      *out_value = editor_node_get_legacy_fill_color(node).a;
      return true;
    }
    switch (node->kind) {
    case STYGIAN_EDITOR_SHAPE_PATH:
      *out_value = node->as.path.stroke.a;
      break;
    case STYGIAN_EDITOR_SHAPE_LINE:
      *out_value = node->as.line.stroke.a;
      break;
    case STYGIAN_EDITOR_SHAPE_ARROW:
      *out_value = node->as.arrow.stroke.a;
      break;
    case STYGIAN_EDITOR_SHAPE_ARC:
      *out_value = node->as.arc.stroke.a;
      break;
    default:
      return false;
    }
    return true;

  case STYGIAN_EDITOR_PROP_RADIUS_TL:
    if (node->kind != STYGIAN_EDITOR_SHAPE_RECT)
      return false;
    *out_value = node->as.rect.radius[0];
    return true;

  case STYGIAN_EDITOR_PROP_RADIUS_TR:
    if (node->kind != STYGIAN_EDITOR_SHAPE_RECT)
      return false;
    *out_value = node->as.rect.radius[1];
    return true;

  case STYGIAN_EDITOR_PROP_RADIUS_BR:
    if (node->kind != STYGIAN_EDITOR_SHAPE_RECT)
      return false;
    *out_value = node->as.rect.radius[2];
    return true;

  case STYGIAN_EDITOR_PROP_RADIUS_BL:
    if (node->kind != STYGIAN_EDITOR_SHAPE_RECT)
      return false;
    *out_value = node->as.rect.radius[3];
    return true;

  case STYGIAN_EDITOR_PROP_VALUE:
    *out_value = node->value;
    return true;
  case STYGIAN_EDITOR_PROP_ROTATION_DEG:
    *out_value = node->rotation_deg;
    return true;

  default:
    return false;
  }
}

static bool editor_node_apply_property(StygianEditor *editor,
                                       StygianEditorNode *node,
                                       StygianEditorPropertyKind property,
                                       float value) {
  if (!editor || !node)
    return false;

  switch (property) {
  case STYGIAN_EDITOR_PROP_X:
    if (node->kind == STYGIAN_EDITOR_SHAPE_PATH) {
      float delta = value - node->as.path.min_x;
      uint32_t i;
      for (i = 0; i < node->as.path.point_count; ++i) {
        editor->path_points[node->as.path.first_point + i].x += delta;
      }
      node->as.path.min_x += delta;
      node->as.path.max_x += delta;
      return true;
    }
    switch (node->kind) {
    case STYGIAN_EDITOR_SHAPE_RECT:
      node->as.rect.x = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_ELLIPSE:
      node->as.ellipse.x = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_FRAME:
      node->as.frame.x = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_TEXT:
      node->as.text.x = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_IMAGE:
      node->as.image.x = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_LINE: {
      float dx = value - node->as.line.x1;
      node->as.line.x1 += dx;
      node->as.line.x2 += dx;
      return true;
    }
    case STYGIAN_EDITOR_SHAPE_ARROW: {
      float dx = value - node->as.arrow.x1;
      node->as.arrow.x1 += dx;
      node->as.arrow.x2 += dx;
      return true;
    }
    case STYGIAN_EDITOR_SHAPE_POLYGON:
      node->as.polygon.x = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_STAR:
      node->as.star.x = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_ARC:
      node->as.arc.x = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_GROUP:
      node->as.group.x = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_DEF:
      node->as.component_def.x = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE:
      node->as.component_instance.x = value;
      return true;
    default:
      return false;
    }

  case STYGIAN_EDITOR_PROP_Y:
    if (node->kind == STYGIAN_EDITOR_SHAPE_PATH) {
      float delta = value - node->as.path.min_y;
      uint32_t i;
      for (i = 0; i < node->as.path.point_count; ++i) {
        editor->path_points[node->as.path.first_point + i].y += delta;
      }
      node->as.path.min_y += delta;
      node->as.path.max_y += delta;
      return true;
    }
    switch (node->kind) {
    case STYGIAN_EDITOR_SHAPE_RECT:
      node->as.rect.y = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_ELLIPSE:
      node->as.ellipse.y = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_FRAME:
      node->as.frame.y = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_TEXT:
      node->as.text.y = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_IMAGE:
      node->as.image.y = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_LINE: {
      float dy = value - node->as.line.y1;
      node->as.line.y1 += dy;
      node->as.line.y2 += dy;
      return true;
    }
    case STYGIAN_EDITOR_SHAPE_ARROW: {
      float dy = value - node->as.arrow.y1;
      node->as.arrow.y1 += dy;
      node->as.arrow.y2 += dy;
      return true;
    }
    case STYGIAN_EDITOR_SHAPE_POLYGON:
      node->as.polygon.y = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_STAR:
      node->as.star.y = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_ARC:
      node->as.arc.y = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_GROUP:
      node->as.group.y = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_DEF:
      node->as.component_def.y = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE:
      node->as.component_instance.y = value;
      return true;
    default:
      return false;
    }

  case STYGIAN_EDITOR_PROP_WIDTH:
    value = editor_node_clamp_width(node, value);
    if (node->kind == STYGIAN_EDITOR_SHAPE_PATH) {
      float old_w = node->as.path.max_x - node->as.path.min_x;
      uint32_t i;
      if (old_w <= 0.000001f)
        return false;
      for (i = 0; i < node->as.path.point_count; ++i) {
        StygianEditorPoint *p =
            &editor->path_points[node->as.path.first_point + i];
        p->x =
            node->as.path.min_x + ((p->x - node->as.path.min_x) * value / old_w);
      }
      editor_recompute_path_bounds(editor, node);
      return true;
    }
    switch (node->kind) {
    case STYGIAN_EDITOR_SHAPE_RECT:
      node->as.rect.w = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_ELLIPSE:
      node->as.ellipse.w = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_FRAME:
      node->as.frame.w = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_TEXT:
      node->as.text.w = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_IMAGE:
      node->as.image.w = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_LINE:
      node->as.line.x2 = node->as.line.x1 + value;
      return true;
    case STYGIAN_EDITOR_SHAPE_ARROW:
      node->as.arrow.x2 = node->as.arrow.x1 + value;
      return true;
    case STYGIAN_EDITOR_SHAPE_POLYGON:
      node->as.polygon.w = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_STAR:
      node->as.star.w = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_ARC:
      node->as.arc.w = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_GROUP:
      node->as.group.w = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_DEF:
      node->as.component_def.w = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE:
      node->as.component_instance.w = value;
      return true;
    default:
      return false;
    }

  case STYGIAN_EDITOR_PROP_HEIGHT:
    value = editor_node_clamp_height(node, value);
    if (node->kind == STYGIAN_EDITOR_SHAPE_PATH) {
      float old_h = node->as.path.max_y - node->as.path.min_y;
      uint32_t i;
      if (old_h <= 0.000001f)
        return false;
      for (i = 0; i < node->as.path.point_count; ++i) {
        StygianEditorPoint *p =
            &editor->path_points[node->as.path.first_point + i];
        p->y =
            node->as.path.min_y + ((p->y - node->as.path.min_y) * value / old_h);
      }
      editor_recompute_path_bounds(editor, node);
      return true;
    }
    switch (node->kind) {
    case STYGIAN_EDITOR_SHAPE_RECT:
      node->as.rect.h = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_ELLIPSE:
      node->as.ellipse.h = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_FRAME:
      node->as.frame.h = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_TEXT:
      node->as.text.h = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_IMAGE:
      node->as.image.h = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_LINE:
      node->as.line.y2 = node->as.line.y1 + value;
      return true;
    case STYGIAN_EDITOR_SHAPE_ARROW:
      node->as.arrow.y2 = node->as.arrow.y1 + value;
      return true;
    case STYGIAN_EDITOR_SHAPE_POLYGON:
      node->as.polygon.h = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_STAR:
      node->as.star.h = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_ARC:
      node->as.arc.h = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_GROUP:
      node->as.group.h = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_DEF:
      node->as.component_def.h = value;
      return true;
    case STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE:
      node->as.component_instance.h = value;
      return true;
    default:
      return false;
    }

  case STYGIAN_EDITOR_PROP_OPACITY:
    value = editor_clampf(value, 0.0f, 1.0f);
    if (editor_node_supports_fill(node->kind)) {
      uint32_t i;
      editor_node_fill_sync_from_legacy(node);
      for (i = 0u; i < node->fill_count && i < STYGIAN_EDITOR_NODE_FILL_CAP; ++i) {
        node->fills[i].opacity = value;
      }
      if (node->fill_count > 0u && node->fills[0].kind == STYGIAN_EDITOR_FILL_SOLID)
        editor_node_set_legacy_fill_color(node, editor_node_primary_fill_color(node));
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_PATH)
      node->as.path.stroke.a = value;
    else if (node->kind == STYGIAN_EDITOR_SHAPE_LINE)
      node->as.line.stroke.a = value;
    else if (node->kind == STYGIAN_EDITOR_SHAPE_ARROW)
      node->as.arrow.stroke.a = value;
    else if (node->kind == STYGIAN_EDITOR_SHAPE_ARC)
      node->as.arc.stroke.a = value;
    else
      return false;
    return true;

  case STYGIAN_EDITOR_PROP_RADIUS_TL:
    if (node->kind != STYGIAN_EDITOR_SHAPE_RECT)
      return false;
    node->as.rect.radius[0] = value;
    return true;

  case STYGIAN_EDITOR_PROP_RADIUS_TR:
    if (node->kind != STYGIAN_EDITOR_SHAPE_RECT)
      return false;
    node->as.rect.radius[1] = value;
    return true;

  case STYGIAN_EDITOR_PROP_RADIUS_BR:
    if (node->kind != STYGIAN_EDITOR_SHAPE_RECT)
      return false;
    node->as.rect.radius[2] = value;
    return true;

  case STYGIAN_EDITOR_PROP_RADIUS_BL:
    if (node->kind != STYGIAN_EDITOR_SHAPE_RECT)
      return false;
    node->as.rect.radius[3] = value;
    return true;

  case STYGIAN_EDITOR_PROP_VALUE:
    node->value = value;
    return true;
  case STYGIAN_EDITOR_PROP_ROTATION_DEG:
    node->rotation_deg = value;
    return true;

  default:
    return false;
  }
}

static bool editor_property_descriptor(StygianEditorPropertyKind kind,
                                       StygianEditorPropertyDescriptor *out_desc) {
  StygianEditorPropertyDescriptor desc;
  memset(&desc, 0, sizeof(desc));
  desc.kind = kind;
  desc.writable = true;
  desc.animatable = true;
  desc.supports_mixed = true;

  switch (kind) {
  case STYGIAN_EDITOR_PROP_X:
  case STYGIAN_EDITOR_PROP_Y:
  case STYGIAN_EDITOR_PROP_VALUE:
  case STYGIAN_EDITOR_PROP_ROTATION_DEG:
    desc.value_type = STYGIAN_EDITOR_VALUE_FLOAT;
    break;
  case STYGIAN_EDITOR_PROP_WIDTH:
  case STYGIAN_EDITOR_PROP_HEIGHT:
  case STYGIAN_EDITOR_PROP_RADIUS_TL:
  case STYGIAN_EDITOR_PROP_RADIUS_TR:
  case STYGIAN_EDITOR_PROP_RADIUS_BR:
  case STYGIAN_EDITOR_PROP_RADIUS_BL:
    desc.value_type = STYGIAN_EDITOR_VALUE_FLOAT;
    desc.has_numeric_range = true;
    desc.min_value = 0.0f;
    desc.max_value = 1000000.0f;
    break;
  case STYGIAN_EDITOR_PROP_OPACITY:
    desc.value_type = STYGIAN_EDITOR_VALUE_FLOAT;
    desc.has_numeric_range = true;
    desc.min_value = 0.0f;
    desc.max_value = 1.0f;
    break;
  case STYGIAN_EDITOR_PROP_VISIBLE:
    desc.value_type = STYGIAN_EDITOR_VALUE_BOOL;
    desc.animatable = false;
    break;
  case STYGIAN_EDITOR_PROP_FILL_COLOR:
    desc.value_type = STYGIAN_EDITOR_VALUE_COLOR;
    break;
  case STYGIAN_EDITOR_PROP_COLOR_TOKEN:
    desc.value_type = STYGIAN_EDITOR_VALUE_STRING;
    desc.animatable = false;
    break;
  case STYGIAN_EDITOR_PROP_SHAPE_KIND:
    desc.value_type = STYGIAN_EDITOR_VALUE_ENUM;
    desc.writable = false;
    desc.animatable = false;
    break;
  default:
    return false;
  }

  if (out_desc)
    *out_desc = desc;
  return true;
}

static bool editor_color_valid(StygianEditorColor color) {
  return color.r >= 0.0f && color.r <= 1.0f && color.g >= 0.0f &&
         color.g <= 1.0f && color.b >= 0.0f && color.b <= 1.0f &&
         color.a >= 0.0f && color.a <= 1.0f;
}

static bool editor_property_value_valid(StygianEditorPropertyKind kind,
                                        const StygianEditorPropertyValue *value) {
  StygianEditorPropertyDescriptor desc;
  if (!value || !editor_property_descriptor(kind, &desc))
    return false;
  if (value->type != desc.value_type)
    return false;
  if (desc.has_numeric_range && desc.value_type == STYGIAN_EDITOR_VALUE_FLOAT) {
    if (value->as.number < desc.min_value || value->as.number > desc.max_value)
      return false;
  }
  if (kind == STYGIAN_EDITOR_PROP_FILL_COLOR)
    return editor_color_valid(value->as.color);
  if (kind == STYGIAN_EDITOR_PROP_COLOR_TOKEN)
    return strlen(value->as.string) < 32u;
  return true;
}

static bool editor_node_supports_property(const StygianEditor *editor,
                                          const StygianEditorNode *node,
                                          StygianEditorPropertyKind property) {
  float dummy = 0.0f;
  StygianEditorColor color;
  if (!editor || !node)
    return false;
  switch (property) {
  case STYGIAN_EDITOR_PROP_VISIBLE:
  case STYGIAN_EDITOR_PROP_SHAPE_KIND:
    return true;
  case STYGIAN_EDITOR_PROP_FILL_COLOR:
  case STYGIAN_EDITOR_PROP_COLOR_TOKEN:
    return stygian_editor_node_get_color(editor, node->id, &color);
  default:
    return editor_node_get_property(editor, node, property, &dummy);
  }
}

static bool editor_property_values_equal(const StygianEditorPropertyValue *a,
                                         const StygianEditorPropertyValue *b) {
  if (!a || !b || a->type != b->type)
    return false;
  switch (a->type) {
  case STYGIAN_EDITOR_VALUE_FLOAT:
    return fabsf(a->as.number - b->as.number) <= 0.0001f;
  case STYGIAN_EDITOR_VALUE_BOOL:
    return a->as.boolean == b->as.boolean;
  case STYGIAN_EDITOR_VALUE_COLOR:
    return fabsf(a->as.color.r - b->as.color.r) <= 0.0001f &&
           fabsf(a->as.color.g - b->as.color.g) <= 0.0001f &&
           fabsf(a->as.color.b - b->as.color.b) <= 0.0001f &&
           fabsf(a->as.color.a - b->as.color.a) <= 0.0001f;
  case STYGIAN_EDITOR_VALUE_STRING:
    return strcmp(a->as.string, b->as.string) == 0;
  case STYGIAN_EDITOR_VALUE_ENUM:
    return a->as.enum_value == b->as.enum_value;
  default:
    return false;
  }
}

static float editor_ease(StygianEditorEasing easing, float t) {
  t = editor_clampf(t, 0.0f, 1.0f);
  switch (easing) {
  case STYGIAN_EDITOR_EASING_OUT_CUBIC: {
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
  }
  case STYGIAN_EDITOR_EASING_IN_OUT_CUBIC:
    if (t < 0.5f)
      return 4.0f * t * t * t;
    else {
      float inv = -2.0f * t + 2.0f;
      return 1.0f - (inv * inv * inv) * 0.5f;
    }
  case STYGIAN_EDITOR_EASING_LINEAR:
  default:
    return t;
  }
}

static int editor_find_color_token(const StygianEditor *editor, const char *name) {
  uint32_t i;
  if (!editor || !name || !name[0])
    return -1;
  for (i = 0; i < editor->color_token_count; ++i) {
    if (strncmp(editor->color_tokens[i].name, name,
                sizeof(editor->color_tokens[i].name)) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static bool editor_copy_token_name(char *dst, size_t dst_cap,
                                   const char *name) {
  size_t len;
  if (!dst || dst_cap == 0u || !name || !name[0])
    return false;
  len = strlen(name);
  if (len >= dst_cap)
    return false;
  memcpy(dst, name, len + 1u);
  return true;
}

static bool editor_copy_short_name(char *dst, size_t dst_cap,
                                   const char *name) {
  size_t len = 0u;
  if (!dst || dst_cap == 0u)
    return false;
  if (!name || !name[0]) {
    dst[0] = '\0';
    return true;
  }
  len = strlen(name);
  if (len >= dst_cap)
    return false;
  memcpy(dst, name, len + 1u);
  return true;
}

static int editor_find_text_style(const StygianEditor *editor, const char *name) {
  uint32_t i;
  if (!editor || !name || !name[0])
    return -1;
  for (i = 0u; i < editor->text_style_count; ++i) {
    if (strncmp(editor->text_styles[i].def.name, name,
                sizeof(editor->text_styles[i].def.name)) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static int editor_find_effect_style(const StygianEditor *editor,
                                    const char *name) {
  uint32_t i;
  if (!editor || !name || !name[0])
    return -1;
  for (i = 0u; i < editor->effect_style_count; ++i) {
    if (strncmp(editor->effect_styles[i].def.name, name,
                sizeof(editor->effect_styles[i].def.name)) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static int editor_find_layout_style(const StygianEditor *editor,
                                    const char *name) {
  uint32_t i;
  if (!editor || !name || !name[0])
    return -1;
  for (i = 0u; i < editor->layout_style_count; ++i) {
    if (strncmp(editor->layout_styles[i].def.name, name,
                sizeof(editor->layout_styles[i].def.name)) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static int editor_find_variable(const StygianEditor *editor, const char *name,
                                StygianEditorVariableKind kind) {
  uint32_t i;
  if (!editor || !name || !name[0])
    return -1;
  for (i = 0u; i < editor->variable_count; ++i) {
    const StygianEditorVariableDef *def = &editor->variables[i].def;
    if (def->kind != kind)
      continue;
    if (strncmp(def->name, name, sizeof(def->name)) == 0)
      return (int)i;
  }
  return -1;
}

static int editor_find_guide_index(const StygianEditor *editor, uint32_t guide_id) {
  uint32_t i;
  if (!editor || guide_id == 0u)
    return -1;
  for (i = 0u; i < editor->guide_count; ++i) {
    if (editor->guides[i].id == guide_id)
      return (int)i;
  }
  return -1;
}

static int editor_find_timeline_track_index(const StygianEditor *editor,
                                            StygianEditorTimelineTrackId id) {
  uint32_t i;
  if (!editor || id == STYGIAN_EDITOR_INVALID_ID)
    return -1;
  for (i = 0u; i < editor->timeline_track_count; ++i) {
    if (editor->timeline_tracks[i].id == id)
      return (int)i;
  }
  return -1;
}

static int editor_find_timeline_clip_index(const StygianEditor *editor,
                                           StygianEditorTimelineClipId id) {
  uint32_t i;
  if (!editor || id == STYGIAN_EDITOR_INVALID_ID)
    return -1;
  for (i = 0u; i < editor->timeline_clip_count; ++i) {
    if (editor->timeline_clips[i].id == id)
      return (int)i;
  }
  return -1;
}

static int editor_find_driver_index(const StygianEditor *editor,
                                    StygianEditorDriverId id) {
  uint32_t i;
  if (!editor || id == STYGIAN_EDITOR_INVALID_ID)
    return -1;
  for (i = 0u; i < editor->driver_count; ++i) {
    if (editor->drivers[i].id == id)
      return (int)i;
  }
  return -1;
}

static float editor_driver_map_value(const StygianEditorDriverRule *rule,
                                     float source_value) {
  float t;
  float out_value;
  if (!rule)
    return source_value;
  if (fabsf(rule->in_max - rule->in_min) < 0.0001f)
    return rule->out_min;
  t = (source_value - rule->in_min) / (rule->in_max - rule->in_min);
  if (rule->clamp_output) {
    if (t < 0.0f)
      t = 0.0f;
    if (t > 1.0f)
      t = 1.0f;
  }
  out_value = rule->out_min + (rule->out_max - rule->out_min) * t;
  return out_value;
}

static int editor_timeline_keyframe_compare(const void *a, const void *b) {
  const StygianEditorTimelineKeyframe *ka =
      (const StygianEditorTimelineKeyframe *)a;
  const StygianEditorTimelineKeyframe *kb =
      (const StygianEditorTimelineKeyframe *)b;
  if (ka->time_ms < kb->time_ms)
    return -1;
  if (ka->time_ms > kb->time_ms)
    return 1;
  return 0;
}

StygianEditorConfig stygian_editor_config_default(void) {
  StygianEditorConfig cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.max_nodes = 4096u;
  cfg.max_path_points = 32768u;
  cfg.max_behaviors = 2048u;
  cfg.max_color_tokens = 256u;
  cfg.max_timeline_tracks = 1024u;
  cfg.max_timeline_clips = 256u;
  cfg.max_drivers = 512u;
  cfg.grid.enabled = true;
  cfg.grid.sub_snap_enabled = true;
  cfg.grid.major_step_px = 96.0f;
  cfg.grid.sub_divisions = 4u;
  cfg.grid.min_minor_px = 8.0f;
  cfg.grid.snap_tolerance_px = -1.0f;
  return cfg;
}

StygianEditorColor stygian_editor_color_rgba(float r, float g, float b, float a) {
  StygianEditorColor color;
  color.r = r;
  color.g = g;
  color.b = b;
  color.a = a;
  return color;
}

StygianEditor *stygian_editor_create(const StygianEditorConfig *config,
                                     const StygianEditorHost *host) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor;

  if (config)
    cfg = *config;

  if (cfg.max_nodes == 0u)
    cfg.max_nodes = 1u;
  if (cfg.max_path_points == 0u)
    cfg.max_path_points = 1u;
  if (cfg.max_behaviors == 0u)
    cfg.max_behaviors = 1u;
  if (cfg.max_color_tokens == 0u)
    cfg.max_color_tokens = 1u;
  if (cfg.max_timeline_tracks == 0u)
    cfg.max_timeline_tracks = 1u;
  if (cfg.max_timeline_clips == 0u)
    cfg.max_timeline_clips = 1u;
  if (cfg.max_drivers == 0u)
    cfg.max_drivers = 1u;

  editor = (StygianEditor *)calloc(1u, sizeof(StygianEditor));
  if (!editor)
    return NULL;

  editor->nodes = (StygianEditorNode *)calloc(
      (size_t)cfg.max_nodes, sizeof(StygianEditorNode));
  editor->path_points = (StygianEditorPoint *)calloc(
      (size_t)cfg.max_path_points, sizeof(StygianEditorPoint));
  editor->behaviors = (StygianEditorBehaviorSlot *)calloc(
      (size_t)cfg.max_behaviors, sizeof(StygianEditorBehaviorSlot));
  editor->active_anims = (StygianEditorActiveAnimation *)calloc(
      (size_t)cfg.max_behaviors, sizeof(StygianEditorActiveAnimation));
  editor->color_tokens = (StygianEditorColorToken *)calloc(
      (size_t)cfg.max_color_tokens, sizeof(StygianEditorColorToken));
  editor->timeline_tracks = (StygianEditorTimelineTrackSlot *)calloc(
      (size_t)cfg.max_timeline_tracks, sizeof(StygianEditorTimelineTrackSlot));
  editor->timeline_clips = (StygianEditorTimelineClipSlot *)calloc(
      (size_t)cfg.max_timeline_clips, sizeof(StygianEditorTimelineClipSlot));
  editor->drivers = (StygianEditorDriverSlot *)calloc(
      (size_t)cfg.max_drivers, sizeof(StygianEditorDriverSlot));
  editor->guides = (StygianEditorGuideSlot *)calloc(
      (size_t)cfg.max_nodes, sizeof(StygianEditorGuideSlot));
  editor->history_entries = (StygianEditorHistoryEntry *)calloc(
      (size_t)STYGIAN_EDITOR_HISTORY_CAPACITY, sizeof(StygianEditorHistoryEntry));

  if (!editor->nodes || !editor->path_points || !editor->behaviors ||
      !editor->active_anims || !editor->color_tokens || !editor->timeline_tracks ||
      !editor->timeline_clips || !editor->drivers || !editor->guides ||
      !editor->history_entries) {
    stygian_editor_destroy(editor);
    return NULL;
  }

  editor->max_nodes = cfg.max_nodes;
  editor->max_path_points = cfg.max_path_points;
  editor->max_behaviors = cfg.max_behaviors;
  editor->max_color_tokens = cfg.max_color_tokens;
  editor->max_timeline_tracks = cfg.max_timeline_tracks;
  editor->max_timeline_clips = cfg.max_timeline_clips;
  editor->max_drivers = cfg.max_drivers;
  editor->max_guides = cfg.max_nodes;
  editor->grid = cfg.grid;
  editor->viewport.width = 1280.0f;
  editor->viewport.height = 720.0f;
  editor->viewport.pan_x = 0.0f;
  editor->viewport.pan_y = 0.0f;
  editor->viewport.zoom = 1.0f;
  editor->next_node_id = 1u;
  editor->next_path_id = 1u;
  editor->next_behavior_id = 1u;
  editor->next_timeline_track_id = 1u;
  editor->next_timeline_clip_id = 1u;
  editor->next_driver_id = 1u;
  editor->selected_node = STYGIAN_EDITOR_INVALID_ID;
  editor->next_guide_id = 1u;
  editor->ruler_unit = STYGIAN_EDITOR_RULER_UNIT_PX;
  editor->snap_sources.use_grid = true;
  editor->snap_sources.use_guides = true;
  editor->snap_sources.use_bounds = true;
  editor->snap_sources.use_parent_edges = true;
  editor->variable_mode_count = 1u;
  snprintf(editor->variable_modes[0], sizeof(editor->variable_modes[0]),
           "default");
  editor->active_variable_mode = 0u;
  editor->history_capacity = STYGIAN_EDITOR_HISTORY_CAPACITY;

  if (host)
    editor->host = *host;

  return editor;
}

void stygian_editor_destroy(StygianEditor *editor) {
  if (!editor)
    return;
  free(editor->nodes);
  free(editor->path_points);
  free(editor->behaviors);
  free(editor->active_anims);
  free(editor->color_tokens);
  free(editor->timeline_tracks);
  free(editor->timeline_clips);
  free(editor->drivers);
  free(editor->guides);
  editor_history_clear(editor);
  free(editor->history_entries);
  free(editor->transaction_before_json);
  free(editor);
}

void stygian_editor_reset(StygianEditor *editor) {
  if (!editor)
    return;

  editor->node_count = 0u;
  editor->point_count = 0u;
  editor->behavior_count = 0u;
  editor->timeline_track_count = 0u;
  editor->timeline_clip_count = 0u;
  editor->driver_count = 0u;
  editor->color_token_count = 0u;
  editor->guide_count = 0u;
  editor->text_style_count = 0u;
  editor->effect_style_count = 0u;
  editor->layout_style_count = 0u;
  editor->variable_count = 0u;
  memset(editor->variable_modes, 0, sizeof(editor->variable_modes));
  editor->variable_mode_count = 1u;
  snprintf(editor->variable_modes[0], sizeof(editor->variable_modes[0]),
           "default");
  editor->active_variable_mode = 0u;
  editor->selected_node = STYGIAN_EDITOR_INVALID_ID;
  editor->path_builder.active = false;
  editor->next_node_id = 1u;
  editor->next_path_id = 1u;
  editor->next_behavior_id = 1u;
  editor->next_timeline_track_id = 1u;
  editor->next_timeline_clip_id = 1u;
  editor->next_driver_id = 1u;
  editor->next_guide_id = 1u;
  editor->ruler_unit = STYGIAN_EDITOR_RULER_UNIT_PX;
  editor->snap_sources.use_grid = true;
  editor->snap_sources.use_guides = true;
  editor->snap_sources.use_bounds = true;
  editor->snap_sources.use_parent_edges = true;
  editor_history_clear(editor);
  editor_history_drop_transaction(editor);
}

void stygian_editor_set_host(StygianEditor *editor,
                             const StygianEditorHost *host) {
  if (!editor)
    return;
  memset(&editor->host, 0, sizeof(editor->host));
  if (host)
    editor->host = *host;
}

void stygian_editor_set_viewport(StygianEditor *editor,
                                 const StygianEditorViewport2D *viewport) {
  if (!editor || !viewport)
    return;
  editor->viewport = *viewport;
  if (editor->viewport.zoom < 0.0001f)
    editor->viewport.zoom = 0.0001f;
}

StygianEditorViewport2D stygian_editor_get_viewport(const StygianEditor *editor) {
  StygianEditorViewport2D viewport = {0};
  if (!editor)
    return viewport;
  return editor->viewport;
}

void stygian_editor_set_grid_config(StygianEditor *editor,
                                    const StygianEditorGridConfig *grid) {
  if (!editor || !grid)
    return;
  editor->grid = *grid;
  if (editor->grid.sub_divisions == 0u)
    editor->grid.sub_divisions = 1u;
}

StygianEditorGridConfig stygian_editor_get_grid_config(const StygianEditor *editor) {
  StygianEditorGridConfig grid = {0};
  if (!editor)
    return grid;
  return editor->grid;
}

uint32_t stygian_editor_get_grid_levels(const StygianEditor *editor,
                                        StygianEditorGridLevel *out_levels,
                                        uint32_t max_levels) {
  float major_world = 0.0f;
  float minor_world = 0.0f;
  bool minor_visible = false;
  uint32_t count = 0u;

  if (!editor || !editor->grid.enabled || !out_levels || max_levels == 0u)
    return 0u;

  editor_grid_steps(editor, &major_world, &minor_world, &minor_visible);

  if (minor_visible && count < max_levels) {
    out_levels[count].step_world = minor_world;
    out_levels[count].step_screen = minor_world * editor_safe_zoom(editor);
    out_levels[count].alpha = 0.10f;
    out_levels[count].major = false;
    ++count;
  }

  if (count < max_levels) {
    out_levels[count].step_world = major_world;
    out_levels[count].step_screen = major_world * editor_safe_zoom(editor);
    out_levels[count].alpha = 0.24f;
    out_levels[count].major = true;
    ++count;
  }

  return count;
}

void stygian_editor_world_to_view(const StygianEditor *editor, float world_x,
                                  float world_y, float *out_view_x,
                                  float *out_view_y) {
  float zoom = editor_safe_zoom(editor);
  if (out_view_x)
    *out_view_x = world_x * zoom + (editor ? editor->viewport.pan_x : 0.0f);
  if (out_view_y)
    *out_view_y = world_y * zoom + (editor ? editor->viewport.pan_y : 0.0f);
}

void stygian_editor_view_to_world(const StygianEditor *editor, float view_x,
                                  float view_y, float *out_world_x,
                                  float *out_world_y) {
  float zoom = editor_safe_zoom(editor);
  if (out_world_x)
    *out_world_x = (view_x - (editor ? editor->viewport.pan_x : 0.0f)) / zoom;
  if (out_world_y)
    *out_world_y = (view_y - (editor ? editor->viewport.pan_y : 0.0f)) / zoom;
}

float stygian_editor_snap_world_scalar(const StygianEditor *editor,
                                       float world_value) {
  float major_world = 0.0f;
  float minor_world = 0.0f;
  bool minor_visible = false;
  float step;
  float snapped;
  float tolerance_px;
  float delta_px;

  if (!editor || !editor->grid.enabled || !editor->snap_sources.use_grid)
    return world_value;

  editor_grid_steps(editor, &major_world, &minor_world, &minor_visible);
  (void)minor_visible;

  step = major_world;
  if (editor->grid.sub_snap_enabled && editor->grid.sub_divisions > 1u &&
      minor_world > 0.0f) {
    step = minor_world;
  }

  if (step <= 0.000001f)
    return world_value;

  snapped = roundf(world_value / step) * step;
  tolerance_px = editor->grid.snap_tolerance_px;
  if (tolerance_px < 0.0f)
    return snapped;

  delta_px = editor_absf(snapped - world_value) * editor_safe_zoom(editor);
  if (delta_px <= tolerance_px)
    return snapped;

  return world_value;
}

static void editor_snap_try(float source, float candidate, float tolerance_world,
                            float *io_best_delta, float *io_best_value) {
  float delta;
  if (!io_best_delta || !io_best_value)
    return;
  delta = editor_absf(candidate - source);
  if (delta > tolerance_world || delta >= *io_best_delta)
    return;
  *io_best_delta = delta;
  *io_best_value = candidate;
}

void stygian_editor_snap_world_point(const StygianEditor *editor,
                                     float world_x, float world_y,
                                     float *out_world_x, float *out_world_y) {
  float snapped_x = world_x;
  float snapped_y = world_y;
  float best_dx = 1.0e30f;
  float best_dy = 1.0e30f;
  float tolerance_px = 0.0f;
  float tolerance_world = 0.0f;
  if (!editor) {
    if (out_world_x)
      *out_world_x = world_x;
    if (out_world_y)
      *out_world_y = world_y;
    return;
  }

  tolerance_px = editor->grid.snap_tolerance_px;
  if (tolerance_px < 0.0f)
    tolerance_world = 1.0e30f;
  else
    tolerance_world = tolerance_px / editor_safe_zoom(editor);

  if (editor->snap_sources.use_grid && editor->grid.enabled) {
    float gx = stygian_editor_snap_world_scalar(editor, world_x);
    float gy = stygian_editor_snap_world_scalar(editor, world_y);
    editor_snap_try(world_x, gx, tolerance_world, &best_dx, &snapped_x);
    editor_snap_try(world_y, gy, tolerance_world, &best_dy, &snapped_y);
  }

  if (editor->snap_sources.use_guides) {
    uint32_t i;
    for (i = 0u; i < editor->guide_count; ++i) {
      const StygianEditorGuideSlot *guide = &editor->guides[i];
      if (guide->axis == STYGIAN_EDITOR_GUIDE_VERTICAL) {
        editor_snap_try(world_x, guide->position, tolerance_world, &best_dx,
                        &snapped_x);
      } else {
        editor_snap_try(world_y, guide->position, tolerance_world, &best_dy,
                        &snapped_y);
      }
    }
  }

  if (editor->selected_node != STYGIAN_EDITOR_INVALID_ID &&
      (editor->snap_sources.use_bounds || editor->snap_sources.use_parent_edges)) {
    const StygianEditorNode *selected =
        editor_find_node_const(editor, editor->selected_node);
    if (selected) {
      if (editor->snap_sources.use_bounds) {
        uint32_t i;
        for (i = 0u; i < editor->node_count; ++i) {
          const StygianEditorNode *node = &editor->nodes[i];
          float nx = 0.0f, ny = 0.0f, nw = 0.0f, nh = 0.0f;
          if (node->id == selected->id || !node->visible)
            continue;
          if (!editor_node_bounds_world(editor, node, &nx, &ny, &nw, &nh))
            continue;
          editor_snap_try(world_x, nx, tolerance_world, &best_dx, &snapped_x);
          editor_snap_try(world_x, nx + nw * 0.5f, tolerance_world, &best_dx,
                          &snapped_x);
          editor_snap_try(world_x, nx + nw, tolerance_world, &best_dx, &snapped_x);
          editor_snap_try(world_y, ny, tolerance_world, &best_dy, &snapped_y);
          editor_snap_try(world_y, ny + nh * 0.5f, tolerance_world, &best_dy,
                          &snapped_y);
          editor_snap_try(world_y, ny + nh, tolerance_world, &best_dy, &snapped_y);
        }
      }
      if (editor->snap_sources.use_parent_edges &&
          selected->parent_id != STYGIAN_EDITOR_INVALID_ID) {
        const StygianEditorNode *parent =
            editor_find_node_const(editor, selected->parent_id);
        float px = 0.0f, py = 0.0f, pw = 0.0f, ph = 0.0f;
        if (parent && editor_node_bounds_world(editor, parent, &px, &py, &pw, &ph)) {
          editor_snap_try(world_x, px, tolerance_world, &best_dx, &snapped_x);
          editor_snap_try(world_x, px + pw * 0.5f, tolerance_world, &best_dx,
                          &snapped_x);
          editor_snap_try(world_x, px + pw, tolerance_world, &best_dx, &snapped_x);
          editor_snap_try(world_y, py, tolerance_world, &best_dy, &snapped_y);
          editor_snap_try(world_y, py + ph * 0.5f, tolerance_world, &best_dy,
                          &snapped_y);
          editor_snap_try(world_y, py + ph, tolerance_world, &best_dy, &snapped_y);
        }
      }
    }
  }

  if (out_world_x)
    *out_world_x = snapped_x;
  if (out_world_y)
    *out_world_y = snapped_y;
}

void stygian_editor_set_ruler_unit(StygianEditor *editor,
                                   StygianEditorRulerUnit unit) {
  if (!editor)
    return;
  if (unit > STYGIAN_EDITOR_RULER_UNIT_PX)
    return;
  editor->ruler_unit = unit;
}

StygianEditorRulerUnit
stygian_editor_get_ruler_unit(const StygianEditor *editor) {
  if (!editor)
    return STYGIAN_EDITOR_RULER_UNIT_PX;
  return editor->ruler_unit;
}

void stygian_editor_set_snap_sources(StygianEditor *editor,
                                     const StygianEditorSnapSources *sources) {
  if (!editor || !sources)
    return;
  editor->snap_sources = *sources;
}

StygianEditorSnapSources
stygian_editor_get_snap_sources(const StygianEditor *editor) {
  StygianEditorSnapSources sources = {0};
  if (!editor)
    return sources;
  return editor->snap_sources;
}

uint32_t stygian_editor_add_guide(StygianEditor *editor,
                                  const StygianEditorGuide *guide) {
  char *before_json = NULL;
  StygianEditorGuideSlot *slot;
  uint32_t id;
  if (!editor || !guide || editor->guide_count >= editor->max_guides)
    return 0u;
  if (guide->axis > STYGIAN_EDITOR_GUIDE_HORIZONTAL)
    return 0u;
  if (guide->id != 0u && editor_find_guide_index(editor, guide->id) >= 0)
    return 0u;
  if (!editor_begin_mutation(editor, &before_json))
    return 0u;
  slot = &editor->guides[editor->guide_count++];
  memset(slot, 0, sizeof(*slot));
  id = guide->id;
  if (id == 0u)
    id = editor->next_guide_id++;
  if (editor->next_guide_id <= id)
    editor->next_guide_id = id + 1u;
  slot->id = id;
  slot->axis = guide->axis;
  slot->position = guide->position;
  slot->parent_id = guide->parent_id;
  editor_request_repaint(editor, 120u);
  if (!editor_finish_mutation(editor, before_json, true))
    return 0u;
  return id;
}

bool stygian_editor_remove_guide(StygianEditor *editor, uint32_t guide_id) {
  char *before_json = NULL;
  int idx = editor_find_guide_index(editor, guide_id);
  uint32_t i;
  if (!editor || idx < 0)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  for (i = (uint32_t)idx; (i + 1u) < editor->guide_count; ++i)
    editor->guides[i] = editor->guides[i + 1u];
  editor->guide_count -= 1u;
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

uint32_t stygian_editor_guide_count(const StygianEditor *editor) {
  if (!editor)
    return 0u;
  return editor->guide_count;
}

bool stygian_editor_get_guide(const StygianEditor *editor, uint32_t index,
                              StygianEditorGuide *out_guide) {
  const StygianEditorGuideSlot *slot;
  if (!editor || !out_guide || index >= editor->guide_count)
    return false;
  slot = &editor->guides[index];
  out_guide->id = slot->id;
  out_guide->axis = slot->axis;
  out_guide->position = slot->position;
  out_guide->parent_id = slot->parent_id;
  return true;
}

StygianEditorNodeId stygian_editor_add_rect(StygianEditor *editor,
                                            const StygianEditorRectDesc *desc) {
  char *before_json = NULL;
  StygianEditorNodeId id;
  StygianEditorNode *node;
  if (!editor || !desc)
    return STYGIAN_EDITOR_INVALID_ID;
  if (!editor_begin_mutation(editor, &before_json))
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot add rect: node capacity reached (%u).",
                editor->max_nodes);
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }

  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_RECT;
  node->locked = false;
  node->visible = desc->visible;
  node->selected = false;
  node->z = desc->z;
  node->as.rect.x = desc->x;
  node->as.rect.y = desc->y;
  node->as.rect.w = desc->w;
  node->as.rect.h = desc->h;
  node->as.rect.radius[0] = desc->radius[0];
  node->as.rect.radius[1] = desc->radius[1];
  node->as.rect.radius[2] = desc->radius[2];
  node->as.rect.radius[3] = desc->radius[3];
  node->as.rect.fill = desc->fill;
  editor_node_fill_sync_from_legacy(node);
  editor_node_seed_legacy_stroke(node);
  node->constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
  node->constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
  node->layout_absolute = false;
  node->layout_sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->layout_sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->size_min_w = 0.0f;
  node->size_max_w = 0.0f;
  node->size_min_h = 0.0f;
  node->size_max_h = 0.0f;
  editor_set_default_node_name(node);
  editor_constraints_capture(editor, node);
  id = node->id;
  if (!editor_finish_mutation(editor, before_json, true))
    return STYGIAN_EDITOR_INVALID_ID;
  return id;
}

StygianEditorNodeId
stygian_editor_add_ellipse(StygianEditor *editor,
                           const StygianEditorEllipseDesc *desc) {
  char *before_json = NULL;
  StygianEditorNodeId id;
  StygianEditorNode *node;
  if (!editor || !desc)
    return STYGIAN_EDITOR_INVALID_ID;
  if (!editor_begin_mutation(editor, &before_json))
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot add ellipse: node capacity reached (%u).",
                editor->max_nodes);
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }

  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_ELLIPSE;
  node->locked = false;
  node->visible = desc->visible;
  node->selected = false;
  node->z = desc->z;
  node->as.ellipse.x = desc->x;
  node->as.ellipse.y = desc->y;
  node->as.ellipse.w = desc->w;
  node->as.ellipse.h = desc->h;
  node->as.ellipse.fill = desc->fill;
  editor_node_fill_sync_from_legacy(node);
  editor_node_seed_legacy_stroke(node);
  node->constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
  node->constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
  node->layout_absolute = false;
  node->layout_sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->layout_sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->size_min_w = 0.0f;
  node->size_max_w = 0.0f;
  node->size_min_h = 0.0f;
  node->size_max_h = 0.0f;
  editor_set_default_node_name(node);
  editor_constraints_capture(editor, node);
  id = node->id;
  if (!editor_finish_mutation(editor, before_json, true))
    return STYGIAN_EDITOR_INVALID_ID;
  return id;
}

StygianEditorNodeId stygian_editor_add_frame(StygianEditor *editor,
                                             const StygianEditorFrameDesc *desc) {
  char *before_json = NULL;
  StygianEditorNodeId id;
  StygianEditorNode *node;
  if (!editor || !desc)
    return STYGIAN_EDITOR_INVALID_ID;
  if (!editor_begin_mutation(editor, &before_json))
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot add frame: node capacity reached (%u).",
                editor->max_nodes);
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }

  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_FRAME;
  node->locked = false;
  node->visible = desc->visible;
  node->selected = false;
  node->z = desc->z;
  node->as.frame.x = desc->x;
  node->as.frame.y = desc->y;
  node->as.frame.w = desc->w;
  node->as.frame.h = desc->h;
  node->as.frame.clip_content = desc->clip_content;
  node->as.frame.fill = desc->fill;
  node->as.frame.layout_mode = STYGIAN_EDITOR_AUTO_LAYOUT_OFF;
  node->as.frame.layout_wrap = STYGIAN_EDITOR_AUTO_LAYOUT_NO_WRAP;
  node->as.frame.layout_padding_left = 0.0f;
  node->as.frame.layout_padding_right = 0.0f;
  node->as.frame.layout_padding_top = 0.0f;
  node->as.frame.layout_padding_bottom = 0.0f;
  node->as.frame.layout_gap = 0.0f;
  node->as.frame.layout_primary_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  node->as.frame.layout_cross_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  node->as.frame.overflow_policy = STYGIAN_EDITOR_FRAME_OVERFLOW_VISIBLE;
  node->as.frame.scroll_x = 0.0f;
  node->as.frame.scroll_y = 0.0f;
  editor_node_fill_sync_from_legacy(node);
  editor_node_seed_legacy_stroke(node);
  node->constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
  node->constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
  node->layout_absolute = false;
  node->layout_sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->layout_sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->size_min_w = 0.0f;
  node->size_max_w = 0.0f;
  node->size_min_h = 0.0f;
  node->size_max_h = 0.0f;
  editor_set_default_node_name(node);
  editor_constraints_capture(editor, node);
  id = node->id;
  if (!editor_finish_mutation(editor, before_json, true))
    return STYGIAN_EDITOR_INVALID_ID;
  return id;
}

StygianEditorNodeId stygian_editor_add_line(StygianEditor *editor,
                                            const StygianEditorLineDesc *desc) {
  char *before_json = NULL;
  StygianEditorNodeId id;
  StygianEditorNode *node;
  if (!editor || !desc)
    return STYGIAN_EDITOR_INVALID_ID;
  if (!editor_begin_mutation(editor, &before_json))
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot add line: node capacity reached (%u).",
                editor->max_nodes);
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }

  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_LINE;
  node->locked = false;
  node->visible = desc->visible;
  node->selected = false;
  node->z = desc->z;
  node->as.line.x1 = desc->x1;
  node->as.line.y1 = desc->y1;
  node->as.line.x2 = desc->x2;
  node->as.line.y2 = desc->y2;
  node->as.line.thickness = desc->thickness > 0.0f ? desc->thickness : 1.0f;
  node->as.line.stroke = desc->stroke;
  editor_node_seed_legacy_stroke(node);
  node->constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
  node->constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
  node->layout_absolute = false;
  node->layout_sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->layout_sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->size_min_w = 0.0f;
  node->size_max_w = 0.0f;
  node->size_min_h = 0.0f;
  node->size_max_h = 0.0f;
  editor_set_default_node_name(node);
  editor_constraints_capture(editor, node);
  id = node->id;
  if (!editor_finish_mutation(editor, before_json, true))
    return STYGIAN_EDITOR_INVALID_ID;
  return id;
}

StygianEditorNodeId stygian_editor_add_arrow(StygianEditor *editor,
                                             const StygianEditorArrowDesc *desc) {
  char *before_json = NULL;
  StygianEditorNodeId id;
  StygianEditorNode *node;
  if (!editor || !desc)
    return STYGIAN_EDITOR_INVALID_ID;
  if (!editor_begin_mutation(editor, &before_json))
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot add arrow: node capacity reached (%u).",
                editor->max_nodes);
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }

  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_ARROW;
  node->locked = false;
  node->visible = desc->visible;
  node->selected = false;
  node->z = desc->z;
  node->as.arrow.x1 = desc->x1;
  node->as.arrow.y1 = desc->y1;
  node->as.arrow.x2 = desc->x2;
  node->as.arrow.y2 = desc->y2;
  node->as.arrow.thickness = desc->thickness > 0.0f ? desc->thickness : 1.0f;
  node->as.arrow.head_size = desc->head_size > 0.0f ? desc->head_size : 10.0f;
  node->as.arrow.stroke = desc->stroke;
  editor_node_seed_legacy_stroke(node);
  node->constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
  node->constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
  node->layout_absolute = false;
  node->layout_sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->layout_sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->size_min_w = 0.0f;
  node->size_max_w = 0.0f;
  node->size_min_h = 0.0f;
  node->size_max_h = 0.0f;
  editor_set_default_node_name(node);
  editor_constraints_capture(editor, node);
  id = node->id;
  if (!editor_finish_mutation(editor, before_json, true))
    return STYGIAN_EDITOR_INVALID_ID;
  return id;
}

StygianEditorNodeId
stygian_editor_add_polygon(StygianEditor *editor,
                           const StygianEditorPolygonDesc *desc) {
  char *before_json = NULL;
  StygianEditorNodeId id;
  StygianEditorNode *node;
  if (!editor || !desc)
    return STYGIAN_EDITOR_INVALID_ID;
  if (!editor_begin_mutation(editor, &before_json))
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot add polygon: node capacity reached (%u).",
                editor->max_nodes);
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }

  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_POLYGON;
  node->locked = false;
  node->visible = desc->visible;
  node->selected = false;
  node->z = desc->z;
  node->as.polygon.x = desc->x;
  node->as.polygon.y = desc->y;
  node->as.polygon.w = desc->w;
  node->as.polygon.h = desc->h;
  node->as.polygon.sides = desc->sides < 3u ? 3u : desc->sides;
  node->as.polygon.corner_radius = desc->corner_radius;
  node->as.polygon.fill = desc->fill;
  editor_node_fill_sync_from_legacy(node);
  editor_node_seed_legacy_stroke(node);
  node->constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
  node->constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
  node->layout_absolute = false;
  node->layout_sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->layout_sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->size_min_w = 0.0f;
  node->size_max_w = 0.0f;
  node->size_min_h = 0.0f;
  node->size_max_h = 0.0f;
  editor_set_default_node_name(node);
  editor_constraints_capture(editor, node);
  id = node->id;
  if (!editor_finish_mutation(editor, before_json, true))
    return STYGIAN_EDITOR_INVALID_ID;
  return id;
}

StygianEditorNodeId stygian_editor_add_star(StygianEditor *editor,
                                            const StygianEditorStarDesc *desc) {
  char *before_json = NULL;
  StygianEditorNodeId id;
  StygianEditorNode *node;
  float inner_ratio;
  if (!editor || !desc)
    return STYGIAN_EDITOR_INVALID_ID;
  if (!editor_begin_mutation(editor, &before_json))
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot add star: node capacity reached (%u).",
                editor->max_nodes);
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }

  inner_ratio = desc->inner_ratio;
  if (inner_ratio < 0.05f)
    inner_ratio = 0.05f;
  if (inner_ratio > 0.95f)
    inner_ratio = 0.95f;

  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_STAR;
  node->locked = false;
  node->visible = desc->visible;
  node->selected = false;
  node->z = desc->z;
  node->as.star.x = desc->x;
  node->as.star.y = desc->y;
  node->as.star.w = desc->w;
  node->as.star.h = desc->h;
  node->as.star.points = desc->points < 3u ? 5u : desc->points;
  node->as.star.inner_ratio = inner_ratio;
  node->as.star.fill = desc->fill;
  editor_node_fill_sync_from_legacy(node);
  editor_node_seed_legacy_stroke(node);
  node->constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
  node->constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
  node->layout_absolute = false;
  node->layout_sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->layout_sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->size_min_w = 0.0f;
  node->size_max_w = 0.0f;
  node->size_min_h = 0.0f;
  node->size_max_h = 0.0f;
  editor_set_default_node_name(node);
  editor_constraints_capture(editor, node);
  id = node->id;
  if (!editor_finish_mutation(editor, before_json, true))
    return STYGIAN_EDITOR_INVALID_ID;
  return id;
}

StygianEditorNodeId stygian_editor_add_arc(StygianEditor *editor,
                                           const StygianEditorArcDesc *desc) {
  char *before_json = NULL;
  StygianEditorNodeId id;
  StygianEditorNode *node;
  if (!editor || !desc)
    return STYGIAN_EDITOR_INVALID_ID;
  if (!editor_begin_mutation(editor, &before_json))
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot add arc: node capacity reached (%u).",
                editor->max_nodes);
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }

  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_ARC;
  node->locked = false;
  node->visible = desc->visible;
  node->selected = false;
  node->z = desc->z;
  node->as.arc.x = desc->x;
  node->as.arc.y = desc->y;
  node->as.arc.w = desc->w;
  node->as.arc.h = desc->h;
  node->as.arc.start_angle = desc->start_angle;
  node->as.arc.sweep_angle = desc->sweep_angle;
  node->as.arc.thickness = desc->thickness > 0.0f ? desc->thickness : 1.0f;
  node->as.arc.stroke = desc->stroke;
  editor_node_seed_legacy_stroke(node);
  node->constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
  node->constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
  node->layout_absolute = false;
  node->layout_sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->layout_sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->size_min_w = 0.0f;
  node->size_max_w = 0.0f;
  node->size_min_h = 0.0f;
  node->size_max_h = 0.0f;
  editor_set_default_node_name(node);
  editor_constraints_capture(editor, node);
  id = node->id;
  if (!editor_finish_mutation(editor, before_json, true))
    return STYGIAN_EDITOR_INVALID_ID;
  return id;
}

StygianEditorNodeId stygian_editor_add_text(StygianEditor *editor,
                                            const StygianEditorTextDesc *desc) {
  char *before_json = NULL;
  StygianEditorNodeId id;
  StygianEditorNode *node;
  const char *text_value = "";
  size_t text_len = 0u;
  if (!editor || !desc)
    return STYGIAN_EDITOR_INVALID_ID;
  if (!editor_begin_mutation(editor, &before_json))
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot add text: node capacity reached (%u).",
                editor->max_nodes);
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }
  text_value = desc->text ? desc->text : "";
  text_len = strlen(text_value);
  if (text_len >= sizeof(node->as.text.text)) {
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }
  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_TEXT;
  node->locked = false;
  node->visible = desc->visible;
  node->selected = false;
  node->z = desc->z;
  node->as.text.x = desc->x;
  node->as.text.y = desc->y;
  node->as.text.w = desc->w;
  node->as.text.h = desc->h;
  node->as.text.font_size = desc->font_size > 0.0f ? desc->font_size : 16.0f;
  node->as.text.line_height = desc->line_height;
  node->as.text.letter_spacing = desc->letter_spacing;
  node->as.text.font_weight = desc->font_weight;
  node->as.text.box_mode = desc->box_mode;
  node->as.text.align_h = desc->align_h;
  node->as.text.align_v = desc->align_v;
  node->as.text.auto_size = desc->auto_size;
  node->as.text.fill = desc->fill;
  memcpy(node->as.text.text, text_value, text_len + 1u);
  editor_node_fill_sync_from_legacy(node);
  editor_node_seed_legacy_stroke(node);
  editor_text_apply_auto_size(node);
  node->constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
  node->constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
  node->layout_absolute = false;
  node->layout_sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->layout_sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->size_min_w = 0.0f;
  node->size_max_w = 0.0f;
  node->size_min_h = 0.0f;
  node->size_max_h = 0.0f;
  editor_set_default_node_name(node);
  editor_constraints_capture(editor, node);
  id = node->id;
  if (!editor_finish_mutation(editor, before_json, true))
    return STYGIAN_EDITOR_INVALID_ID;
  return id;
}

StygianEditorNodeId stygian_editor_add_image(StygianEditor *editor,
                                             const StygianEditorImageDesc *desc) {
  char *before_json = NULL;
  StygianEditorNodeId id;
  StygianEditorNode *node;
  const char *source = "";
  size_t source_len = 0u;
  if (!editor || !desc)
    return STYGIAN_EDITOR_INVALID_ID;
  if (!editor_begin_mutation(editor, &before_json))
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot add image: node capacity reached (%u).",
                editor->max_nodes);
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }
  source = desc->source ? desc->source : "";
  source_len = strlen(source);
  if (source_len >= sizeof(node->as.image.source)) {
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }
  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_IMAGE;
  node->locked = false;
  node->visible = desc->visible;
  node->selected = false;
  node->z = desc->z;
  node->as.image.x = desc->x;
  node->as.image.y = desc->y;
  node->as.image.w = desc->w;
  node->as.image.h = desc->h;
  node->as.image.fit_mode = desc->fit_mode;
  memcpy(node->as.image.source, source, source_len + 1u);
  node->constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
  node->constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
  node->layout_absolute = false;
  node->layout_sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->layout_sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->size_min_w = 0.0f;
  node->size_max_w = 0.0f;
  node->size_min_h = 0.0f;
  node->size_max_h = 0.0f;
  editor_set_default_node_name(node);
  editor_constraints_capture(editor, node);
  id = node->id;
  if (!editor_finish_mutation(editor, before_json, true))
    return STYGIAN_EDITOR_INVALID_ID;
  return id;
}

StygianEditorNodeId stygian_editor_add_component_def(
    StygianEditor *editor, const StygianEditorComponentDefDesc *desc) {
  char *before_json = NULL;
  StygianEditorNodeId id;
  StygianEditorNode *node;
  const char *symbol = NULL;
  size_t symbol_len = 0u;
  if (!editor || !desc)
    return STYGIAN_EDITOR_INVALID_ID;
  if (!editor_begin_mutation(editor, &before_json))
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot add component def: node capacity reached (%u).",
                editor->max_nodes);
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }
  symbol = desc->symbol ? desc->symbol : "";
  symbol_len = strlen(symbol);
  if (symbol_len >= sizeof(node->as.component_def.symbol)) {
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }
  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_COMPONENT_DEF;
  node->locked = false;
  node->visible = desc->visible;
  node->selected = false;
  node->z = desc->z;
  node->as.component_def.x = desc->x;
  node->as.component_def.y = desc->y;
  node->as.component_def.w = desc->w;
  node->as.component_def.h = desc->h;
  if (symbol_len == 0u) {
    snprintf(node->as.component_def.symbol, sizeof(node->as.component_def.symbol),
             "component_%u", node->id);
  } else {
    memcpy(node->as.component_def.symbol, symbol, symbol_len + 1u);
  }
  node->constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
  node->constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
  node->layout_absolute = false;
  node->layout_sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->layout_sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->size_min_w = 0.0f;
  node->size_max_w = 0.0f;
  node->size_min_h = 0.0f;
  node->size_max_h = 0.0f;
  editor_set_default_node_name(node);
  editor_constraints_capture(editor, node);
  id = node->id;
  if (!editor_finish_mutation(editor, before_json, true))
    return STYGIAN_EDITOR_INVALID_ID;
  return id;
}

StygianEditorNodeId stygian_editor_add_component_instance(
    StygianEditor *editor, const StygianEditorComponentInstanceDesc *desc) {
  char *before_json = NULL;
  StygianEditorNodeId id;
  StygianEditorNode *node;
  const StygianEditorNode *def = NULL;
  if (!editor || !desc)
    return STYGIAN_EDITOR_INVALID_ID;
  if (!editor_begin_mutation(editor, &before_json))
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot add component instance: node capacity reached (%u).",
                editor->max_nodes);
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }
  def = editor_find_node_const(editor, desc->component_def_id);
  if (!editor_is_component_def(def)) {
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }
  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE;
  node->locked = false;
  node->visible = desc->visible;
  node->selected = false;
  node->z = desc->z;
  node->as.component_instance.x = desc->x;
  node->as.component_instance.y = desc->y;
  node->as.component_instance.w = desc->w > 0.0f ? desc->w : def->as.component_def.w;
  node->as.component_instance.h = desc->h > 0.0f ? desc->h : def->as.component_def.h;
  node->as.component_instance.component_def_id = desc->component_def_id;
  (void)editor_component_symbol_copy(node->as.component_instance.symbol_ref,
                                     sizeof(node->as.component_instance.symbol_ref),
                                     def->as.component_def.symbol);
  node->component_detached = false;
  node->constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
  node->constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
  node->layout_absolute = false;
  node->layout_sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->layout_sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->size_min_w = 0.0f;
  node->size_max_w = 0.0f;
  node->size_min_h = 0.0f;
  node->size_max_h = 0.0f;
  editor_set_default_node_name(node);
  editor_constraints_capture(editor, node);
  id = node->id;
  if (!editor_finish_mutation(editor, before_json, true))
    return STYGIAN_EDITOR_INVALID_ID;
  return id;
}

bool stygian_editor_component_def_get_symbol(
    const StygianEditor *editor, StygianEditorNodeId component_def_id,
    char *out_symbol, size_t out_symbol_capacity) {
  const StygianEditorNode *def = editor_find_node_const(editor, component_def_id);
  size_t len = 0u;
  if (!editor_is_component_def(def) || !out_symbol || out_symbol_capacity == 0u)
    return false;
  len = strlen(def->as.component_def.symbol);
  if (len >= out_symbol_capacity)
    return false;
  memcpy(out_symbol, def->as.component_def.symbol, len + 1u);
  return true;
}

bool stygian_editor_component_def_set_symbol(
    StygianEditor *editor, StygianEditorNodeId component_def_id,
    const char *symbol) {
  char *before_json = NULL;
  StygianEditorNode *def = editor_find_node(editor, component_def_id);
  char old_symbol[64];
  size_t symbol_len = 0u;
  uint32_t i;
  if (!editor_is_component_def(def) || !symbol || !symbol[0])
    return false;
  symbol_len = strlen(symbol);
  if (symbol_len >= sizeof(def->as.component_def.symbol))
    return false;
  (void)editor_component_symbol_copy(old_symbol, sizeof(old_symbol),
                                     def->as.component_def.symbol);
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  if (!editor_component_symbol_copy(def->as.component_def.symbol,
                                    sizeof(def->as.component_def.symbol),
                                    symbol)) {
    return editor_finish_mutation(editor, before_json, false);
  }
  for (i = 0u; i < editor->node_count; ++i) {
    StygianEditorNode *node = &editor->nodes[i];
    if (!editor_is_component_instance(node))
      continue;
    if (node->as.component_instance.component_def_id == component_def_id ||
        (old_symbol[0] &&
         strncmp(node->as.component_instance.symbol_ref, old_symbol,
                 sizeof(node->as.component_instance.symbol_ref)) == 0)) {
      (void)editor_component_symbol_copy(
          node->as.component_instance.symbol_ref,
          sizeof(node->as.component_instance.symbol_ref), symbol);
    }
  }
  editor_component_apply_definition_internal(editor, component_def_id);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_component_instance_get_def(
    const StygianEditor *editor, StygianEditorNodeId instance_id,
    StygianEditorNodeId *out_component_def_id) {
  const StygianEditorNode *instance = editor_find_node_const(editor, instance_id);
  const StygianEditorNode *def = NULL;
  if (!instance || !editor_is_component_instance(instance) || !out_component_def_id)
    return false;
  def = editor_component_def_for_instance(editor, instance);
  if (!def)
    return false;
  *out_component_def_id = def->id;
  return true;
}

bool stygian_editor_component_instance_set_def(
    StygianEditor *editor, StygianEditorNodeId instance_id,
    StygianEditorNodeId component_def_id) {
  char *before_json = NULL;
  StygianEditorNode *instance = editor_find_node(editor, instance_id);
  const StygianEditorNode *def = editor_find_node_const(editor, component_def_id);
  const StygianEditorNode *current_def = NULL;
  uint32_t i;
  if (!instance || !editor_is_component_instance(instance) || !editor_is_component_def(def))
    return false;
  current_def = editor_component_def_for_instance(editor, instance);
  if (current_def && !stygian_editor_component_defs_compatible(editor, current_def->id,
                                                               component_def_id)) {
    return false;
  }
  for (i = 0u; i < instance->as.component_instance.overrides.property_override_count &&
              i < STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP;
       ++i) {
    if (!editor_component_property_value_valid_for_instance_context(
            editor, instance, def,
            &instance->as.component_instance.overrides.property_overrides[i])) {
      return false;
    }
  }
  if (instance->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  instance->as.component_instance.component_def_id = component_def_id;
  (void)editor_component_symbol_copy(instance->as.component_instance.symbol_ref,
                                     sizeof(instance->as.component_instance.symbol_ref),
                                     def->as.component_def.symbol);
  instance->component_detached = false;
  instance->as.component_instance.w = def->as.component_def.w;
  instance->as.component_instance.h = def->as.component_def.h;
  instance->visible = def->visible;
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_component_apply_definition(
    StygianEditor *editor, StygianEditorNodeId component_def_id) {
  char *before_json = NULL;
  const StygianEditorNode *def = editor_find_node_const(editor, component_def_id);
  if (!editor_is_component_def(def))
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  editor_component_apply_definition_internal(editor, component_def_id);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_component_instance_set_override_state(
    StygianEditor *editor, StygianEditorNodeId instance_id,
    const StygianEditorComponentOverrideState *state) {
  char *before_json = NULL;
  StygianEditorNode *instance = editor_find_node(editor, instance_id);
  const StygianEditorNode *def = NULL;
  uint32_t i;
  StygianEditorNodeId resolved = STYGIAN_EDITOR_INVALID_ID;
  if (!instance || !editor_is_component_instance(instance) || !state)
    return false;
  if (instance->locked)
    return false;
  def = editor_component_def_for_instance(editor, instance);
  if (!def)
    return false;
  for (i = 0u; i < state->property_override_count &&
              i < STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP;
       ++i) {
    if (!editor_component_property_value_valid_for_instance_context(
            editor, instance, def, &state->property_overrides[i])) {
      return false;
    }
  }
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  instance->as.component_instance.overrides = *state;
  if (!editor_component_resolve_instance_definition(
          editor, instance, def->id, &instance->as.component_instance.overrides,
          &resolved)) {
    return editor_finish_mutation(editor, before_json, false);
  }
  def = editor_find_node_const(editor, resolved);
  if (!editor_is_component_def(def))
    return editor_finish_mutation(editor, before_json, false);
  instance->as.component_instance.component_def_id = def->id;
  (void)editor_component_symbol_copy(
      instance->as.component_instance.symbol_ref,
      sizeof(instance->as.component_instance.symbol_ref),
      def->as.component_def.symbol);
  if (state->mask & STYGIAN_EDITOR_COMPONENT_OVERRIDE_VISIBLE)
    instance->visible = state->visible;
  if (state->mask & STYGIAN_EDITOR_COMPONENT_OVERRIDE_X)
    instance->as.component_instance.x = state->x;
  if (state->mask & STYGIAN_EDITOR_COMPONENT_OVERRIDE_Y)
    instance->as.component_instance.y = state->y;
  if (state->mask & STYGIAN_EDITOR_COMPONENT_OVERRIDE_W)
    instance->as.component_instance.w = editor_node_clamp_width(instance, state->w);
  if (state->mask & STYGIAN_EDITOR_COMPONENT_OVERRIDE_H)
    instance->as.component_instance.h = editor_node_clamp_height(instance, state->h);
  if (state->mask & STYGIAN_EDITOR_COMPONENT_OVERRIDE_STYLE_BINDING) {
    (void)editor_copy_short_name(instance->color_token,
                                 sizeof(instance->color_token),
                                 state->style_binding);
  }
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_component_instance_get_override_state(
    const StygianEditor *editor, StygianEditorNodeId instance_id,
    StygianEditorComponentOverrideState *out_state) {
  const StygianEditorNode *instance = editor_find_node_const(editor, instance_id);
  if (!instance || !editor_is_component_instance(instance) || !out_state)
    return false;
  *out_state = instance->as.component_instance.overrides;
  if ((out_state->mask & STYGIAN_EDITOR_COMPONENT_OVERRIDE_VISIBLE) == 0u)
    out_state->visible = instance->visible;
  if ((out_state->mask & STYGIAN_EDITOR_COMPONENT_OVERRIDE_X) == 0u)
    out_state->x = instance->as.component_instance.x;
  if ((out_state->mask & STYGIAN_EDITOR_COMPONENT_OVERRIDE_Y) == 0u)
    out_state->y = instance->as.component_instance.y;
  if ((out_state->mask & STYGIAN_EDITOR_COMPONENT_OVERRIDE_W) == 0u)
    out_state->w = instance->as.component_instance.w;
  if ((out_state->mask & STYGIAN_EDITOR_COMPONENT_OVERRIDE_H) == 0u)
    out_state->h = instance->as.component_instance.h;
  return true;
}

bool stygian_editor_component_instance_reset_overrides(
    StygianEditor *editor, StygianEditorNodeId instance_id) {
  char *before_json = NULL;
  StygianEditorNode *instance = editor_find_node(editor, instance_id);
  const StygianEditorNode *def = NULL;
  if (!instance || !editor_is_component_instance(instance))
    return false;
  if (instance->locked)
    return false;
  def = editor_component_def_for_instance(editor, instance);
  if (!def)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  memset(&instance->as.component_instance.overrides, 0,
         sizeof(instance->as.component_instance.overrides));
  instance->as.component_instance.component_def_id = def->id;
  (void)editor_component_symbol_copy(instance->as.component_instance.symbol_ref,
                                     sizeof(instance->as.component_instance.symbol_ref),
                                     def->as.component_def.symbol);
  instance->component_detached = false;
  instance->as.component_instance.w = def->as.component_def.w;
  instance->as.component_instance.h = def->as.component_def.h;
  instance->visible = def->visible;
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_component_instance_detach(StygianEditor *editor,
                                              StygianEditorNodeId instance_id) {
  char *before_json = NULL;
  StygianEditorNode *instance = editor_find_node(editor, instance_id);
  if (!instance || !editor_is_component_instance(instance))
    return false;
  if (instance->locked)
    return false;
  if (instance->component_detached)
    return true;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  instance->component_detached = true;
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_component_instance_is_detached(
    const StygianEditor *editor, StygianEditorNodeId instance_id,
    bool *out_detached) {
  const StygianEditorNode *instance = editor_find_node_const(editor, instance_id);
  if (!instance || !editor_is_component_instance(instance) || !out_detached)
    return false;
  *out_detached = instance->component_detached;
  return true;
}

bool stygian_editor_component_instance_repair(
    StygianEditor *editor, StygianEditorNodeId instance_id,
    StygianEditorNodeId preferred_component_def_id, bool preserve_overrides) {
  char *before_json = NULL;
  StygianEditorNode *instance = editor_find_node(editor, instance_id);
  const StygianEditorNode *target_def = NULL;
  const StygianEditorNode *hint_def = NULL;
  uint32_t i;
  if (!instance || !editor_is_component_instance(instance))
    return false;
  if (instance->locked)
    return false;
  if (!instance->component_detached)
    return true;

  if (preferred_component_def_id != STYGIAN_EDITOR_INVALID_ID) {
    const StygianEditorNode *candidate =
        editor_find_node_const(editor, preferred_component_def_id);
    if (!editor_is_component_def(candidate))
      return false;
    target_def = candidate;
  } else {
    if (instance->as.component_instance.component_def_id !=
        STYGIAN_EDITOR_INVALID_ID) {
      hint_def = editor_find_node_const(
          editor, instance->as.component_instance.component_def_id);
      if (editor_is_component_def(hint_def))
        target_def = hint_def;
    }
    if (!target_def && instance->as.component_instance.symbol_ref[0]) {
      for (i = 0u; i < editor->node_count; ++i) {
        const StygianEditorNode *candidate = &editor->nodes[i];
        if (!editor_is_component_def(candidate))
          continue;
        if (strncmp(candidate->as.component_def.symbol,
                    instance->as.component_instance.symbol_ref,
                    sizeof(candidate->as.component_def.symbol)) == 0) {
          target_def = candidate;
          break;
        }
      }
    }
  }
  if (!target_def)
    return false;

  if (preserve_overrides) {
    for (i = 0u; i < instance->as.component_instance.overrides.property_override_count &&
                i < STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP;
         ++i) {
      if (!editor_component_property_value_valid_for_instance_context(
              editor, instance, target_def,
              &instance->as.component_instance.overrides.property_overrides[i])) {
        return false;
      }
    }
  }

  if (!editor_begin_mutation(editor, &before_json))
    return false;
  instance->component_detached = false;
  instance->as.component_instance.component_def_id = target_def->id;
  (void)editor_component_symbol_copy(instance->as.component_instance.symbol_ref,
                                     sizeof(instance->as.component_instance.symbol_ref),
                                     target_def->as.component_def.symbol);
  if (!preserve_overrides) {
    memset(&instance->as.component_instance.overrides, 0,
           sizeof(instance->as.component_instance.overrides));
  }
  if ((instance->as.component_instance.overrides.mask &
       STYGIAN_EDITOR_COMPONENT_OVERRIDE_W) == 0u) {
    instance->as.component_instance.w = target_def->as.component_def.w;
  }
  if ((instance->as.component_instance.overrides.mask &
       STYGIAN_EDITOR_COMPONENT_OVERRIDE_H) == 0u) {
    instance->as.component_instance.h = target_def->as.component_def.h;
  }
  if ((instance->as.component_instance.overrides.mask &
       STYGIAN_EDITOR_COMPONENT_OVERRIDE_VISIBLE) == 0u) {
    instance->visible = target_def->visible;
  }
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_component_def_set_variant(StygianEditor *editor,
                                              StygianEditorNodeId component_def_id,
                                              const char *group_name,
                                              const char *variant_name) {
  char *before_json = NULL;
  StygianEditorNode *def = editor_find_node(editor, component_def_id);
  if (!def || !editor_is_component_def(def))
    return false;
  if (def->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  if (!editor_copy_short_name(def->as.component_def.variant_group,
                              sizeof(def->as.component_def.variant_group),
                              group_name)) {
    def->as.component_def.variant_group[0] = '\0';
  }
  if (!editor_copy_short_name(def->as.component_def.variant_name,
                              sizeof(def->as.component_def.variant_name),
                              variant_name)) {
    def->as.component_def.variant_name[0] = '\0';
  }
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_component_def_get_variant(
    const StygianEditor *editor, StygianEditorNodeId component_def_id,
    char *out_group_name, size_t out_group_name_capacity,
    char *out_variant_name, size_t out_variant_name_capacity) {
  const StygianEditorNode *def = editor_find_node_const(editor, component_def_id);
  size_t glen = 0u;
  size_t vlen = 0u;
  if (!def || !editor_is_component_def(def) || !out_group_name ||
      !out_variant_name || out_group_name_capacity == 0u ||
      out_variant_name_capacity == 0u) {
    return false;
  }
  glen = strlen(def->as.component_def.variant_group);
  vlen = strlen(def->as.component_def.variant_name);
  if (glen >= out_group_name_capacity || vlen >= out_variant_name_capacity)
    return false;
  memcpy(out_group_name, def->as.component_def.variant_group, glen + 1u);
  memcpy(out_variant_name, def->as.component_def.variant_name, vlen + 1u);
  return true;
}

bool stygian_editor_component_def_set_property(
    StygianEditor *editor, StygianEditorNodeId component_def_id,
    const StygianEditorComponentPropertyDef *property_def) {
  char *before_json = NULL;
  StygianEditorNode *def = editor_find_node(editor, component_def_id);
  int prop_idx = -1;
  if (!def || !editor_is_component_def(def) || !property_def ||
      !property_def->name[0]) {
    return false;
  }
  if (def->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  prop_idx = editor_component_def_property_index(def, property_def->name);
  if (prop_idx < 0) {
    if (def->as.component_def.property_count >= STYGIAN_EDITOR_COMPONENT_PROPERTY_CAP)
      return editor_finish_mutation(editor, before_json, false);
    prop_idx = (int)def->as.component_def.property_count++;
  }
  def->as.component_def.properties[prop_idx] = *property_def;
  if (def->as.component_def.properties[prop_idx].enum_option_count >
      STYGIAN_EDITOR_COMPONENT_ENUM_OPTION_CAP) {
    def->as.component_def.properties[prop_idx].enum_option_count =
        STYGIAN_EDITOR_COMPONENT_ENUM_OPTION_CAP;
  }
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

uint32_t stygian_editor_component_def_property_count(
    const StygianEditor *editor, StygianEditorNodeId component_def_id) {
  const StygianEditorNode *def = editor_find_node_const(editor, component_def_id);
  if (!def || !editor_is_component_def(def))
    return 0u;
  return def->as.component_def.property_count;
}

bool stygian_editor_component_def_get_property(
    const StygianEditor *editor, StygianEditorNodeId component_def_id,
    uint32_t index, StygianEditorComponentPropertyDef *out_property_def) {
  const StygianEditorNode *def = editor_find_node_const(editor, component_def_id);
  if (!def || !editor_is_component_def(def) || !out_property_def)
    return false;
  if (index >= def->as.component_def.property_count ||
      index >= STYGIAN_EDITOR_COMPONENT_PROPERTY_CAP) {
    return false;
  }
  *out_property_def = def->as.component_def.properties[index];
  return true;
}

bool stygian_editor_component_defs_compatible(
    const StygianEditor *editor, StygianEditorNodeId component_def_a,
    StygianEditorNodeId component_def_b) {
  const StygianEditorNode *a = editor_find_node_const(editor, component_def_a);
  const StygianEditorNode *b = editor_find_node_const(editor, component_def_b);
  uint32_t i;
  if (!editor_is_component_def(a) || !editor_is_component_def(b))
    return false;
  if (a->as.component_def.variant_group[0] || b->as.component_def.variant_group[0]) {
    if (strncmp(a->as.component_def.variant_group, b->as.component_def.variant_group,
                sizeof(a->as.component_def.variant_group)) != 0) {
      return false;
    }
  }
  if (a->as.component_def.property_count != b->as.component_def.property_count)
    return false;
  for (i = 0u; i < a->as.component_def.property_count &&
              i < STYGIAN_EDITOR_COMPONENT_PROPERTY_CAP;
       ++i) {
    const StygianEditorComponentPropertyDef *pa =
        &a->as.component_def.properties[i];
    int b_idx = editor_component_def_property_index(b, pa->name);
    const StygianEditorComponentPropertyDef *pb;
    if (b_idx < 0)
      return false;
    pb = &b->as.component_def.properties[b_idx];
    if (pa->type != pb->type)
      return false;
    if (pa->type == STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM &&
        pa->enum_option_count != pb->enum_option_count) {
      return false;
    }
    if (pa->type == STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM) {
      uint32_t oi;
      for (oi = 0u; oi < pa->enum_option_count &&
                    oi < STYGIAN_EDITOR_COMPONENT_ENUM_OPTION_CAP;
           ++oi) {
        if (strncmp(pa->enum_options[oi], pb->enum_options[oi],
                    sizeof(pa->enum_options[oi])) != 0) {
          return false;
        }
      }
    }
  }
  return true;
}

static bool editor_component_defs_same_set(const StygianEditorNode *a,
                                           const StygianEditorNode *b) {
  if (!editor_is_component_def(a) || !editor_is_component_def(b))
    return false;
  if (!a->as.component_def.variant_group[0] || !b->as.component_def.variant_group[0])
    return false;
  return strncmp(a->as.component_def.variant_group, b->as.component_def.variant_group,
                 sizeof(a->as.component_def.variant_group)) == 0;
}

static bool editor_component_swap_is_legal(const StygianEditor *editor,
                                           StygianEditorNodeId from_def_id,
                                           StygianEditorNodeId to_def_id) {
  const StygianEditorNode *from_def = editor_find_node_const(editor, from_def_id);
  const StygianEditorNode *to_def = editor_find_node_const(editor, to_def_id);
  if (!editor_is_component_def(from_def) || !editor_is_component_def(to_def))
    return false;
  if (from_def_id == to_def_id)
    return true;
  if (!editor_component_defs_same_set(from_def, to_def))
    return false;
  return stygian_editor_component_defs_compatible(editor, from_def_id, to_def_id);
}

static bool editor_component_parse_scoped_override_key(const char *name,
                                                       char *out_target,
                                                       size_t out_target_cap,
                                                       char *out_property,
                                                       size_t out_property_cap) {
  const char *dot = NULL;
  const char *at = name;
  size_t target_len = 0u;
  size_t prop_len = 0u;
  if (!name || !name[0] || !out_target || !out_property || out_target_cap == 0u ||
      out_property_cap == 0u) {
    return false;
  }
  while (*at) {
    if (*at == '.')
      dot = at;
    ++at;
  }
  if (!dot || dot == name || !dot[1])
    return false;
  target_len = (size_t)(dot - name);
  prop_len = strlen(dot + 1u);
  if (target_len >= out_target_cap || prop_len >= out_property_cap)
    return false;
  memcpy(out_target, name, target_len);
  out_target[target_len] = '\0';
  memcpy(out_property, dot + 1u, prop_len + 1u);
  return true;
}

static bool editor_component_find_unique_child_named(const StygianEditor *editor,
                                                     StygianEditorNodeId parent_id,
                                                     const char *name,
                                                     StygianEditorNodeId *out_id) {
  uint32_t i;
  uint32_t count = 0u;
  StygianEditorNodeId match = STYGIAN_EDITOR_INVALID_ID;
  if (!editor || !name || !name[0] || !out_id)
    return false;
  for (i = 0u; i < editor->node_count; ++i) {
    const StygianEditorNode *node = &editor->nodes[i];
    if (node->parent_id != parent_id)
      continue;
    if (strncmp(node->name, name, sizeof(node->name)) != 0)
      continue;
    match = node->id;
    count += 1u;
  }
  if (count != 1u)
    return false;
  *out_id = match;
  return true;
}

static bool editor_component_find_unique_descendant_named(
    const StygianEditor *editor, StygianEditorNodeId root_id, const char *name,
    StygianEditorNodeId *out_id) {
  uint32_t i;
  uint32_t count = 0u;
  StygianEditorNodeId match = STYGIAN_EDITOR_INVALID_ID;
  if (!editor || !name || !name[0] || !out_id)
    return false;
  for (i = 0u; i < editor->node_count; ++i) {
    const StygianEditorNode *node = &editor->nodes[i];
    if (node->id == root_id || !editor_node_is_ancestor(editor, root_id, node->id))
      continue;
    if (strncmp(node->name, name, sizeof(node->name)) != 0)
      continue;
    match = node->id;
    count += 1u;
  }
  if (count != 1u)
    return false;
  *out_id = match;
  return true;
}

static bool editor_component_resolve_scoped_target(const StygianEditor *editor,
                                                   StygianEditorNodeId root_def_id,
                                                   const char *target_path,
                                                   StygianEditorNodeId *out_target_id) {
  char path_copy[32];
  char *cursor = path_copy;
  StygianEditorNodeId cursor_parent = root_def_id;
  if (!editor || !target_path || !target_path[0] || !out_target_id)
    return false;
  if (target_path[0] == '#') {
    unsigned int parsed = 0u;
    StygianEditorNodeId id;
    if (sscanf(target_path + 1, "%u", &parsed) != 1)
      return false;
    id = (StygianEditorNodeId)parsed;
    if (!editor_find_node_const(editor, id) ||
        !editor_node_is_ancestor(editor, root_def_id, id)) {
      return false;
    }
    *out_target_id = id;
    return true;
  }
  if (!editor_copy_short_name(path_copy, sizeof(path_copy), target_path))
    return false;
  while (*cursor) {
    char *slash = strchr(cursor, '/');
    StygianEditorNodeId next_id = STYGIAN_EDITOR_INVALID_ID;
    if (slash)
      *slash = '\0';
    if (!cursor[0])
      return false;
    if (!editor_component_find_unique_child_named(editor, cursor_parent, cursor,
                                                  &next_id)) {
      if (!slash) {
        if (!editor_component_find_unique_descendant_named(editor, root_def_id, cursor,
                                                           &next_id)) {
          return false;
        }
      } else {
        return false;
      }
    }
    cursor_parent = next_id;
    if (!slash)
      break;
    cursor = slash + 1u;
  }
  *out_target_id = cursor_parent;
  return true;
}

static bool editor_component_property_value_valid_for_instance_context(
    const StygianEditor *editor, const StygianEditorNode *instance,
    const StygianEditorNode *instance_def,
    const StygianEditorComponentPropertyValue *value) {
  const char *base_name = editor_component_property_base_name(value ? value->name : NULL);
  int idx = editor_component_def_property_index(instance_def, base_name);
  if (!editor || !editor_is_component_instance(instance) ||
      !editor_is_component_def(instance_def) || !value || !value->name[0]) {
    return false;
  }
  if (idx >= 0)
    return editor_component_property_value_valid_for_def(instance_def, value);
  if (strchr(value->name, '.') != NULL) {
    char target_ref[32];
    char property[32];
    StygianEditorNodeId target_id = STYGIAN_EDITOR_INVALID_ID;
    const StygianEditorNode *target = NULL;
    if (!editor_component_parse_scoped_override_key(value->name, target_ref,
                                                    sizeof(target_ref), property,
                                                    sizeof(property))) {
      return false;
    }
    if (!editor_component_resolve_scoped_target(editor, instance_def->id, target_ref,
                                                &target_id))
      return false;
    target = editor_find_node_const(editor, target_id);
    if (!target)
      return false;
    if (target->kind == STYGIAN_EDITOR_SHAPE_COMPONENT_DEF ||
        target->kind == STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE) {
      const StygianEditorNode *target_def = target;
      StygianEditorComponentPropertyValue scoped = *value;
      if (target->kind == STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE) {
        target_def = editor_component_def_for_instance(editor, target);
      }
      if (!editor_is_component_def(target_def))
        return false;
      (void)editor_copy_short_name(scoped.name, sizeof(scoped.name), property);
      return editor_component_property_value_valid_for_def(target_def, &scoped);
    }
    if (strncmp(property, "visible", sizeof("visible")) == 0)
      return value->type == STYGIAN_EDITOR_COMPONENT_PROPERTY_BOOL;
    if (strncmp(property, "style", sizeof("style")) == 0 ||
        strncmp(property, "token", sizeof("token")) == 0) {
      return value->type == STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM &&
             value->enum_value[0];
    }
    return false;
  }
  return false;
}

static bool editor_component_resolve_instance_definition(
    const StygianEditor *editor, const StygianEditorNode *instance,
    StygianEditorNodeId base_def_id, const StygianEditorComponentOverrideState *state,
    StygianEditorNodeId *out_def_id) {
  StygianEditorNodeId candidate = base_def_id;
  StygianEditorNodeId resolved = STYGIAN_EDITOR_INVALID_ID;
  const StygianEditorNode *base_def = editor_find_node_const(editor, base_def_id);
  if (!editor || !editor_is_component_instance(instance) || !editor_is_component_def(base_def) ||
      !state || !out_def_id) {
    return false;
  }
  if ((state->mask & STYGIAN_EDITOR_COMPONENT_OVERRIDE_SWAP) != 0u) {
    if (!editor_component_swap_is_legal(editor, base_def_id, state->swap_component_def_id))
      return false;
    candidate = state->swap_component_def_id;
  }
  resolved = candidate;
  if (state->property_override_count > 0u) {
    if (stygian_editor_component_resolve_variant(
            editor, candidate, state->property_overrides, state->property_override_count,
            &resolved, NULL, NULL)) {
      *out_def_id = resolved;
      return true;
    }
  }
  *out_def_id = candidate;
  return true;
}

typedef struct EditorVariantRequestProperty {
  StygianEditorComponentPropertyValue value;
} EditorVariantRequestProperty;

static uint32_t editor_variant_request_normalize(
    const StygianEditorComponentPropertyValue *properties, uint32_t property_count,
    EditorVariantRequestProperty *out_props, uint32_t out_cap) {
  uint32_t count = 0u;
  uint32_t i;
  if (!out_props || out_cap == 0u)
    return 0u;
  for (i = 0u; i < property_count; ++i) {
    const StygianEditorComponentPropertyValue *pv = &properties[i];
    const char *base_name = editor_component_property_base_name(pv->name);
    uint32_t existing = UINT32_MAX;
    uint32_t j;
    if (!base_name || !base_name[0] || strchr(pv->name, '.') != NULL)
      continue;
    for (j = 0u; j < count; ++j) {
      if (strncmp(out_props[j].value.name, base_name,
                  sizeof(out_props[j].value.name)) == 0) {
        existing = j;
        break;
      }
    }
    if (existing != UINT32_MAX) {
      out_props[existing].value = *pv;
      (void)editor_copy_short_name(out_props[existing].value.name,
                                   sizeof(out_props[existing].value.name),
                                   base_name);
      continue;
    }
    if (count >= out_cap)
      break;
    out_props[count].value = *pv;
    (void)editor_copy_short_name(out_props[count].value.name,
                                 sizeof(out_props[count].value.name),
                                 base_name);
    count += 1u;
  }
  return count;
}

bool stygian_editor_component_resolve_variant(
    const StygianEditor *editor, StygianEditorNodeId seed_component_def_id,
    const StygianEditorComponentPropertyValue *properties,
    uint32_t property_count, StygianEditorNodeId *out_component_def_id,
    uint32_t *out_match_score, bool *out_exact_match) {
  const StygianEditorNode *seed = editor_find_node_const(editor, seed_component_def_id);
  EditorVariantRequestProperty request_props[STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP];
  uint32_t request_count = 0u;
  StygianEditorNodeId best_id = STYGIAN_EDITOR_INVALID_ID;
  uint32_t best_score = 0u;
  bool best_exact = false;
  uint32_t best_match_count = 0u;
  uint32_t best_mismatch_count = UINT32_MAX;
  uint32_t best_missing_count = UINT32_MAX;
  uint32_t i;
  if (!editor_is_component_def(seed) || !out_component_def_id)
    return false;
  request_count = editor_variant_request_normalize(
      properties ? properties : (const StygianEditorComponentPropertyValue *)0,
      property_count, request_props, STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP);
  for (i = 0u; i < editor->node_count; ++i) {
    const StygianEditorNode *candidate = &editor->nodes[i];
    uint32_t match_count = 0u;
    uint32_t mismatch_count = 0u;
    uint32_t missing_count = 0u;
    uint32_t p, score;
    bool exact;
    if (!editor_is_component_def(candidate))
      continue;
    if (!stygian_editor_component_defs_compatible(editor, seed->id, candidate->id))
      continue;
    for (p = 0u; p < request_count; ++p) {
      const StygianEditorComponentPropertyValue *pv = &request_props[p].value;
      const char *base_name = pv->name;
      int idx;
      idx = editor_component_def_property_index(candidate, base_name);
      if (idx < 0) {
        missing_count += 1u;
        continue;
      }
      if (!editor_component_property_value_valid_for_def(candidate, pv)) {
        mismatch_count += 1u;
        continue;
      }
      if (editor_component_property_value_matches_default(
              &candidate->as.component_def.properties[idx], pv)) {
        match_count += 1u;
      } else {
        mismatch_count += 1u;
      }
    }
    exact = (request_count > 0u && mismatch_count == 0u && missing_count == 0u);
    score = (match_count * 1024u);
    if (request_count > mismatch_count + missing_count) {
      score += (request_count - mismatch_count - missing_count) * 16u;
    }
    if (exact)
      score += 1u;
    if (best_id == STYGIAN_EDITOR_INVALID_ID || match_count > best_match_count ||
        (match_count == best_match_count &&
         mismatch_count < best_mismatch_count) ||
        (match_count == best_match_count &&
         mismatch_count == best_mismatch_count &&
         missing_count < best_missing_count) ||
        (match_count == best_match_count &&
         mismatch_count == best_mismatch_count &&
         missing_count == best_missing_count && exact && !best_exact) ||
        (match_count == best_match_count &&
         mismatch_count == best_mismatch_count &&
         missing_count == best_missing_count && exact == best_exact &&
         candidate->id < best_id)) {
      best_id = candidate->id;
      best_match_count = match_count;
      best_mismatch_count = mismatch_count;
      best_missing_count = missing_count;
      best_score = score;
      best_exact = exact;
    }
  }
  if (best_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  *out_component_def_id = best_id;
  if (out_match_score)
    *out_match_score = best_score;
  if (out_exact_match)
    *out_exact_match = best_exact;
  return true;
}

bool stygian_editor_component_instance_set_property_override(
    StygianEditor *editor, StygianEditorNodeId instance_id,
    const StygianEditorComponentPropertyValue *property_override) {
  char *before_json = NULL;
  StygianEditorNode *instance = editor_find_node(editor, instance_id);
  const StygianEditorNode *def = NULL;
  uint32_t i;
  int existing = -1;
  if (!instance || !editor_is_component_instance(instance) || !property_override ||
      !property_override->name[0]) {
    return false;
  }
  def = editor_component_def_for_instance(editor, instance);
  if (!def || !editor_component_property_value_valid_for_instance_context(
                  editor, instance, def, property_override))
    return false;
  if (instance->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  for (i = 0u;
       i < instance->as.component_instance.overrides.property_override_count &&
       i < STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP;
       ++i) {
    if (strncmp(instance->as.component_instance.overrides.property_overrides[i].name,
                property_override->name,
                sizeof(instance->as.component_instance.overrides
                           .property_overrides[i]
                           .name)) == 0) {
      existing = (int)i;
      break;
    }
  }
  if (existing < 0) {
    if (instance->as.component_instance.overrides.property_override_count >=
        STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP) {
      return editor_finish_mutation(editor, before_json, false);
    }
    existing = (int)instance->as.component_instance.overrides.property_override_count++;
  }
  instance->as.component_instance.overrides.property_overrides[existing] =
      *property_override;
  {
    StygianEditorNodeId resolved = STYGIAN_EDITOR_INVALID_ID;
    const StygianEditorNode *current_def =
        editor_component_def_for_instance(editor, instance);
    const StygianEditorNode *resolved_def = NULL;
    StygianEditorNodeId base_id = STYGIAN_EDITOR_INVALID_ID;
    if ((instance->as.component_instance.overrides.mask &
         STYGIAN_EDITOR_COMPONENT_OVERRIDE_SWAP) != 0u &&
        current_def && editor_component_swap_is_legal(
                           editor, current_def->id,
                           instance->as.component_instance.overrides.swap_component_def_id)) {
      base_id = current_def->id;
    } else if (current_def) {
      base_id = current_def->id;
    }
    if (base_id != STYGIAN_EDITOR_INVALID_ID &&
        editor_component_resolve_instance_definition(
            editor, instance, base_id, &instance->as.component_instance.overrides,
            &resolved)) {
      resolved_def = editor_find_node_const(editor, resolved);
      if (editor_is_component_def(resolved_def)) {
        instance->as.component_instance.component_def_id = resolved;
        (void)editor_component_symbol_copy(
            instance->as.component_instance.symbol_ref,
            sizeof(instance->as.component_instance.symbol_ref),
            resolved_def->as.component_def.symbol);
      }
    }
  }
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_component_instance_get_property_override(
    const StygianEditor *editor, StygianEditorNodeId instance_id,
    const char *name, StygianEditorComponentPropertyValue *out_property_override) {
  const StygianEditorNode *instance = editor_find_node_const(editor, instance_id);
  uint32_t i;
  if (!instance || !editor_is_component_instance(instance) || !name || !name[0] ||
      !out_property_override) {
    return false;
  }
  for (i = 0u;
       i < instance->as.component_instance.overrides.property_override_count &&
       i < STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP;
       ++i) {
    if (strncmp(instance->as.component_instance.overrides.property_overrides[i].name,
                name,
                sizeof(instance->as.component_instance.overrides
                           .property_overrides[i]
                           .name)) == 0) {
      *out_property_override =
          instance->as.component_instance.overrides.property_overrides[i];
      return true;
    }
  }
  return false;
}

bool stygian_editor_component_instance_clear_property_override(
    StygianEditor *editor, StygianEditorNodeId instance_id, const char *name) {
  char *before_json = NULL;
  StygianEditorNode *instance = editor_find_node(editor, instance_id);
  uint32_t i;
  if (!instance || !editor_is_component_instance(instance) || !name || !name[0])
    return false;
  if (instance->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  for (i = 0u;
       i < instance->as.component_instance.overrides.property_override_count &&
       i < STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP;
       ++i) {
    if (strncmp(instance->as.component_instance.overrides.property_overrides[i].name,
                name,
                sizeof(instance->as.component_instance.overrides
                           .property_overrides[i]
                           .name)) == 0) {
      uint32_t j;
      for (j = i + 1u;
           j < instance->as.component_instance.overrides.property_override_count &&
           j < STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP;
           ++j) {
        instance->as.component_instance.overrides.property_overrides[j - 1u] =
            instance->as.component_instance.overrides.property_overrides[j];
      }
      instance->as.component_instance.overrides.property_override_count -= 1u;
      {
        StygianEditorNodeId resolved = STYGIAN_EDITOR_INVALID_ID;
        const StygianEditorNode *current_def =
            editor_component_def_for_instance(editor, instance);
        StygianEditorNodeId base_id = STYGIAN_EDITOR_INVALID_ID;
        if (current_def)
          base_id = current_def->id;
        if (base_id != STYGIAN_EDITOR_INVALID_ID &&
            editor_component_resolve_instance_definition(
                editor, instance, base_id,
                &instance->as.component_instance.overrides, &resolved)) {
          const StygianEditorNode *resolved_def =
              editor_find_node_const(editor, resolved);
          if (editor_is_component_def(resolved_def)) {
            instance->as.component_instance.component_def_id = resolved;
            (void)editor_component_symbol_copy(
                instance->as.component_instance.symbol_ref,
                sizeof(instance->as.component_instance.symbol_ref),
                resolved_def->as.component_def.symbol);
          }
        }
      }
      editor_request_repaint(editor, 120u);
      return editor_finish_mutation(editor, before_json, true);
    }
  }
  return editor_finish_mutation(editor, before_json, false);
}

StygianEditorPathId stygian_editor_path_begin(StygianEditor *editor,
                                              const StygianEditorPathDesc *desc) {
  if (!editor)
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->path_builder.active) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_WARN,
                "Path builder already active. Commit or cancel before begin.");
    return STYGIAN_EDITOR_INVALID_ID;
  }

  editor->path_builder.active = true;
  editor->path_builder.id = editor->next_path_id++;
  editor->path_builder.first_point = editor->point_count;
  editor->path_builder.point_count = 0u;

  if (desc) {
    editor->path_builder.desc = *desc;
  } else {
    editor->path_builder.desc.stroke = stygian_editor_color_rgba(1, 1, 1, 1);
    editor->path_builder.desc.thickness = 1.0f;
    editor->path_builder.desc.closed = false;
    editor->path_builder.desc.visible = true;
    editor->path_builder.desc.z = 0.0f;
  }

  return editor->path_builder.id;
}

bool stygian_editor_path_add_point(StygianEditor *editor, StygianEditorPathId id,
                                   float x, float y, bool snap_to_grid) {
  if (!editor || !editor->path_builder.active || editor->path_builder.id != id)
    return false;
  if (editor->point_count >= editor->max_path_points) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot add path point: point capacity reached (%u).",
                editor->max_path_points);
    return false;
  }

  if (snap_to_grid)
    stygian_editor_snap_world_point(editor, x, y, &x, &y);

  editor->path_points[editor->point_count].x = x;
  editor->path_points[editor->point_count].y = y;
  editor->path_points[editor->point_count].has_in_tangent = false;
  editor->path_points[editor->point_count].in_x = x;
  editor->path_points[editor->point_count].in_y = y;
  editor->path_points[editor->point_count].has_out_tangent = false;
  editor->path_points[editor->point_count].out_x = x;
  editor->path_points[editor->point_count].out_y = y;
  editor->path_points[editor->point_count].kind = STYGIAN_EDITOR_PATH_POINT_CORNER;
  editor->point_count += 1u;
  editor->path_builder.point_count += 1u;
  return true;
}

StygianEditorNodeId stygian_editor_path_commit(StygianEditor *editor,
                                               StygianEditorPathId id) {
  char *before_json = NULL;
  StygianEditorNodeId node_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNode *node;
  if (!editor || !editor->path_builder.active || editor->path_builder.id != id)
    return STYGIAN_EDITOR_INVALID_ID;
  if (!editor_begin_mutation(editor, &before_json))
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->path_builder.point_count < 2u) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_WARN,
                "Cannot commit path with fewer than 2 points.");
    stygian_editor_path_cancel(editor, id);
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot commit path: node capacity reached (%u).",
                editor->max_nodes);
    stygian_editor_path_cancel(editor, id);
    free(before_json);
    return STYGIAN_EDITOR_INVALID_ID;
  }

  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_PATH;
  node->locked = false;
  node->visible = editor->path_builder.desc.visible;
  node->selected = false;
  node->z = editor->path_builder.desc.z;
  node->as.path.first_point = editor->path_builder.first_point;
  node->as.path.point_count = editor->path_builder.point_count;
  node->as.path.closed = editor->path_builder.desc.closed;
  node->as.path.thickness = editor->path_builder.desc.thickness;
  node->as.path.stroke = editor->path_builder.desc.stroke;
  editor_node_seed_legacy_stroke(node);
  node->constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
  node->constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
  node->layout_absolute = false;
  node->layout_sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->layout_sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node->size_min_w = 0.0f;
  node->size_max_w = 0.0f;
  node->size_min_h = 0.0f;
  node->size_max_h = 0.0f;
  editor_set_default_node_name(node);
  editor_recompute_path_bounds(editor, node);
  editor_constraints_capture(editor, node);

  editor->path_builder.active = false;
  node_id = node->id;
  if (!editor_finish_mutation(editor, before_json, true))
    return STYGIAN_EDITOR_INVALID_ID;
  return node_id;
}

void stygian_editor_path_cancel(StygianEditor *editor, StygianEditorPathId id) {
  if (!editor || !editor->path_builder.active || editor->path_builder.id != id)
    return;
  editor->point_count = editor->path_builder.first_point;
  editor->path_builder.active = false;
}

uint32_t stygian_editor_node_count(const StygianEditor *editor) {
  if (!editor)
    return 0u;
  return editor->node_count;
}

bool stygian_editor_node_set_position(StygianEditor *editor,
                                      StygianEditorNodeId node_id, float x,
                                      float y, bool snap_to_grid) {
  StygianEditorNode *node = editor_find_node(editor, node_id);
  StygianEditorTransformCommand cmd;
  StygianEditorNodeId one_id;
  float cur_x = 0.0f;
  float cur_y = 0.0f;
  if (!node)
    return false;
  if (node->locked)
    return false;
  if (snap_to_grid)
    stygian_editor_snap_world_point(editor, x, y, &x, &y);
  if (!editor_node_get_property(editor, node, STYGIAN_EDITOR_PROP_X, &cur_x) ||
      !editor_node_get_property(editor, node, STYGIAN_EDITOR_PROP_Y, &cur_y))
    return false;
  memset(&cmd, 0, sizeof(cmd));
  cmd.kind = STYGIAN_EDITOR_TRANSFORM_MOVE;
  cmd.dx = x - cur_x;
  cmd.dy = y - cur_y;
  one_id = node_id;
  return stygian_editor_apply_transform(editor, &one_id, 1u, &cmd);
}

bool stygian_editor_node_set_size(StygianEditor *editor,
                                  StygianEditorNodeId node_id, float w,
                                  float h) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  StygianEditorTransformCommand cmd;
  StygianEditorNodeId one_id;
  float wx = 0.0f, wy = 0.0f, ww = 0.0f, wh = 0.0f;
  if (!node || w < 0.0f || h < 0.0f)
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  if (!editor_node_bounds_world(editor, node, &wx, &wy, &ww, &wh))
    return editor_finish_mutation(editor, before_json, false);
  if (ww <= 0.000001f || wh <= 0.000001f) {
    if (!editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_WIDTH, w) ||
        !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_HEIGHT, h))
      return editor_finish_mutation(editor, before_json, false);
    editor_request_repaint(editor, 120u);
    return editor_finish_mutation(editor, before_json, true);
  }
  free(before_json);
  memset(&cmd, 0, sizeof(cmd));
  cmd.kind = STYGIAN_EDITOR_TRANSFORM_SCALE;
  cmd.sx = w / ww;
  cmd.sy = h / wh;
  cmd.pivot_world_x = wx;
  cmd.pivot_world_y = wy;
  one_id = node_id;
  return stygian_editor_apply_transform(editor, &one_id, 1u, &cmd);
}

bool stygian_editor_node_set_color(StygianEditor *editor,
                                   StygianEditorNodeId node_id,
                                   StygianEditorColor color) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!node)
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;

  if (node->kind == STYGIAN_EDITOR_SHAPE_PATH)
    node->as.path.stroke = color;
  else if (node->kind == STYGIAN_EDITOR_SHAPE_LINE)
    node->as.line.stroke = color;
  else if (node->kind == STYGIAN_EDITOR_SHAPE_ARROW)
    node->as.arrow.stroke = color;
  else if (node->kind == STYGIAN_EDITOR_SHAPE_ARC)
    node->as.arc.stroke = color;
  else if (editor_node_supports_fill(node->kind)) {
    editor_node_fill_sync_from_legacy(node);
    if (node->fill_count == 0u)
      return editor_finish_mutation(editor, before_json, false);
    node->fills[0].kind = STYGIAN_EDITOR_FILL_SOLID;
    node->fills[0].visible = true;
    node->fills[0].opacity = 1.0f;
    node->fills[0].solid = color;
    editor_node_set_legacy_fill_color(node, color);
  } else
    return editor_finish_mutation(editor, before_json, false);
  if (editor_node_supports_stroke(node->kind) && node->stroke_count > 0u) {
    node->strokes[0].color = color;
    node->strokes[0].visible = true;
    node->strokes[0].opacity = 1.0f;
  }

  node->color_token[0] = '\0';
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_color(const StygianEditor *editor,
                                   StygianEditorNodeId node_id,
                                   StygianEditorColor *out_color) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || !out_color)
    return false;
  if (node->kind == STYGIAN_EDITOR_SHAPE_PATH)
    *out_color = node->as.path.stroke;
  else if (node->kind == STYGIAN_EDITOR_SHAPE_LINE)
    *out_color = node->as.line.stroke;
  else if (node->kind == STYGIAN_EDITOR_SHAPE_ARROW)
    *out_color = node->as.arrow.stroke;
  else if (node->kind == STYGIAN_EDITOR_SHAPE_ARC)
    *out_color = node->as.arc.stroke;
  else if (editor_node_supports_stroke(node->kind) && node->stroke_count > 0u) {
    *out_color = node->strokes[0].color;
    out_color->a *= node->strokes[0].opacity;
  }
  else if (editor_node_supports_fill(node->kind))
    *out_color = editor_node_primary_fill_color(node);
  else
    return false;
  return true;
}

uint32_t stygian_editor_node_fill_count(const StygianEditor *editor,
                                        StygianEditorNodeId node_id) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || !editor_node_supports_fill(node->kind))
    return 0u;
  if (node->fill_count > STYGIAN_EDITOR_NODE_FILL_CAP)
    return STYGIAN_EDITOR_NODE_FILL_CAP;
  return node->fill_count;
}

bool stygian_editor_node_get_fill(const StygianEditor *editor,
                                  StygianEditorNodeId node_id, uint32_t index,
                                  StygianEditorNodeFill *out_fill) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || !out_fill || !editor_node_supports_fill(node->kind))
    return false;
  if (index >= node->fill_count || index >= STYGIAN_EDITOR_NODE_FILL_CAP)
    return false;
  *out_fill = node->fills[index];
  return true;
}

bool stygian_editor_node_set_fills(StygianEditor *editor,
                                   StygianEditorNodeId node_id,
                                   const StygianEditorNodeFill *fills,
                                   uint32_t fill_count) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  uint32_t i;
  if (!node || !editor_node_supports_fill(node->kind))
    return false;
  if (fill_count > STYGIAN_EDITOR_NODE_FILL_CAP)
    return false;
  if (fill_count > 0u && !fills)
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;

  node->fill_count = fill_count;
  for (i = 0u; i < fill_count; ++i) {
    StygianEditorNodeFill fill = fills[i];
    if (fill.opacity < 0.0f)
      fill.opacity = 0.0f;
    if (fill.opacity > 1.0f)
      fill.opacity = 1.0f;
    if (fill.kind > STYGIAN_EDITOR_FILL_IMAGE)
      fill.kind = STYGIAN_EDITOR_FILL_SOLID;
    node->fills[i] = fill;
    node->fill_gradient_xform[i] = editor_gradient_xform_default();
  }
  for (; i < STYGIAN_EDITOR_NODE_FILL_CAP; ++i) {
    memset(&node->fills[i], 0, sizeof(node->fills[i]));
    node->fill_gradient_xform[i] = editor_gradient_xform_default();
  }

  if (node->fill_count > 0u) {
    StygianEditorColor color = editor_node_primary_fill_color(node);
    editor_node_set_legacy_fill_color(node, color);
  }

  node->color_token[0] = '\0';
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_set_fill_gradient_transform(
    StygianEditor *editor, StygianEditorNodeId node_id, uint32_t fill_index,
    const StygianEditorGradientTransform *transform) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  StygianEditorGradientTransform next;
  if (!node || !transform || !editor_node_supports_fill(node->kind))
    return false;
  if (fill_index >= node->fill_count || fill_index >= STYGIAN_EDITOR_NODE_FILL_CAP)
    return false;
  if (node->fills[fill_index].kind != STYGIAN_EDITOR_FILL_LINEAR_GRADIENT &&
      node->fills[fill_index].kind != STYGIAN_EDITOR_FILL_RADIAL_GRADIENT) {
    return false;
  }
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  next = *transform;
  editor_gradient_xform_sanitize(&next);
  node->fill_gradient_xform[fill_index] = next;
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_fill_gradient_transform(
    const StygianEditor *editor, StygianEditorNodeId node_id,
    uint32_t fill_index, StygianEditorGradientTransform *out_transform) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || !out_transform || !editor_node_supports_fill(node->kind))
    return false;
  if (fill_index >= node->fill_count || fill_index >= STYGIAN_EDITOR_NODE_FILL_CAP)
    return false;
  *out_transform = node->fill_gradient_xform[fill_index];
  editor_gradient_xform_sanitize(out_transform);
  return true;
}

bool stygian_editor_node_set_text_content(StygianEditor *editor,
                                          StygianEditorNodeId node_id,
                                          const char *text) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  size_t len = 0u;
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_TEXT || !text)
    return false;
  len = strlen(text);
  if (len >= sizeof(node->as.text.text))
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  memcpy(node->as.text.text, text, len + 1u);
  editor_text_apply_auto_size(node);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_text_content(const StygianEditor *editor,
                                          StygianEditorNodeId node_id,
                                          char *out_text,
                                          size_t out_text_capacity) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  size_t len = 0u;
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_TEXT || !out_text ||
      out_text_capacity == 0u) {
    return false;
  }
  len = strlen(node->as.text.text);
  if (len >= out_text_capacity)
    return false;
  memcpy(out_text, node->as.text.text, len + 1u);
  return true;
}

bool stygian_editor_node_set_text_layout(StygianEditor *editor,
                                         StygianEditorNodeId node_id,
                                         StygianEditorTextBoxMode box_mode,
                                         StygianEditorTextHAlign align_h,
                                         StygianEditorTextVAlign align_v,
                                         StygianEditorTextAutoSize auto_size) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_TEXT)
    return false;
  if (box_mode > STYGIAN_EDITOR_TEXT_BOX_AREA ||
      align_h > STYGIAN_EDITOR_TEXT_ALIGN_RIGHT ||
      align_v > STYGIAN_EDITOR_TEXT_ALIGN_BOTTOM ||
      auto_size > STYGIAN_EDITOR_TEXT_AUTOSIZE_BOTH) {
    return false;
  }
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->as.text.box_mode = box_mode;
  node->as.text.align_h = align_h;
  node->as.text.align_v = align_v;
  node->as.text.auto_size = auto_size;
  editor_text_apply_auto_size(node);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_text_layout(
    const StygianEditor *editor, StygianEditorNodeId node_id,
    StygianEditorTextBoxMode *out_box_mode, StygianEditorTextHAlign *out_align_h,
    StygianEditorTextVAlign *out_align_v, StygianEditorTextAutoSize *out_auto_size) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_TEXT)
    return false;
  if (out_box_mode)
    *out_box_mode = node->as.text.box_mode;
  if (out_align_h)
    *out_align_h = node->as.text.align_h;
  if (out_align_v)
    *out_align_v = node->as.text.align_v;
  if (out_auto_size)
    *out_auto_size = node->as.text.auto_size;
  return true;
}

bool stygian_editor_node_set_text_typography(StygianEditor *editor,
                                             StygianEditorNodeId node_id,
                                             float font_size, float line_height,
                                             float letter_spacing,
                                             uint32_t font_weight) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_TEXT)
    return false;
  if (font_size <= 0.0f)
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->as.text.font_size = font_size;
  node->as.text.line_height = line_height;
  node->as.text.letter_spacing = letter_spacing;
  node->as.text.font_weight = font_weight;
  editor_text_apply_auto_size(node);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_text_typography(
    const StygianEditor *editor, StygianEditorNodeId node_id,
    float *out_font_size, float *out_line_height, float *out_letter_spacing,
    uint32_t *out_font_weight) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_TEXT)
    return false;
  if (out_font_size)
    *out_font_size = node->as.text.font_size;
  if (out_line_height)
    *out_line_height = node->as.text.line_height;
  if (out_letter_spacing)
    *out_letter_spacing = node->as.text.letter_spacing;
  if (out_font_weight)
    *out_font_weight = node->as.text.font_weight;
  return true;
}

uint32_t stygian_editor_node_text_span_count(const StygianEditor *editor,
                                             StygianEditorNodeId node_id) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_TEXT)
    return 0u;
  if (node->as.text.span_count > STYGIAN_EDITOR_TEXT_SPAN_CAP)
    return STYGIAN_EDITOR_TEXT_SPAN_CAP;
  return node->as.text.span_count;
}

bool stygian_editor_node_get_text_span(const StygianEditor *editor,
                                       StygianEditorNodeId node_id,
                                       uint32_t index,
                                       StygianEditorTextStyleSpan *out_span) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_TEXT || !out_span)
    return false;
  if (index >= node->as.text.span_count || index >= STYGIAN_EDITOR_TEXT_SPAN_CAP)
    return false;
  *out_span = node->as.text.spans[index];
  return true;
}

bool stygian_editor_node_set_text_spans(StygianEditor *editor,
                                        StygianEditorNodeId node_id,
                                        const StygianEditorTextStyleSpan *spans,
                                        uint32_t span_count) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  uint32_t i;
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_TEXT)
    return false;
  if (span_count > STYGIAN_EDITOR_TEXT_SPAN_CAP)
    return false;
  if (span_count > 0u && !spans)
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->as.text.span_count = span_count;
  for (i = 0u; i < span_count; ++i)
    node->as.text.spans[i] = spans[i];
  for (; i < STYGIAN_EDITOR_TEXT_SPAN_CAP; ++i)
    memset(&node->as.text.spans[i], 0, sizeof(node->as.text.spans[i]));
  editor_text_apply_auto_size(node);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_measure_text(const StygianEditor *editor,
                                      StygianEditorNodeId node_id,
                                      float *out_width, float *out_height) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_TEXT)
    return false;
  editor_measure_text_block(&node->as.text, out_width, out_height);
  return true;
}

bool stygian_editor_node_set_image_source(StygianEditor *editor,
                                          StygianEditorNodeId node_id,
                                          const char *source) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  size_t len = 0u;
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_IMAGE || !source)
    return false;
  len = strlen(source);
  if (len >= sizeof(node->as.image.source))
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  memcpy(node->as.image.source, source, len + 1u);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_image_source(const StygianEditor *editor,
                                          StygianEditorNodeId node_id,
                                          char *out_source,
                                          size_t out_source_capacity) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  size_t len = 0u;
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_IMAGE || !out_source ||
      out_source_capacity == 0u) {
    return false;
  }
  len = strlen(node->as.image.source);
  if (len >= out_source_capacity)
    return false;
  memcpy(out_source, node->as.image.source, len + 1u);
  return true;
}

bool stygian_editor_node_set_image_fit_mode(StygianEditor *editor,
                                            StygianEditorNodeId node_id,
                                            uint32_t fit_mode) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_IMAGE)
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->as.image.fit_mode = fit_mode;
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_image_fit_mode(const StygianEditor *editor,
                                            StygianEditorNodeId node_id,
                                            uint32_t *out_fit_mode) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_IMAGE || !out_fit_mode)
    return false;
  *out_fit_mode = node->as.image.fit_mode;
  return true;
}

uint32_t stygian_editor_node_stroke_count(const StygianEditor *editor,
                                          StygianEditorNodeId node_id) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || !editor_node_supports_stroke(node->kind))
    return 0u;
  if (node->stroke_count > STYGIAN_EDITOR_NODE_STROKE_CAP)
    return STYGIAN_EDITOR_NODE_STROKE_CAP;
  return node->stroke_count;
}

bool stygian_editor_node_get_stroke(const StygianEditor *editor,
                                    StygianEditorNodeId node_id, uint32_t index,
                                    StygianEditorNodeStroke *out_stroke) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || !out_stroke || !editor_node_supports_stroke(node->kind))
    return false;
  if (index >= node->stroke_count || index >= STYGIAN_EDITOR_NODE_STROKE_CAP)
    return false;
  *out_stroke = node->strokes[index];
  return true;
}

bool stygian_editor_node_set_strokes(StygianEditor *editor,
                                     StygianEditorNodeId node_id,
                                     const StygianEditorNodeStroke *strokes,
                                     uint32_t stroke_count) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  uint32_t i;
  if (!node || !editor_node_supports_stroke(node->kind))
    return false;
  if (stroke_count > STYGIAN_EDITOR_NODE_STROKE_CAP)
    return false;
  if (stroke_count > 0u && !strokes)
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->stroke_count = stroke_count;
  for (i = 0u; i < stroke_count; ++i) {
    StygianEditorNodeStroke stroke = strokes[i];
    if (stroke.opacity < 0.0f)
      stroke.opacity = 0.0f;
    if (stroke.opacity > 1.0f)
      stroke.opacity = 1.0f;
    if (stroke.thickness < 0.0f)
      stroke.thickness = 0.0f;
    if (stroke.cap > STYGIAN_EDITOR_STROKE_CAP_SQUARE)
      stroke.cap = STYGIAN_EDITOR_STROKE_CAP_BUTT;
    if (stroke.join > STYGIAN_EDITOR_STROKE_JOIN_BEVEL)
      stroke.join = STYGIAN_EDITOR_STROKE_JOIN_MITER;
    if (stroke.align > STYGIAN_EDITOR_STROKE_ALIGN_OUTSIDE)
      stroke.align = STYGIAN_EDITOR_STROKE_ALIGN_CENTER;
    if (stroke.dash_count > STYGIAN_EDITOR_STROKE_DASH_CAP)
      stroke.dash_count = STYGIAN_EDITOR_STROKE_DASH_CAP;
    node->strokes[i] = stroke;
  }
  for (; i < STYGIAN_EDITOR_NODE_STROKE_CAP; ++i)
    memset(&node->strokes[i], 0, sizeof(node->strokes[i]));
  if (node->stroke_count > 0u) {
    const StygianEditorNodeStroke *stroke = &node->strokes[0];
    switch (node->kind) {
    case STYGIAN_EDITOR_SHAPE_PATH:
      node->as.path.stroke = stroke->color;
      node->as.path.stroke.a *= stroke->opacity;
      node->as.path.thickness = stroke->thickness;
      break;
    case STYGIAN_EDITOR_SHAPE_LINE:
      node->as.line.stroke = stroke->color;
      node->as.line.stroke.a *= stroke->opacity;
      node->as.line.thickness = stroke->thickness;
      break;
    case STYGIAN_EDITOR_SHAPE_ARROW:
      node->as.arrow.stroke = stroke->color;
      node->as.arrow.stroke.a *= stroke->opacity;
      node->as.arrow.thickness = stroke->thickness;
      break;
    case STYGIAN_EDITOR_SHAPE_ARC:
      node->as.arc.stroke = stroke->color;
      node->as.arc.stroke.a *= stroke->opacity;
      node->as.arc.thickness = stroke->thickness;
      break;
    default:
      break;
    }
  }
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

uint32_t stygian_editor_node_effect_count(const StygianEditor *editor,
                                          StygianEditorNodeId node_id) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || !editor_node_supports_effects(node->kind))
    return 0u;
  if (node->effect_count > STYGIAN_EDITOR_NODE_EFFECT_CAP)
    return STYGIAN_EDITOR_NODE_EFFECT_CAP;
  return node->effect_count;
}

bool stygian_editor_node_get_effect(const StygianEditor *editor,
                                    StygianEditorNodeId node_id, uint32_t index,
                                    StygianEditorNodeEffect *out_effect) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || !out_effect || !editor_node_supports_effects(node->kind))
    return false;
  if (index >= node->effect_count || index >= STYGIAN_EDITOR_NODE_EFFECT_CAP)
    return false;
  *out_effect = node->effects[index];
  return true;
}

bool stygian_editor_node_set_effects(StygianEditor *editor,
                                     StygianEditorNodeId node_id,
                                     const StygianEditorNodeEffect *effects,
                                     uint32_t effect_count) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  uint32_t i;
  if (!node || !editor_node_supports_effects(node->kind))
    return false;
  if (effect_count > STYGIAN_EDITOR_NODE_EFFECT_CAP)
    return false;
  if (effect_count > 0u && !effects)
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->effect_count = effect_count;
  for (i = 0u; i < effect_count; ++i) {
    StygianEditorNodeEffect effect = effects[i];
    if (effect.kind > STYGIAN_EDITOR_EFFECT_NOISE)
      effect.kind = STYGIAN_EDITOR_EFFECT_DROP_SHADOW;
    if (effect.opacity < 0.0f)
      effect.opacity = 0.0f;
    if (effect.opacity > 1.0f)
      effect.opacity = 1.0f;
    node->effects[i] = effect;
    node->effect_xform[i] = editor_effect_xform_default();
  }
  for (; i < STYGIAN_EDITOR_NODE_EFFECT_CAP; ++i) {
    memset(&node->effects[i], 0, sizeof(node->effects[i]));
    node->effect_xform[i] = editor_effect_xform_default();
  }
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_set_effect_transform(
    StygianEditor *editor, StygianEditorNodeId node_id, uint32_t effect_index,
    const StygianEditorEffectTransform *transform) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  StygianEditorEffectTransform next;
  if (!node || !transform || !editor_node_supports_effects(node->kind))
    return false;
  if (effect_index >= node->effect_count ||
      effect_index >= STYGIAN_EDITOR_NODE_EFFECT_CAP) {
    return false;
  }
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  next = *transform;
  editor_effect_xform_sanitize(&next);
  node->effect_xform[effect_index] = next;
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_effect_transform(
    const StygianEditor *editor, StygianEditorNodeId node_id,
    uint32_t effect_index, StygianEditorEffectTransform *out_transform) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || !out_transform || !editor_node_supports_effects(node->kind))
    return false;
  if (effect_index >= node->effect_count ||
      effect_index >= STYGIAN_EDITOR_NODE_EFFECT_CAP) {
    return false;
  }
  *out_transform = node->effect_xform[effect_index];
  editor_effect_xform_sanitize(out_transform);
  return true;
}

bool stygian_editor_node_set_mask(StygianEditor *editor,
                                  StygianEditorNodeId node_id,
                                  StygianEditorNodeId mask_node_id,
                                  StygianEditorMaskMode mode, bool invert) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!node)
    return false;
  if (node->locked)
    return false;
  if (mode > STYGIAN_EDITOR_MASK_LUMINANCE)
    return false;
  if (mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
    if (mask_node_id == node_id)
      return false;
    if (!editor_find_node_const(editor, mask_node_id))
      return false;
    if (editor_mask_would_cycle(editor, node_id, mask_node_id))
      return false;
  }
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->mask_node_id = mask_node_id;
  node->mask_mode = mode;
  node->mask_invert = invert;
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_mask(const StygianEditor *editor,
                                  StygianEditorNodeId node_id,
                                  StygianEditorNodeId *out_mask_node_id,
                                  StygianEditorMaskMode *out_mode,
                                  bool *out_invert) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node)
    return false;
  if (out_mask_node_id)
    *out_mask_node_id = node->mask_node_id;
  if (out_mode)
    *out_mode = node->mask_mode;
  if (out_invert)
    *out_invert = node->mask_invert;
  return true;
}

bool stygian_editor_node_set_shader_attachment(
    StygianEditor *editor, StygianEditorNodeId node_id,
    const StygianEditorShaderAttachment *attachment) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  StygianEditorShaderAttachment next = {0};
  if (!node || !attachment)
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  next = *attachment;
  editor_shader_attachment_sanitize(&next);
  node->shader_attachment = next;
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_shader_attachment(
    const StygianEditor *editor, StygianEditorNodeId node_id,
    StygianEditorShaderAttachment *out_attachment) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || !out_attachment)
    return false;
  *out_attachment = node->shader_attachment;
  editor_shader_attachment_sanitize(out_attachment);
  return true;
}

bool stygian_editor_node_set_opacity(StygianEditor *editor,
                                     StygianEditorNodeId node_id,
                                     float opacity) {
  StygianEditorColor color;
  if (!editor)
    return false;
  if (!stygian_editor_node_get_color(editor, node_id, &color))
    return false;
  color.a = editor_clampf(opacity, 0.0f, 1.0f);
  return stygian_editor_node_set_color(editor, node_id, color);
}

bool stygian_editor_node_set_rotation(StygianEditor *editor,
                                      StygianEditorNodeId node_id,
                                      float rotation_degrees) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!node)
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->rotation_deg = rotation_degrees;
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_rotation(const StygianEditor *editor,
                                      StygianEditorNodeId node_id,
                                      float *out_rotation_degrees) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || !out_rotation_degrees)
    return false;
  *out_rotation_degrees = node->rotation_deg;
  return true;
}

bool stygian_editor_node_get_shape_kind(const StygianEditor *editor,
                                        StygianEditorNodeId node_id,
                                        StygianEditorShapeKind *out_kind) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || !out_kind)
    return false;
  *out_kind = node->kind;
  return true;
}

bool stygian_editor_node_set_corner_radii(StygianEditor *editor,
                                          StygianEditorNodeId node_id,
                                          float top_left, float top_right,
                                          float bottom_right,
                                          float bottom_left) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_RECT)
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->as.rect.radius[0] = top_left < 0.0f ? 0.0f : top_left;
  node->as.rect.radius[1] = top_right < 0.0f ? 0.0f : top_right;
  node->as.rect.radius[2] = bottom_right < 0.0f ? 0.0f : bottom_right;
  node->as.rect.radius[3] = bottom_left < 0.0f ? 0.0f : bottom_left;
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_corner_radii(const StygianEditor *editor,
                                          StygianEditorNodeId node_id,
                                          float *out_top_left,
                                          float *out_top_right,
                                          float *out_bottom_right,
                                          float *out_bottom_left) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_RECT)
    return false;
  if (out_top_left)
    *out_top_left = node->as.rect.radius[0];
  if (out_top_right)
    *out_top_right = node->as.rect.radius[1];
  if (out_bottom_right)
    *out_bottom_right = node->as.rect.radius[2];
  if (out_bottom_left)
    *out_bottom_left = node->as.rect.radius[3];
  return true;
}

bool stygian_editor_node_get_bounds(const StygianEditor *editor,
                                    StygianEditorNodeId node_id, float *out_x,
                                    float *out_y, float *out_w, float *out_h) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node)
    return false;
  return editor_node_bounds_world(editor, node, out_x, out_y, out_w, out_h);
}

bool stygian_editor_node_get_parent(const StygianEditor *editor,
                                    StygianEditorNodeId node_id,
                                    StygianEditorNodeId *out_parent_id) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || !out_parent_id)
    return false;
  *out_parent_id = node->parent_id;
  return true;
}

bool stygian_editor_reparent_node(StygianEditor *editor,
                                  StygianEditorNodeId node_id,
                                  StygianEditorNodeId new_parent_id,
                                  bool keep_world_transform) {
  char *before_json = NULL;
  StygianEditorNode *node = NULL;
  float wx_before = 0.0f;
  float wy_before = 0.0f;
  float wx_after_parent = 0.0f;
  float wy_after_parent = 0.0f;
  float local_x = 0.0f;
  float local_y = 0.0f;
  if (!editor || node_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node = editor_find_node(editor, node_id);
  if (!node)
    return editor_finish_mutation(editor, before_json, false);
  if (node->locked)
    return editor_finish_mutation(editor, before_json, false);
  if (new_parent_id == node_id)
    return editor_finish_mutation(editor, before_json, false);
  if (new_parent_id != STYGIAN_EDITOR_INVALID_ID &&
      !editor_find_node_const(editor, new_parent_id))
    return editor_finish_mutation(editor, before_json, false);
  if (new_parent_id != STYGIAN_EDITOR_INVALID_ID &&
      editor_node_is_ancestor(editor, node_id, new_parent_id)) {
    return editor_finish_mutation(editor, before_json, false);
  }
  if (keep_world_transform) {
    if (!editor_node_get_local_xy(node, &local_x, &local_y))
      return editor_finish_mutation(editor, before_json, false);
    editor_node_world_offset(editor, node, &wx_before, &wy_before);
    wx_before += local_x;
    wy_before += local_y;
  }
  node->parent_id = new_parent_id;
  if (keep_world_transform) {
    if (!editor_node_get_local_xy(node, &local_x, &local_y))
      return editor_finish_mutation(editor, before_json, false);
    if (new_parent_id != STYGIAN_EDITOR_INVALID_ID) {
      const StygianEditorNode *parent =
          editor_find_node_const(editor, new_parent_id);
      editor_node_world_offset(editor, parent, &wx_after_parent, &wy_after_parent);
      if (editor_node_get_local_xy(parent, &local_x, &local_y)) {
        wx_after_parent += local_x;
        wy_after_parent += local_y;
      }
    }
    if (!editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_X,
                                    wx_before - wx_after_parent))
      return editor_finish_mutation(editor, before_json, false);
    if (!editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_Y,
                                    wy_before - wy_after_parent))
      return editor_finish_mutation(editor, before_json, false);
  }
  editor_constraints_capture(editor, node);
  if (new_parent_id != STYGIAN_EDITOR_INVALID_ID) {
    StygianEditorNode *new_parent = editor_find_node(editor, new_parent_id);
    if (new_parent && new_parent->kind == STYGIAN_EDITOR_SHAPE_FRAME) {
      if (!editor_auto_layout_recompute_frame_node(editor, new_parent))
        return editor_finish_mutation(editor, before_json, false);
    }
  }
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

static void editor_parent_world_origin(const StygianEditor *editor,
                                       StygianEditorNodeId parent_id,
                                       float *out_x, float *out_y) {
  float x = 0.0f;
  float y = 0.0f;
  if (parent_id != STYGIAN_EDITOR_INVALID_ID) {
    const StygianEditorNode *parent = editor_find_node_const(editor, parent_id);
    float lx = 0.0f;
    float ly = 0.0f;
    if (parent) {
      editor_node_world_offset(editor, parent, &x, &y);
      if (editor_node_get_local_xy(parent, &lx, &ly)) {
        x += lx;
        y += ly;
      }
    }
  }
  if (out_x)
    *out_x = x;
  if (out_y)
    *out_y = y;
}

bool stygian_editor_apply_transform(
    StygianEditor *editor, const StygianEditorNodeId *node_ids,
    uint32_t node_count, const StygianEditorTransformCommand *command) {
  char *before_json = NULL;
  uint32_t i;
  if (!editor || !command || !node_ids || node_count == 0u)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  for (i = 0u; i < node_count; ++i) {
    const StygianEditorNode *node = editor_find_node_const(editor, node_ids[i]);
    if (!node || node->locked)
      return editor_finish_mutation(editor, before_json, false);
  }

  switch (command->kind) {
  case STYGIAN_EDITOR_TRANSFORM_MOVE:
    for (i = 0u; i < node_count; ++i) {
      StygianEditorNode *node = editor_find_node(editor, node_ids[i]);
      float x = 0.0f;
      float y = 0.0f;
      if (!node)
        return editor_finish_mutation(editor, before_json, false);
      if (!editor_node_get_property(editor, node, STYGIAN_EDITOR_PROP_X, &x) ||
          !editor_node_get_property(editor, node, STYGIAN_EDITOR_PROP_Y, &y))
        return editor_finish_mutation(editor, before_json, false);
      if (!editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_X,
                                      x + command->dx) ||
          !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_Y,
                                      y + command->dy))
        return editor_finish_mutation(editor, before_json, false);
      editor_constraints_capture(editor, node);
    }
    break;

  case STYGIAN_EDITOR_TRANSFORM_SCALE:
    for (i = 0u; i < node_count; ++i) {
      StygianEditorNode *node = editor_find_node(editor, node_ids[i]);
      float wx = 0.0f, wy = 0.0f, ww = 0.0f, wh = 0.0f;
      float new_w = 0.0f, new_h = 0.0f;
      float new_wx = 0.0f, new_wy = 0.0f;
      float parent_wx = 0.0f, parent_wy = 0.0f;
      if (!node)
        return editor_finish_mutation(editor, before_json, false);
      if (!editor_node_bounds_world(editor, node, &wx, &wy, &ww, &wh))
        return editor_finish_mutation(editor, before_json, false);
      new_w = ww * command->sx;
      new_h = wh * command->sy;
      new_wx = command->pivot_world_x + (wx - command->pivot_world_x) * command->sx;
      new_wy = command->pivot_world_y + (wy - command->pivot_world_y) * command->sy;
      editor_parent_world_origin(editor, node->parent_id, &parent_wx, &parent_wy);
      if (!editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_X,
                                      new_wx - parent_wx) ||
          !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_Y,
                                      new_wy - parent_wy) ||
          !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_WIDTH,
                                      new_w) ||
          !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_HEIGHT,
                                      new_h))
        return editor_finish_mutation(editor, before_json, false);
      editor_constraints_capture(editor, node);
    }
    break;

  case STYGIAN_EDITOR_TRANSFORM_ROTATE: {
    float rad = command->degrees * 0.017453292519943295f;
    float c = cosf(rad);
    float s = sinf(rad);
    for (i = 0u; i < node_count; ++i) {
      StygianEditorNode *node = editor_find_node(editor, node_ids[i]);
      float wx = 0.0f, wy = 0.0f, ww = 0.0f, wh = 0.0f;
      float tx = 0.0f, ty = 0.0f;
      float rwx = 0.0f, rwy = 0.0f;
      float parent_wx = 0.0f, parent_wy = 0.0f;
      float r = 0.0f;
      (void)ww;
      (void)wh;
      if (!node)
        return editor_finish_mutation(editor, before_json, false);
      if (!editor_node_bounds_world(editor, node, &wx, &wy, &ww, &wh))
        return editor_finish_mutation(editor, before_json, false);
      tx = wx - command->pivot_world_x;
      ty = wy - command->pivot_world_y;
      rwx = command->pivot_world_x + (tx * c - ty * s);
      rwy = command->pivot_world_y + (tx * s + ty * c);
      editor_parent_world_origin(editor, node->parent_id, &parent_wx, &parent_wy);
      if (!editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_X,
                                      rwx - parent_wx) ||
          !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_Y,
                                      rwy - parent_wy))
        return editor_finish_mutation(editor, before_json, false);
      if (!editor_node_get_property(editor, node, STYGIAN_EDITOR_PROP_ROTATION_DEG,
                                    &r))
        return editor_finish_mutation(editor, before_json, false);
      if (!editor_node_apply_property(editor, node,
                                      STYGIAN_EDITOR_PROP_ROTATION_DEG,
                                      r + command->degrees))
        return editor_finish_mutation(editor, before_json, false);
      editor_constraints_capture(editor, node);
    }
    break;
  }

  case STYGIAN_EDITOR_TRANSFORM_REPARENT:
    {
      bool was_suspended = editor->history_suspended;
      editor->history_suspended = true;
    for (i = 0u; i < node_count; ++i) {
      if (!stygian_editor_reparent_node(editor, node_ids[i], command->new_parent_id,
                                        command->keep_world_transform)) {
          editor->history_suspended = was_suspended;
        return editor_finish_mutation(editor, before_json, false);
      }
    }
      editor->history_suspended = was_suspended;
    }
    break;
  default:
    return editor_finish_mutation(editor, before_json, false);
  }

  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_delete_node(StygianEditor *editor,
                                StygianEditorNodeId node_id) {
  char *before_json = NULL;
  bool own_suspend = false;
  int idx;
  uint32_t i;
  if (!editor || node_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  if (editor_find_node_const(editor, node_id) &&
      editor_find_node_const(editor, node_id)->locked) {
    return editor_finish_mutation(editor, before_json, false);
  }
  if (!editor->history_suspended) {
    editor->history_suspended = true;
    own_suspend = true;
  }

  // Parent owns descendants; drop subtree first so we don't leave orphans.
  for (i = 0u; i < editor->node_count; ++i) {
    if (editor->nodes[i].parent_id == node_id) {
      StygianEditorNodeId child_id = editor->nodes[i].id;
      if (!stygian_editor_delete_node(editor, child_id))
        goto delete_fail;
      i = 0u;
    }
  }

  idx = editor_find_node_index(editor, node_id);
  if (idx < 0)
    goto delete_fail;

  for (i = (uint32_t)idx; (i + 1u) < editor->node_count; ++i) {
    editor->nodes[i] = editor->nodes[i + 1u];
  }
  editor->node_count -= 1u;

  editor_refresh_primary_selected(editor);

  for (i = 0u; i < editor->behavior_count;) {
    StygianEditorBehaviorSlot *slot = &editor->behaviors[i];
    bool touches_node = false;
    if (slot->rule.trigger_node == node_id)
      touches_node = true;
    if (slot->rule.action_kind == STYGIAN_EDITOR_ACTION_ANIMATE &&
        slot->rule.animate.target_node == node_id) {
      touches_node = true;
    } else if (slot->rule.action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY &&
               slot->rule.set_property.target_node == node_id) {
      touches_node = true;
    } else if (slot->rule.action_kind ==
                   STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY &&
               slot->rule.toggle_visibility.target_node == node_id) {
      touches_node = true;
    }
    if (touches_node) {
      uint32_t j;
      for (j = i; (j + 1u) < editor->behavior_count; ++j) {
        editor->behaviors[j] = editor->behaviors[j + 1u];
        editor->active_anims[j] = editor->active_anims[j + 1u];
      }
      editor->behavior_count -= 1u;
      continue;
    }
    ++i;
  }

  for (i = 0u; i < editor->timeline_track_count;) {
    if (editor->timeline_tracks[i].track.target_node == node_id) {
      uint32_t j;
      StygianEditorTimelineTrackId removed_id = editor->timeline_tracks[i].id;
      for (j = i; (j + 1u) < editor->timeline_track_count; ++j)
        editor->timeline_tracks[j] = editor->timeline_tracks[j + 1u];
      editor->timeline_track_count -= 1u;
      for (j = 0u; j < editor->timeline_clip_count; ++j) {
        StygianEditorTimelineClip *clip = &editor->timeline_clips[j].clip;
        uint32_t write_i = 0u;
        for (uint32_t k = 0u; k < clip->track_count; ++k) {
          if (clip->track_ids[k] != removed_id)
            clip->track_ids[write_i++] = clip->track_ids[k];
        }
        clip->track_count = write_i;
      }
      continue;
    }
    ++i;
  }

  for (i = 0u; i < editor->driver_count;) {
    if (editor->drivers[i].rule.source_node == node_id) {
      uint32_t j;
      for (j = i; (j + 1u) < editor->driver_count; ++j)
        editor->drivers[j] = editor->drivers[j + 1u];
      editor->driver_count -= 1u;
      continue;
    }
    ++i;
  }

  editor_request_repaint(editor, 120u);
  if (own_suspend)
    editor->history_suspended = false;
  return editor_finish_mutation(editor, before_json, true);

delete_fail:
  if (own_suspend)
    editor->history_suspended = false;
  return editor_finish_mutation(editor, before_json, false);
}

bool stygian_editor_property_get_descriptor(
    StygianEditorPropertyKind kind, StygianEditorPropertyDescriptor *out_desc) {
  if (!out_desc)
    return false;
  return editor_property_descriptor(kind, out_desc);
}

bool stygian_editor_node_get_property_value(
    const StygianEditor *editor, StygianEditorNodeId node_id,
    StygianEditorPropertyKind property, StygianEditorPropertyValue *out_value) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  StygianEditorPropertyDescriptor desc;
  float number = 0.0f;
  if (!editor || !node || !out_value)
    return false;
  if (!editor_property_descriptor(property, &desc))
    return false;
  memset(out_value, 0, sizeof(*out_value));
  out_value->type = desc.value_type;

  switch (property) {
  case STYGIAN_EDITOR_PROP_VISIBLE:
    out_value->as.boolean = node->visible;
    return true;
  case STYGIAN_EDITOR_PROP_FILL_COLOR:
    return stygian_editor_node_get_color(editor, node_id, &out_value->as.color);
  case STYGIAN_EDITOR_PROP_COLOR_TOKEN:
    memcpy(out_value->as.string, node->color_token, sizeof(node->color_token));
    out_value->as.string[sizeof(out_value->as.string) - 1u] = '\0';
    return true;
  case STYGIAN_EDITOR_PROP_SHAPE_KIND:
    out_value->as.enum_value = (uint32_t)node->kind;
    return true;
  default:
    if (!editor_node_get_property(editor, node, property, &number))
      return false;
    out_value->as.number = number;
    return true;
  }
}

bool stygian_editor_node_set_property_value(
    StygianEditor *editor, StygianEditorNodeId node_id,
    StygianEditorPropertyKind property, const StygianEditorPropertyValue *value) {
  StygianEditorNode *node = editor_find_node(editor, node_id);
  StygianEditorPropertyDescriptor desc;
  float x, y, w, h;
  bool visible;
  char *before_json = NULL;
  if (!editor || !node || !value)
    return false;
  if (!editor_property_descriptor(property, &desc) || !desc.writable)
    return false;
  if (!editor_node_supports_property(editor, node, property))
    return false;
  if (!editor_property_value_valid(property, value))
    return false;

  switch (property) {
  case STYGIAN_EDITOR_PROP_X:
    if (!editor_node_get_property(editor, node, STYGIAN_EDITOR_PROP_Y, &y))
      return false;
    return stygian_editor_node_set_position(editor, node_id, value->as.number, y,
                                            false);
  case STYGIAN_EDITOR_PROP_Y:
    if (!editor_node_get_property(editor, node, STYGIAN_EDITOR_PROP_X, &x))
      return false;
    return stygian_editor_node_set_position(editor, node_id, x, value->as.number,
                                            false);
  case STYGIAN_EDITOR_PROP_WIDTH:
    if (node->kind == STYGIAN_EDITOR_SHAPE_COMPONENT_DEF) {
      if (!editor_begin_mutation(editor, &before_json))
        return false;
      node->as.component_def.w = editor_node_clamp_width(node, value->as.number);
      editor_component_apply_definition_internal(editor, node->id);
      editor_request_repaint(editor, 120u);
      return editor_finish_mutation(editor, before_json, true);
    }
    if (!editor_node_get_property(editor, node, STYGIAN_EDITOR_PROP_HEIGHT, &h))
      return false;
    return stygian_editor_node_set_size(editor, node_id, value->as.number, h);
  case STYGIAN_EDITOR_PROP_HEIGHT:
    if (node->kind == STYGIAN_EDITOR_SHAPE_COMPONENT_DEF) {
      if (!editor_begin_mutation(editor, &before_json))
        return false;
      node->as.component_def.h = editor_node_clamp_height(node, value->as.number);
      editor_component_apply_definition_internal(editor, node->id);
      editor_request_repaint(editor, 120u);
      return editor_finish_mutation(editor, before_json, true);
    }
    if (!editor_node_get_property(editor, node, STYGIAN_EDITOR_PROP_WIDTH, &w))
      return false;
    return stygian_editor_node_set_size(editor, node_id, w, value->as.number);
  case STYGIAN_EDITOR_PROP_OPACITY:
    return stygian_editor_node_set_opacity(editor, node_id, value->as.number);
  case STYGIAN_EDITOR_PROP_ROTATION_DEG:
    return stygian_editor_node_set_rotation(editor, node_id, value->as.number);
  case STYGIAN_EDITOR_PROP_RADIUS_TL:
  case STYGIAN_EDITOR_PROP_RADIUS_TR:
  case STYGIAN_EDITOR_PROP_RADIUS_BR:
  case STYGIAN_EDITOR_PROP_RADIUS_BL: {
    float tl = 0.0f, tr = 0.0f, br = 0.0f, bl = 0.0f;
    if (!stygian_editor_node_get_corner_radii(editor, node_id, &tl, &tr, &br, &bl))
      return false;
    if (property == STYGIAN_EDITOR_PROP_RADIUS_TL)
      tl = value->as.number;
    else if (property == STYGIAN_EDITOR_PROP_RADIUS_TR)
      tr = value->as.number;
    else if (property == STYGIAN_EDITOR_PROP_RADIUS_BR)
      br = value->as.number;
    else
      bl = value->as.number;
    return stygian_editor_node_set_corner_radii(editor, node_id, tl, tr, br, bl);
  }
  case STYGIAN_EDITOR_PROP_VALUE:
    if (!editor_begin_mutation(editor, &before_json))
      return false;
    if (!editor_node_apply_property(editor, node, property, value->as.number))
      return editor_finish_mutation(editor, before_json, false);
    editor_request_repaint(editor, 120u);
    return editor_finish_mutation(editor, before_json, true);
  case STYGIAN_EDITOR_PROP_VISIBLE:
    visible = value->as.boolean;
    if (!editor_begin_mutation(editor, &before_json))
      return false;
    node->visible = visible;
    if (node->kind == STYGIAN_EDITOR_SHAPE_COMPONENT_DEF)
      editor_component_apply_definition_internal(editor, node->id);
    editor_request_repaint(editor, 120u);
    return editor_finish_mutation(editor, before_json, true);
  case STYGIAN_EDITOR_PROP_FILL_COLOR:
    return stygian_editor_node_set_color(editor, node_id, value->as.color);
  case STYGIAN_EDITOR_PROP_COLOR_TOKEN:
    if (value->as.string[0] == '\0') {
      if (!editor_begin_mutation(editor, &before_json))
        return false;
      node->color_token[0] = '\0';
      editor_request_repaint(editor, 120u);
      return editor_finish_mutation(editor, before_json, true);
    }
    return stygian_editor_apply_color_token(editor, node_id, value->as.string);
  default:
    return false;
  }
}

bool stygian_editor_selection_get_property_value(
    const StygianEditor *editor, StygianEditorPropertyKind property,
    StygianEditorPropertyValue *out_value, bool *out_mixed,
    uint32_t *out_supported_count) {
  uint32_t i;
  uint32_t supported = 0u;
  bool mixed = false;
  StygianEditorPropertyValue first = {0};
  if (!editor || !out_value)
    return false;

  for (i = 0u; i < editor->node_count; ++i) {
    const StygianEditorNode *node = &editor->nodes[i];
    StygianEditorPropertyValue current;
    if (!node->selected)
      continue;
    if (!editor_node_supports_property(editor, node, property))
      continue;
    if (!stygian_editor_node_get_property_value(editor, node->id, property, &current))
      continue;
    if (supported == 0u) {
      first = current;
    } else if (!editor_property_values_equal(&first, &current)) {
      mixed = true;
    }
    supported += 1u;
  }

  if (out_supported_count)
    *out_supported_count = supported;
  if (out_mixed)
    *out_mixed = mixed;
  if (supported == 0u)
    return false;
  *out_value = first;
  return true;
}

bool stygian_editor_selection_set_property_value(
    StygianEditor *editor, StygianEditorPropertyKind property,
    const StygianEditorPropertyValue *value, uint32_t *out_applied_count) {
  uint32_t i;
  uint32_t applied = 0u;
  if (!editor || !value)
    return false;
  if (!editor_property_value_valid(property, value))
    return false;
  if (!stygian_editor_begin_transaction(editor))
    return false;

  for (i = 0u; i < editor->node_count; ++i) {
    const StygianEditorNode *node = &editor->nodes[i];
    if (!node->selected)
      continue;
    if (!editor_node_supports_property(editor, node, property))
      continue;
    if (!stygian_editor_node_set_property_value(editor, node->id, property, value)) {
      stygian_editor_end_transaction(editor, false);
      return false;
    }
    applied += 1u;
  }

  if (out_applied_count)
    *out_applied_count = applied;
  if (applied == 0u)
    return stygian_editor_end_transaction(editor, false);
  return stygian_editor_end_transaction(editor, true);
}

bool stygian_editor_node_set_name(StygianEditor *editor,
                                  StygianEditorNodeId node_id,
                                  const char *name) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  size_t len;
  if (!editor || !node || !name)
    return false;
  len = strlen(name);
  if (len == 0u || len >= sizeof(node->name))
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  memcpy(node->name, name, len + 1u);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_name(const StygianEditor *editor,
                                  StygianEditorNodeId node_id, char *out_name,
                                  size_t out_name_capacity) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  size_t len;
  if (!editor || !node || !out_name || out_name_capacity == 0u)
    return false;
  len = strlen(node->name);
  if (len + 1u > out_name_capacity)
    return false;
  memcpy(out_name, node->name, len + 1u);
  return true;
}

bool stygian_editor_node_set_locked(StygianEditor *editor,
                                    StygianEditorNodeId node_id, bool locked) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!editor || !node)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->locked = locked;
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_locked(const StygianEditor *editor,
                                    StygianEditorNodeId node_id,
                                    bool *out_locked) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!editor || !node || !out_locked)
    return false;
  *out_locked = node->locked;
  return true;
}

bool stygian_editor_node_set_visible(StygianEditor *editor,
                                     StygianEditorNodeId node_id,
                                     bool visible) {
  StygianEditorPropertyValue v;
  memset(&v, 0, sizeof(v));
  v.type = STYGIAN_EDITOR_VALUE_BOOL;
  v.as.boolean = visible;
  return stygian_editor_node_set_property_value(editor, node_id,
                                                STYGIAN_EDITOR_PROP_VISIBLE, &v);
}

bool stygian_editor_node_get_visible(const StygianEditor *editor,
                                     StygianEditorNodeId node_id,
                                     bool *out_visible) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!editor || !node || !out_visible)
    return false;
  *out_visible = node->visible;
  return true;
}

uint32_t stygian_editor_tree_list_children(const StygianEditor *editor,
                                           StygianEditorNodeId parent_id,
                                           StygianEditorNodeId *out_ids,
                                           uint32_t max_ids) {
  uint32_t i;
  uint32_t count = 0u;
  if (!editor)
    return 0u;
  for (i = 0u; i < editor->node_count; ++i) {
    if (editor->nodes[i].parent_id != parent_id)
      continue;
    if (out_ids && count < max_ids)
      out_ids[count] = editor->nodes[i].id;
    count += 1u;
  }
  return count;
}

bool stygian_editor_tree_reorder_child(StygianEditor *editor,
                                       StygianEditorNodeId node_id,
                                       uint32_t new_sibling_index) {
  char *before_json = NULL;
  int idx = editor_find_node_index(editor, node_id);
  StygianEditorNodeId parent_id;
  uint32_t i;
  uint32_t sibling_count = 0u;
  uint32_t seen = 0u;
  int target_idx = -1;
  if (!editor || idx < 0)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  parent_id = editor->nodes[idx].parent_id;
  for (i = 0u; i < editor->node_count; ++i) {
    if (editor->nodes[i].parent_id == parent_id)
      sibling_count += 1u;
  }
  if (new_sibling_index >= sibling_count)
    new_sibling_index = sibling_count - 1u;
  for (i = 0u; i < editor->node_count; ++i) {
    if (editor->nodes[i].parent_id != parent_id)
      continue;
    if (seen == new_sibling_index) {
      target_idx = (int)i;
      break;
    }
    seen += 1u;
  }
  if (target_idx < 0)
    return editor_finish_mutation(editor, before_json, false);
  if (target_idx == idx)
    return editor_finish_mutation(editor, before_json, true);
  {
    StygianEditorNode moved = editor->nodes[idx];
    if (idx < target_idx) {
      for (i = (uint32_t)idx; i < (uint32_t)target_idx; ++i)
        editor->nodes[i] = editor->nodes[i + 1u];
      editor->nodes[target_idx] = moved;
    } else {
      for (i = (uint32_t)idx; i > (uint32_t)target_idx; --i)
        editor->nodes[i] = editor->nodes[i - 1u];
      editor->nodes[target_idx] = moved;
    }
  }
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_set_constraints(StygianEditor *editor,
                                         StygianEditorNodeId node_id,
                                         StygianEditorConstraintH horizontal,
                                         StygianEditorConstraintV vertical) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!editor || !node)
    return false;
  if (horizontal > STYGIAN_EDITOR_CONSTRAINT_H_SCALE ||
      vertical > STYGIAN_EDITOR_CONSTRAINT_V_SCALE) {
    return false;
  }
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->constraint_h = horizontal;
  node->constraint_v = vertical;
  editor_constraints_capture(editor, node);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_constraints(const StygianEditor *editor,
                                         StygianEditorNodeId node_id,
                                         StygianEditorConstraintH *out_horizontal,
                                         StygianEditorConstraintV *out_vertical) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!editor || !node || !out_horizontal || !out_vertical)
    return false;
  *out_horizontal = node->constraint_h;
  *out_vertical = node->constraint_v;
  return true;
}

bool stygian_editor_resize_frame(StygianEditor *editor,
                                 StygianEditorNodeId frame_node_id,
                                 float new_width, float new_height) {
  char *before_json = NULL;
  StygianEditorNode *frame = editor_find_node(editor, frame_node_id);
  float old_w = 0.0f, old_h = 0.0f;
  if (!editor || !frame || new_width < 0.0f || new_height < 0.0f)
    return false;
  if (frame->kind != STYGIAN_EDITOR_SHAPE_FRAME)
    return false;
  old_w = frame->as.frame.w;
  old_h = frame->as.frame.h;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  frame->as.frame.w = new_width;
  frame->as.frame.h = new_height;
  if (fabsf(old_w - new_width) > 0.0001f || fabsf(old_h - new_height) > 0.0001f) {
    if (!editor_constraints_apply_to_children(editor, frame_node_id, new_width,
                                              new_height)) {
      return editor_finish_mutation(editor, before_json, false);
    }
    if (!editor_auto_layout_recompute_frame_node(editor, frame))
      return editor_finish_mutation(editor, before_json, false);
  }
  editor_constraints_capture(editor, frame);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_frame_set_auto_layout(
    StygianEditor *editor, StygianEditorNodeId frame_node_id,
    const StygianEditorFrameAutoLayout *layout) {
  char *before_json = NULL;
  StygianEditorNode *frame = editor_find_node(editor, frame_node_id);
  if (!editor || !frame || !layout)
    return false;
  if (frame->kind != STYGIAN_EDITOR_SHAPE_FRAME)
    return false;
  if (layout->mode > STYGIAN_EDITOR_AUTO_LAYOUT_VERTICAL)
    return false;
  if (layout->wrap > STYGIAN_EDITOR_AUTO_LAYOUT_WRAP)
    return false;
  if (layout->primary_align > STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_STRETCH ||
      layout->cross_align > STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_STRETCH) {
    return false;
  }
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  frame->as.frame.layout_mode = layout->mode;
  frame->as.frame.layout_wrap = layout->wrap;
  frame->as.frame.layout_padding_left = layout->padding_left;
  frame->as.frame.layout_padding_right = layout->padding_right;
  frame->as.frame.layout_padding_top = layout->padding_top;
  frame->as.frame.layout_padding_bottom = layout->padding_bottom;
  frame->as.frame.layout_gap = layout->gap;
  frame->as.frame.layout_primary_align = layout->primary_align;
  frame->as.frame.layout_cross_align = layout->cross_align;
  if (!editor_auto_layout_recompute_frame_node(editor, frame))
    return editor_finish_mutation(editor, before_json, false);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_frame_get_auto_layout(
    const StygianEditor *editor, StygianEditorNodeId frame_node_id,
    StygianEditorFrameAutoLayout *out_layout) {
  const StygianEditorNode *frame = editor_find_node_const(editor, frame_node_id);
  if (!editor || !frame || !out_layout)
    return false;
  if (frame->kind != STYGIAN_EDITOR_SHAPE_FRAME)
    return false;
  out_layout->mode = frame->as.frame.layout_mode;
  out_layout->wrap = frame->as.frame.layout_wrap;
  out_layout->padding_left = frame->as.frame.layout_padding_left;
  out_layout->padding_right = frame->as.frame.layout_padding_right;
  out_layout->padding_top = frame->as.frame.layout_padding_top;
  out_layout->padding_bottom = frame->as.frame.layout_padding_bottom;
  out_layout->gap = frame->as.frame.layout_gap;
  out_layout->primary_align = frame->as.frame.layout_primary_align;
  out_layout->cross_align = frame->as.frame.layout_cross_align;
  return true;
}

bool stygian_editor_node_set_layout_options(
    StygianEditor *editor, StygianEditorNodeId node_id,
    const StygianEditorNodeLayoutOptions *options) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!editor || !node || !options)
    return false;
  if (options->sizing_h > STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FILL ||
      options->sizing_v > STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FILL) {
    return false;
  }
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->layout_absolute = options->absolute_position;
  node->layout_sizing_h = options->sizing_h;
  node->layout_sizing_v = options->sizing_v;
  if (node->parent_id != STYGIAN_EDITOR_INVALID_ID) {
    StygianEditorNode *parent = editor_find_node(editor, node->parent_id);
    if (parent && parent->kind == STYGIAN_EDITOR_SHAPE_FRAME) {
      if (!editor_auto_layout_recompute_frame_node(editor, parent))
        return editor_finish_mutation(editor, before_json, false);
    }
  }
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_layout_options(
    const StygianEditor *editor, StygianEditorNodeId node_id,
    StygianEditorNodeLayoutOptions *out_options) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!editor || !node || !out_options)
    return false;
  out_options->absolute_position = node->layout_absolute;
  out_options->sizing_h = node->layout_sizing_h;
  out_options->sizing_v = node->layout_sizing_v;
  return true;
}

bool stygian_editor_frame_recompute_layout(StygianEditor *editor,
                                           StygianEditorNodeId frame_node_id) {
  char *before_json = NULL;
  StygianEditorNode *frame = editor_find_node(editor, frame_node_id);
  if (!editor || !frame)
    return false;
  if (frame->kind != STYGIAN_EDITOR_SHAPE_FRAME)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  if (!editor_auto_layout_recompute_frame_node(editor, frame))
    return editor_finish_mutation(editor, before_json, false);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_set_size_limits(StygianEditor *editor,
                                         StygianEditorNodeId node_id,
                                         float min_width, float max_width,
                                         float min_height, float max_height) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!editor || !node)
    return false;
  if (min_width < 0.0f || min_height < 0.0f || max_width < 0.0f ||
      max_height < 0.0f) {
    return false;
  }
  if (max_width > 0.0f && max_width < min_width)
    return false;
  if (max_height > 0.0f && max_height < min_height)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->size_min_w = min_width;
  node->size_max_w = max_width;
  node->size_min_h = min_height;
  node->size_max_h = max_height;
  {
    float w = 0.0f, h = 0.0f;
    if (!editor_node_get_local_wh(node, &w, &h))
      return editor_finish_mutation(editor, before_json, false);
    w = editor_node_clamp_width(node, w);
    h = editor_node_clamp_height(node, h);
    if (!editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_WIDTH, w) ||
        !editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_HEIGHT,
                                    h)) {
      return editor_finish_mutation(editor, before_json, false);
    }
  }
  if (node->parent_id != STYGIAN_EDITOR_INVALID_ID) {
    StygianEditorNode *parent = editor_find_node(editor, node->parent_id);
    if (parent && parent->kind == STYGIAN_EDITOR_SHAPE_FRAME) {
      if (!editor_auto_layout_recompute_frame_node(editor, parent))
        return editor_finish_mutation(editor, before_json, false);
    }
  }
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_size_limits(const StygianEditor *editor,
                                         StygianEditorNodeId node_id,
                                         float *out_min_width,
                                         float *out_max_width,
                                         float *out_min_height,
                                         float *out_max_height) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!editor || !node || !out_min_width || !out_max_width || !out_min_height ||
      !out_max_height) {
    return false;
  }
  *out_min_width = node->size_min_w;
  *out_max_width = node->size_max_w;
  *out_min_height = node->size_min_h;
  *out_max_height = node->size_max_h;
  return true;
}

bool stygian_editor_frame_set_overflow_policy(
    StygianEditor *editor, StygianEditorNodeId frame_node_id,
    StygianEditorFrameOverflowPolicy policy) {
  char *before_json = NULL;
  StygianEditorNode *frame = editor_find_node(editor, frame_node_id);
  if (!editor || !frame)
    return false;
  if (frame->kind != STYGIAN_EDITOR_SHAPE_FRAME)
    return false;
  if (policy > STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_BOTH)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  frame->as.frame.overflow_policy = policy;
  if (!editor_auto_layout_recompute_frame_node(editor, frame))
    return editor_finish_mutation(editor, before_json, false);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_frame_get_overflow_policy(
    const StygianEditor *editor, StygianEditorNodeId frame_node_id,
    StygianEditorFrameOverflowPolicy *out_policy) {
  const StygianEditorNode *frame = editor_find_node_const(editor, frame_node_id);
  if (!editor || !frame || !out_policy)
    return false;
  if (frame->kind != STYGIAN_EDITOR_SHAPE_FRAME)
    return false;
  *out_policy = frame->as.frame.overflow_policy;
  return true;
}

bool stygian_editor_frame_set_scroll(StygianEditor *editor,
                                     StygianEditorNodeId frame_node_id,
                                     float scroll_x, float scroll_y) {
  char *before_json = NULL;
  StygianEditorNode *frame = editor_find_node(editor, frame_node_id);
  if (!editor || !frame || scroll_x < 0.0f || scroll_y < 0.0f)
    return false;
  if (frame->kind != STYGIAN_EDITOR_SHAPE_FRAME)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  frame->as.frame.scroll_x = scroll_x;
  frame->as.frame.scroll_y = scroll_y;
  if (!editor_auto_layout_recompute_frame_node(editor, frame))
    return editor_finish_mutation(editor, before_json, false);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_frame_get_scroll(const StygianEditor *editor,
                                     StygianEditorNodeId frame_node_id,
                                     float *out_scroll_x, float *out_scroll_y) {
  const StygianEditorNode *frame = editor_find_node_const(editor, frame_node_id);
  if (!editor || !frame || !out_scroll_x || !out_scroll_y)
    return false;
  if (frame->kind != STYGIAN_EDITOR_SHAPE_FRAME)
    return false;
  *out_scroll_x = frame->as.frame.scroll_x;
  *out_scroll_y = frame->as.frame.scroll_y;
  return true;
}

StygianEditorNodeId stygian_editor_select_at(StygianEditor *editor, float x,
                                             float y, bool additive) {
  int best_index;
  StygianEditorNode *hit = NULL;

  if (!editor)
    return STYGIAN_EDITOR_INVALID_ID;

  best_index = editor_hit_test_top_node_index(editor, x, y);

  if (best_index < 0) {
    if (!additive)
      editor_clear_selection(editor);
    return STYGIAN_EDITOR_INVALID_ID;
  }

  hit = &editor->nodes[best_index];
  if (!additive) {
    editor_clear_selection(editor);
    hit->selected = true;
    editor->selected_node = hit->id;
    return hit->id;
  }

  hit->selected = !hit->selected;
  if (hit->selected) {
    editor->selected_node = hit->id;
    return hit->id;
  }
  if (editor->selected_node == hit->id)
    editor_refresh_primary_selected(editor);
  return STYGIAN_EDITOR_INVALID_ID;
}

bool stygian_editor_select_node(StygianEditor *editor,
                                StygianEditorNodeId node_id, bool additive) {
  StygianEditorNode *node;
  if (!editor)
    return false;
  if (node_id == STYGIAN_EDITOR_INVALID_ID) {
    if (!additive)
      editor_clear_selection(editor);
    return !additive;
  }
  node = editor_find_node(editor, node_id);
  if (!node)
    return false;
  if (!additive) {
    editor_clear_selection(editor);
    node->selected = true;
    editor->selected_node = node_id;
    return true;
  }
  node->selected = !node->selected;
  if (node->selected) {
    editor->selected_node = node_id;
    return true;
  }
  if (editor->selected_node == node_id)
    editor_refresh_primary_selected(editor);
  return false;
}

StygianEditorNodeId stygian_editor_selected_node(const StygianEditor *editor) {
  if (!editor)
    return STYGIAN_EDITOR_INVALID_ID;
  return editor->selected_node;
}

uint32_t stygian_editor_selected_count(const StygianEditor *editor) {
  return editor_count_selected_nodes(editor);
}

bool stygian_editor_node_is_selected(const StygianEditor *editor,
                                     StygianEditorNodeId node_id) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node)
    return false;
  return node->selected;
}

uint32_t stygian_editor_selected_nodes(const StygianEditor *editor,
                                       StygianEditorNodeId *out_ids,
                                       uint32_t max_ids) {
  uint32_t i;
  uint32_t count = 0u;
  if (!editor || !out_ids || max_ids == 0u)
    return 0u;
  for (i = 0u; i < editor->node_count && count < max_ids; ++i) {
    if (!editor->nodes[i].selected)
      continue;
    out_ids[count++] = editor->nodes[i].id;
  }
  return count;
}

StygianEditorNodeId stygian_editor_hit_test_at(const StygianEditor *editor,
                                               float x, float y) {
  int best_index = editor_hit_test_top_node_index(editor, x, y);
  if (!editor || best_index < 0)
    return STYGIAN_EDITOR_INVALID_ID;
  return editor->nodes[best_index].id;
}

uint32_t stygian_editor_select_in_rect(StygianEditor *editor, float x0, float y0,
                                       float x1, float y1, bool additive) {
  uint32_t i;
  float min_x;
  float min_y;
  float max_x;
  float max_y;
  float w;
  float h;
  if (!editor)
    return 0u;

  min_x = x0 < x1 ? x0 : x1;
  max_x = x0 < x1 ? x1 : x0;
  min_y = y0 < y1 ? y0 : y1;
  max_y = y0 < y1 ? y1 : y0;
  w = max_x - min_x;
  h = max_y - min_y;

  if (!additive)
    editor_clear_selection(editor);

  if (w <= 0.0001f || h <= 0.0001f) {
    editor_refresh_primary_selected(editor);
    return editor_count_selected_nodes(editor);
  }

  for (i = 0u; i < editor->node_count; ++i) {
    StygianEditorNode *node = &editor->nodes[i];
    if (!node->visible)
      continue;
    if (editor_node_intersects_rect(editor, node, min_x, min_y, w, h))
      node->selected = true;
  }

  editor_refresh_primary_selected(editor);
  return editor_count_selected_nodes(editor);
}

uint32_t stygian_editor_path_point_count(const StygianEditor *editor,
                                         StygianEditorNodeId path_node_id) {
  const StygianEditorNode *node = editor_find_node_const(editor, path_node_id);
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_PATH)
    return 0u;
  return node->as.path.point_count;
}

bool stygian_editor_path_get_point(const StygianEditor *editor,
                                   StygianEditorNodeId path_node_id,
                                   uint32_t point_index,
                                   StygianEditorPoint *out_point) {
  const StygianEditorNode *node = editor_find_node_const(editor, path_node_id);
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_PATH || !out_point)
    return false;
  if (point_index >= node->as.path.point_count)
    return false;
  *out_point = editor->path_points[node->as.path.first_point + point_index];
  return true;
}

bool stygian_editor_path_set_point(StygianEditor *editor,
                                   StygianEditorNodeId path_node_id,
                                   uint32_t point_index,
                                   const StygianEditorPoint *point) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, path_node_id);
  StygianEditorPoint p;
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_PATH || !point)
    return false;
  if (point_index >= node->as.path.point_count)
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  p = *point;
  if (p.kind > STYGIAN_EDITOR_PATH_POINT_ASYMMETRIC)
    p.kind = STYGIAN_EDITOR_PATH_POINT_CORNER;
  editor->path_points[node->as.path.first_point + point_index] = p;
  editor_recompute_path_bounds(editor, node);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_path_insert_point(StygianEditor *editor,
                                      StygianEditorNodeId path_node_id,
                                      uint32_t point_index,
                                      const StygianEditorPoint *point) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, path_node_id);
  StygianEditorPoint tmp[4096];
  StygianEditorPoint p;
  uint32_t i, src = 0u;
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_PATH || !point)
    return false;
  if (node->locked)
    return false;
  if (node->as.path.point_count >= 4096u)
    return false;
  if (point_index > node->as.path.point_count)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  p = *point;
  if (p.kind > STYGIAN_EDITOR_PATH_POINT_ASYMMETRIC)
    p.kind = STYGIAN_EDITOR_PATH_POINT_CORNER;
  for (i = 0u; i < node->as.path.point_count + 1u; ++i) {
    if (i == point_index) {
      tmp[i] = p;
    } else {
      tmp[i] = editor->path_points[node->as.path.first_point + src];
      src += 1u;
    }
  }
  if (!editor_node_path_replace_points(editor, node, tmp,
                                       node->as.path.point_count + 1u)) {
    return editor_finish_mutation(editor, before_json, false);
  }
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_path_remove_point(StygianEditor *editor,
                                      StygianEditorNodeId path_node_id,
                                      uint32_t point_index) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, path_node_id);
  StygianEditorPoint tmp[4096];
  uint32_t i, dst = 0u;
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_PATH)
    return false;
  if (node->locked)
    return false;
  if (node->as.path.point_count <= 2u)
    return false;
  if (node->as.path.point_count > 4096u)
    return false;
  if (point_index >= node->as.path.point_count)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  for (i = 0u; i < node->as.path.point_count; ++i) {
    if (i == point_index)
      continue;
    tmp[dst++] = editor->path_points[node->as.path.first_point + i];
  }
  if (!editor_node_path_replace_points(editor, node, tmp, dst))
    return editor_finish_mutation(editor, before_json, false);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_path_set_closed(StygianEditor *editor,
                                    StygianEditorNodeId path_node_id,
                                    bool closed) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, path_node_id);
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_PATH)
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->as.path.closed = closed;
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

StygianEditorNodeId stygian_editor_boolean_compose(
    StygianEditor *editor, StygianEditorBooleanOp op,
    const StygianEditorNodeId *operand_ids, uint32_t operand_count,
    bool keep_sources) {
  char *before_json = NULL;
  StygianEditorNode *node = NULL;
  StygianEditorNodeId id = STYGIAN_EDITOR_INVALID_ID;
  uint32_t i;
  if (!editor || !operand_ids || operand_count < 2u)
    return STYGIAN_EDITOR_INVALID_ID;
  if (operand_count > STYGIAN_EDITOR_NODE_BOOLEAN_OPERAND_CAP)
    return STYGIAN_EDITOR_INVALID_ID;
  if (op > STYGIAN_EDITOR_BOOLEAN_EXCLUDE)
    return STYGIAN_EDITOR_INVALID_ID;
  if (!editor_begin_mutation(editor, &before_json))
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->node_count >= editor->max_nodes)
    return editor_finish_mutation(editor, before_json, false),
           STYGIAN_EDITOR_INVALID_ID;

  for (i = 0u; i < operand_count; ++i) {
    const StygianEditorNode *operand =
        editor_find_node_const(editor, operand_ids[i]);
    if (!operand)
      return editor_finish_mutation(editor, before_json, false),
             STYGIAN_EDITOR_INVALID_ID;
  }

  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_GROUP;
  node->visible = true;
  node->selected = false;
  node->as.group.x = 0.0f;
  node->as.group.y = 0.0f;
  node->as.group.w = 0.0f;
  node->as.group.h = 0.0f;
  node->boolean_valid = true;
  node->boolean_op = op;
  node->boolean_operand_count = operand_count;
  for (i = 0u; i < operand_count; ++i) {
    node->boolean_operands[i] = operand_ids[i];
    if (!keep_sources) {
      StygianEditorNode *operand = editor_find_node(editor, operand_ids[i]);
      if (operand)
        operand->visible = false;
    }
  }
  if (!editor_boolean_solve_node(editor, node)) {
    return editor_finish_mutation(editor, before_json, false),
           STYGIAN_EDITOR_INVALID_ID;
  }
  editor_set_default_node_name(node);
  editor_constraints_capture(editor, node);
  id = node->id;
  if (!editor_finish_mutation(editor, before_json, true))
    return STYGIAN_EDITOR_INVALID_ID;
  return id;
}

bool stygian_editor_boolean_flatten(StygianEditor *editor,
                                    StygianEditorNodeId boolean_node_id,
                                    StygianEditorNodeId *out_path_node_id) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, boolean_node_id);
  StygianEditorPoint pts[STYGIAN_EDITOR_BOOLEAN_SOLVED_POINT_CAP];
  StygianEditorNodeId new_id = STYGIAN_EDITOR_INVALID_ID;
  uint32_t point_count = 0u;
  if (!node || !node->boolean_valid)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  if (!editor_boolean_solve_node(editor, node))
    return editor_finish_mutation(editor, before_json, false);
  point_count = node->boolean_solved_point_count;
  if (point_count < 3u) {
    if (node->as.group.w <= 0.0001f || node->as.group.h <= 0.0001f)
      return editor_finish_mutation(editor, before_json, false);
    point_count = 4u;
  }
  if (editor->node_count >= editor->max_nodes ||
      editor->point_count + point_count > editor->max_path_points) {
    return editor_finish_mutation(editor, before_json, false);
  }
  memset(pts, 0, sizeof(pts));
  if (point_count == 4u && node->boolean_solved_point_count < 3u) {
    pts[0].x = node->as.group.x;
    pts[0].y = node->as.group.y;
    pts[1].x = node->as.group.x + node->as.group.w;
    pts[1].y = node->as.group.y;
    pts[2].x = node->as.group.x + node->as.group.w;
    pts[2].y = node->as.group.y + node->as.group.h;
    pts[3].x = node->as.group.x;
    pts[3].y = node->as.group.y + node->as.group.h;
  } else {
    for (uint32_t i = 0u; i < point_count; ++i)
      pts[i] = node->boolean_solved_points[i];
  }
  for (uint32_t i = 0u; i < point_count; ++i) {
    pts[i].kind = STYGIAN_EDITOR_PATH_POINT_CORNER;
    pts[i].in_x = pts[i].x;
    pts[i].in_y = pts[i].y;
    pts[i].out_x = pts[i].x;
    pts[i].out_y = pts[i].y;
  }
  {
    StygianEditorNode *path_node = &editor->nodes[editor->node_count++];
    memset(path_node, 0, sizeof(*path_node));
    path_node->id = editor->next_node_id++;
    path_node->kind = STYGIAN_EDITOR_SHAPE_PATH;
    path_node->visible = node->visible;
    path_node->z = node->z;
    path_node->as.path.closed = true;
    path_node->as.path.thickness = 1.0f;
    path_node->as.path.stroke = stygian_editor_color_rgba(1.0f, 1.0f, 1.0f, 1.0f);
    if (!editor_node_path_replace_points(editor, path_node, pts, point_count))
      return editor_finish_mutation(editor, before_json, false);
    editor_node_seed_legacy_stroke(path_node);
    editor_set_default_node_name(path_node);
    editor_constraints_capture(editor, path_node);
    new_id = path_node->id;
  }
  node->visible = false;
  if (out_path_node_id)
    *out_path_node_id = new_id;
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_node_get_boolean(const StygianEditor *editor,
                                     StygianEditorNodeId node_id,
                                     StygianEditorBooleanOp *out_op,
                                     StygianEditorNodeId *out_operand_ids,
                                     uint32_t max_operands,
                                     uint32_t *out_operand_count) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  uint32_t i;
  if (!node || !node->boolean_valid)
    return false;
  if (out_op)
    *out_op = node->boolean_op;
  if (out_operand_count)
    *out_operand_count = node->boolean_operand_count;
  if (out_operand_ids && max_operands > 0u) {
    uint32_t n = node->boolean_operand_count < max_operands
                     ? node->boolean_operand_count
                     : max_operands;
    for (i = 0u; i < n; ++i)
      out_operand_ids[i] = node->boolean_operands[i];
  }
  return true;
}

bool stygian_editor_set_color_token(StygianEditor *editor, const char *name,
                                    StygianEditorColor color) {
  char *before_json = NULL;
  int idx;
  if (!editor || !name || !name[0])
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;

  idx = editor_find_color_token(editor, name);
  if (idx >= 0) {
    editor->color_tokens[idx].color = color;
    return editor_finish_mutation(editor, before_json, true);
  }

  if (editor->color_token_count >= editor->max_color_tokens)
    return editor_finish_mutation(editor, before_json, false);
  if (!editor_copy_token_name(editor->color_tokens[editor->color_token_count].name,
                              sizeof(editor->color_tokens[0].name), name)) {
    return editor_finish_mutation(editor, before_json, false);
  }

  editor->color_tokens[editor->color_token_count].color = color;
  editor->color_token_count += 1u;
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_get_color_token(const StygianEditor *editor,
                                    const char *name,
                                    StygianEditorColor *out_color) {
  int idx = editor_find_color_token(editor, name);
  if (idx < 0 || !out_color)
    return false;
  *out_color = editor->color_tokens[idx].color;
  return true;
}

bool stygian_editor_apply_color_token(StygianEditor *editor,
                                      StygianEditorNodeId node_id,
                                      const char *name) {
  char *before_json = NULL;
  bool was_suspended = false;
  StygianEditorColor color;
  StygianEditorNode *node;
  if (!editor || !name || !name[0])
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  if (!stygian_editor_get_color_token(editor, name, &color))
    return editor_finish_mutation(editor, before_json, false);
  node = editor_find_node(editor, node_id);
  if (!node)
    return editor_finish_mutation(editor, before_json, false);
  was_suspended = editor->history_suspended;
  editor->history_suspended = true;
  if (!stygian_editor_node_set_color(editor, node_id, color))
    goto apply_token_fail;
  editor->history_suspended = was_suspended;
  if (!editor_copy_token_name(node->color_token, sizeof(node->color_token), name))
    node->color_token[0] = '\0';
  if (node->kind == STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE) {
    node->as.component_instance.overrides.mask |=
        STYGIAN_EDITOR_COMPONENT_OVERRIDE_STYLE_BINDING;
    (void)editor_copy_short_name(node->as.component_instance.overrides.style_binding,
                                 sizeof(node->as.component_instance.overrides.style_binding),
                                 name);
  }
  return editor_finish_mutation(editor, before_json, true);

apply_token_fail:
  editor->history_suspended = was_suspended;
  return editor_finish_mutation(editor, before_json, false);
}

bool stygian_editor_set_text_style(StygianEditor *editor,
                                   const StygianEditorTextStyleDef *style_def) {
  char *before_json = NULL;
  int idx;
  if (!editor || !style_def || !style_def->name[0])
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  idx = editor_find_text_style(editor, style_def->name);
  if (idx < 0) {
    if (editor->text_style_count >= STYGIAN_EDITOR_STYLE_CAP)
      return editor_finish_mutation(editor, before_json, false);
    idx = (int)editor->text_style_count++;
  }
  editor->text_styles[idx].def = *style_def;
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_get_text_style(const StygianEditor *editor, const char *name,
                                   StygianEditorTextStyleDef *out_style_def) {
  int idx = editor_find_text_style(editor, name);
  if (idx < 0 || !out_style_def)
    return false;
  *out_style_def = editor->text_styles[idx].def;
  return true;
}

bool stygian_editor_apply_text_style(StygianEditor *editor,
                                     StygianEditorNodeId node_id,
                                     const char *name) {
  char *before_json = NULL;
  int idx = editor_find_text_style(editor, name);
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (idx < 0 || !node || node->kind != STYGIAN_EDITOR_SHAPE_TEXT)
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->as.text.font_size = editor->text_styles[idx].def.font_size;
  node->as.text.line_height = editor->text_styles[idx].def.line_height;
  node->as.text.letter_spacing = editor->text_styles[idx].def.letter_spacing;
  node->as.text.font_weight = editor->text_styles[idx].def.font_weight;
  node->as.text.fill = editor->text_styles[idx].def.color;
  (void)editor_copy_short_name(node->text_style, sizeof(node->text_style), name);
  editor_node_set_legacy_fill_color(node, node->as.text.fill);
  editor_node_fill_sync_from_legacy(node);
  editor_text_apply_auto_size(node);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_set_effect_style(
    StygianEditor *editor, const StygianEditorEffectStyleDef *style_def) {
  char *before_json = NULL;
  int idx;
  if (!editor || !style_def || !style_def->name[0])
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  idx = editor_find_effect_style(editor, style_def->name);
  if (idx < 0) {
    if (editor->effect_style_count >= STYGIAN_EDITOR_STYLE_CAP)
      return editor_finish_mutation(editor, before_json, false);
    idx = (int)editor->effect_style_count++;
  }
  editor->effect_styles[idx].def = *style_def;
  if (editor->effect_styles[idx].def.effect_count > STYGIAN_EDITOR_NODE_EFFECT_CAP)
    editor->effect_styles[idx].def.effect_count = STYGIAN_EDITOR_NODE_EFFECT_CAP;
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_get_effect_style(
    const StygianEditor *editor, const char *name,
    StygianEditorEffectStyleDef *out_style_def) {
  int idx = editor_find_effect_style(editor, name);
  if (idx < 0 || !out_style_def)
    return false;
  *out_style_def = editor->effect_styles[idx].def;
  return true;
}

bool stygian_editor_apply_effect_style(StygianEditor *editor,
                                       StygianEditorNodeId node_id,
                                       const char *name) {
  char *before_json = NULL;
  int idx = editor_find_effect_style(editor, name);
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (idx < 0 || !node || !editor_node_supports_effects(node->kind))
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->effect_count = editor->effect_styles[idx].def.effect_count;
  memcpy(node->effects, editor->effect_styles[idx].def.effects,
         sizeof(node->effects));
  for (uint32_t i = 0u; i < STYGIAN_EDITOR_NODE_EFFECT_CAP; ++i)
    node->effect_xform[i] = editor_effect_xform_default();
  (void)editor_copy_short_name(node->effect_style, sizeof(node->effect_style),
                               name);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_set_layout_style(
    StygianEditor *editor, const StygianEditorLayoutStyleDef *style_def) {
  char *before_json = NULL;
  int idx;
  if (!editor || !style_def || !style_def->name[0])
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  idx = editor_find_layout_style(editor, style_def->name);
  if (idx < 0) {
    if (editor->layout_style_count >= STYGIAN_EDITOR_STYLE_CAP)
      return editor_finish_mutation(editor, before_json, false);
    idx = (int)editor->layout_style_count++;
  }
  editor->layout_styles[idx].def = *style_def;
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_get_layout_style(
    const StygianEditor *editor, const char *name,
    StygianEditorLayoutStyleDef *out_style_def) {
  int idx = editor_find_layout_style(editor, name);
  if (idx < 0 || !out_style_def)
    return false;
  *out_style_def = editor->layout_styles[idx].def;
  return true;
}

bool stygian_editor_apply_layout_style(StygianEditor *editor,
                                       StygianEditorNodeId frame_node_id,
                                       const char *name) {
  char *before_json = NULL;
  int idx = editor_find_layout_style(editor, name);
  StygianEditorNode *node = editor_find_node(editor, frame_node_id);
  if (idx < 0 || !node || node->kind != STYGIAN_EDITOR_SHAPE_FRAME)
    return false;
  if (node->locked)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  node->as.frame.layout_mode = editor->layout_styles[idx].def.layout.mode;
  node->as.frame.layout_wrap = editor->layout_styles[idx].def.layout.wrap;
  node->as.frame.layout_padding_left =
      editor->layout_styles[idx].def.layout.padding_left;
  node->as.frame.layout_padding_right =
      editor->layout_styles[idx].def.layout.padding_right;
  node->as.frame.layout_padding_top =
      editor->layout_styles[idx].def.layout.padding_top;
  node->as.frame.layout_padding_bottom =
      editor->layout_styles[idx].def.layout.padding_bottom;
  node->as.frame.layout_gap = editor->layout_styles[idx].def.layout.gap;
  node->as.frame.layout_primary_align =
      editor->layout_styles[idx].def.layout.primary_align;
  node->as.frame.layout_cross_align =
      editor->layout_styles[idx].def.layout.cross_align;
  (void)editor_copy_short_name(node->layout_style, sizeof(node->layout_style),
                               name);
  editor_request_repaint(editor, 120u);
  return editor_finish_mutation(editor, before_json, true);
}

uint32_t stygian_editor_style_usage_count(const StygianEditor *editor,
                                          StygianEditorStyleKind kind,
                                          const char *name) {
  uint32_t i;
  uint32_t count = 0u;
  if (!editor || !name || !name[0])
    return 0u;
  for (i = 0u; i < editor->node_count; ++i) {
    const StygianEditorNode *node = &editor->nodes[i];
    switch (kind) {
    case STYGIAN_EDITOR_STYLE_TEXT:
      if (strncmp(node->text_style, name, sizeof(node->text_style)) == 0)
        count += 1u;
      break;
    case STYGIAN_EDITOR_STYLE_EFFECT:
      if (strncmp(node->effect_style, name, sizeof(node->effect_style)) == 0)
        count += 1u;
      break;
    case STYGIAN_EDITOR_STYLE_LAYOUT:
      if (strncmp(node->layout_style, name, sizeof(node->layout_style)) == 0)
        count += 1u;
      break;
    default:
      break;
    }
  }
  return count;
}

bool stygian_editor_set_variable_mode(StygianEditor *editor, uint32_t mode_index,
                                      const char *mode_name) {
  char *before_json = NULL;
  if (!editor || !mode_name || !mode_name[0] ||
      mode_index >= STYGIAN_EDITOR_VARIABLE_MODE_CAP) {
    return false;
  }
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  if (!editor_copy_short_name(editor->variable_modes[mode_index],
                              sizeof(editor->variable_modes[mode_index]),
                              mode_name)) {
    return editor_finish_mutation(editor, before_json, false);
  }
  if (editor->variable_mode_count <= mode_index)
    editor->variable_mode_count = mode_index + 1u;
  return editor_finish_mutation(editor, before_json, true);
}

uint32_t stygian_editor_variable_mode_count(const StygianEditor *editor) {
  if (!editor)
    return 0u;
  return editor->variable_mode_count;
}

bool stygian_editor_get_variable_mode(const StygianEditor *editor,
                                      uint32_t mode_index, char *out_mode_name,
                                      size_t out_mode_name_capacity) {
  size_t len = 0u;
  if (!editor || !out_mode_name || out_mode_name_capacity == 0u ||
      mode_index >= editor->variable_mode_count) {
    return false;
  }
  len = strlen(editor->variable_modes[mode_index]);
  if (len >= out_mode_name_capacity)
    return false;
  memcpy(out_mode_name, editor->variable_modes[mode_index], len + 1u);
  return true;
}

bool stygian_editor_set_active_variable_mode(StygianEditor *editor,
                                             uint32_t mode_index) {
  char *before_json = NULL;
  if (!editor || mode_index >= editor->variable_mode_count)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  editor->active_variable_mode = mode_index;
  return editor_finish_mutation(editor, before_json, true);
}

uint32_t stygian_editor_get_active_variable_mode(const StygianEditor *editor) {
  if (!editor)
    return 0u;
  return editor->active_variable_mode;
}

bool stygian_editor_set_color_variable(StygianEditor *editor, const char *name,
                                       uint32_t mode_index,
                                       StygianEditorColor value) {
  char *before_json = NULL;
  int idx;
  if (!editor || !name || !name[0] || mode_index >= editor->variable_mode_count)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  idx = editor_find_variable(editor, name, STYGIAN_EDITOR_VARIABLE_COLOR);
  if (idx < 0) {
    if (editor->variable_count >= STYGIAN_EDITOR_VARIABLE_CAP)
      return editor_finish_mutation(editor, before_json, false);
    idx = (int)editor->variable_count++;
    memset(&editor->variables[idx], 0, sizeof(editor->variables[idx]));
    editor->variables[idx].def.kind = STYGIAN_EDITOR_VARIABLE_COLOR;
    if (!editor_copy_short_name(editor->variables[idx].def.name,
                                sizeof(editor->variables[idx].def.name), name)) {
      editor->variable_count -= 1u;
      return editor_finish_mutation(editor, before_json, false);
    }
  }
  editor->variables[idx].def.color_values[mode_index] = value;
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_get_color_variable(const StygianEditor *editor,
                                       const char *name, uint32_t mode_index,
                                       StygianEditorColor *out_value) {
  int idx = editor_find_variable(editor, name, STYGIAN_EDITOR_VARIABLE_COLOR);
  if (idx < 0 || !out_value || mode_index >= editor->variable_mode_count)
    return false;
  *out_value = editor->variables[idx].def.color_values[mode_index];
  return true;
}

bool stygian_editor_set_number_variable(StygianEditor *editor, const char *name,
                                        uint32_t mode_index, float value) {
  char *before_json = NULL;
  int idx;
  if (!editor || !name || !name[0] || mode_index >= editor->variable_mode_count)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  idx = editor_find_variable(editor, name, STYGIAN_EDITOR_VARIABLE_NUMBER);
  if (idx < 0) {
    if (editor->variable_count >= STYGIAN_EDITOR_VARIABLE_CAP)
      return editor_finish_mutation(editor, before_json, false);
    idx = (int)editor->variable_count++;
    memset(&editor->variables[idx], 0, sizeof(editor->variables[idx]));
    editor->variables[idx].def.kind = STYGIAN_EDITOR_VARIABLE_NUMBER;
    if (!editor_copy_short_name(editor->variables[idx].def.name,
                                sizeof(editor->variables[idx].def.name), name)) {
      editor->variable_count -= 1u;
      return editor_finish_mutation(editor, before_json, false);
    }
  }
  editor->variables[idx].def.number_values[mode_index] = value;
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_get_number_variable(const StygianEditor *editor,
                                        const char *name, uint32_t mode_index,
                                        float *out_value) {
  int idx = editor_find_variable(editor, name, STYGIAN_EDITOR_VARIABLE_NUMBER);
  if (idx < 0 || !out_value || mode_index >= editor->variable_mode_count)
    return false;
  *out_value = editor->variables[idx].def.number_values[mode_index];
  return true;
}

bool stygian_editor_bind_node_color_variable(StygianEditor *editor,
                                             StygianEditorNodeId node_id,
                                             const char *variable_name) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!node || !variable_name || !variable_name[0])
    return false;
  if (node->locked)
    return false;
  if (editor_find_variable(editor, variable_name, STYGIAN_EDITOR_VARIABLE_COLOR) < 0)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  if (!editor_copy_short_name(node->color_variable, sizeof(node->color_variable),
                              variable_name)) {
    return editor_finish_mutation(editor, before_json, false);
  }
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_bind_node_number_variable(
    StygianEditor *editor, StygianEditorNodeId node_id,
    StygianEditorPropertyKind property, const char *variable_name) {
  char *before_json = NULL;
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!node || !variable_name || !variable_name[0])
    return false;
  if (node->locked)
    return false;
  if (!editor_property_allows_number_variable(property)) {
    return false;
  }
  if (editor_find_variable(editor, variable_name, STYGIAN_EDITOR_VARIABLE_NUMBER) <
      0) {
    return false;
  }
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  if (!editor_copy_short_name(node->number_variable,
                              sizeof(node->number_variable), variable_name)) {
    return editor_finish_mutation(editor, before_json, false);
  }
  node->number_variable_property = property;
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_apply_active_variable_mode(StygianEditor *editor) {
  uint32_t i;
  uint32_t mode = 0u;
  bool ok = true;
  if (!editor || editor->variable_mode_count == 0u)
    return false;
  mode = editor->active_variable_mode;
  if (mode >= editor->variable_mode_count)
    return false;
  for (i = 0u; i < editor->node_count; ++i) {
    StygianEditorNode *node = &editor->nodes[i];
    if (node->color_variable[0]) {
      int vidx =
          editor_find_variable(editor, node->color_variable,
                               STYGIAN_EDITOR_VARIABLE_COLOR);
      if (vidx >= 0) {
        if (!stygian_editor_node_set_color(editor, node->id,
                                           editor->variables[vidx]
                                               .def.color_values[mode])) {
          ok = false;
        }
      }
    }
    if (node->number_variable[0]) {
      int vidx =
          editor_find_variable(editor, node->number_variable,
                               STYGIAN_EDITOR_VARIABLE_NUMBER);
      if (vidx >= 0) {
        if (!editor_node_apply_property(editor, node, node->number_variable_property,
                                        editor->variables[vidx]
                                            .def.number_values[mode])) {
          ok = false;
        }
      }
    }
  }
  return ok;
}

bool stygian_editor_add_control_recipe(StygianEditor *editor,
                                       StygianEditorControlRecipeKind kind,
                                       float x, float y, float w, float h,
                                       const char *name,
                                       StygianEditorNodeId *out_root_id) {
  StygianEditorRectDesc rect = {0};
  StygianEditorEllipseDesc ellipse = {0};
  StygianEditorFrameDesc frame = {0};
  StygianEditorTextDesc text = {0};
  StygianEditorBehaviorRule rule;
  StygianEditorNodeId root = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId child = STYGIAN_EDITOR_INVALID_ID;
  (void)name;
  if (!editor || w <= 0.0f || h <= 0.0f)
    return false;

  if (kind == STYGIAN_EDITOR_RECIPE_BUTTON) {
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    rect.radius[0] = rect.radius[1] = rect.radius[2] = rect.radius[3] = 8.0f;
    rect.fill = stygian_editor_color_rgba(0.20f, 0.45f, 0.86f, 1.0f);
    rect.visible = true;
    root = stygian_editor_add_rect(editor, &rect);
    text.x = x + 10.0f;
    text.y = y + 10.0f;
    text.w = w - 20.0f;
    text.h = h - 20.0f;
    text.font_size = 16.0f;
    text.fill = stygian_editor_color_rgba(1, 1, 1, 1);
    text.text = name && name[0] ? name : "Button";
    text.visible = true;
    child = stygian_editor_add_text(editor, &text);
    (void)stygian_editor_reparent_node(editor, child, root, true);
    memset(&rule, 0, sizeof(rule));
    rule.trigger_node = root;
    rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
    rule.action_kind = STYGIAN_EDITOR_ACTION_SET_PROPERTY;
    rule.set_property.target_node = root;
    rule.set_property.property = STYGIAN_EDITOR_PROP_OPACITY;
    rule.set_property.value = 0.75f;
    (void)stygian_editor_add_behavior(editor, &rule, NULL);
  } else if (kind == STYGIAN_EDITOR_RECIPE_SLIDER) {
    frame.x = x;
    frame.y = y;
    frame.w = w;
    frame.h = h;
    frame.fill = stygian_editor_color_rgba(0.20f, 0.20f, 0.24f, 1.0f);
    frame.visible = true;
    root = stygian_editor_add_frame(editor, &frame);
    ellipse.x = x;
    ellipse.y = y;
    ellipse.w = h;
    ellipse.h = h;
    ellipse.fill = stygian_editor_color_rgba(0.80f, 0.85f, 0.95f, 1.0f);
    child = stygian_editor_add_ellipse(editor, &ellipse);
    (void)stygian_editor_reparent_node(editor, child, root, true);
  } else if (kind == STYGIAN_EDITOR_RECIPE_TOGGLE) {
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    rect.radius[0] = rect.radius[1] = rect.radius[2] = rect.radius[3] = h * 0.5f;
    rect.fill = stygian_editor_color_rgba(0.25f, 0.27f, 0.30f, 1.0f);
    rect.visible = true;
    root = stygian_editor_add_rect(editor, &rect);
    ellipse.x = x + 2.0f;
    ellipse.y = y + 2.0f;
    ellipse.w = h - 4.0f;
    ellipse.h = h - 4.0f;
    ellipse.fill = stygian_editor_color_rgba(0.95f, 0.95f, 0.95f, 1.0f);
    child = stygian_editor_add_ellipse(editor, &ellipse);
    (void)stygian_editor_reparent_node(editor, child, root, true);
    memset(&rule, 0, sizeof(rule));
    rule.trigger_node = root;
    rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
    rule.action_kind = STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY;
    rule.toggle_visibility.target_node = child;
    (void)stygian_editor_add_behavior(editor, &rule, NULL);
  } else if (kind == STYGIAN_EDITOR_RECIPE_SCROLL_REGION) {
    frame.x = x;
    frame.y = y;
    frame.w = w;
    frame.h = h;
    frame.clip_content = true;
    frame.fill = stygian_editor_color_rgba(0.14f, 0.14f, 0.16f, 1.0f);
    frame.visible = true;
    root = stygian_editor_add_frame(editor, &frame);
    (void)stygian_editor_frame_set_overflow_policy(
        editor, root, STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_BOTH);
  } else if (kind == STYGIAN_EDITOR_RECIPE_TABS) {
    frame.x = x;
    frame.y = y;
    frame.w = w;
    frame.h = h;
    frame.fill = stygian_editor_color_rgba(0.10f, 0.11f, 0.13f, 1.0f);
    frame.visible = true;
    root = stygian_editor_add_frame(editor, &frame);
    rect.w = w / 3.0f;
    rect.h = h;
    rect.y = y;
    rect.fill = stygian_editor_color_rgba(0.22f, 0.23f, 0.26f, 1.0f);
    rect.visible = true;
    rect.x = x;
    child = stygian_editor_add_rect(editor, &rect);
    (void)stygian_editor_reparent_node(editor, child, root, true);
    rect.x = x + rect.w;
    child = stygian_editor_add_rect(editor, &rect);
    (void)stygian_editor_reparent_node(editor, child, root, true);
    rect.x = x + rect.w * 2.0f;
    child = stygian_editor_add_rect(editor, &rect);
    (void)stygian_editor_reparent_node(editor, child, root, true);
  } else if (kind == STYGIAN_EDITOR_RECIPE_ACCORDION) {
    frame.x = x;
    frame.y = y;
    frame.w = w;
    frame.h = h;
    frame.fill = stygian_editor_color_rgba(0.10f, 0.11f, 0.13f, 1.0f);
    frame.visible = true;
    root = stygian_editor_add_frame(editor, &frame);
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h * 0.28f;
    rect.fill = stygian_editor_color_rgba(0.22f, 0.23f, 0.26f, 1.0f);
    rect.visible = true;
    child = stygian_editor_add_rect(editor, &rect);
    (void)stygian_editor_reparent_node(editor, child, root, true);
    text.x = x + 10.0f;
    text.y = y + 8.0f;
    text.w = w - 20.0f;
    text.h = rect.h - 10.0f;
    text.font_size = 14.0f;
    text.fill = stygian_editor_color_rgba(0.95f, 0.95f, 0.96f, 1.0f);
    text.text = name && name[0] ? name : "Accordion";
    text.visible = true;
    {
      StygianEditorNodeId header_text = stygian_editor_add_text(editor, &text);
      (void)stygian_editor_reparent_node(editor, header_text, root, true);
    }
  } else if (kind == STYGIAN_EDITOR_RECIPE_CHECKBOX) {
    rect.x = x;
    rect.y = y;
    rect.w = h;
    rect.h = h;
    rect.radius[0] = rect.radius[1] = rect.radius[2] = rect.radius[3] = 4.0f;
    rect.fill = stygian_editor_color_rgba(0.20f, 0.22f, 0.26f, 1.0f);
    rect.visible = true;
    root = stygian_editor_add_rect(editor, &rect);
    child = stygian_editor_add_text(editor, &(StygianEditorTextDesc){
                                              .x = x + h + 10.0f,
                                              .y = y + 2.0f,
                                              .w = w - h - 10.0f,
                                              .h = h,
                                              .font_size = 14.0f,
                                              .fill = stygian_editor_color_rgba(
                                                  0.92f, 0.93f, 0.96f, 1.0f),
                                              .text = name && name[0] ? name
                                                                      : "Checkbox",
                                              .visible = true});
    if (child)
      (void)stygian_editor_reparent_node(editor, child, root, true);
    child = stygian_editor_add_rect(editor, &(StygianEditorRectDesc){
                                               .x = x + 3.0f,
                                               .y = y + 3.0f,
                                               .w = h - 6.0f,
                                               .h = h - 6.0f,
                                               .radius = {2.0f, 2.0f, 2.0f, 2.0f},
                                               .fill = stygian_editor_color_rgba(
                                                   0.18f, 0.60f, 0.34f, 1.0f),
                                               .visible = false});
    if (child) {
      (void)stygian_editor_reparent_node(editor, child, root, true);
      memset(&rule, 0, sizeof(rule));
      rule.trigger_node = root;
      rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
      rule.action_kind = STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY;
      rule.toggle_visibility.target_node = child;
      (void)stygian_editor_add_behavior(editor, &rule, NULL);
    }
  } else if (kind == STYGIAN_EDITOR_RECIPE_RADIO_GROUP) {
    frame.x = x;
    frame.y = y;
    frame.w = w;
    frame.h = h;
    frame.fill = stygian_editor_color_rgba(0.10f, 0.11f, 0.13f, 1.0f);
    frame.visible = true;
    root = stygian_editor_add_frame(editor, &frame);
    for (uint32_t ri = 0u; ri < 3u; ++ri) {
      float cy = y + 6.0f + (float)ri * ((h - 12.0f) / 3.0f);
      StygianEditorNodeId dot;
      ellipse.x = x + 8.0f;
      ellipse.y = cy;
      ellipse.w = 14.0f;
      ellipse.h = 14.0f;
      ellipse.fill = stygian_editor_color_rgba(0.24f, 0.26f, 0.30f, 1.0f);
      child = stygian_editor_add_ellipse(editor, &ellipse);
      if (child)
        (void)stygian_editor_reparent_node(editor, child, root, true);
      dot = stygian_editor_add_ellipse(editor, &(StygianEditorEllipseDesc){
                                                  .x = x + 11.0f,
                                                  .y = cy + 3.0f,
                                                  .w = 8.0f,
                                                  .h = 8.0f,
                                                  .fill = stygian_editor_color_rgba(
                                                      0.20f, 0.55f, 0.92f, 1.0f),
                                                  .visible = ri == 0u});
      if (dot)
        (void)stygian_editor_reparent_node(editor, dot, root, true);
    }
  } else if (kind == STYGIAN_EDITOR_RECIPE_INPUT_FIELD) {
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    rect.radius[0] = rect.radius[1] = rect.radius[2] = rect.radius[3] = 6.0f;
    rect.fill = stygian_editor_color_rgba(0.13f, 0.14f, 0.17f, 1.0f);
    rect.visible = true;
    root = stygian_editor_add_rect(editor, &rect);
    text.x = x + 10.0f;
    text.y = y + (h * 0.5f - 8.0f);
    text.w = w - 20.0f;
    text.h = 16.0f;
    text.font_size = 14.0f;
    text.fill = stygian_editor_color_rgba(0.62f, 0.66f, 0.72f, 1.0f);
    text.text = name && name[0] ? name : "Input";
    text.visible = true;
    child = stygian_editor_add_text(editor, &text);
    if (child)
      (void)stygian_editor_reparent_node(editor, child, root, true);
  } else if (kind == STYGIAN_EDITOR_RECIPE_DROPDOWN) {
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    rect.radius[0] = rect.radius[1] = rect.radius[2] = rect.radius[3] = 6.0f;
    rect.fill = stygian_editor_color_rgba(0.13f, 0.14f, 0.17f, 1.0f);
    rect.visible = true;
    root = stygian_editor_add_rect(editor, &rect);
    text.x = x + 10.0f;
    text.y = y + (h * 0.5f - 8.0f);
    text.w = w - 30.0f;
    text.h = 16.0f;
    text.font_size = 14.0f;
    text.fill = stygian_editor_color_rgba(0.90f, 0.92f, 0.95f, 1.0f);
    text.text = name && name[0] ? name : "Select";
    text.visible = true;
    child = stygian_editor_add_text(editor, &text);
    if (child)
      (void)stygian_editor_reparent_node(editor, child, root, true);
    text.x = x + w - 18.0f;
    text.y = y + (h * 0.5f - 8.0f);
    text.w = 10.0f;
    text.h = 16.0f;
    text.font_size = 14.0f;
    text.fill = stygian_editor_color_rgba(0.70f, 0.74f, 0.80f, 1.0f);
    text.text = "v";
    text.visible = true;
    child = stygian_editor_add_text(editor, &text);
    if (child)
      (void)stygian_editor_reparent_node(editor, child, root, true);
  } else if (kind == STYGIAN_EDITOR_RECIPE_DATA_TABLE) {
    frame.x = x;
    frame.y = y;
    frame.w = w;
    frame.h = h;
    frame.fill = stygian_editor_color_rgba(0.09f, 0.10f, 0.12f, 1.0f);
    frame.visible = true;
    root = stygian_editor_add_frame(editor, &frame);
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h * 0.18f;
    rect.fill = stygian_editor_color_rgba(0.18f, 0.20f, 0.24f, 1.0f);
    rect.visible = true;
    child = stygian_editor_add_rect(editor, &rect);
    if (child)
      (void)stygian_editor_reparent_node(editor, child, root, true);
    for (uint32_t row = 0u; row < 3u; ++row) {
      rect.x = x;
      rect.y = y + h * 0.18f + (float)row * ((h * 0.82f) / 3.0f);
      rect.w = w;
      rect.h = (h * 0.82f) / 3.0f - 2.0f;
      rect.fill = stygian_editor_color_rgba(0.13f + 0.01f * (float)row,
                                            0.14f + 0.01f * (float)row,
                                            0.17f + 0.01f * (float)row, 1.0f);
      rect.visible = true;
      child = stygian_editor_add_rect(editor, &rect);
      if (child)
        (void)stygian_editor_reparent_node(editor, child, root, true);
    }
  } else if (kind == STYGIAN_EDITOR_RECIPE_DATA_CARD) {
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    rect.radius[0] = rect.radius[1] = rect.radius[2] = rect.radius[3] = 12.0f;
    rect.fill = stygian_editor_color_rgba(0.12f, 0.13f, 0.16f, 1.0f);
    rect.visible = true;
    root = stygian_editor_add_rect(editor, &rect);
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h * 0.46f;
    rect.radius[0] = rect.radius[1] = 12.0f;
    rect.radius[2] = rect.radius[3] = 0.0f;
    rect.fill = stygian_editor_color_rgba(0.22f, 0.36f, 0.58f, 1.0f);
    rect.visible = true;
    child = stygian_editor_add_rect(editor, &rect);
    if (child)
      (void)stygian_editor_reparent_node(editor, child, root, true);
    text.x = x + 10.0f;
    text.y = y + h * 0.56f;
    text.w = w - 20.0f;
    text.h = 18.0f;
    text.font_size = 14.0f;
    text.fill = stygian_editor_color_rgba(0.94f, 0.95f, 0.98f, 1.0f);
    text.text = name && name[0] ? name : "Card title";
    text.visible = true;
    child = stygian_editor_add_text(editor, &text);
    if (child)
      (void)stygian_editor_reparent_node(editor, child, root, true);
  } else {
    return false;
  }

  if (out_root_id)
    *out_root_id = root;
  return root != STYGIAN_EDITOR_INVALID_ID;
}

bool stygian_editor_add_behavior(StygianEditor *editor,
                                 const StygianEditorBehaviorRule *rule,
                                 StygianEditorBehaviorId *out_behavior_id) {
  char *before_json = NULL;
  StygianEditorBehaviorSlot *slot;
  if (!editor || !rule)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  if (editor->behavior_count >= editor->max_behaviors)
    return editor_finish_mutation(editor, before_json, false);
  if (editor_find_node_index(editor, rule->trigger_node) < 0)
    return editor_finish_mutation(editor, before_json, false);
  if (rule->action_kind > STYGIAN_EDITOR_ACTION_NAVIGATE)
    return editor_finish_mutation(editor, before_json, false);
  if (rule->action_kind == STYGIAN_EDITOR_ACTION_ANIMATE) {
    if (editor_find_node_index(editor, rule->animate.target_node) < 0)
      return editor_finish_mutation(editor, before_json, false);
  } else if (rule->action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY) {
    if (editor_find_node_index(editor, rule->set_property.target_node) < 0)
      return editor_finish_mutation(editor, before_json, false);
  } else if (rule->action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY) {
    if (editor_find_node_index(editor, rule->toggle_visibility.target_node) < 0)
      return editor_finish_mutation(editor, before_json, false);
  }

  slot = &editor->behaviors[editor->behavior_count];
  memset(slot, 0, sizeof(*slot));
  slot->id = editor->next_behavior_id++;
  slot->rule = *rule;
  editor->active_anims[editor->behavior_count].active = false;
  editor->behavior_count += 1u;

  if (out_behavior_id)
    *out_behavior_id = slot->id;
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_remove_behavior(StygianEditor *editor,
                                    StygianEditorBehaviorId behavior_id) {
  char *before_json = NULL;
  uint32_t i;
  if (!editor || behavior_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  for (i = 0u; i < editor->behavior_count; ++i) {
    if (editor->behaviors[i].id == behavior_id) {
      uint32_t j;
      for (j = i; (j + 1u) < editor->behavior_count; ++j) {
        editor->behaviors[j] = editor->behaviors[j + 1u];
        editor->active_anims[j] = editor->active_anims[j + 1u];
      }
      editor->behavior_count -= 1u;
      return editor_finish_mutation(editor, before_json, true);
    }
  }
  return editor_finish_mutation(editor, before_json, false);
}

uint32_t stygian_editor_behavior_count(const StygianEditor *editor) {
  if (!editor)
    return 0u;
  return editor->behavior_count;
}

bool stygian_editor_get_behavior_rule(const StygianEditor *editor,
                                      uint32_t index,
                                      StygianEditorBehaviorId *out_behavior_id,
                                      StygianEditorBehaviorRule *out_rule) {
  if (!editor)
    return false;
  if (index >= editor->behavior_count)
    return false;
  if (out_behavior_id)
    *out_behavior_id = editor->behaviors[index].id;
  if (out_rule)
    *out_rule = editor->behaviors[index].rule;
  return true;
}

bool stygian_editor_add_driver(StygianEditor *editor,
                               const StygianEditorDriverRule *rule,
                               StygianEditorDriverId *out_driver_id) {
  char *before_json = NULL;
  StygianEditorDriverSlot *slot;
  if (!editor || !rule || !rule->variable_name[0])
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  if (editor->driver_count >= editor->max_drivers)
    return editor_finish_mutation(editor, before_json, false);
  if (editor_find_node_index(editor, rule->source_node) < 0)
    return editor_finish_mutation(editor, before_json, false);
  if (!editor_property_allows_number_variable(rule->source_property))
    return editor_finish_mutation(editor, before_json, false);
  if (editor_find_variable(editor, rule->variable_name,
                           STYGIAN_EDITOR_VARIABLE_NUMBER) < 0) {
    if (!stygian_editor_set_number_variable(editor, rule->variable_name, 0u,
                                            rule->out_min)) {
      return editor_finish_mutation(editor, before_json, false);
    }
  }
  slot = &editor->drivers[editor->driver_count];
  memset(slot, 0, sizeof(*slot));
  slot->id = editor->next_driver_id++;
  slot->rule = *rule;
  editor->driver_count += 1u;
  if (out_driver_id)
    *out_driver_id = slot->id;
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_remove_driver(StygianEditor *editor,
                                  StygianEditorDriverId driver_id) {
  char *before_json = NULL;
  uint32_t i;
  if (!editor || driver_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  for (i = 0u; i < editor->driver_count; ++i) {
    if (editor->drivers[i].id == driver_id) {
      for (; (i + 1u) < editor->driver_count; ++i)
        editor->drivers[i] = editor->drivers[i + 1u];
      editor->driver_count -= 1u;
      return editor_finish_mutation(editor, before_json, true);
    }
  }
  return editor_finish_mutation(editor, before_json, false);
}

uint32_t stygian_editor_driver_count(const StygianEditor *editor) {
  if (!editor)
    return 0u;
  return editor->driver_count;
}

bool stygian_editor_get_driver_rule(const StygianEditor *editor, uint32_t index,
                                    StygianEditorDriverId *out_driver_id,
                                    StygianEditorDriverRule *out_rule) {
  if (!editor || index >= editor->driver_count)
    return false;
  if (out_driver_id)
    *out_driver_id = editor->drivers[index].id;
  if (out_rule)
    *out_rule = editor->drivers[index].rule;
  return true;
}

bool stygian_editor_apply_driver_sample(StygianEditor *editor,
                                        StygianEditorDriverId driver_id,
                                        float source_value) {
  int idx;
  int vidx;
  uint32_t mode;
  float mapped;
  uint32_t i;
  if (!editor || driver_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  idx = editor_find_driver_index(editor, driver_id);
  if (idx < 0)
    return false;
  mode = editor->drivers[idx].rule.use_active_mode
             ? editor->active_variable_mode
             : editor->drivers[idx].rule.mode_index;
  if (mode >= editor->variable_mode_count)
    mode = editor->active_variable_mode;
  mapped = editor_driver_map_value(&editor->drivers[idx].rule, source_value);
  vidx = editor_find_variable(editor, editor->drivers[idx].rule.variable_name,
                              STYGIAN_EDITOR_VARIABLE_NUMBER);
  if (vidx < 0)
    return false;
  editor->variables[vidx].def.number_values[mode] = mapped;
  for (i = 0u; i < editor->node_count; ++i) {
    StygianEditorNode *node = &editor->nodes[i];
    if (!node->number_variable[0] ||
        strcmp(node->number_variable, editor->drivers[idx].rule.variable_name) != 0) {
      continue;
    }
    if (!editor_node_apply_property(editor, node, node->number_variable_property,
                                    editor->variables[vidx].def.number_values[mode])) {
      return false;
    }
  }
  editor_request_repaint(editor, 120u);
  return true;
}

bool stygian_editor_timeline_add_track(
    StygianEditor *editor, const StygianEditorTimelineTrack *track_template,
    StygianEditorTimelineTrackId *out_track_id) {
  char *before_json = NULL;
  StygianEditorTimelineTrackSlot *slot;
  if (!editor || !track_template)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  if (editor->timeline_track_count >= editor->max_timeline_tracks)
    return editor_finish_mutation(editor, before_json, false);
  if (editor_find_node_index(editor, track_template->target_node) < 0)
    return editor_finish_mutation(editor, before_json, false);
  if (track_template->keyframe_count > STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP)
    return editor_finish_mutation(editor, before_json, false);
  slot = &editor->timeline_tracks[editor->timeline_track_count];
  memset(slot, 0, sizeof(*slot));
  slot->id = editor->next_timeline_track_id++;
  slot->track = *track_template;
  slot->track.id = slot->id;
  if (slot->track.keyframe_count > 1u) {
    qsort(slot->track.keyframes, slot->track.keyframe_count,
          sizeof(StygianEditorTimelineKeyframe),
          editor_timeline_keyframe_compare);
  }
  editor->timeline_track_count += 1u;
  if (out_track_id)
    *out_track_id = slot->id;
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_timeline_remove_track(StygianEditor *editor,
                                          StygianEditorTimelineTrackId track_id) {
  char *before_json = NULL;
  uint32_t i;
  if (!editor || track_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  for (i = 0u; i < editor->timeline_track_count; ++i) {
    if (editor->timeline_tracks[i].id == track_id) {
      uint32_t j;
      for (j = i; (j + 1u) < editor->timeline_track_count; ++j)
        editor->timeline_tracks[j] = editor->timeline_tracks[j + 1u];
      editor->timeline_track_count -= 1u;
      for (j = 0u; j < editor->timeline_clip_count; ++j) {
        StygianEditorTimelineClip *clip = &editor->timeline_clips[j].clip;
        uint32_t r = 0u;
        for (uint32_t k = 0u; k < clip->track_count; ++k) {
          if (clip->track_ids[k] != track_id)
            clip->track_ids[r++] = clip->track_ids[k];
        }
        clip->track_count = r;
      }
      return editor_finish_mutation(editor, before_json, true);
    }
  }
  return editor_finish_mutation(editor, before_json, false);
}

uint32_t stygian_editor_timeline_track_count(const StygianEditor *editor) {
  if (!editor)
    return 0u;
  return editor->timeline_track_count;
}

bool stygian_editor_timeline_get_track(const StygianEditor *editor, uint32_t index,
                                       StygianEditorTimelineTrack *out_track) {
  if (!editor || !out_track)
    return false;
  if (index >= editor->timeline_track_count)
    return false;
  *out_track = editor->timeline_tracks[index].track;
  return true;
}

bool stygian_editor_timeline_set_track_keyframes(
    StygianEditor *editor, StygianEditorTimelineTrackId track_id,
    const StygianEditorTimelineKeyframe *keyframes, uint32_t keyframe_count) {
  char *before_json = NULL;
  int idx;
  StygianEditorTimelineTrack *track;
  if (!editor || track_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  if (keyframe_count > STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP)
    return false;
  if (keyframe_count > 0u && !keyframes)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  idx = editor_find_timeline_track_index(editor, track_id);
  if (idx < 0)
    return editor_finish_mutation(editor, before_json, false);
  track = &editor->timeline_tracks[idx].track;
  track->keyframe_count = keyframe_count;
  if (keyframe_count > 0u) {
    memcpy(track->keyframes, keyframes,
           (size_t)keyframe_count * sizeof(StygianEditorTimelineKeyframe));
    if (keyframe_count > 1u) {
      qsort(track->keyframes, keyframe_count,
            sizeof(StygianEditorTimelineKeyframe),
            editor_timeline_keyframe_compare);
    }
  }
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_timeline_add_clip(
    StygianEditor *editor, const StygianEditorTimelineClip *clip_template,
    StygianEditorTimelineClipId *out_clip_id) {
  char *before_json = NULL;
  StygianEditorTimelineClipSlot *slot;
  uint32_t i;
  if (!editor || !clip_template)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  if (editor->timeline_clip_count >= editor->max_timeline_clips)
    return editor_finish_mutation(editor, before_json, false);
  if (clip_template->track_count > STYGIAN_EDITOR_TIMELINE_CLIP_TRACK_CAP)
    return editor_finish_mutation(editor, before_json, false);
  for (i = 0u; i < clip_template->track_count; ++i) {
    if (editor_find_timeline_track_index(editor, clip_template->track_ids[i]) < 0)
      return editor_finish_mutation(editor, before_json, false);
  }
  slot = &editor->timeline_clips[editor->timeline_clip_count];
  memset(slot, 0, sizeof(*slot));
  slot->id = editor->next_timeline_clip_id++;
  slot->clip = *clip_template;
  slot->clip.id = slot->id;
  editor->timeline_clip_count += 1u;
  if (out_clip_id)
    *out_clip_id = slot->id;
  return editor_finish_mutation(editor, before_json, true);
}

bool stygian_editor_timeline_remove_clip(StygianEditor *editor,
                                         StygianEditorTimelineClipId clip_id) {
  char *before_json = NULL;
  uint32_t i;
  if (!editor || clip_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  for (i = 0u; i < editor->timeline_clip_count; ++i) {
    if (editor->timeline_clips[i].id == clip_id) {
      for (uint32_t j = i; (j + 1u) < editor->timeline_clip_count; ++j)
        editor->timeline_clips[j] = editor->timeline_clips[j + 1u];
      editor->timeline_clip_count -= 1u;
      return editor_finish_mutation(editor, before_json, true);
    }
  }
  return editor_finish_mutation(editor, before_json, false);
}

uint32_t stygian_editor_timeline_clip_count(const StygianEditor *editor) {
  if (!editor)
    return 0u;
  return editor->timeline_clip_count;
}

bool stygian_editor_timeline_get_clip(const StygianEditor *editor, uint32_t index,
                                      StygianEditorTimelineClip *out_clip) {
  if (!editor || !out_clip)
    return false;
  if (index >= editor->timeline_clip_count)
    return false;
  *out_clip = editor->timeline_clips[index].clip;
  return true;
}

bool stygian_editor_timeline_set_clip_tracks(
    StygianEditor *editor, StygianEditorTimelineClipId clip_id,
    const StygianEditorTimelineTrackId *track_ids, uint32_t track_count) {
  char *before_json = NULL;
  int idx;
  uint32_t i;
  if (!editor || clip_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  if (track_count > STYGIAN_EDITOR_TIMELINE_CLIP_TRACK_CAP)
    return false;
  if (track_count > 0u && !track_ids)
    return false;
  if (!editor_begin_mutation(editor, &before_json))
    return false;
  idx = editor_find_timeline_clip_index(editor, clip_id);
  if (idx < 0)
    return editor_finish_mutation(editor, before_json, false);
  for (i = 0u; i < track_count; ++i) {
    if (editor_find_timeline_track_index(editor, track_ids[i]) < 0)
      return editor_finish_mutation(editor, before_json, false);
  }
  editor->timeline_clips[idx].clip.track_count = track_count;
  if (track_count > 0u) {
    memcpy(editor->timeline_clips[idx].clip.track_ids, track_ids,
           (size_t)track_count * sizeof(StygianEditorTimelineTrackId));
  }
  return editor_finish_mutation(editor, before_json, true);
}

void stygian_editor_trigger_event(StygianEditor *editor,
                                  StygianEditorNodeId trigger_node,
                                  StygianEditorEventKind event_kind) {
  uint32_t i;
  uint64_t now;
  bool activated = false;

  if (!editor)
    return;

  now = editor_now_ms(editor);

  for (i = 0u; i < editor->behavior_count; ++i) {
    const StygianEditorBehaviorSlot *slot = &editor->behaviors[i];
    StygianEditorActiveAnimation *active = &editor->active_anims[i];
    const StygianEditorBehaviorRule *rule = &slot->rule;

    if (rule->trigger_node != trigger_node || rule->trigger_event != event_kind) {
      continue;
    }

    if (rule->action_kind == STYGIAN_EDITOR_ACTION_ANIMATE) {
      float from_value = rule->animate.from_value;
      const StygianEditorNode *target_node =
          editor_find_node_const(editor, rule->animate.target_node);
      if (!target_node)
        continue;

      if (editor_float_isnan(from_value)) {
        if (!editor_node_get_property(editor, target_node, rule->animate.property,
                                      &from_value)) {
          continue;
        }
      }

      active->active = true;
      active->target_node = rule->animate.target_node;
      active->property = rule->animate.property;
      active->from_value = from_value;
      active->to_value = rule->animate.to_value;
      active->duration_ms =
          (rule->animate.duration_ms == 0u) ? 1u : rule->animate.duration_ms;
      active->easing = rule->animate.easing;
      active->started_ms = now;
      activated = true;
    } else if (rule->action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY) {
      StygianEditorNode *target =
          editor_find_node(editor, rule->set_property.target_node);
      if (target && !target->locked &&
          editor_node_apply_property(editor, target, rule->set_property.property,
                                     rule->set_property.value)) {
        activated = true;
      }
    } else if (rule->action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY) {
      StygianEditorNode *target =
          editor_find_node(editor, rule->toggle_visibility.target_node);
      if (target && !target->locked) {
        target->visible = !target->visible;
        activated = true;
      }
    } else if (rule->action_kind == STYGIAN_EDITOR_ACTION_SET_VARIABLE) {
      uint32_t mode = rule->set_variable.use_active_mode
                          ? editor->active_variable_mode
                          : rule->set_variable.mode_index;
      if (mode >= editor->variable_mode_count)
        mode = editor->active_variable_mode;
      if (rule->set_variable.variable_kind == STYGIAN_EDITOR_VARIABLE_COLOR) {
        (void)stygian_editor_set_color_variable(
            editor, rule->set_variable.variable_name, mode,
            rule->set_variable.color_value);
      } else {
        (void)stygian_editor_set_number_variable(
            editor, rule->set_variable.variable_name, mode,
            rule->set_variable.number_value);
      }
      (void)stygian_editor_apply_active_variable_mode(editor);
      activated = true;
    } else if (rule->action_kind == STYGIAN_EDITOR_ACTION_NAVIGATE) {
      if (editor->host.navigate) {
        editor->host.navigate(editor->host.user_data, rule->navigate.target);
      } else {
        editor_logf(editor, STYGIAN_EDITOR_LOG_INFO, "Navigate: %s",
                    rule->navigate.target);
      }
      activated = true;
    }
  }

  if (activated)
    editor_request_repaint(editor, 120u);
}

void stygian_editor_tick(StygianEditor *editor, uint64_t now_ms) {
  uint32_t i;
  bool any_active = false;

  if (!editor)
    return;

  if (now_ms == 0u)
    now_ms = editor_now_ms(editor);

  for (i = 0u; i < editor->behavior_count; ++i) {
    StygianEditorActiveAnimation *active = &editor->active_anims[i];
    StygianEditorNode *target;
    float t;
    float eased;
    float value;

    if (!active->active)
      continue;

    target = editor_find_node(editor, active->target_node);
    if (!target) {
      active->active = false;
      continue;
    }

    if (now_ms <= active->started_ms) {
      t = 0.0f;
    } else {
      uint64_t elapsed = now_ms - active->started_ms;
      t = (float)((double)elapsed / (double)active->duration_ms);
    }

    t = editor_clampf(t, 0.0f, 1.0f);
    eased = editor_ease(active->easing, t);
    value =
        active->from_value + (active->to_value - active->from_value) * eased;

    if (editor_node_apply_property(editor, target, active->property, value)) {
      any_active = true;
    }

    if (t >= 1.0f)
      active->active = false;
  }

  if (any_active)
    editor_request_repaint(editor, 120u);
}

bool stygian_editor_begin_transaction(StygianEditor *editor) {
  if (!editor || editor->transaction_active || editor->history_suspended ||
      editor->history_replaying) {
    return false;
  }
  editor->transaction_before_json = editor_capture_project_json(editor);
  if (!editor->transaction_before_json)
    return false;
  editor->transaction_active = true;
  editor->transaction_dirty = false;
  return true;
}

bool stygian_editor_end_transaction(StygianEditor *editor, bool commit) {
  char *after_json = NULL;
  if (!editor || !editor->transaction_active || !editor->transaction_before_json)
    return false;

  if (!commit && editor->transaction_dirty) {
    bool ok;
    editor->history_suspended = true;
    ok = stygian_editor_load_project_json(editor, editor->transaction_before_json);
    editor->history_suspended = false;
    editor_history_drop_transaction(editor);
    return ok;
  }

  if (commit && editor->transaction_dirty) {
    after_json = editor_capture_project_json(editor);
    if (!after_json)
      return false;
    if (!editor_history_push(editor, editor->transaction_before_json, after_json)) {
      free(after_json);
      return false;
    }
    editor->transaction_before_json = NULL;
  }

  editor_history_drop_transaction(editor);
  return true;
}

bool stygian_editor_undo(StygianEditor *editor) {
  bool ok;
  StygianEditorHistoryEntry *entry;
  if (!editor || editor->transaction_active || editor->history_cursor == 0u)
    return false;
  entry = &editor->history_entries[editor->history_cursor - 1u];
  if (!entry->before_json)
    return false;
  editor->history_replaying = true;
  ok = stygian_editor_load_project_json(editor, entry->before_json);
  editor->history_replaying = false;
  if (!ok)
    return false;
  editor->history_cursor -= 1u;
  return true;
}

bool stygian_editor_redo(StygianEditor *editor) {
  bool ok;
  StygianEditorHistoryEntry *entry;
  if (!editor || editor->transaction_active ||
      editor->history_cursor >= editor->history_count)
    return false;
  entry = &editor->history_entries[editor->history_cursor];
  if (!entry->after_json)
    return false;
  editor->history_replaying = true;
  ok = stygian_editor_load_project_json(editor, entry->after_json);
  editor->history_replaying = false;
  if (!ok)
    return false;
  editor->history_cursor += 1u;
  return true;
}

static void editor_draw_grid_level(const StygianEditor *editor, StygianContext *ctx,
                                   float step_world, float alpha) {
  float min_wx;
  float min_wy;
  float max_wx;
  float max_wy;
  float start_x;
  float start_y;
  uint32_t i;
  const uint32_t max_lines = 2048u;

  if (!editor || !ctx || step_world <= 0.000001f)
    return;

  stygian_editor_view_to_world(editor, 0.0f, 0.0f, &min_wx, &min_wy);
  stygian_editor_view_to_world(editor, editor->viewport.width, editor->viewport.height,
                               &max_wx, &max_wy);

  if (max_wx < min_wx) {
    float t = max_wx;
    max_wx = min_wx;
    min_wx = t;
  }
  if (max_wy < min_wy) {
    float t = max_wy;
    max_wy = min_wy;
    min_wy = t;
  }

  start_x = floorf(min_wx / step_world) * step_world;
  for (i = 0u; i < max_lines; ++i) {
    float world_x = start_x + step_world * (float)i;
    float view_x;
    float line_y0;
    float line_y1;
    if (world_x > max_wx + step_world)
      break;
    stygian_editor_world_to_view(editor, world_x, min_wy, &view_x, &line_y0);
    stygian_editor_world_to_view(editor, world_x, max_wy, NULL, &line_y1);
    stygian_line(ctx, view_x, line_y0, view_x, line_y1, 1.0f, 0.27f, 0.30f,
                 0.34f, alpha);
  }

  start_y = floorf(min_wy / step_world) * step_world;
  for (i = 0u; i < max_lines; ++i) {
    float world_y = start_y + step_world * (float)i;
    float view_y;
    float line_x0;
    float line_x1;
    if (world_y > max_wy + step_world)
      break;
    stygian_editor_world_to_view(editor, min_wx, world_y, &line_x0, &view_y);
    stygian_editor_world_to_view(editor, max_wx, world_y, &line_x1, NULL);
    stygian_line(ctx, line_x0, view_y, line_x1, view_y, 1.0f, 0.27f, 0.30f,
                 0.34f, alpha);
  }
}

void stygian_editor_render_viewport2d(const StygianEditor *editor,
                                      StygianContext *ctx, bool draw_grid,
                                      bool draw_selection) {
  uint32_t i;
  float major_world = 0.0f;
  float minor_world = 0.0f;
  bool minor_visible = false;

  if (!editor || !ctx)
    return;

  if (draw_grid && editor->grid.enabled) {
    editor_grid_steps(editor, &major_world, &minor_world, &minor_visible);
    if (minor_visible)
      editor_draw_grid_level(editor, ctx, minor_world, 0.09f);
    editor_draw_grid_level(editor, ctx, major_world, 0.22f);
  }

  for (i = 0u; i < editor->node_count; ++i) {
    const StygianEditorNode *node = &editor->nodes[i];
    float ox = 0.0f;
    float oy = 0.0f;
    if (!node->visible)
      continue;
    editor_node_world_offset(editor, node, &ox, &oy);

    if (node->kind == STYGIAN_EDITOR_SHAPE_RECT) {
      StygianEditorColor fill = editor_node_primary_fill_color(node);
      float x;
      float y;
      float w = node->as.rect.w * editor_safe_zoom(editor);
      float h = node->as.rect.h * editor_safe_zoom(editor);
      StygianElement e = stygian_element_transient(ctx);
      stygian_editor_world_to_view(editor, node->as.rect.x + ox,
                                   node->as.rect.y + oy, &x, &y);
      stygian_set_type(ctx, e, STYGIAN_RECT);
      stygian_set_bounds(ctx, e, x, y, w, h);
      stygian_set_radius(
          ctx, e, node->as.rect.radius[0] * editor_safe_zoom(editor),
          node->as.rect.radius[1] * editor_safe_zoom(editor),
          node->as.rect.radius[2] * editor_safe_zoom(editor),
          node->as.rect.radius[3] * editor_safe_zoom(editor));
      stygian_set_color(ctx, e, fill.r, fill.g, fill.b, fill.a);
      stygian_set_z(ctx, e, editor_safe_clip_z(node->z));
      stygian_set_visible(ctx, e, true);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE) {
      StygianEditorColor fill = editor_node_primary_fill_color(node);
      float x;
      float y;
      float w = node->as.ellipse.w * editor_safe_zoom(editor);
      float h = node->as.ellipse.h * editor_safe_zoom(editor);
      StygianElement e = stygian_element_transient(ctx);
      stygian_editor_world_to_view(editor, node->as.ellipse.x + ox,
                                   node->as.ellipse.y + oy, &x, &y);
      stygian_set_type(ctx, e, STYGIAN_CIRCLE);
      stygian_set_bounds(ctx, e, x, y, w, h);
      stygian_set_color(ctx, e, fill.r, fill.g, fill.b, fill.a);
      stygian_set_z(ctx, e, editor_safe_clip_z(node->z));
      stygian_set_visible(ctx, e, true);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_FRAME) {
      StygianEditorColor fill = editor_node_primary_fill_color(node);
      float x;
      float y;
      float w = node->as.frame.w * editor_safe_zoom(editor);
      float h = node->as.frame.h * editor_safe_zoom(editor);
      StygianElement e = stygian_element_transient(ctx);
      stygian_editor_world_to_view(editor, node->as.frame.x + ox,
                                   node->as.frame.y + oy, &x, &y);
      stygian_set_type(ctx, e, STYGIAN_RECT);
      stygian_set_bounds(ctx, e, x, y, w, h);
      stygian_set_radius(ctx, e, 0.0f, 0.0f, 0.0f, 0.0f);
      stygian_set_color(ctx, e, fill.r, fill.g, fill.b, fill.a);
      stygian_set_z(ctx, e, editor_safe_clip_z(node->z));
      stygian_set_visible(ctx, e, true);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_PATH &&
               node->as.path.point_count >= 2u) {
      uint32_t j;
      for (j = 1u; j < node->as.path.point_count; ++j) {
        StygianEditorPoint a =
            editor->path_points[node->as.path.first_point + j - 1u];
        StygianEditorPoint b =
            editor->path_points[node->as.path.first_point + j];
        float x1;
        float y1;
        float x2;
        float y2;
        stygian_editor_world_to_view(editor, a.x + ox, a.y + oy, &x1, &y1);
        stygian_editor_world_to_view(editor, b.x + ox, b.y + oy, &x2, &y2);
        stygian_line(ctx, x1, y1, x2, y2,
                     node->as.path.thickness * editor_safe_zoom(editor),
                     node->as.path.stroke.r, node->as.path.stroke.g,
                     node->as.path.stroke.b, node->as.path.stroke.a);
      }
      if (node->as.path.closed && node->as.path.point_count > 2u) {
        StygianEditorPoint a =
            editor->path_points[node->as.path.first_point + node->as.path.point_count -
                                1u];
        StygianEditorPoint b = editor->path_points[node->as.path.first_point];
        float x1;
        float y1;
        float x2;
        float y2;
        stygian_editor_world_to_view(editor, a.x + ox, a.y + oy, &x1, &y1);
        stygian_editor_world_to_view(editor, b.x + ox, b.y + oy, &x2, &y2);
        stygian_line(ctx, x1, y1, x2, y2,
                     node->as.path.thickness * editor_safe_zoom(editor),
                     node->as.path.stroke.r, node->as.path.stroke.g,
                     node->as.path.stroke.b, node->as.path.stroke.a);
      }
    }
  }

  if (draw_selection) {
    for (i = 0u; i < editor->node_count; ++i) {
      const StygianEditorNode *selected = &editor->nodes[i];
      float x;
      float y;
      float w;
      float h;
      float vx;
      float vy;
      if (!selected->selected)
        continue;
      if (!editor_node_bounds_world(editor, selected, &x, &y, &w, &h))
        continue;
      stygian_editor_world_to_view(editor, x, y, &vx, &vy);
      w *= editor_safe_zoom(editor);
      h *= editor_safe_zoom(editor);
      stygian_line(ctx, vx, vy, vx + w, vy, 1.5f, 0.24f, 0.72f, 0.98f, 1.0f);
      stygian_line(ctx, vx + w, vy, vx + w, vy + h, 1.5f, 0.24f, 0.72f, 0.98f,
                   1.0f);
      stygian_line(ctx, vx + w, vy + h, vx, vy + h, 1.5f, 0.24f, 0.72f, 0.98f,
                   1.0f);
      stygian_line(ctx, vx, vy + h, vx, vy, 1.5f, 0.24f, 0.72f, 0.98f, 1.0f);
    }
  }
}

static void sb_finish(StygianEditorStringBuilder *sb) {
  size_t pos;
  if (!sb || !sb->dst || sb->cap == 0u)
    return;
  pos = sb->len;
  if (pos >= sb->cap)
    pos = sb->cap - 1u;
  sb->dst[pos] = '\0';
}

static void sb_append_raw(StygianEditorStringBuilder *sb, const char *text) {
  size_t text_len;
  size_t copy_len;
  if (!sb || !text)
    return;
  text_len = strlen(text);
  if (sb->dst && sb->cap > 0u && sb->len < sb->cap - 1u) {
    copy_len = sb->cap - 1u - sb->len;
    if (copy_len > text_len)
      copy_len = text_len;
    memcpy(sb->dst + sb->len, text, copy_len);
  }
  sb->len += text_len;
}

static void sb_appendf(StygianEditorStringBuilder *sb, const char *fmt, ...) {
  va_list args;
  va_list copy;
  int required;
  if (!sb || !fmt)
    return;

  va_start(args, fmt);
  va_copy(copy, args);
#if defined(_MSC_VER)
  required = _vscprintf(fmt, copy);
#else
  required = vsnprintf(NULL, 0, fmt, copy);
#endif
  va_end(copy);

  if (required < 0) {
    va_end(args);
    return;
  }

  if (sb->dst && sb->cap > 0u && sb->len < sb->cap - 1u) {
    size_t room = sb->cap - sb->len;
    vsnprintf(sb->dst + sb->len, room, fmt, args);
  }
  sb->len += (size_t)required;
  va_end(args);
}

static void sb_append_float(StygianEditorStringBuilder *sb, float value) {
  if (editor_float_isnan(value)) {
    sb_append_raw(sb, "NAN");
  } else {
    sb_appendf(sb, "%.6ff", value);
  }
}

static const char *editor_codegen_node_kind(StygianEditorShapeKind kind) {
  switch (kind) {
  case STYGIAN_EDITOR_SHAPE_RECT:
    return "RECT";
  case STYGIAN_EDITOR_SHAPE_ELLIPSE:
    return "ELLIPSE";
  case STYGIAN_EDITOR_SHAPE_PATH:
    return "PATH";
  case STYGIAN_EDITOR_SHAPE_FRAME:
    return "FRAME";
  case STYGIAN_EDITOR_SHAPE_TEXT:
    return "TEXT";
  case STYGIAN_EDITOR_SHAPE_IMAGE:
    return "IMAGE";
  case STYGIAN_EDITOR_SHAPE_LINE:
    return "LINE";
  case STYGIAN_EDITOR_SHAPE_ARROW:
    return "ARROW";
  case STYGIAN_EDITOR_SHAPE_POLYGON:
    return "POLYGON";
  case STYGIAN_EDITOR_SHAPE_STAR:
    return "STAR";
  case STYGIAN_EDITOR_SHAPE_ARC:
    return "ARC";
  case STYGIAN_EDITOR_SHAPE_GROUP:
    return "GROUP";
  case STYGIAN_EDITOR_SHAPE_COMPONENT_DEF:
    return "COMPONENT_DEF";
  case STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE:
    return "COMPONENT_INSTANCE";
  default:
    return "UNKNOWN";
  }
}

static void sb_append_c_string(StygianEditorStringBuilder *sb,
                               const char *text) {
  const unsigned char *at = (const unsigned char *)(text ? text : "");
  sb_append_raw(sb, "\"");
  while (*at) {
    unsigned char ch = *at++;
    if (ch == '\\')
      sb_append_raw(sb, "\\\\");
    else if (ch == '"')
      sb_append_raw(sb, "\\\"");
    else if (ch == '\n')
      sb_append_raw(sb, "\\n");
    else if (ch == '\r')
      sb_append_raw(sb, "\\r");
    else if (ch == '\t')
      sb_append_raw(sb, "\\t");
    else if (ch < 0x20u)
      sb_append_raw(sb, "?");
    else
      sb_appendf(sb, "%c", ch);
  }
  sb_append_raw(sb, "\"");
}

static void sb_append_c_string_range(StygianEditorStringBuilder *sb,
                                     const char *text, size_t start,
                                     size_t end) {
  size_t i;
  if (!sb)
    return;
  if (!text)
    text = "";
  sb_append_raw(sb, "\"");
  for (i = start; text[i] && i < end; ++i) {
    unsigned char ch = (unsigned char)text[i];
    if (ch == '\\')
      sb_append_raw(sb, "\\\\");
    else if (ch == '"')
      sb_append_raw(sb, "\\\"");
    else if (ch == '\n')
      sb_append_raw(sb, "\\n");
    else if (ch == '\r')
      sb_append_raw(sb, "\\r");
    else if (ch == '\t')
      sb_append_raw(sb, "\\t");
    else if (ch < 0x20u)
      sb_append_raw(sb, "?");
    else
      sb_appendf(sb, "%c", ch);
  }
  sb_append_raw(sb, "\"");
}

static void editor_diag_push(StygianEditorExportDiagnostic *out_diags,
                             uint32_t max_diags, uint32_t *io_count,
                             StygianEditorDiagnosticSeverity severity,
                             StygianEditorNodeId node_id, const char *feature,
                             const char *message) {
  uint32_t idx;
  if (!io_count)
    return;
  idx = *io_count;
  *io_count = idx + 1u;
  if (!out_diags || idx >= max_diags)
    return;
  memset(&out_diags[idx], 0, sizeof(out_diags[idx]));
  out_diags[idx].severity = severity;
  out_diags[idx].node_id = node_id;
  if (feature)
    snprintf(out_diags[idx].feature, sizeof(out_diags[idx].feature), "%s",
             feature);
  if (message)
    snprintf(out_diags[idx].message, sizeof(out_diags[idx].message), "%s",
             message);
}

static uint32_t editor_collect_export_diagnostics_internal(
    const StygianEditor *editor, StygianEditorExportDiagnostic *out_diags,
    uint32_t max_diags, bool *out_has_error) {
  uint32_t i;
  uint32_t count = 0u;
  bool has_error = false;

  if (!editor) {
    if (out_has_error)
      *out_has_error = true;
    return 0u;
  }

  for (i = 0u; i < editor->node_count; ++i) {
    const StygianEditorNode *node = &editor->nodes[i];
    if (node->kind == STYGIAN_EDITOR_SHAPE_PATH) {
      if (node->as.path.point_count < 2u) {
        editor_diag_push(
            out_diags, max_diags, &count, STYGIAN_EDITOR_DIAG_ERROR, node->id,
            "path_point_count",
            "Path export failed: path has fewer than 2 points.");
        has_error = true;
      } else if (node->as.path.closed) {
        editor_diag_push(
            out_diags, max_diags, &count, STYGIAN_EDITOR_DIAG_WARNING, node->id,
            "closed_path_fill",
            "Closed path fill is not exported yet; exporting stroke segments "
            "only.");
      }
    } else if (node->kind != STYGIAN_EDITOR_SHAPE_RECT &&
               node->kind != STYGIAN_EDITOR_SHAPE_ELLIPSE &&
               node->kind != STYGIAN_EDITOR_SHAPE_FRAME &&
               node->kind != STYGIAN_EDITOR_SHAPE_TEXT &&
               node->kind != STYGIAN_EDITOR_SHAPE_IMAGE &&
               node->kind != STYGIAN_EDITOR_SHAPE_LINE &&
               node->kind != STYGIAN_EDITOR_SHAPE_ARROW &&
               node->kind != STYGIAN_EDITOR_SHAPE_POLYGON &&
               node->kind != STYGIAN_EDITOR_SHAPE_STAR &&
               node->kind != STYGIAN_EDITOR_SHAPE_ARC) {
      editor_diag_push(
          out_diags, max_diags, &count, STYGIAN_EDITOR_DIAG_WARNING, node->id,
          "codegen_shape_fallback",
          "Shape kind has no dedicated C23 emitter yet; using generic fallback "
          "output.");
    }
    if (editor_node_supports_fill(node->kind)) {
      bool has_non_solid = false;
      if (node->fill_count > 1u) {
        editor_diag_push(
            out_diags, max_diags, &count, STYGIAN_EDITOR_DIAG_WARNING, node->id,
            "fill_stack_partial_export",
            "Only one solid fill is mapped directly today; extra fills are kept "
            "in authoring data.");
      }
      if (node->fill_count > 0u) {
        uint32_t j;
        for (j = 0u; j < node->fill_count; ++j) {
          const StygianEditorGradientTransform *xform =
              &node->fill_gradient_xform[j];
          if (!node->fills[j].visible)
            continue;
          if (node->fills[j].kind == STYGIAN_EDITOR_FILL_RADIAL_GRADIENT ||
              node->fills[j].kind == STYGIAN_EDITOR_FILL_IMAGE) {
            has_non_solid = true;
            break;
          }
          if (node->fills[j].kind == STYGIAN_EDITOR_FILL_LINEAR_GRADIENT &&
              !editor_gradient_xform_is_default(xform)) {
            editor_diag_push(
                out_diags, max_diags, &count, STYGIAN_EDITOR_DIAG_WARNING,
                node->id, "gradient_transform_partial_export",
                "Gradient origin/scale transform metadata is persisted. Runtime "
                "mapping currently applies rotation while origin/scale remain "
                "authoring-side metadata.");
          }
        }
      }
      if (has_non_solid) {
        editor_diag_push(out_diags, max_diags, &count,
                         STYGIAN_EDITOR_DIAG_WARNING, node->id,
                         "fill_stack_unsupported_kind",
                         "Only linear gradients map directly today. Radial, "
                         "angular, and image fills stay in authoring data.");
      }
    }
    if (node->stroke_count > 0u) {
      const StygianEditorNodeStroke *stroke = &node->strokes[0];
      if (stroke->dash_count > 0u || stroke->align != STYGIAN_EDITOR_STROKE_ALIGN_CENTER ||
          stroke->cap != STYGIAN_EDITOR_STROKE_CAP_BUTT ||
          stroke->join != STYGIAN_EDITOR_STROKE_JOIN_MITER) {
        editor_diag_push(out_diags, max_diags, &count,
                         STYGIAN_EDITOR_DIAG_WARNING, node->id,
                         "stroke_partial_export",
                         "Stroke width/color export is supported. Dash, cap/join, "
                         "and align are stored but not mapped yet.");
      }
    }
    if (node->kind == STYGIAN_EDITOR_SHAPE_TEXT &&
        node->as.text.span_count > 0u &&
        (node->as.text.align_h != STYGIAN_EDITOR_TEXT_ALIGN_LEFT ||
         node->as.text.align_v != STYGIAN_EDITOR_TEXT_ALIGN_TOP)) {
      editor_diag_push(out_diags, max_diags, &count,
                       STYGIAN_EDITOR_DIAG_WARNING, node->id,
                       "text_span_alignment_partial_export",
                       "Mixed text spans export as positioned runs. Non-default "
                       "area alignment stays approximate until rich text layout "
                       "is unified in runtime export.");
    }
    if (node->effect_count > 1u) {
      editor_diag_push(out_diags, max_diags, &count,
                       STYGIAN_EDITOR_DIAG_WARNING, node->id,
                       "effect_stack_partial_export",
                       "Only one effect per runtime channel maps directly. "
                       "Extra effects are retained in authoring data.");
    }
    if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
      if (node->mask_mode != STYGIAN_EDITOR_MASK_ALPHA || node->mask_invert) {
        editor_diag_push(
            out_diags, max_diags, &count, STYGIAN_EDITOR_DIAG_WARNING, node->id,
            "mask_mode_partial_export",
            "Mask chain export maps to clip-rect intersection. Invert and "
            "non-alpha modes remain authoring metadata.");
      }
    }
    if (node->shader_attachment.enabled) {
      if (!node->shader_attachment.asset_path[0]) {
        editor_diag_push(out_diags, max_diags, &count,
                         STYGIAN_EDITOR_DIAG_WARNING, node->id,
                         "shader_attachment_missing_asset",
                         "Shader attachment is enabled but has no asset path.");
      } else {
        editor_diag_push(
            out_diags, max_diags, &count, STYGIAN_EDITOR_DIAG_INFO, node->id,
            "shader_attachment_host_binding",
            "Shader attachment exports as authored metadata. Host/runtime "
            "binding still needs explicit integration.");
      }
    }
    if (node->boolean_valid) {
      if (node->boolean_solved_point_count < 3u) {
        editor_diag_push(out_diags, max_diags, &count,
                         STYGIAN_EDITOR_DIAG_WARNING, node->id,
                         "boolean_partial_export",
                         "Boolean result produced no polygon surface. "
                         "Flatten/export use resolved bounds fallback.");
      }
    }
    if (node->kind == STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE) {
      const StygianEditorNode *def = editor_component_def_for_instance(editor, node);
      uint32_t oi;
      if (!def && !node->component_detached) {
        editor_diag_push(out_diags, max_diags, &count, STYGIAN_EDITOR_DIAG_ERROR,
                         node->id, "component_instance_missing_def",
                         "Component instance has no valid definition link.");
        has_error = true;
      } else if (def) {
        if ((node->as.component_instance.overrides.mask &
             STYGIAN_EDITOR_COMPONENT_OVERRIDE_SWAP) != 0u) {
          if (!editor_component_swap_is_legal(
                  editor, def->id,
                  node->as.component_instance.overrides.swap_component_def_id)) {
            editor_diag_push(out_diags, max_diags, &count,
                             STYGIAN_EDITOR_DIAG_ERROR, node->id,
                             "component_swap_incompatible",
                             "Swap target is outside the component set or "
                             "violates component property contract.");
            has_error = true;
          }
        }
        for (oi = 0u;
             oi < node->as.component_instance.overrides.property_override_count &&
             oi < STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP;
             ++oi) {
          const StygianEditorComponentPropertyValue *ov =
              &node->as.component_instance.overrides.property_overrides[oi];
          if (!editor_component_property_value_valid_for_instance_context(
                  editor, node, def, ov)) {
            editor_diag_push(out_diags, max_diags, &count,
                             STYGIAN_EDITOR_DIAG_WARNING, node->id,
                             "component_override_conflict",
                             "Component override path or value does not match "
                             "resolved definition schema.");
          }
        }
      }
    }
  }

  for (i = 0u; i < editor->behavior_count; ++i) {
    const StygianEditorBehaviorRule *rule = &editor->behaviors[i].rule;
    bool bad = false;
    if (editor_find_node_index(editor, rule->trigger_node) < 0)
      bad = true;
    if (rule->action_kind == STYGIAN_EDITOR_ACTION_ANIMATE &&
        editor_find_node_index(editor, rule->animate.target_node) < 0) {
      bad = true;
    }
    if (rule->action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY &&
        editor_find_node_index(editor, rule->set_property.target_node) < 0) {
      bad = true;
    }
    if (rule->action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY &&
        editor_find_node_index(editor, rule->toggle_visibility.target_node) < 0) {
      bad = true;
    }
    if (bad) {
      editor_diag_push(out_diags, max_diags, &count, STYGIAN_EDITOR_DIAG_ERROR,
                       rule->trigger_node, "behavior_missing_node",
                       "Behavior export failed: trigger or target node is "
                       "missing.");
      has_error = true;
    }
  }

  if (out_has_error)
    *out_has_error = has_error;
  return count;
}

size_t stygian_editor_build_c23(const StygianEditor *editor, char *out_code,
                                size_t out_capacity) {
  StygianEditorStringBuilder sb;
  uint32_t i;
  uint32_t frame_count = 0u;
  uint32_t constraint_count = 0u;
  uint32_t size_limit_count = 0u;
  uint32_t auto_layout_frame_count = 0u;
  uint32_t auto_layout_child_count = 0u;
  uint32_t variable_binding_count = 0u;
  uint32_t driver_count = 0u;
  uint32_t component_def_count = 0u;
  uint32_t component_property_count = 0u;
  uint32_t component_instance_count = 0u;
  uint32_t component_override_count = 0u;

  if (!editor)
    return 0u;

  sb.dst = out_code;
  sb.cap = out_capacity;
  sb.len = 0u;

  sb_append_raw(&sb,
                "/* Auto-generated by Stygian Editor (attachable module). */\n");
  sb_append_raw(&sb,
                "/* Exporter-owned generated zone: regenerated on every export. */\n");
  sb_append_raw(
      &sb,
      "/* Hook zone lives in stygian_editor_hooks.h/.c and stays user-owned. */\n");
  sb_append_raw(
      &sb,
      "/* Authored project state lives in the editor project file, not in this generated C23. */\n");
  sb_append_raw(&sb,
                "/* Deterministic output: same model state => same C23 text. */\n");
  sb_append_raw(&sb, "#include \"stygian.h\"\n");
  sb_append_raw(&sb, "#include <stdint.h>\n");
  sb_append_raw(&sb, "#include <stdbool.h>\n");
  sb_append_raw(&sb, "#include <math.h>\n");
  sb_append_raw(&sb, "#include <string.h>\n\n");
  sb_append_raw(
      &sb,
      "/* Host override hook for exported image assets. */\n"
      "StygianTexture stygian_editor_generated_resolve_texture(const char *asset_name);\n\n");
  sb_append_raw(
      &sb,
      "static uint32_t stygian_editor_generated_utf8_char_len(unsigned char lead) {\n"
      "  if ((lead & 0x80u) == 0u) return 1u;\n"
      "  if ((lead & 0xE0u) == 0xC0u) return 2u;\n"
      "  if ((lead & 0xF0u) == 0xE0u) return 3u;\n"
      "  if ((lead & 0xF8u) == 0xF0u) return 4u;\n"
      "  return 1u;\n"
      "}\n\n"
      "float stygian_editor_generated_measure_text_simple(StygianContext *ctx, const char *text, float font_size, float letter_spacing) {\n"
      "  float w = 0.0f;\n"
      "  float line = 0.0f;\n"
      "  const unsigned char *p = (const unsigned char *)text;\n"
      "  char glyph[8];\n"
      "  if (!ctx || !text)\n"
      "    return 0.0f;\n"
      "  while (*p) {\n"
      "    uint32_t len;\n"
      "    uint32_t copied = 0u;\n"
      "    if (*p == '\\n') {\n"
      "      if (line > w) w = line;\n"
      "      line = 0.0f;\n"
      "      ++p;\n"
      "      continue;\n"
      "    }\n"
      "    len = stygian_editor_generated_utf8_char_len(*p);\n"
      "    if (len >= sizeof(glyph)) len = (uint32_t)sizeof(glyph) - 1u;\n"
      "    while (copied < len && p[copied]) {\n"
      "      glyph[copied] = (char)p[copied];\n"
      "      ++copied;\n"
      "    }\n"
      "    glyph[copied] = '\\0';\n"
      "    if (copied == 0u)\n"
      "      break;\n"
      "    line += stygian_text_width(ctx, 0u, glyph, font_size) + letter_spacing;\n"
      "    p += copied;\n"
      "  }\n"
      "  if (line > w) w = line;\n"
      "  return w;\n"
      "}\n\n"
      "static void stygian_editor_generated_apply_rotation(\n"
      "    StygianContext *ctx, StygianElement element, float x, float y, float w,\n"
      "    float h, float rotation_deg) {\n"
      "  float cx;\n"
      "  float cy;\n"
      "  float radians;\n"
      "  float c;\n"
      "  float s;\n"
      "  StygianTransform2D t;\n"
      "  if (!ctx || element == 0u || fabsf(rotation_deg) < 0.0001f)\n"
      "    return;\n"
      "  cx = x + w * 0.5f;\n"
      "  cy = y + h * 0.5f;\n"
      "  radians = rotation_deg * (3.14159265358979323846f / 180.0f);\n"
      "  c = cosf(radians);\n"
      "  s = sinf(radians);\n"
      "  t.m00 = c;\n"
      "  t.m01 = -s;\n"
      "  t.m02 = cx - cx * c + cy * s;\n"
      "  t.m10 = s;\n"
      "  t.m11 = c;\n"
      "  t.m12 = cy - cx * s - cy * c;\n"
      "  stygian_set_element_transform(ctx, element, t);\n"
      "}\n\n"
      "static bool stygian_editor_generated_push_rotation(\n"
      "    StygianContext *ctx, float x, float y, float w, float h,\n"
      "    float rotation_deg) {\n"
      "  float cx;\n"
      "  float cy;\n"
      "  if (!ctx || fabsf(rotation_deg) < 0.0001f)\n"
      "    return false;\n"
      "  cx = x + w * 0.5f;\n"
      "  cy = y + h * 0.5f;\n"
      "  stygian_transform_push(ctx);\n"
      "  stygian_transform_translate(ctx, cx, cy);\n"
      "  stygian_transform_rotate(ctx,\n"
      "                           rotation_deg * (3.14159265358979323846f /\n"
      "                                           180.0f));\n"
      "  stygian_transform_translate(ctx, -cx, -cy);\n"
      "  return true;\n"
      "}\n\n"
      "static StygianElement stygian_editor_generated_text_run(\n"
      "    StygianContext *ctx, const char *text, float x, float y, float font_size,\n"
      "    float letter_spacing, uint32_t weight, float r, float g, float b,\n"
      "    float a) {\n"
      "  const unsigned char *p = (const unsigned char *)text;\n"
      "  float cursor_x = x;\n"
      "  char glyph[8];\n"
      "  StygianElement first = 0u;\n"
      "  if (!ctx || !text)\n"
      "    return 0u;\n"
      "  while (*p) {\n"
      "    uint32_t len;\n"
      "    uint32_t copied = 0u;\n"
      "    StygianElement element;\n"
      "    float advance;\n"
      "    if (*p == '\\n')\n"
      "      break;\n"
      "    len = stygian_editor_generated_utf8_char_len(*p);\n"
      "    if (len >= sizeof(glyph)) len = (uint32_t)sizeof(glyph) - 1u;\n"
      "    while (copied < len && p[copied]) {\n"
      "      glyph[copied] = (char)p[copied];\n"
      "      ++copied;\n"
      "    }\n"
      "    glyph[copied] = '\\0';\n"
      "    if (copied == 0u)\n"
      "      break;\n"
      "    element = stygian_text(ctx, 0u, glyph, cursor_x, y, font_size, r, g, b, a);\n"
      "    if (first == 0u)\n"
      "      first = element;\n"
      "    if (weight >= 600u)\n"
      "      (void)stygian_text(ctx, 0u, glyph, cursor_x + 0.35f, y, font_size,\n"
      "                         r, g, b, a * 0.72f);\n"
      "    advance = stygian_text_width(ctx, 0u, glyph, font_size);\n"
      "    cursor_x += advance + letter_spacing;\n"
      "    p += copied;\n"
      "  }\n"
      "  return first;\n"
      "}\n\n"
      "static StygianElement stygian_editor_generated_draw_text_block(\n"
      "    StygianContext *ctx, const char *text, float x, float y, float w, float h,\n"
      "    float font_size, float line_height, float letter_spacing, uint32_t weight,\n"
      "    float r, float g, float b, float a, uint32_t box_mode, uint32_t align_h,\n"
      "    uint32_t align_v, float rotation_deg) {\n"
      "  char lines[64][256];\n"
      "  float widths[64];\n"
      "  uint32_t line_count = 0u;\n"
      "  float block_w = 0.0f;\n"
      "  float block_h;\n"
      "  float draw_y;\n"
      "  const char *cursor = text ? text : \"\";\n"
      "  StygianElement first = 0u;\n"
      "  uint8_t clip_id = 0u;\n"
      "  bool pushed = false;\n"
      "  uint32_t li;\n"
      "  if (!ctx)\n"
      "    return 0u;\n"
      "  if (font_size <= 0.0f)\n"
      "    font_size = 16.0f;\n"
      "  if (line_height <= 0.0f)\n"
      "    line_height = font_size * 1.2f;\n"
      "  if (line_height < font_size)\n"
      "    line_height = font_size * 1.1f;\n"
      "  while (line_count < 64u) {\n"
      "    size_t len = 0u;\n"
      "    while (cursor[len] && cursor[len] != '\\n' && len < sizeof(lines[0]) - 1u) {\n"
      "      lines[line_count][len] = cursor[len];\n"
      "      ++len;\n"
      "    }\n"
      "    lines[line_count][len] = '\\0';\n"
      "    widths[line_count] = stygian_editor_generated_measure_text_simple(\n"
      "        ctx, lines[line_count], font_size, letter_spacing);\n"
      "    if (widths[line_count] > block_w)\n"
      "      block_w = widths[line_count];\n"
      "    ++line_count;\n"
      "    cursor += len;\n"
      "    if (*cursor == '\\n') {\n"
      "      ++cursor;\n"
      "      continue;\n"
      "    }\n"
      "    break;\n"
      "  }\n"
      "  if (line_count == 0u)\n"
      "    line_count = 1u;\n"
      "  block_h = line_height * (float)line_count;\n"
      "  if (box_mode != 1u || w <= 0.0f)\n"
      "    w = block_w;\n"
      "  if (box_mode != 1u || h <= 0.0f)\n"
      "    h = block_h;\n"
      "  if (box_mode == 1u && w > 0.0f && h > 0.0f)\n"
      "    clip_id = stygian_clip_push(ctx, x, y, w, h);\n"
      "  pushed = stygian_editor_generated_push_rotation(ctx, x, y, w, h, rotation_deg);\n"
      "  draw_y = y;\n"
      "  if (box_mode == 1u) {\n"
      "    if (align_v == 1u)\n"
      "      draw_y = y + fmaxf(0.0f, (h - block_h) * 0.5f);\n"
      "    else if (align_v == 2u)\n"
      "      draw_y = y + fmaxf(0.0f, h - block_h);\n"
      "  }\n"
      "  for (li = 0u; li < line_count; ++li) {\n"
      "    float line_x = x;\n"
      "    StygianElement element;\n"
      "    if (box_mode == 1u) {\n"
      "      if (align_h == 1u)\n"
      "        line_x = x + fmaxf(0.0f, (w - widths[li]) * 0.5f);\n"
      "      else if (align_h == 2u)\n"
      "        line_x = x + fmaxf(0.0f, w - widths[li]);\n"
      "    }\n"
      "    element = stygian_editor_generated_text_run(\n"
      "        ctx, lines[li], line_x, draw_y + (float)li * line_height,\n"
      "        font_size, letter_spacing, weight, r, g, b, a);\n"
      "    if (first == 0u)\n"
      "      first = element;\n"
      "  }\n"
      "  if (pushed)\n"
      "    stygian_transform_pop(ctx);\n"
      "  if (clip_id != 0u)\n"
      "    stygian_clip_pop(ctx);\n"
      "  return first;\n"
      "}\n\n"
      "static StygianElement stygian_editor_generated_draw_image(\n"
      "    StygianContext *ctx, StygianTexture tex, float x, float y, float w, float h,\n"
      "    uint32_t fit_mode, float alpha, float rotation_deg) {\n"
      "  int tex_w = 0;\n"
      "  int tex_h = 0;\n"
      "  float draw_x = x;\n"
      "  float draw_y = y;\n"
      "  float draw_w = w;\n"
      "  float draw_h = h;\n"
      "  float u0 = 0.0f;\n"
      "  float v0 = 0.0f;\n"
      "  float u1 = 1.0f;\n"
      "  float v1 = 1.0f;\n"
      "  StygianElement element;\n"
      "  if (!ctx || tex == 0u)\n"
      "    return 0u;\n"
      "  if (stygian_texture_get_size(ctx, tex, &tex_w, &tex_h) && tex_w > 0 && tex_h > 0 && w > 0.0f && h > 0.0f) {\n"
      "    float src_aspect = (float)tex_w / (float)tex_h;\n"
      "    float dst_aspect = w / h;\n"
      "    if (fit_mode == 1u) {\n"
      "      if (src_aspect > dst_aspect) {\n"
      "        draw_h = w / src_aspect;\n"
      "        draw_y = y + (h - draw_h) * 0.5f;\n"
      "      } else {\n"
      "        draw_w = h * src_aspect;\n"
      "        draw_x = x + (w - draw_w) * 0.5f;\n"
      "      }\n"
      "    } else if (fit_mode == 2u) {\n"
      "      if (src_aspect > dst_aspect) {\n"
      "        float visible = dst_aspect / src_aspect;\n"
      "        u0 = (1.0f - visible) * 0.5f;\n"
      "        u1 = u0 + visible;\n"
      "      } else {\n"
      "        float visible = src_aspect / dst_aspect;\n"
      "        v0 = (1.0f - visible) * 0.5f;\n"
      "        v1 = v0 + visible;\n"
      "      }\n"
      "    }\n"
      "  }\n"
      "  element = stygian_element(ctx);\n"
      "  if (element == 0u)\n"
      "    return 0u;\n"
      "  stygian_set_type(ctx, element, STYGIAN_TEXTURE);\n"
      "  stygian_set_bounds(ctx, element, draw_x, draw_y, draw_w, draw_h);\n"
      "  stygian_set_color(ctx, element, 1.0f, 1.0f, 1.0f, alpha);\n"
      "  stygian_set_texture(ctx, element, tex, u0, v0, u1, v1);\n"
      "  stygian_editor_generated_apply_rotation(ctx, element, x, y, w, h, rotation_deg);\n"
      "  return element;\n"
      "}\n\n");

  sb_appendf(&sb, "#define STYGIAN_EDITOR_GENERATED_NODE_COUNT %uu\n",
             editor->node_count);
  sb_appendf(&sb, "#define STYGIAN_EDITOR_GENERATED_BEHAVIOR_COUNT %uu\n\n",
             editor->behavior_count);
  sb_appendf(&sb, "#define STYGIAN_EDITOR_GENERATED_COLOR_TOKEN_COUNT %uu\n",
             editor->color_token_count);
  sb_appendf(&sb, "#define STYGIAN_EDITOR_GENERATED_VARIABLE_COUNT %uu\n",
             editor->variable_count);
  sb_appendf(&sb, "#define STYGIAN_EDITOR_GENERATED_VARIABLE_MODE_COUNT %uu\n\n",
             editor->variable_mode_count);

  sb_append_raw(&sb, "typedef struct StygianEditorGeneratedColorToken {\n");
  sb_append_raw(&sb, "  const char *name;\n");
  sb_append_raw(&sb, "  float r;\n");
  sb_append_raw(&sb, "  float g;\n");
  sb_append_raw(&sb, "  float b;\n");
  sb_append_raw(&sb, "  float a;\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedColorToken;\n\n");

  if (editor->color_token_count > 0u) {
    sb_appendf(&sb,
               "static const StygianEditorGeneratedColorToken "
               "kStygianEditorGeneratedColorTokens[%uu] = {\n",
               editor->color_token_count);
    for (i = 0u; i < editor->color_token_count; ++i) {
      const StygianEditorColorToken *tok = &editor->color_tokens[i];
      sb_append_raw(&sb, "  { ");
      sb_append_c_string(&sb, tok->name);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, tok->color.r);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, tok->color.g);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, tok->color.b);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, tok->color.a);
      sb_append_raw(&sb, " },\n");
    }
    sb_append_raw(&sb, "};\n\n");
  } else {
    sb_append_raw(&sb,
                  "static const StygianEditorGeneratedColorToken "
                  "kStygianEditorGeneratedColorTokens[1] = {\n");
    sb_append_raw(&sb, "  { \"\", 0.0f, 0.0f, 0.0f, 0.0f },\n");
    sb_append_raw(&sb, "};\n\n");
  }

  sb_append_raw(
      &sb,
      "const StygianEditorGeneratedColorToken *\n"
      "stygian_editor_generated_color_tokens(uint32_t *out_count) {\n");
  sb_appendf(&sb, "  if (out_count) *out_count = %uu;\n", editor->color_token_count);
  sb_append_raw(&sb, "  return kStygianEditorGeneratedColorTokens;\n");
  sb_append_raw(&sb, "}\n\n");

  sb_append_raw(&sb, "typedef struct StygianEditorGeneratedVariable {\n");
  sb_append_raw(&sb, "  const char *name;\n");
  sb_append_raw(&sb, "  uint32_t kind;\n");
  sb_appendf(&sb, "  float number_values[%uu];\n", STYGIAN_EDITOR_VARIABLE_MODE_CAP);
  sb_appendf(&sb, "  float color_values[%uu][4];\n",
             STYGIAN_EDITOR_VARIABLE_MODE_CAP);
  sb_append_raw(&sb, "} StygianEditorGeneratedVariable;\n\n");

  if (editor->variable_count > 0u) {
    sb_appendf(&sb,
               "static const StygianEditorGeneratedVariable "
               "kStygianEditorGeneratedVariables[%uu] = {\n",
               editor->variable_count);
    for (i = 0u; i < editor->variable_count; ++i) {
      uint32_t m;
      const StygianEditorVariableDef *def = &editor->variables[i].def;
      sb_append_raw(&sb, "  { ");
      sb_append_c_string(&sb, def->name);
      sb_appendf(&sb, ", %uu, { ", (uint32_t)def->kind);
      for (m = 0u; m < STYGIAN_EDITOR_VARIABLE_MODE_CAP; ++m) {
        if (m > 0u)
          sb_append_raw(&sb, ", ");
        sb_append_float(&sb, def->number_values[m]);
      }
      sb_append_raw(&sb, " }, { ");
      for (m = 0u; m < STYGIAN_EDITOR_VARIABLE_MODE_CAP; ++m) {
        if (m > 0u)
          sb_append_raw(&sb, ", ");
        sb_append_raw(&sb, "{ ");
        sb_append_float(&sb, def->color_values[m].r);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, def->color_values[m].g);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, def->color_values[m].b);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, def->color_values[m].a);
        sb_append_raw(&sb, " }");
      }
      sb_append_raw(&sb, " } },\n");
    }
    sb_append_raw(&sb, "};\n\n");
  } else {
    sb_append_raw(&sb,
                  "static const StygianEditorGeneratedVariable "
                  "kStygianEditorGeneratedVariables[1] = {\n");
    sb_append_raw(
        &sb,
        "  { \"\", 0u, {0}, {{0.0f,0.0f,0.0f,0.0f}} },\n");
    sb_append_raw(&sb, "};\n\n");
  }

  sb_append_raw(
      &sb,
      "const StygianEditorGeneratedVariable *\n"
      "stygian_editor_generated_variables(uint32_t *out_count) {\n");
  sb_appendf(&sb, "  if (out_count) *out_count = %uu;\n", editor->variable_count);
  sb_append_raw(&sb, "  return kStygianEditorGeneratedVariables;\n");
  sb_append_raw(&sb, "}\n\n");

  sb_append_raw(&sb,
                "static const char *kStygianEditorGeneratedVariableModes[");
  sb_appendf(&sb, "%uu] = {\n", STYGIAN_EDITOR_VARIABLE_MODE_CAP);
  for (i = 0u; i < STYGIAN_EDITOR_VARIABLE_MODE_CAP; ++i) {
    sb_append_raw(&sb, "  ");
    if (i < editor->variable_mode_count)
      sb_append_c_string(&sb, editor->variable_modes[i]);
    else
      sb_append_raw(&sb, "\"\"");
    sb_append_raw(&sb, ",\n");
  }
  sb_append_raw(&sb, "};\n\n");

  for (i = 0u; i < editor->node_count; ++i) {
    if (editor->nodes[i].kind == STYGIAN_EDITOR_SHAPE_FRAME)
      frame_count += 1u;
    if (editor->nodes[i].kind == STYGIAN_EDITOR_SHAPE_FRAME &&
        editor->nodes[i].as.frame.layout_mode != STYGIAN_EDITOR_AUTO_LAYOUT_OFF) {
      auto_layout_frame_count += 1u;
    }
    if (editor->nodes[i].parent_id != STYGIAN_EDITOR_INVALID_ID &&
        editor->nodes[i].kind != STYGIAN_EDITOR_SHAPE_PATH) {
      constraint_count += 1u;
    }
    if (editor->nodes[i].size_min_w > 0.0f || editor->nodes[i].size_max_w > 0.0f ||
        editor->nodes[i].size_min_h > 0.0f ||
        editor->nodes[i].size_max_h > 0.0f) {
      size_limit_count += 1u;
    }
    if (editor->nodes[i].parent_id != STYGIAN_EDITOR_INVALID_ID &&
        editor->nodes[i].kind != STYGIAN_EDITOR_SHAPE_PATH) {
      const StygianEditorNode *parent =
          editor_find_node_const(editor, editor->nodes[i].parent_id);
      if (parent && parent->kind == STYGIAN_EDITOR_SHAPE_FRAME &&
          parent->as.frame.layout_mode != STYGIAN_EDITOR_AUTO_LAYOUT_OFF) {
        auto_layout_child_count += 1u;
      }
    }
    if (editor->nodes[i].color_variable[0] &&
        editor_find_variable(editor, editor->nodes[i].color_variable,
                             STYGIAN_EDITOR_VARIABLE_COLOR) >= 0) {
      variable_binding_count += 1u;
    }
    if (editor->nodes[i].number_variable[0] &&
        editor_property_allows_number_variable(
            editor->nodes[i].number_variable_property) &&
        editor_find_variable(editor, editor->nodes[i].number_variable,
                             STYGIAN_EDITOR_VARIABLE_NUMBER) >= 0) {
      variable_binding_count += 1u;
    }
    if (editor->nodes[i].kind == STYGIAN_EDITOR_SHAPE_COMPONENT_DEF) {
      component_def_count += 1u;
      component_property_count +=
          editor->nodes[i].as.component_def.property_count;
    }
    if (editor->nodes[i].kind == STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE) {
      component_instance_count += 1u;
      component_override_count +=
          editor->nodes[i].as.component_instance.overrides.property_override_count;
    }
  }
  for (i = 0u; i < editor->driver_count; ++i) {
    const StygianEditorDriverRule *rule = &editor->drivers[i].rule;
    if (!rule->variable_name[0])
      continue;
    if (editor_find_node_index(editor, rule->source_node) < 0)
      continue;
    if (!editor_property_allows_number_variable(rule->source_property))
      continue;
    if (editor_find_variable(editor, rule->variable_name,
                             STYGIAN_EDITOR_VARIABLE_NUMBER) < 0) {
      continue;
    }
    driver_count += 1u;
  }

  sb_append_raw(&sb, "typedef struct StygianEditorGeneratedComponentDefRecord {\n");
  sb_append_raw(&sb, "  uint32_t node_id;\n");
  sb_append_raw(&sb, "  const char *symbol;\n");
  sb_append_raw(&sb, "  const char *variant_group;\n");
  sb_append_raw(&sb, "  const char *variant_name;\n");
  sb_append_raw(&sb, "  uint32_t property_count;\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedComponentDefRecord;\n\n");
  sb_append_raw(
      &sb,
      "typedef struct StygianEditorGeneratedComponentPropertyRecord {\n");
  sb_append_raw(&sb, "  uint32_t component_def_id;\n");
  sb_append_raw(&sb, "  const char *name;\n");
  sb_append_raw(&sb, "  uint32_t type;\n");
  sb_append_raw(&sb, "  bool default_bool;\n");
  sb_append_raw(&sb, "  const char *default_enum;\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedComponentPropertyRecord;\n\n");
  sb_append_raw(
      &sb,
      "typedef struct StygianEditorGeneratedComponentInstanceRecord {\n");
  sb_append_raw(&sb, "  uint32_t node_id;\n");
  sb_append_raw(&sb, "  uint32_t resolved_component_def_id;\n");
  sb_append_raw(&sb, "  const char *symbol;\n");
  sb_append_raw(&sb, "  uint32_t property_override_count;\n");
  sb_append_raw(&sb, "  bool detached;\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedComponentInstanceRecord;\n\n");
  sb_append_raw(
      &sb,
      "typedef struct StygianEditorGeneratedComponentOverrideRecord {\n");
  sb_append_raw(&sb, "  uint32_t instance_node_id;\n");
  sb_append_raw(&sb, "  const char *name;\n");
  sb_append_raw(&sb, "  uint32_t type;\n");
  sb_append_raw(&sb, "  bool bool_value;\n");
  sb_append_raw(&sb, "  const char *enum_value;\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedComponentOverrideRecord;\n\n");

  if (component_def_count > 0u) {
    sb_appendf(
        &sb,
        "static const StygianEditorGeneratedComponentDefRecord "
        "kStygianEditorGeneratedComponentDefs[%uu] = {\n",
        component_def_count);
    for (i = 0u; i < editor->node_count; ++i) {
      const StygianEditorNode *node = &editor->nodes[i];
      if (node->kind != STYGIAN_EDITOR_SHAPE_COMPONENT_DEF)
        continue;
      sb_append_raw(&sb, "  { ");
      sb_appendf(&sb, "%uu, ", node->id);
      sb_append_c_string(&sb, node->as.component_def.symbol);
      sb_append_raw(&sb, ", ");
      sb_append_c_string(&sb, node->as.component_def.variant_group);
      sb_append_raw(&sb, ", ");
      sb_append_c_string(&sb, node->as.component_def.variant_name);
      sb_appendf(&sb, ", %uu },\n", node->as.component_def.property_count);
    }
    sb_append_raw(&sb, "};\n\n");
  }
  if (component_property_count > 0u) {
    sb_appendf(
        &sb,
        "static const StygianEditorGeneratedComponentPropertyRecord "
        "kStygianEditorGeneratedComponentProperties[%uu] = {\n",
        component_property_count);
    for (i = 0u; i < editor->node_count; ++i) {
      const StygianEditorNode *node = &editor->nodes[i];
      uint32_t pi;
      if (node->kind != STYGIAN_EDITOR_SHAPE_COMPONENT_DEF)
        continue;
      for (pi = 0u; pi < node->as.component_def.property_count &&
                     pi < STYGIAN_EDITOR_COMPONENT_PROPERTY_CAP;
           ++pi) {
        const StygianEditorComponentPropertyDef *prop =
            &node->as.component_def.properties[pi];
        sb_append_raw(&sb, "  { ");
        sb_appendf(&sb, "%uu, ", node->id);
        sb_append_c_string(&sb, prop->name);
        sb_appendf(&sb, ", %uu, %s, ", (uint32_t)prop->type,
                   prop->default_bool ? "true" : "false");
        sb_append_c_string(&sb, prop->default_enum);
        sb_append_raw(&sb, " },\n");
      }
    }
    sb_append_raw(&sb, "};\n\n");
  }
  if (component_instance_count > 0u) {
    sb_appendf(
        &sb,
        "static const StygianEditorGeneratedComponentInstanceRecord "
        "kStygianEditorGeneratedComponentInstances[%uu] = {\n",
        component_instance_count);
    for (i = 0u; i < editor->node_count; ++i) {
      const StygianEditorNode *node = &editor->nodes[i];
      const StygianEditorNode *resolved_def = NULL;
      if (node->kind != STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE)
        continue;
      resolved_def = editor_component_def_for_instance(editor, node);
      sb_append_raw(&sb, "  { ");
      sb_appendf(&sb, "%uu, ", node->id);
      sb_appendf(&sb, "%uu, ",
                 resolved_def ? resolved_def->id : STYGIAN_EDITOR_INVALID_ID);
      if (resolved_def)
        sb_append_c_string(&sb, resolved_def->as.component_def.symbol);
      else
        sb_append_c_string(&sb, node->as.component_instance.symbol_ref);
      sb_appendf(&sb, ", %uu, %s },\n",
                 node->as.component_instance.overrides.property_override_count,
                 node->component_detached ? "true" : "false");
    }
    sb_append_raw(&sb, "};\n\n");
  }
  if (component_override_count > 0u) {
    sb_appendf(
        &sb,
        "static const StygianEditorGeneratedComponentOverrideRecord "
        "kStygianEditorGeneratedComponentOverrides[%uu] = {\n",
        component_override_count);
    for (i = 0u; i < editor->node_count; ++i) {
      const StygianEditorNode *node = &editor->nodes[i];
      uint32_t oi;
      if (node->kind != STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE)
        continue;
      for (oi = 0u;
           oi < node->as.component_instance.overrides.property_override_count &&
           oi < STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP;
           ++oi) {
        const StygianEditorComponentPropertyValue *ov =
            &node->as.component_instance.overrides.property_overrides[oi];
        sb_append_raw(&sb, "  { ");
        sb_appendf(&sb, "%uu, ", node->id);
        sb_append_c_string(&sb, ov->name);
        sb_appendf(&sb, ", %uu, %s, ", (uint32_t)ov->type,
                   ov->bool_value ? "true" : "false");
        sb_append_c_string(&sb, ov->enum_value);
        sb_append_raw(&sb, " },\n");
      }
    }
    sb_append_raw(&sb, "};\n\n");
  }

  sb_append_raw(&sb, "typedef struct StygianEditorGeneratedVariableBinding {\n");
  sb_append_raw(&sb, "  uint32_t node_id;\n");
  sb_append_raw(&sb, "  uint32_t kind;\n");
  sb_append_raw(&sb, "  uint32_t property;\n");
  sb_append_raw(&sb, "  const char *name;\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedVariableBinding;\n\n");
  if (variable_binding_count > 0u) {
    sb_appendf(&sb,
               "static const StygianEditorGeneratedVariableBinding "
               "kStygianEditorGeneratedVariableBindings[%uu] = {\n",
               variable_binding_count);
    for (i = 0u; i < editor->node_count; ++i) {
      const StygianEditorNode *node = &editor->nodes[i];
      if (node->color_variable[0] &&
          editor_find_variable(editor, node->color_variable,
                               STYGIAN_EDITOR_VARIABLE_COLOR) >= 0) {
        sb_append_raw(&sb, "  { ");
        sb_appendf(&sb, "%uu, %uu, %uu, ", node->id,
                   (uint32_t)STYGIAN_EDITOR_VARIABLE_COLOR,
                   (uint32_t)STYGIAN_EDITOR_PROP_FILL_COLOR);
        sb_append_c_string(&sb, node->color_variable);
        sb_append_raw(&sb, " },\n");
      }
      if (node->number_variable[0] &&
          editor_property_allows_number_variable(node->number_variable_property) &&
          editor_find_variable(editor, node->number_variable,
                               STYGIAN_EDITOR_VARIABLE_NUMBER) >= 0) {
        sb_append_raw(&sb, "  { ");
        sb_appendf(&sb, "%uu, %uu, %uu, ", node->id,
                   (uint32_t)STYGIAN_EDITOR_VARIABLE_NUMBER,
                   (uint32_t)node->number_variable_property);
        sb_append_c_string(&sb, node->number_variable);
        sb_append_raw(&sb, " },\n");
      }
    }
    sb_append_raw(&sb, "};\n\n");
  } else {
    sb_append_raw(
        &sb,
        "static const StygianEditorGeneratedVariableBinding "
        "kStygianEditorGeneratedVariableBindings[1] = {\n");
    sb_append_raw(&sb, "  { 0u, 0u, 0u, \"\" },\n");
    sb_append_raw(&sb, "};\n\n");
  }
  sb_appendf(
      &sb,
      "static uint32_t gStygianEditorGeneratedActiveVariableMode = %uu;\n\n",
      editor->active_variable_mode);

  sb_append_raw(&sb, "typedef struct StygianEditorGeneratedDriverRule {\n");
  sb_append_raw(&sb, "  uint32_t id;\n");
  sb_append_raw(&sb, "  uint32_t source_node_id;\n");
  sb_append_raw(&sb, "  uint32_t source_property;\n");
  sb_append_raw(&sb, "  const char *variable_name;\n");
  sb_append_raw(&sb, "  bool use_active_mode;\n");
  sb_append_raw(&sb, "  uint32_t mode_index;\n");
  sb_append_raw(&sb, "  float in_min;\n");
  sb_append_raw(&sb, "  float in_max;\n");
  sb_append_raw(&sb, "  float out_min;\n");
  sb_append_raw(&sb, "  float out_max;\n");
  sb_append_raw(&sb, "  bool clamp_output;\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedDriverRule;\n\n");
  if (driver_count > 0u) {
    sb_appendf(&sb,
               "static const StygianEditorGeneratedDriverRule "
               "kStygianEditorGeneratedDrivers[%uu] = {\n",
               driver_count);
    for (i = 0u; i < editor->driver_count; ++i) {
      const StygianEditorDriverSlot *slot = &editor->drivers[i];
      const StygianEditorDriverRule *rule = &slot->rule;
      if (!rule->variable_name[0] ||
          editor_find_node_index(editor, rule->source_node) < 0 ||
          !editor_property_allows_number_variable(rule->source_property) ||
          editor_find_variable(editor, rule->variable_name,
                               STYGIAN_EDITOR_VARIABLE_NUMBER) < 0) {
        continue;
      }
      sb_append_raw(&sb, "  { ");
      sb_appendf(&sb, "%uu, %uu, %uu, ", slot->id, rule->source_node,
                 (uint32_t)rule->source_property);
      sb_append_c_string(&sb, rule->variable_name);
      sb_appendf(&sb, ", %s, %uu, ", rule->use_active_mode ? "true" : "false",
                 rule->mode_index);
      sb_append_float(&sb, rule->in_min);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, rule->in_max);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, rule->out_min);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, rule->out_max);
      sb_appendf(&sb, ", %s },\n", rule->clamp_output ? "true" : "false");
    }
    sb_append_raw(&sb, "};\n\n");
  } else {
    sb_append_raw(
        &sb,
        "static const StygianEditorGeneratedDriverRule "
        "kStygianEditorGeneratedDrivers[1] = {\n");
    sb_append_raw(
        &sb,
        "  { 0u, 0u, 0u, \"\", false, 0u, 0.0f, 1.0f, 0.0f, 1.0f, false },\n");
    sb_append_raw(&sb, "};\n\n");
  }

  sb_append_raw(&sb, "typedef enum StygianEditorGeneratedEventKind {\n");
  sb_append_raw(&sb, "  STYGIAN_EDITOR_GENERATED_EVENT_PRESS = 0,\n");
  sb_append_raw(&sb, "  STYGIAN_EDITOR_GENERATED_EVENT_RELEASE = 1,\n");
  sb_append_raw(&sb, "  STYGIAN_EDITOR_GENERATED_EVENT_HOVER_ENTER = 2,\n");
  sb_append_raw(&sb, "  STYGIAN_EDITOR_GENERATED_EVENT_HOVER_LEAVE = 3,\n");
  sb_append_raw(&sb, "  STYGIAN_EDITOR_GENERATED_EVENT_DRAG_START = 4,\n");
  sb_append_raw(&sb, "  STYGIAN_EDITOR_GENERATED_EVENT_DRAG_MOVE = 5,\n");
  sb_append_raw(&sb, "  STYGIAN_EDITOR_GENERATED_EVENT_DRAG_END = 6,\n");
  sb_append_raw(&sb, "  STYGIAN_EDITOR_GENERATED_EVENT_SCROLL = 7,\n");
  sb_append_raw(&sb, "  STYGIAN_EDITOR_GENERATED_EVENT_VALUE_CHANGED = 8,\n");
  sb_append_raw(&sb, "  STYGIAN_EDITOR_GENERATED_EVENT_FOCUS_ENTER = 9,\n");
  sb_append_raw(&sb, "  STYGIAN_EDITOR_GENERATED_EVENT_FOCUS_LEAVE = 10,\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedEventKind;\n\n");

  sb_append_raw(&sb, "typedef struct StygianEditorGeneratedAnimationRule {\n");
  sb_append_raw(&sb, "  uint32_t trigger_node_id;\n");
  sb_append_raw(&sb, "  uint32_t trigger_event;\n");
  sb_append_raw(&sb, "  uint32_t action_kind;\n");
  sb_append_raw(&sb, "  uint32_t target_node_id;\n");
  sb_append_raw(&sb, "  uint32_t target_property;\n");
  sb_append_raw(&sb, "  float from_value;\n");
  sb_append_raw(&sb, "  float to_value;\n");
  sb_append_raw(&sb, "  uint32_t duration_ms;\n");
  sb_append_raw(&sb, "  uint32_t easing;\n");
  sb_append_raw(&sb, "  const char *text0;\n");
  sb_append_raw(&sb, "  uint32_t u0;\n");
  sb_append_raw(&sb, "  uint32_t u1;\n");
  sb_append_raw(&sb, "  float f0;\n");
  sb_append_raw(&sb, "  float f1;\n");
  sb_append_raw(&sb, "  float f2;\n");
  sb_append_raw(&sb, "  float f3;\n");
  sb_append_raw(&sb, "  float f4;\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedAnimationRule;\n\n");
  sb_append_raw(&sb,
                "/* Host hooks for exported behavior actions that cross app boundaries. */\n"
                "void stygian_editor_generated_hook_set_variable(\n"
                "    const char *name, uint32_t kind, uint32_t mode_index,\n"
                "    float number_value, float r, float g, float b, float a);\n"
                "void stygian_editor_generated_hook_navigate(const char *target);\n\n");

  if (editor->behavior_count > 0u) {
    sb_appendf(
        &sb,
        "static const StygianEditorGeneratedAnimationRule "
        "kStygianEditorGeneratedRules[%uu] = {\n",
        editor->behavior_count);
    for (i = 0u; i < editor->behavior_count; ++i) {
      const StygianEditorBehaviorRule *rule = &editor->behaviors[i].rule;
      sb_append_raw(&sb, "  { ");
      if (rule->action_kind == STYGIAN_EDITOR_ACTION_ANIMATE) {
        sb_appendf(&sb, "%uu, %uu, %uu, %uu, %uu, ", rule->trigger_node,
                   (uint32_t)rule->trigger_event, (uint32_t)rule->action_kind,
                   rule->animate.target_node,
                   (uint32_t)rule->animate.property);
        sb_append_float(&sb, rule->animate.from_value);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, rule->animate.to_value);
        sb_appendf(&sb, ", %uu, %uu, \"\", 0u, 0u, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },\n",
                   rule->animate.duration_ms, (uint32_t)rule->animate.easing);
      } else if (rule->action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY) {
        sb_appendf(&sb, "%uu, %uu, %uu, %uu, %uu, 0.0f, %.6f, 0u, 0u, \"\", 0u, 0u, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },\n",
                   rule->trigger_node, (uint32_t)rule->trigger_event,
                   (uint32_t)rule->action_kind, rule->set_property.target_node,
                   (uint32_t)rule->set_property.property,
                   rule->set_property.value);
      } else if (rule->action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY) {
        sb_appendf(&sb, "%uu, %uu, %uu, %uu, 0u, 0.0f, 0.0f, 0u, 0u, \"\", 0u, 0u, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },\n",
                   rule->trigger_node, (uint32_t)rule->trigger_event,
                   (uint32_t)rule->action_kind,
                   rule->toggle_visibility.target_node);
      } else if (rule->action_kind == STYGIAN_EDITOR_ACTION_SET_VARIABLE) {
        sb_appendf(&sb, "%uu, %uu, %uu, 0u, 0u, 0.0f, 0.0f, 0u, 0u, ",
                   rule->trigger_node, (uint32_t)rule->trigger_event,
                   (uint32_t)rule->action_kind);
        sb_append_c_string(&sb, rule->set_variable.variable_name);
        sb_appendf(&sb, ", %uu, %uu, %.6f, %.6f, %.6f, %.6f, %.6f },\n",
                   (uint32_t)rule->set_variable.variable_kind,
                   rule->set_variable.use_active_mode
                       ? 0xFFFFFFFFu
                       : rule->set_variable.mode_index,
                   rule->set_variable.number_value,
                   rule->set_variable.color_value.r,
                   rule->set_variable.color_value.g,
                   rule->set_variable.color_value.b,
                   rule->set_variable.color_value.a);
      } else {
        sb_appendf(&sb, "%uu, %uu, %uu, 0u, 0u, 0.0f, 0.0f, 0u, 0u, ",
                   rule->trigger_node, (uint32_t)rule->trigger_event,
                   (uint32_t)rule->action_kind);
        sb_append_c_string(&sb, rule->navigate.target);
        sb_append_raw(&sb, ", 0u, 0u, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },\n");
      }
    }
    sb_append_raw(&sb, "};\n\n");
  } else {
    sb_append_raw(
        &sb,
        "static const StygianEditorGeneratedAnimationRule "
        "kStygianEditorGeneratedRules[1] = {\n");
    sb_append_raw(&sb,
                  "  { 0u, 0u, 0u, 0u, 0u, 0.0f, 0.0f, 0u, 0u, \"\", 0u, 0u, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },\n");
    sb_append_raw(&sb, "};\n\n");
  }

  sb_append_raw(&sb, "typedef struct StygianEditorGeneratedFrameRecord {\n");
  sb_append_raw(&sb, "  uint32_t node_id;\n");
  sb_append_raw(&sb, "  uint32_t parent_id;\n");
  sb_append_raw(&sb, "  bool clip_content;\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedFrameRecord;\n\n");

  if (frame_count > 0u) {
    sb_appendf(&sb,
               "static const StygianEditorGeneratedFrameRecord "
               "kStygianEditorGeneratedFrames[%uu] = {\n",
               frame_count);
    for (i = 0u; i < editor->node_count; ++i) {
      const StygianEditorNode *node = &editor->nodes[i];
      if (node->kind != STYGIAN_EDITOR_SHAPE_FRAME)
        continue;
      sb_appendf(&sb, "  { %uu, %uu, %s },\n", node->id, node->parent_id,
                 node->as.frame.clip_content ? "true" : "false");
    }
    sb_append_raw(&sb, "};\n\n");
  } else {
    sb_append_raw(
        &sb,
        "static const StygianEditorGeneratedFrameRecord "
        "kStygianEditorGeneratedFrames[1] = {\n");
    sb_append_raw(&sb, "  { 0u, 0u, false },\n");
    sb_append_raw(&sb, "};\n\n");
  }

  sb_append_raw(
      &sb,
      "const StygianEditorGeneratedFrameRecord *\n"
      "stygian_editor_generated_frame_records(uint32_t *out_count) {\n");
  sb_appendf(&sb, "  if (out_count) *out_count = %uu;\n", frame_count);
  sb_append_raw(&sb, "  return kStygianEditorGeneratedFrames;\n");
  sb_append_raw(&sb, "}\n\n");

  sb_append_raw(&sb, "typedef struct StygianEditorGeneratedSizeLimitRecord {\n");
  sb_append_raw(&sb, "  uint32_t child_id;\n");
  sb_append_raw(&sb, "  float min_w;\n");
  sb_append_raw(&sb, "  float max_w;\n");
  sb_append_raw(&sb, "  float min_h;\n");
  sb_append_raw(&sb, "  float max_h;\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedSizeLimitRecord;\n\n");

  if (size_limit_count > 0u) {
    sb_appendf(
        &sb,
        "static const StygianEditorGeneratedSizeLimitRecord "
        "kStygianEditorGeneratedSizeLimits[%uu] = {\n",
        size_limit_count);
    for (i = 0u; i < editor->node_count; ++i) {
      const StygianEditorNode *node = &editor->nodes[i];
      if (node->size_min_w <= 0.0f && node->size_max_w <= 0.0f &&
          node->size_min_h <= 0.0f && node->size_max_h <= 0.0f) {
        continue;
      }
      sb_appendf(&sb, "  { %uu, %.6f, %.6f, %.6f, %.6f },\n", node->id,
                 node->size_min_w, node->size_max_w, node->size_min_h,
                 node->size_max_h);
    }
    sb_append_raw(&sb, "};\n\n");
  } else {
    sb_append_raw(
        &sb,
        "static const StygianEditorGeneratedSizeLimitRecord "
        "kStygianEditorGeneratedSizeLimits[1] = {\n");
    sb_append_raw(&sb, "  { 0u, 0.0f, 0.0f, 0.0f, 0.0f },\n");
    sb_append_raw(&sb, "};\n\n");
  }

  sb_append_raw(
      &sb,
      "const StygianEditorGeneratedSizeLimitRecord *\n"
      "stygian_editor_generated_size_limit_records(uint32_t *out_count) {\n");
  sb_appendf(&sb, "  if (out_count) *out_count = %uu;\n", size_limit_count);
  sb_append_raw(&sb, "  return kStygianEditorGeneratedSizeLimits;\n");
  sb_append_raw(&sb, "}\n\n");

  sb_append_raw(&sb, "typedef struct StygianEditorGeneratedAutoLayoutFrame {\n");
  sb_append_raw(&sb, "  uint32_t node_id;\n");
  sb_append_raw(&sb, "  uint32_t mode;\n");
  sb_append_raw(&sb, "  uint32_t wrap;\n");
  sb_append_raw(&sb, "  float padding_left;\n");
  sb_append_raw(&sb, "  float padding_right;\n");
  sb_append_raw(&sb, "  float padding_top;\n");
  sb_append_raw(&sb, "  float padding_bottom;\n");
  sb_append_raw(&sb, "  float gap;\n");
  sb_append_raw(&sb, "  uint32_t primary_align;\n");
  sb_append_raw(&sb, "  uint32_t cross_align;\n");
  sb_append_raw(&sb, "  uint32_t overflow_policy;\n");
  sb_append_raw(&sb, "  float scroll_x;\n");
  sb_append_raw(&sb, "  float scroll_y;\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedAutoLayoutFrame;\n\n");

  sb_append_raw(&sb, "typedef struct StygianEditorGeneratedAutoLayoutChild {\n");
  sb_append_raw(&sb, "  uint32_t parent_id;\n");
  sb_append_raw(&sb, "  uint32_t child_id;\n");
  sb_append_raw(&sb, "  uint32_t order_index;\n");
  sb_append_raw(&sb, "  bool absolute;\n");
  sb_append_raw(&sb, "  uint32_t sizing_h;\n");
  sb_append_raw(&sb, "  uint32_t sizing_v;\n");
  sb_append_raw(&sb, "  float min_w;\n");
  sb_append_raw(&sb, "  float max_w;\n");
  sb_append_raw(&sb, "  float min_h;\n");
  sb_append_raw(&sb, "  float max_h;\n");
  sb_append_raw(&sb, "  float base_x;\n");
  sb_append_raw(&sb, "  float base_y;\n");
  sb_append_raw(&sb, "  float base_w;\n");
  sb_append_raw(&sb, "  float base_h;\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedAutoLayoutChild;\n\n");

  if (auto_layout_frame_count > 0u) {
    sb_appendf(
        &sb,
        "static const StygianEditorGeneratedAutoLayoutFrame "
        "kStygianEditorGeneratedAutoLayoutFrames[%uu] = {\n",
        auto_layout_frame_count);
    for (i = 0u; i < editor->node_count; ++i) {
      const StygianEditorNode *node = &editor->nodes[i];
      if (node->kind != STYGIAN_EDITOR_SHAPE_FRAME ||
          node->as.frame.layout_mode == STYGIAN_EDITOR_AUTO_LAYOUT_OFF) {
        continue;
      }
      sb_appendf(&sb,
                 "  { %uu, %uu, %uu, %.6f, %.6f, %.6f, %.6f, %.6f, %uu, %uu, %uu, %.6f, %.6f },\n",
                 node->id, (uint32_t)node->as.frame.layout_mode,
                 (uint32_t)node->as.frame.layout_wrap,
                 node->as.frame.layout_padding_left,
                 node->as.frame.layout_padding_right,
                 node->as.frame.layout_padding_top,
                 node->as.frame.layout_padding_bottom, node->as.frame.layout_gap,
                 (uint32_t)node->as.frame.layout_primary_align,
                 (uint32_t)node->as.frame.layout_cross_align,
                 (uint32_t)node->as.frame.overflow_policy,
                 node->as.frame.scroll_x, node->as.frame.scroll_y);
    }
    sb_append_raw(&sb, "};\n\n");
  } else {
    sb_append_raw(
        &sb,
        "static const StygianEditorGeneratedAutoLayoutFrame "
        "kStygianEditorGeneratedAutoLayoutFrames[1] = {\n");
    sb_append_raw(&sb, "  { 0u, 0u, 0u, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0u, 0u, 0u, 0.0f, 0.0f },\n");
    sb_append_raw(&sb, "};\n\n");
  }

  if (auto_layout_child_count > 0u) {
    uint32_t order = 0u;
    sb_appendf(
        &sb,
        "static const StygianEditorGeneratedAutoLayoutChild "
        "kStygianEditorGeneratedAutoLayoutChildren[%uu] = {\n",
        auto_layout_child_count);
    for (i = 0u; i < editor->node_count; ++i) {
      const StygianEditorNode *node = &editor->nodes[i];
      const StygianEditorNode *parent;
      float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
      if (node->parent_id == STYGIAN_EDITOR_INVALID_ID ||
          node->kind == STYGIAN_EDITOR_SHAPE_PATH) {
        continue;
      }
      parent = editor_find_node_const(editor, node->parent_id);
      if (!parent || parent->kind != STYGIAN_EDITOR_SHAPE_FRAME ||
          parent->as.frame.layout_mode == STYGIAN_EDITOR_AUTO_LAYOUT_OFF) {
        continue;
      }
      editor_node_get_local_xy(node, &x, &y);
      editor_node_get_local_wh(node, &w, &h);
      sb_appendf(&sb,
                 "  { %uu, %uu, %uu, %s, %uu, %uu, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f },\n",
                 node->parent_id, node->id, order++, node->layout_absolute ? "true" : "false",
                 (uint32_t)node->layout_sizing_h, (uint32_t)node->layout_sizing_v,
                 node->size_min_w, node->size_max_w, node->size_min_h,
                 node->size_max_h, x, y, w, h);
    }
    sb_append_raw(&sb, "};\n\n");
  } else {
    sb_append_raw(
        &sb,
        "static const StygianEditorGeneratedAutoLayoutChild "
        "kStygianEditorGeneratedAutoLayoutChildren[1] = {\n");
    sb_append_raw(&sb, "  { 0u, 0u, 0u, false, 0u, 0u, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },\n");
    sb_append_raw(&sb, "};\n\n");
  }

  sb_append_raw(
      &sb,
      "const StygianEditorGeneratedAutoLayoutFrame *\n"
      "stygian_editor_generated_auto_layout_frames(uint32_t *out_count) {\n");
  sb_appendf(&sb, "  if (out_count) *out_count = %uu;\n", auto_layout_frame_count);
  sb_append_raw(&sb, "  return kStygianEditorGeneratedAutoLayoutFrames;\n");
  sb_append_raw(&sb, "}\n\n");

  sb_append_raw(
      &sb,
      "const StygianEditorGeneratedAutoLayoutChild *\n"
      "stygian_editor_generated_auto_layout_children(uint32_t *out_count) {\n");
  sb_appendf(&sb, "  if (out_count) *out_count = %uu;\n", auto_layout_child_count);
  sb_append_raw(&sb, "  return kStygianEditorGeneratedAutoLayoutChildren;\n");
  sb_append_raw(&sb, "}\n\n");

  sb_append_raw(
      &sb,
      "static float stygian_editor_generated_clamp_size(float v, float min_v, float max_v) {\n"
      "  if (v < min_v) v = min_v;\n"
      "  if (max_v > 0.0f && v > max_v) v = max_v;\n"
      "  if (v < 0.0f) v = 0.0f;\n"
      "  return v;\n"
      "}\n\n"
      "uint32_t stygian_editor_generated_apply_auto_layout_for_parent(\n"
      "    uint32_t parent_id, float parent_w, float parent_h,\n"
      "    StygianEditorGeneratedResolvedConstraint *out_resolved,\n"
      "    uint32_t max_resolved) {\n"
      "  uint32_t i;\n"
      "  uint32_t resolved = 0u;\n"
      "  const StygianEditorGeneratedAutoLayoutFrame *frame = NULL;\n"
      "  if (!out_resolved || max_resolved == 0u)\n"
      "    return 0u;\n"
      "  for (i = 0u; i < ");
  sb_appendf(&sb, "%uu", auto_layout_frame_count);
  sb_append_raw(
      &sb,
      "; ++i) {\n"
      "    if (kStygianEditorGeneratedAutoLayoutFrames[i].node_id == parent_id) {\n"
      "      frame = &kStygianEditorGeneratedAutoLayoutFrames[i];\n"
      "      break;\n"
      "    }\n"
      "  }\n"
      "  if (!frame || frame->mode == 0u)\n"
      "    return 0u;\n"
      "  {\n"
      "    bool horizontal = frame->mode == 1u;\n"
      "    float inner_w = parent_w - frame->padding_left - frame->padding_right;\n"
      "    float inner_h = parent_h - frame->padding_top - frame->padding_bottom;\n"
      "    float gap = frame->gap < 0.0f ? 0.0f : frame->gap;\n"
      "    float fixed_primary = 0.0f;\n"
      "    uint32_t flow_count = 0u;\n"
      "    uint32_t fill_count = 0u;\n"
      "    float available_primary;\n"
      "    float fill_each = 0.0f;\n"
      "    float total_primary = 0.0f;\n"
      "    float cursor;\n"
      "    float scroll = 0.0f;\n"
      "    if (inner_w < 0.0f) inner_w = 0.0f;\n"
      "    if (inner_h < 0.0f) inner_h = 0.0f;\n"
      "    for (i = 0u; i < ");
  sb_appendf(&sb, "%uu", auto_layout_child_count);
  sb_append_raw(
      &sb,
      "; ++i) {\n"
      "      const StygianEditorGeneratedAutoLayoutChild *c = &kStygianEditorGeneratedAutoLayoutChildren[i];\n"
      "      float w;\n"
      "      float h;\n"
      "      if (c->parent_id != parent_id || c->absolute)\n"
      "        continue;\n"
      "      w = stygian_editor_generated_clamp_size(c->base_w, c->min_w, c->max_w);\n"
      "      h = stygian_editor_generated_clamp_size(c->base_h, c->min_h, c->max_h);\n"
      "      flow_count += 1u;\n"
      "      if (horizontal) {\n"
      "        if (c->sizing_h == 2u) fill_count += 1u; else fixed_primary += w;\n"
      "      } else {\n"
      "        if (c->sizing_v == 2u) fill_count += 1u; else fixed_primary += h;\n"
      "      }\n"
      "    }\n"
      "    if (flow_count > 1u) fixed_primary += gap * (float)(flow_count - 1u);\n"
      "    available_primary = (horizontal ? inner_w : inner_h) - fixed_primary;\n"
      "    if (available_primary < 0.0f) available_primary = 0.0f;\n"
      "    if (fill_count > 0u) fill_each = available_primary / (float)fill_count;\n"
      "    for (i = 0u; i < ");
  sb_appendf(&sb, "%uu", auto_layout_child_count);
  sb_append_raw(
      &sb,
      "; ++i) {\n"
      "      const StygianEditorGeneratedAutoLayoutChild *c = &kStygianEditorGeneratedAutoLayoutChildren[i];\n"
      "      float primary;\n"
      "      if (c->parent_id != parent_id || c->absolute)\n"
      "        continue;\n"
      "      primary = horizontal ? c->base_w : c->base_h;\n"
      "      if (horizontal && c->sizing_h == 2u) primary = fill_each;\n"
      "      if (!horizontal && c->sizing_v == 2u) primary = fill_each;\n"
      "      primary = stygian_editor_generated_clamp_size(primary,\n"
      "                  horizontal ? c->min_w : c->min_h,\n"
      "                  horizontal ? c->max_w : c->max_h);\n"
      "      total_primary += primary;\n"
      "    }\n"
      "    if (flow_count > 1u) total_primary += gap * (float)(flow_count - 1u);\n"
      "    cursor = horizontal ? frame->padding_left : frame->padding_top;\n"
      "    if (frame->primary_align == 1u)\n"
      "      cursor += ((horizontal ? inner_w : inner_h) - total_primary) * 0.5f;\n"
      "    else if (frame->primary_align == 2u)\n"
      "      cursor += (horizontal ? inner_w : inner_h) - total_primary;\n"
      "    if (horizontal && (frame->overflow_policy == 2u || frame->overflow_policy == 4u)) {\n"
      "      float max_scroll = total_primary - inner_w;\n"
      "      if (max_scroll < 0.0f) max_scroll = 0.0f;\n"
      "      scroll = frame->scroll_x;\n"
      "      if (scroll < 0.0f) scroll = 0.0f;\n"
      "      if (scroll > max_scroll) scroll = max_scroll;\n"
      "    } else if (!horizontal && (frame->overflow_policy == 3u || frame->overflow_policy == 4u)) {\n"
      "      float max_scroll = total_primary - inner_h;\n"
      "      if (max_scroll < 0.0f) max_scroll = 0.0f;\n"
      "      scroll = frame->scroll_y;\n"
      "      if (scroll < 0.0f) scroll = 0.0f;\n"
      "      if (scroll > max_scroll) scroll = max_scroll;\n"
      "    }\n"
      "    for (i = 0u; i < ");
  sb_appendf(&sb, "%uu", auto_layout_child_count);
  sb_append_raw(
      &sb,
      "; ++i) {\n"
      "      const StygianEditorGeneratedAutoLayoutChild *c = &kStygianEditorGeneratedAutoLayoutChildren[i];\n"
      "      float x = c->base_x;\n"
      "      float y = c->base_y;\n"
      "      float w = stygian_editor_generated_clamp_size(c->base_w, c->min_w, c->max_w);\n"
      "      float h = stygian_editor_generated_clamp_size(c->base_h, c->min_h, c->max_h);\n"
      "      if (c->parent_id != parent_id)\n"
      "        continue;\n"
      "      if (!c->absolute) {\n"
      "        float cross;\n"
      "        float cross_offset = 0.0f;\n"
      "        if (horizontal) {\n"
      "          if (c->sizing_h == 2u) w = fill_each;\n"
      "          if (c->sizing_v == 2u || frame->cross_align == 3u) h = inner_h;\n"
      "          w = stygian_editor_generated_clamp_size(w, c->min_w, c->max_w);\n"
      "          h = stygian_editor_generated_clamp_size(h, c->min_h, c->max_h);\n"
      "          cross = inner_h - h;\n"
      "          if (cross < 0.0f) cross = 0.0f;\n"
      "          if (frame->cross_align == 1u) cross_offset = cross * 0.5f;\n"
      "          else if (frame->cross_align == 2u) cross_offset = cross;\n"
      "          x = cursor - scroll;\n"
      "          y = frame->padding_top + cross_offset;\n"
      "          cursor += w + gap;\n"
      "        } else {\n"
      "          if (c->sizing_v == 2u) h = fill_each;\n"
      "          if (c->sizing_h == 2u || frame->cross_align == 3u) w = inner_w;\n"
      "          w = stygian_editor_generated_clamp_size(w, c->min_w, c->max_w);\n"
      "          h = stygian_editor_generated_clamp_size(h, c->min_h, c->max_h);\n"
      "          cross = inner_w - w;\n"
      "          if (cross < 0.0f) cross = 0.0f;\n"
      "          if (frame->cross_align == 1u) cross_offset = cross * 0.5f;\n"
      "          else if (frame->cross_align == 2u) cross_offset = cross;\n"
      "          x = frame->padding_left + cross_offset;\n"
      "          y = cursor - scroll;\n"
      "          cursor += h + gap;\n"
      "        }\n"
      "      }\n"
      "      if (resolved >= max_resolved)\n"
      "        continue;\n"
      "      out_resolved[resolved].child_id = c->child_id;\n"
      "      out_resolved[resolved].x = x;\n"
      "      out_resolved[resolved].y = y;\n"
      "      out_resolved[resolved].w = w;\n"
      "      out_resolved[resolved].h = h;\n"
      "      resolved += 1u;\n"
      "    }\n"
      "  }\n"
      "  return resolved;\n"
      "}\n\n");

  sb_append_raw(
      &sb, "typedef struct StygianEditorGeneratedConstraintRecord {\n");
  sb_append_raw(&sb, "  uint32_t parent_id;\n");
  sb_append_raw(&sb, "  uint32_t child_id;\n");
  sb_append_raw(&sb, "  uint32_t h_mode;\n");
  sb_append_raw(&sb, "  uint32_t v_mode;\n");
  sb_append_raw(&sb, "  float left;\n");
  sb_append_raw(&sb, "  float right;\n");
  sb_append_raw(&sb, "  float top;\n");
  sb_append_raw(&sb, "  float bottom;\n");
  sb_append_raw(&sb, "  float center_dx;\n");
  sb_append_raw(&sb, "  float center_dy;\n");
  sb_append_raw(&sb, "  float x_ratio;\n");
  sb_append_raw(&sb, "  float y_ratio;\n");
  sb_append_raw(&sb, "  float w_ratio;\n");
  sb_append_raw(&sb, "  float h_ratio;\n");
  sb_append_raw(&sb, "  float base_w;\n");
  sb_append_raw(&sb, "  float base_h;\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedConstraintRecord;\n\n");

  if (constraint_count > 0u) {
    sb_appendf(
        &sb,
        "static const StygianEditorGeneratedConstraintRecord "
        "kStygianEditorGeneratedConstraints[%uu] = {\n",
        constraint_count);
    for (i = 0u; i < editor->node_count; ++i) {
      const StygianEditorNode *node = &editor->nodes[i];
      if (node->parent_id == STYGIAN_EDITOR_INVALID_ID ||
          node->kind == STYGIAN_EDITOR_SHAPE_PATH) {
        continue;
      }
      {
        float bw = 0.0f;
        float bh = 0.0f;
        editor_node_get_local_wh(node, &bw, &bh);
      sb_appendf(&sb,
                 "  { %uu, %uu, %uu, %uu, %.6f, %.6f, %.6f, %.6f, "
                 "%.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f },\n",
                 node->parent_id, node->id, (uint32_t)node->constraint_h,
                 (uint32_t)node->constraint_v, node->constraint_left,
                 node->constraint_right, node->constraint_top,
                 node->constraint_bottom, node->constraint_center_dx,
                 node->constraint_center_dy, node->constraint_x_ratio,
                 node->constraint_y_ratio, node->constraint_w_ratio,
                 node->constraint_h_ratio, bw, bh);
      }
    }
    sb_append_raw(&sb, "};\n\n");
  } else {
    sb_append_raw(
        &sb,
        "static const StygianEditorGeneratedConstraintRecord "
        "kStygianEditorGeneratedConstraints[1] = {\n");
    sb_append_raw(
        &sb,
        "  { 0u, 0u, 0u, 0u, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, "
        "0.0, 0.0, 0.0, 0.0 },\n");
    sb_append_raw(&sb, "};\n\n");
  }

  sb_append_raw(
      &sb,
      "const StygianEditorGeneratedConstraintRecord *\n"
      "stygian_editor_generated_constraint_records(uint32_t *out_count) {\n");
  sb_appendf(&sb, "  if (out_count) *out_count = %uu;\n", constraint_count);
  sb_append_raw(&sb, "  return kStygianEditorGeneratedConstraints;\n");
  sb_append_raw(&sb, "}\n\n");

  sb_append_raw(
      &sb,
      "typedef struct StygianEditorGeneratedResolvedConstraint {\n"
      "  uint32_t child_id;\n"
      "  float x;\n"
      "  float y;\n"
      "  float w;\n"
      "  float h;\n"
      "} StygianEditorGeneratedResolvedConstraint;\n\n"
      "uint32_t stygian_editor_generated_apply_constraints_for_parent(\n"
      "    uint32_t parent_id, float parent_w, float parent_h,\n"
      "    StygianEditorGeneratedResolvedConstraint *out_resolved,\n"
      "    uint32_t max_resolved) {\n"
      "  uint32_t i;\n"
      "  uint32_t resolved = 0u;\n"
      "  if (!out_resolved || max_resolved == 0u) return 0u;\n"
      "  for (i = 0u; i < ");
  sb_appendf(&sb, "%uu", constraint_count);
  sb_append_raw(
      &sb,
      "; ++i) {\n"
      "    const StygianEditorGeneratedConstraintRecord *c = "
      "&kStygianEditorGeneratedConstraints[i];\n"
      "    float x;\n"
      "    float y;\n"
      "    float w;\n"
      "    float h;\n"
      "    if (c->parent_id != parent_id)\n"
      "      continue;\n"
      "    if (resolved >= max_resolved)\n"
      "      continue;\n"
      "    w = c->base_w;\n"
      "    h = c->base_h;\n"
      "    switch (c->h_mode) {\n"
      "    case 1u: x = parent_w - c->right - c->base_w; break;\n"
      "    case 2u: x = c->left; w = parent_w - c->left - c->right; break;\n"
      "    case 3u: x = (parent_w - c->base_w) * 0.5f + c->center_dx; break;\n"
      "    case 4u: x = parent_w * c->x_ratio; w = parent_w * c->w_ratio; break;\n"
      "    default: x = c->left; break;\n"
      "    }\n"
      "    if (w < 0.0f) w = 0.0f;\n"
      "    switch (c->v_mode) {\n"
      "    case 1u: y = parent_h - c->bottom - c->base_h; break;\n"
      "    case 2u: y = c->top; h = parent_h - c->top - c->bottom; break;\n"
      "    case 3u: y = (parent_h - c->base_h) * 0.5f + c->center_dy; break;\n"
      "    case 4u: y = parent_h * c->y_ratio; h = parent_h * c->h_ratio; break;\n"
      "    default: y = c->top; break;\n"
      "    }\n"
      "    if (h < 0.0f) h = 0.0f;\n"
      "    {\n"
      "      uint32_t j;\n"
      "      for (j = 0u; j < ");
  sb_appendf(&sb, "%uu", size_limit_count);
  sb_append_raw(
      &sb,
      "; ++j) {\n"
      "        const StygianEditorGeneratedSizeLimitRecord *s = "
      "&kStygianEditorGeneratedSizeLimits[j];\n"
      "        if (s->child_id != c->child_id)\n"
      "          continue;\n"
      "        if (w < s->min_w) w = s->min_w;\n"
      "        if (s->max_w > 0.0f && w > s->max_w) w = s->max_w;\n"
      "        if (h < s->min_h) h = s->min_h;\n"
      "        if (s->max_h > 0.0f && h > s->max_h) h = s->max_h;\n"
      "        break;\n"
      "      }\n"
      "    }\n"
      "    out_resolved[resolved].child_id = c->child_id;\n"
      "    out_resolved[resolved].x = x;\n"
      "    out_resolved[resolved].y = y;\n"
      "    out_resolved[resolved].w = w;\n"
      "    out_resolved[resolved].h = h;\n"
      "    resolved += 1u;\n"
      "  }\n"
      "  return resolved;\n"
      "}\n\n");

  sb_append_raw(
      &sb,
      "const StygianEditorGeneratedAnimationRule *\n"
      "stygian_editor_generated_behavior_rules(uint32_t *out_count) {\n");
  sb_appendf(&sb,
             "  if (out_count) *out_count = STYGIAN_EDITOR_GENERATED_BEHAVIOR_"
             "COUNT;\n");
  sb_append_raw(&sb, "  return kStygianEditorGeneratedRules;\n");
  sb_append_raw(&sb, "}\n\n");
  sb_append_raw(
      &sb,
      "void stygian_editor_generated_dispatch_event(\n"
      "    StygianContext *ctx, StygianElement *elements, uint32_t element_count,\n"
      "    uint32_t trigger_node_id, uint32_t trigger_event);\n"
      "int stygian_editor_generated_node_index(uint32_t node_id);\n"
      "void stygian_editor_generated_apply_variable_assignment(\n"
      "    StygianContext *ctx, StygianElement *elements, uint32_t element_count,\n"
      "    const char *name, uint32_t kind, uint32_t mode_index,\n"
      "    float number_value, float r, float g, float b, float a);\n"
      "void stygian_editor_generated_apply_variable_mode(\n"
      "    StygianContext *ctx, StygianElement *elements, uint32_t element_count,\n"
      "    uint32_t mode_index);\n"
      "bool stygian_editor_generated_apply_driver_sample(\n"
      "    StygianContext *ctx, StygianElement *elements, uint32_t element_count,\n"
      "    uint32_t driver_id, float source_value);\n"
      "void stygian_editor_generated_set_active_variable_mode(uint32_t mode_index);\n"
      "uint32_t stygian_editor_generated_get_active_variable_mode(void);\n\n"
      "void stygian_editor_generated_dispatch_event(\n"
      "    StygianContext *ctx, StygianElement *elements, uint32_t element_count,\n"
      "    uint32_t trigger_node_id, uint32_t trigger_event) {\n"
      "  uint32_t i;\n"
      "  (void)ctx;\n"
      "  if (!elements) return;\n"
      "  for (i = 0u; i < STYGIAN_EDITOR_GENERATED_BEHAVIOR_COUNT; ++i) {\n"
      "    const StygianEditorGeneratedAnimationRule *r = &kStygianEditorGeneratedRules[i];\n"
      "    int idx;\n"
      "    if (r->trigger_node_id != trigger_node_id || r->trigger_event != trigger_event)\n"
      "      continue;\n"
      "    idx = stygian_editor_generated_node_index(r->target_node_id);\n"
      "    if (r->action_kind == 0u) {\n"
      "      /* Animate action remains table-driven; host can consume behavior rules. */\n"
      "      continue;\n"
      "    } else if (r->action_kind == 1u) {\n"
      "      if (idx < 0 || (uint32_t)idx >= element_count) continue;\n"
      "      switch (r->target_property) {\n"
      "      case 0u: stygian_set_x(ctx, elements[idx], r->to_value); break;\n"
      "      case 1u: stygian_set_y(ctx, elements[idx], r->to_value); break;\n"
      "      case 2u: stygian_set_w(ctx, elements[idx], r->to_value); break;\n"
      "      case 3u: stygian_set_h(ctx, elements[idx], r->to_value); break;\n"
      "      case 4u: stygian_set_a(ctx, elements[idx], r->to_value); break;\n"
      "      default: break;\n"
      "      }\n"
      "    } else if (r->action_kind == 2u) {\n"
      "      if (idx < 0 || (uint32_t)idx >= element_count) continue;\n"
      "      stygian_set_visible(ctx, elements[idx], false);\n"
      "    } else if (r->action_kind == 3u) {\n"
      "      uint32_t _mode = (r->u1 == 0xFFFFFFFFu)\n"
      "                           ? gStygianEditorGeneratedActiveVariableMode\n"
      "                           : r->u1;\n"
      "      stygian_editor_generated_apply_variable_assignment(\n"
      "          ctx, elements, element_count, r->text0, r->u0, _mode,\n"
      "          r->f0, r->f1, r->f2, r->f3, r->f4);\n"
      "      stygian_editor_generated_hook_set_variable(\n"
      "          r->text0, r->u0, _mode, r->f0, r->f1, r->f2, r->f3, r->f4);\n"
      "    } else if (r->action_kind == 4u) {\n"
      "      stygian_editor_generated_hook_navigate(r->text0);\n"
      "    }\n"
      "  }\n"
      "}\n\n");

  sb_append_raw(
      &sb,
      "int stygian_editor_generated_node_index(uint32_t node_id) {\n");
  sb_append_raw(&sb, "  switch (node_id) {\n");
  for (i = 0u; i < editor->node_count; ++i) {
    sb_appendf(&sb, "  case %uu: return %d;\n", editor->nodes[i].id, (int)i);
  }
  sb_append_raw(&sb, "  default: return -1;\n");
  sb_append_raw(&sb, "  }\n");
  sb_append_raw(&sb, "}\n\n");

  sb_appendf(
      &sb,
      "static int stygian_editor_generated_find_variable_index(const char *name, "
      "uint32_t kind) {\n"
      "  uint32_t i;\n"
      "  if (!name || !name[0])\n"
      "    return -1;\n"
      "  for (i = 0u; i < %uu; ++i) {\n"
      "    const StygianEditorGeneratedVariable *v = &kStygianEditorGeneratedVariables[i];\n"
      "    if (v->kind != kind)\n"
      "      continue;\n"
      "    if (strcmp(v->name, name) == 0)\n"
      "      return (int)i;\n"
      "  }\n"
      "  return -1;\n"
      "}\n\n",
      editor->variable_count);
  sb_appendf(
      &sb,
      "void stygian_editor_generated_apply_variable_assignment(\n"
      "    StygianContext *ctx, StygianElement *elements, uint32_t element_count,\n"
      "    const char *name, uint32_t kind, uint32_t mode_index,\n"
      "    float number_value, float r, float g, float b, float a) {\n"
      "  uint32_t i;\n"
      "  if (!ctx || !elements)\n"
      "    return;\n"
      "  for (i = 0u; i < %uu; ++i) {\n"
      "    const StygianEditorGeneratedVariableBinding *bnd = &kStygianEditorGeneratedVariableBindings[i];\n"
      "    int idx;\n"
      "    if (bnd->kind != kind)\n"
      "      continue;\n"
      "    if (strcmp(bnd->name, name) != 0)\n"
      "      continue;\n"
      "    idx = stygian_editor_generated_node_index(bnd->node_id);\n"
      "    if (idx < 0 || (uint32_t)idx >= element_count)\n"
      "      continue;\n"
      "    if (kind == 0u) {\n"
      "      (void)mode_index;\n"
      "      stygian_set_color(ctx, elements[idx], r, g, b, a);\n"
      "    } else {\n"
      "      switch (bnd->property) {\n"
      "      case 0u: stygian_set_x(ctx, elements[idx], number_value); break;\n"
      "      case 1u: stygian_set_y(ctx, elements[idx], number_value); break;\n"
      "      case 2u: stygian_set_w(ctx, elements[idx], number_value); break;\n"
      "      case 3u: stygian_set_h(ctx, elements[idx], number_value); break;\n"
      "      case 4u: stygian_set_a(ctx, elements[idx], number_value); break;\n"
      "      case 9u: stygian_set_value(ctx, elements[idx], number_value); break;\n"
      "      default: break;\n"
      "      }\n"
      "    }\n"
      "  }\n"
      "}\n\n",
      variable_binding_count);
  sb_appendf(
      &sb,
      "void stygian_editor_generated_apply_variable_mode(\n"
      "    StygianContext *ctx, StygianElement *elements, uint32_t element_count,\n"
      "    uint32_t mode_index) {\n"
      "  uint32_t i;\n"
      "  if (mode_index >= %uu)\n"
      "    return;\n"
      "  gStygianEditorGeneratedActiveVariableMode = mode_index;\n"
      "  for (i = 0u; i < %uu; ++i) {\n"
      "    const StygianEditorGeneratedVariableBinding *bnd = &kStygianEditorGeneratedVariableBindings[i];\n"
      "    int vidx = stygian_editor_generated_find_variable_index(bnd->name, bnd->kind);\n"
      "    if (vidx < 0)\n"
      "      continue;\n"
      "    if (bnd->kind == 0u) {\n"
      "      stygian_editor_generated_apply_variable_assignment(\n"
      "          ctx, elements, element_count, bnd->name, bnd->kind, mode_index,\n"
      "          0.0f,\n"
      "          kStygianEditorGeneratedVariables[vidx].color_values[mode_index][0],\n"
      "          kStygianEditorGeneratedVariables[vidx].color_values[mode_index][1],\n"
      "          kStygianEditorGeneratedVariables[vidx].color_values[mode_index][2],\n"
      "          kStygianEditorGeneratedVariables[vidx].color_values[mode_index][3]);\n"
      "    } else {\n"
      "      stygian_editor_generated_apply_variable_assignment(\n"
      "          ctx, elements, element_count, bnd->name, bnd->kind, mode_index,\n"
      "          kStygianEditorGeneratedVariables[vidx].number_values[mode_index],\n"
      "          0.0f, 0.0f, 0.0f, 0.0f);\n"
      "    }\n"
      "  }\n"
      "}\n\n"
      "void stygian_editor_generated_set_active_variable_mode(uint32_t mode_index) {\n"
      "  if (mode_index < %uu)\n"
      "    gStygianEditorGeneratedActiveVariableMode = mode_index;\n"
      "}\n\n"
      "uint32_t stygian_editor_generated_get_active_variable_mode(void) {\n"
      "  return gStygianEditorGeneratedActiveVariableMode;\n"
      "}\n\n",
      editor->variable_mode_count, variable_binding_count,
      editor->variable_mode_count);
  sb_appendf(
      &sb,
      "bool stygian_editor_generated_apply_driver_sample(\n"
      "    StygianContext *ctx, StygianElement *elements, uint32_t element_count,\n"
      "    uint32_t driver_id, float source_value) {\n"
      "  uint32_t i;\n"
      "  for (i = 0u; i < %uu; ++i) {\n"
      "    const StygianEditorGeneratedDriverRule *drv = &kStygianEditorGeneratedDrivers[i];\n"
      "    uint32_t mode_index;\n"
      "    float t;\n"
      "    float mapped;\n"
      "    if (drv->id != driver_id)\n"
      "      continue;\n"
      "    mode_index = drv->use_active_mode ? gStygianEditorGeneratedActiveVariableMode : drv->mode_index;\n"
      "    if (fabsf(drv->in_max - drv->in_min) < 0.0001f) {\n"
      "      mapped = drv->out_min;\n"
      "    } else {\n"
      "      t = (source_value - drv->in_min) / (drv->in_max - drv->in_min);\n"
      "      if (drv->clamp_output) {\n"
      "        if (t < 0.0f) t = 0.0f;\n"
      "        if (t > 1.0f) t = 1.0f;\n"
      "      }\n"
      "      mapped = drv->out_min + (drv->out_max - drv->out_min) * t;\n"
      "    }\n"
      "    stygian_editor_generated_apply_variable_assignment(\n"
      "        ctx, elements, element_count, drv->variable_name, 1u, mode_index,\n"
      "        mapped, 0.0f, 0.0f, 0.0f, 0.0f);\n"
      "    stygian_editor_generated_hook_set_variable(\n"
      "        drv->variable_name, 1u, mode_index, mapped, 0.0f, 0.0f, 0.0f, 0.0f);\n"
      "    return true;\n"
      "  }\n"
      "  return false;\n"
      "}\n\n",
      driver_count);

  sb_append_raw(&sb, "typedef struct StygianEditorGeneratedMaskRecord {\n");
  sb_append_raw(&sb, "  int16_t mask_index;\n");
  sb_append_raw(&sb, "  uint8_t mode;\n");
  sb_append_raw(&sb, "  bool invert;\n");
  sb_append_raw(&sb, "  float mask_x;\n");
  sb_append_raw(&sb, "  float mask_y;\n");
  sb_append_raw(&sb, "  float mask_w;\n");
  sb_append_raw(&sb, "  float mask_h;\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedMaskRecord;\n\n");

  if (editor->node_count > 0u) {
    sb_appendf(
        &sb,
        "static const StygianEditorGeneratedMaskRecord "
        "kStygianEditorGeneratedMaskRecords[%uu] = {\n",
        editor->node_count);
    for (i = 0u; i < editor->node_count; ++i) {
      const StygianEditorNode *node = &editor->nodes[i];
      int mask_index = editor_find_node_index(editor, node->mask_node_id);
      float mx = 0.0f;
      float my = 0.0f;
      float mw = 0.0f;
      float mh = 0.0f;
      if (mask_index >= 0) {
        const StygianEditorNode *mask_node = &editor->nodes[(uint32_t)mask_index];
        (void)editor_node_bounds_world(editor, mask_node, &mx, &my, &mw, &mh);
      }
      sb_append_raw(&sb, "  { ");
      sb_appendf(&sb, "%d, %uu, %s, ", mask_index, (uint32_t)node->mask_mode,
                 node->mask_invert ? "true" : "false");
      sb_append_float(&sb, mx);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, my);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, mw);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, mh);
      sb_append_raw(&sb, " },\n");
    }
    sb_append_raw(&sb, "};\n\n");
  } else {
    sb_append_raw(
        &sb,
        "static const StygianEditorGeneratedMaskRecord "
        "kStygianEditorGeneratedMaskRecords[1] = {\n");
    sb_append_raw(&sb, "  { -1, 0u, false, 0.0f, 0.0f, 0.0f, 0.0f },\n");
    sb_append_raw(&sb, "};\n\n");
  }

  sb_append_raw(
      &sb,
      "static bool stygian_editor_generated_mask_intersect(\n"
      "    float ax, float ay, float aw, float ah,\n"
      "    float bx, float by, float bw, float bh,\n"
      "    float *ox, float *oy, float *ow, float *oh) {\n"
      "  float x0 = ax > bx ? ax : bx;\n"
      "  float y0 = ay > by ? ay : by;\n"
      "  float x1 = (ax + aw) < (bx + bw) ? (ax + aw) : (bx + bw);\n"
      "  float y1 = (ay + ah) < (by + bh) ? (ay + ah) : (by + bh);\n"
      "  if (x1 <= x0 || y1 <= y0)\n"
      "    return false;\n"
      "  if (ox) *ox = x0;\n"
      "  if (oy) *oy = y0;\n"
      "  if (ow) *ow = x1 - x0;\n"
      "  if (oh) *oh = y1 - y0;\n"
      "  return true;\n"
      "}\n\n"
      "static bool stygian_editor_generated_mask_rect_for_owner(\n"
      "    uint32_t owner_index, uint32_t depth,\n"
      "    float *out_x, float *out_y, float *out_w, float *out_h) {\n"
      "  const StygianEditorGeneratedMaskRecord *rel;\n"
      "  int16_t mask_index;\n"
      "  float x;\n"
      "  float y;\n"
      "  float w;\n"
      "  float h;\n"
      "  if (owner_index >= STYGIAN_EDITOR_GENERATED_NODE_COUNT)\n"
      "    return false;\n"
      "  if (depth > STYGIAN_EDITOR_GENERATED_NODE_COUNT)\n"
      "    return false;\n"
      "  rel = &kStygianEditorGeneratedMaskRecords[owner_index];\n"
      "  mask_index = rel->mask_index;\n"
      "  if (mask_index < 0)\n"
      "    return false;\n"
      "  x = rel->mask_x;\n"
      "  y = rel->mask_y;\n"
      "  w = rel->mask_w;\n"
      "  h = rel->mask_h;\n"
      "  if (w <= 0.0f || h <= 0.0f)\n"
      "    return false;\n"
      "  if (kStygianEditorGeneratedMaskRecords[(uint32_t)mask_index].mask_index >= 0) {\n"
      "    float px = 0.0f;\n"
      "    float py = 0.0f;\n"
      "    float pw = 0.0f;\n"
      "    float ph = 0.0f;\n"
      "    if (stygian_editor_generated_mask_rect_for_owner((uint32_t)mask_index,\n"
      "            depth + 1u, &px, &py, &pw, &ph)) {\n"
      "      if (!stygian_editor_generated_mask_intersect(\n"
      "              x, y, w, h, px, py, pw, ph, &x, &y, &w, &h)) {\n"
      "        return false;\n"
      "      }\n"
      "    }\n"
      "  }\n"
      "  if (out_x) *out_x = x;\n"
      "  if (out_y) *out_y = y;\n"
      "  if (out_w) *out_w = w;\n"
      "  if (out_h) *out_h = h;\n"
      "  return true;\n"
      "}\n\n"
      "static uint8_t stygian_editor_generated_clip_id_for_owner(\n"
      "    StygianContext *ctx, uint32_t owner_index) {\n"
      "  float x = 0.0f;\n"
      "  float y = 0.0f;\n"
      "  float w = 0.0f;\n"
      "  float h = 0.0f;\n"
      "  uint8_t id;\n"
      "  if (!ctx)\n"
      "    return 0u;\n"
      "  if (!stygian_editor_generated_mask_rect_for_owner(\n"
      "          owner_index, 0u, &x, &y, &w, &h)) {\n"
      "    return 0u;\n"
      "  }\n"
      "  id = stygian_clip_push(ctx, x, y, w, h);\n"
      "  if (id != 0u)\n"
      "    stygian_clip_pop(ctx);\n"
      "  return id;\n"
      "}\n\n");

  sb_append_raw(
      &sb,
      "void stygian_editor_generated_build_scene(StygianContext *ctx,\n"
      "                                        StygianElement *out_elements,\n"
      "                                        uint32_t out_count) {\n");
  sb_appendf(&sb,
             "  if (!ctx || !out_elements || out_count < "
             "STYGIAN_EDITOR_GENERATED_NODE_COUNT)\n");
  sb_append_raw(&sb, "    return;\n\n");

  for (i = 0u; i < editor->node_count; ++i) {
    const StygianEditorNode *node = &editor->nodes[i];
    StygianEditorColor node_fill = editor_node_primary_fill_color(node);
    const StygianEditorNodeFill *primary_fill =
        editor_node_primary_visible_fill(node);
    int primary_fill_index = editor_node_primary_visible_fill_index(node);
    uint32_t eff_i;
    sb_appendf(&sb, "  /* Node %uu (%s) */\n", node->id,
               editor_codegen_node_kind(node->kind));
    if (node->kind == STYGIAN_EDITOR_SHAPE_COMPONENT_DEF) {
      sb_append_raw(&sb, "  /* component-def symbol=");
      sb_append_c_string(&sb, node->as.component_def.symbol);
      sb_append_raw(&sb, " variant=");
      sb_append_c_string(&sb, node->as.component_def.variant_group);
      sb_append_raw(&sb, "/");
      sb_append_c_string(&sb, node->as.component_def.variant_name);
      sb_append_raw(&sb, " properties=");
      sb_appendf(&sb, "%uu */\n", node->as.component_def.property_count);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE) {
      const StygianEditorNode *resolved_def = editor_component_def_for_instance(editor, node);
      sb_append_raw(&sb, "  /* component-instance resolved-symbol=");
      if (resolved_def)
        sb_append_c_string(&sb, resolved_def->as.component_def.symbol);
      else
        sb_append_c_string(&sb, node->as.component_instance.symbol_ref);
      sb_append_raw(&sb, " overrides=");
      sb_appendf(&sb, "%uu detached=%s */\n",
                 node->as.component_instance.overrides.property_override_count,
                 node->component_detached ? "true" : "false");
    }

    if (node->kind == STYGIAN_EDITOR_SHAPE_PATH ||
        node->kind == STYGIAN_EDITOR_SHAPE_LINE ||
        node->kind == STYGIAN_EDITOR_SHAPE_ARROW ||
        node->kind == STYGIAN_EDITOR_SHAPE_POLYGON ||
        node->kind == STYGIAN_EDITOR_SHAPE_STAR ||
        node->kind == STYGIAN_EDITOR_SHAPE_ARC) {
      sb_appendf(&sb,
                 "  out_elements[%uu] = 0u; /* vector primitive emits via "
                 "stygian_editor_generated_draw_paths */\n\n",
                 i);
      continue;
    }

    sb_appendf(&sb, "  out_elements[%uu] = stygian_element(ctx);\n", i);
    sb_appendf(&sb, "  stygian_set_visible(ctx, out_elements[%uu], %s);\n", i,
               node->visible ? "true" : "false");
    sb_appendf(&sb, "  stygian_set_z(ctx, out_elements[%uu], ", i);
    sb_append_float(&sb, node->z);
    sb_append_raw(&sb, ");\n");

    if (node->kind == STYGIAN_EDITOR_SHAPE_RECT) {
      sb_appendf(&sb, "  stygian_set_type(ctx, out_elements[%uu], STYGIAN_RECT);\n",
                 i);
      sb_appendf(&sb, "  stygian_set_bounds(ctx, out_elements[%uu], ", i);
      sb_append_float(&sb, node->as.rect.x);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.rect.y);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.rect.w);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.rect.h);
      sb_append_raw(&sb, ");\n");

      sb_appendf(&sb, "  stygian_set_radius(ctx, out_elements[%uu], ", i);
      sb_append_float(&sb, node->as.rect.radius[0]);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.rect.radius[1]);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.rect.radius[2]);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.rect.radius[3]);
      sb_append_raw(&sb, ");\n");

      sb_appendf(&sb, "  stygian_set_color(ctx, out_elements[%uu], ", i);
      sb_append_float(&sb, node_fill.r);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node_fill.g);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node_fill.b);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node_fill.a);
      sb_append_raw(&sb, ");\n");
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE) {
      sb_appendf(&sb,
                 "  stygian_set_type(ctx, out_elements[%uu], STYGIAN_CIRCLE);\n",
                 i);
      sb_appendf(&sb, "  stygian_set_bounds(ctx, out_elements[%uu], ", i);
      sb_append_float(&sb, node->as.ellipse.x);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.ellipse.y);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.ellipse.w);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.ellipse.h);
      sb_append_raw(&sb, ");\n");

      sb_appendf(&sb, "  stygian_set_color(ctx, out_elements[%uu], ", i);
      sb_append_float(&sb, node_fill.r);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node_fill.g);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node_fill.b);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node_fill.a);
      sb_append_raw(&sb, ");\n");
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_FRAME) {
      sb_appendf(&sb, "  stygian_set_type(ctx, out_elements[%uu], STYGIAN_RECT);\n",
                 i);
      sb_appendf(&sb, "  stygian_set_bounds(ctx, out_elements[%uu], ", i);
      sb_append_float(&sb, node->as.frame.x);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.frame.y);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.frame.w);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.frame.h);
      sb_append_raw(&sb, ");\n");
      sb_appendf(&sb,
                 "  stygian_set_radius(ctx, out_elements[%uu], 0.0f, 0.0f, 0.0f, "
                 "0.0f);\n",
                 i);
      sb_appendf(&sb, "  stygian_set_color(ctx, out_elements[%uu], ", i);
      sb_append_float(&sb, node_fill.r);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node_fill.g);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node_fill.b);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node_fill.a);
      sb_append_raw(&sb, ");\n");
      sb_appendf(&sb, "  /* frame_clip_content=%s */\n",
                 node->as.frame.clip_content ? "true" : "false");
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_TEXT) {
      const char *text = node->as.text.text;
      float text_w = 0.0f;
      float text_h = 0.0f;
      const size_t text_len = strlen(text);
      size_t run_start = 0u;
      float cursor_x = node->as.text.x;
      float cursor_y = node->as.text.y;
      float run_x = cursor_x;
      float run_y = cursor_y;
      float run_fs = node->as.text.font_size > 0.0f ? node->as.text.font_size : 16.0f;
      float run_lh = editor_text_span_line_height(run_fs, node->as.text.line_height);
      float run_ls = node->as.text.letter_spacing;
      uint32_t run_weight = node->as.text.font_weight;
      StygianEditorColor run_color = node_fill;
      float line_max_lh = 0.0f;
      bool have_run = false;
      bool have_element = false;
      bool use_clip = node->as.text.box_mode == STYGIAN_EDITOR_TEXT_BOX_AREA &&
                      node->as.text.w > 0.0f && node->as.text.h > 0.0f;
      editor_measure_text_block(&node->as.text, &text_w, &text_h);
      if (use_clip) {
        text_w = node->as.text.w;
        text_h = node->as.text.h;
      }
      if (node->as.text.span_count == 0u) {
        sb_appendf(&sb,
                   "  out_elements[%uu] = stygian_editor_generated_draw_text_block(ctx, ",
                   i);
        sb_append_c_string(&sb, text);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.text.x);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.text.y);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.text.w);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.text.h);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.text.font_size);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.text.line_height);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.text.letter_spacing);
        sb_append_raw(&sb, ", ");
        sb_appendf(&sb, "%uu", node->as.text.font_weight);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node_fill.r);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node_fill.g);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node_fill.b);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node_fill.a);
        sb_append_raw(&sb, ", ");
        sb_appendf(&sb, "%uu, %uu, %uu, ", (uint32_t)node->as.text.box_mode,
                   (uint32_t)node->as.text.align_h,
                   (uint32_t)node->as.text.align_v);
        sb_append_float(&sb, node->rotation_deg);
        sb_append_raw(&sb, ");\n");
      } else {
        if (use_clip || fabsf(node->rotation_deg) >= 0.0001f)
          sb_append_raw(&sb, "  {\n");
        if (use_clip) {
          sb_append_raw(&sb, "    uint8_t _text_clip = stygian_clip_push(ctx, ");
          sb_append_float(&sb, node->as.text.x);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.text.y);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.text.w);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.text.h);
          sb_append_raw(&sb, ");\n");
        }
        if (fabsf(node->rotation_deg) >= 0.0001f) {
          sb_append_raw(&sb, "    bool _text_rot = stygian_editor_generated_push_rotation(ctx, ");
          sb_append_float(&sb, node->as.text.x);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.text.y);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, text_w);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, text_h);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->rotation_deg);
          sb_append_raw(&sb, ");\n");
        }
        sb_appendf(&sb, "    out_elements[%uu] = 0u;\n", i);
        for (size_t pos = 0u; pos <= text_len; ++pos) {
          unsigned char ch = (unsigned char)(pos < text_len ? text[pos] : '\0');
          float fs = run_fs;
          float lh = run_lh;
          float ls = run_ls;
          uint32_t weight = run_weight;
          StygianEditorColor color = run_color;
          bool break_line = (ch == '\n' || ch == '\0');
          if (pos < text_len) {
            editor_text_style_for_byte(&node->as.text, pos, &fs, &lh, &ls, &color,
                                       &weight);
            if (lh > line_max_lh)
              line_max_lh = lh;
          }
          if (!have_run && !break_line) {
            run_start = pos;
            run_x = cursor_x;
            run_y = cursor_y;
            run_fs = fs;
            run_lh = lh;
            run_ls = ls;
            run_weight = weight;
            run_color = color;
            have_run = true;
          } else if (have_run && !break_line &&
                     (editor_absf(run_fs - fs) > 0.0001f ||
                      editor_absf(run_lh - lh) > 0.0001f ||
                      editor_absf(run_ls - ls) > 0.0001f ||
                      run_weight != weight ||
                      editor_absf(run_color.r - color.r) > 0.0001f ||
                      editor_absf(run_color.g - color.g) > 0.0001f ||
                      editor_absf(run_color.b - color.b) > 0.0001f ||
                      editor_absf(run_color.a - color.a) > 0.0001f)) {
            if (!have_element) {
              sb_appendf(&sb,
                         "    out_elements[%uu] = stygian_editor_generated_text_run(ctx, ",
                         i);
            } else {
              sb_append_raw(&sb,
                            "    (void)stygian_editor_generated_text_run(ctx, ");
            }
            sb_append_c_string_range(&sb, text, run_start, pos);
            sb_append_raw(&sb, ", ");
            sb_append_float(&sb, run_x);
            sb_append_raw(&sb, ", ");
            sb_append_float(&sb, run_y);
            sb_append_raw(&sb, ", ");
            sb_append_float(&sb, run_fs);
            sb_append_raw(&sb, ", ");
            sb_append_float(&sb, run_ls);
            sb_append_raw(&sb, ", ");
            sb_appendf(&sb, "%uu, ", run_weight);
            sb_append_float(&sb, run_color.r);
            sb_append_raw(&sb, ", ");
            sb_append_float(&sb, run_color.g);
            sb_append_raw(&sb, ", ");
            sb_append_float(&sb, run_color.b);
            sb_append_raw(&sb, ", ");
            sb_append_float(&sb, run_color.a);
            sb_append_raw(&sb, ");\n");
            have_element = true;
            run_start = pos;
            run_x = cursor_x;
            run_y = cursor_y;
            run_fs = fs;
            run_lh = lh;
            run_ls = ls;
            run_weight = weight;
            run_color = color;
          }
          if (break_line) {
            if (have_run && pos > run_start) {
              if (!have_element) {
                sb_appendf(&sb,
                           "    out_elements[%uu] = stygian_editor_generated_text_run(ctx, ",
                           i);
              } else {
                sb_append_raw(&sb,
                              "    (void)stygian_editor_generated_text_run(ctx, ");
              }
              sb_append_c_string_range(&sb, text, run_start, pos);
              sb_append_raw(&sb, ", ");
              sb_append_float(&sb, run_x);
              sb_append_raw(&sb, ", ");
              sb_append_float(&sb, run_y);
              sb_append_raw(&sb, ", ");
              sb_append_float(&sb, run_fs);
              sb_append_raw(&sb, ", ");
              sb_append_float(&sb, run_ls);
              sb_append_raw(&sb, ", ");
              sb_appendf(&sb, "%uu, ", run_weight);
              sb_append_float(&sb, run_color.r);
              sb_append_raw(&sb, ", ");
              sb_append_float(&sb, run_color.g);
              sb_append_raw(&sb, ", ");
              sb_append_float(&sb, run_color.b);
              sb_append_raw(&sb, ", ");
              sb_append_float(&sb, run_color.a);
              sb_append_raw(&sb, ");\n");
              have_element = true;
            }
            have_run = false;
            if (ch == '\n') {
              float line_step = line_max_lh > 0.0f
                                    ? line_max_lh
                                    : editor_text_span_line_height(
                                          node->as.text.font_size > 0.0f
                                              ? node->as.text.font_size
                                              : 16.0f,
                                          node->as.text.line_height);
              cursor_x = node->as.text.x;
              cursor_y += line_step;
              line_max_lh = 0.0f;
            }
            continue;
          }
          cursor_x += fs * 0.6f + ls;
        }
        if (!have_element) {
          sb_appendf(&sb,
                     "    out_elements[%uu] = stygian_editor_generated_text_run(ctx, ",
                     i);
          sb_append_c_string(&sb, text);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.text.x);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.text.y);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.text.font_size);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.text.letter_spacing);
          sb_append_raw(&sb, ", ");
          sb_appendf(&sb, "%uu, ", node->as.text.font_weight);
          sb_append_float(&sb, node_fill.r);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node_fill.g);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node_fill.b);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node_fill.a);
          sb_append_raw(&sb, ");\n");
        }
        if (fabsf(node->rotation_deg) >= 0.0001f)
          sb_append_raw(&sb, "    if (_text_rot) stygian_transform_pop(ctx);\n");
        if (use_clip)
          sb_append_raw(&sb, "    if (_text_clip != 0u) stygian_clip_pop(ctx);\n");
        if (use_clip || fabsf(node->rotation_deg) >= 0.0001f)
          sb_append_raw(&sb, "  }\n");
      }
      sb_appendf(&sb, "  /* text_box=%uu align_h=%uu align_v=%uu auto=%uu */\n",
                 (uint32_t)node->as.text.box_mode,
                 (uint32_t)node->as.text.align_h,
                 (uint32_t)node->as.text.align_v,
                 (uint32_t)node->as.text.auto_size);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_IMAGE) {
      sb_appendf(&sb,
                 "  { StygianTexture _tex = "
                 "stygian_editor_generated_resolve_texture(");
      sb_append_c_string(&sb, node->as.image.source);
      sb_append_raw(&sb, ");\n");
      sb_appendf(&sb,
                 "    out_elements[%uu] = stygian_editor_generated_draw_image(ctx, _tex, ",
                 i);
      sb_append_float(&sb, node->as.image.x);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.image.y);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.image.w);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.image.h);
      sb_append_raw(&sb, ", ");
      sb_appendf(&sb, "%uu, ", node->as.image.fit_mode);
      sb_append_float(&sb, node_fill.a);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->rotation_deg);
      sb_append_raw(&sb, ");\n");
      sb_appendf(&sb, "    /* image_fit=%uu */\n", node->as.image.fit_mode);
      sb_append_raw(&sb, "  }\n");
    }

    if (fabsf(node->rotation_deg) >= 0.0001f) {
      float rot_x = 0.0f;
      float rot_y = 0.0f;
      float rot_w = 0.0f;
      float rot_h = 0.0f;
      bool can_rotate_element = true;
      if (node->kind == STYGIAN_EDITOR_SHAPE_RECT) {
        rot_x = node->as.rect.x;
        rot_y = node->as.rect.y;
        rot_w = node->as.rect.w;
        rot_h = node->as.rect.h;
      } else if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE) {
        rot_x = node->as.ellipse.x;
        rot_y = node->as.ellipse.y;
        rot_w = node->as.ellipse.w;
        rot_h = node->as.ellipse.h;
      } else if (node->kind == STYGIAN_EDITOR_SHAPE_FRAME) {
        rot_x = node->as.frame.x;
        rot_y = node->as.frame.y;
        rot_w = node->as.frame.w;
        rot_h = node->as.frame.h;
      } else {
        can_rotate_element = false;
      }
      if (can_rotate_element) {
        sb_appendf(&sb,
                   "  stygian_editor_generated_apply_rotation(ctx, "
                   "out_elements[%uu], ",
                   i);
        sb_append_float(&sb, rot_x);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, rot_y);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, rot_w);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, rot_h);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->rotation_deg);
        sb_append_raw(&sb, ");\n");
      }
    }

    if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
      sb_appendf(
          &sb,
          "  { uint8_t _clip_id = stygian_editor_generated_clip_id_for_owner(ctx, %uu); "
          "if (_clip_id != 0u) stygian_set_clip(ctx, out_elements[%uu], _clip_id); }\n",
          i, i);
    }
    if (node->shader_attachment.enabled) {
      sb_append_raw(&sb, "  /* authored shader attachment: slot=");
      sb_append_c_string(&sb, node->shader_attachment.slot_name);
      sb_append_raw(&sb, " asset=");
      sb_append_c_string(&sb, node->shader_attachment.asset_path);
      sb_append_raw(&sb, " entry=");
      sb_append_c_string(&sb, node->shader_attachment.entry_point);
      sb_append_raw(&sb,
                    " owner=editor-authored; runtime binding remains host-owned. */\n");
    }

    if (primary_fill &&
        primary_fill->kind == STYGIAN_EDITOR_FILL_LINEAR_GRADIENT) {
      StygianEditorColor c0 = primary_fill->stops[0].color;
      StygianEditorColor c1 = primary_fill->stops[1].color;
      StygianEditorGradientTransform gx = editor_gradient_xform_default();
      float gradient_angle = primary_fill->gradient_angle_deg;
      c0.a *= primary_fill->opacity;
      c1.a *= primary_fill->opacity;
      if (primary_fill_index >= 0 &&
          (uint32_t)primary_fill_index < STYGIAN_EDITOR_NODE_FILL_CAP) {
        gx = node->fill_gradient_xform[(uint32_t)primary_fill_index];
        editor_gradient_xform_sanitize(&gx);
      }
      gradient_angle += gx.rotation_deg + node->rotation_deg;
      sb_append_raw(&sb, "  /* gradient-xform: origin=");
      sb_append_float(&sb, gx.origin_x);
      sb_append_raw(&sb, ",");
      sb_append_float(&sb, gx.origin_y);
      sb_append_raw(&sb, " scale=");
      sb_append_float(&sb, gx.scale_x);
      sb_append_raw(&sb, ",");
      sb_append_float(&sb, gx.scale_y);
      sb_append_raw(&sb, " rot=");
      sb_append_float(&sb, gx.rotation_deg);
      sb_append_raw(&sb, " */\n");
      sb_appendf(&sb, "  stygian_set_gradient(ctx, out_elements[%uu], ", i);
      sb_append_float(&sb, gradient_angle);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, c0.r);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, c0.g);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, c0.b);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, c0.a);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, c1.r);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, c1.g);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, c1.b);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, c1.a);
      sb_append_raw(&sb, ");\n");
    }
    if (primary_fill && primary_fill->kind == STYGIAN_EDITOR_FILL_IMAGE &&
        (node->kind == STYGIAN_EDITOR_SHAPE_RECT ||
         node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE ||
         node->kind == STYGIAN_EDITOR_SHAPE_FRAME)) {
      sb_append_raw(&sb, "  { StygianTexture _tex = "
                         "stygian_editor_generated_resolve_texture(");
      sb_append_c_string(&sb, primary_fill->image_asset);
      sb_append_raw(&sb, ");\n");
      sb_appendf(&sb,
                 "    stygian_set_texture(ctx, out_elements[%uu], _tex, 0.0f, "
                 "0.0f, 1.0f, 1.0f);\n",
                 i);
      sb_append_raw(&sb, "  }\n");
    }

    for (eff_i = 0u; eff_i < node->effect_count &&
                    eff_i < STYGIAN_EDITOR_NODE_EFFECT_CAP;
         ++eff_i) {
      const StygianEditorNodeEffect *effect = &node->effects[eff_i];
      StygianEditorEffectTransform ex = node->effect_xform[eff_i];
      float mapped_offset_x;
      float mapped_offset_y;
      float mapped_radius;
      float mapped_spread;
      float mapped_intensity;
      float rotate_rad;
      StygianEditorColor c = effect->color;
      if (!effect->visible)
        continue;
      editor_effect_xform_sanitize(&ex);
      rotate_rad =
          (ex.rotation_deg + node->rotation_deg) * (3.14159265358979323846f / 180.0f);
      mapped_offset_x = effect->offset_x * ex.scale_x;
      mapped_offset_y = effect->offset_y * ex.scale_y;
      {
        float rx = mapped_offset_x * cosf(rotate_rad) -
                   mapped_offset_y * sinf(rotate_rad);
        float ry = mapped_offset_x * sinf(rotate_rad) +
                   mapped_offset_y * cosf(rotate_rad);
        mapped_offset_x = rx;
        mapped_offset_y = ry;
      }
      {
        float scale_mag = (fabsf(ex.scale_x) + fabsf(ex.scale_y)) * 0.5f;
        if (scale_mag < 0.0001f)
          scale_mag = 1.0f;
        mapped_radius = effect->radius * scale_mag;
        mapped_spread = effect->spread * scale_mag;
        mapped_intensity = effect->intensity * scale_mag;
      }
      c.a *= effect->opacity;
      sb_append_raw(&sb, "  /* effect-xform: scale=");
      sb_append_float(&sb, ex.scale_x);
      sb_append_raw(&sb, ",");
      sb_append_float(&sb, ex.scale_y);
      sb_append_raw(&sb, " rot=");
      sb_append_float(&sb, ex.rotation_deg);
      sb_append_raw(&sb, " */\n");
      if (effect->kind == STYGIAN_EDITOR_EFFECT_DROP_SHADOW ||
          effect->kind == STYGIAN_EDITOR_EFFECT_INNER_SHADOW) {
        sb_appendf(&sb, "  stygian_set_shadow(ctx, out_elements[%uu], ", i);
        sb_append_float(&sb, mapped_offset_x);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, mapped_offset_y);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, mapped_radius);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, mapped_spread);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, c.r);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, c.g);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, c.b);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, c.a);
        sb_append_raw(&sb, ");\n");
      } else if (effect->kind == STYGIAN_EDITOR_EFFECT_LAYER_BLUR) {
        sb_appendf(&sb, "  stygian_set_blur(ctx, out_elements[%uu], ", i);
        sb_append_float(&sb, mapped_radius);
        sb_append_raw(&sb, ");\n");
      } else if (effect->kind == STYGIAN_EDITOR_EFFECT_GLOW) {
        sb_appendf(&sb, "  stygian_set_glow(ctx, out_elements[%uu], ", i);
        sb_append_float(&sb, effect->intensity > 0.0f ? mapped_intensity
                                                       : mapped_radius);
        sb_append_raw(&sb, ");\n");
      }
    }

    if (node->color_token[0]) {
      sb_appendf(&sb, "  /* color-token: %s */\n", node->color_token);
    }
    sb_append_raw(&sb, "\n");
  }
  sb_appendf(&sb,
             "  stygian_editor_generated_apply_variable_mode("
             "ctx, out_elements, out_count, %uu);\n",
             editor->active_variable_mode);

  sb_append_raw(&sb, "}\n\n");

  sb_append_raw(
      &sb,
      "void stygian_editor_generated_draw_paths(StygianContext *ctx) {\n");
  if (editor->node_count == 0u) {
    sb_append_raw(&sb, "  (void)ctx;\n");
  } else {
    bool emitted_any = false;
    for (i = 0u; i < editor->node_count; ++i) {
      const StygianEditorNode *node = &editor->nodes[i];
      uint32_t j;
      if (node->kind == STYGIAN_EDITOR_SHAPE_PATH &&
          node->as.path.point_count >= 2u) {
        float rot_min_x = 0.0f;
        float rot_min_y = 0.0f;
        float rot_max_x = 0.0f;
        float rot_max_y = 0.0f;
        emitted_any = true;
        for (j = 0u; j < node->as.path.point_count; ++j) {
          StygianEditorPoint p =
              editor->path_points[node->as.path.first_point + j];
          if (j == 0u) {
            rot_min_x = rot_max_x = p.x;
            rot_min_y = rot_max_y = p.y;
          } else {
            if (p.x < rot_min_x)
              rot_min_x = p.x;
            if (p.y < rot_min_y)
              rot_min_y = p.y;
            if (p.x > rot_max_x)
              rot_max_x = p.x;
            if (p.y > rot_max_y)
              rot_max_y = p.y;
          }
        }
        sb_appendf(&sb, "  /* Path node %uu */\n", node->id);
        sb_appendf(&sb,
                   "  { bool _rot_%uu = stygian_editor_generated_push_rotation(ctx, ",
                   i);
        sb_append_float(&sb, rot_min_x);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, rot_min_y);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, rot_max_x - rot_min_x);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, rot_max_y - rot_min_y);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->rotation_deg);
        sb_append_raw(&sb, ");\n");
        if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
          sb_appendf(&sb,
                     "  { float _mx, _my, _mw, _mh; "
                     "if (stygian_editor_generated_mask_rect_for_owner(%uu, 0u, "
                     "&_mx, &_my, &_mw, &_mh)) {\n"
                     "      uint8_t _clip = stygian_clip_push(ctx, _mx, _my, _mw, _mh);\n"
                     "      if (_clip != 0u) {\n",
                     i);
        }
        for (j = 1u; j < node->as.path.point_count; ++j) {
          StygianEditorPoint a =
              editor->path_points[node->as.path.first_point + j - 1u];
          StygianEditorPoint b =
              editor->path_points[node->as.path.first_point + j];
          sb_append_raw(&sb, "  stygian_line(ctx, ");
          sb_append_float(&sb, a.x);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, a.y);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, b.x);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, b.y);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.path.thickness);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.path.stroke.r);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.path.stroke.g);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.path.stroke.b);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.path.stroke.a);
          sb_append_raw(&sb, ");\n");
        }
        if (node->as.path.closed && node->as.path.point_count > 2u) {
          StygianEditorPoint a =
              editor->path_points[node->as.path.first_point +
                                  node->as.path.point_count - 1u];
          StygianEditorPoint b = editor->path_points[node->as.path.first_point];
          sb_append_raw(&sb, "  stygian_line(ctx, ");
          sb_append_float(&sb, a.x);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, a.y);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, b.x);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, b.y);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.path.thickness);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.path.stroke.r);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.path.stroke.g);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.path.stroke.b);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.path.stroke.a);
          sb_append_raw(&sb, ");\n");
        }
        if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
          sb_append_raw(&sb,
                        "        stygian_clip_pop(ctx);\n"
                        "      }\n"
                        "    }\n"
                        "  }\n");
        }
        sb_appendf(&sb, "    if (_rot_%uu) stygian_transform_pop(ctx);\n", i);
        sb_append_raw(&sb, "  }\n\n");
      } else if (node->kind == STYGIAN_EDITOR_SHAPE_LINE) {
        float rot_x = node->as.line.x1 < node->as.line.x2 ? node->as.line.x1
                                                          : node->as.line.x2;
        float rot_y = node->as.line.y1 < node->as.line.y2 ? node->as.line.y1
                                                          : node->as.line.y2;
        float rot_w = fabsf(node->as.line.x2 - node->as.line.x1);
        float rot_h = fabsf(node->as.line.y2 - node->as.line.y1);
        emitted_any = true;
        sb_appendf(&sb, "  /* Line node %uu */\n", node->id);
        sb_appendf(&sb,
                   "  { bool _rot_%uu = stygian_editor_generated_push_rotation(ctx, ",
                   i);
        sb_append_float(&sb, rot_x);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, rot_y);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, rot_w);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, rot_h);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->rotation_deg);
        sb_append_raw(&sb, ");\n");
        if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
          sb_appendf(&sb,
                     "  { float _mx, _my, _mw, _mh; "
                     "if (stygian_editor_generated_mask_rect_for_owner(%uu, 0u, "
                     "&_mx, &_my, &_mw, &_mh)) {\n"
                     "      uint8_t _clip = stygian_clip_push(ctx, _mx, _my, _mw, _mh);\n"
                     "      if (_clip != 0u) {\n",
                     i);
        }
        sb_append_raw(&sb, "  stygian_line(ctx, ");
        sb_append_float(&sb, node->as.line.x1);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.line.y1);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.line.x2);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.line.y2);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.line.thickness);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.line.stroke.r);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.line.stroke.g);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.line.stroke.b);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.line.stroke.a);
        sb_append_raw(&sb, ");\n");
        if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
          sb_append_raw(&sb,
                        "        stygian_clip_pop(ctx);\n"
                        "      }\n"
                        "    }\n"
                        "  }\n");
        }
        sb_appendf(&sb, "    if (_rot_%uu) stygian_transform_pop(ctx);\n", i);
        sb_append_raw(&sb, "  }\n\n");
      } else if (node->kind == STYGIAN_EDITOR_SHAPE_ARROW) {
        float dx = node->as.arrow.x2 - node->as.arrow.x1;
        float dy = node->as.arrow.y2 - node->as.arrow.y1;
        float len = sqrtf(dx * dx + dy * dy);
        float ux = 0.0f;
        float uy = 0.0f;
        float px = 0.0f;
        float py = 0.0f;
        float back_x;
        float back_y;
        float left_x;
        float left_y;
        float right_x;
        float right_y;
        float rot_min_x;
        float rot_min_y;
        float rot_max_x;
        float rot_max_y;
        if (len < 0.0001f)
          continue;
        emitted_any = true;
        ux = dx / len;
        uy = dy / len;
        px = -uy;
        py = ux;
        back_x = node->as.arrow.x2 - ux * node->as.arrow.head_size;
        back_y = node->as.arrow.y2 - uy * node->as.arrow.head_size;
        left_x = back_x + px * (node->as.arrow.head_size * 0.5f);
        left_y = back_y + py * (node->as.arrow.head_size * 0.5f);
        right_x = back_x - px * (node->as.arrow.head_size * 0.5f);
        right_y = back_y - py * (node->as.arrow.head_size * 0.5f);
        rot_min_x = node->as.arrow.x1;
        rot_max_x = node->as.arrow.x1;
        rot_min_y = node->as.arrow.y1;
        rot_max_y = node->as.arrow.y1;
        if (node->as.arrow.x2 < rot_min_x)
          rot_min_x = node->as.arrow.x2;
        if (node->as.arrow.x2 > rot_max_x)
          rot_max_x = node->as.arrow.x2;
        if (left_x < rot_min_x)
          rot_min_x = left_x;
        if (left_x > rot_max_x)
          rot_max_x = left_x;
        if (right_x < rot_min_x)
          rot_min_x = right_x;
        if (right_x > rot_max_x)
          rot_max_x = right_x;
        if (node->as.arrow.y2 < rot_min_y)
          rot_min_y = node->as.arrow.y2;
        if (node->as.arrow.y2 > rot_max_y)
          rot_max_y = node->as.arrow.y2;
        if (left_y < rot_min_y)
          rot_min_y = left_y;
        if (left_y > rot_max_y)
          rot_max_y = left_y;
        if (right_y < rot_min_y)
          rot_min_y = right_y;
        if (right_y > rot_max_y)
          rot_max_y = right_y;
        sb_appendf(&sb, "  /* Arrow node %uu */\n", node->id);
        sb_appendf(&sb,
                   "  { bool _rot_%uu = stygian_editor_generated_push_rotation(ctx, ",
                   i);
        sb_append_float(&sb, rot_min_x);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, rot_min_y);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, rot_max_x - rot_min_x);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, rot_max_y - rot_min_y);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->rotation_deg);
        sb_append_raw(&sb, ");\n");
        if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
          sb_appendf(&sb,
                     "  { float _mx, _my, _mw, _mh; "
                     "if (stygian_editor_generated_mask_rect_for_owner(%uu, 0u, "
                     "&_mx, &_my, &_mw, &_mh)) {\n"
                     "      uint8_t _clip = stygian_clip_push(ctx, _mx, _my, _mw, _mh);\n"
                     "      if (_clip != 0u) {\n",
                     i);
        }
        sb_append_raw(&sb, "  stygian_line(ctx, ");
        sb_append_float(&sb, node->as.arrow.x1);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.y1);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.x2);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.y2);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.thickness);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.stroke.r);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.stroke.g);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.stroke.b);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.stroke.a);
        sb_append_raw(&sb, ");\n");
        sb_append_raw(&sb, "  stygian_line(ctx, ");
        sb_append_float(&sb, node->as.arrow.x2);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.y2);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, left_x);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, left_y);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.thickness);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.stroke.r);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.stroke.g);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.stroke.b);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.stroke.a);
        sb_append_raw(&sb, ");\n");
        sb_append_raw(&sb, "  stygian_line(ctx, ");
        sb_append_float(&sb, node->as.arrow.x2);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.y2);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, right_x);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, right_y);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.thickness);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.stroke.r);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.stroke.g);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.stroke.b);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arrow.stroke.a);
        sb_append_raw(&sb, ");\n");
        if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
          sb_append_raw(&sb,
                        "        stygian_clip_pop(ctx);\n"
                        "      }\n"
                        "    }\n"
                        "  }\n");
        }
        sb_appendf(&sb, "    if (_rot_%uu) stygian_transform_pop(ctx);\n", i);
        sb_append_raw(&sb, "  }\n\n");
      } else if (node->kind == STYGIAN_EDITOR_SHAPE_POLYGON) {
        uint32_t sides = node->as.polygon.sides < 3u ? 3u : node->as.polygon.sides;
        StygianEditorColor fill = editor_node_primary_fill_color(node);
        float cx = node->as.polygon.x + node->as.polygon.w * 0.5f;
        float cy = node->as.polygon.y + node->as.polygon.h * 0.5f;
        float rx = fabsf(node->as.polygon.w) * 0.5f;
        float ry = fabsf(node->as.polygon.h) * 0.5f;
        emitted_any = true;
        sb_appendf(&sb, "  /* Polygon node %uu */\n", node->id);
        sb_appendf(&sb,
                   "  { bool _rot_%uu = stygian_editor_generated_push_rotation(ctx, ",
                   i);
        sb_append_float(&sb, node->as.polygon.x);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.polygon.y);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.polygon.w);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.polygon.h);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->rotation_deg);
        sb_append_raw(&sb, ");\n");
        if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
          sb_appendf(&sb,
                     "  { float _mx, _my, _mw, _mh; "
                     "if (stygian_editor_generated_mask_rect_for_owner(%uu, 0u, "
                     "&_mx, &_my, &_mw, &_mh)) {\n"
                     "      uint8_t _clip = stygian_clip_push(ctx, _mx, _my, _mw, _mh);\n"
                     "      if (_clip != 0u) {\n",
                     i);
        }
        for (j = 0u; j < sides; ++j) {
          float a0 = (2.0f * 3.14159265358979323846f * (float)j) / (float)sides -
                     3.14159265358979323846f * 0.5f;
          float a1 = (2.0f * 3.14159265358979323846f * (float)(j + 1u)) /
                         (float)sides -
                     3.14159265358979323846f * 0.5f;
          float x0 = cx + cosf(a0) * rx;
          float y0 = cy + sinf(a0) * ry;
          float x1 = cx + cosf(a1) * rx;
          float y1 = cy + sinf(a1) * ry;
          sb_append_raw(&sb, "  stygian_line(ctx, ");
          sb_append_float(&sb, x0);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, y0);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, x1);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, y1);
          sb_append_raw(&sb, ", 1.000000f, ");
          sb_append_float(&sb, fill.r);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, fill.g);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, fill.b);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, fill.a);
          sb_append_raw(&sb, ");\n");
        }
        if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
          sb_append_raw(&sb,
                        "        stygian_clip_pop(ctx);\n"
                        "      }\n"
                        "    }\n"
                        "  }\n");
        }
        sb_appendf(&sb, "    if (_rot_%uu) stygian_transform_pop(ctx);\n", i);
        sb_append_raw(&sb, "  }\n\n");
      } else if (node->kind == STYGIAN_EDITOR_SHAPE_STAR) {
        uint32_t points = node->as.star.points < 3u ? 5u : node->as.star.points;
        uint32_t total = points * 2u;
        float inner = node->as.star.inner_ratio;
        StygianEditorColor fill = editor_node_primary_fill_color(node);
        float cx = node->as.star.x + node->as.star.w * 0.5f;
        float cy = node->as.star.y + node->as.star.h * 0.5f;
        float outer_rx = fabsf(node->as.star.w) * 0.5f;
        float outer_ry = fabsf(node->as.star.h) * 0.5f;
        float inner_rx = outer_rx * inner;
        float inner_ry = outer_ry * inner;
        emitted_any = true;
        sb_appendf(&sb, "  /* Star node %uu */\n", node->id);
        sb_appendf(&sb,
                   "  { bool _rot_%uu = stygian_editor_generated_push_rotation(ctx, ",
                   i);
        sb_append_float(&sb, node->as.star.x);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.star.y);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.star.w);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.star.h);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->rotation_deg);
        sb_append_raw(&sb, ");\n");
        if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
          sb_appendf(&sb,
                     "  { float _mx, _my, _mw, _mh; "
                     "if (stygian_editor_generated_mask_rect_for_owner(%uu, 0u, "
                     "&_mx, &_my, &_mw, &_mh)) {\n"
                     "      uint8_t _clip = stygian_clip_push(ctx, _mx, _my, _mw, _mh);\n"
                     "      if (_clip != 0u) {\n",
                     i);
        }
        for (j = 0u; j < total; ++j) {
          float a0 = (2.0f * 3.14159265358979323846f * (float)j) / (float)total -
                     3.14159265358979323846f * 0.5f;
          float a1 = (2.0f * 3.14159265358979323846f * (float)(j + 1u)) /
                         (float)total -
                     3.14159265358979323846f * 0.5f;
          float r0x = (j & 1u) ? inner_rx : outer_rx;
          float r0y = (j & 1u) ? inner_ry : outer_ry;
          float r1x = ((j + 1u) & 1u) ? inner_rx : outer_rx;
          float r1y = ((j + 1u) & 1u) ? inner_ry : outer_ry;
          float x0 = cx + cosf(a0) * r0x;
          float y0 = cy + sinf(a0) * r0y;
          float x1 = cx + cosf(a1) * r1x;
          float y1 = cy + sinf(a1) * r1y;
          sb_append_raw(&sb, "  stygian_line(ctx, ");
          sb_append_float(&sb, x0);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, y0);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, x1);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, y1);
          sb_append_raw(&sb, ", 1.000000f, ");
          sb_append_float(&sb, fill.r);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, fill.g);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, fill.b);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, fill.a);
          sb_append_raw(&sb, ");\n");
        }
        if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
          sb_append_raw(&sb,
                        "        stygian_clip_pop(ctx);\n"
                        "      }\n"
                        "    }\n"
                        "  }\n");
        }
        sb_appendf(&sb, "    if (_rot_%uu) stygian_transform_pop(ctx);\n", i);
        sb_append_raw(&sb, "  }\n\n");
      } else if (node->kind == STYGIAN_EDITOR_SHAPE_ARC) {
        uint32_t segments = 32u;
        float cx = node->as.arc.x + node->as.arc.w * 0.5f;
        float cy = node->as.arc.y + node->as.arc.h * 0.5f;
        float rx = fabsf(node->as.arc.w) * 0.5f;
        float ry = fabsf(node->as.arc.h) * 0.5f;
        float start = node->as.arc.start_angle * 3.14159265358979323846f / 180.0f;
        float sweep = node->as.arc.sweep_angle * 3.14159265358979323846f / 180.0f;
        emitted_any = true;
        sb_appendf(&sb, "  /* Arc node %uu */\n", node->id);
        sb_appendf(&sb,
                   "  { bool _rot_%uu = stygian_editor_generated_push_rotation(ctx, ",
                   i);
        sb_append_float(&sb, node->as.arc.x);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arc.y);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arc.w);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->as.arc.h);
        sb_append_raw(&sb, ", ");
        sb_append_float(&sb, node->rotation_deg);
        sb_append_raw(&sb, ");\n");
        if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
          sb_appendf(&sb,
                     "  { float _mx, _my, _mw, _mh; "
                     "if (stygian_editor_generated_mask_rect_for_owner(%uu, 0u, "
                     "&_mx, &_my, &_mw, &_mh)) {\n"
                     "      uint8_t _clip = stygian_clip_push(ctx, _mx, _my, _mw, _mh);\n"
                     "      if (_clip != 0u) {\n",
                     i);
        }
        for (j = 0u; j < segments; ++j) {
          float t0 = (float)j / (float)segments;
          float t1 = (float)(j + 1u) / (float)segments;
          float a0 = start + sweep * t0;
          float a1 = start + sweep * t1;
          float x0 = cx + cosf(a0) * rx;
          float y0 = cy + sinf(a0) * ry;
          float x1 = cx + cosf(a1) * rx;
          float y1 = cy + sinf(a1) * ry;
          sb_append_raw(&sb, "  stygian_line(ctx, ");
          sb_append_float(&sb, x0);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, y0);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, x1);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, y1);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.arc.thickness);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.arc.stroke.r);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.arc.stroke.g);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.arc.stroke.b);
          sb_append_raw(&sb, ", ");
          sb_append_float(&sb, node->as.arc.stroke.a);
          sb_append_raw(&sb, ");\n");
        }
        if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
          sb_append_raw(&sb,
                        "        stygian_clip_pop(ctx);\n"
                        "      }\n"
                        "    }\n"
                        "  }\n");
        }
        sb_appendf(&sb, "    if (_rot_%uu) stygian_transform_pop(ctx);\n", i);
        sb_append_raw(&sb, "  }\n\n");
      }
    }
    if (!emitted_any) {
      sb_append_raw(&sb, "  (void)ctx;\n");
    }
  }
  sb_append_raw(&sb, "}\n");

  sb_finish(&sb);
  return sb.len + 1u;
}

uint32_t stygian_editor_collect_export_diagnostics(
    const StygianEditor *editor, StygianEditorExportDiagnostic *out_diags,
    uint32_t max_diags) {
  return editor_collect_export_diagnostics_internal(editor, out_diags, max_diags,
                                                    NULL);
}

size_t stygian_editor_build_c23_with_diagnostics(
    const StygianEditor *editor, char *out_code, size_t out_capacity,
    StygianEditorExportDiagnostic *out_diags, uint32_t max_diags,
    uint32_t *out_diag_count, bool *out_has_error) {
  uint32_t diag_count = 0u;
  bool has_error = false;
  size_t result = 0u;

  diag_count = editor_collect_export_diagnostics_internal(
      editor, out_diags, max_diags, &has_error);
  if (out_diag_count)
    *out_diag_count = diag_count;
  if (out_has_error)
    *out_has_error = has_error;
  if (!editor || has_error)
    return 0u;
  result = stygian_editor_build_c23(editor, out_code, out_capacity);
  return result;
}

#include "stygian_editor_project_io.inl"
