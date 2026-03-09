#include "../include/stygian.h"
#include "../include/stygian_editor.h"
#include "../layout/stygian_dock.h"
#include "../widgets/stygian_widgets.h"
#include "../window/stygian_input.h"
#include "../window/stygian_window.h"

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef STYGIAN_DEMO_VULKAN
#define STYGIAN_EDITOR_BOOTSTRAP_BACKEND STYGIAN_BACKEND_VULKAN
#define STYGIAN_EDITOR_BOOTSTRAP_WINDOW_FLAGS                                  \
  (STYGIAN_WINDOW_RESIZABLE | STYGIAN_WINDOW_VULKAN)
#define STYGIAN_EDITOR_BOOTSTRAP_RENDERER "Vulkan"
#else
#define STYGIAN_EDITOR_BOOTSTRAP_BACKEND STYGIAN_BACKEND_OPENGL
#define STYGIAN_EDITOR_BOOTSTRAP_WINDOW_FLAGS                                  \
  (STYGIAN_WINDOW_RESIZABLE | STYGIAN_WINDOW_OPENGL)
#define STYGIAN_EDITOR_BOOTSTRAP_RENDERER "OpenGL"
#endif

typedef struct EditorLayout {
  float top_x;
  float top_y;
  float top_w;
  float top_h;

  float left_x;
  float left_y;
  float left_w;
  float left_h;

  float viewport_x;
  float viewport_y;
  float viewport_w;
  float viewport_h;

  float right_x;
  float right_y;
  float right_w;
  float right_h;
} EditorLayout;

typedef enum AppPrimitiveKind {
  APP_PRIMITIVE_SQUARE = 0,
  APP_PRIMITIVE_CIRCLE = 1,
  APP_PRIMITIVE_RECTANGLE = 2,
  APP_PRIMITIVE_ELLIPSE = 3
} AppPrimitiveKind;

typedef enum AppResizeHandle {
  APP_RESIZE_NONE = 0,
  APP_RESIZE_N = 1,
  APP_RESIZE_NE = 2,
  APP_RESIZE_E = 3,
  APP_RESIZE_SE = 4,
  APP_RESIZE_S = 5,
  APP_RESIZE_SW = 6,
  APP_RESIZE_W = 7,
  APP_RESIZE_NW = 8
} AppResizeHandle;

#define APP_MAX_TRANSFORM_SELECTION 8192u
#define APP_MAX_NODE_FLAGS 16384u

typedef struct EditorBootstrapApp {
  StygianContext *ctx;
  StygianEditor *editor;
  StygianDockSpace *dock;
  StygianFont font;
  EditorLayout layout;

  uint32_t panel_tools_id;
  uint32_t panel_viewport_id;
  uint32_t panel_inspector_id;
  uint32_t panel_behavior_graph_id;
  uint32_t panel_console_id;

  int last_mouse_x;
  int last_mouse_y;

  float camera_pan_x;
  float camera_pan_y;
  float camera_zoom;

  bool dragging_node;
  StygianEditorNodeId drag_node_id;
  float drag_offset_x;
  float drag_offset_y;
  float drag_start_mouse_world_x;
  float drag_start_mouse_world_y;
  uint32_t drag_selection_count;
  StygianEditorNodeId drag_selection_ids[APP_MAX_TRANSFORM_SELECTION];
  float drag_selection_start_x[APP_MAX_TRANSFORM_SELECTION];
  float drag_selection_start_y[APP_MAX_TRANSFORM_SELECTION];
  bool resizing_node;
  StygianEditorNodeId resize_node_id;
  AppResizeHandle resize_handle;
  float resize_start_mouse_world_x;
  float resize_start_mouse_world_y;
  float resize_start_x;
  float resize_start_y;
  float resize_start_w;
  float resize_start_h;
  uint32_t resize_selection_count;
  StygianEditorNodeId resize_selection_ids[APP_MAX_TRANSFORM_SELECTION];
  float resize_selection_start_x[APP_MAX_TRANSFORM_SELECTION];
  float resize_selection_start_y[APP_MAX_TRANSFORM_SELECTION];
  float resize_selection_start_w[APP_MAX_TRANSFORM_SELECTION];
  float resize_selection_start_h[APP_MAX_TRANSFORM_SELECTION];
  StygianEditorNodeId selection_ids_cache[APP_MAX_TRANSFORM_SELECTION];
  StygianEditorNodeId uniform_scale_node_ids[APP_MAX_NODE_FLAGS];
  bool uniform_scale_values[APP_MAX_NODE_FLAGS];
  uint32_t uniform_scale_count;
  bool marquee_selecting;
  bool marquee_additive;
  float marquee_start_view_x;
  float marquee_start_view_y;
  float marquee_curr_view_x;
  float marquee_curr_view_y;

  bool panning;

  bool path_tool_active;
  StygianEditorPathId active_path;

  AppPrimitiveKind selected_primitive;
  bool file_menu_open;
  bool primitive_menu_open;
  int primitive_menu_hover_index;
  bool left_mouse_was_down;

  bool grid_enabled;
  bool sub_snap_enabled;
  float major_step_px;
  float snap_tolerance_px;

  StygianEditorColor paint_color;
  float panel_group_x;
  float panel_group_y;
  float panel_group_w;
  float panel_group_h;
  float panel_radius_tl;
  float panel_radius_tr;
  float panel_radius_br;
  float panel_radius_bl;
  float panel_opacity;
  bool panel_uniform_scale;
  char token_name[32];

  char behavior_trigger_id[16];
  char behavior_target_id[16];
  char behavior_from_value[24];
  char behavior_to_value[24];
  char behavior_duration_ms[16];
  int behavior_event_choice;
  int behavior_property_choice;
  int behavior_easing_choice;

  StygianEditorNodeId demo_slider_node;
  StygianEditorNodeId demo_box_node;

  float preview_slider_value;
  char status_text[192];
  char behavior_graph_message[192];
  StygianEditorPoint path_preview_points[4096];
  uint32_t path_preview_count;

  uint64_t now_ms;
} EditorBootstrapApp;

static EditorBootstrapApp *g_editor_bootstrap_app = NULL;

static uint64_t app_now_ms_system(void) {
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}

static float app_clampf(float v, float lo, float hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static bool app_point_in_rect(float px, float py, float x, float y, float w,
                              float h) {
  return px >= x && px <= x + w && py >= y && py <= y + h;
}

static void app_set_status(EditorBootstrapApp *app, const char *text) {
  if (!app || !text)
    return;
  snprintf(app->status_text, sizeof(app->status_text), "%s", text);
}

static float app_menu_bar_height(void) { return 24.0f; }

static float app_tool_strip_height(void) { return 34.0f; }

static float app_top_chrome_height(void) {
  return app_menu_bar_height() + app_tool_strip_height();
}

static const char *app_primitive_name(AppPrimitiveKind primitive) {
  switch (primitive) {
  case APP_PRIMITIVE_SQUARE:
    return "Square";
  case APP_PRIMITIVE_CIRCLE:
    return "Circle";
  case APP_PRIMITIVE_RECTANGLE:
    return "Rectangle";
  case APP_PRIMITIVE_ELLIPSE:
    return "Ellipse";
  default:
    return "Rectangle";
  }
}

static float app_properties_panel_width(float window_w) {
  float panel_w = 292.0f;
  if (window_w < 980.0f)
    panel_w = 246.0f;
  if (panel_w > window_w * 0.46f)
    panel_w = window_w * 0.46f;
  if (panel_w < 210.0f)
    panel_w = 210.0f;
  return panel_w;
}

static int app_find_uniform_scale_index(const EditorBootstrapApp *app,
                                        StygianEditorNodeId node_id) {
  uint32_t i;
  if (!app || node_id == STYGIAN_EDITOR_INVALID_ID)
    return -1;
  for (i = 0u; i < app->uniform_scale_count; ++i) {
    if (app->uniform_scale_node_ids[i] == node_id)
      return (int)i;
  }
  return -1;
}

static bool app_default_uniform_scale_for_kind(StygianEditorShapeKind kind) {
  return kind == STYGIAN_EDITOR_SHAPE_ELLIPSE;
}

static bool app_get_node_shape_kind(const EditorBootstrapApp *app,
                                    StygianEditorNodeId node_id,
                                    StygianEditorShapeKind *out_kind) {
  if (!app || !app->editor || !out_kind)
    return false;
  return stygian_editor_node_get_shape_kind(app->editor, node_id, out_kind);
}

static bool app_node_uniform_scale_enabled(const EditorBootstrapApp *app,
                                           StygianEditorNodeId node_id) {
  int idx;
  StygianEditorShapeKind kind = STYGIAN_EDITOR_SHAPE_RECT;
  if (!app)
    return false;
  idx = app_find_uniform_scale_index(app, node_id);
  if (idx >= 0)
    return app->uniform_scale_values[idx];
  if (app_get_node_shape_kind(app, node_id, &kind))
    return app_default_uniform_scale_for_kind(kind);
  return false;
}

static void app_set_node_uniform_scale(EditorBootstrapApp *app,
                                       StygianEditorNodeId node_id, bool enabled) {
  int idx;
  if (!app || node_id == STYGIAN_EDITOR_INVALID_ID)
    return;
  idx = app_find_uniform_scale_index(app, node_id);
  if (idx >= 0) {
    app->uniform_scale_values[idx] = enabled;
    return;
  }
  if (app->uniform_scale_count >= APP_MAX_NODE_FLAGS)
    return;
  app->uniform_scale_node_ids[app->uniform_scale_count] = node_id;
  app->uniform_scale_values[app->uniform_scale_count] = enabled;
  app->uniform_scale_count += 1u;
}

static void app_layout_set_canvas(EditorBootstrapApp *app, int width,
                                  int height) {
  float top_h;
  float right_w;
  float viewport_w;
  if (!app)
    return;
  top_h = app_top_chrome_height();
  if ((float)height < top_h)
    top_h = (float)height;
  right_w = app_properties_panel_width((float)width);
  viewport_w = (float)width - right_w;
  if (viewport_w < 220.0f) {
    right_w = (float)width - 220.0f;
    if (right_w < 0.0f)
      right_w = 0.0f;
    viewport_w = (float)width - right_w;
  }

  app->layout.top_x = 0.0f;
  app->layout.top_y = 0.0f;
  app->layout.top_w = (float)width;
  app->layout.top_h = top_h;
  app->layout.viewport_x = 0.0f;
  app->layout.viewport_y = top_h;
  app->layout.viewport_w = viewport_w;
  app->layout.viewport_h = (float)height - top_h;
  if (app->layout.viewport_h < 0.0f)
    app->layout.viewport_h = 0.0f;
  app->layout.right_x = app->layout.viewport_w;
  app->layout.right_y = top_h;
  app->layout.right_w = right_w;
  app->layout.right_h = app->layout.viewport_h;
}

static void app_primitive_menu_geometry(const EditorBootstrapApp *app,
                                        float *out_x, float *out_y,
                                        float *out_w, float *out_row_h,
                                        float *out_h) {
  (void)app;
  if (out_x)
    *out_x = 18.0f;
  if (out_y)
    *out_y = app_top_chrome_height() + 2.0f;
  if (out_w)
    *out_w = 142.0f;
  if (out_row_h)
    *out_row_h = 20.0f;
  if (out_h)
    *out_h = 20.0f * 4.0f + 8.0f;
}

static int app_primitive_menu_hover_index_at(const EditorBootstrapApp *app,
                                             float mouse_x, float mouse_y) {
  float menu_x = 0.0f;
  float menu_y = 0.0f;
  float menu_w = 0.0f;
  float row_h = 0.0f;
  float content_x;
  float content_y;
  float content_w;
  float content_h;
  int hover;
  if (!app)
    return -1;
  app_primitive_menu_geometry(app, &menu_x, &menu_y, &menu_w, &row_h, NULL);
  content_x = menu_x + 4.0f;
  content_y = menu_y + 4.0f;
  content_w = menu_w - 8.0f;
  content_h = row_h * 4.0f;
  if (!app_point_in_rect(mouse_x, mouse_y, content_x, content_y, content_w,
                         content_h))
    return -1;
  hover = (int)((mouse_y - content_y) / row_h);
  if (hover < 0 || hover >= 4)
    return -1;
  return hover;
}

static float app_resize_handle_size_px(void) { return 10.0f; }

static bool app_bounds_for_nodes_world(const EditorBootstrapApp *app,
                                       const StygianEditorNodeId *node_ids,
                                       uint32_t node_count, float *out_x,
                                       float *out_y, float *out_w,
                                       float *out_h) {
  uint32_t i;
  bool have_bounds = false;
  float min_x = 0.0f;
  float min_y = 0.0f;
  float max_x = 0.0f;
  float max_y = 0.0f;
  if (!app || !app->editor || !node_ids || node_count == 0u)
    return false;
  for (i = 0u; i < node_count; ++i) {
    float nx = 0.0f;
    float ny = 0.0f;
    float nw = 0.0f;
    float nh = 0.0f;
    if (!stygian_editor_node_get_bounds(app->editor, node_ids[i], &nx, &ny, &nw,
                                        &nh)) {
      continue;
    }
    if (!have_bounds) {
      min_x = nx;
      min_y = ny;
      max_x = nx + nw;
      max_y = ny + nh;
      have_bounds = true;
    } else {
      if (nx < min_x)
        min_x = nx;
      if (ny < min_y)
        min_y = ny;
      if (nx + nw > max_x)
        max_x = nx + nw;
      if (ny + nh > max_y)
        max_y = ny + nh;
    }
  }
  if (!have_bounds)
    return false;
  if (out_x)
    *out_x = min_x;
  if (out_y)
    *out_y = min_y;
  if (out_w)
    *out_w = max_x - min_x;
  if (out_h)
    *out_h = max_y - min_y;
  return true;
}

static bool app_selected_bounds_world(EditorBootstrapApp *app, float *out_x,
                                      float *out_y, float *out_w,
                                      float *out_h) {
  uint32_t selected_count;
  if (!app || !app->editor)
    return false;
  selected_count =
      stygian_editor_selected_nodes(app->editor, app->selection_ids_cache,
                                    APP_MAX_TRANSFORM_SELECTION);
  if (selected_count == 0u)
    return false;
  return app_bounds_for_nodes_world(app, app->selection_ids_cache, selected_count,
                                    out_x, out_y, out_w, out_h);
}

static bool app_selected_bounds_view(EditorBootstrapApp *app, float *out_x,
                                     float *out_y, float *out_w, float *out_h) {
  float wx = 0.0f;
  float wy = 0.0f;
  float ww = 0.0f;
  float wh = 0.0f;
  float vx0 = 0.0f;
  float vy0 = 0.0f;
  float vx1 = 0.0f;
  float vy1 = 0.0f;
  float t;
  if (!app_selected_bounds_world(app, &wx, &wy, &ww, &wh))
    return false;
  stygian_editor_world_to_view(app->editor, wx, wy, &vx0, &vy0);
  stygian_editor_world_to_view(app->editor, wx + ww, wy + wh, &vx1, &vy1);
  if (vx1 < vx0) {
    t = vx0;
    vx0 = vx1;
    vx1 = t;
  }
  if (vy1 < vy0) {
    t = vy0;
    vy0 = vy1;
    vy1 = t;
  }
  if (out_x)
    *out_x = vx0;
  if (out_y)
    *out_y = vy0;
  if (out_w)
    *out_w = vx1 - vx0;
  if (out_h)
    *out_h = vy1 - vy0;
  return true;
}

static bool app_capture_drag_selection(EditorBootstrapApp *app,
                                       float mouse_world_x,
                                       float mouse_world_y) {
  uint32_t i;
  uint32_t selected_count;
  uint32_t valid_count = 0u;
  if (!app || !app->editor)
    return false;
  app->drag_selection_count = 0u;
  selected_count =
      stygian_editor_selected_nodes(app->editor, app->drag_selection_ids,
                                    APP_MAX_TRANSFORM_SELECTION);
  if (selected_count == 0u)
    return false;
  for (i = 0u; i < selected_count; ++i) {
    float nx = 0.0f;
    float ny = 0.0f;
    float nw = 0.0f;
    float nh = 0.0f;
    StygianEditorNodeId id = app->drag_selection_ids[i];
    if (!stygian_editor_node_get_bounds(app->editor, id, &nx, &ny, &nw, &nh))
      continue;
    app->drag_selection_ids[valid_count] = id;
    app->drag_selection_start_x[valid_count] = nx;
    app->drag_selection_start_y[valid_count] = ny;
    ++valid_count;
  }
  if (valid_count == 0u)
    return false;
  app->drag_selection_count = valid_count;
  app->drag_start_mouse_world_x = mouse_world_x;
  app->drag_start_mouse_world_y = mouse_world_y;
  return true;
}

static bool app_capture_resize_selection(EditorBootstrapApp *app,
                                         AppResizeHandle handle,
                                         float mouse_world_x,
                                         float mouse_world_y) {
  uint32_t i;
  uint32_t selected_count;
  uint32_t valid_count = 0u;
  bool have_group = false;
  float group_min_x = 0.0f;
  float group_min_y = 0.0f;
  float group_max_x = 0.0f;
  float group_max_y = 0.0f;
  if (!app || !app->editor || handle == APP_RESIZE_NONE)
    return false;
  app->resize_selection_count = 0u;
  selected_count =
      stygian_editor_selected_nodes(app->editor, app->resize_selection_ids,
                                    APP_MAX_TRANSFORM_SELECTION);
  if (selected_count == 0u)
    return false;
  for (i = 0u; i < selected_count; ++i) {
    float nx = 0.0f;
    float ny = 0.0f;
    float nw = 0.0f;
    float nh = 0.0f;
    StygianEditorNodeId id = app->resize_selection_ids[i];
    if (!stygian_editor_node_get_bounds(app->editor, id, &nx, &ny, &nw, &nh))
      continue;
    app->resize_selection_ids[valid_count] = id;
    app->resize_selection_start_x[valid_count] = nx;
    app->resize_selection_start_y[valid_count] = ny;
    app->resize_selection_start_w[valid_count] = nw;
    app->resize_selection_start_h[valid_count] = nh;
    if (!have_group) {
      group_min_x = nx;
      group_min_y = ny;
      group_max_x = nx + nw;
      group_max_y = ny + nh;
      have_group = true;
    } else {
      if (nx < group_min_x)
        group_min_x = nx;
      if (ny < group_min_y)
        group_min_y = ny;
      if (nx + nw > group_max_x)
        group_max_x = nx + nw;
      if (ny + nh > group_max_y)
        group_max_y = ny + nh;
    }
    ++valid_count;
  }
  if (!have_group || valid_count == 0u)
    return false;
  app->resize_selection_count = valid_count;
  app->resize_node_id = app->resize_selection_ids[0];
  app->resize_handle = handle;
  app->resize_start_x = group_min_x;
  app->resize_start_y = group_min_y;
  app->resize_start_w = group_max_x - group_min_x;
  app->resize_start_h = group_max_y - group_min_y;
  app->resize_start_mouse_world_x = mouse_world_x;
  app->resize_start_mouse_world_y = mouse_world_y;
  return true;
}

static uint32_t app_selected_nodes(EditorBootstrapApp *app,
                                   StygianEditorNodeId *out_ids,
                                   uint32_t max_ids) {
  if (!app || !app->editor || !out_ids || max_ids == 0u)
    return 0u;
  return stygian_editor_selected_nodes(app->editor, out_ids, max_ids);
}

static bool app_apply_group_translation(EditorBootstrapApp *app, float target_x,
                                        float target_y) {
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count;
  float gx = 0.0f;
  float gy = 0.0f;
  float gw = 0.0f;
  float gh = 0.0f;
  float dx;
  float dy;
  uint32_t i;
  bool mutated = false;
  if (!app || !app->editor)
    return false;
  count = app_selected_nodes(app, ids, APP_MAX_TRANSFORM_SELECTION);
  if (count == 0u)
    return false;
  if (!app_bounds_for_nodes_world(app, ids, count, &gx, &gy, &gw, &gh))
    return false;
  dx = target_x - gx;
  dy = target_y - gy;
  for (i = 0u; i < count; ++i) {
    float nx = 0.0f;
    float ny = 0.0f;
    float nw = 0.0f;
    float nh = 0.0f;
    if (!stygian_editor_node_get_bounds(app->editor, ids[i], &nx, &ny, &nw, &nh))
      continue;
    if (stygian_editor_node_set_position(app->editor, ids[i], nx + dx, ny + dy,
                                         false)) {
      mutated = true;
    }
  }
  return mutated;
}

static bool app_apply_group_resize(EditorBootstrapApp *app, float target_w,
                                   float target_h, bool affects_x,
                                   bool affects_y) {
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count;
  float gx = 0.0f;
  float gy = 0.0f;
  float gw = 0.0f;
  float gh = 0.0f;
  float scale_x;
  float scale_y;
  uint32_t i;
  bool mutated = false;
  if (!app || !app->editor)
    return false;
  count = app_selected_nodes(app, ids, APP_MAX_TRANSFORM_SELECTION);
  if (count == 0u)
    return false;
  if (!app_bounds_for_nodes_world(app, ids, count, &gx, &gy, &gw, &gh))
    return false;
  if (gw < 0.0001f)
    gw = 0.0001f;
  if (gh < 0.0001f)
    gh = 0.0001f;
  if (target_w < 1.0f)
    target_w = 1.0f;
  if (target_h < 1.0f)
    target_h = 1.0f;
  scale_x = target_w / gw;
  scale_y = target_h / gh;

  for (i = 0u; i < count; ++i) {
    StygianEditorNodeId node_id = ids[i];
    float sx = 0.0f;
    float sy = 0.0f;
    float sw = 0.0f;
    float sh = 0.0f;
    float node_scale_x = scale_x;
    float node_scale_y = scale_y;
    float nx;
    float ny;
    float nw;
    float nh;
    if (!stygian_editor_node_get_bounds(app->editor, node_id, &sx, &sy, &sw, &sh))
      continue;
    if (app_node_uniform_scale_enabled(app, node_id)) {
      float uniform_scale;
      if (affects_x && affects_y) {
        uniform_scale = (fabsf(scale_x - 1.0f) >= fabsf(scale_y - 1.0f))
                            ? scale_x
                            : scale_y;
      } else if (affects_x) {
        uniform_scale = scale_x;
      } else {
        uniform_scale = scale_y;
      }
      node_scale_x = uniform_scale;
      node_scale_y = uniform_scale;
    }
    nx = gx + (sx - gx) * node_scale_x;
    ny = gy + (sy - gy) * node_scale_y;
    nw = sw * node_scale_x;
    nh = sh * node_scale_y;
    if (nw < 1.0f)
      nw = 1.0f;
    if (nh < 1.0f)
      nh = 1.0f;
    if (stygian_editor_node_set_position(app->editor, node_id, nx, ny, false))
      mutated = true;
    if (stygian_editor_node_set_size(app->editor, node_id, nw, nh))
      mutated = true;
  }
  return mutated;
}

static bool app_apply_opacity_to_selection(EditorBootstrapApp *app,
                                           float opacity) {
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count;
  uint32_t i;
  bool mutated = false;
  if (!app || !app->editor)
    return false;
  count = app_selected_nodes(app, ids, APP_MAX_TRANSFORM_SELECTION);
  for (i = 0u; i < count; ++i) {
    if (stygian_editor_node_set_opacity(app->editor, ids[i], opacity))
      mutated = true;
  }
  return mutated;
}

static bool app_apply_color_to_selection(EditorBootstrapApp *app,
                                         StygianEditorColor color) {
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count;
  uint32_t i;
  bool mutated = false;
  if (!app || !app->editor)
    return false;
  count = app_selected_nodes(app, ids, APP_MAX_TRANSFORM_SELECTION);
  for (i = 0u; i < count; ++i) {
    if (stygian_editor_node_set_color(app->editor, ids[i], color))
      mutated = true;
  }
  return mutated;
}

static bool app_apply_corner_radii_to_selection(EditorBootstrapApp *app, float tl,
                                                float tr, float br, float bl) {
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count;
  uint32_t i;
  bool mutated = false;
  if (!app || !app->editor)
    return false;
  count = app_selected_nodes(app, ids, APP_MAX_TRANSFORM_SELECTION);
  for (i = 0u; i < count; ++i) {
    if (stygian_editor_node_set_corner_radii(app->editor, ids[i], tl, tr, br, bl))
      mutated = true;
  }
  return mutated;
}

static void app_resize_handle_center(AppResizeHandle handle, float x, float y,
                                     float w, float h, float *out_cx,
                                     float *out_cy) {
  float cx = x + w * 0.5f;
  float cy = y + h * 0.5f;
  switch (handle) {
  case APP_RESIZE_N:
    cx = x + w * 0.5f;
    cy = y;
    break;
  case APP_RESIZE_NE:
    cx = x + w;
    cy = y;
    break;
  case APP_RESIZE_E:
    cx = x + w;
    cy = y + h * 0.5f;
    break;
  case APP_RESIZE_SE:
    cx = x + w;
    cy = y + h;
    break;
  case APP_RESIZE_S:
    cx = x + w * 0.5f;
    cy = y + h;
    break;
  case APP_RESIZE_SW:
    cx = x;
    cy = y + h;
    break;
  case APP_RESIZE_W:
    cx = x;
    cy = y + h * 0.5f;
    break;
  case APP_RESIZE_NW:
    cx = x;
    cy = y;
    break;
  default:
    break;
  }
  if (out_cx)
    *out_cx = cx;
  if (out_cy)
    *out_cy = cy;
}

static AppResizeHandle app_resize_handle_at(EditorBootstrapApp *app,
                                            float mouse_x, float mouse_y) {
  static const AppResizeHandle handles[] = {
      APP_RESIZE_NW, APP_RESIZE_NE, APP_RESIZE_SW, APP_RESIZE_SE,
      APP_RESIZE_N,  APP_RESIZE_E,  APP_RESIZE_S,  APP_RESIZE_W};
  float vx = 0.0f;
  float vy = 0.0f;
  float vw = 0.0f;
  float vh = 0.0f;
  float hs = app_resize_handle_size_px();
  uint32_t i;
  if (!app || !app->editor)
    return APP_RESIZE_NONE;
  if (!app_selected_bounds_view(app, &vx, &vy, &vw, &vh))
    return APP_RESIZE_NONE;
  if (vw <= 0.0f || vh <= 0.0f)
    return APP_RESIZE_NONE;
  for (i = 0u; i < (uint32_t)(sizeof(handles) / sizeof(handles[0])); ++i) {
    float cx = 0.0f;
    float cy = 0.0f;
    app_resize_handle_center(handles[i], vx, vy, vw, vh, &cx, &cy);
    if (app_point_in_rect(mouse_x, mouse_y, cx - hs * 0.5f, cy - hs * 0.5f, hs,
                          hs)) {
      return handles[i];
    }
  }
  return APP_RESIZE_NONE;
}

static bool app_apply_resize_drag(EditorBootstrapApp *app, float mouse_x,
                                  float mouse_y) {
  float world_x = 0.0f;
  float world_y = 0.0f;
  float dx;
  float dy;
  float left;
  float top;
  float right;
  float bottom;
  float min_w = 8.0f;
  float min_h = 8.0f;
  float scale_x;
  float scale_y;
  float group_w;
  float group_h;
  bool handle_affects_x;
  bool handle_affects_y;
  bool touches_left = false;
  bool touches_top = false;
  bool mutated = false;
  uint32_t i;
  if (!app || !app->editor || !app->resizing_node ||
      app->resize_selection_count == 0u)
    return false;

  stygian_editor_view_to_world(app->editor, mouse_x, mouse_y, &world_x, &world_y);
  dx = world_x - app->resize_start_mouse_world_x;
  dy = world_y - app->resize_start_mouse_world_y;

  left = app->resize_start_x;
  top = app->resize_start_y;
  right = app->resize_start_x + app->resize_start_w;
  bottom = app->resize_start_y + app->resize_start_h;

  switch (app->resize_handle) {
  case APP_RESIZE_N:
    top += dy;
    touches_top = true;
    break;
  case APP_RESIZE_NE:
    top += dy;
    right += dx;
    touches_top = true;
    break;
  case APP_RESIZE_E:
    right += dx;
    break;
  case APP_RESIZE_SE:
    right += dx;
    bottom += dy;
    break;
  case APP_RESIZE_S:
    bottom += dy;
    break;
  case APP_RESIZE_SW:
    left += dx;
    bottom += dy;
    touches_left = true;
    break;
  case APP_RESIZE_W:
    left += dx;
    touches_left = true;
    break;
  case APP_RESIZE_NW:
    left += dx;
    top += dy;
    touches_left = true;
    touches_top = true;
    break;
  default:
    return false;
  }

  if (right - left < min_w) {
    if (touches_left)
      left = right - min_w;
    else
      right = left + min_w;
  }
  if (bottom - top < min_h) {
    if (touches_top)
      top = bottom - min_h;
    else
      bottom = top + min_h;
  }
  group_w = app->resize_start_w;
  group_h = app->resize_start_h;
  if (group_w < 0.0001f)
    group_w = 0.0001f;
  if (group_h < 0.0001f)
    group_h = 0.0001f;
  scale_x = (right - left) / group_w;
  scale_y = (bottom - top) / group_h;
  handle_affects_x =
      app->resize_handle != APP_RESIZE_N && app->resize_handle != APP_RESIZE_S;
  handle_affects_y =
      app->resize_handle != APP_RESIZE_E && app->resize_handle != APP_RESIZE_W;
  for (i = 0u; i < app->resize_selection_count; ++i) {
    StygianEditorNodeId node_id = app->resize_selection_ids[i];
    float start_x = app->resize_selection_start_x[i];
    float start_y = app->resize_selection_start_y[i];
    float start_w = app->resize_selection_start_w[i];
    float start_h = app->resize_selection_start_h[i];
    float node_scale_x = scale_x;
    float node_scale_y = scale_y;
    float next_x;
    float next_y;
    float next_w;
    float next_h;
    if (app_node_uniform_scale_enabled(app, node_id)) {
      float uniform_scale;
      if (handle_affects_x && handle_affects_y) {
        uniform_scale = (fabsf(scale_x - 1.0f) >= fabsf(scale_y - 1.0f))
                            ? scale_x
                            : scale_y;
      } else if (handle_affects_x) {
        uniform_scale = scale_x;
      } else {
        uniform_scale = scale_y;
      }
      node_scale_x = uniform_scale;
      node_scale_y = uniform_scale;
    }
    next_x = left + (start_x - app->resize_start_x) * node_scale_x;
    next_y = top + (start_y - app->resize_start_y) * node_scale_y;
    next_w = start_w * node_scale_x;
    next_h = start_h * node_scale_y;
    if (next_w < 1.0f)
      next_w = 1.0f;
    if (next_h < 1.0f)
      next_h = 1.0f;
    if (stygian_editor_node_set_position(app->editor, node_id, next_x, next_y,
                                         false)) {
      mutated = true;
    }
    if (stygian_editor_node_set_size(app->editor, node_id, next_w, next_h)) {
      mutated = true;
    }
  }
  return mutated;
}

static bool app_marquee_has_area(const EditorBootstrapApp *app) {
  float dx;
  float dy;
  if (!app)
    return false;
  dx = app->marquee_curr_view_x - app->marquee_start_view_x;
  dy = app->marquee_curr_view_y - app->marquee_start_view_y;
  return fabsf(dx) >= 3.0f || fabsf(dy) >= 3.0f;
}

static bool app_commit_marquee_selection(EditorBootstrapApp *app) {
  float x0w = 0.0f;
  float y0w = 0.0f;
  float x1w = 0.0f;
  float y1w = 0.0f;
  uint32_t selected_count;
  char status[96];
  if (!app || !app->editor || !app->marquee_selecting)
    return false;
  if (!app_marquee_has_area(app))
    return false;
  stygian_editor_view_to_world(app->editor, app->marquee_start_view_x,
                               app->marquee_start_view_y, &x0w, &y0w);
  stygian_editor_view_to_world(app->editor, app->marquee_curr_view_x,
                               app->marquee_curr_view_y, &x1w, &y1w);
  selected_count = stygian_editor_select_in_rect(app->editor, x0w, y0w, x1w, y1w,
                                                 app->marquee_additive);
  snprintf(status, sizeof(status), "Selection: %u item(s).", selected_count);
  app_set_status(app, status);
  return true;
}

static uint64_t app_host_now_ms(void *user_data) {
  const EditorBootstrapApp *app = (const EditorBootstrapApp *)user_data;
  if (!app)
    return 0u;
  return app->now_ms;
}

static void app_host_log(void *user_data, StygianEditorLogLevel level,
                         const char *message) {
  EditorBootstrapApp *app = (EditorBootstrapApp *)user_data;
  const char *prefix = "INFO";
  if (!app || !message)
    return;
  if (level == STYGIAN_EDITOR_LOG_DEBUG)
    prefix = "DEBUG";
  else if (level == STYGIAN_EDITOR_LOG_WARN)
    prefix = "WARN";
  else if (level == STYGIAN_EDITOR_LOG_ERROR)
    prefix = "ERROR";
  snprintf(app->status_text, sizeof(app->status_text), "[%s] %s", prefix,
           message);
}

static void app_host_request_repaint_hz(void *user_data, uint32_t hz) {
  EditorBootstrapApp *app = (EditorBootstrapApp *)user_data;
  if (!app || !app->ctx)
    return;
  stygian_request_repaint_hz(app->ctx, hz);
}

static void app_layout_compute(int width, int height, EditorLayout *out) {
  float top_h = 42.0f;
  float left_w = 254.0f;
  float right_w = 350.0f;
  float viewport_w;

  if (!out)
    return;

  if (width < 1220) {
    left_w = 232.0f;
    right_w = 312.0f;
  }
  viewport_w = (float)width - left_w - right_w;
  if (viewport_w < 320.0f) {
    float deficit = 320.0f - viewport_w;
    left_w = app_clampf(left_w - deficit * 0.45f, 180.0f, left_w);
    right_w = app_clampf(right_w - deficit * 0.55f, 220.0f, right_w);
    viewport_w = (float)width - left_w - right_w;
  }
  if (viewport_w < 220.0f)
    viewport_w = 220.0f;

  out->top_x = 0.0f;
  out->top_y = 0.0f;
  out->top_w = (float)width;
  out->top_h = top_h;

  out->left_x = 0.0f;
  out->left_y = top_h;
  out->left_w = left_w;
  out->left_h = (float)height - top_h;

  out->viewport_x = left_w;
  out->viewport_y = top_h;
  out->viewport_w = viewport_w;
  out->viewport_h = (float)height - top_h;

  out->right_x = left_w + viewport_w;
  out->right_y = top_h;
  out->right_w = right_w;
  out->right_h = (float)height - top_h;
}

static void app_sync_grid_config(EditorBootstrapApp *app) {
  StygianEditorGridConfig grid;
  if (!app || !app->editor)
    return;
  grid = stygian_editor_get_grid_config(app->editor);
  grid.enabled = app->grid_enabled;
  grid.sub_snap_enabled = app->sub_snap_enabled;
  grid.major_step_px = app->major_step_px;
  grid.sub_divisions = 4u;
  grid.min_minor_px = 7.0f;
  grid.snap_tolerance_px = app->snap_tolerance_px;
  stygian_editor_set_grid_config(app->editor, &grid);
}

static void app_sync_editor_viewport(EditorBootstrapApp *app) {
  StygianEditorViewport2D viewport;
  if (!app || !app->editor)
    return;
  // Bootstrap currently feeds global-space pointer coordinates into the editor.
  // Keep viewport extents in the same space so grid bounds cover the full panel.
  viewport.width = app->layout.viewport_x + app->layout.viewport_w;
  viewport.height = app->layout.viewport_y + app->layout.viewport_h;
  viewport.pan_x = app->camera_pan_x;
  viewport.pan_y = app->camera_pan_y;
  viewport.zoom = app_clampf(app->camera_zoom, 0.12f, 8.0f);
  app->camera_zoom = viewport.zoom;
  stygian_editor_set_viewport(app->editor, &viewport);
}

static bool app_ci_equals(const char *a, const char *b) {
  size_t i = 0;
  if (!a || !b)
    return false;
  while (a[i] && b[i]) {
    int ca = tolower((unsigned char)a[i]);
    int cb = tolower((unsigned char)b[i]);
    if (ca != cb)
      return false;
    ++i;
  }
  return a[i] == '\0' && b[i] == '\0';
}

static bool app_parse_u32(const char *text, uint32_t *out_value) {
  char *end = NULL;
  unsigned long value;
  if (!text || !text[0] || !out_value)
    return false;
  value = strtoul(text, &end, 10);
  if (end == text || *end != '\0')
    return false;
  if (value > 0xFFFFFFFFul)
    return false;
  *out_value = (uint32_t)value;
  return true;
}

static bool app_parse_u32_or_default(const char *text, uint32_t fallback,
                                     uint32_t *out_value) {
  if (!text || !text[0]) {
    *out_value = fallback;
    return true;
  }
  return app_parse_u32(text, out_value);
}

static bool app_parse_float_text(const char *text, float *out_value) {
  char *end = NULL;
  float value;
  if (!text || !text[0] || !out_value)
    return false;
  if (app_ci_equals(text, "nan")) {
    *out_value = NAN;
    return true;
  }
  value = strtof(text, &end);
  if (end == text || *end != '\0')
    return false;
  *out_value = value;
  return true;
}

static StygianEditorEventKind app_event_kind_from_choice(int choice) {
  if (choice == 1)
    return STYGIAN_EDITOR_EVENT_RELEASE;
  if (choice == 2)
    return STYGIAN_EDITOR_EVENT_VALUE_CHANGED;
  return STYGIAN_EDITOR_EVENT_PRESS;
}

static StygianEditorPropertyKind app_property_kind_from_choice(int choice) {
  if (choice == 1)
    return STYGIAN_EDITOR_PROP_Y;
  if (choice == 2)
    return STYGIAN_EDITOR_PROP_OPACITY;
  return STYGIAN_EDITOR_PROP_X;
}

static StygianEditorEasing app_easing_from_choice(int choice) {
  if (choice == 1)
    return STYGIAN_EDITOR_EASING_OUT_CUBIC;
  if (choice == 2)
    return STYGIAN_EDITOR_EASING_IN_OUT_CUBIC;
  return STYGIAN_EDITOR_EASING_LINEAR;
}

static const char *app_event_kind_name(StygianEditorEventKind kind) {
  switch (kind) {
  case STYGIAN_EDITOR_EVENT_PRESS:
    return "Press";
  case STYGIAN_EDITOR_EVENT_RELEASE:
    return "Release";
  case STYGIAN_EDITOR_EVENT_HOVER_ENTER:
    return "Hover Enter";
  case STYGIAN_EDITOR_EVENT_HOVER_LEAVE:
    return "Hover Leave";
  case STYGIAN_EDITOR_EVENT_DRAG_START:
    return "Drag Start";
  case STYGIAN_EDITOR_EVENT_DRAG_MOVE:
    return "Drag Move";
  case STYGIAN_EDITOR_EVENT_DRAG_END:
    return "Drag End";
  case STYGIAN_EDITOR_EVENT_SCROLL:
    return "Scroll";
  case STYGIAN_EDITOR_EVENT_VALUE_CHANGED:
    return "Value Changed";
  default:
    return "Unknown";
  }
}

static const char *app_property_kind_name(StygianEditorPropertyKind kind) {
  switch (kind) {
  case STYGIAN_EDITOR_PROP_X:
    return "X";
  case STYGIAN_EDITOR_PROP_Y:
    return "Y";
  case STYGIAN_EDITOR_PROP_WIDTH:
    return "Width";
  case STYGIAN_EDITOR_PROP_HEIGHT:
    return "Height";
  case STYGIAN_EDITOR_PROP_OPACITY:
    return "Opacity";
  case STYGIAN_EDITOR_PROP_RADIUS_TL:
    return "Radius TL";
  case STYGIAN_EDITOR_PROP_RADIUS_TR:
    return "Radius TR";
  case STYGIAN_EDITOR_PROP_RADIUS_BR:
    return "Radius BR";
  case STYGIAN_EDITOR_PROP_RADIUS_BL:
    return "Radius BL";
  case STYGIAN_EDITOR_PROP_VALUE:
    return "Value";
  default:
    return "Unknown";
  }
}

static const char *app_easing_name(StygianEditorEasing easing) {
  switch (easing) {
  case STYGIAN_EDITOR_EASING_LINEAR:
    return "Linear";
  case STYGIAN_EDITOR_EASING_OUT_CUBIC:
    return "Out Cubic";
  case STYGIAN_EDITOR_EASING_IN_OUT_CUBIC:
    return "InOut Cubic";
  default:
    return "Unknown";
  }
}

static bool app_write_generated_c23(EditorBootstrapApp *app,
                                    const char *output_path) {
  size_t required;
  char *buffer;
  FILE *file;
  if (!app || !app->editor || !output_path)
    return false;

  required = stygian_editor_build_c23(app->editor, NULL, 0u);
  if (required < 2u)
    return false;

  buffer = (char *)malloc(required);
  if (!buffer)
    return false;

  (void)stygian_editor_build_c23(app->editor, buffer, required);

  file = fopen(output_path, "wb");
  if (!file) {
    free(buffer);
    return false;
  }

  fwrite(buffer, 1u, strlen(buffer), file);
  fclose(file);
  free(buffer);
  return true;
}

static void app_add_primitive_at_cursor(EditorBootstrapApp *app,
                                        AppPrimitiveKind primitive, int mouse_x,
                                        int mouse_y) {
  float world_x;
  float world_y;
  float shape_w = 140.0f;
  float shape_h = 90.0f;
  bool is_ellipse = false;
  StygianEditorNodeId id = STYGIAN_EDITOR_INVALID_ID;

  if (!app || !app->editor)
    return;

  if (app_point_in_rect((float)mouse_x, (float)mouse_y, app->layout.viewport_x,
                        app->layout.viewport_y, app->layout.viewport_w,
                        app->layout.viewport_h)) {
    stygian_editor_view_to_world(app->editor, (float)mouse_x, (float)mouse_y,
                                 &world_x, &world_y);
  } else {
    float center_x = app->layout.viewport_x + app->layout.viewport_w * 0.5f;
    float center_y = app->layout.viewport_y + app->layout.viewport_h * 0.5f;
    stygian_editor_view_to_world(app->editor, center_x, center_y, &world_x,
                                 &world_y);
  }

  switch (primitive) {
  case APP_PRIMITIVE_SQUARE:
    shape_w = 100.0f;
    shape_h = 100.0f;
    is_ellipse = false;
    break;
  case APP_PRIMITIVE_CIRCLE:
    shape_w = 100.0f;
    shape_h = 100.0f;
    is_ellipse = true;
    break;
  case APP_PRIMITIVE_RECTANGLE:
    shape_w = 140.0f;
    shape_h = 90.0f;
    is_ellipse = false;
    break;
  case APP_PRIMITIVE_ELLIPSE:
    shape_w = 140.0f;
    shape_h = 90.0f;
    is_ellipse = true;
    break;
  default:
    shape_w = 140.0f;
    shape_h = 90.0f;
    is_ellipse = false;
    break;
  }

  world_x -= shape_w * 0.5f;
  world_y -= shape_h * 0.5f;

  if (!is_ellipse) {
    StygianEditorRectDesc rect = {0};
    rect.x = world_x;
    rect.y = world_y;
    rect.w = shape_w;
    rect.h = shape_h;
    if (primitive == APP_PRIMITIVE_SQUARE) {
      rect.radius[0] = 6.0f;
      rect.radius[1] = 6.0f;
      rect.radius[2] = 6.0f;
      rect.radius[3] = 6.0f;
    } else {
      rect.radius[0] = 12.0f;
      rect.radius[1] = 12.0f;
      rect.radius[2] = 12.0f;
      rect.radius[3] = 12.0f;
    }
    rect.fill = app->paint_color;
    rect.visible = true;
    rect.z = 0.20f;
    id = stygian_editor_add_rect(app->editor, &rect);
  } else {
    StygianEditorEllipseDesc ellipse_desc = {0};
    ellipse_desc.x = world_x;
    ellipse_desc.y = world_y;
    ellipse_desc.w = shape_w;
    ellipse_desc.h = shape_h;
    ellipse_desc.fill = app->paint_color;
    ellipse_desc.visible = true;
    ellipse_desc.z = 0.20f;
    id = stygian_editor_add_ellipse(app->editor, &ellipse_desc);
  }

  if (id != STYGIAN_EDITOR_INVALID_ID) {
    char status[96];
    bool uniform_default = false;
    if (primitive == APP_PRIMITIVE_CIRCLE || primitive == APP_PRIMITIVE_ELLIPSE)
      uniform_default = true;
    app_set_node_uniform_scale(app, id, uniform_default);
    stygian_editor_select_node(app->editor, id, false);
    snprintf(status, sizeof(status), "%s added.", app_primitive_name(primitive));
    app_set_status(app, status);
  } else {
    app_set_status(app, "Primitive add failed (capacity or invalid state).");
  }
}

static void app_add_shape_at_cursor(EditorBootstrapApp *app, bool ellipse,
                                    int mouse_x, int mouse_y) {
  app_add_primitive_at_cursor(app,
                              ellipse ? APP_PRIMITIVE_ELLIPSE
                                      : APP_PRIMITIVE_RECTANGLE,
                              mouse_x, mouse_y);
}

static void app_add_selected_primitive_at_cursor(EditorBootstrapApp *app,
                                                 int mouse_x, int mouse_y) {
  if (!app)
    return;
  app_add_primitive_at_cursor(app, app->selected_primitive, mouse_x, mouse_y);
}

static void app_drop_selected_primitive_in_viewport(EditorBootstrapApp *app,
                                                    int preferred_x,
                                                    int preferred_y) {
  int px;
  int py;
  int min_x;
  int max_x;
  int min_y;
  int max_y;
  if (!app)
    return;
  min_x = (int)(app->layout.viewport_x + 24.0f);
  max_x = (int)(app->layout.viewport_x + app->layout.viewport_w - 24.0f);
  min_y = (int)(app->layout.viewport_y + 24.0f);
  max_y = (int)(app->layout.viewport_y + app->layout.viewport_h - 24.0f);
  if (max_x < min_x)
    max_x = min_x;
  if (max_y < min_y)
    max_y = min_y;
  px = preferred_x;
  py = preferred_y;
  if (!app_point_in_rect((float)preferred_x, (float)preferred_y,
                         app->layout.viewport_x, app->layout.viewport_y,
                         app->layout.viewport_w, app->layout.viewport_h)) {
    px = (int)(app->layout.viewport_x + app->layout.viewport_w * 0.5f);
    py = (int)(app->layout.viewport_y + app->layout.viewport_h * 0.5f);
  }
  if (px < min_x)
    px = min_x;
  if (px > max_x)
    px = max_x;
  if (py < min_y)
    py = min_y;
  if (py > max_y)
    py = max_y;
  app_add_selected_primitive_at_cursor(app, px, py);
}

static bool app_start_path_tool(EditorBootstrapApp *app) {
  StygianEditorPathDesc path = {0};
  if (!app || !app->editor || app->path_tool_active)
    return false;

  path.stroke = app->paint_color;
  path.thickness = 2.0f;
  path.closed = false;
  path.visible = true;
  path.z = 0.25f;
  app->active_path = stygian_editor_path_begin(app->editor, &path);
  app->path_tool_active = (app->active_path != STYGIAN_EDITOR_INVALID_ID);
  app->path_preview_count = 0u;
  if (app->path_tool_active) {
    app_set_status(app,
                   "Path tool active. Left click in viewport to place points.");
  }
  return app->path_tool_active;
}

static void app_commit_path_tool(EditorBootstrapApp *app) {
  StygianEditorNodeId node;
  if (!app || !app->editor || !app->path_tool_active)
    return;
  node = stygian_editor_path_commit(app->editor, app->active_path);
  if (node != STYGIAN_EDITOR_INVALID_ID) {
    stygian_editor_select_node(app->editor, node, false);
    app_set_status(app, "Path committed.");
  } else {
    app_set_status(app, "Path commit failed (need 2+ points).");
  }
  app->path_tool_active = false;
  app->active_path = STYGIAN_EDITOR_INVALID_ID;
  app->path_preview_count = 0u;
}

static void app_cancel_path_tool(EditorBootstrapApp *app) {
  if (!app || !app->editor || !app->path_tool_active)
    return;
  stygian_editor_path_cancel(app->editor, app->active_path);
  app->path_tool_active = false;
  app->active_path = STYGIAN_EDITOR_INVALID_ID;
  app->path_preview_count = 0u;
  app_set_status(app, "Path tool cancelled.");
}

static void app_seed_demo(EditorBootstrapApp *app) {
  StygianEditorRectDesc slider = {0};
  StygianEditorRectDesc box = {0};
  StygianEditorBehaviorRule rule = {0};

  if (!app || !app->editor)
    return;

  slider.x = 70.0f;
  slider.y = 470.0f;
  slider.w = 260.0f;
  slider.h = 24.0f;
  slider.radius[0] = 12.0f;
  slider.radius[1] = 12.0f;
  slider.radius[2] = 12.0f;
  slider.radius[3] = 12.0f;
  slider.fill = stygian_editor_color_rgba(0.25f, 0.29f, 0.35f, 1.0f);
  slider.visible = true;
  slider.z = 0.10f;
  app->demo_slider_node = stygian_editor_add_rect(app->editor, &slider);

  box.x = 430.0f;
  box.y = 470.0f;
  box.w = 180.0f;
  box.h = 110.0f;
  box.radius[0] = 14.0f;
  box.radius[1] = 14.0f;
  box.radius[2] = 14.0f;
  box.radius[3] = 14.0f;
  box.fill = stygian_editor_color_rgba(0.13f, 0.57f, 0.93f, 1.0f);
  box.visible = true;
  box.z = 0.20f;
  app->demo_box_node = stygian_editor_add_rect(app->editor, &box);

  if (app->demo_slider_node != STYGIAN_EDITOR_INVALID_ID &&
      app->demo_box_node != STYGIAN_EDITOR_INVALID_ID) {
    rule.trigger_node = app->demo_slider_node;
    rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
    rule.action_kind = STYGIAN_EDITOR_ACTION_ANIMATE;
    rule.animate.target_node = app->demo_box_node;
    rule.animate.property = STYGIAN_EDITOR_PROP_Y;
    rule.animate.from_value = NAN;
    rule.animate.to_value = 60.0f;
    rule.animate.duration_ms = 350u;
    rule.animate.easing = STYGIAN_EDITOR_EASING_OUT_CUBIC;
    (void)stygian_editor_add_behavior(app->editor, &rule, NULL);
  }

  snprintf(app->behavior_trigger_id, sizeof(app->behavior_trigger_id), "%u",
           app->demo_slider_node);
  snprintf(app->behavior_target_id, sizeof(app->behavior_target_id), "%u",
           app->demo_box_node);
  snprintf(app->behavior_from_value, sizeof(app->behavior_from_value), "nan");
  snprintf(app->behavior_to_value, sizeof(app->behavior_to_value), "60");
  snprintf(app->behavior_duration_ms, sizeof(app->behavior_duration_ms), "350");
  app_set_status(app,
                 "Bootstrap ready. Demo slider press animates the blue box.");
}

static void app_camera_zoom_about(EditorBootstrapApp *app, float focus_x,
                                  float focus_y, float new_zoom) {
  float world_x;
  float world_y;
  float old_zoom;
  if (!app)
    return;

  old_zoom = app->camera_zoom;
  new_zoom = app_clampf(new_zoom, 0.12f, 8.0f);
  if (fabsf(new_zoom - old_zoom) < 0.0001f)
    return;

  world_x = (focus_x - app->camera_pan_x) / old_zoom;
  world_y = (focus_y - app->camera_pan_y) / old_zoom;

  app->camera_zoom = new_zoom;
  app->camera_pan_x = focus_x - world_x * new_zoom;
  app->camera_pan_y = focus_y - world_y * new_zoom;
  app_sync_editor_viewport(app);
}

static bool app_handle_viewport_event(EditorBootstrapApp *app,
                                      const StygianEvent *event) {
  float mx;
  float my;
  bool mutated = false;

  if (!app || !event)
    return false;

  if (event->type == STYGIAN_EVENT_KEY_DOWN) {
    float focus_x = app->layout.viewport_x + app->layout.viewport_w * 0.5f;
    float focus_y = app->layout.viewport_y + app->layout.viewport_h * 0.5f;
    if (event->key.key == STYGIAN_KEY_EQUALS) {
      app_camera_zoom_about(app, focus_x, focus_y, app->camera_zoom * 1.15f);
      return true;
    }
    if (event->key.key == STYGIAN_KEY_MINUS) {
      app_camera_zoom_about(app, focus_x, focus_y, app->camera_zoom / 1.15f);
      return true;
    }
    if (event->key.key == STYGIAN_KEY_0) {
      app_camera_zoom_about(app, focus_x, focus_y, 1.0f);
      return true;
    }
    if (event->key.key == STYGIAN_KEY_ESCAPE) {
      if (app->file_menu_open || app->primitive_menu_open) {
        app->file_menu_open = false;
        app->primitive_menu_open = false;
        return true;
      }
      if (app->marquee_selecting) {
        app->marquee_selecting = false;
        app->marquee_additive = false;
        return true;
      }
      if (app->path_tool_active) {
        app_cancel_path_tool(app);
        return true;
      }
    }
    return false;
  }

  if (event->type == STYGIAN_EVENT_MOUSE_DOWN ||
      event->type == STYGIAN_EVENT_MOUSE_UP) {
    mx = (float)event->mouse_button.x;
    my = (float)event->mouse_button.y;
  } else if (event->type == STYGIAN_EVENT_MOUSE_MOVE) {
    mx = (float)event->mouse_move.x;
    my = (float)event->mouse_move.y;
  } else if (event->type == STYGIAN_EVENT_SCROLL) {
    mx = (float)event->scroll.x;
    my = (float)event->scroll.y;
  } else {
    return false;
  }

  if (event->type == STYGIAN_EVENT_MOUSE_DOWN &&
      event->mouse_button.button == STYGIAN_MOUSE_LEFT &&
      (app->file_menu_open || app->primitive_menu_open) &&
      app_point_in_rect(mx, my, app->layout.viewport_x, app->layout.viewport_y,
                        app->layout.viewport_w, app->layout.viewport_h)) {
    bool clicked_menu_surface = false;
    if (app->primitive_menu_open) {
      float pmx = 0.0f;
      float pmy = 0.0f;
      float pmw = 0.0f;
      float pmrh = 0.0f;
      float pmh = 0.0f;
      app_primitive_menu_geometry(app, &pmx, &pmy, &pmw, &pmrh, &pmh);
      if (app_point_in_rect(mx, my, pmx, pmy, pmw, pmh))
        clicked_menu_surface = true;
    }

    // Important: primitive menu is drawn in viewport space.
    // Do not auto-close it when user clicks an item.
    if (!clicked_menu_surface) {
      app->file_menu_open = false;
      app->primitive_menu_open = false;
      mutated = true;
    }
  }

  if (event->type == STYGIAN_EVENT_MOUSE_DOWN &&
      event->mouse_button.button == STYGIAN_MOUSE_MIDDLE &&
      app_point_in_rect(mx, my, app->layout.viewport_x, app->layout.viewport_y,
                        app->layout.viewport_w, app->layout.viewport_h)) {
    app->panning = true;
    return false;
  }
  if (event->type == STYGIAN_EVENT_MOUSE_UP &&
      event->mouse_button.button == STYGIAN_MOUSE_MIDDLE) {
    app->panning = false;
    return false;
  }

  if (event->type == STYGIAN_EVENT_MOUSE_MOVE && app->panning) {
    app->camera_pan_x += (float)event->mouse_move.dx;
    app->camera_pan_y += (float)event->mouse_move.dy;
    app_sync_editor_viewport(app);
    return true;
  }

  if (event->type == STYGIAN_EVENT_SCROLL &&
      app_point_in_rect(mx, my, app->layout.viewport_x, app->layout.viewport_y,
                        app->layout.viewport_w, app->layout.viewport_h)) {
    float scale = powf(1.12f, event->scroll.dy);
    if (scale < 0.05f)
      scale = 0.05f;
    app_camera_zoom_about(app, mx, my, app->camera_zoom * scale);
    return true;
  }

  if (event->type == STYGIAN_EVENT_MOUSE_MOVE && app->primitive_menu_open)
    return true;

  if (event->type == STYGIAN_EVENT_MOUSE_MOVE && app->marquee_selecting) {
    app->marquee_curr_view_x = mx;
    app->marquee_curr_view_y = my;
    return true;
  }

  if (event->type == STYGIAN_EVENT_MOUSE_MOVE && app->resizing_node &&
      app->resize_selection_count > 0u) {
    return app_apply_resize_drag(app, mx, my);
  }

  if (event->type == STYGIAN_EVENT_MOUSE_MOVE && app->dragging_node &&
      app->drag_selection_count > 0u) {
    float world_x = 0.0f;
    float world_y = 0.0f;
    float dx = 0.0f;
    float dy = 0.0f;
    bool moved = false;
    uint32_t i;
    stygian_editor_view_to_world(app->editor, mx, my, &world_x, &world_y);
    dx = world_x - app->drag_start_mouse_world_x;
    dy = world_y - app->drag_start_mouse_world_y;
    for (i = 0u; i < app->drag_selection_count; ++i) {
      if (stygian_editor_node_set_position(
              app->editor, app->drag_selection_ids[i],
              app->drag_selection_start_x[i] + dx,
              app->drag_selection_start_y[i] + dy, true)) {
        moved = true;
      }
    }
    return moved;
  }

  if (event->type == STYGIAN_EVENT_MOUSE_UP &&
      event->mouse_button.button == STYGIAN_MOUSE_LEFT) {
    bool was_resizing = app->resizing_node;
    bool was_dragging = app->dragging_node;
    bool was_marquee = app->marquee_selecting;
    bool marquee_mutated = false;
    if (app->marquee_selecting)
      marquee_mutated = app_commit_marquee_selection(app);
    app->dragging_node = false;
    app->drag_node_id = STYGIAN_EDITOR_INVALID_ID;
    app->drag_selection_count = 0u;
    app->resizing_node = false;
    app->resize_node_id = STYGIAN_EDITOR_INVALID_ID;
    app->resize_handle = APP_RESIZE_NONE;
    app->resize_selection_count = 0u;
    app->marquee_selecting = false;
    app->marquee_additive = false;
    app->marquee_start_view_x = 0.0f;
    app->marquee_start_view_y = 0.0f;
    app->marquee_curr_view_x = 0.0f;
    app->marquee_curr_view_y = 0.0f;
    return was_resizing || was_dragging || was_marquee || marquee_mutated;
  }

  if (!app_point_in_rect(mx, my, app->layout.viewport_x, app->layout.viewport_y,
                         app->layout.viewport_w, app->layout.viewport_h)) {
    return false;
  }

  if (event->type == STYGIAN_EVENT_MOUSE_MOVE && app->path_tool_active)
    return true;

  if (event->type == STYGIAN_EVENT_MOUSE_DOWN &&
      event->mouse_button.button == STYGIAN_MOUSE_LEFT) {
    float world_x;
    float world_y;
    stygian_editor_view_to_world(app->editor, mx, my, &world_x, &world_y);

    if (app->path_tool_active) {
      float snapped_x = world_x;
      float snapped_y = world_y;
      stygian_editor_snap_world_point(app->editor, world_x, world_y, &snapped_x,
                                      &snapped_y);
      if (stygian_editor_path_add_point(app->editor, app->active_path, world_x,
                                        world_y, true)) {
        if (app->path_preview_count <
            (uint32_t)(sizeof(app->path_preview_points) /
                       sizeof(app->path_preview_points[0]))) {
          app->path_preview_points[app->path_preview_count].x = snapped_x;
          app->path_preview_points[app->path_preview_count].y = snapped_y;
          app->path_preview_count += 1u;
        }
        mutated = true;
        app_set_status(app, "Path point added.");
      }
      if (event->mouse_button.clicks >= 2) {
        app_commit_path_tool(app);
        mutated = true;
      }
      return mutated;
    }

    {
      AppResizeHandle handle_hit = app_resize_handle_at(app, mx, my);
      if (handle_hit != APP_RESIZE_NONE) {
        if (app_capture_resize_selection(app, handle_hit, world_x, world_y)) {
          app->resizing_node = true;
          app->dragging_node = false;
          app->drag_node_id = STYGIAN_EDITOR_INVALID_ID;
          app->drag_selection_count = 0u;
          app_set_status(app, "Resize handle active.");
          return true;
        }
      }
    }

    {
      bool shift_additive = (event->mouse_button.mods & STYGIAN_MOD_SHIFT) != 0u;
      uint32_t selected_before = stygian_editor_selected_count(app->editor);
      uint32_t selected_after;
      StygianEditorNodeId hit_node =
          stygian_editor_hit_test_at(app->editor, world_x, world_y);
      if (!shift_additive && hit_node != STYGIAN_EDITOR_INVALID_ID &&
          stygian_editor_node_is_selected(app->editor, hit_node) &&
          selected_before > 0u) {
        app->drag_node_id = hit_node;
        app->dragging_node = app_capture_drag_selection(app, world_x, world_y);
        app->resizing_node = false;
        app->resize_node_id = STYGIAN_EDITOR_INVALID_ID;
        app->resize_handle = APP_RESIZE_NONE;
        app->resize_selection_count = 0u;
        app->marquee_selecting = false;
        app->marquee_additive = false;
        if (app->dragging_node)
          return true;
      }

      app->drag_node_id =
          stygian_editor_select_at(app->editor, world_x, world_y, shift_additive);
      selected_after = stygian_editor_selected_count(app->editor);
      if (selected_after != selected_before)
        mutated = true;

      if (app->drag_node_id != STYGIAN_EDITOR_INVALID_ID && !shift_additive) {
        app->dragging_node = app_capture_drag_selection(app, world_x, world_y);
        app->resizing_node = false;
        app->resize_node_id = STYGIAN_EDITOR_INVALID_ID;
        app->resize_handle = APP_RESIZE_NONE;
        app->resize_selection_count = 0u;
        app->marquee_selecting = false;
        app->marquee_additive = false;
        if (app->dragging_node)
          return true;
      }

      // Shift+click toggles selection without dragging.
      if (shift_additive && selected_after != selected_before)
        return true;

      app->dragging_node = false;
      app->drag_node_id = STYGIAN_EDITOR_INVALID_ID;
      app->drag_selection_count = 0u;
      app->resizing_node = false;
      app->resize_node_id = STYGIAN_EDITOR_INVALID_ID;
      app->resize_handle = APP_RESIZE_NONE;
      app->resize_selection_count = 0u;

      // Blank-canvas drag starts marquee selection.
      app->marquee_selecting = true;
      app->marquee_additive = shift_additive;
      app->marquee_start_view_x = mx;
      app->marquee_start_view_y = my;
      app->marquee_curr_view_x = mx;
      app->marquee_curr_view_y = my;
      return true;
    }
  }

  if (event->type == STYGIAN_EVENT_MOUSE_DOWN &&
      event->mouse_button.button == STYGIAN_MOUSE_RIGHT &&
      app->path_tool_active) {
    app_commit_path_tool(app);
    return true;
  }

  return false;
}

static bool app_draw_left_panel(EditorBootstrapApp *app, int mouse_x,
                                int mouse_y) {
  float x = app->layout.left_x + 12.0f;
  float y = app->layout.left_y + 10.0f;
  float w = app->layout.left_w - 24.0f;
  bool mutated = false;

  stygian_rect(app->ctx, app->layout.left_x, app->layout.left_y, app->layout.left_w,
               app->layout.left_h, 0.10f, 0.12f, 0.15f, 1.0f);

  if (app->font) {
    stygian_text(app->ctx, app->font, "Tools", x, y, 17.0f, 0.93f, 0.95f, 1.0f,
                 1.0f);
  }
  y += 26.0f;

  if (stygian_button(app->ctx, app->font, "Add Rectangle", x, y, w, 28.0f)) {
    app_add_shape_at_cursor(app, false, mouse_x, mouse_y);
    mutated = true;
  }
  y += 34.0f;
  if (stygian_button(app->ctx, app->font, "Add Ellipse", x, y, w, 28.0f)) {
    app_add_shape_at_cursor(app, true, mouse_x, mouse_y);
    mutated = true;
  }
  y += 34.0f;

  if (!app->path_tool_active) {
    if (stygian_button(app->ctx, app->font, "Start Path Tool", x, y, w, 28.0f)) {
      mutated = app_start_path_tool(app) || mutated;
    }
    y += 34.0f;
  } else {
    if (stygian_button(app->ctx, app->font, "Commit Path", x, y, w, 28.0f)) {
      app_commit_path_tool(app);
      mutated = true;
    }
    y += 34.0f;
    if (stygian_button(app->ctx, app->font, "Cancel Path", x, y, w, 28.0f)) {
      app_cancel_path_tool(app);
      mutated = true;
    }
    y += 34.0f;
  }

  if (stygian_checkbox(app->ctx, app->font, "Grid + Snap", x, y,
                       &app->grid_enabled)) {
    app_sync_grid_config(app);
    mutated = true;
  }
  y += 26.0f;
  if (stygian_checkbox(app->ctx, app->font, "Sub Snap", x, y,
                       &app->sub_snap_enabled)) {
    app_sync_grid_config(app);
    mutated = true;
  }
  y += 28.0f;

  if (app->font) {
    char zoom_label[48];
    snprintf(zoom_label, sizeof(zoom_label), "Zoom %.2fx", app->camera_zoom);
    stygian_text(app->ctx, app->font, zoom_label, x, y, 14.0f, 0.85f, 0.90f,
                 0.98f, 1.0f);
  }
  y += 20.0f;
  {
    float zoom = app->camera_zoom;
    if (stygian_slider(app->ctx, x, y, w, 16.0f, &zoom, 0.12f, 5.0f)) {
      float focus_x = app->layout.viewport_x + app->layout.viewport_w * 0.5f;
      float focus_y = app->layout.viewport_y + app->layout.viewport_h * 0.5f;
      app_camera_zoom_about(app, focus_x, focus_y, zoom);
      mutated = true;
    }
  }
  y += 30.0f;

  if (app->font) {
    stygian_text(app->ctx, app->font, "Major Grid Density", x, y, 13.0f, 0.8f,
                 0.85f, 0.93f, 1.0f);
  }
  y += 16.0f;
  if (stygian_slider(app->ctx, x, y, w, 14.0f, &app->major_step_px, 36.0f,
                     180.0f)) {
    app_sync_grid_config(app);
    mutated = true;
  }
  y += 26.0f;

  if (stygian_button(app->ctx, app->font, "Build C23 -> build/stygian_editor_generated.c",
                     x, y, w, 28.0f)) {
    if (app_write_generated_c23(app, "build/stygian_editor_generated.c")) {
      app_set_status(app,
                     "Generated build/stygian_editor_generated.c successfully.");
    } else {
      app_set_status(app, "Failed to write generated C23 output.");
    }
  }
  y += 38.0f;

  if (stygian_button(app->ctx, app->font, "Save Dock Layout", x, y, w * 0.48f,
                     24.0f)) {
    if (app->dock &&
        stygian_dock_save(app->dock, "build/stygian_editor_layout.json")) {
      app_set_status(app, "Saved build/stygian_editor_layout.json");
    } else {
      app_set_status(app, "Dock layout save failed.");
    }
  }
  if (stygian_button(app->ctx, app->font, "Load Dock Layout", x + w * 0.52f, y,
                     w * 0.48f, 24.0f)) {
    if (app->dock &&
        stygian_dock_load(app->dock, "build/stygian_editor_layout.json")) {
      app_set_status(app, "Loaded build/stygian_editor_layout.json");
      mutated = true;
    } else {
      app_set_status(app, "Dock layout load failed.");
    }
  }
  y += 32.0f;

  if (app->font) {
    stygian_text(app->ctx, app->font, "Color Suite", x, y, 16.0f, 0.93f, 0.95f,
                 1.0f, 1.0f);
  }
  y += 24.0f;

  if (app->font)
    stygian_text(app->ctx, app->font, "R", x, y, 14.0f, 0.95f, 0.50f, 0.50f,
                 1.0f);
  if (stygian_slider(app->ctx, x + 16.0f, y, w - 16.0f, 14.0f, &app->paint_color.r,
                     0.0f, 1.0f)) {
    mutated = true;
  }
  y += 20.0f;
  if (app->font)
    stygian_text(app->ctx, app->font, "G", x, y, 14.0f, 0.50f, 0.95f, 0.50f,
                 1.0f);
  if (stygian_slider(app->ctx, x + 16.0f, y, w - 16.0f, 14.0f, &app->paint_color.g,
                     0.0f, 1.0f)) {
    mutated = true;
  }
  y += 20.0f;
  if (app->font)
    stygian_text(app->ctx, app->font, "B", x, y, 14.0f, 0.50f, 0.75f, 1.0f,
                 1.0f);
  if (stygian_slider(app->ctx, x + 16.0f, y, w - 16.0f, 14.0f, &app->paint_color.b,
                     0.0f, 1.0f)) {
    mutated = true;
  }
  y += 20.0f;
  if (app->font)
    stygian_text(app->ctx, app->font, "A", x, y, 14.0f, 0.95f, 0.95f, 0.95f,
                 1.0f);
  if (stygian_slider(app->ctx, x + 16.0f, y, w - 16.0f, 14.0f, &app->paint_color.a,
                     0.0f, 1.0f)) {
    mutated = true;
  }
  y += 24.0f;

  stygian_rect_rounded(app->ctx, x, y, w, 20.0f, app->paint_color.r,
                       app->paint_color.g, app->paint_color.b, app->paint_color.a,
                       4.0f);
  y += 28.0f;

  stygian_text_input(app->ctx, app->font, x, y, w, 24.0f, app->token_name,
                     (int)sizeof(app->token_name));
  y += 30.0f;
  if (stygian_button(app->ctx, app->font, "Save Token", x, y, w * 0.48f, 24.0f)) {
    if (stygian_editor_set_color_token(app->editor, app->token_name,
                                       app->paint_color)) {
      app_set_status(app, "Color token saved.");
    } else {
      app_set_status(app, "Failed to save color token.");
    }
  }
  if (stygian_button(app->ctx, app->font, "Apply Token", x + w * 0.52f, y,
                     w * 0.48f, 24.0f)) {
    StygianEditorNodeId selected = stygian_editor_selected_node(app->editor);
    if (selected != STYGIAN_EDITOR_INVALID_ID &&
        stygian_editor_apply_color_token(app->editor, selected, app->token_name)) {
      app_set_status(app, "Color token applied.");
      mutated = true;
    } else {
      app_set_status(app, "Apply token failed: select node + valid token.");
    }
  }

  return mutated;
}

static bool app_draw_right_panel(EditorBootstrapApp *app) {
  float x = app->layout.right_x + 12.0f;
  float y = app->layout.right_y + 10.0f;
  float w = app->layout.right_w - 24.0f;
  bool mutated = false;
  StygianEditorNodeId selected = stygian_editor_selected_node(app->editor);

  stygian_rect(app->ctx, app->layout.right_x, app->layout.right_y,
               app->layout.right_w, app->layout.right_h, 0.10f, 0.12f, 0.15f,
               1.0f);

  if (app->font) {
    stygian_text(app->ctx, app->font, "Inspector", x, y, 17.0f, 0.93f, 0.95f,
                 1.0f, 1.0f);
  }
  y += 24.0f;

  if (selected == STYGIAN_EDITOR_INVALID_ID) {
    if (app->font) {
      stygian_text(app->ctx, app->font, "No node selected.", x, y, 14.0f, 0.8f,
                   0.83f, 0.90f, 1.0f);
    }
    y += 20.0f;
  } else {
    float nx = 0.0f, ny = 0.0f, nw = 0.0f, nh = 0.0f;
    char id_text[64];
    bool have_bounds =
        stygian_editor_node_get_bounds(app->editor, selected, &nx, &ny, &nw, &nh);

    snprintf(id_text, sizeof(id_text), "Node ID %u", selected);
    if (app->font) {
      stygian_text(app->ctx, app->font, id_text, x, y, 14.0f, 0.84f, 0.88f,
                   0.97f, 1.0f);
    }
    y += 20.0f;

    if (have_bounds) {
      if (app->font)
        stygian_text(app->ctx, app->font, "X", x, y, 13.0f, 0.85f, 0.85f, 0.90f,
                     1.0f);
      if (stygian_slider(app->ctx, x + 14.0f, y, w - 14.0f, 14.0f, &nx, -2000.0f,
                         2000.0f)) {
        if (stygian_editor_node_set_position(app->editor, selected, nx, ny, true))
          mutated = true;
      }
      y += 20.0f;

      if (app->font)
        stygian_text(app->ctx, app->font, "Y", x, y, 13.0f, 0.85f, 0.85f, 0.90f,
                     1.0f);
      if (stygian_slider(app->ctx, x + 14.0f, y, w - 14.0f, 14.0f, &ny, -2000.0f,
                         2000.0f)) {
        if (stygian_editor_node_set_position(app->editor, selected, nx, ny, true))
          mutated = true;
      }
      y += 20.0f;

      if (app->font)
        stygian_text(app->ctx, app->font, "W", x, y, 13.0f, 0.85f, 0.85f, 0.90f,
                     1.0f);
      if (stygian_slider(app->ctx, x + 14.0f, y, w - 14.0f, 14.0f, &nw, 6.0f,
                         1200.0f)) {
        if (stygian_editor_node_set_size(app->editor, selected, nw, nh))
          mutated = true;
      }
      y += 20.0f;

      if (app->font)
        stygian_text(app->ctx, app->font, "H", x, y, 13.0f, 0.85f, 0.85f, 0.90f,
                     1.0f);
      if (stygian_slider(app->ctx, x + 14.0f, y, w - 14.0f, 14.0f, &nh, 6.0f,
                         1200.0f)) {
        if (stygian_editor_node_set_size(app->editor, selected, nw, nh))
          mutated = true;
      }
      y += 24.0f;
    }

    if (stygian_button(app->ctx, app->font, "Apply Paint Color", x, y, w, 26.0f)) {
      if (stygian_editor_node_set_color(app->editor, selected, app->paint_color)) {
        app_set_status(app, "Paint color applied to selected node.");
        mutated = true;
      } else {
        app_set_status(app, "Could not apply paint color to selected node.");
      }
    }
    y += 34.0f;
  }

  if (app->font) {
    stygian_text(app->ctx, app->font, "Function Editor", x, y, 17.0f, 0.93f,
                 0.95f, 1.0f, 1.0f);
  }
  y += 22.0f;

  if (app->font)
    stygian_text(app->ctx, app->font, "Trigger Node ID", x, y, 13.0f, 0.85f,
                 0.88f, 0.95f, 1.0f);
  y += 14.0f;
  stygian_text_input(app->ctx, app->font, x, y, w, 22.0f, app->behavior_trigger_id,
                     (int)sizeof(app->behavior_trigger_id));
  y += 28.0f;

  if (app->font)
    stygian_text(app->ctx, app->font, "Target Node ID", x, y, 13.0f, 0.85f,
                 0.88f, 0.95f, 1.0f);
  y += 14.0f;
  stygian_text_input(app->ctx, app->font, x, y, w, 22.0f, app->behavior_target_id,
                     (int)sizeof(app->behavior_target_id));
  y += 28.0f;

  if (stygian_button(app->ctx, app->font, "Use Selected As Target", x, y, w,
                     24.0f)) {
    StygianEditorNodeId sel = stygian_editor_selected_node(app->editor);
    if (sel != STYGIAN_EDITOR_INVALID_ID) {
      snprintf(app->behavior_target_id, sizeof(app->behavior_target_id), "%u",
               sel);
      app_set_status(app, "Target node set from current selection.");
    }
  }
  y += 30.0f;

  if (app->font)
    stygian_text(app->ctx, app->font, "Trigger Event", x, y, 13.0f, 0.85f, 0.88f,
                 0.95f, 1.0f);
  y += 16.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Press", x, y,
                             &app->behavior_event_choice, 0);
  y += 22.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Release", x, y,
                             &app->behavior_event_choice, 1);
  y += 22.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Value Changed", x, y,
                             &app->behavior_event_choice, 2);
  y += 26.0f;

  if (app->font)
    stygian_text(app->ctx, app->font, "Property", x, y, 13.0f, 0.85f, 0.88f,
                 0.95f, 1.0f);
  y += 16.0f;
  (void)stygian_radio_button(app->ctx, app->font, "X", x, y,
                             &app->behavior_property_choice, 0);
  y += 22.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Y", x, y,
                             &app->behavior_property_choice, 1);
  y += 22.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Opacity", x, y,
                             &app->behavior_property_choice, 2);
  y += 26.0f;

  if (app->font)
    stygian_text(app->ctx, app->font, "From", x, y, 13.0f, 0.85f, 0.88f, 0.95f,
                 1.0f);
  y += 14.0f;
  stygian_text_input(app->ctx, app->font, x, y, w, 22.0f, app->behavior_from_value,
                     (int)sizeof(app->behavior_from_value));
  y += 28.0f;

  if (app->font)
    stygian_text(app->ctx, app->font, "To", x, y, 13.0f, 0.85f, 0.88f, 0.95f,
                 1.0f);
  y += 14.0f;
  stygian_text_input(app->ctx, app->font, x, y, w, 22.0f, app->behavior_to_value,
                     (int)sizeof(app->behavior_to_value));
  y += 28.0f;

  if (app->font)
    stygian_text(app->ctx, app->font, "Duration ms", x, y, 13.0f, 0.85f, 0.88f,
                 0.95f, 1.0f);
  y += 14.0f;
  stygian_text_input(app->ctx, app->font, x, y, w, 22.0f,
                     app->behavior_duration_ms,
                     (int)sizeof(app->behavior_duration_ms));
  y += 30.0f;

  if (app->font)
    stygian_text(app->ctx, app->font, "Easing", x, y, 13.0f, 0.85f, 0.88f, 0.95f,
                 1.0f);
  y += 16.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Linear", x, y,
                             &app->behavior_easing_choice, 0);
  y += 22.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Out Cubic", x, y,
                             &app->behavior_easing_choice, 1);
  y += 22.0f;
  (void)stygian_radio_button(app->ctx, app->font, "InOut Cubic", x, y,
                             &app->behavior_easing_choice, 2);
  y += 28.0f;

  if (stygian_button(app->ctx, app->font, "Add Function Rule", x, y, w, 26.0f)) {
    StygianEditorBehaviorRule rule;
    uint32_t trigger_id = 0u;
    uint32_t target_id = 0u;
    uint32_t duration_ms = 0u;
    float from_value = NAN;
    float to_value = 0.0f;
    bool ok = true;

    memset(&rule, 0, sizeof(rule));
    if (!app_parse_u32(app->behavior_trigger_id, &trigger_id)) {
      app_set_status(app, "Function rule error: trigger node id invalid.");
      ok = false;
    }
    if (!app_parse_u32(app->behavior_target_id, &target_id)) {
      app_set_status(app, "Function rule error: target node id invalid.");
      ok = false;
    }
    if (!app_parse_float_text(app->behavior_from_value, &from_value)) {
      app_set_status(app,
                     "Function rule error: from value must be number or nan.");
      ok = false;
    }
    if (!app_parse_float_text(app->behavior_to_value, &to_value)) {
      app_set_status(app, "Function rule error: to value must be a number.");
      ok = false;
    }
    if (!app_parse_u32_or_default(app->behavior_duration_ms, 350u, &duration_ms) ||
        duration_ms == 0u) {
      app_set_status(app, "Function rule error: duration must be > 0.");
      ok = false;
    }

    if (ok) {
      rule.trigger_node = trigger_id;
      rule.trigger_event = app_event_kind_from_choice(app->behavior_event_choice);
      rule.action_kind = STYGIAN_EDITOR_ACTION_ANIMATE;
      rule.animate.target_node = target_id;
      rule.animate.property =
          app_property_kind_from_choice(app->behavior_property_choice);
      rule.animate.from_value = from_value;
      rule.animate.to_value = to_value;
      rule.animate.duration_ms = duration_ms;
      rule.animate.easing = app_easing_from_choice(app->behavior_easing_choice);

      if (stygian_editor_add_behavior(app->editor, &rule, NULL)) {
        app_set_status(app, "Function rule added.");
        mutated = true;
      } else {
        app_set_status(
            app,
            "Function rule add failed (node ids invalid or behavior capacity).");
      }
    }
  }
  y += 32.0f;

  if (stygian_button(app->ctx, app->font, "Fire Trigger Event", x, y, w, 26.0f)) {
    uint32_t trigger_id = 0u;
    if (app_parse_u32(app->behavior_trigger_id, &trigger_id)) {
      stygian_editor_trigger_event(app->editor, trigger_id,
                                   app_event_kind_from_choice(
                                       app->behavior_event_choice));
      app_set_status(app, "Trigger event fired.");
      mutated = true;
    } else {
      app_set_status(app, "Trigger event error: trigger node id invalid.");
    }
  }

  return mutated;
}

static bool app_draw_top_chrome(EditorBootstrapApp *app, StygianWindow *window,
                                int mouse_x, int mouse_y) {
  static const char *menu_labels[] = {"FILE", "EDIT", "VIEW", "SELECTION",
                                      "HELP"};
  static const char *primitive_labels[] = {"Square", "Circle", "Rectangle",
                                           "Ellipse"};
  float menu_h = app_menu_bar_height();
  float strip_h = app_tool_strip_height();
  float top_h = app_top_chrome_height();
  float menu_x = 8.0f;
  float place_x = 18.0f;
  float button_h = 20.0f;
  float button_y = menu_h + (strip_h - button_h) * 0.5f;
  float place_w = 116.0f;
  float point_x = place_x + place_w + 10.0f;
  float point_w = 112.0f;
  float hint_x = point_x + point_w + 14.0f;
  float hint_w = app->layout.top_w - hint_x - 10.0f;
  float file_menu_x = 8.0f;
  float file_menu_y = menu_h + 2.0f;
  float file_menu_w = 186.0f;
  float file_menu_hh = 84.0f;
  float prim_menu_x = 0.0f;
  float prim_menu_y = 0.0f;
  float prim_menu_w = 0.0f;
  float prim_row_h = 0.0f;
  float prim_menu_hh = 0.0f;
  float interactive_h = top_h;
  bool mutated = false;
  bool left_down;
  bool released_left;
  int i;
  char hint_text[196];

  if (!app || !window)
    return false;

  app_primitive_menu_geometry(app, &prim_menu_x, &prim_menu_y, &prim_menu_w,
                              &prim_row_h, &prim_menu_hh);

  stygian_rect(app->ctx, app->layout.top_x, app->layout.top_y, app->layout.top_w,
               menu_h, 0.50f, 0.50f, 0.50f, 1.0f);
  stygian_rect(app->ctx, app->layout.top_x, menu_h, app->layout.top_w, strip_h,
               0.84f, 0.84f, 0.86f, 1.0f);
  stygian_rect(app->ctx, 10.0f, menu_h + 4.0f, 258.0f, strip_h - 8.0f, 0.76f,
               0.82f, 0.92f, 1.0f);
  stygian_line(app->ctx, 0.0f, menu_h, app->layout.top_w, menu_h, 1.0f, 0.35f,
               0.35f, 0.35f, 1.0f);
  stygian_line(app->ctx, 0.0f, top_h - 1.0f, app->layout.top_w, top_h - 1.0f,
               1.0f, 0.35f, 0.35f, 0.35f, 1.0f);

  left_down = stygian_mouse_down(window, STYGIAN_MOUSE_LEFT);
  released_left = app->left_mouse_was_down && !left_down;

  for (i = 0; i < (int)(sizeof(menu_labels) / sizeof(menu_labels[0])); ++i) {
    float label_w = 54.0f;
    bool hover;
    if (app->font) {
      label_w = stygian_text_width(app->ctx, app->font, menu_labels[i], 12.0f) +
                18.0f;
      if (label_w < 46.0f)
        label_w = 46.0f;
    }
    hover = app_point_in_rect((float)mouse_x, (float)mouse_y, menu_x, 0.0f,
                              label_w, menu_h);
    if (hover) {
      stygian_rect_rounded(app->ctx, menu_x + 1.0f, 2.0f, label_w - 2.0f,
                           menu_h - 4.0f, 0.42f, 0.42f, 0.42f, 0.85f, 3.0f);
    }
    if (app->font) {
      stygian_text(app->ctx, app->font, menu_labels[i], menu_x + 6.0f, 5.0f,
                   12.0f, 0.95f, 0.95f, 0.95f, 1.0f);
    }
    if (released_left && hover) {
      if (i == 0) {
        app->file_menu_open = !app->file_menu_open;
        app->primitive_menu_open = false;
      } else {
        char status[96];
        app->file_menu_open = false;
        app->primitive_menu_open = false;
        snprintf(status, sizeof(status), "%s menu is a stub for now.",
                 menu_labels[i]);
        app_set_status(app, status);
      }
      mutated = true;
    }
    menu_x += label_w + 4.0f;
  }

  if (app->file_menu_open) {
    stygian_rect(app->ctx, file_menu_x, file_menu_y, file_menu_w, file_menu_hh,
                 0.16f, 0.18f, 0.20f, 1.0f);
    stygian_rect(app->ctx, file_menu_x + 1.0f, file_menu_y + 1.0f,
                 file_menu_w - 2.0f, file_menu_hh - 2.0f, 0.11f, 0.13f, 0.16f,
                 1.0f);
    if (stygian_button(app->ctx, app->font, "New Canvas", file_menu_x + 6.0f,
                       file_menu_y + 6.0f, file_menu_w - 12.0f, 21.0f)) {
      stygian_editor_reset(app->editor);
      app->path_tool_active = false;
      app->active_path = STYGIAN_EDITOR_INVALID_ID;
      app->camera_pan_x = 24.0f;
      app->camera_pan_y = app_top_chrome_height() + 24.0f;
      app->camera_zoom = 1.0f;
      app_sync_grid_config(app);
      app_sync_editor_viewport(app);
      app_set_status(app, "Canvas reset.");
      app->file_menu_open = false;
      app->primitive_menu_open = false;
      mutated = true;
    }
    if (stygian_button(app->ctx, app->font, "Export C23", file_menu_x + 6.0f,
                       file_menu_y + 31.0f, file_menu_w - 12.0f, 21.0f)) {
      if (app_write_generated_c23(app, "build/stygian_editor_generated.c")) {
        app_set_status(app, "Exported build/stygian_editor_generated.c");
      } else {
        app_set_status(app, "Export failed.");
      }
      app->file_menu_open = false;
    }
    if (stygian_button(app->ctx, app->font, "Quit", file_menu_x + 6.0f,
                       file_menu_y + 56.0f, file_menu_w - 12.0f, 21.0f)) {
      stygian_window_request_close(window);
    }
  }

  app->primitive_menu_hover_index = -1;
  if (stygian_button(app->ctx, app->font, "Place Shape", place_x, button_y,
                     place_w, button_h)) {
    app->primitive_menu_open = !app->primitive_menu_open;
    app->file_menu_open = false;
    mutated = true;
  }

  if (stygian_button(app->ctx, app->font,
                     app->path_tool_active ? "Finish Points" : "Point Place",
                     point_x, button_y, point_w, button_h)) {
    app->file_menu_open = false;
    app->primitive_menu_open = false;
    if (app->path_tool_active) {
      app_commit_path_tool(app);
      mutated = true;
    } else if (app_start_path_tool(app)) {
      mutated = true;
    }
  }

  if (app->primitive_menu_open) {
    app->primitive_menu_hover_index =
        app_primitive_menu_hover_index_at(app, (float)mouse_x, (float)mouse_y);

    stygian_rect(app->ctx, prim_menu_x, prim_menu_y, prim_menu_w, prim_menu_hh,
                 0.16f, 0.18f, 0.20f, 1.0f);
    stygian_rect(app->ctx, prim_menu_x + 1.0f, prim_menu_y + 1.0f,
                 prim_menu_w - 2.0f, prim_menu_hh - 2.0f, 0.10f, 0.12f, 0.15f,
                 1.0f);
    for (i = 0; i < 4; ++i) {
      if (app->primitive_menu_hover_index == i) {
        stygian_rect_rounded(app->ctx, prim_menu_x + 3.0f,
                             prim_menu_y + 3.0f + prim_row_h * (float)i,
                             prim_menu_w - 6.0f, prim_row_h - 1.0f, 0.24f, 0.44f,
                             0.63f, 0.92f, 3.0f);
      }
      if (stygian_button(app->ctx, app->font, primitive_labels[i],
                         prim_menu_x + 4.0f,
                         prim_menu_y + 4.0f + prim_row_h * (float)i,
                         prim_menu_w - 8.0f, prim_row_h - 2.0f)) {
        app->selected_primitive = (AppPrimitiveKind)i;
        app_drop_selected_primitive_in_viewport(app, -1, -1);
        mutated = true;
        app->primitive_menu_open = false;
        app_set_status(app, "Shape dropped.");
      }
    }
  }

  if (app->font) {
    if (hint_w > 170.0f) {
      snprintf(hint_text, sizeof(hint_text),
               "Current: %s | nodes:%u. Click Place Shape, then pick primitive.",
               app_primitive_name(app->selected_primitive),
               stygian_editor_node_count(app->editor));
      stygian_text(app->ctx, app->font, hint_text, hint_x, button_y + 4.0f,
                   11.0f, 0.18f, 0.22f, 0.28f, 1.0f);
    }
  }

  if (app->file_menu_open) {
    if (file_menu_y + file_menu_hh > interactive_h)
      interactive_h = file_menu_y + file_menu_hh;
  }
  if (app->primitive_menu_open) {
    if (prim_menu_y + prim_menu_hh > interactive_h)
      interactive_h = prim_menu_y + prim_menu_hh;
  }
  stygian_widgets_register_region(
      0.0f, 0.0f, app->layout.top_w, interactive_h,
      STYGIAN_WIDGET_REGION_POINTER_LEFT_MUTATES |
          STYGIAN_WIDGET_REGION_POINTER_RIGHT_MUTATES |
          STYGIAN_WIDGET_REGION_SCROLL);
  app->left_mouse_was_down = left_down;

  return mutated;
}

static void app_draw_path_preview(EditorBootstrapApp *app) {
  uint32_t i;
  if (!app || !app->path_tool_active || app->path_preview_count == 0u)
    return;

  for (i = 0u; i < app->path_preview_count; ++i) {
    float vx = 0.0f;
    float vy = 0.0f;
    stygian_editor_world_to_view(app->editor, app->path_preview_points[i].x,
                                 app->path_preview_points[i].y, &vx, &vy);
    if (i > 0u) {
      float pvx = 0.0f;
      float pvy = 0.0f;
      stygian_editor_world_to_view(app->editor, app->path_preview_points[i - 1u].x,
                                   app->path_preview_points[i - 1u].y, &pvx,
                                   &pvy);
      stygian_line(app->ctx, pvx, pvy, vx, vy, 1.8f, 0.18f, 0.78f, 1.0f, 0.95f);
    }
    stygian_rect_rounded(app->ctx, vx - 3.0f, vy - 3.0f, 6.0f, 6.0f, 0.18f,
                         0.78f, 1.0f, 0.96f, 3.0f);
  }

  if (app_point_in_rect((float)app->last_mouse_x, (float)app->last_mouse_y,
                        app->layout.viewport_x, app->layout.viewport_y,
                        app->layout.viewport_w, app->layout.viewport_h)) {
    float world_x = 0.0f;
    float world_y = 0.0f;
    float snapped_x = 0.0f;
    float snapped_y = 0.0f;
    float last_vx = 0.0f;
    float last_vy = 0.0f;
    float cursor_vx = 0.0f;
    float cursor_vy = 0.0f;
    uint32_t last_i = app->path_preview_count - 1u;
    stygian_editor_view_to_world(app->editor, (float)app->last_mouse_x,
                                 (float)app->last_mouse_y, &world_x, &world_y);
    snapped_x = world_x;
    snapped_y = world_y;
    stygian_editor_snap_world_point(app->editor, world_x, world_y, &snapped_x,
                                    &snapped_y);
    stygian_editor_world_to_view(app->editor, app->path_preview_points[last_i].x,
                                 app->path_preview_points[last_i].y, &last_vx,
                                 &last_vy);
    stygian_editor_world_to_view(app->editor, snapped_x, snapped_y, &cursor_vx,
                                 &cursor_vy);
    stygian_line(app->ctx, last_vx, last_vy, cursor_vx, cursor_vy, 1.2f, 0.55f,
                 0.86f, 1.0f, 0.72f);
    stygian_rect_rounded(app->ctx, cursor_vx - 2.5f, cursor_vy - 2.5f, 5.0f,
                         5.0f, 0.73f, 0.92f, 1.0f, 0.82f, 2.5f);
  }
}

static void app_draw_marquee_selection(EditorBootstrapApp *app) {
  float x0;
  float y0;
  float x1;
  float y1;
  float min_x;
  float min_y;
  float max_x;
  float max_y;
  float w;
  float h;
  if (!app || !app->marquee_selecting || !app_marquee_has_area(app))
    return;
  x0 = app->marquee_start_view_x;
  y0 = app->marquee_start_view_y;
  x1 = app->marquee_curr_view_x;
  y1 = app->marquee_curr_view_y;
  min_x = x0 < x1 ? x0 : x1;
  max_x = x0 < x1 ? x1 : x0;
  min_y = y0 < y1 ? y0 : y1;
  max_y = y0 < y1 ? y1 : y0;
  w = max_x - min_x;
  h = max_y - min_y;
  if (w <= 0.0001f || h <= 0.0001f)
    return;
  stygian_rect(app->ctx, min_x, min_y, w, h, 0.16f, 0.56f, 0.96f, 0.18f);
  stygian_line(app->ctx, min_x, min_y, max_x, min_y, 1.0f, 0.26f, 0.72f, 0.99f,
               0.95f);
  stygian_line(app->ctx, max_x, min_y, max_x, max_y, 1.0f, 0.26f, 0.72f, 0.99f,
               0.95f);
  stygian_line(app->ctx, max_x, max_y, min_x, max_y, 1.0f, 0.26f, 0.72f, 0.99f,
               0.95f);
  stygian_line(app->ctx, min_x, max_y, min_x, min_y, 1.0f, 0.26f, 0.72f, 0.99f,
               0.95f);
}

static void app_draw_resize_handles(EditorBootstrapApp *app) {
  static const AppResizeHandle handles[] = {
      APP_RESIZE_NW, APP_RESIZE_N, APP_RESIZE_NE, APP_RESIZE_E,
      APP_RESIZE_SE, APP_RESIZE_S, APP_RESIZE_SW, APP_RESIZE_W};
  AppResizeHandle hover = APP_RESIZE_NONE;
  float vx = 0.0f;
  float vy = 0.0f;
  float vw = 0.0f;
  float vh = 0.0f;
  float hs = app_resize_handle_size_px();
  uint32_t i;
  if (!app || !app->editor)
    return;
  if (!app_selected_bounds_view(app, &vx, &vy, &vw, &vh))
    return;
  if (vw <= 0.0f || vh <= 0.0f)
    return;

  hover = app_resize_handle_at(app, (float)app->last_mouse_x,
                               (float)app->last_mouse_y);
  for (i = 0u; i < (uint32_t)(sizeof(handles) / sizeof(handles[0])); ++i) {
    float cx = 0.0f;
    float cy = 0.0f;
    bool hot = false;
    app_resize_handle_center(handles[i], vx, vy, vw, vh, &cx, &cy);
    hot = (hover == handles[i]) ||
          (app->resizing_node && app->resize_handle == handles[i]);
    stygian_rect_rounded(app->ctx, cx - hs * 0.5f, cy - hs * 0.5f, hs, hs,
                         hot ? 0.16f : 0.10f, hot ? 0.74f : 0.58f, 0.98f, 1.0f,
                         2.0f);
    stygian_rect_rounded(app->ctx, cx - hs * 0.5f + 1.0f, cy - hs * 0.5f + 1.0f,
                         hs - 2.0f, hs - 2.0f, hot ? 0.76f : 0.66f,
                         hot ? 0.92f : 0.84f, 1.0f, 1.0f, 1.5f);
  }
}

static bool app_draw_properties_panel(EditorBootstrapApp *app) {
  float panel_x;
  float panel_y;
  float panel_w;
  float panel_h;
  float x;
  float y;
  float content_w;
  bool mutated = false;
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count;
  float gx = 0.0f;
  float gy = 0.0f;
  float gw = 0.0f;
  float gh = 0.0f;
  StygianEditorColor first_color = {0};
  bool have_first_color = false;
  bool have_rect = false;
  float rtl = 0.0f;
  float rtr = 0.0f;
  float rbr = 0.0f;
  float rbl = 0.0f;
  bool uniform_all = true;
  bool uniform_any = false;
  bool uniform_mixed = false;
  uint32_t i;

  if (!app || !app->editor)
    return false;

  panel_x = app->layout.right_x;
  panel_y = app->layout.right_y;
  panel_w = app->layout.right_w;
  panel_h = app->layout.right_h;
  if (panel_w <= 0.0f || panel_h <= 0.0f)
    return false;

  stygian_rect(app->ctx, panel_x, panel_y, panel_w, panel_h, 0.10f, 0.12f, 0.15f,
               1.0f);
  stygian_rect(app->ctx, panel_x + 1.0f, panel_y + 1.0f, panel_w - 2.0f,
               panel_h - 2.0f, 0.08f, 0.10f, 0.13f, 1.0f);
  stygian_widgets_register_region(
      panel_x, panel_y, panel_w, panel_h,
      STYGIAN_WIDGET_REGION_POINTER_LEFT_MUTATES |
          STYGIAN_WIDGET_REGION_POINTER_RIGHT_MUTATES |
          STYGIAN_WIDGET_REGION_SCROLL);

  x = panel_x + 12.0f;
  y = panel_y + 10.0f;
  content_w = panel_w - 24.0f;

  if (app->font) {
    stygian_text(app->ctx, app->font, "Properties", x, y, 17.0f, 0.93f, 0.95f,
                 1.0f, 1.0f);
  }
  y += 24.0f;

  count = app_selected_nodes(app, ids, APP_MAX_TRANSFORM_SELECTION);
  if (count == 0u) {
    if (app->font) {
      stygian_text(app->ctx, app->font, "No selection.", x, y, 14.0f, 0.82f, 0.86f,
                   0.93f, 1.0f);
    }
    return false;
  }

  if (app_bounds_for_nodes_world(app, ids, count, &gx, &gy, &gw, &gh)) {
    app->panel_group_x = gx;
    app->panel_group_y = gy;
    app->panel_group_w = gw;
    app->panel_group_h = gh;
  }

  if (app->font) {
    char sel_text[64];
    snprintf(sel_text, sizeof(sel_text), "Selected: %u", count);
    stygian_text(app->ctx, app->font, sel_text, x, y, 13.0f, 0.78f, 0.90f, 0.99f,
                 1.0f);
  }
  y += 18.0f;

  if (app->font)
    stygian_text(app->ctx, app->font, "X", x, y, 13.0f, 0.86f, 0.88f, 0.92f, 1.0f);
  {
    float v = app->panel_group_x;
    if (stygian_slider(app->ctx, x + 12.0f, y, content_w - 12.0f, 14.0f, &v,
                       -4000.0f, 4000.0f)) {
      if (app_apply_group_translation(app, v, app->panel_group_y)) {
        mutated = true;
        (void)app_selected_bounds_world(app, &app->panel_group_x, &app->panel_group_y,
                                        &app->panel_group_w, &app->panel_group_h);
      }
    }
  }
  y += 20.0f;

  if (app->font)
    stygian_text(app->ctx, app->font, "Y", x, y, 13.0f, 0.86f, 0.88f, 0.92f, 1.0f);
  {
    float v = app->panel_group_y;
    if (stygian_slider(app->ctx, x + 12.0f, y, content_w - 12.0f, 14.0f, &v,
                       -4000.0f, 4000.0f)) {
      if (app_apply_group_translation(app, app->panel_group_x, v)) {
        mutated = true;
        (void)app_selected_bounds_world(app, &app->panel_group_x, &app->panel_group_y,
                                        &app->panel_group_w, &app->panel_group_h);
      }
    }
  }
  y += 20.0f;

  if (app->font)
    stygian_text(app->ctx, app->font, "W", x, y, 13.0f, 0.86f, 0.88f, 0.92f, 1.0f);
  {
    float v = app->panel_group_w;
    if (stygian_slider(app->ctx, x + 12.0f, y, content_w - 12.0f, 14.0f, &v, 1.0f,
                       4000.0f)) {
      if (app_apply_group_resize(app, v, app->panel_group_h, true, false)) {
        mutated = true;
        (void)app_selected_bounds_world(app, &app->panel_group_x, &app->panel_group_y,
                                        &app->panel_group_w, &app->panel_group_h);
      }
    }
  }
  y += 20.0f;

  if (app->font)
    stygian_text(app->ctx, app->font, "H", x, y, 13.0f, 0.86f, 0.88f, 0.92f, 1.0f);
  {
    float v = app->panel_group_h;
    if (stygian_slider(app->ctx, x + 12.0f, y, content_w - 12.0f, 14.0f, &v, 1.0f,
                       4000.0f)) {
      if (app_apply_group_resize(app, app->panel_group_w, v, false, true)) {
        mutated = true;
        (void)app_selected_bounds_world(app, &app->panel_group_x, &app->panel_group_y,
                                        &app->panel_group_w, &app->panel_group_h);
      }
    }
  }
  y += 24.0f;

  for (i = 0u; i < count; ++i) {
    bool enabled = app_node_uniform_scale_enabled(app, ids[i]);
    if (enabled)
      uniform_any = true;
    else
      uniform_all = false;
    if (!have_first_color &&
        stygian_editor_node_get_color(app->editor, ids[i], &first_color)) {
      have_first_color = true;
    }
    if (!have_rect) {
      StygianEditorShapeKind kind;
      if (stygian_editor_node_get_shape_kind(app->editor, ids[i], &kind) &&
          kind == STYGIAN_EDITOR_SHAPE_RECT &&
          stygian_editor_node_get_corner_radii(app->editor, ids[i], &rtl, &rtr,
                                               &rbr, &rbl)) {
        have_rect = true;
      }
    }
  }
  uniform_mixed = uniform_any && !uniform_all;
  app->panel_uniform_scale = uniform_any;
  if (stygian_checkbox(app->ctx, app->font, "Uniform Scale (per shape)", x, y,
                       &app->panel_uniform_scale)) {
    for (i = 0u; i < count; ++i) {
      app_set_node_uniform_scale(app, ids[i], app->panel_uniform_scale);
    }
    mutated = true;
  }
  y += 24.0f;
  if (uniform_mixed && app->font) {
    stygian_text(app->ctx, app->font, "Mixed values in selection.", x, y, 11.0f,
                 0.73f, 0.83f, 0.92f, 1.0f);
    y += 16.0f;
  }

  if (app->font) {
    stygian_text(app->ctx, app->font, "Appearance", x, y, 15.0f, 0.93f, 0.95f,
                 1.0f, 1.0f);
  }
  y += 20.0f;

  if (have_first_color) {
    app->panel_opacity = first_color.a;
  } else {
    app->panel_opacity = 1.0f;
  }
  if (app->font)
    stygian_text(app->ctx, app->font, "Opacity", x, y, 12.0f, 0.84f, 0.88f, 0.92f,
                 1.0f);
  {
    float opacity = app->panel_opacity;
    if (stygian_slider(app->ctx, x + 56.0f, y, content_w - 56.0f, 14.0f, &opacity,
                       0.0f, 1.0f)) {
      if (app_apply_opacity_to_selection(app, opacity))
        mutated = true;
      app->panel_opacity = opacity;
    }
  }
  y += 22.0f;

  if (stygian_button(app->ctx, app->font, "Apply Paint Color", x, y, content_w,
                     24.0f)) {
    if (app_apply_color_to_selection(app, app->paint_color))
      mutated = true;
  }
  y += 32.0f;

  if (app->font) {
    stygian_text(app->ctx, app->font, "Corner Radius", x, y, 15.0f, 0.93f, 0.95f,
                 1.0f, 1.0f);
  }
  y += 20.0f;
  if (have_rect) {
    app->panel_radius_tl = rtl;
    app->panel_radius_tr = rtr;
    app->panel_radius_br = rbr;
    app->panel_radius_bl = rbl;

    if (app->font)
      stygian_text(app->ctx, app->font, "TL", x, y, 12.0f, 0.84f, 0.88f, 0.92f,
                   1.0f);
    {
      float v = app->panel_radius_tl;
      if (stygian_slider(app->ctx, x + 18.0f, y, content_w - 18.0f, 14.0f, &v,
                         0.0f, 300.0f)) {
        app->panel_radius_tl = v;
        if (app_apply_corner_radii_to_selection(app, app->panel_radius_tl,
                                                app->panel_radius_tr,
                                                app->panel_radius_br,
                                                app->panel_radius_bl)) {
          mutated = true;
        }
      }
    }
    y += 20.0f;

    if (app->font)
      stygian_text(app->ctx, app->font, "TR", x, y, 12.0f, 0.84f, 0.88f, 0.92f,
                   1.0f);
    {
      float v = app->panel_radius_tr;
      if (stygian_slider(app->ctx, x + 18.0f, y, content_w - 18.0f, 14.0f, &v,
                         0.0f, 300.0f)) {
        app->panel_radius_tr = v;
        if (app_apply_corner_radii_to_selection(app, app->panel_radius_tl,
                                                app->panel_radius_tr,
                                                app->panel_radius_br,
                                                app->panel_radius_bl)) {
          mutated = true;
        }
      }
    }
    y += 20.0f;

    if (app->font)
      stygian_text(app->ctx, app->font, "BR", x, y, 12.0f, 0.84f, 0.88f, 0.92f,
                   1.0f);
    {
      float v = app->panel_radius_br;
      if (stygian_slider(app->ctx, x + 18.0f, y, content_w - 18.0f, 14.0f, &v,
                         0.0f, 300.0f)) {
        app->panel_radius_br = v;
        if (app_apply_corner_radii_to_selection(app, app->panel_radius_tl,
                                                app->panel_radius_tr,
                                                app->panel_radius_br,
                                                app->panel_radius_bl)) {
          mutated = true;
        }
      }
    }
    y += 20.0f;

    if (app->font)
      stygian_text(app->ctx, app->font, "BL", x, y, 12.0f, 0.84f, 0.88f, 0.92f,
                   1.0f);
    {
      float v = app->panel_radius_bl;
      if (stygian_slider(app->ctx, x + 18.0f, y, content_w - 18.0f, 14.0f, &v,
                         0.0f, 300.0f)) {
        app->panel_radius_bl = v;
        if (app_apply_corner_radii_to_selection(app, app->panel_radius_tl,
                                                app->panel_radius_tr,
                                                app->panel_radius_br,
                                                app->panel_radius_bl)) {
          mutated = true;
        }
      }
    }
  } else if (app->font) {
    stygian_text(app->ctx, app->font, "No rectangle in selection.", x, y, 12.0f,
                 0.74f, 0.82f, 0.90f, 1.0f);
  }

  return mutated;
}

static bool app_draw_viewport(EditorBootstrapApp *app) {
  float panel_x = app->layout.viewport_x;
  float panel_y = app->layout.viewport_y;
  float panel_w = app->layout.viewport_w;
  float panel_h = app->layout.viewport_h;

  stygian_rect(app->ctx, panel_x, panel_y, panel_w, panel_h, 0.06f, 0.07f, 0.09f,
               1.0f);
  stygian_rect(app->ctx, panel_x + 1.0f, panel_y + 1.0f, panel_w - 2.0f,
               panel_h - 2.0f, 0.05f, 0.06f, 0.08f, 1.0f);
  stygian_widgets_register_region(
      panel_x, panel_y, panel_w, panel_h,
      STYGIAN_WIDGET_REGION_POINTER_LEFT_MUTATES |
          STYGIAN_WIDGET_REGION_POINTER_RIGHT_MUTATES |
          STYGIAN_WIDGET_REGION_SCROLL);

  stygian_clip_push(app->ctx, panel_x, panel_y, panel_w, panel_h);
  stygian_editor_render_viewport2d(app->editor, app->ctx, true, true);
  app_draw_path_preview(app);
  app_draw_marquee_selection(app);
  app_draw_resize_handles(app);
  stygian_clip_pop(app->ctx);

  return false;
}

static bool app_draw_behavior_graph_panel(EditorBootstrapApp *app, float panel_x,
                                          float panel_y, float panel_w,
                                          float panel_h) {
  uint32_t behavior_count = stygian_editor_behavior_count(app->editor);
  uint32_t i;
  bool mutated = false;
  float x = panel_x + 10.0f;
  float y = panel_y + 8.0f;
  float content_w = panel_w - 20.0f;
  float lane_h = 82.0f;

  stygian_rect(app->ctx, panel_x, panel_y, panel_w, panel_h, 0.09f, 0.11f, 0.14f,
               1.0f);

  if (app->font) {
    char header[96];
    snprintf(header, sizeof(header), "Behavior Graph (%u rules)", behavior_count);
    stygian_text(app->ctx, app->font, header, x, y, 16.0f, 0.93f, 0.95f, 1.0f,
                 1.0f);
  }
  y += 26.0f;

  if (behavior_count == 0u) {
    if (app->font) {
      stygian_text(
          app->ctx, app->font,
          "No rules yet. Add one in the Function Editor panel.", x, y, 13.0f,
          0.80f, 0.84f, 0.92f, 1.0f);
    }
    if (app->behavior_graph_message[0] && app->font) {
      stygian_text(app->ctx, app->font, app->behavior_graph_message, x, y + 22.0f,
                   12.0f, 0.70f, 0.86f, 0.95f, 1.0f);
    }
    return false;
  }

  for (i = 0u; i < behavior_count; ++i) {
    StygianEditorBehaviorId behavior_id = 0u;
    StygianEditorBehaviorRule rule;
    float row_top = y + lane_h * (float)i;
    float left_x;
    float right_x;
    float node_w;
    float node_h;
    float center_y;
    char trigger_label[96];
    char action_label[128];
    char easing_label[64];
    char button_label[64];
    float button_x;
    float button_w = 76.0f;

    if (row_top + lane_h > panel_y + panel_h - 6.0f)
      break;

    if (!stygian_editor_get_behavior_rule(app->editor, i, &behavior_id, &rule))
      continue;

    node_w = app_clampf(content_w * 0.38f, 140.0f, 220.0f);
    node_h = 54.0f;
    left_x = x;
    right_x = x + content_w - node_w;
    center_y = row_top + 24.0f;

    stygian_rect_rounded(app->ctx, panel_x + 4.0f, row_top,
                         panel_w - 8.0f, lane_h - 8.0f, 0.08f, 0.09f, 0.12f,
                         0.95f, 6.0f);

    stygian_rect_rounded(app->ctx, left_x, row_top + 8.0f, node_w, node_h, 0.13f,
                         0.29f, 0.48f, 0.98f, 8.0f);
    stygian_rect_rounded(app->ctx, right_x, row_top + 8.0f, node_w, node_h, 0.18f,
                         0.35f, 0.22f, 0.98f, 8.0f);

    stygian_line(app->ctx, left_x + node_w, center_y, right_x, center_y, 2.0f,
                 0.42f, 0.72f, 0.96f, 1.0f);

    snprintf(trigger_label, sizeof(trigger_label), "N%u %s", rule.trigger_node,
             app_event_kind_name(rule.trigger_event));
    snprintf(action_label, sizeof(action_label), "N%u %s -> %.2f",
             rule.animate.target_node,
             app_property_kind_name(rule.animate.property),
             rule.animate.to_value);
    snprintf(easing_label, sizeof(easing_label), "%ums %s",
             rule.animate.duration_ms, app_easing_name(rule.animate.easing));

    if (app->font) {
      stygian_text(app->ctx, app->font, trigger_label, left_x + 8.0f,
                   row_top + 14.0f, 12.5f, 0.90f, 0.95f, 1.0f, 1.0f);
      stygian_text(app->ctx, app->font, action_label, right_x + 8.0f,
                   row_top + 14.0f, 12.5f, 0.91f, 1.0f, 0.91f, 1.0f);
      stygian_text(app->ctx, app->font, easing_label, right_x + 8.0f,
                   row_top + 30.0f, 11.5f, 0.83f, 0.95f, 0.87f, 1.0f);
    }

    snprintf(button_label, sizeof(button_label), "Fire %u", behavior_id);
    button_x = right_x + node_w - button_w - 6.0f;
    if (stygian_button(app->ctx, app->font, button_label, button_x,
                       row_top + lane_h - 28.0f, button_w, 20.0f)) {
      stygian_editor_trigger_event(app->editor, rule.trigger_node,
                                   rule.trigger_event);
      snprintf(app->behavior_graph_message, sizeof(app->behavior_graph_message),
               "Fired rule %u.", behavior_id);
      app_set_status(app, app->behavior_graph_message);
      mutated = true;
    }
  }

  if (app->behavior_graph_message[0] && app->font) {
    stygian_text(app->ctx, app->font, app->behavior_graph_message, x,
                 panel_y + panel_h - 18.0f, 11.5f, 0.72f, 0.87f, 0.96f, 1.0f);
  }

  return mutated;
}

static void app_draw_console_panel(EditorBootstrapApp *app, float panel_x,
                                   float panel_y, float panel_w, float panel_h) {
  char line_a[192];
  char line_b[192];
  char line_c[192];
  uint32_t nodes = stygian_editor_node_count(app->editor);
  uint32_t rules = stygian_editor_behavior_count(app->editor);
  StygianEditorNodeId selected = stygian_editor_selected_node(app->editor);

  stygian_rect(app->ctx, panel_x, panel_y, panel_w, panel_h, 0.07f, 0.08f, 0.10f,
               1.0f);
  stygian_rect(app->ctx, panel_x + 1.0f, panel_y + 1.0f, panel_w - 2.0f,
               panel_h - 2.0f, 0.05f, 0.06f, 0.08f, 1.0f);

  snprintf(line_a, sizeof(line_a),
           "Dockable Bootstrap: drag panel tabs to redock/split/float.");
  snprintf(line_b, sizeof(line_b), "nodes=%u rules=%u selected=%u", nodes, rules,
           selected);
  snprintf(line_c, sizeof(line_c), "status: %s", app->status_text);

  if (app->font) {
    stygian_text(app->ctx, app->font, line_a, panel_x + 10.0f, panel_y + 10.0f,
                 12.0f, 0.82f, 0.88f, 0.96f, 1.0f);
    stygian_text(app->ctx, app->font, line_b, panel_x + 10.0f, panel_y + 28.0f,
                 12.0f, 0.73f, 0.86f, 0.93f, 1.0f);
    stygian_text(app->ctx, app->font, line_c, panel_x + 10.0f, panel_y + 46.0f,
                 11.5f, 0.88f, 0.89f, 0.95f, 1.0f);
  }
}

static void app_panel_tools_cb(StygianDockPanel *panel, StygianContext *ctx,
                               StygianFont font, float x, float y, float w,
                               float h) {
  EditorBootstrapApp *app = g_editor_bootstrap_app;
  bool mutated;
  (void)panel;
  if (!app)
    return;
  app->ctx = ctx;
  app->font = font;
  app->layout.left_x = x;
  app->layout.left_y = y;
  app->layout.left_w = w;
  app->layout.left_h = h;
  mutated = app_draw_left_panel(app, app->last_mouse_x, app->last_mouse_y);
  if (mutated)
    stygian_request_repaint_after_ms(ctx, 0u);
}

static void app_panel_viewport_cb(StygianDockPanel *panel, StygianContext *ctx,
                                  StygianFont font, float x, float y, float w,
                                  float h) {
  EditorBootstrapApp *app = g_editor_bootstrap_app;
  bool mutated;
  (void)panel;
  if (!app)
    return;
  app->ctx = ctx;
  app->font = font;
  app->layout.viewport_x = x;
  app->layout.viewport_y = y;
  app->layout.viewport_w = w;
  app->layout.viewport_h = h;
  app_sync_editor_viewport(app);
  mutated = app_draw_viewport(app);
  if (mutated)
    stygian_request_repaint_after_ms(ctx, 0u);
}

static void app_panel_inspector_cb(StygianDockPanel *panel, StygianContext *ctx,
                                   StygianFont font, float x, float y, float w,
                                   float h) {
  EditorBootstrapApp *app = g_editor_bootstrap_app;
  bool mutated;
  (void)panel;
  if (!app)
    return;
  app->ctx = ctx;
  app->font = font;
  app->layout.right_x = x;
  app->layout.right_y = y;
  app->layout.right_w = w;
  app->layout.right_h = h;
  mutated = app_draw_right_panel(app);
  if (mutated)
    stygian_request_repaint_after_ms(ctx, 0u);
}

static void app_panel_behavior_graph_cb(StygianDockPanel *panel, StygianContext *ctx,
                                        StygianFont font, float x, float y,
                                        float w, float h) {
  EditorBootstrapApp *app = g_editor_bootstrap_app;
  bool mutated;
  (void)panel;
  if (!app)
    return;
  app->ctx = ctx;
  app->font = font;
  mutated = app_draw_behavior_graph_panel(app, x, y, w, h);
  if (mutated)
    stygian_request_repaint_after_ms(ctx, 0u);
}

static void app_panel_console_cb(StygianDockPanel *panel, StygianContext *ctx,
                                 StygianFont font, float x, float y, float w,
                                 float h) {
  EditorBootstrapApp *app = g_editor_bootstrap_app;
  (void)panel;
  if (!app)
    return;
  app->ctx = ctx;
  app->font = font;
  app_draw_console_panel(app, x, y, w, h);
}

static bool app_init_dock(EditorBootstrapApp *app) {
  StygianWindow *window;
  StygianDockNode *left = NULL;
  StygianDockNode *right = NULL;
  StygianDockNode *center = NULL;
  StygianDockNode *inspector_stack = NULL;

  if (!app || !app->ctx)
    return false;

  window = stygian_get_window(app->ctx);
  app->dock = stygian_dock_create(stygian_window_native_context(window),
                                  stygian_window_native_handle(window));
  if (!app->dock)
    return false;

  app->panel_tools_id = stygian_dock_register_panel(
      app->dock, "Tools", false, app_panel_tools_cb, NULL);
  app->panel_viewport_id = stygian_dock_register_panel(
      app->dock, "Viewport", false, app_panel_viewport_cb, NULL);
  app->panel_inspector_id = stygian_dock_register_panel(
      app->dock, "Inspector", false, app_panel_inspector_cb, NULL);
  app->panel_behavior_graph_id = stygian_dock_register_panel(
      app->dock, "Behavior Graph", true, app_panel_behavior_graph_cb, NULL);
  app->panel_console_id = stygian_dock_register_panel(
      app->dock, "Console", true, app_panel_console_cb, NULL);

  if (!app->panel_tools_id || !app->panel_viewport_id || !app->panel_inspector_id ||
      !app->panel_behavior_graph_id || !app->panel_console_id) {
    return false;
  }

  {
    StygianDockNode *root = stygian_dock_get_root(app->dock);
    stygian_dock_add_panel_to_node(app->dock, root, app->panel_tools_id);
    stygian_dock_split(app->dock, root, STYGIAN_DOCK_SPLIT_VERTICAL, 0.22f, &left,
                       &right);
    if (!left || !right)
      return false;

    stygian_dock_add_panel_to_node(app->dock, right, app->panel_viewport_id);
    stygian_dock_split(app->dock, right, STYGIAN_DOCK_SPLIT_VERTICAL, 0.72f,
                       &center, &inspector_stack);
    if (!center || !inspector_stack)
      return false;

    stygian_dock_add_panel_to_node(app->dock, inspector_stack,
                                   app->panel_behavior_graph_id);
    stygian_dock_add_panel_to_node(app->dock, inspector_stack,
                                   app->panel_console_id);
    stygian_dock_add_panel_to_node(app->dock, inspector_stack,
                                   app->panel_inspector_id);
  }

  return true;
}

int main(void) {
  StygianWindowConfig win_cfg = {
      .title = "Stygian Editor Bootstrap",
      .width = 1440,
      .height = 900,
      .flags = STYGIAN_EDITOR_BOOTSTRAP_WINDOW_FLAGS,
  };
  StygianWindow *window = NULL;
  StygianConfig cfg;
  EditorBootstrapApp app;
  StygianEditorConfig editor_cfg;
  StygianEditorHost editor_host;
  bool first_frame = true;

  memset(&cfg, 0, sizeof(cfg));
  memset(&app, 0, sizeof(app));
  memset(&editor_host, 0, sizeof(editor_host));

  window = stygian_window_create(&win_cfg);
  if (!window)
    return 1;

  cfg.backend = STYGIAN_EDITOR_BOOTSTRAP_BACKEND;
  cfg.window = window;
  app.ctx = stygian_create(&cfg);
  if (!app.ctx) {
    stygian_window_destroy(window);
    return 1;
  }

  app.font = stygian_font_load(app.ctx, "assets/atlas.png", "assets/atlas.json");
  app.paint_color = stygian_editor_color_rgba(0.16f, 0.61f, 0.95f, 1.0f);
  app.grid_enabled = true;
  app.sub_snap_enabled = true;
  app.major_step_px = 96.0f;
  app.snap_tolerance_px = -1.0f;
  app.camera_zoom = 1.0f;
  app.selected_primitive = APP_PRIMITIVE_RECTANGLE;
  snprintf(app.token_name, sizeof(app.token_name), "primary");
  app.behavior_event_choice = 0;
  app.behavior_property_choice = 1;
  app.behavior_easing_choice = 1;
  app.now_ms = app_now_ms_system();

  editor_cfg = stygian_editor_config_default();
  editor_cfg.max_nodes = 8192u;
  editor_cfg.max_path_points = 65536u;
  editor_cfg.max_behaviors = 4096u;
  editor_host.user_data = &app;
  editor_host.now_ms = app_host_now_ms;
  editor_host.log = app_host_log;
  editor_host.request_repaint_hz = app_host_request_repaint_hz;
  app.editor = stygian_editor_create(&editor_cfg, &editor_host);
  if (!app.editor) {
    if (app.font)
      stygian_font_destroy(app.ctx, app.font);
    stygian_destroy(app.ctx);
    stygian_window_destroy(window);
    return 1;
  }

  {
    int width = 0;
    int height = 0;
    stygian_window_get_size(window, &width, &height);
    app_layout_set_canvas(&app, width, height);
    app.camera_pan_x = 24.0f;
    app.camera_pan_y = app_top_chrome_height() + 24.0f;
    app_sync_grid_config(&app);
    app_sync_editor_viewport(&app);
    app_set_status(&app, "Canvas ready.");
  }

  while (!stygian_window_should_close(window)) {
    StygianEvent event;
    bool event_mutated = false;
    bool event_requested = false;
    bool event_eval_requested = false;
    uint32_t wait_ms = stygian_next_repaint_wait_ms(app.ctx, 250u);
    int win_w = 0;
    int win_h = 0;

    app.now_ms = app_now_ms_system();
    stygian_window_get_size(window, &win_w, &win_h);
    app_layout_set_canvas(&app, win_w, win_h);
    app_sync_editor_viewport(&app);

    stygian_widgets_begin_frame(app.ctx);

    while (stygian_window_poll_event(window, &event)) {
      StygianWidgetEventImpact impact =
          stygian_widgets_process_event_ex(app.ctx, &event);
      if (impact & STYGIAN_IMPACT_MUTATED_STATE)
        event_mutated = true;
      if (impact & STYGIAN_IMPACT_REQUEST_REPAINT)
        event_requested = true;
      if (impact & STYGIAN_IMPACT_REQUEST_EVAL)
        event_eval_requested = true;

      if (event.type == STYGIAN_EVENT_CLOSE)
        stygian_window_request_close(window);

      if (app_handle_viewport_event(&app, &event))
        event_mutated = true;
    }

    if (!first_frame && !event_mutated && !event_requested &&
        !event_eval_requested) {
      if (stygian_window_wait_event_timeout(window, &event, wait_ms)) {
        StygianWidgetEventImpact impact =
            stygian_widgets_process_event_ex(app.ctx, &event);
        if (impact & STYGIAN_IMPACT_MUTATED_STATE)
          event_mutated = true;
        if (impact & STYGIAN_IMPACT_REQUEST_REPAINT)
          event_requested = true;
        if (impact & STYGIAN_IMPACT_REQUEST_EVAL)
          event_eval_requested = true;
        if (event.type == STYGIAN_EVENT_CLOSE)
          stygian_window_request_close(window);
        if (app_handle_viewport_event(&app, &event))
          event_mutated = true;
      }
    }

    stygian_editor_tick(app.editor, app.now_ms);

    {
      bool repaint_pending = stygian_has_pending_repaint(app.ctx);
      bool render_frame = first_frame || event_mutated || repaint_pending;
      bool eval_only_frame =
          (!render_frame && (event_requested || event_eval_requested));
      bool frame_mutated = event_mutated;

      if (!render_frame && !eval_only_frame)
        continue;
      first_frame = false;

      stygian_mouse_pos(window, &app.last_mouse_x, &app.last_mouse_y);
      stygian_begin_frame_intent(
          app.ctx, win_w, win_h,
          eval_only_frame ? STYGIAN_FRAME_EVAL_ONLY : STYGIAN_FRAME_RENDER);

      (void)app_draw_viewport(&app);
      if (app_draw_properties_panel(&app))
        frame_mutated = true;
      if (app_draw_top_chrome(&app, window, app.last_mouse_x, app.last_mouse_y))
        frame_mutated = true;

      if (frame_mutated) {
        stygian_set_repaint_source(app.ctx, "editor_mutation");
        stygian_request_repaint_after_ms(app.ctx, 0u);
      }

      stygian_widgets_commit_regions();
      stygian_end_frame(app.ctx);
    }
  }

  if (app.editor)
    stygian_editor_destroy(app.editor);
  if (app.font)
    stygian_font_destroy(app.ctx, app.font);
  stygian_destroy(app.ctx);
  stygian_window_destroy(window);
  return 0;
}
