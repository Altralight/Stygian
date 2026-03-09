#include "../include/stygian_editor.h"

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

typedef struct StygianEditorNode {
  StygianEditorNodeId id;
  StygianEditorShapeKind kind;
  bool visible;
  bool selected;
  float z;
  float value;
  char color_token[32];
  union {
    StygianEditorNodeRect rect;
    StygianEditorNodeEllipse ellipse;
    StygianEditorNodePath path;
  } as;
} StygianEditorNode;

typedef struct StygianEditorBehaviorSlot {
  StygianEditorBehaviorId id;
  StygianEditorBehaviorRule rule;
} StygianEditorBehaviorSlot;

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

  StygianEditorColorToken *color_tokens;
  uint32_t color_token_count;
  uint32_t max_color_tokens;

  StygianEditorPathBuilder path_builder;
  StygianEditorNodeId selected_node;

  StygianEditorNodeId next_node_id;
  StygianEditorPathId next_path_id;
  StygianEditorBehaviorId next_behavior_id;
};

static float editor_clampf(float v, float lo, float hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static float editor_absf(float v) { return v < 0.0f ? -v : v; }

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

static bool editor_node_bounds(const StygianEditorNode *node, float *out_x,
                               float *out_y, float *out_w, float *out_h);
static bool editor_node_hit_test(const StygianEditor *editor,
                                 const StygianEditorNode *node, float x,
                                 float y);

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

static bool editor_node_intersects_rect(const StygianEditorNode *node, float x,
                                        float y, float w, float h) {
  float nx = 0.0f;
  float ny = 0.0f;
  float nw = 0.0f;
  float nh = 0.0f;
  if (!node)
    return false;
  if (!editor_node_bounds(node, &nx, &ny, &nw, &nh))
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
  float bx;
  float by;
  float bw;
  float bh;

  if (!editor || !node || !node->visible)
    return false;

  switch (node->kind) {
  case STYGIAN_EDITOR_SHAPE_RECT:
    return editor_node_bounds(node, &bx, &by, &bw, &bh) &&
           x >= bx && x <= (bx + bw) && y >= by && y <= (by + bh);

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
    nx = (x - cx) / rx;
    ny = (y - cy) / ry;
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
      float d = editor_dist_sq_point_segment(x, y, a.x, a.y, b.x, b.y);
      if (d <= best_dist_sq)
        return true;
    }

    if (node->as.path.closed && node->as.path.point_count > 2u) {
      StygianEditorPoint a =
          editor->path_points[node->as.path.first_point + node->as.path.point_count -
                              1u];
      StygianEditorPoint b = editor->path_points[node->as.path.first_point];
      float d = editor_dist_sq_point_segment(x, y, a.x, a.y, b.x, b.y);
      if (d <= best_dist_sq)
        return true;
    }
    return false;
  }

  default:
    return false;
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
    if (node->kind == STYGIAN_EDITOR_SHAPE_RECT)
      *out_value = node->as.rect.x;
    else if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE)
      *out_value = node->as.ellipse.x;
    else if (node->kind == STYGIAN_EDITOR_SHAPE_PATH)
      *out_value = node->as.path.min_x;
    else
      return false;
    return true;

  case STYGIAN_EDITOR_PROP_Y:
    if (node->kind == STYGIAN_EDITOR_SHAPE_RECT)
      *out_value = node->as.rect.y;
    else if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE)
      *out_value = node->as.ellipse.y;
    else if (node->kind == STYGIAN_EDITOR_SHAPE_PATH)
      *out_value = node->as.path.min_y;
    else
      return false;
    return true;

  case STYGIAN_EDITOR_PROP_WIDTH:
    if (node->kind == STYGIAN_EDITOR_SHAPE_RECT)
      *out_value = node->as.rect.w;
    else if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE)
      *out_value = node->as.ellipse.w;
    else if (node->kind == STYGIAN_EDITOR_SHAPE_PATH)
      *out_value = node->as.path.max_x - node->as.path.min_x;
    else
      return false;
    return true;

  case STYGIAN_EDITOR_PROP_HEIGHT:
    if (node->kind == STYGIAN_EDITOR_SHAPE_RECT)
      *out_value = node->as.rect.h;
    else if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE)
      *out_value = node->as.ellipse.h;
    else if (node->kind == STYGIAN_EDITOR_SHAPE_PATH)
      *out_value = node->as.path.max_y - node->as.path.min_y;
    else
      return false;
    return true;

  case STYGIAN_EDITOR_PROP_OPACITY:
    if (node->kind == STYGIAN_EDITOR_SHAPE_RECT)
      *out_value = node->as.rect.fill.a;
    else if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE)
      *out_value = node->as.ellipse.fill.a;
    else if (node->kind == STYGIAN_EDITOR_SHAPE_PATH)
      *out_value = node->as.path.stroke.a;
    else
      return false;
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
    if (node->kind == STYGIAN_EDITOR_SHAPE_RECT) {
      node->as.rect.x = value;
      return true;
    }
    if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE) {
      node->as.ellipse.x = value;
      return true;
    }
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
    return false;

  case STYGIAN_EDITOR_PROP_Y:
    if (node->kind == STYGIAN_EDITOR_SHAPE_RECT) {
      node->as.rect.y = value;
      return true;
    }
    if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE) {
      node->as.ellipse.y = value;
      return true;
    }
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
    return false;

  case STYGIAN_EDITOR_PROP_WIDTH:
    if (node->kind == STYGIAN_EDITOR_SHAPE_RECT) {
      node->as.rect.w = value;
      return true;
    }
    if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE) {
      node->as.ellipse.w = value;
      return true;
    }
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
    return false;

  case STYGIAN_EDITOR_PROP_HEIGHT:
    if (node->kind == STYGIAN_EDITOR_SHAPE_RECT) {
      node->as.rect.h = value;
      return true;
    }
    if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE) {
      node->as.ellipse.h = value;
      return true;
    }
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
    return false;

  case STYGIAN_EDITOR_PROP_OPACITY:
    value = editor_clampf(value, 0.0f, 1.0f);
    if (node->kind == STYGIAN_EDITOR_SHAPE_RECT)
      node->as.rect.fill.a = value;
    else if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE)
      node->as.ellipse.fill.a = value;
    else if (node->kind == STYGIAN_EDITOR_SHAPE_PATH)
      node->as.path.stroke.a = value;
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

StygianEditorConfig stygian_editor_config_default(void) {
  StygianEditorConfig cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.max_nodes = 4096u;
  cfg.max_path_points = 32768u;
  cfg.max_behaviors = 2048u;
  cfg.max_color_tokens = 256u;
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

  if (!editor->nodes || !editor->path_points || !editor->behaviors ||
      !editor->active_anims || !editor->color_tokens) {
    stygian_editor_destroy(editor);
    return NULL;
  }

  editor->max_nodes = cfg.max_nodes;
  editor->max_path_points = cfg.max_path_points;
  editor->max_behaviors = cfg.max_behaviors;
  editor->max_color_tokens = cfg.max_color_tokens;
  editor->grid = cfg.grid;
  editor->viewport.width = 1280.0f;
  editor->viewport.height = 720.0f;
  editor->viewport.pan_x = 0.0f;
  editor->viewport.pan_y = 0.0f;
  editor->viewport.zoom = 1.0f;
  editor->next_node_id = 1u;
  editor->next_path_id = 1u;
  editor->next_behavior_id = 1u;
  editor->selected_node = STYGIAN_EDITOR_INVALID_ID;

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
  free(editor);
}

void stygian_editor_reset(StygianEditor *editor) {
  if (!editor)
    return;

  editor->node_count = 0u;
  editor->point_count = 0u;
  editor->behavior_count = 0u;
  editor->color_token_count = 0u;
  editor->selected_node = STYGIAN_EDITOR_INVALID_ID;
  editor->path_builder.active = false;
  editor->next_node_id = 1u;
  editor->next_path_id = 1u;
  editor->next_behavior_id = 1u;
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

  if (!editor || !editor->grid.enabled)
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

void stygian_editor_snap_world_point(const StygianEditor *editor,
                                     float world_x, float world_y,
                                     float *out_world_x, float *out_world_y) {
  if (out_world_x)
    *out_world_x = stygian_editor_snap_world_scalar(editor, world_x);
  if (out_world_y)
    *out_world_y = stygian_editor_snap_world_scalar(editor, world_y);
}

StygianEditorNodeId stygian_editor_add_rect(StygianEditor *editor,
                                            const StygianEditorRectDesc *desc) {
  StygianEditorNode *node;
  if (!editor || !desc)
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot add rect: node capacity reached (%u).",
                editor->max_nodes);
    return STYGIAN_EDITOR_INVALID_ID;
  }

  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_RECT;
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
  return node->id;
}

StygianEditorNodeId
stygian_editor_add_ellipse(StygianEditor *editor,
                           const StygianEditorEllipseDesc *desc) {
  StygianEditorNode *node;
  if (!editor || !desc)
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot add ellipse: node capacity reached (%u).",
                editor->max_nodes);
    return STYGIAN_EDITOR_INVALID_ID;
  }

  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_ELLIPSE;
  node->visible = desc->visible;
  node->selected = false;
  node->z = desc->z;
  node->as.ellipse.x = desc->x;
  node->as.ellipse.y = desc->y;
  node->as.ellipse.w = desc->w;
  node->as.ellipse.h = desc->h;
  node->as.ellipse.fill = desc->fill;
  return node->id;
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
  editor->point_count += 1u;
  editor->path_builder.point_count += 1u;
  return true;
}

StygianEditorNodeId stygian_editor_path_commit(StygianEditor *editor,
                                               StygianEditorPathId id) {
  StygianEditorNode *node;
  if (!editor || !editor->path_builder.active || editor->path_builder.id != id)
    return STYGIAN_EDITOR_INVALID_ID;
  if (editor->path_builder.point_count < 2u) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_WARN,
                "Cannot commit path with fewer than 2 points.");
    stygian_editor_path_cancel(editor, id);
    return STYGIAN_EDITOR_INVALID_ID;
  }
  if (editor->node_count >= editor->max_nodes) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR,
                "Cannot commit path: node capacity reached (%u).",
                editor->max_nodes);
    stygian_editor_path_cancel(editor, id);
    return STYGIAN_EDITOR_INVALID_ID;
  }

  node = &editor->nodes[editor->node_count++];
  memset(node, 0, sizeof(*node));
  node->id = editor->next_node_id++;
  node->kind = STYGIAN_EDITOR_SHAPE_PATH;
  node->visible = editor->path_builder.desc.visible;
  node->selected = false;
  node->z = editor->path_builder.desc.z;
  node->as.path.first_point = editor->path_builder.first_point;
  node->as.path.point_count = editor->path_builder.point_count;
  node->as.path.closed = editor->path_builder.desc.closed;
  node->as.path.thickness = editor->path_builder.desc.thickness;
  node->as.path.stroke = editor->path_builder.desc.stroke;
  editor_recompute_path_bounds(editor, node);

  editor->path_builder.active = false;
  return node->id;
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
  if (!node)
    return false;
  if (snap_to_grid)
    stygian_editor_snap_world_point(editor, x, y, &x, &y);
  if (!editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_X, x))
    return false;
  if (!editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_Y, y))
    return false;
  editor_request_repaint(editor, 120u);
  return true;
}

bool stygian_editor_node_set_size(StygianEditor *editor,
                                  StygianEditorNodeId node_id, float w,
                                  float h) {
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!node)
    return false;
  if (!editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_WIDTH, w))
    return false;
  if (!editor_node_apply_property(editor, node, STYGIAN_EDITOR_PROP_HEIGHT, h))
    return false;
  editor_request_repaint(editor, 120u);
  return true;
}

bool stygian_editor_node_set_color(StygianEditor *editor,
                                   StygianEditorNodeId node_id,
                                   StygianEditorColor color) {
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!node)
    return false;

  if (node->kind == STYGIAN_EDITOR_SHAPE_RECT)
    node->as.rect.fill = color;
  else if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE)
    node->as.ellipse.fill = color;
  else if (node->kind == STYGIAN_EDITOR_SHAPE_PATH)
    node->as.path.stroke = color;
  else
    return false;

  node->color_token[0] = '\0';
  editor_request_repaint(editor, 120u);
  return true;
}

bool stygian_editor_node_get_color(const StygianEditor *editor,
                                   StygianEditorNodeId node_id,
                                   StygianEditorColor *out_color) {
  const StygianEditorNode *node = editor_find_node_const(editor, node_id);
  if (!node || !out_color)
    return false;
  if (node->kind == STYGIAN_EDITOR_SHAPE_RECT)
    *out_color = node->as.rect.fill;
  else if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE)
    *out_color = node->as.ellipse.fill;
  else if (node->kind == STYGIAN_EDITOR_SHAPE_PATH)
    *out_color = node->as.path.stroke;
  else
    return false;
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
  StygianEditorNode *node = editor_find_node(editor, node_id);
  if (!node || node->kind != STYGIAN_EDITOR_SHAPE_RECT)
    return false;
  node->as.rect.radius[0] = top_left < 0.0f ? 0.0f : top_left;
  node->as.rect.radius[1] = top_right < 0.0f ? 0.0f : top_right;
  node->as.rect.radius[2] = bottom_right < 0.0f ? 0.0f : bottom_right;
  node->as.rect.radius[3] = bottom_left < 0.0f ? 0.0f : bottom_left;
  editor_request_repaint(editor, 120u);
  return true;
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
  return editor_node_bounds(node, out_x, out_y, out_w, out_h);
}

bool stygian_editor_delete_node(StygianEditor *editor,
                                StygianEditorNodeId node_id) {
  int idx;
  uint32_t i;
  if (!editor || node_id == STYGIAN_EDITOR_INVALID_ID)
    return false;

  idx = editor_find_node_index(editor, node_id);
  if (idx < 0)
    return false;

  for (i = (uint32_t)idx; (i + 1u) < editor->node_count; ++i) {
    editor->nodes[i] = editor->nodes[i + 1u];
  }
  editor->node_count -= 1u;

  editor_refresh_primary_selected(editor);

  for (i = 0u; i < editor->behavior_count;) {
    StygianEditorBehaviorSlot *slot = &editor->behaviors[i];
    if (slot->rule.trigger_node == node_id ||
        slot->rule.animate.target_node == node_id) {
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

  editor_request_repaint(editor, 120u);
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
    if (editor_node_intersects_rect(node, min_x, min_y, w, h))
      node->selected = true;
  }

  editor_refresh_primary_selected(editor);
  return editor_count_selected_nodes(editor);
}

bool stygian_editor_set_color_token(StygianEditor *editor, const char *name,
                                    StygianEditorColor color) {
  int idx;
  if (!editor || !name || !name[0])
    return false;

  idx = editor_find_color_token(editor, name);
  if (idx >= 0) {
    editor->color_tokens[idx].color = color;
    return true;
  }

  if (editor->color_token_count >= editor->max_color_tokens)
    return false;
  if (!editor_copy_token_name(editor->color_tokens[editor->color_token_count].name,
                              sizeof(editor->color_tokens[0].name), name)) {
    return false;
  }

  editor->color_tokens[editor->color_token_count].color = color;
  editor->color_token_count += 1u;
  return true;
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
  StygianEditorColor color;
  StygianEditorNode *node;
  if (!editor || !name || !name[0])
    return false;
  if (!stygian_editor_get_color_token(editor, name, &color))
    return false;
  node = editor_find_node(editor, node_id);
  if (!node)
    return false;
  if (!stygian_editor_node_set_color(editor, node_id, color))
    return false;
  if (!editor_copy_token_name(node->color_token, sizeof(node->color_token), name))
    node->color_token[0] = '\0';
  return true;
}

bool stygian_editor_add_behavior(StygianEditor *editor,
                                 const StygianEditorBehaviorRule *rule,
                                 StygianEditorBehaviorId *out_behavior_id) {
  StygianEditorBehaviorSlot *slot;
  if (!editor || !rule)
    return false;
  if (editor->behavior_count >= editor->max_behaviors)
    return false;
  if (editor_find_node_index(editor, rule->trigger_node) < 0)
    return false;
  if (rule->action_kind != STYGIAN_EDITOR_ACTION_ANIMATE)
    return false;
  if (editor_find_node_index(editor, rule->animate.target_node) < 0)
    return false;

  slot = &editor->behaviors[editor->behavior_count];
  memset(slot, 0, sizeof(*slot));
  slot->id = editor->next_behavior_id++;
  slot->rule = *rule;
  editor->active_anims[editor->behavior_count].active = false;
  editor->behavior_count += 1u;

  if (out_behavior_id)
    *out_behavior_id = slot->id;
  return true;
}

bool stygian_editor_remove_behavior(StygianEditor *editor,
                                    StygianEditorBehaviorId behavior_id) {
  uint32_t i;
  if (!editor || behavior_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  for (i = 0u; i < editor->behavior_count; ++i) {
    if (editor->behaviors[i].id == behavior_id) {
      uint32_t j;
      for (j = i; (j + 1u) < editor->behavior_count; ++j) {
        editor->behaviors[j] = editor->behaviors[j + 1u];
        editor->active_anims[j] = editor->active_anims[j + 1u];
      }
      editor->behavior_count -= 1u;
      return true;
    }
  }
  return false;
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
    float from_value = rule->animate.from_value;
    const StygianEditorNode *target_node;

    if (rule->trigger_node != trigger_node ||
        rule->trigger_event != event_kind ||
        rule->action_kind != STYGIAN_EDITOR_ACTION_ANIMATE) {
      continue;
    }

    target_node = editor_find_node_const(editor, rule->animate.target_node);
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
    if (!node->visible)
      continue;

    if (node->kind == STYGIAN_EDITOR_SHAPE_RECT) {
      float x;
      float y;
      float w = node->as.rect.w * editor_safe_zoom(editor);
      float h = node->as.rect.h * editor_safe_zoom(editor);
      StygianElement e = stygian_element_transient(ctx);
      stygian_editor_world_to_view(editor, node->as.rect.x, node->as.rect.y, &x, &y);
      stygian_set_type(ctx, e, STYGIAN_RECT);
      stygian_set_bounds(ctx, e, x, y, w, h);
      stygian_set_radius(
          ctx, e, node->as.rect.radius[0] * editor_safe_zoom(editor),
          node->as.rect.radius[1] * editor_safe_zoom(editor),
          node->as.rect.radius[2] * editor_safe_zoom(editor),
          node->as.rect.radius[3] * editor_safe_zoom(editor));
      stygian_set_color(ctx, e, node->as.rect.fill.r, node->as.rect.fill.g,
                        node->as.rect.fill.b, node->as.rect.fill.a);
      stygian_set_z(ctx, e, editor_safe_clip_z(node->z));
      stygian_set_visible(ctx, e, true);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE) {
      float x;
      float y;
      float w = node->as.ellipse.w * editor_safe_zoom(editor);
      float h = node->as.ellipse.h * editor_safe_zoom(editor);
      StygianElement e = stygian_element_transient(ctx);
      stygian_editor_world_to_view(editor, node->as.ellipse.x, node->as.ellipse.y,
                                   &x, &y);
      stygian_set_type(ctx, e, STYGIAN_CIRCLE);
      stygian_set_bounds(ctx, e, x, y, w, h);
      stygian_set_color(ctx, e, node->as.ellipse.fill.r, node->as.ellipse.fill.g,
                        node->as.ellipse.fill.b, node->as.ellipse.fill.a);
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
        stygian_editor_world_to_view(editor, a.x, a.y, &x1, &y1);
        stygian_editor_world_to_view(editor, b.x, b.y, &x2, &y2);
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
        stygian_editor_world_to_view(editor, a.x, a.y, &x1, &y1);
        stygian_editor_world_to_view(editor, b.x, b.y, &x2, &y2);
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
      if (!editor_node_bounds(selected, &x, &y, &w, &h))
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
  default:
    return "UNKNOWN";
  }
}

size_t stygian_editor_build_c23(const StygianEditor *editor, char *out_code,
                                size_t out_capacity) {
  StygianEditorStringBuilder sb;
  uint32_t i;

  if (!editor)
    return 0u;

  sb.dst = out_code;
  sb.cap = out_capacity;
  sb.len = 0u;

  sb_append_raw(&sb,
                "/* Auto-generated by Stygian Editor (attachable module). */\n");
  sb_append_raw(&sb,
                "/* Deterministic output: same model state => same C23 text. */\n");
  sb_append_raw(&sb, "#include \"stygian.h\"\n");
  sb_append_raw(&sb, "#include <stdint.h>\n");
  sb_append_raw(&sb, "#include <stdbool.h>\n");
  sb_append_raw(&sb, "#include <math.h>\n\n");

  sb_appendf(&sb, "#define STYGIAN_EDITOR_GENERATED_NODE_COUNT %uu\n",
             editor->node_count);
  sb_appendf(&sb, "#define STYGIAN_EDITOR_GENERATED_BEHAVIOR_COUNT %uu\n\n",
             editor->behavior_count);

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
  sb_append_raw(&sb, "} StygianEditorGeneratedEventKind;\n\n");

  sb_append_raw(&sb, "typedef struct StygianEditorGeneratedAnimationRule {\n");
  sb_append_raw(&sb, "  uint32_t trigger_node_id;\n");
  sb_append_raw(&sb, "  uint32_t trigger_event;\n");
  sb_append_raw(&sb, "  uint32_t target_node_id;\n");
  sb_append_raw(&sb, "  uint32_t target_property;\n");
  sb_append_raw(&sb, "  float from_value;\n");
  sb_append_raw(&sb, "  float to_value;\n");
  sb_append_raw(&sb, "  uint32_t duration_ms;\n");
  sb_append_raw(&sb, "  uint32_t easing;\n");
  sb_append_raw(&sb, "} StygianEditorGeneratedAnimationRule;\n\n");

  if (editor->behavior_count > 0u) {
    sb_appendf(
        &sb,
        "static const StygianEditorGeneratedAnimationRule "
        "kStygianEditorGeneratedRules[%uu] = {\n",
        editor->behavior_count);
    for (i = 0u; i < editor->behavior_count; ++i) {
      const StygianEditorBehaviorRule *rule = &editor->behaviors[i].rule;
      sb_append_raw(&sb, "  { ");
      sb_appendf(&sb, "%uu, %uu, %uu, %uu, ", rule->trigger_node,
                 (uint32_t)rule->trigger_event, rule->animate.target_node,
                 (uint32_t)rule->animate.property);
      sb_append_float(&sb, rule->animate.from_value);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, rule->animate.to_value);
      sb_appendf(&sb, ", %uu, %uu },\n", rule->animate.duration_ms,
                 (uint32_t)rule->animate.easing);
    }
    sb_append_raw(&sb, "};\n\n");
  } else {
    sb_append_raw(
        &sb,
        "static const StygianEditorGeneratedAnimationRule "
        "kStygianEditorGeneratedRules[1] = {\n");
    sb_append_raw(&sb, "  { 0u, 0u, 0u, 0u, 0.0f, 0.0f, 0u, 0u },\n");
    sb_append_raw(&sb, "};\n\n");
  }

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
      "int stygian_editor_generated_node_index(uint32_t node_id) {\n");
  sb_append_raw(&sb, "  switch (node_id) {\n");
  for (i = 0u; i < editor->node_count; ++i) {
    sb_appendf(&sb, "  case %uu: return %d;\n", editor->nodes[i].id, (int)i);
  }
  sb_append_raw(&sb, "  default: return -1;\n");
  sb_append_raw(&sb, "  }\n");
  sb_append_raw(&sb, "}\n\n");

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
    sb_appendf(&sb, "  /* Node %uu (%s) */\n", node->id,
               editor_codegen_node_kind(node->kind));

    if (node->kind == STYGIAN_EDITOR_SHAPE_PATH) {
      sb_appendf(&sb,
                 "  out_elements[%uu] = 0u; /* path emits via "
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
      sb_append_float(&sb, node->as.rect.fill.r);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.rect.fill.g);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.rect.fill.b);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.rect.fill.a);
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
      sb_append_float(&sb, node->as.ellipse.fill.r);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.ellipse.fill.g);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.ellipse.fill.b);
      sb_append_raw(&sb, ", ");
      sb_append_float(&sb, node->as.ellipse.fill.a);
      sb_append_raw(&sb, ");\n");
    }

    if (node->color_token[0]) {
      sb_appendf(&sb, "  /* color-token: %s */\n", node->color_token);
    }
    sb_append_raw(&sb, "\n");
  }

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
      if (node->kind != STYGIAN_EDITOR_SHAPE_PATH || node->as.path.point_count < 2u)
        continue;
      emitted_any = true;
      sb_appendf(&sb, "  /* Path node %uu */\n", node->id);
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
            editor->path_points[node->as.path.first_point + node->as.path.point_count -
                                1u];
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
      sb_append_raw(&sb, "\n");
    }
    if (!emitted_any) {
      sb_append_raw(&sb, "  (void)ctx;\n");
    }
  }
  sb_append_raw(&sb, "}\n");

  sb_finish(&sb);
  return sb.len + 1u;
}
