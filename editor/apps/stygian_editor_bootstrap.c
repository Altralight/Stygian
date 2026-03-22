#include "../../include/stygian.h"
#include "../include/stygian_editor.h"
#include "../../layout/stygian_dock.h"
#include "../../widgets/stygian_widgets.h"
#include "../../window/stygian_input.h"
#include "../../window/stygian_window.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#endif

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
  float timeline_x;
  float timeline_y;
  float timeline_w;
  float timeline_h;
  float timeline_lane_x;
  float timeline_lane_y;
  float timeline_lane_w;
  float timeline_lane_h;
  float timeline_curve_x;
  float timeline_curve_y;
  float timeline_curve_w;
  float timeline_curve_h;
  uint32_t timeline_max_time_ms;

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

typedef enum AppTool {
  APP_TOOL_SELECT = 0,
  APP_TOOL_RECT_FRAME = 1,
  APP_TOOL_ELLIPSE = 2,
  APP_TOOL_PATH = 3,
  APP_TOOL_PAN = 4,
  APP_TOOL_ZOOM = 5,
  APP_TOOL_TEXT = 6,
  APP_TOOL_IMAGE = 7
} AppTool;

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

typedef enum AppSelfHostStage {
  APP_SELF_HOST_STAGE_1 = 1,
  APP_SELF_HOST_STAGE_2 = 2,
  APP_SELF_HOST_STAGE_3 = 3,
} AppSelfHostStage;

typedef enum AppComplexityMode {
  APP_COMPLEXITY_BEGINNER = 0,
  APP_COMPLEXITY_SYSTEMS = 1,
} AppComplexityMode;

#define APP_MAX_TRANSFORM_SELECTION 8192u
#define APP_MAX_NODE_FLAGS 16384u
#define APP_CLIPBOARD_NODE_CAP 4096u
#define APP_PERF_LOAD_BUDGET_MS 80.0f
#define APP_PERF_LAYOUT_BUDGET_MS 380.0f
#define APP_PERF_EXPORT_BUDGET_MS 80.0f
#define APP_PERF_INTERACTION_BUDGET_MS 80.0f

typedef struct EditorBootstrapApp {
  StygianContext *ctx;
  StygianEditor *editor;
  StygianDockSpace *dock;
  StygianFont font;
  EditorLayout layout;

  uint32_t panel_scene_layers_id;
  uint32_t panel_assets_id;
  uint32_t panel_viewport_id;
  uint32_t panel_inspector_id;
  uint32_t panel_timeline_id;
  uint32_t panel_logic_id;
  uint32_t panel_diagnostics_id;
  uint32_t panel_code_insight_id;

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
  bool rotating_selection;
  uint32_t rotate_selection_count;
  StygianEditorNodeId rotate_selection_ids[APP_MAX_TRANSFORM_SELECTION];
  float rotate_selection_start_x[APP_MAX_TRANSFORM_SELECTION];
  float rotate_selection_start_y[APP_MAX_TRANSFORM_SELECTION];
  float rotate_selection_start_w[APP_MAX_TRANSFORM_SELECTION];
  float rotate_selection_start_h[APP_MAX_TRANSFORM_SELECTION];
  float rotate_selection_start_rotation[APP_MAX_TRANSFORM_SELECTION];
  float rotate_group_center_x;
  float rotate_group_center_y;
  float rotate_start_angle;
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
  bool placing_primitive;
  float place_start_view_x;
  float place_start_view_y;
  float place_curr_view_x;
  float place_curr_view_y;

  bool panning;

  bool path_tool_active;
  StygianEditorPathId active_path;

  AppTool current_tool;
  AppPrimitiveKind selected_primitive;
  bool file_menu_open;
  int active_menu_index;
  bool primitive_menu_open;
  bool context_menu_open;
  float context_menu_x;
  float context_menu_y;
  float context_menu_world_x;
  float context_menu_world_y;
  bool command_palette_open;
  bool shortcut_help_open;
  int primitive_menu_hover_index;
  bool left_mouse_was_down;
  AppComplexityMode complexity_mode;
  bool code_insight_enabled;
  StygianEditorNodeId code_insight_node;
  char code_insight_text[2048];
  int code_view_mode;

  bool grid_enabled;
  bool sub_snap_enabled;
  float major_step_px;
  float snap_tolerance_px;
  bool snap_feedback_active;
  float snap_feedback_view_x;
  float snap_feedback_view_y;
  char snap_feedback_text[96];

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
  int behavior_action_choice;
  int behavior_property_choice;
  int behavior_easing_choice;
  int behavior_variable_kind_choice;
  bool behavior_variable_use_active_mode;
  char behavior_variable_name[32];
  char behavior_variable_number[24];
  char behavior_variable_mode[16];
  char behavior_navigate_target[64];

  StygianEditorNodeId demo_slider_node;
  StygianEditorNodeId demo_box_node;

  float preview_slider_value;
  char status_text[192];
  char behavior_graph_message[192];
  char project_path[260];
  char project_path_draft[260];
  char export_dir_draft[260];
  char import_image_source[260];
  char selected_name_text[96];
  char text_edit_buffer[1024];
  StygianEditorNodeId text_edit_node_id;
  StygianEditorNodeId component_edit_node_id;
  char component_symbol_text[64];
  char component_variant_group_text[32];
  char component_variant_name_text[32];
  char component_property_name_text[32];
  char component_property_default_enum_text[32];
  char component_property_options_text[128];
  int component_property_type_choice;
  bool component_property_default_bool;
  StygianEditorNodeId fx_edit_node_id;
  uint32_t fx_selected_effect_index;
  char mask_node_id_text[16];
  char shader_slot_text[32];
  char shader_asset_path_text[128];
  char shader_entry_point_text[64];
  bool shader_enabled;
  char recent_project_paths[6][260];
  uint32_t recent_project_count;
  char repeater_count[16];
  char repeater_step_x[24];
  char repeater_step_y[24];
  int boolean_op_choice;
  bool boolean_flatten_after;
  bool boolean_request_smooth_fallback;
  bool snap_use_grid;
  bool snap_use_guides;
  bool snap_use_bounds;
  bool snap_use_parent_edges;
  StygianEditorPoint path_preview_points[4096];
  uint32_t path_preview_count;
  float perf_last_load_ms;
  float perf_last_layout_ms;
  float perf_last_export_ms;
  float perf_last_interaction_ms;
  StygianEditorNodeId scene_parent_focus;
  uint32_t timeline_selected_track_index;
  uint32_t timeline_selected_clip_index;
  uint32_t timeline_selected_keyframe_index;
  int motion_operator_choice;
  char motion_start_value[24];
  char motion_end_value[24];
  char motion_duration_ms[16];
  char motion_cycles[16];
  char motion_step_ms[16];
  char driver_variable_name[32];
  char driver_source_node_id[16];
  int driver_target_property_choice;
  char timeline_track_name[32];
  char timeline_track_target_id[16];
  int timeline_track_property_choice;
  char timeline_cursor_ms[16];
  char timeline_key_value[24];
  int timeline_key_easing_choice;
  char timeline_clip_name[32];
  char timeline_clip_start_ms[16];
  char timeline_clip_duration_ms[16];
  char timeline_clip_layer[16];
  uint32_t timeline_view_start_ms;
  uint32_t timeline_view_span_ms;
  bool timeline_scrubbing;
  bool timeline_dragging_key;
  bool timeline_dragging_curve;
  uint32_t timeline_drag_track_index;
  uint32_t timeline_drag_key_index;

  struct {
    uint32_t count;
    float anchor_min_x;
    float anchor_min_y;
    struct {
      StygianEditorShapeKind kind;
      int32_t parent_index;
      bool selected_root;
      float x;
      float y;
      float w;
      float h;
      float rotation;
      float opacity;
      float radius[4];
      bool visible;
      bool clip_content;
      float z;
      uint32_t fit_mode;
      char name[96];
      char text[256];
      float font_size;
      float line_height;
      float letter_spacing;
      uint32_t font_weight;
      StygianEditorTextBoxMode box_mode;
      StygianEditorTextHAlign align_h;
      StygianEditorTextVAlign align_v;
      StygianEditorTextAutoSize auto_size;
      char image_source[260];
      uint32_t fill_count;
      StygianEditorNodeFill fills[STYGIAN_EDITOR_NODE_FILL_CAP];
      uint32_t stroke_count;
      StygianEditorNodeStroke strokes[STYGIAN_EDITOR_NODE_STROKE_CAP];
      uint32_t effect_count;
      StygianEditorNodeEffect effects[STYGIAN_EDITOR_NODE_EFFECT_CAP];
      uint32_t text_span_count;
      StygianEditorTextStyleSpan spans[STYGIAN_EDITOR_TEXT_SPAN_CAP];
      uint32_t path_point_count;
      StygianEditorPoint path_points[64];
      bool path_closed;
      float path_thickness;
      StygianEditorColor path_stroke;
    } nodes[APP_CLIPBOARD_NODE_CAP];
  } clipboard;

  uint64_t now_ms;
} EditorBootstrapApp;

static EditorBootstrapApp *g_editor_bootstrap_app = NULL;

static void app_draw_panel_shell(EditorBootstrapApp *app, float x, float y,
                                 float w, float h, float accent_r,
                                 float accent_g, float accent_b);
static void app_draw_panel_card(EditorBootstrapApp *app, float x, float y,
                                float w, float h);
static void app_draw_badge(EditorBootstrapApp *app, float x, float y, float w,
                           float h, float r, float g, float b,
                           const char *label);
static bool app_export_runtime_bundle(EditorBootstrapApp *app,
                                      const char *output_dir,
                                      const char *report_path);
static bool app_start_path_tool(EditorBootstrapApp *app);
static void app_commit_path_tool(EditorBootstrapApp *app);
static void app_cancel_path_tool(EditorBootstrapApp *app);
static void app_draw_placement_preview(EditorBootstrapApp *app);
static void app_draw_context_menu(EditorBootstrapApp *app);
static uint32_t app_text_line_for_substring(const char *text,
                                            const char *needle);
static bool app_run_binding_doctor(EditorBootstrapApp *app,
                                   const char *report_path,
                                   const char *hook_path);
static bool app_handle_timeline_event(EditorBootstrapApp *app,
                                      const StygianEvent *event);
static uint32_t app_selected_nodes(EditorBootstrapApp *app,
                                   StygianEditorNodeId *out_ids,
                                   uint32_t max_ids);
static bool app_read_text_file(const char *path, char *out, size_t out_capacity);
static bool app_hook_function_exists(const char *hooks_source,
                                     const char *function_name);
static bool app_build_node_path(const EditorBootstrapApp *app,
                                StygianEditorNodeId node_id, char *out_path,
                                size_t out_path_capacity);
static bool app_parse_u32(const char *text, uint32_t *out_value);
static bool app_parse_u32_or_default(const char *text, uint32_t fallback,
                                     uint32_t *out_value);
static bool app_parse_float_text(const char *text, float *out_value);
static bool app_apply_repeater_from_selection(EditorBootstrapApp *app);

static uint64_t app_now_ms_system(void) {
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}

static bool app_path_up_one(char *path) {
  char *slash = NULL;
  if (!path || !path[0])
    return false;
  slash = strrchr(path, '/');
  if (!slash)
    return false;
  *slash = '\0';
  return true;
}

static bool app_resolve_repo_resource_paths(char *out_shader_dir,
                                            size_t out_shader_dir_size,
                                            char *out_atlas_png,
                                            size_t out_atlas_png_size,
                                            char *out_atlas_json,
                                            size_t out_atlas_json_size) {
#ifdef _WIN32
  char module_path[1024];
  DWORD len = GetModuleFileNameA(NULL, module_path, (DWORD)sizeof(module_path));
  if (len == 0 || len >= (DWORD)sizeof(module_path))
    return false;
  module_path[len] = '\0';
  for (char *p = module_path; *p; ++p) {
    if (*p == '\\')
      *p = '/';
  }

  // module_path starts as .../editor/build/windows/<exe>; climb to repo root.
  if (!app_path_up_one(module_path) || !app_path_up_one(module_path) ||
      !app_path_up_one(module_path) || !app_path_up_one(module_path)) {
    return false;
  }

  snprintf(out_shader_dir, out_shader_dir_size, "%s/shaders", module_path);
  snprintf(out_atlas_png, out_atlas_png_size, "%s/assets/atlas.png",
           module_path);
  snprintf(out_atlas_json, out_atlas_json_size, "%s/assets/atlas.json",
           module_path);
  return true;
#else
  (void)out_shader_dir;
  (void)out_shader_dir_size;
  (void)out_atlas_png;
  (void)out_atlas_png_size;
  (void)out_atlas_json;
  (void)out_atlas_json_size;
  return false;
#endif
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

static const char *app_fill_kind_name(StygianEditorFillKind kind) {
  switch (kind) {
  case STYGIAN_EDITOR_FILL_LINEAR_GRADIENT:
    return "Linear";
  case STYGIAN_EDITOR_FILL_RADIAL_GRADIENT:
    return "Radial";
  case STYGIAN_EDITOR_FILL_IMAGE:
    return "Image";
  case STYGIAN_EDITOR_FILL_SOLID:
  default:
    return "Solid";
  }
}

static const char *app_effect_kind_name(StygianEditorEffectKind kind) {
  switch (kind) {
  case STYGIAN_EDITOR_EFFECT_DROP_SHADOW:
    return "Drop Shadow";
  case STYGIAN_EDITOR_EFFECT_INNER_SHADOW:
    return "Inner Shadow";
  case STYGIAN_EDITOR_EFFECT_LAYER_BLUR:
    return "Layer Blur";
  case STYGIAN_EDITOR_EFFECT_GLOW:
    return "Glow";
  case STYGIAN_EDITOR_EFFECT_NOISE:
    return "Noise";
  default:
    return "Effect";
  }
}

static const char *app_image_fit_name(uint32_t fit_mode) {
  switch (fit_mode) {
  case 1u:
    return "Contain";
  case 2u:
    return "Cover";
  case 3u:
    return "Stretch";
  default:
    return "Fill";
  }
}

static void app_push_recent_project(EditorBootstrapApp *app, const char *path) {
  uint32_t insert_at = 0u;
  uint32_t i;
  if (!app || !path || !path[0])
    return;
  for (i = 0u; i < app->recent_project_count; ++i) {
    if (_stricmp(app->recent_project_paths[i], path) == 0) {
      insert_at = i;
      break;
    }
  }
  if (insert_at < app->recent_project_count) {
    for (i = insert_at; i > 0u; --i)
      snprintf(app->recent_project_paths[i], sizeof(app->recent_project_paths[i]),
               "%s", app->recent_project_paths[i - 1u]);
  } else {
    if (app->recent_project_count < 6u)
      app->recent_project_count += 1u;
    for (i = app->recent_project_count - 1u; i > 0u; --i)
      snprintf(app->recent_project_paths[i], sizeof(app->recent_project_paths[i]),
               "%s", app->recent_project_paths[i - 1u]);
  }
  snprintf(app->recent_project_paths[0], sizeof(app->recent_project_paths[0]),
           "%s", path);
}

static void app_sync_project_drafts(EditorBootstrapApp *app) {
  if (!app)
    return;
  if (!app->project_path_draft[0] && app->project_path[0]) {
    snprintf(app->project_path_draft, sizeof(app->project_path_draft), "%s",
             app->project_path);
  }
  if (!app->export_dir_draft[0]) {
    snprintf(app->export_dir_draft, sizeof(app->export_dir_draft),
             "editor/build/windows/stygian_export");
  }
}

static StygianEditorNodeId app_selected_single_node(EditorBootstrapApp *app) {
  StygianEditorNodeId ids[2];
  if (!app || !app->editor)
    return STYGIAN_EDITOR_INVALID_ID;
  if (app_selected_nodes(app, ids, 2u) != 1u)
    return STYGIAN_EDITOR_INVALID_ID;
  return ids[0];
}

static void app_sync_selected_name(EditorBootstrapApp *app) {
  StygianEditorNodeId id = app_selected_single_node(app);
  char name[96];
  if (!app)
    return;
  if (id == STYGIAN_EDITOR_INVALID_ID) {
    app->selected_name_text[0] = '\0';
    return;
  }
  if (stygian_editor_node_get_name(app->editor, id, name, sizeof(name)) &&
      name[0]) {
    snprintf(app->selected_name_text, sizeof(app->selected_name_text), "%s",
             name);
  } else {
    app->selected_name_text[0] = '\0';
  }
}

static void app_sync_text_editor_buffer(EditorBootstrapApp *app) {
  StygianEditorNodeId id = app_selected_single_node(app);
  StygianEditorShapeKind kind = STYGIAN_EDITOR_SHAPE_RECT;
  if (!app || !app->editor) 
    return;
  if (id == STYGIAN_EDITOR_INVALID_ID ||
      !stygian_editor_node_get_shape_kind(app->editor, id, &kind) ||
      kind != STYGIAN_EDITOR_SHAPE_TEXT) {
    app->text_edit_node_id = STYGIAN_EDITOR_INVALID_ID;
    app->text_edit_buffer[0] = '\0';
    return;
  }
  if (app->text_edit_node_id == id)
    return;
  app->text_edit_node_id = id;
  if (!stygian_editor_node_get_text_content(app->editor, id, app->text_edit_buffer,
                                            sizeof(app->text_edit_buffer))) {
    app->text_edit_buffer[0] = '\0';
  }
}

static void app_sync_fx_editor_buffers(EditorBootstrapApp *app) {
  StygianEditorNodeId id = app_selected_single_node(app);
  StygianEditorShaderAttachment shader = {0};
  StygianEditorNodeId mask_id = STYGIAN_EDITOR_INVALID_ID;
  if (!app || !app->editor) {
    return;
  }
  if (id == STYGIAN_EDITOR_INVALID_ID) {
    app->fx_edit_node_id = STYGIAN_EDITOR_INVALID_ID;
    app->fx_selected_effect_index = 0u;
    app->mask_node_id_text[0] = '\0';
    app->shader_slot_text[0] = '\0';
    app->shader_asset_path_text[0] = '\0';
    app->shader_entry_point_text[0] = '\0';
    app->shader_enabled = false;
    return;
  }
  if (app->fx_edit_node_id == id) {
    return;
  }
  app->fx_edit_node_id = id;
  if (app->fx_selected_effect_index >= stygian_editor_node_effect_count(app->editor, id))
    app->fx_selected_effect_index = 0u;
  if (stygian_editor_node_get_mask(app->editor, id, &mask_id, NULL, NULL) &&
      mask_id != STYGIAN_EDITOR_INVALID_ID) {
    snprintf(app->mask_node_id_text, sizeof(app->mask_node_id_text), "%u", mask_id);
  } else {
    app->mask_node_id_text[0] = '\0';
  }
  if (stygian_editor_node_get_shader_attachment(app->editor, id, &shader)) {
    snprintf(app->shader_slot_text, sizeof(app->shader_slot_text), "%s",
             shader.slot_name);
    snprintf(app->shader_asset_path_text, sizeof(app->shader_asset_path_text), "%s",
             shader.asset_path);
    snprintf(app->shader_entry_point_text, sizeof(app->shader_entry_point_text), "%s",
             shader.entry_point);
    app->shader_enabled = shader.enabled;
  } else {
    app->shader_slot_text[0] = '\0';
    app->shader_asset_path_text[0] = '\0';
    app->shader_entry_point_text[0] = '\0';
    app->shader_enabled = false;
  }
}

static void app_sync_component_editor_buffer(EditorBootstrapApp *app) {
  StygianEditorNodeId id = app_selected_single_node(app);
  StygianEditorShapeKind kind = STYGIAN_EDITOR_SHAPE_RECT;
  StygianEditorNodeId def_id = STYGIAN_EDITOR_INVALID_ID;
  if (!app || !app->editor)
    return;
  if (id == STYGIAN_EDITOR_INVALID_ID ||
      !stygian_editor_node_get_shape_kind(app->editor, id, &kind) ||
      (kind != STYGIAN_EDITOR_SHAPE_COMPONENT_DEF &&
       kind != STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE)) {
    app->component_edit_node_id = STYGIAN_EDITOR_INVALID_ID;
    app->component_symbol_text[0] = '\0';
    app->component_variant_group_text[0] = '\0';
    app->component_variant_name_text[0] = '\0';
    return;
  }
  if (app->component_edit_node_id == id)
    return;
  app->component_edit_node_id = id;
  def_id = id;
  if (kind == STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE &&
      !stygian_editor_component_instance_get_def(app->editor, id, &def_id)) {
    def_id = STYGIAN_EDITOR_INVALID_ID;
  }
  app->component_symbol_text[0] = '\0';
  app->component_variant_group_text[0] = '\0';
  app->component_variant_name_text[0] = '\0';
  if (def_id != STYGIAN_EDITOR_INVALID_ID) {
    (void)stygian_editor_component_def_get_symbol(
        app->editor, def_id, app->component_symbol_text,
        sizeof(app->component_symbol_text));
    (void)stygian_editor_component_def_get_variant(
        app->editor, def_id, app->component_variant_group_text,
        sizeof(app->component_variant_group_text), app->component_variant_name_text,
        sizeof(app->component_variant_name_text));
  }
}

static StygianEditorNodeId app_add_component_def_node(EditorBootstrapApp *app,
                                                      float world_x,
                                                      float world_y,
                                                      const char *symbol) {
  StygianEditorComponentDefDesc desc = {0};
  char auto_symbol[64];
  if (!app || !app->editor)
    return STYGIAN_EDITOR_INVALID_ID;
  desc.x = world_x;
  desc.y = world_y;
  desc.w = 180.0f;
  desc.h = 56.0f;
  if (!symbol || !symbol[0]) {
    snprintf(auto_symbol, sizeof(auto_symbol), "component_%u",
             stygian_editor_node_count(app->editor) + 1u);
    desc.symbol = auto_symbol;
  } else {
    desc.symbol = symbol;
  }
  desc.visible = true;
  desc.z = 0.0f;
  return stygian_editor_add_component_def(app->editor, &desc);
}

static StygianEditorNodeId app_add_component_instance_node(
    EditorBootstrapApp *app, StygianEditorNodeId def_id, float world_x,
    float world_y) {
  StygianEditorComponentInstanceDesc desc = {0};
  if (!app || !app->editor || def_id == STYGIAN_EDITOR_INVALID_ID)
    return STYGIAN_EDITOR_INVALID_ID;
  desc.component_def_id = def_id;
  desc.x = world_x;
  desc.y = world_y;
  desc.w = 0.0f;
  desc.h = 0.0f;
  desc.visible = true;
  desc.z = 0.0f;
  return stygian_editor_add_component_instance(app->editor, &desc);
}

static StygianEditorNodeId app_component_def_for_selection(EditorBootstrapApp *app) {
  StygianEditorNodeId selected = app_selected_single_node(app);
  StygianEditorShapeKind kind = STYGIAN_EDITOR_SHAPE_RECT;
  if (!app || !app->editor || selected == STYGIAN_EDITOR_INVALID_ID)
    return STYGIAN_EDITOR_INVALID_ID;
  if (!stygian_editor_node_get_shape_kind(app->editor, selected, &kind))
    return STYGIAN_EDITOR_INVALID_ID;
  if (kind == STYGIAN_EDITOR_SHAPE_COMPONENT_DEF)
    return selected;
  if (kind == STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE) {
    StygianEditorNodeId def_id = STYGIAN_EDITOR_INVALID_ID;
    if (stygian_editor_component_instance_get_def(app->editor, selected, &def_id))
      return def_id;
  }
  return STYGIAN_EDITOR_INVALID_ID;
}

static bool app_seed_component_variant_family(EditorBootstrapApp *app,
                                              float world_x, float world_y,
                                              bool is_tabs) {
  StygianEditorNodeId defs[3] = {0};
  const char *group_name = is_tabs ? "tab_variant" : "toggle_variant";
  const char *property_name = is_tabs ? "state" : "checked";
  const char *symbols[3] = {is_tabs ? "tab_default" : "toggle_off",
                            is_tabs ? "tab_active" : "toggle_on",
                            is_tabs ? "tab_disabled" : "toggle_disabled"};
  const char *variant_names[3] = {is_tabs ? "default" : "off",
                                  is_tabs ? "active" : "on",
                                  "disabled"};
  uint32_t def_count = is_tabs ? 3u : 2u;
  for (uint32_t i = 0u; i < def_count; ++i) {
    StygianEditorComponentDefDesc def = {0};
    StygianEditorComponentPropertyDef prop = {0};
    def.x = world_x + (float)i * 220.0f;
    def.y = world_y;
    def.w = 180.0f;
    def.h = 56.0f;
    def.symbol = symbols[i];
    def.visible = true;
    defs[i] = stygian_editor_add_component_def(app->editor, &def);
    if (!defs[i])
      return false;
    if (!stygian_editor_node_set_name(app->editor, defs[i], symbols[i]))
      return false;
    if (!stygian_editor_component_def_set_variant(app->editor, defs[i], group_name,
                                                  variant_names[i])) {
      return false;
    }
    snprintf(prop.name, sizeof(prop.name), "%s", property_name);
    if (is_tabs) {
      prop.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
      prop.enum_option_count = 3u;
      snprintf(prop.enum_options[0], sizeof(prop.enum_options[0]), "default");
      snprintf(prop.enum_options[1], sizeof(prop.enum_options[1]), "active");
      snprintf(prop.enum_options[2], sizeof(prop.enum_options[2]), "disabled");
      snprintf(prop.default_enum, sizeof(prop.default_enum), "%s",
               variant_names[i]);
    } else {
      prop.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_BOOL;
      prop.default_bool = (i == 1u);
    }
    if (!stygian_editor_component_def_set_property(app->editor, defs[i], &prop))
      return false;
  }
  if (is_tabs) {
    StygianEditorComponentPropertyDef bool_prop = {0};
    snprintf(bool_prop.name, sizeof(bool_prop.name), "disabled");
    bool_prop.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_BOOL;
    bool_prop.default_bool = false;
    for (uint32_t i = 0u; i < def_count; ++i) {
      if (!stygian_editor_component_def_set_property(app->editor, defs[i], &bool_prop))
        return false;
    }
  }
  if (defs[0]) {
    StygianEditorNodeId inst =
        app_add_component_instance_node(app, defs[0], world_x, world_y + 120.0f);
    if (!inst)
      return false;
    if (!stygian_editor_node_set_name(app->editor, inst,
                                      is_tabs ? "tab_instance" : "toggle_instance"))
      return false;
    (void)stygian_editor_select_node(app->editor, inst, false);
    app_sync_selected_name(app);
    app_sync_component_editor_buffer(app);
  }
  return true;
}

static void app_component_fill_options_from_csv(
    StygianEditorComponentPropertyDef *prop, const char *csv) {
  const char *at = csv;
  uint32_t count = 0u;
  if (!prop)
    return;
  prop->enum_option_count = 0u;
  while (at && *at && count < STYGIAN_EDITOR_COMPONENT_ENUM_OPTION_CAP) {
    const char *comma = strchr(at, ',');
    size_t len = comma ? (size_t)(comma - at) : strlen(at);
    while (len > 0u && (*at == ' ' || *at == '\t')) {
      at += 1;
      len -= 1u;
    }
    while (len > 0u &&
           (at[len - 1u] == ' ' || at[len - 1u] == '\t')) {
      len -= 1u;
    }
    if (len > 0u) {
      size_t copy_len = len;
      if (copy_len >= sizeof(prop->enum_options[count]))
        copy_len = sizeof(prop->enum_options[count]) - 1u;
      memcpy(prop->enum_options[count], at, copy_len);
      prop->enum_options[count][copy_len] = '\0';
      count += 1u;
    }
    at = comma ? comma + 1 : NULL;
  }
  prop->enum_option_count = count;
}

static const char *app_component_property_type_name(
    StygianEditorComponentPropertyType type) {
  return type == STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM ? "enum" : "bool";
}

static StygianEditorNodeId app_add_text_node(EditorBootstrapApp *app,
                                             float world_x, float world_y,
                                             const char *text) {
  StygianEditorTextDesc desc = {0};
  if (!app || !app->editor)
    return STYGIAN_EDITOR_INVALID_ID;
  desc.x = world_x;
  desc.y = world_y;
  desc.w = 240.0f;
  desc.h = 72.0f;
  desc.font_size = 24.0f;
  desc.line_height = 28.0f;
  desc.letter_spacing = 0.0f;
  desc.font_weight = 400u;
  desc.box_mode = STYGIAN_EDITOR_TEXT_BOX_AREA;
  desc.align_h = STYGIAN_EDITOR_TEXT_ALIGN_LEFT;
  desc.align_v = STYGIAN_EDITOR_TEXT_ALIGN_TOP;
  desc.auto_size = STYGIAN_EDITOR_TEXT_AUTOSIZE_HEIGHT;
  desc.fill = stygian_editor_color_rgba(0.92f, 0.94f, 0.98f, 1.0f);
  desc.text = (text && text[0]) ? text : "Text";
  desc.visible = true;
  desc.z = 0.0f;
  return stygian_editor_add_text(app->editor, &desc);
}

static StygianEditorNodeId app_add_image_node(EditorBootstrapApp *app,
                                              float world_x, float world_y,
                                              const char *source) {
  StygianEditorImageDesc desc = {0};
  if (!app || !app->editor)
    return STYGIAN_EDITOR_INVALID_ID;
  desc.x = world_x;
  desc.y = world_y;
  desc.w = 220.0f;
  desc.h = 160.0f;
  desc.fit_mode = 2u;
  desc.source = (source && source[0]) ? source : "assets/atlas.png";
  desc.visible = true;
  desc.z = 0.0f;
  return stygian_editor_add_image(app->editor, &desc);
}

static const char *app_motion_operator_code(int choice) {
  switch (choice) {
  case 0:
    return "loop";
  case 1:
    return "ping";
  case 2:
    return "stag";
  case 3:
    return "dly";
  case 4:
    return "roff";
  default:
    return "loop";
  }
}

static StygianEditorPropertyKind app_motion_operator_property(EditorBootstrapApp *app) {
  if (!app)
    return STYGIAN_EDITOR_PROP_X;
  switch (app->timeline_track_property_choice) {
  case 0:
    return STYGIAN_EDITOR_PROP_X;
  case 1:
    return STYGIAN_EDITOR_PROP_Y;
  case 2:
    return STYGIAN_EDITOR_PROP_WIDTH;
  case 3:
    return STYGIAN_EDITOR_PROP_HEIGHT;
  case 4:
    return STYGIAN_EDITOR_PROP_OPACITY;
  default:
    return STYGIAN_EDITOR_PROP_X;
  }
}

static float app_node_property_or_default(EditorBootstrapApp *app,
                                          StygianEditorNodeId node_id,
                                          StygianEditorPropertyKind property,
                                          float fallback) {
  StygianEditorPropertyValue value = {0};
  if (!app || !app->editor)
    return fallback;
  if (!stygian_editor_node_get_property_value(app->editor, node_id, property, &value))
    return fallback;
  if (value.type != STYGIAN_EDITOR_VALUE_FLOAT)
    return fallback;
  return value.as.number;
}

static void app_remove_controller_tracks(EditorBootstrapApp *app,
                                         StygianEditorNodeId node_id,
                                         const char *prefix,
                                         StygianEditorPropertyKind property) {
  StygianEditorTimelineTrack track = {0};
  uint32_t count;
  if (!app || !app->editor || !prefix)
    return;
  count = stygian_editor_timeline_track_count(app->editor);
  while (count > 0u) {
    count -= 1u;
    if (!stygian_editor_timeline_get_track(app->editor, count, &track))
      continue;
    if (track.target_node == node_id && track.property == property &&
        strncmp(track.name, prefix, strlen(prefix)) == 0) {
      (void)stygian_editor_timeline_remove_track(app->editor, track.id);
    }
  }
}

static bool app_apply_motion_operator(EditorBootstrapApp *app, int choice) {
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count = app_selected_nodes(app, ids, APP_MAX_TRANSFORM_SELECTION);
  StygianEditorPropertyKind property = app_motion_operator_property(app);
  const char *code = app_motion_operator_code(choice);
  uint32_t duration_ms = 0u;
  uint32_t cycles = 0u;
  uint32_t step_ms = 0u;
  float end_value = 0.0f;
  uint32_t applied = 0u;
  char prefix[24];

  if (!app || !app->editor || count == 0u)
    return false;
  if (!app_parse_u32_or_default(app->motion_duration_ms, 600u, &duration_ms))
    duration_ms = 600u;
  if (!app_parse_u32_or_default(app->motion_cycles, 2u, &cycles))
    cycles = 2u;
  if (!app_parse_u32_or_default(app->motion_step_ms, 120u, &step_ms))
    step_ms = 120u;
  if (!app_parse_float_text(app->motion_end_value, &end_value))
    return false;
  if (cycles == 0u)
    cycles = 1u;
  snprintf(prefix, sizeof(prefix), "ctl:%s:", code);

  if (choice == 4) {
    if (!app_apply_repeater_from_selection(app))
      return false;
    choice = 2;
    code = app_motion_operator_code(choice);
    snprintf(prefix, sizeof(prefix), "ctl:%s:", code);
  }

  for (uint32_t i = 0u; i < count; ++i) {
    StygianEditorTimelineTrack track = {0};
    StygianEditorTimelineClip clip = {0};
    StygianEditorTimelineKeyframe keys[STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP];
    float start_value =
        app_node_property_or_default(app, ids[i], property, 0.0f);
    uint32_t key_count = 0u;
    uint32_t base_offset = 0u;

    if (app_parse_float_text(app->motion_start_value, &start_value)) {
      // Shared authored start values matter when the selection should move as one system.
    }

    app_remove_controller_tracks(app, ids[i], prefix, property);
    memset(&track, 0, sizeof(track));
    track.target_node = ids[i];
    track.property = property;
    track.layer = i;
    snprintf(track.name, sizeof(track.name), "ctl:%s:%u", code, ids[i]);
    if (!stygian_editor_timeline_add_track(app->editor, &track, &track.id))
      continue;

    if (choice == 2)
      base_offset = i * step_ms;
    else if (choice == 3)
      base_offset = step_ms;

    for (uint32_t cycle = 0u; cycle < cycles; ++cycle) {
      uint32_t cycle_start = base_offset + cycle * duration_ms;
      if (key_count < STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP) {
        keys[key_count].time_ms = cycle_start;
        keys[key_count].value = start_value;
        keys[key_count].easing = STYGIAN_EDITOR_EASING_LINEAR;
        key_count += 1u;
      }
      if (choice == 1) {
        if (key_count < STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP) {
          keys[key_count].time_ms = cycle_start + duration_ms / 2u;
          keys[key_count].value = end_value;
          keys[key_count].easing = STYGIAN_EDITOR_EASING_OUT_CUBIC;
          key_count += 1u;
        }
        if (key_count < STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP) {
          keys[key_count].time_ms = cycle_start + duration_ms;
          keys[key_count].value = start_value;
          keys[key_count].easing = STYGIAN_EDITOR_EASING_IN_OUT_CUBIC;
          key_count += 1u;
        }
      } else {
        if (key_count < STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP) {
          keys[key_count].time_ms = cycle_start + duration_ms;
          keys[key_count].value = end_value;
          keys[key_count].easing = STYGIAN_EDITOR_EASING_IN_OUT_CUBIC;
          key_count += 1u;
        }
      }
    }
    if (!stygian_editor_timeline_set_track_keyframes(app->editor, track.id, keys,
                                                      key_count))
      continue;
    memset(&clip, 0, sizeof(clip));
    snprintf(clip.name, sizeof(clip.name), "ctl:%s:%u", code, ids[i]);
    clip.start_ms = base_offset;
    clip.duration_ms = duration_ms * cycles;
    clip.layer = i;
    clip.track_count = 1u;
    clip.track_ids[0] = track.id;
    (void)stygian_editor_timeline_add_clip(app->editor, &clip, NULL);
    applied += 1u;
  }
  return applied > 0u;
}

static bool app_apply_motion_preset(EditorBootstrapApp *app, const char *preset) {
  StygianEditorNodeId target = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorTimelineTrack track = {0};
  StygianEditorTimelineClip clip = {0};
  StygianEditorTimelineKeyframe keys[3];
  if (!app || !preset)
    return false;
  if (strcmp(preset, "pulse") == 0) {
    app->timeline_track_property_choice = 4;
    snprintf(app->motion_end_value, sizeof(app->motion_end_value), "0.35");
    snprintf(app->motion_duration_ms, sizeof(app->motion_duration_ms), "480");
    snprintf(app->motion_cycles, sizeof(app->motion_cycles), "2");
    app->motion_operator_choice = 1;
  } else if (strcmp(preset, "slide") == 0) {
    app->timeline_track_property_choice = 1;
    snprintf(app->motion_end_value, sizeof(app->motion_end_value), "60");
    snprintf(app->motion_duration_ms, sizeof(app->motion_duration_ms), "420");
    snprintf(app->motion_cycles, sizeof(app->motion_cycles), "1");
    app->motion_operator_choice = 0;
  } else {
    target = app_selected_single_node(app);
    if (target == STYGIAN_EDITOR_INVALID_ID)
      return false;
    app_remove_controller_tracks(app, target, "ctl:spin:",
                                 STYGIAN_EDITOR_PROP_ROTATION_DEG);
    memset(&track, 0, sizeof(track));
    track.target_node = target;
    track.property = STYGIAN_EDITOR_PROP_ROTATION_DEG;
    snprintf(track.name, sizeof(track.name), "ctl:spin:%u", target);
    if (!stygian_editor_timeline_add_track(app->editor, &track, &track.id))
      return false;
    memset(keys, 0, sizeof(keys));
    keys[0].time_ms = 0u;
    keys[0].value = 0.0f;
    keys[0].easing = STYGIAN_EDITOR_EASING_LINEAR;
    keys[1].time_ms = 600u;
    keys[1].value = 180.0f;
    keys[1].easing = STYGIAN_EDITOR_EASING_IN_OUT_CUBIC;
    keys[2].time_ms = 1200u;
    keys[2].value = 360.0f;
    keys[2].easing = STYGIAN_EDITOR_EASING_LINEAR;
    if (!stygian_editor_timeline_set_track_keyframes(app->editor, track.id, keys, 3u))
      return false;
    memset(&clip, 0, sizeof(clip));
    snprintf(clip.name, sizeof(clip.name), "ctl:spin:%u", target);
    clip.start_ms = 0u;
    clip.duration_ms = 1200u;
    clip.layer = 0u;
    clip.track_count = 1u;
    clip.track_ids[0] = track.id;
    (void)stygian_editor_timeline_add_clip(app->editor, &clip, NULL);
    return true;
  }
  return app_apply_motion_operator(app, app->motion_operator_choice);
}

static bool app_bind_driver_variable(EditorBootstrapApp *app) {
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count = app_selected_nodes(app, ids, APP_MAX_TRANSFORM_SELECTION);
  uint32_t source_id = 0u;
  StygianEditorPropertyKind target_property;
  StygianEditorDriverId driver_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorDriverRule driver = {0};
  float seed_value = 0.0f;
  uint32_t i;

  if (!app || !app->editor || count == 0u || !app->driver_variable_name[0])
    return false;
  if (!app_parse_u32(app->driver_source_node_id, &source_id) ||
      source_id == STYGIAN_EDITOR_INVALID_ID) {
    return false;
  }
  target_property = app_motion_operator_property(app);
  seed_value = app_node_property_or_default(app, source_id, STYGIAN_EDITOR_PROP_VALUE,
                                            0.0f);
  if (!stygian_editor_set_variable_mode(app->editor, 0u, "default"))
    return false;
  if (!stygian_editor_set_number_variable(app->editor, app->driver_variable_name, 0u,
                                          seed_value))
    return false;
  for (uint32_t i = 0u; i < count; ++i) {
    if (!stygian_editor_bind_node_number_variable(app->editor, ids[i], target_property,
                                                  app->driver_variable_name)) {
      return false;
    }
  }
  for (i = stygian_editor_driver_count(app->editor); i > 0u; --i) {
    StygianEditorDriverId driver_id = STYGIAN_EDITOR_INVALID_ID;
    StygianEditorDriverRule existing = {0};
    if (!stygian_editor_get_driver_rule(app->editor, i - 1u, &driver_id, &existing))
      continue;
    if (existing.source_node == source_id &&
        existing.source_property == STYGIAN_EDITOR_PROP_VALUE &&
        strcmp(existing.variable_name, app->driver_variable_name) == 0) {
      (void)stygian_editor_remove_driver(app->editor, driver_id);
    }
  }
  driver.source_node = source_id;
  driver.source_property = STYGIAN_EDITOR_PROP_VALUE;
  driver.use_active_mode = true;
  driver.mode_index = 0u;
  driver.in_min = 0.0f;
  driver.in_max = 1.0f;
  driver.out_min = 0.0f;
  driver.out_max = 1.0f;
  driver.clamp_output = true;
  snprintf(driver.variable_name, sizeof(driver.variable_name), "%s",
           app->driver_variable_name);
  if (!stygian_editor_add_driver(app->editor, &driver, &driver_id))
    return false;
  return stygian_editor_apply_driver_sample(app->editor, driver_id, seed_value);
}

static void app_sync_driver_rules(EditorBootstrapApp *app) {
  uint32_t count;
  uint32_t i;
  if (!app || !app->editor)
    return;
  count = stygian_editor_driver_count(app->editor);
  for (i = 0u; i < count; ++i) {
    StygianEditorDriverId driver_id = STYGIAN_EDITOR_INVALID_ID;
    StygianEditorDriverRule rule = {0};
    float sample = 0.0f;
    if (!stygian_editor_get_driver_rule(app->editor, i, &driver_id, &rule))
      continue;
    sample = app_node_property_or_default(app, rule.source_node, rule.source_property,
                                          0.0f);
    (void)stygian_editor_apply_driver_sample(app->editor, driver_id, sample);
  }
}

static void app_close_top_menus(EditorBootstrapApp *app) {
  if (!app)
    return;
  app->file_menu_open = false;
  app->active_menu_index = -1;
}

static void app_toggle_top_menu(EditorBootstrapApp *app, int menu_index) {
  if (!app)
    return;
  if (app->file_menu_open && app->active_menu_index == menu_index) {
    app_close_top_menus(app);
    return;
  }
  app->file_menu_open = true;
  app->active_menu_index = menu_index;
  app->primitive_menu_open = false;
}

#if 0
static const char *app_action_kind_name(StygianEditorBehaviorActionKind kind) {
  switch (kind) {
  case STYGIAN_EDITOR_ACTION_ANIMATE:
    return "Animate";
  case STYGIAN_EDITOR_ACTION_SET_PROPERTY:
    return "Set Property";
  case STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY:
    return "Toggle Visibility";
  case STYGIAN_EDITOR_ACTION_SET_VARIABLE:
    return "Set Variable";
  case STYGIAN_EDITOR_ACTION_NAVIGATE:
    return "Navigate";
  default:
    return "Unknown";
  }
}
#endif

static const char *app_hook_source_path(void) {
  return "editor/build/windows/stygian_export/stygian_editor_hooks.c";
}

static const char *app_generated_scene_path(void) {
  return "editor/build/windows/stygian_export/generated/stygian_editor_scene.generated.c";
}

static const char *app_diag_severity_name(StygianEditorDiagnosticSeverity sev) {
  switch (sev) {
  case STYGIAN_EDITOR_DIAG_ERROR:
    return "error";
  case STYGIAN_EDITOR_DIAG_WARNING:
    return "warning";
  default:
    return "info";
  }
}

static void app_node_path(EditorBootstrapApp *app, StygianEditorNodeId node_id,
                          char *out, size_t out_cap) {
  StygianEditorNodeId chain[32];
  uint32_t count = 0u;
  StygianEditorNodeId cursor = node_id;
  if (!out || out_cap < 2u)
    return;
  out[0] = '\0';
  if (!app || !app->editor || node_id == STYGIAN_EDITOR_INVALID_ID) {
    snprintf(out, out_cap, "root");
    return;
  }
  while (count < 32u && cursor != STYGIAN_EDITOR_INVALID_ID) {
    chain[count++] = cursor;
    if (!stygian_editor_node_get_parent(app->editor, cursor, &cursor))
      break;
  }
  if (count == 0u) {
    snprintf(out, out_cap, "root");
    return;
  }
  for (int i = (int)count - 1; i >= 0; --i) {
    char seg[64];
    char name[32];
    bool have_name = stygian_editor_node_get_name(app->editor, chain[i], name,
                                                  sizeof(name));
    snprintf(seg, sizeof(seg), "%s%s#%u",
             (i == (int)count - 1) ? "" : "/",
             (have_name && name[0]) ? name : "node", chain[i]);
    if (strlen(out) + strlen(seg) + 1u >= out_cap)
      break;
    strcat(out, seg);
  }
}

static bool app_draw_diagnostics_panel(EditorBootstrapApp *app, float panel_x,
                                       float panel_y, float panel_w,
                                       float panel_h) {
  StygianEditorExportDiagnostic diags[96];
  uint32_t diag_count = 0u;
  uint32_t errors = 0u;
  uint32_t warnings = 0u;
  uint32_t infos = 0u;
  float x = panel_x + 10.0f;
  float y = panel_y + 8.0f;
  float content_w = panel_w - 20.0f;
  bool mutated = false;

  app_draw_panel_shell(app, panel_x, panel_y, panel_w, panel_h, 0.72f, 0.43f,
                       0.31f);
  stygian_widgets_register_region(
      panel_x, panel_y, panel_w, panel_h,
      STYGIAN_WIDGET_REGION_POINTER_LEFT_MUTATES |
          STYGIAN_WIDGET_REGION_POINTER_RIGHT_MUTATES |
          STYGIAN_WIDGET_REGION_SCROLL);
  if (!app->font || !app->editor)
    return false;

  diag_count =
      stygian_editor_collect_export_diagnostics(app->editor, diags,
                                                (uint32_t)(sizeof(diags) /
                                                           sizeof(diags[0])));
  for (uint32_t i = 0u; i < diag_count; ++i) {
    if (diags[i].severity == STYGIAN_EDITOR_DIAG_ERROR)
      errors += 1u;
    else if (diags[i].severity == STYGIAN_EDITOR_DIAG_WARNING)
      warnings += 1u;
    else
      infos += 1u;
  }

  {
    char hdr[128];
    snprintf(hdr, sizeof(hdr), "Diagnostics (%u)  E:%u W:%u I:%u", diag_count,
             errors, warnings, infos);
    stygian_text(app->ctx, app->font, hdr, x, y, 16.0f, 0.93f, 0.95f, 1.0f, 1.0f);
  }
  stygian_text(app->ctx, app->font,
               "Export trouble should point to a node, a cause, and the next move.",
               x, y + 16.0f, 10.8f, 0.74f, 0.81f, 0.90f, 1.0f);
  y += 30.0f;

  app_draw_panel_card(app, x, y, content_w, 54.0f);
  app_draw_badge(app, x + 10.0f, y + 10.0f, 52.0f, 16.0f, 0.46f, 0.24f, 0.21f,
                 "Err");
  app_draw_badge(app, x + 68.0f, y + 10.0f, 60.0f, 16.0f, 0.46f, 0.35f, 0.18f,
                 "Warn");
  app_draw_badge(app, x + 134.0f, y + 10.0f, 52.0f, 16.0f, 0.21f, 0.34f, 0.42f,
                 "Info");
  stygian_text(app->ctx, app->font,
               app->status_text[0] ? app->status_text : "No recent action.",
               x + 10.0f, y + 31.0f, 10.5f, 0.76f, 0.84f, 0.93f, 1.0f);
  if (stygian_button(app->ctx, app->font, "Export", x + content_w - 122.0f,
                     y + 24.0f, 54.0f, 20.0f)) {
    if (app_export_runtime_bundle(
            app, "editor/build/windows/stygian_export",
            "editor/build/windows/stygian_export/stygian_export_report.txt")) {
      app_set_status(app, "Exported runtime bundle.");
    } else {
      app_set_status(app, "Runtime bundle export failed.");
    }
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, "Doctor", x + content_w - 62.0f,
                     y + 24.0f, 52.0f, 20.0f)) {
    if (app_run_binding_doctor(
            app, "editor/build/windows/stygian_export/stygian_binding_doctor.txt",
            "editor/build/windows/stygian_export/stygian_editor_hooks.c")) {
      app_set_status(app, "Binding doctor report written.");
    } else {
      app_set_status(app, "Binding doctor failed.");
    }
    mutated = true;
  }
  y += 64.0f;

  if (diag_count == 0u) {
    app_draw_panel_card(app, x, y, content_w, 46.0f);
    stygian_text(app->ctx, app->font, "No export/runtime diagnostics.", x + 10.0f,
                 y + 11.0f, 12.0f, 0.72f, 0.90f, 0.76f, 1.0f);
    stygian_text(app->ctx, app->font,
                 "That is the goal. Quiet is good here.",
                 x + 10.0f, y + 26.0f, 10.2f, 0.68f, 0.76f, 0.86f, 1.0f);
    return mutated;
  }

  for (uint32_t i = 0u; i < diag_count && i < 10u; ++i) {
    char line_a[220];
    char line_b[220];
    char path[220];
    float sr = 0.75f, sg = 0.86f, sb = 0.95f;
    app_node_path(app, diags[i].node_id, path, sizeof(path));
    if (diags[i].severity == STYGIAN_EDITOR_DIAG_ERROR) {
      sr = 0.96f;
      sg = 0.66f;
      sb = 0.66f;
    } else if (diags[i].severity == STYGIAN_EDITOR_DIAG_WARNING) {
      sr = 0.98f;
      sg = 0.86f;
      sb = 0.62f;
    }
    snprintf(line_a, sizeof(line_a), "[%s] #%u %s", app_diag_severity_name(diags[i].severity),
             diags[i].node_id, diags[i].feature);
    snprintf(line_b, sizeof(line_b), "%s | %s", path, diags[i].message);
    app_draw_panel_card(app, x, y, content_w, 40.0f);
    stygian_rect(app->ctx, x + 1.0f, y + 8.0f, 3.0f, 24.0f, sr, sg, sb, 0.95f);
    stygian_text(app->ctx, app->font, line_a, x + 10.0f, y + 6.0f, 11.5f, sr, sg,
                 sb, 1.0f);
    stygian_text(app->ctx, app->font, line_b, x + 10.0f, y + 21.0f, 10.2f, 0.78f,
                 0.86f, 0.95f, 1.0f);
    if (stygian_button(app->ctx, app->font, "Focus", panel_x + panel_w - 62.0f, y,
                       48.0f, 16.0f)) {
      (void)stygian_editor_select_node(app->editor, diags[i].node_id, false);
      app_set_status(app, "Focused diagnostic node.");
      mutated = true;
    }
    y += 44.0f;
  }
  return mutated;
}

static void app_refresh_code_insight(EditorBootstrapApp *app,
                                     StygianEditorNodeId node_id) {
  static char code[524288];
  static char hooks_text[32768];
  size_t need = 0u;
  char node_marker[64];
  char node_path[256];
  char hook_summary[384];
  char generated_location[320];
  char *match = NULL;
  uint32_t generated_line = 0u;
  uint32_t behavior_count = 0u;
  uint32_t uses_hook_set_variable = 0u;
  uint32_t uses_hook_navigate = 0u;
  uint32_t set_var_line = 0u;
  uint32_t navigate_line = 0u;
  bool has_hook_set_variable = false;
  bool has_hook_navigate = false;
  if (!app || !app->editor || node_id == STYGIAN_EDITOR_INVALID_ID) {
    if (app)
      snprintf(app->code_insight_text, sizeof(app->code_insight_text),
               "Code insight: select one node.");
    return;
  }
  need = stygian_editor_build_c23(app->editor, NULL, 0u);
  if (need < 2u || need > sizeof(code)) {
    snprintf(app->code_insight_text, sizeof(app->code_insight_text),
             "Code insight unavailable: export size probe failed.");
    return;
  }
  if (stygian_editor_build_c23(app->editor, code, sizeof(code)) != need) {
    snprintf(app->code_insight_text, sizeof(app->code_insight_text),
             "Code insight unavailable: export generation failed.");
    return;
  }
  snprintf(node_marker, sizeof(node_marker), "  /* Node %uu (", node_id);
  match = strstr(code, node_marker);
  if (!match) {
    snprintf(app->code_insight_text, sizeof(app->code_insight_text),
             "Node %u not found in generated code. This can happen for non-runtime nodes.",
             node_id);
    return;
  }
  generated_line = app_text_line_for_substring(code, node_marker);
  node_path[0] = '\0';
  (void)app_build_node_path(app, node_id, node_path, sizeof(node_path));
  hooks_text[0] = '\0';
  (void)app_read_text_file(app_hook_source_path(), hooks_text, sizeof(hooks_text));
  has_hook_set_variable = app_hook_function_exists(
      hooks_text, "stygian_editor_generated_hook_set_variable");
  has_hook_navigate = app_hook_function_exists(
      hooks_text, "stygian_editor_generated_hook_navigate");
  set_var_line = app_text_line_for_substring(
      hooks_text, "stygian_editor_generated_hook_set_variable(");
  navigate_line = app_text_line_for_substring(
      hooks_text, "stygian_editor_generated_hook_navigate(");
  behavior_count = stygian_editor_behavior_count(app->editor);
  for (uint32_t i = 0u; i < behavior_count; ++i) {
    StygianEditorBehaviorRule rule;
    StygianEditorBehaviorId behavior_id = 0u;
    StygianEditorNodeId target = STYGIAN_EDITOR_INVALID_ID;
    if (!stygian_editor_get_behavior_rule(app->editor, i, &behavior_id, &rule))
      continue;
    (void)behavior_id;
    if (rule.action_kind == STYGIAN_EDITOR_ACTION_ANIMATE)
      target = rule.animate.target_node;
    else if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY)
      target = rule.set_property.target_node;
    else if (rule.action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY)
      target = rule.toggle_visibility.target_node;
    else
      target = rule.trigger_node;
    if (rule.trigger_node != node_id && target != node_id)
      continue;
    if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_VARIABLE)
      uses_hook_set_variable += 1u;
    if (rule.action_kind == STYGIAN_EDITOR_ACTION_NAVIGATE)
      uses_hook_navigate += 1u;
  }
  hook_summary[0] = '\0';
  if (uses_hook_set_variable > 0u) {
    snprintf(hook_summary + strlen(hook_summary),
             sizeof(hook_summary) - strlen(hook_summary),
             "set_variable x%u [%s]",
             uses_hook_set_variable,
             has_hook_set_variable ? "present" : "missing");
    if (has_hook_set_variable) {
      snprintf(hook_summary + strlen(hook_summary),
               sizeof(hook_summary) - strlen(hook_summary), " @ line %u",
               set_var_line);
    }
  }
  if (uses_hook_navigate > 0u) {
    if (hook_summary[0]) {
      strncat(hook_summary, " | ",
              sizeof(hook_summary) - strlen(hook_summary) - 1u);
    }
    snprintf(hook_summary + strlen(hook_summary),
             sizeof(hook_summary) - strlen(hook_summary),
             "navigate x%u [%s]", uses_hook_navigate,
             has_hook_navigate ? "present" : "missing");
    if (has_hook_navigate) {
      snprintf(hook_summary + strlen(hook_summary),
               sizeof(hook_summary) - strlen(hook_summary), " @ line %u",
               navigate_line);
    }
  }
  if (!hook_summary[0])
    snprintf(hook_summary, sizeof(hook_summary), "no external hooks required");
  snprintf(generated_location, sizeof(generated_location), "%s:%u",
           app_generated_scene_path(), generated_line);
  snprintf(app->code_insight_text, sizeof(app->code_insight_text),
           "Selection\n"
           "---------\n"
           "Node %u\n"
           "Path: %s\n\n"
           "Generated zone\n"
           "--------------\n"
           "%s\n"
           "Ownership: exporter-owned\n\n"
           "Hook zone\n"
           "---------\n"
           "%s\n"
           "Hooks: %s\n"
           "Ownership: user-owned\n\n"
           "Authored zone\n"
           "------------\n"
           "%s\n"
           "Ownership: project-authored state\n",
           node_id, node_path[0] ? node_path : "(unnamed)", generated_location,
           app_hook_source_path(), hook_summary,
           app->project_path[0] ? app->project_path : "in-memory session");
  app->code_insight_node = node_id;
}

static float app_menu_bar_height(void) { return 26.0f; }

static float app_tool_strip_height(void) { return 48.0f; }

static AppTool app_tool_from_primitive(AppPrimitiveKind primitive) {
  switch (primitive) {
  case APP_PRIMITIVE_SQUARE:
  case APP_PRIMITIVE_RECTANGLE:
    return APP_TOOL_RECT_FRAME;
  case APP_PRIMITIVE_CIRCLE:
  case APP_PRIMITIVE_ELLIPSE:
    return APP_TOOL_ELLIPSE;
  default:
    return APP_TOOL_SELECT;
  }
}

static const char *app_tool_name(AppTool tool) {
  switch (tool) {
  case APP_TOOL_SELECT:
    return "Select";
  case APP_TOOL_RECT_FRAME:
    return "Rect/Frame";
  case APP_TOOL_ELLIPSE:
    return "Ellipse";
  case APP_TOOL_PATH:
    return "Path";
  case APP_TOOL_PAN:
    return "Pan";
  case APP_TOOL_ZOOM:
    return "Zoom";
  default:
    return "Select";
  }
}

static void app_set_current_tool(EditorBootstrapApp *app, AppTool tool) {
  if (!app)
    return;
  if (tool != APP_TOOL_PATH && app->path_tool_active)
    app_cancel_path_tool(app);
  app->current_tool = tool;
  if (tool == APP_TOOL_PATH && !app->path_tool_active)
    (void)app_start_path_tool(app);
  if (tool != APP_TOOL_PATH)
    app->primitive_menu_open = false;
}

static float app_top_chrome_height(void) {
  return app_menu_bar_height() + app_tool_strip_height();
}

static void app_draw_panel_shell(EditorBootstrapApp *app, float x, float y,
                                 float w, float h, float accent_r,
                                 float accent_g, float accent_b) {
  if (!app || !app->ctx || w <= 0.0f || h <= 0.0f)
    return;
  stygian_rect_rounded(app->ctx, x, y, w, h, 0.035f, 0.045f, 0.060f, 1.0f, 10.0f);
  stygian_rect_rounded(app->ctx, x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f, 0.055f,
                       0.070f, 0.095f, 0.98f, 9.0f);
  stygian_rect_rounded(app->ctx, x + 1.0f, y + 1.0f, w - 2.0f, 30.0f, 0.085f,
                       0.100f, 0.130f, 0.96f, 9.0f);
  stygian_rect_rounded(app->ctx, x + 10.0f, y + 8.0f, 68.0f, 3.0f, accent_r,
                       accent_g, accent_b, 0.92f, 1.5f);
  stygian_rect(app->ctx, x + 10.0f, y + 31.0f, w - 20.0f, 1.0f, 0.18f, 0.24f,
               0.31f, 0.65f);
}

static void app_draw_panel_card(EditorBootstrapApp *app, float x, float y,
                                float w, float h) {
  if (!app || !app->ctx || w <= 0.0f || h <= 0.0f)
    return;
  stygian_rect_rounded(app->ctx, x, y, w, h, 0.075f, 0.095f, 0.125f, 0.96f, 8.0f);
  stygian_rect_rounded(app->ctx, x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f, 0.10f,
                       0.125f, 0.165f, 0.72f, 7.0f);
  stygian_rect(app->ctx, x + 1.0f, y + 1.0f, w - 2.0f, 1.0f, 0.26f, 0.34f, 0.44f,
               0.34f);
}

static void app_draw_badge(EditorBootstrapApp *app, float x, float y, float w,
                           float h, float r, float g, float b,
                           const char *label) {
  if (!app || !app->ctx || !app->font || !label)
    return;
  stygian_rect_rounded(app->ctx, x, y, w, h, r, g, b, 0.82f, h * 0.45f);
  stygian_text(app->ctx, app->font, label, x + 7.0f, y + 3.0f, 10.5f, 0.96f,
               0.97f, 0.99f, 1.0f);
}

static void app_draw_divider(EditorBootstrapApp *app, float x0, float y0,
                             float x1, float y1, float a) {
  if (!app || !app->ctx)
    return;
  stygian_line(app->ctx, x0, y0, x1, y1, 1.0f, 0.22f, 0.30f, 0.39f, a);
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
    *out_x = 100.0f;
  if (out_y)
    *out_y = app_top_chrome_height() + 2.0f;
  if (out_w)
    *out_w = 156.0f;
  if (out_row_h)
    *out_row_h = 24.0f;
  if (out_h)
    *out_h = 24.0f * 4.0f + 14.0f;
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

static void app_context_menu_geometry(const EditorBootstrapApp *app, float *out_x,
                                      float *out_y, float *out_w, float *out_h) {
  float x;
  float y;
  float w = 168.0f;
  float h = 198.0f;
  if (!app)
    return;
  x = app->context_menu_x;
  y = app->context_menu_y;
  if (x + w > app->layout.viewport_x + app->layout.viewport_w - 8.0f)
    x = app->layout.viewport_x + app->layout.viewport_w - w - 8.0f;
  if (y + h > app->layout.viewport_y + app->layout.viewport_h - 8.0f)
    y = app->layout.viewport_y + app->layout.viewport_h - h - 8.0f;
  if (x < app->layout.viewport_x + 8.0f)
    x = app->layout.viewport_x + 8.0f;
  if (y < app->layout.viewport_y + 8.0f)
    y = app->layout.viewport_y + 8.0f;
  if (out_x)
    *out_x = x;
  if (out_y)
    *out_y = y;
  if (out_w)
    *out_w = w;
  if (out_h)
    *out_h = h;
}

static bool app_point_in_context_menu(const EditorBootstrapApp *app, float x,
                                      float y) {
  float menu_x = 0.0f;
  float menu_y = 0.0f;
  float menu_w = 0.0f;
  float menu_h = 0.0f;
  if (!app || !app->context_menu_open)
    return false;
  app_context_menu_geometry(app, &menu_x, &menu_y, &menu_w, &menu_h);
  return app_point_in_rect(x, y, menu_x, menu_y, menu_w, menu_h);
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

static bool app_set_selection_visible(EditorBootstrapApp *app, bool visible) {
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count;
  bool mutated = false;
  if (!app || !app->editor)
    return false;
  count = app_selected_nodes(app, ids, APP_MAX_TRANSFORM_SELECTION);
  for (uint32_t i = 0u; i < count; ++i) {
    if (stygian_editor_node_set_visible(app->editor, ids[i], visible))
      mutated = true;
  }
  return mutated;
}

static bool app_set_selection_locked(EditorBootstrapApp *app, bool locked) {
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count;
  bool mutated = false;
  if (!app || !app->editor)
    return false;
  count = app_selected_nodes(app, ids, APP_MAX_TRANSFORM_SELECTION);
  for (uint32_t i = 0u; i < count; ++i) {
    if (stygian_editor_node_set_locked(app->editor, ids[i], locked))
      mutated = true;
  }
  return mutated;
}

static bool app_select_focus_children(EditorBootstrapApp *app) {
  StygianEditorNodeId ids[64];
  uint32_t count;
  if (!app || !app->editor)
    return false;
  count = stygian_editor_tree_list_children(app->editor, app->scene_parent_focus,
                                            ids, 64u);
  if (count == 0u)
    return false;
  (void)stygian_editor_select_node(app->editor, STYGIAN_EDITOR_INVALID_ID, false);
  for (uint32_t i = 0u; i < count; ++i)
    (void)stygian_editor_select_node(app->editor, ids[i], i > 0u);
  return true;
}

static bool app_select_parent(EditorBootstrapApp *app) {
  StygianEditorNodeId selected;
  StygianEditorNodeId parent = STYGIAN_EDITOR_INVALID_ID;
  if (!app || !app->editor)
    return false;
  selected = stygian_editor_selected_node(app->editor);
  if (selected == STYGIAN_EDITOR_INVALID_ID)
    return false;
  if (!stygian_editor_node_get_parent(app->editor, selected, &parent))
    return false;
  return stygian_editor_select_node(app->editor, parent, false);
}

static bool app_undo(EditorBootstrapApp *app) {
  if (!app || !app->editor)
    return false;
  return stygian_editor_undo(app->editor);
}

static bool app_redo(EditorBootstrapApp *app) {
  if (!app || !app->editor)
    return false;
  return stygian_editor_redo(app->editor);
}

static void app_timeline_set_cursor(EditorBootstrapApp *app, uint32_t ms) {
  if (!app)
    return;
  snprintf(app->timeline_cursor_ms, sizeof(app->timeline_cursor_ms), "%u", ms);
}

static uint32_t app_timeline_visible_span_ms(EditorBootstrapApp *app,
                                             uint32_t content_max_ms) {
  uint32_t span = 0u;
  if (!app)
    return content_max_ms > 0u ? content_max_ms : 1000u;
  span = app->timeline_view_span_ms;
  if (span < 250u)
    span = content_max_ms > 0u ? content_max_ms : 1000u;
  if (span < 250u)
    span = 250u;
  return span;
}

static uint32_t app_timeline_visible_start_ms(EditorBootstrapApp *app,
                                              uint32_t content_max_ms) {
  uint32_t span;
  uint32_t start;
  if (!app)
    return 0u;
  span = app_timeline_visible_span_ms(app, content_max_ms);
  start = app->timeline_view_start_ms;
  if (content_max_ms > span && start > content_max_ms - span)
    start = content_max_ms - span;
  if (content_max_ms <= span)
    start = 0u;
  return start;
}

static float app_timeline_time_to_view_x(uint32_t time_ms, uint32_t start_ms,
                                         uint32_t span_ms, float lane_x,
                                         float lane_w) {
  float t = 0.0f;
  if (span_ms == 0u || lane_w <= 0.0f)
    return lane_x;
  if (time_ms < start_ms)
    time_ms = start_ms;
  if (time_ms > start_ms + span_ms)
    time_ms = start_ms + span_ms;
  t = (float)(time_ms - start_ms) / (float)span_ms;
  return lane_x + lane_w * t;
}

static uint32_t app_timeline_view_x_to_time(float view_x, uint32_t start_ms,
                                            uint32_t span_ms, float lane_x,
                                            float lane_w) {
  float t;
  if (span_ms == 0u || lane_w <= 0.0f)
    return start_ms;
  t = (view_x - lane_x) / lane_w;
  t = app_clampf(t, 0.0f, 1.0f);
  return start_ms + (uint32_t)(t * (float)span_ms + 0.5f);
}

static bool app_timeline_step_cursor(EditorBootstrapApp *app, int delta_ms) {
  uint32_t cursor = 0u;
  int64_t stepped;
  if (!app)
    return false;
  if (app->timeline_cursor_ms[0]) {
    char *end = NULL;
    unsigned long parsed = strtoul(app->timeline_cursor_ms, &end, 10);
    if (end && *end == '\0')
      cursor = (uint32_t)parsed;
  }
  stepped = (int64_t)cursor + (int64_t)delta_ms;
  if (stepped < 0)
    stepped = 0;
  app_timeline_set_cursor(app, (uint32_t)stepped);
  return true;
}

static bool app_timeline_update_selected_key(EditorBootstrapApp *app,
                                             uint32_t new_time_ms,
                                             float *new_value_or_null,
                                             bool update_value) {
  StygianEditorTimelineTrack track = {0};
  StygianEditorTimelineKeyframe keys[STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP];
  uint32_t count;
  uint32_t min_time = 0u;
  uint32_t max_time = UINT32_MAX;
  if (!app || !app->editor)
    return false;
  if (!stygian_editor_timeline_get_track(app->editor, app->timeline_selected_track_index,
                                         &track))
    return false;
  if (app->timeline_selected_keyframe_index >= track.keyframe_count)
    return false;
  count = track.keyframe_count;
  for (uint32_t i = 0u; i < count; ++i)
    keys[i] = track.keyframes[i];
  if (app->timeline_selected_keyframe_index > 0u)
    min_time = keys[app->timeline_selected_keyframe_index - 1u].time_ms + 1u;
  if (app->timeline_selected_keyframe_index + 1u < count)
    max_time = keys[app->timeline_selected_keyframe_index + 1u].time_ms - 1u;
  if (new_time_ms < min_time)
    new_time_ms = min_time;
  if (new_time_ms > max_time)
    new_time_ms = max_time;
  keys[app->timeline_selected_keyframe_index].time_ms = new_time_ms;
  if (update_value && new_value_or_null)
    keys[app->timeline_selected_keyframe_index].value = *new_value_or_null;
  if (!stygian_editor_timeline_set_track_keyframes(app->editor, track.id, keys, count))
    return false;
  app_timeline_set_cursor(app, new_time_ms);
  snprintf(app->timeline_key_value, sizeof(app->timeline_key_value), "%.3f",
           keys[app->timeline_selected_keyframe_index].value);
  return true;
}

static bool app_timeline_select_key_from_lane(EditorBootstrapApp *app, float mouse_x,
                                              float mouse_y) {
  uint32_t track_count;
  uint32_t max_time_ms;
  uint32_t start_ms;
  uint32_t span_ms;
  if (!app || !app->editor)
    return false;
  if (!app_point_in_rect(mouse_x, mouse_y, app->layout.timeline_lane_x,
                         app->layout.timeline_lane_y, app->layout.timeline_lane_w,
                         app->layout.timeline_lane_h))
    return false;
  track_count = stygian_editor_timeline_track_count(app->editor);
  max_time_ms = app->layout.timeline_max_time_ms > 0u ? app->layout.timeline_max_time_ms
                                                      : 1000u;
  start_ms = app_timeline_visible_start_ms(app, max_time_ms);
  span_ms = app_timeline_visible_span_ms(app, max_time_ms);
  for (uint32_t i = 0u; i < track_count && i < 5u; ++i) {
    StygianEditorTimelineTrack t = {0};
    float row_y = app->layout.timeline_y + 24.0f + 20.0f * (float)i;
    if (!stygian_editor_timeline_get_track(app->editor, i, &t))
      continue;
    for (uint32_t k = 0u; k < t.keyframe_count; ++k) {
      float key_x;
      if (t.keyframes[k].time_ms < start_ms || t.keyframes[k].time_ms > start_ms + span_ms)
        continue;
      key_x = app_timeline_time_to_view_x(t.keyframes[k].time_ms, start_ms, span_ms,
                                          app->layout.timeline_lane_x,
                                          app->layout.timeline_lane_w);
      if (app_point_in_rect(mouse_x, mouse_y, key_x - 6.0f, row_y, 12.0f, 18.0f)) {
        app->timeline_selected_track_index = i;
        app->timeline_selected_keyframe_index = k;
        app_timeline_set_cursor(app, t.keyframes[k].time_ms);
        snprintf(app->timeline_key_value, sizeof(app->timeline_key_value), "%.3f",
                 t.keyframes[k].value);
        app->timeline_key_easing_choice = (int)t.keyframes[k].easing;
        return true;
      }
    }
  }
  return false;
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

static bool app_apply_rotation_to_selection(EditorBootstrapApp *app,
                                            float rotation_degrees) {
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count;
  uint32_t i;
  bool mutated = false;
  if (!app || !app->editor)
    return false;
  count = app_selected_nodes(app, ids, APP_MAX_TRANSFORM_SELECTION);
  for (i = 0u; i < count; ++i) {
    if (stygian_editor_node_set_rotation(app->editor, ids[i], rotation_degrees))
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

static float app_rotation_handle_size_px(void) { return 14.0f; }

static bool app_selected_group_center_world(EditorBootstrapApp *app, float *out_x,
                                            float *out_y) {
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count;
  if (!app || !app->editor)
    return false;
  count = app_selected_nodes(app, ids, APP_MAX_TRANSFORM_SELECTION);
  if (count == 0u)
    return false;
  if (!app_bounds_for_nodes_world(app, ids, count, &x, &y, &w, &h))
    return false;
  if (out_x)
    *out_x = x + w * 0.5f;
  if (out_y)
    *out_y = y + h * 0.5f;
  return true;
}

static bool app_rotation_handle_center_view(EditorBootstrapApp *app,
                                            float *out_x, float *out_y) {
  float vx = 0.0f;
  float vy = 0.0f;
  float vw = 0.0f;
  float vh = 0.0f;
  if (!app || !app_selected_bounds_view(app, &vx, &vy, &vw, &vh))
    return false;
  if (out_x)
    *out_x = vx + vw * 0.5f;
  if (out_y)
    *out_y = vy - 26.0f;
  return true;
}

static bool app_rotation_handle_hit(EditorBootstrapApp *app, float mouse_x,
                                    float mouse_y) {
  float cx = 0.0f;
  float cy = 0.0f;
  float hs = app_rotation_handle_size_px();
  if (!app_rotation_handle_center_view(app, &cx, &cy))
    return false;
  return app_point_in_rect(mouse_x, mouse_y, cx - hs * 0.5f, cy - hs * 0.5f, hs,
                           hs);
}

static bool app_capture_rotate_selection(EditorBootstrapApp *app, float world_x,
                                         float world_y) {
  uint32_t count;
  uint32_t i;
  if (!app || !app->editor)
    return false;
  count = stygian_editor_selected_nodes(app->editor, app->rotate_selection_ids,
                                        APP_MAX_TRANSFORM_SELECTION);
  if (count == 0u)
    return false;
  if (!app_selected_group_center_world(app, &app->rotate_group_center_x,
                                       &app->rotate_group_center_y)) {
    return false;
  }
  app->rotate_start_angle =
      atan2f(world_y - app->rotate_group_center_y,
             world_x - app->rotate_group_center_x);
  app->rotate_selection_count = 0u;
  for (i = 0u; i < count; ++i) {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    float rotation = 0.0f;
    StygianEditorNodeId id = app->rotate_selection_ids[i];
    if (!stygian_editor_node_get_bounds(app->editor, id, &x, &y, &w, &h))
      continue;
    (void)stygian_editor_node_get_rotation(app->editor, id, &rotation);
    app->rotate_selection_ids[app->rotate_selection_count] = id;
    app->rotate_selection_start_x[app->rotate_selection_count] = x;
    app->rotate_selection_start_y[app->rotate_selection_count] = y;
    app->rotate_selection_start_w[app->rotate_selection_count] = w;
    app->rotate_selection_start_h[app->rotate_selection_count] = h;
    app->rotate_selection_start_rotation[app->rotate_selection_count] = rotation;
    app->rotate_selection_count += 1u;
  }
  return app->rotate_selection_count > 0u;
}

static bool app_apply_rotate_drag(EditorBootstrapApp *app, float mouse_x,
                                  float mouse_y) {
  float world_x = 0.0f;
  float world_y = 0.0f;
  float angle;
  float delta;
  float degrees;
  bool mutated = false;
  uint32_t i;
  if (!app || !app->editor || !app->rotating_selection ||
      app->rotate_selection_count == 0u) {
    return false;
  }
  stygian_editor_view_to_world(app->editor, mouse_x, mouse_y, &world_x, &world_y);
  angle = atan2f(world_y - app->rotate_group_center_y,
                 world_x - app->rotate_group_center_x);
  delta = angle - app->rotate_start_angle;
  degrees = delta * (180.0f / 3.14159265f);
  if (app->ctx) {
    StygianWindow *win = stygian_get_window(app->ctx);
    if (win && !stygian_key_down(win, STYGIAN_KEY_ALT)) {
      degrees = roundf(degrees / 15.0f) * 15.0f;
      delta = degrees * (3.14159265f / 180.0f);
    }
  }
  for (i = 0u; i < app->rotate_selection_count; ++i) {
    float start_x = app->rotate_selection_start_x[i];
    float start_y = app->rotate_selection_start_y[i];
    float start_w = app->rotate_selection_start_w[i];
    float start_h = app->rotate_selection_start_h[i];
    float center_x = start_x + start_w * 0.5f;
    float center_y = start_y + start_h * 0.5f;
    float rel_x = center_x - app->rotate_group_center_x;
    float rel_y = center_y - app->rotate_group_center_y;
    float sin_a = sinf(delta);
    float cos_a = cosf(delta);
    float next_center_x = app->rotate_group_center_x + rel_x * cos_a - rel_y * sin_a;
    float next_center_y = app->rotate_group_center_y + rel_x * sin_a + rel_y * cos_a;
    float next_x = next_center_x - start_w * 0.5f;
    float next_y = next_center_y - start_h * 0.5f;
    float next_rotation = app->rotate_selection_start_rotation[i] + degrees;
    if (stygian_editor_node_set_position(app->editor, app->rotate_selection_ids[i],
                                         next_x, next_y, false)) {
      mutated = true;
    }
    if (stygian_editor_node_set_rotation(app->editor, app->rotate_selection_ids[i],
                                         next_rotation)) {
      mutated = true;
    }
  }
  return mutated;
}

static uint32_t app_collect_tree(EditorBootstrapApp *app, StygianEditorNodeId parent_id,
                                 StygianEditorNodeId *out_ids, uint32_t max_ids,
                                 bool include_parent) {
  StygianEditorNodeId children[512];
  uint32_t count = 0u;
  uint32_t child_count;
  uint32_t i;
  if (!app || !app->editor || !out_ids || max_ids == 0u)
    return 0u;
  if (include_parent && parent_id != STYGIAN_EDITOR_INVALID_ID && count < max_ids)
    out_ids[count++] = parent_id;
  child_count = stygian_editor_tree_list_children(app->editor, parent_id, children,
                                                  (uint32_t)(sizeof(children) /
                                                             sizeof(children[0])));
  for (i = 0u; i < child_count && count < max_ids; ++i) {
    count += app_collect_tree(app, children[i], out_ids + count, max_ids - count,
                              true);
  }
  return count;
}

static bool app_select_all_nodes(EditorBootstrapApp *app) {
  StygianEditorNodeId ids[APP_MAX_NODE_FLAGS];
  uint32_t count;
  uint32_t i;
  bool mutated = false;
  if (!app || !app->editor)
    return false;
  count = app_collect_tree(app, STYGIAN_EDITOR_INVALID_ID, ids, APP_MAX_NODE_FLAGS,
                           false);
  for (i = 0u; i < count; ++i) {
    if (stygian_editor_select_node(app->editor, ids[i], true))
      mutated = true;
  }
  return mutated;
}

static uint32_t app_selected_root_nodes(EditorBootstrapApp *app,
                                        StygianEditorNodeId *out_ids,
                                        uint32_t max_ids) {
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count;
  uint32_t out_count = 0u;
  uint32_t i;
  if (!app || !app->editor || !out_ids || max_ids == 0u)
    return 0u;
  count = stygian_editor_selected_nodes(app->editor, ids, APP_MAX_TRANSFORM_SELECTION);
  for (i = 0u; i < count && out_count < max_ids; ++i) {
    StygianEditorNodeId parent_id = STYGIAN_EDITOR_INVALID_ID;
    if (stygian_editor_node_get_parent(app->editor, ids[i], &parent_id) &&
        parent_id != STYGIAN_EDITOR_INVALID_ID &&
        stygian_editor_node_is_selected(app->editor, parent_id)) {
      continue;
    }
    out_ids[out_count++] = ids[i];
  }
  return out_count;
}

static void app_clipboard_clear(EditorBootstrapApp *app) {
  if (!app)
    return;
  memset(&app->clipboard, 0, sizeof(app->clipboard));
}

static int32_t app_clipboard_capture_node(EditorBootstrapApp *app,
                                          StygianEditorNodeId node_id,
                                          int32_t parent_index, bool selected_root,
                                          bool *out_supported) {
  StygianEditorShapeKind kind = STYGIAN_EDITOR_SHAPE_RECT;
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
  uint32_t idx;
  uint32_t child_count;
  StygianEditorNodeId children[512];
  uint32_t i;
  if (out_supported)
    *out_supported = false;
  if (!app || !app->editor || !node_id || app->clipboard.count >= APP_CLIPBOARD_NODE_CAP)
    return -1;
  if (!stygian_editor_node_get_shape_kind(app->editor, node_id, &kind))
    return -1;
  if (!stygian_editor_node_get_bounds(app->editor, node_id, &x, &y, &w, &h))
    return -1;

  switch (kind) {
  case STYGIAN_EDITOR_SHAPE_RECT:
  case STYGIAN_EDITOR_SHAPE_ELLIPSE:
  case STYGIAN_EDITOR_SHAPE_FRAME:
  case STYGIAN_EDITOR_SHAPE_TEXT:
  case STYGIAN_EDITOR_SHAPE_IMAGE:
  case STYGIAN_EDITOR_SHAPE_PATH:
    break;
  default:
    return -1;
  }

  idx = app->clipboard.count++;
  app->clipboard.nodes[idx].kind = kind;
  app->clipboard.nodes[idx].parent_index = parent_index;
  app->clipboard.nodes[idx].selected_root = selected_root;
  app->clipboard.nodes[idx].x = x;
  app->clipboard.nodes[idx].y = y;
  app->clipboard.nodes[idx].w = w;
  app->clipboard.nodes[idx].h = h;
  app->clipboard.nodes[idx].visible = true;
  app->clipboard.nodes[idx].rotation = 0.0f;
  app->clipboard.nodes[idx].opacity = 1.0f;
  app->clipboard.nodes[idx].z = 0.0f;
  (void)stygian_editor_node_get_rotation(app->editor, node_id,
                                         &app->clipboard.nodes[idx].rotation);
  (void)stygian_editor_node_get_visible(app->editor, node_id,
                                        &app->clipboard.nodes[idx].visible);
  (void)stygian_editor_node_get_name(app->editor, node_id,
                                     app->clipboard.nodes[idx].name,
                                     sizeof(app->clipboard.nodes[idx].name));
  app->clipboard.nodes[idx].fill_count =
      stygian_editor_node_fill_count(app->editor, node_id);
  if (app->clipboard.nodes[idx].fill_count > STYGIAN_EDITOR_NODE_FILL_CAP)
    app->clipboard.nodes[idx].fill_count = STYGIAN_EDITOR_NODE_FILL_CAP;
  for (i = 0u; i < app->clipboard.nodes[idx].fill_count; ++i)
    (void)stygian_editor_node_get_fill(app->editor, node_id, i,
                                       &app->clipboard.nodes[idx].fills[i]);
  if (app->clipboard.nodes[idx].fill_count > 0u)
    app->clipboard.nodes[idx].opacity = app->clipboard.nodes[idx].fills[0].opacity;
  app->clipboard.nodes[idx].stroke_count =
      stygian_editor_node_stroke_count(app->editor, node_id);
  if (app->clipboard.nodes[idx].stroke_count > STYGIAN_EDITOR_NODE_STROKE_CAP)
    app->clipboard.nodes[idx].stroke_count = STYGIAN_EDITOR_NODE_STROKE_CAP;
  for (i = 0u; i < app->clipboard.nodes[idx].stroke_count; ++i)
    (void)stygian_editor_node_get_stroke(app->editor, node_id, i,
                                         &app->clipboard.nodes[idx].strokes[i]);
  app->clipboard.nodes[idx].effect_count =
      stygian_editor_node_effect_count(app->editor, node_id);
  if (app->clipboard.nodes[idx].effect_count > STYGIAN_EDITOR_NODE_EFFECT_CAP)
    app->clipboard.nodes[idx].effect_count = STYGIAN_EDITOR_NODE_EFFECT_CAP;
  for (i = 0u; i < app->clipboard.nodes[idx].effect_count; ++i)
    (void)stygian_editor_node_get_effect(app->editor, node_id, i,
                                         &app->clipboard.nodes[idx].effects[i]);

  if (kind == STYGIAN_EDITOR_SHAPE_RECT) {
    (void)stygian_editor_node_get_corner_radii(
        app->editor, node_id, &app->clipboard.nodes[idx].radius[0],
        &app->clipboard.nodes[idx].radius[1], &app->clipboard.nodes[idx].radius[2],
        &app->clipboard.nodes[idx].radius[3]);
  } else if (kind == STYGIAN_EDITOR_SHAPE_TEXT) {
    (void)stygian_editor_node_get_text_content(
        app->editor, node_id, app->clipboard.nodes[idx].text,
        sizeof(app->clipboard.nodes[idx].text));
    (void)stygian_editor_node_get_text_typography(
        app->editor, node_id, &app->clipboard.nodes[idx].font_size,
        &app->clipboard.nodes[idx].line_height,
        &app->clipboard.nodes[idx].letter_spacing,
        &app->clipboard.nodes[idx].font_weight);
    (void)stygian_editor_node_get_text_layout(
        app->editor, node_id, &app->clipboard.nodes[idx].box_mode,
        &app->clipboard.nodes[idx].align_h, &app->clipboard.nodes[idx].align_v,
        &app->clipboard.nodes[idx].auto_size);
    app->clipboard.nodes[idx].text_span_count =
        stygian_editor_node_text_span_count(app->editor, node_id);
    if (app->clipboard.nodes[idx].text_span_count > STYGIAN_EDITOR_TEXT_SPAN_CAP)
      app->clipboard.nodes[idx].text_span_count = STYGIAN_EDITOR_TEXT_SPAN_CAP;
    for (i = 0u; i < app->clipboard.nodes[idx].text_span_count; ++i)
      (void)stygian_editor_node_get_text_span(app->editor, node_id, i,
                                              &app->clipboard.nodes[idx].spans[i]);
  } else if (kind == STYGIAN_EDITOR_SHAPE_IMAGE) {
    (void)stygian_editor_node_get_image_source(
        app->editor, node_id, app->clipboard.nodes[idx].image_source,
        sizeof(app->clipboard.nodes[idx].image_source));
    (void)stygian_editor_node_get_image_fit_mode(
        app->editor, node_id, &app->clipboard.nodes[idx].fit_mode);
  } else if (kind == STYGIAN_EDITOR_SHAPE_PATH) {
    app->clipboard.nodes[idx].path_point_count =
        stygian_editor_path_point_count(app->editor, node_id);
    if (app->clipboard.nodes[idx].path_point_count > 64u)
      app->clipboard.nodes[idx].path_point_count = 64u;
    for (i = 0u; i < app->clipboard.nodes[idx].path_point_count; ++i)
      (void)stygian_editor_path_get_point(app->editor, node_id, i,
                                          &app->clipboard.nodes[idx].path_points[i]);
    if (app->clipboard.nodes[idx].stroke_count > 0u) {
      app->clipboard.nodes[idx].path_thickness =
          app->clipboard.nodes[idx].strokes[0].thickness;
      app->clipboard.nodes[idx].path_stroke =
          app->clipboard.nodes[idx].strokes[0].color;
    }
  }

  if (selected_root && app->clipboard.count == 1u) {
    app->clipboard.anchor_min_x = x;
    app->clipboard.anchor_min_y = y;
  } else if (selected_root) {
    if (x < app->clipboard.anchor_min_x)
      app->clipboard.anchor_min_x = x;
    if (y < app->clipboard.anchor_min_y)
      app->clipboard.anchor_min_y = y;
  }

  child_count = stygian_editor_tree_list_children(app->editor, node_id, children,
                                                  (uint32_t)(sizeof(children) /
                                                             sizeof(children[0])));
  for (i = 0u; i < child_count; ++i)
    (void)app_clipboard_capture_node(app, children[i], (int32_t)idx, false, NULL);
  if (out_supported)
    *out_supported = true;
  return (int32_t)idx;
}

static StygianEditorNodeId app_paste_clipboard_node(EditorBootstrapApp *app,
                                                    uint32_t clip_index,
                                                    const StygianEditorNodeId *mapped_ids,
                                                    float dx, float dy) {
  StygianEditorNodeId new_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId parent_id = STYGIAN_EDITOR_INVALID_ID;
  if (!app || !app->editor || clip_index >= app->clipboard.count)
    return STYGIAN_EDITOR_INVALID_ID;
  if (app->clipboard.nodes[clip_index].parent_index >= 0)
    parent_id = mapped_ids[app->clipboard.nodes[clip_index].parent_index];

  switch (app->clipboard.nodes[clip_index].kind) {
  case STYGIAN_EDITOR_SHAPE_RECT: {
    StygianEditorRectDesc desc = {0};
    desc.x = app->clipboard.nodes[clip_index].x + dx;
    desc.y = app->clipboard.nodes[clip_index].y + dy;
    desc.w = app->clipboard.nodes[clip_index].w;
    desc.h = app->clipboard.nodes[clip_index].h;
    memcpy(desc.radius, app->clipboard.nodes[clip_index].radius, sizeof(desc.radius));
    desc.fill = app->clipboard.nodes[clip_index].fill_count > 0u
                    ? app->clipboard.nodes[clip_index].fills[0].solid
                    : app->paint_color;
    desc.visible = app->clipboard.nodes[clip_index].visible;
    desc.z = app->clipboard.nodes[clip_index].z;
    new_id = stygian_editor_add_rect(app->editor, &desc);
  } break;
  case STYGIAN_EDITOR_SHAPE_ELLIPSE: {
    StygianEditorEllipseDesc desc = {0};
    desc.x = app->clipboard.nodes[clip_index].x + dx;
    desc.y = app->clipboard.nodes[clip_index].y + dy;
    desc.w = app->clipboard.nodes[clip_index].w;
    desc.h = app->clipboard.nodes[clip_index].h;
    desc.fill = app->clipboard.nodes[clip_index].fill_count > 0u
                    ? app->clipboard.nodes[clip_index].fills[0].solid
                    : app->paint_color;
    desc.visible = app->clipboard.nodes[clip_index].visible;
    desc.z = app->clipboard.nodes[clip_index].z;
    new_id = stygian_editor_add_ellipse(app->editor, &desc);
  } break;
  case STYGIAN_EDITOR_SHAPE_FRAME: {
    StygianEditorFrameDesc desc = {0};
    desc.x = app->clipboard.nodes[clip_index].x + dx;
    desc.y = app->clipboard.nodes[clip_index].y + dy;
    desc.w = app->clipboard.nodes[clip_index].w;
    desc.h = app->clipboard.nodes[clip_index].h;
    desc.clip_content = app->clipboard.nodes[clip_index].clip_content;
    desc.fill = app->clipboard.nodes[clip_index].fill_count > 0u
                    ? app->clipboard.nodes[clip_index].fills[0].solid
                    : app->paint_color;
    desc.visible = app->clipboard.nodes[clip_index].visible;
    desc.z = app->clipboard.nodes[clip_index].z;
    new_id = stygian_editor_add_frame(app->editor, &desc);
  } break;
  case STYGIAN_EDITOR_SHAPE_TEXT: {
    StygianEditorTextDesc desc = {0};
    desc.x = app->clipboard.nodes[clip_index].x + dx;
    desc.y = app->clipboard.nodes[clip_index].y + dy;
    desc.w = app->clipboard.nodes[clip_index].w;
    desc.h = app->clipboard.nodes[clip_index].h;
    desc.font_size = app->clipboard.nodes[clip_index].font_size;
    desc.line_height = app->clipboard.nodes[clip_index].line_height;
    desc.letter_spacing = app->clipboard.nodes[clip_index].letter_spacing;
    desc.font_weight = app->clipboard.nodes[clip_index].font_weight;
    desc.box_mode = app->clipboard.nodes[clip_index].box_mode;
    desc.align_h = app->clipboard.nodes[clip_index].align_h;
    desc.align_v = app->clipboard.nodes[clip_index].align_v;
    desc.auto_size = app->clipboard.nodes[clip_index].auto_size;
    desc.fill = app->clipboard.nodes[clip_index].fill_count > 0u
                    ? app->clipboard.nodes[clip_index].fills[0].solid
                    : app->paint_color;
    desc.text = app->clipboard.nodes[clip_index].text;
    desc.visible = app->clipboard.nodes[clip_index].visible;
    desc.z = app->clipboard.nodes[clip_index].z;
    new_id = stygian_editor_add_text(app->editor, &desc);
  } break;
  case STYGIAN_EDITOR_SHAPE_IMAGE: {
    StygianEditorImageDesc desc = {0};
    desc.x = app->clipboard.nodes[clip_index].x + dx;
    desc.y = app->clipboard.nodes[clip_index].y + dy;
    desc.w = app->clipboard.nodes[clip_index].w;
    desc.h = app->clipboard.nodes[clip_index].h;
    desc.fit_mode = app->clipboard.nodes[clip_index].fit_mode;
    desc.source = app->clipboard.nodes[clip_index].image_source;
    desc.visible = app->clipboard.nodes[clip_index].visible;
    desc.z = app->clipboard.nodes[clip_index].z;
    new_id = stygian_editor_add_image(app->editor, &desc);
  } break;
  case STYGIAN_EDITOR_SHAPE_PATH: {
    StygianEditorPathDesc desc = {0};
    StygianEditorPathId path_id;
    uint32_t i;
    desc.stroke = app->clipboard.nodes[clip_index].path_stroke;
    desc.thickness = app->clipboard.nodes[clip_index].path_thickness > 0.0f
                         ? app->clipboard.nodes[clip_index].path_thickness
                         : 2.0f;
    desc.closed = false;
    desc.visible = app->clipboard.nodes[clip_index].visible;
    desc.z = app->clipboard.nodes[clip_index].z;
    path_id = stygian_editor_path_begin(app->editor, &desc);
    for (i = 0u; path_id && i < app->clipboard.nodes[clip_index].path_point_count; ++i) {
      StygianEditorPoint point = app->clipboard.nodes[clip_index].path_points[i];
      (void)stygian_editor_path_add_point(app->editor, path_id, point.x + dx,
                                          point.y + dy, false);
    }
    new_id = path_id ? stygian_editor_path_commit(app->editor, path_id)
                     : STYGIAN_EDITOR_INVALID_ID;
  } break;
  default:
    break;
  }

  if (!new_id)
    return STYGIAN_EDITOR_INVALID_ID;
  if (parent_id != STYGIAN_EDITOR_INVALID_ID)
    (void)stygian_editor_reparent_node(app->editor, new_id, parent_id, true);
  (void)stygian_editor_node_set_rotation(app->editor, new_id,
                                         app->clipboard.nodes[clip_index].rotation);
  if (app->clipboard.nodes[clip_index].fill_count > 0u)
    (void)stygian_editor_node_set_fills(
        app->editor, new_id, app->clipboard.nodes[clip_index].fills,
        app->clipboard.nodes[clip_index].fill_count);
  if (app->clipboard.nodes[clip_index].stroke_count > 0u)
    (void)stygian_editor_node_set_strokes(
        app->editor, new_id, app->clipboard.nodes[clip_index].strokes,
        app->clipboard.nodes[clip_index].stroke_count);
  if (app->clipboard.nodes[clip_index].effect_count > 0u)
    (void)stygian_editor_node_set_effects(
        app->editor, new_id, app->clipboard.nodes[clip_index].effects,
        app->clipboard.nodes[clip_index].effect_count);
  if (app->clipboard.nodes[clip_index].text_span_count > 0u &&
      app->clipboard.nodes[clip_index].kind == STYGIAN_EDITOR_SHAPE_TEXT) {
    (void)stygian_editor_node_set_text_spans(
        app->editor, new_id, app->clipboard.nodes[clip_index].spans,
        app->clipboard.nodes[clip_index].text_span_count);
  }
  if (app->clipboard.nodes[clip_index].name[0])
    (void)stygian_editor_node_set_name(app->editor, new_id,
                                       app->clipboard.nodes[clip_index].name);
  return new_id;
}

static bool app_paste_clipboard(EditorBootstrapApp *app, float target_world_x,
                                float target_world_y, bool select_new) {
  StygianEditorNodeId mapped_ids[APP_CLIPBOARD_NODE_CAP];
  float dx;
  float dy;
  uint32_t i;
  bool mutated = false;
  if (!app || !app->editor || app->clipboard.count == 0u)
    return false;
  memset(mapped_ids, 0, sizeof(mapped_ids));
  dx = target_world_x - app->clipboard.anchor_min_x;
  dy = target_world_y - app->clipboard.anchor_min_y;
  if (select_new)
    (void)stygian_editor_select_node(app->editor, STYGIAN_EDITOR_INVALID_ID, false);
  for (i = 0u; i < app->clipboard.count; ++i) {
    mapped_ids[i] = app_paste_clipboard_node(app, i, mapped_ids, dx, dy);
    if (mapped_ids[i] != STYGIAN_EDITOR_INVALID_ID) {
      mutated = true;
      if (select_new && app->clipboard.nodes[i].selected_root)
        (void)stygian_editor_select_node(app->editor, mapped_ids[i], true);
    }
  }
  return mutated;
}

static bool app_copy_selection(EditorBootstrapApp *app) {
  StygianEditorNodeId roots[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count;
  uint32_t i;
  bool supported_any = false;
  if (!app || !app->editor)
    return false;
  app_clipboard_clear(app);
  count = app_selected_root_nodes(app, roots, APP_MAX_TRANSFORM_SELECTION);
  for (i = 0u; i < count; ++i) {
    bool supported = false;
    (void)app_clipboard_capture_node(app, roots[i], -1, true, &supported);
    supported_any = supported_any || supported;
  }
  if (!supported_any)
    app_clipboard_clear(app);
  return supported_any;
}

static bool app_delete_selection(EditorBootstrapApp *app) {
  StygianEditorNodeId roots[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count;
  bool mutated = false;
  if (!app || !app->editor)
    return false;
  count = app_selected_root_nodes(app, roots, APP_MAX_TRANSFORM_SELECTION);
  for (uint32_t i = 0u; i < count; ++i) {
    if (stygian_editor_delete_node(app->editor, roots[i]))
      mutated = true;
  }
  return mutated;
}

static bool app_cut_selection(EditorBootstrapApp *app) {
  if (!app_copy_selection(app))
    return false;
  return app_delete_selection(app);
}

static bool app_duplicate_selection(EditorBootstrapApp *app, float offset_x,
                                    float offset_y) {
  StygianEditorNodeId roots[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count;
  float min_x = 0.0f;
  float min_y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
  if (!app || !app->editor)
    return false;
  count = app_selected_root_nodes(app, roots, APP_MAX_TRANSFORM_SELECTION);
  if (count == 0u)
    return false;
  if (!app_copy_selection(app))
    return false;
  if (!app_bounds_for_nodes_world(app, roots, count, &min_x, &min_y, &w, &h))
    return false;
  return app_paste_clipboard(app, min_x + offset_x, min_y + offset_y, true);
}

static bool app_nudge_selection(EditorBootstrapApp *app, float dx, float dy) {
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count;
  bool mutated = false;
  if (!app || !app->editor)
    return false;
  count = app_selected_nodes(app, ids, APP_MAX_TRANSFORM_SELECTION);
  for (uint32_t i = 0u; i < count; ++i) {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    if (!stygian_editor_node_get_bounds(app->editor, ids[i], &x, &y, &w, &h))
      continue;
    if (stygian_editor_node_set_position(app->editor, ids[i], x + dx, y + dy, true))
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
  float snapped_world_x = 0.0f;
  float snapped_world_y = 0.0f;
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
  uint64_t started_ms = app_now_ms_system();
  uint32_t i;
  if (!app || !app->editor || !app->resizing_node ||
      app->resize_selection_count == 0u)
    return false;

  stygian_editor_view_to_world(app->editor, mouse_x, mouse_y, &world_x, &world_y);
  snapped_world_x = world_x;
  snapped_world_y = world_y;
  if (app->ctx) {
    StygianWindow *win = stygian_get_window(app->ctx);
    if (win && !stygian_key_down(win, STYGIAN_KEY_ALT)) {
      stygian_editor_snap_world_point(app->editor, world_x, world_y, &snapped_world_x,
                                      &snapped_world_y);
    }
  }
  dx = snapped_world_x - app->resize_start_mouse_world_x;
  dy = snapped_world_y - app->resize_start_mouse_world_y;

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
  if (mutated)
    app->perf_last_layout_ms =
        (float)(app_now_ms_system() - started_ms);
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

#if 0
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
#endif

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

static void app_sync_snap_flags_from_editor(EditorBootstrapApp *app);
static void app_reset_interaction_state(EditorBootstrapApp *app);

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

static void app_reset_canvas(EditorBootstrapApp *app) {
  if (!app || !app->editor)
    return;
  stygian_editor_reset(app->editor);
  app_reset_interaction_state(app);
  app->camera_pan_x = 24.0f;
  app->camera_pan_y = app_top_chrome_height() + 24.0f;
  app->camera_zoom = 1.0f;
  app_sync_grid_config(app);
  app_sync_snap_flags_from_editor(app);
  app_sync_editor_viewport(app);
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

static float app_elapsed_ms(uint64_t start_ms, uint64_t end_ms) {
  if (end_ms < start_ms)
    return 0.0f;
  return (float)(end_ms - start_ms);
}

static void app_sync_snap_flags_from_editor(EditorBootstrapApp *app) {
  StygianEditorSnapSources s;
  if (!app || !app->editor)
    return;
  s = stygian_editor_get_snap_sources(app->editor);
  app->snap_use_grid = s.use_grid;
  app->snap_use_guides = s.use_guides;
  app->snap_use_bounds = s.use_bounds;
  app->snap_use_parent_edges = s.use_parent_edges;
}

#if 0
static void app_push_snap_flags_to_editor(EditorBootstrapApp *app) {
  StygianEditorSnapSources s = {0};
  if (!app || !app->editor)
    return;
  s.use_grid = app->snap_use_grid;
  s.use_guides = app->snap_use_guides;
  s.use_bounds = app->snap_use_bounds;
  s.use_parent_edges = app->snap_use_parent_edges;
  stygian_editor_set_snap_sources(app->editor, &s);
}
#endif

static bool app_repeater_clone_once(EditorBootstrapApp *app,
                                    StygianEditorNodeId source_id, float dx,
                                    float dy, uint32_t index,
                                    StygianEditorNodeId *out_new_id) {
  StygianEditorShapeKind kind = STYGIAN_EDITOR_SHAPE_RECT;
  StygianEditorNodeId parent_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId new_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorColor color = stygian_editor_color_rgba(0.20f, 0.52f, 0.95f, 1.0f);
  bool visible = true;
  char base_name[64] = {0};
  char generated_name[64];
  if (!app || !app->editor || source_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  if (!stygian_editor_node_get_shape_kind(app->editor, source_id, &kind))
    return false;
  (void)stygian_editor_node_get_parent(app->editor, source_id, &parent_id);
  (void)stygian_editor_node_get_color(app->editor, source_id, &color);
  (void)stygian_editor_node_get_visible(app->editor, source_id, &visible);
  if (!stygian_editor_node_get_name(app->editor, source_id, base_name,
                                    sizeof(base_name)) ||
      !base_name[0]) {
    snprintf(base_name, sizeof(base_name), "node_%u", source_id);
  }

  if (kind == STYGIAN_EDITOR_SHAPE_RECT) {
    float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
    float tl = 0.0f, tr = 0.0f, br = 0.0f, bl = 0.0f;
    StygianEditorRectDesc rect = {0};
    if (!stygian_editor_node_get_bounds(app->editor, source_id, &x, &y, &w, &h))
      return false;
    (void)stygian_editor_node_get_corner_radii(app->editor, source_id, &tl, &tr, &br,
                                               &bl);
    rect.x = x + dx;
    rect.y = y + dy;
    rect.w = w;
    rect.h = h;
    rect.radius[0] = tl;
    rect.radius[1] = tr;
    rect.radius[2] = br;
    rect.radius[3] = bl;
    rect.fill = color;
    rect.visible = visible;
    rect.z = 0.2f;
    new_id = stygian_editor_add_rect(app->editor, &rect);
  } else if (kind == STYGIAN_EDITOR_SHAPE_ELLIPSE) {
    float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
    StygianEditorEllipseDesc ellipse = {0};
    if (!stygian_editor_node_get_bounds(app->editor, source_id, &x, &y, &w, &h))
      return false;
    ellipse.x = x + dx;
    ellipse.y = y + dy;
    ellipse.w = w;
    ellipse.h = h;
    ellipse.fill = color;
    ellipse.visible = visible;
    ellipse.z = 0.2f;
    new_id = stygian_editor_add_ellipse(app->editor, &ellipse);
  } else {
    // Keep this loud until clone parity exists for every node kind.
    app_set_status(app,
                   "Repeater currently supports Rectangle/Ellipse only.");
    return false;
  }

  if (new_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  if (parent_id != STYGIAN_EDITOR_INVALID_ID) {
    (void)stygian_editor_reparent_node(app->editor, new_id, parent_id, false);
  }
  snprintf(generated_name, sizeof(generated_name), "%s_rep_%03u", base_name,
           index + 1u);
  (void)stygian_editor_node_set_name(app->editor, new_id, generated_name);
  if (out_new_id)
    *out_new_id = new_id;
  return true;
}

static bool app_apply_repeater_from_selection(EditorBootstrapApp *app) {
  StygianEditorNodeId source = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId last = STYGIAN_EDITOR_INVALID_ID;
  uint32_t copies = 0u;
  float step_x = 0.0f;
  float step_y = 0.0f;
  if (!app || !app->editor)
    return false;
  source = stygian_editor_selected_node(app->editor);
  if (source == STYGIAN_EDITOR_INVALID_ID) {
    app_set_status(app, "Repeater needs one selected source node.");
    return false;
  }
  if (!app_parse_u32_or_default(app->repeater_count, 3u, &copies) || copies == 0u ||
      copies > 256u) {
    app_set_status(app, "Repeater count must be 1..256.");
    return false;
  }
  if (!app_parse_float_text(app->repeater_step_x, &step_x) ||
      !app_parse_float_text(app->repeater_step_y, &step_y)) {
    app_set_status(app, "Repeater step values must be numbers.");
    return false;
  }
  for (uint32_t i = 0u; i < copies; ++i) {
    if (!app_repeater_clone_once(app, source, step_x * (float)(i + 1u),
                                 step_y * (float)(i + 1u), i, &last)) {
      if (!app->status_text[0]) {
        app_set_status(app, "Repeater failed while creating instances.");
      }
      return false;
    }
  }
  if (last != STYGIAN_EDITOR_INVALID_ID)
    (void)stygian_editor_select_node(app->editor, last, false);
  app_set_status(app, "Repeater generated deterministic instance set.");
  return true;
}

#if 0
static bool app_apply_boolean_from_selection(EditorBootstrapApp *app) {
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  uint32_t count = 0u;
  StygianEditorBooleanOp op = STYGIAN_EDITOR_BOOLEAN_UNION;
  StygianEditorNodeId bool_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId flat_id = STYGIAN_EDITOR_INVALID_ID;
  if (!app || !app->editor)
    return false;
  count = app_selected_nodes(app, ids, APP_MAX_TRANSFORM_SELECTION);
  if (count < 2u) {
    app_set_status(app, "Boolean needs at least 2 selected nodes.");
    return false;
  }
  if (app->boolean_op_choice == 1)
    op = STYGIAN_EDITOR_BOOLEAN_SUBTRACT;
  else if (app->boolean_op_choice == 2)
    op = STYGIAN_EDITOR_BOOLEAN_INTERSECT;
  else if (app->boolean_op_choice == 3)
    op = STYGIAN_EDITOR_BOOLEAN_EXCLUDE;
  bool_id = stygian_editor_boolean_compose(app->editor, op, ids, count, true);
  if (bool_id == STYGIAN_EDITOR_INVALID_ID) {
    app_set_status(app, "Boolean compose failed. Check selection kinds.");
    return false;
  }
  if (app->boolean_flatten_after) {
    if (!stygian_editor_boolean_flatten(app->editor, bool_id, &flat_id) ||
        flat_id == STYGIAN_EDITOR_INVALID_ID) {
      app_set_status(app, "Boolean flatten failed.");
      return false;
    }
    (void)stygian_editor_select_node(app->editor, flat_id, false);
  } else {
    (void)stygian_editor_select_node(app->editor, bool_id, false);
  }
  if (app->boolean_request_smooth_fallback) {
    app_set_status(
        app,
        "Smooth/metaball merge requested: exported as deterministic boolean fallback.");
  } else {
    app_set_status(app, "Boolean composed.");
  }
  return true;
}
#endif

static void app_draw_smart_guides(EditorBootstrapApp *app) {
  float wx = 0.0f, wy = 0.0f;
  float sx = 0.0f, sy = 0.0f;
  float vx = 0.0f, vy = 0.0f;
  bool show_x = false;
  bool show_y = false;
  char reason[96];
  if (!app || !app->editor)
    return;
  app->snap_feedback_active = false;
  if (!(app->dragging_node || app->resizing_node))
    return;
  if (!app_point_in_rect((float)app->last_mouse_x, (float)app->last_mouse_y,
                         app->layout.viewport_x, app->layout.viewport_y,
                         app->layout.viewport_w, app->layout.viewport_h)) {
    return;
  }
  stygian_editor_view_to_world(app->editor, (float)app->last_mouse_x,
                               (float)app->last_mouse_y, &wx, &wy);
  sx = wx;
  sy = wy;
  stygian_editor_snap_world_point(app->editor, wx, wy, &sx, &sy);
  show_x = fabsf(sx - wx) >= 0.001f;
  show_y = fabsf(sy - wy) >= 0.001f;
  if (!show_x && !show_y)
    return;
  stygian_editor_world_to_view(app->editor, sx, sy, &vx, &vy);
  if (show_x) {
    stygian_line(app->ctx, vx, app->layout.viewport_y, vx,
                 app->layout.viewport_y + app->layout.viewport_h, 1.2f, 0.25f,
                 0.84f, 0.99f, 0.86f);
  }
  if (show_y) {
    stygian_line(app->ctx, app->layout.viewport_x, vy,
                 app->layout.viewport_x + app->layout.viewport_w, vy, 1.2f, 0.25f,
                 0.84f, 0.99f, 0.86f);
  }
  snprintf(reason, sizeof(reason), "%s%s%s%s%s",
           app->snap_use_grid ? "grid" : "",
           (app->snap_use_grid && (app->snap_use_guides || app->snap_use_bounds ||
                                   app->snap_use_parent_edges))
               ? " + "
               : "",
           app->snap_use_guides ? "guides" : "",
           (app->snap_use_guides && (app->snap_use_bounds || app->snap_use_parent_edges))
               ? " + "
               : "",
           app->snap_use_bounds
               ? (app->snap_use_parent_edges ? "bounds + parent" : "bounds")
               : (app->snap_use_parent_edges ? "parent" : ""));
  if (reason[0] == '\0')
    snprintf(reason, sizeof(reason), "snap");
  snprintf(app->snap_feedback_text, sizeof(app->snap_feedback_text), "%s%s%s via %s",
           show_x ? "X" : "", (show_x && show_y) ? " + " : "", show_y ? "Y" : "",
           reason);
  app->snap_feedback_view_x = vx + 10.0f;
  app->snap_feedback_view_y = vy - 18.0f;
  if (app->snap_feedback_view_y < app->layout.viewport_y + 10.0f)
    app->snap_feedback_view_y = vy + 8.0f;
  app->snap_feedback_active = true;
}

#if 0
static StygianEditorEventKind app_event_kind_from_choice(int choice) {
  if (choice == 1)
    return STYGIAN_EDITOR_EVENT_RELEASE;
  if (choice == 2)
    return STYGIAN_EDITOR_EVENT_VALUE_CHANGED;
  return STYGIAN_EDITOR_EVENT_PRESS;
}
#endif

#if 0
static StygianEditorPropertyKind app_property_kind_from_choice(int choice) {
  if (choice == 1)
    return STYGIAN_EDITOR_PROP_Y;
  if (choice == 2)
    return STYGIAN_EDITOR_PROP_OPACITY;
  return STYGIAN_EDITOR_PROP_X;
}
#endif

#if 0
static StygianEditorBehaviorActionKind app_action_kind_from_choice(int choice) {
  if (choice == 1)
    return STYGIAN_EDITOR_ACTION_SET_PROPERTY;
  if (choice == 2)
    return STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY;
  if (choice == 3)
    return STYGIAN_EDITOR_ACTION_SET_VARIABLE;
  if (choice == 4)
    return STYGIAN_EDITOR_ACTION_NAVIGATE;
  return STYGIAN_EDITOR_ACTION_ANIMATE;
}
#endif

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

static StygianEditorPropertyKind
app_timeline_property_from_choice(int choice) {
  switch (choice) {
  case 1:
    return STYGIAN_EDITOR_PROP_Y;
  case 2:
    return STYGIAN_EDITOR_PROP_WIDTH;
  case 3:
    return STYGIAN_EDITOR_PROP_HEIGHT;
  case 4:
    return STYGIAN_EDITOR_PROP_OPACITY;
  default:
    return STYGIAN_EDITOR_PROP_X;
  }
}

static const char *app_timeline_property_label_from_choice(int choice) {
  return app_property_kind_name(app_timeline_property_from_choice(choice));
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

#if 0
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
#endif

static bool app_write_text_file(const char *path, const char *text) {
  FILE *f = NULL;
  if (!path || !text)
    return false;
  f = fopen(path, "wb");
  if (!f)
    return false;
  fwrite(text, 1u, strlen(text), f);
  fclose(f);
  return true;
}

static bool app_file_exists(const char *path) {
  FILE *f = NULL;
  if (!path)
    return false;
  f = fopen(path, "rb");
  if (!f)
    return false;
  fclose(f);
  return true;
}

static bool app_read_text_file(const char *path, char *out, size_t out_capacity) {
  FILE *f = NULL;
  size_t n = 0u;
  if (!path || !out || out_capacity < 2u)
    return false;
  f = fopen(path, "rb");
  if (!f)
    return false;
  n = fread(out, 1u, out_capacity - 1u, f);
  out[n] = '\0';
  fclose(f);
  return true;
}

static bool app_ensure_dir(const char *path) {
  if (!path || !path[0])
    return false;
#ifdef _WIN32
  if (_mkdir(path) == 0 || errno == EEXIST)
    return true;
#else
  if (mkdir(path, 0777) == 0 || errno == EEXIST)
    return true;
#endif
  return false;
}

static bool app_node_exists(const EditorBootstrapApp *app,
                            StygianEditorNodeId node_id) {
  StygianEditorShapeKind kind = STYGIAN_EDITOR_SHAPE_RECT;
  if (!app || !app->editor || node_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  return stygian_editor_node_get_shape_kind(app->editor, node_id, &kind);
}

#if 0
static void app_node_binding_state(const EditorBootstrapApp *app,
                                   StygianEditorNodeId node_id,
                                   uint32_t *out_connected,
                                   uint32_t *out_missing) {
  uint32_t connected = 0u;
  uint32_t missing = 0u;
  uint32_t behavior_count = 0u;
  if (out_connected)
    *out_connected = 0u;
  if (out_missing)
    *out_missing = 0u;
  if (!app || !app->editor || node_id == STYGIAN_EDITOR_INVALID_ID)
    return;
  behavior_count = stygian_editor_behavior_count(app->editor);
  for (uint32_t i = 0u; i < behavior_count; ++i) {
    StygianEditorBehaviorRule rule;
    StygianEditorBehaviorId behavior_id = 0u;
    StygianEditorNodeId target = STYGIAN_EDITOR_INVALID_ID;
    if (!stygian_editor_get_behavior_rule(app->editor, i, &behavior_id, &rule))
      continue;
    (void)behavior_id;
    if (rule.action_kind == STYGIAN_EDITOR_ACTION_ANIMATE)
      target = rule.animate.target_node;
    else if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY)
      target = rule.set_property.target_node;
    else if (rule.action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY)
      target = rule.toggle_visibility.target_node;
    else
      target = rule.trigger_node;
    if (rule.trigger_node != node_id && target != node_id)
      continue;
    if (!app_node_exists(app, rule.trigger_node) ||
        (target != STYGIAN_EDITOR_INVALID_ID && !app_node_exists(app, target))) {
      missing += 1u;
    } else {
      connected += 1u;
    }
  }
  if (out_connected)
    *out_connected = connected;
  if (out_missing)
    *out_missing = missing;
}
#endif

static bool app_hook_function_exists(const char *hooks_source,
                                     const char *function_name) {
  char needle[160];
  if (!hooks_source || !function_name || !function_name[0])
    return false;
  snprintf(needle, sizeof(needle), "%s(", function_name);
  return strstr(hooks_source, needle) != NULL;
}

static bool app_append_hook_stub_if_missing(const char *hooks_path,
                                            const char *function_name,
                                            const char *stub_body) {
  FILE *f = NULL;
  if (!hooks_path || !function_name || !stub_body)
    return false;
  f = fopen(hooks_path, "ab");
  if (!f)
    return false;
  fputs("\n", f);
  fputs(stub_body, f);
  fclose(f);
  return true;
}

static uint32_t app_text_line_for_substring(const char *text,
                                            const char *needle) {
  uint32_t line = 1u;
  const char *match = NULL;
  if (!text || !needle || !needle[0])
    return 0u;
  match = strstr(text, needle);
  if (!match)
    return 0u;
  while (text < match) {
    if (*text == '\n')
      line += 1u;
    text += 1;
  }
  return line;
}

static bool app_open_text_file_in_default_editor(const char *path) {
  char cmd[768];
  int rc = 0;
  if (!path || !path[0])
    return false;
#ifdef _WIN32
  snprintf(cmd, sizeof(cmd), "start \"\" \"%s\"", path);
#else
  snprintf(cmd, sizeof(cmd), "xdg-open \"%s\" >/dev/null 2>&1 &", path);
#endif
  rc = system(cmd);
  return rc == 0;
}

static bool app_build_node_path(const EditorBootstrapApp *app,
                                StygianEditorNodeId node_id, char *out_path,
                                size_t out_path_capacity) {
  StygianEditorNodeId chain[64];
  uint32_t depth = 0u;
  StygianEditorNodeId cursor = node_id;
  if (!app || !app->editor || !out_path || out_path_capacity < 4u ||
      node_id == STYGIAN_EDITOR_INVALID_ID) {
    return false;
  }
  while (cursor != STYGIAN_EDITOR_INVALID_ID && depth < 64u) {
    chain[depth++] = cursor;
    if (!stygian_editor_node_get_parent(app->editor, cursor, &cursor))
      break;
  }
  out_path[0] = '\0';
  for (uint32_t i = depth; i > 0u; --i) {
    char name[64];
    char seg[96];
    StygianEditorNodeId id = chain[i - 1u];
    if (!stygian_editor_node_get_name(app->editor, id, name, sizeof(name)) ||
        !name[0]) {
      snprintf(name, sizeof(name), "node_%u", id);
    }
    if (out_path[0])
      strncat(out_path, "/", out_path_capacity - strlen(out_path) - 1u);
    snprintf(seg, sizeof(seg), "%s", name);
    strncat(out_path, seg, out_path_capacity - strlen(out_path) - 1u);
  }
  return true;
}

static bool app_export_runtime_bundle(EditorBootstrapApp *app,
                                      const char *out_root,
                                      const char *report_path) {
  char generated_dir[320];
  char scene_h[320];
  char scene_c[320];
  char manifest_path[320];
  char hooks_h_path[320];
  char hooks_c_path[320];
  char code[524288];
  char hooks_text[32768];
  char report_text[8192];
  bool hooks_h_existed = false;
  bool hooks_c_existed = false;
  bool hook_set_var_exists = false;
  bool hook_navigate_exists = false;
  bool hook_before_exists = false;
  bool hook_after_exists = false;
  uint32_t behavior_count = 0u;
  uint32_t required_set_var = 0u;
  uint32_t required_navigate = 0u;
  uint32_t missing_binding_count = 0u;
  uint32_t diagnostics_count = 0u;
  StygianEditorExportDiagnostic diags[64];
  size_t code_need = 0u;
  FILE *f = NULL;
  uint64_t started_ms = app_now_ms_system();

  if (!app || !app->editor || !out_root || !report_path)
    return false;

  snprintf(generated_dir, sizeof(generated_dir), "%s/generated", out_root);
  snprintf(scene_h, sizeof(scene_h), "%s/stygian_editor_scene.generated.h",
           generated_dir);
  snprintf(scene_c, sizeof(scene_c), "%s/stygian_editor_scene.generated.c",
           generated_dir);
  snprintf(manifest_path, sizeof(manifest_path),
           "%s/stygian_editor_manifest.generated.json", generated_dir);
  snprintf(hooks_h_path, sizeof(hooks_h_path), "%s/stygian_editor_hooks.h",
           out_root);
  snprintf(hooks_c_path, sizeof(hooks_c_path), "%s/stygian_editor_hooks.c",
           out_root);

  if (!app_ensure_dir(out_root) || !app_ensure_dir(generated_dir))
    return false;

  code_need = stygian_editor_build_c23(app->editor, NULL, 0u);
  if (code_need < 2u || code_need > sizeof(code))
    return false;
  if (stygian_editor_build_c23(app->editor, code, sizeof(code)) != code_need)
    return false;

  if (!app_write_text_file(
          scene_h,
          "/* AUTO-GENERATED FILE. DO NOT EDIT. */\n"
          "/* Exporter-owned: regenerated on every export. */\n"
          "/* User logic file locations: ../stygian_editor_hooks.h and ../stygian_editor_hooks.c */\n"
          "#pragma once\n"
          "#include \"stygian.h\"\n"
          "#include <stdint.h>\n\n"
          "const void *stygian_editor_generated_behavior_rules(uint32_t *out_count);\n"
          "int stygian_editor_generated_node_index(uint32_t node_id);\n"
          "void stygian_editor_generated_build_scene(StygianContext *ctx,\n"
          "                                          StygianElement *out_elements,\n"
          "                                          uint32_t out_count);\n"
          "void stygian_editor_generated_draw_paths(StygianContext *ctx);\n")) {
    return false;
  }

  f = fopen(scene_c, "wb");
  if (!f)
    return false;
  fputs("/* AUTO-GENERATED FILE. DO NOT EDIT. */\n", f);
  fputs("/* Exporter-owned: regenerated on every export. */\n", f);
  fputs(
      "/* User logic file locations: ../stygian_editor_hooks.h and "
      "../stygian_editor_hooks.c */\n",
      f);
  fputs("#include \"stygian_editor_scene.generated.h\"\n\n", f);
  fputs(code, f);
  fclose(f);

  if (!app_write_text_file(
          manifest_path,
          "{\n"
          "  \"schema\": \"stygian-editor-generated-layout\",\n"
          "  \"version\": 1,\n"
          "  \"ownership\": {\n"
          "    \"generated\": [\n"
          "      \"generated/stygian_editor_scene.generated.h\",\n"
          "      \"generated/stygian_editor_scene.generated.c\",\n"
          "      \"generated/stygian_editor_manifest.generated.json\"\n"
          "    ],\n"
          "    \"hooks\": [\"stygian_editor_hooks.h\", \"stygian_editor_hooks.c\"],\n"
          "    \"rules\": \"generated files are exporter-owned; hooks are user-owned and never overwritten\"\n"
          "  }\n"
          "}\n")) {
    return false;
  }

  hooks_h_existed = app_file_exists(hooks_h_path);
  hooks_c_existed = app_file_exists(hooks_c_path);
  if (!hooks_h_existed) {
    if (!app_write_text_file(
            hooks_h_path,
            "/* HANDWRITTEN FILE. EDIT FREELY. */\n"
            "/* User-owned: exporter may create this file once and never overwrites it. */\n"
            "#pragma once\n"
            "#include \"stygian.h\"\n\n"
            "void stygian_editor_hooks_before_frame(StygianContext *ctx);\n"
            "void stygian_editor_hooks_after_frame(StygianContext *ctx);\n"
            "void stygian_editor_generated_hook_set_variable(\n"
            "    const char *name, uint32_t kind, float number_value,\n"
            "    uint32_t mode_index, StygianColor color_value);\n"
            "void stygian_editor_generated_hook_navigate(const char *target);\n"
            "/* Optional dynamic tabs hooks for data-driven tab counts. */\n"
            "uint32_t stygian_editor_hooks_tab_count(const char *tabs_symbol);\n"
            "const char *stygian_editor_hooks_tab_label(const char *tabs_symbol,\n"
            "                                           uint32_t index);\n")) {
      return false;
    }
  }
  if (!hooks_c_existed) {
    if (!app_write_text_file(
            hooks_c_path,
            "/* HANDWRITTEN FILE. EDIT FREELY. */\n"
            "/* User-owned: exporter may create this file once and never overwrites it. */\n"
            "#include \"stygian_editor_hooks.h\"\n\n"
            "void stygian_editor_hooks_before_frame(StygianContext *ctx) {\n"
            "  (void)ctx;\n"
            "}\n\n"
            "void stygian_editor_hooks_after_frame(StygianContext *ctx) {\n"
            "  (void)ctx;\n"
            "}\n\n"
            "void stygian_editor_generated_hook_set_variable(\n"
            "    const char *name, uint32_t kind, float number_value,\n"
            "    uint32_t mode_index, StygianColor color_value) {\n"
            "  (void)name;\n"
            "  (void)kind;\n"
            "  (void)number_value;\n"
            "  (void)mode_index;\n"
            "  (void)color_value;\n"
            "}\n\n"
            "void stygian_editor_generated_hook_navigate(const char *target) {\n"
            "  (void)target;\n"
            "}\n")) {
      return false;
    }
  }

  hooks_text[0] = '\0';
  (void)app_read_text_file(hooks_c_path, hooks_text, sizeof(hooks_text));
  hook_before_exists = app_hook_function_exists(hooks_text,
                                                "stygian_editor_hooks_before_frame");
  hook_after_exists = app_hook_function_exists(hooks_text,
                                               "stygian_editor_hooks_after_frame");
  hook_set_var_exists = app_hook_function_exists(
      hooks_text, "stygian_editor_generated_hook_set_variable");
  hook_navigate_exists = app_hook_function_exists(
      hooks_text, "stygian_editor_generated_hook_navigate");

  if (!hook_before_exists) {
    if (!app_append_hook_stub_if_missing(
            hooks_c_path, "stygian_editor_hooks_before_frame",
            "void stygian_editor_hooks_before_frame(StygianContext *ctx) {\n"
            "  (void)ctx;\n"
            "  /* TODO: pre-frame user hook */\n"
            "}\n")) {
      return false;
    }
  }
  if (!hook_after_exists) {
    if (!app_append_hook_stub_if_missing(
            hooks_c_path, "stygian_editor_hooks_after_frame",
            "void stygian_editor_hooks_after_frame(StygianContext *ctx) {\n"
            "  (void)ctx;\n"
            "  /* TODO: post-frame user hook */\n"
            "}\n")) {
      return false;
    }
  }
  if (!hook_set_var_exists) {
    if (!app_append_hook_stub_if_missing(
            hooks_c_path, "stygian_editor_generated_hook_set_variable",
            "void stygian_editor_generated_hook_set_variable(\n"
            "    const char *name, uint32_t kind, float number_value,\n"
            "    uint32_t mode_index, StygianColor color_value) {\n"
            "  (void)name;\n"
            "  (void)kind;\n"
            "  (void)number_value;\n"
            "  (void)mode_index;\n"
            "  (void)color_value;\n"
            "  /* TODO: handle exported set-variable behavior in host code */\n"
            "}\n")) {
      return false;
    }
  }
  if (!hook_navigate_exists) {
    if (!app_append_hook_stub_if_missing(
            hooks_c_path, "stygian_editor_generated_hook_navigate",
            "void stygian_editor_generated_hook_navigate(const char *target) {\n"
            "  (void)target;\n"
            "  /* TODO: handle exported navigation behavior in host code */\n"
            "}\n")) {
      return false;
    }
  }
  if (!app_hook_function_exists(hooks_text, "stygian_editor_hooks_tab_count")) {
    if (!app_append_hook_stub_if_missing(
            hooks_c_path, "stygian_editor_hooks_tab_count",
            "uint32_t stygian_editor_hooks_tab_count(const char *tabs_symbol) {\n"
            "  (void)tabs_symbol;\n"
            "  /* Optional hook for dynamic tab counts. */\n"
            "  return 0u;\n"
            "}\n")) {
      return false;
    }
  }
  if (!app_hook_function_exists(hooks_text, "stygian_editor_hooks_tab_label")) {
    if (!app_append_hook_stub_if_missing(
            hooks_c_path, "stygian_editor_hooks_tab_label",
            "const char *stygian_editor_hooks_tab_label(const char *tabs_symbol,\n"
            "                                           uint32_t index) {\n"
            "  (void)tabs_symbol;\n"
            "  (void)index;\n"
            "  /* Optional hook for dynamic tab labels. */\n"
            "  return \"\";\n"
            "}\n")) {
      return false;
    }
  }

  behavior_count = stygian_editor_behavior_count(app->editor);
  for (uint32_t i = 0u; i < behavior_count; ++i) {
    StygianEditorBehaviorRule rule;
    StygianEditorBehaviorId behavior_id = 0u;
    StygianEditorNodeId target_id = STYGIAN_EDITOR_INVALID_ID;
    if (!stygian_editor_get_behavior_rule(app->editor, i, &behavior_id, &rule))
      continue;
    (void)behavior_id;
    if (!app_node_exists(app, rule.trigger_node))
      missing_binding_count += 1u;
    if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_VARIABLE) {
      required_set_var += 1u;
      if (!hook_set_var_exists)
        missing_binding_count += 1u;
    } else if (rule.action_kind == STYGIAN_EDITOR_ACTION_NAVIGATE) {
      required_navigate += 1u;
      if (!hook_navigate_exists)
        missing_binding_count += 1u;
    } else {
      if (rule.action_kind == STYGIAN_EDITOR_ACTION_ANIMATE)
        target_id = rule.animate.target_node;
      else if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY)
        target_id = rule.set_property.target_node;
      else if (rule.action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY)
        target_id = rule.toggle_visibility.target_node;
      if (!app_node_exists(app, target_id))
        missing_binding_count += 1u;
    }
  }

  diagnostics_count = stygian_editor_collect_export_diagnostics(
      app->editor, diags, (uint32_t)(sizeof(diags) / sizeof(diags[0])));
  snprintf(
      report_text, sizeof(report_text),
      "Stygian Export Report\n"
      "=====================\n"
      "Generated zone (overwritten):\n"
      "  - generated/stygian_editor_scene.generated.h\n"
      "  - generated/stygian_editor_scene.generated.c\n"
      "  - generated/stygian_editor_manifest.generated.json\n"
      "Hook zone (preserved):\n"
      "  - stygian_editor_hooks.h (%s)\n"
      "  - stygian_editor_hooks.c (%s)\n"
      "Binding zone:\n"
      "  - behavior rules: %u\n"
      "  - required set_variable hooks: %u\n"
      "  - required navigate hooks: %u\n"
      "  - missing/stale bindings detected: %u\n"
      "  - optional dynamic tabs hooks: stygian_editor_hooks_tab_count/tab_label\n"
      "Diagnostics:\n"
      "  - export diagnostics captured: %u\n"
      "Rule:\n"
      "  Layout is generated. Your logic lives in hooks. Export never overwrites hooks.\n",
      hooks_h_existed ? "preserved" : "created",
      hooks_c_existed ? "preserved" : "created", behavior_count, required_set_var,
      required_navigate, missing_binding_count, diagnostics_count);
  if (!app_write_text_file(report_path, report_text))
    return false;
  app->perf_last_export_ms = app_elapsed_ms(started_ms, app_now_ms_system());
  return true;
}

static bool app_run_binding_doctor(EditorBootstrapApp *app, const char *out_path,
                                   const char *hooks_c_path) {
  FILE *f = NULL;
  char hooks_text[32768];
  uint32_t behavior_count = 0u;
  uint32_t issue_count = 0u;
  uint32_t diagnostics_count = 0u;
  StygianEditorExportDiagnostic diags[64];
  if (!app || !app->editor || !out_path || !hooks_c_path)
    return false;
  f = fopen(out_path, "wb");
  if (!f)
    return false;
  fputs("Stygian Binding Doctor\n", f);
  fputs("======================\n", f);
  hooks_text[0] = '\0';
  (void)app_read_text_file(hooks_c_path, hooks_text, sizeof(hooks_text));
  behavior_count = stygian_editor_behavior_count(app->editor);
  fprintf(f, "Behavior rules: %u\n", behavior_count);
  for (uint32_t i = 0u; i < behavior_count; ++i) {
    StygianEditorBehaviorRule rule;
    StygianEditorBehaviorId behavior_id = 0u;
    char trigger_path[256];
    StygianEditorNodeId target = STYGIAN_EDITOR_INVALID_ID;
    if (!stygian_editor_get_behavior_rule(app->editor, i, &behavior_id, &rule))
      continue;
    trigger_path[0] = '\0';
    (void)app_build_node_path(app, rule.trigger_node, trigger_path,
                              sizeof(trigger_path));
    if (!app_node_exists(app, rule.trigger_node)) {
      fprintf(f, "[issue] behavior %u trigger node missing: %u\n", behavior_id,
              rule.trigger_node);
      issue_count += 1u;
    }
    if (rule.action_kind == STYGIAN_EDITOR_ACTION_NAVIGATE) {
      if (!app_hook_function_exists(hooks_text,
                                    "stygian_editor_generated_hook_navigate")) {
        fprintf(f, "[issue] behavior %u (%s) missing hook: "
                   "stygian_editor_generated_hook_navigate\n",
                behavior_id, trigger_path[0] ? trigger_path : "unknown");
        issue_count += 1u;
      }
    } else if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_VARIABLE) {
      if (!app_hook_function_exists(hooks_text,
                                    "stygian_editor_generated_hook_set_variable")) {
        fprintf(f, "[issue] behavior %u (%s) missing hook: "
                   "stygian_editor_generated_hook_set_variable\n",
                behavior_id, trigger_path[0] ? trigger_path : "unknown");
        issue_count += 1u;
      }
    } else {
      if (rule.action_kind == STYGIAN_EDITOR_ACTION_ANIMATE)
        target = rule.animate.target_node;
      else if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY)
        target = rule.set_property.target_node;
      else if (rule.action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY)
        target = rule.toggle_visibility.target_node;
      if (!app_node_exists(app, target)) {
        fprintf(f, "[issue] behavior %u target node missing: %u\n", behavior_id,
                target);
        issue_count += 1u;
      }
    }
  }
  diagnostics_count = stygian_editor_collect_export_diagnostics(
      app->editor, diags, (uint32_t)(sizeof(diags) / sizeof(diags[0])));
  fprintf(f, "Export diagnostics: %u\n", diagnostics_count);
  for (uint32_t i = 0u; i < diagnostics_count; ++i) {
    fprintf(f, "  - node %u %s: %s\n", diags[i].node_id, diags[i].feature,
            diags[i].message);
  }
  fprintf(f, "Doctor result: %s\n", issue_count == 0u ? "clean" : "issues found");
  fclose(f);
  return true;
}

static void app_reset_interaction_state(EditorBootstrapApp *app);

static bool app_build_self_host_shell_scene(StygianEditor *editor,
                                            AppSelfHostStage stage) {
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorNodeId root_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId top_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId viewport_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId inspector_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId tools_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId behavior_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId console_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId slot_id = STYGIAN_EDITOR_INVALID_ID;

  if (!editor || stage < APP_SELF_HOST_STAGE_1 || stage > APP_SELF_HOST_STAGE_3)
    return false;

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 1440.0f;
  frame.h = 900.0f;
  frame.clip_content = false;
  frame.fill = stygian_editor_color_rgba(0.06f, 0.07f, 0.09f, 1.0f);
  frame.visible = true;
  frame.z = 0.0f;
  root_id = stygian_editor_add_frame(editor, &frame);
  if (!root_id)
    return false;
  (void)stygian_editor_node_set_name(editor, root_id, "editor_root");

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 1440.0f;
  frame.h = 58.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.20f, 0.20f, 0.22f, 1.0f);
  frame.z = 1.0f;
  top_id = stygian_editor_add_frame(editor, &frame);
  if (!top_id || !stygian_editor_reparent_node(editor, top_id, root_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, top_id, "top_chrome");
  (void)stygian_editor_node_set_constraints(editor, top_id,
                                            STYGIAN_EDITOR_CONSTRAINT_H_LEFT_RIGHT,
                                            STYGIAN_EDITOR_CONSTRAINT_V_TOP);

  frame.x = 0.0f;
  frame.y = 58.0f;
  frame.w = 1088.0f;
  frame.h = 842.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.05f, 0.06f, 0.08f, 1.0f);
  frame.z = 1.0f;
  viewport_id = stygian_editor_add_frame(editor, &frame);
  if (!viewport_id ||
      !stygian_editor_reparent_node(editor, viewport_id, root_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, viewport_id, "viewport_shell");
  (void)stygian_editor_node_set_constraints(
      editor, viewport_id, STYGIAN_EDITOR_CONSTRAINT_H_LEFT_RIGHT,
      STYGIAN_EDITOR_CONSTRAINT_V_TOP_BOTTOM);

  frame.x = 1088.0f;
  frame.y = 58.0f;
  frame.w = 352.0f;
  frame.h = 842.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.10f, 0.12f, 0.15f, 1.0f);
  frame.z = 1.0f;
  inspector_id = stygian_editor_add_frame(editor, &frame);
  if (!inspector_id ||
      !stygian_editor_reparent_node(editor, inspector_id, root_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, inspector_id, "inspector_shell");
  (void)stygian_editor_node_set_constraints(
      editor, inspector_id, STYGIAN_EDITOR_CONSTRAINT_H_RIGHT,
      STYGIAN_EDITOR_CONSTRAINT_V_TOP_BOTTOM);

  rect.x = 10.0f;
  rect.y = 6.0f;
  rect.w = 54.0f;
  rect.h = 22.0f;
  rect.fill = stygian_editor_color_rgba(0.24f, 0.24f, 0.26f, 1.0f);
  rect.visible = true;
  rect.z = 2.0f;
  slot_id = stygian_editor_add_rect(editor, &rect);
  if (!slot_id || !stygian_editor_reparent_node(editor, slot_id, top_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, slot_id, "file_menu_slot");

  rect.x = 80.0f;
  rect.y = 6.0f;
  rect.w = 110.0f;
  rect.h = 22.0f;
  rect.fill = stygian_editor_color_rgba(0.14f, 0.38f, 0.61f, 1.0f);
  slot_id = stygian_editor_add_rect(editor, &rect);
  if (!slot_id || !stygian_editor_reparent_node(editor, slot_id, top_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, slot_id, "place_button_slot");

  if (stage >= APP_SELF_HOST_STAGE_2) {
    frame.x = 1088.0f;
    frame.y = 58.0f;
    frame.w = 352.0f;
    frame.h = 380.0f;
    frame.clip_content = true;
    frame.fill = stygian_editor_color_rgba(0.08f, 0.10f, 0.13f, 1.0f);
    frame.z = 2.0f;
    behavior_id = stygian_editor_add_frame(editor, &frame);
    if (!behavior_id ||
        !stygian_editor_reparent_node(editor, behavior_id, inspector_id, false))
      return false;
    (void)stygian_editor_node_set_name(editor, behavior_id, "behavior_graph_surface");

    frame.x = 1088.0f;
    frame.y = 438.0f;
    frame.w = 352.0f;
    frame.h = 462.0f;
    frame.clip_content = true;
    frame.fill = stygian_editor_color_rgba(0.07f, 0.08f, 0.10f, 1.0f);
    frame.z = 2.0f;
    console_id = stygian_editor_add_frame(editor, &frame);
    if (!console_id ||
        !stygian_editor_reparent_node(editor, console_id, inspector_id, false))
      return false;
    (void)stygian_editor_node_set_name(editor, console_id, "console_surface");
  }

  if (stage >= APP_SELF_HOST_STAGE_3) {
    frame.x = 8.0f;
    frame.y = 8.0f;
    frame.w = 320.0f;
    frame.h = 44.0f;
    frame.clip_content = true;
    frame.fill = stygian_editor_color_rgba(0.12f, 0.13f, 0.16f, 1.0f);
    frame.z = 2.0f;
    tools_id = stygian_editor_add_frame(editor, &frame);
    if (!tools_id ||
        !stygian_editor_reparent_node(editor, tools_id, viewport_id, false))
      return false;
    (void)stygian_editor_node_set_name(editor, tools_id, "tools_surface");

    rect.x = 16.0f;
    rect.y = 14.0f;
    rect.w = 132.0f;
    rect.h = 22.0f;
    rect.fill = stygian_editor_color_rgba(0.16f, 0.45f, 0.72f, 1.0f);
    slot_id = stygian_editor_add_rect(editor, &rect);
    if (!slot_id ||
        !stygian_editor_reparent_node(editor, slot_id, tools_id, false))
      return false;
    (void)stygian_editor_node_set_name(editor, slot_id, "tool_place_shape");

    rect.x = 154.0f;
    rect.y = 14.0f;
    rect.w = 132.0f;
    rect.h = 22.0f;
    rect.fill = stygian_editor_color_rgba(0.18f, 0.32f, 0.52f, 1.0f);
    slot_id = stygian_editor_add_rect(editor, &rect);
    if (!slot_id ||
        !stygian_editor_reparent_node(editor, slot_id, tools_id, false))
      return false;
    (void)stygian_editor_node_set_name(editor, slot_id, "tool_point_place");
  }

  return true;
}

static bool app_export_editor_layout_shell(const char *output_path) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  char generated_h[260];
  char host_h[260];
  char host_c[260];
  char *code = NULL;
  size_t need = 0u;
  FILE *file = NULL;
  bool ok = false;

  if (!output_path || !output_path[0])
    return false;
  snprintf(generated_h, sizeof(generated_h),
           "editor/build/stygian_editor_layout_shell.generated.h");
  snprintf(host_h, sizeof(host_h), "editor/build/stygian_editor_layout_shell_host.h");
  snprintf(host_c, sizeof(host_c), "editor/build/stygian_editor_layout_shell_host.c");

  cfg.max_nodes = 256u;
  cfg.max_path_points = 1024u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 32u;

  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return false;

  if (!app_build_self_host_shell_scene(editor, APP_SELF_HOST_STAGE_3))
    goto cleanup;

  need = stygian_editor_build_c23(editor, NULL, 0u);
  if (need < 2u)
    goto cleanup;
  code = (char *)malloc(need);
  if (!code)
    goto cleanup;
  if (stygian_editor_build_c23(editor, code, need) != need)
    goto cleanup;

  file = fopen(output_path, "wb");
  if (!file)
    goto cleanup;
  fputs("/* AUTO-GENERATED FILE. DO NOT EDIT. */\n", file);
  fputs("/* Exporter-owned: regenerated on every export. */\n", file);
  fputs(
      "/* User logic file locations: ../stygian_editor_layout_shell_host.h and "
      "../stygian_editor_layout_shell_host.c */\n",
      file);
  fputs("#include \"stygian_editor_layout_shell.generated.h\"\n\n", file);
  fwrite(code, 1u, strlen(code), file);
  fclose(file);
  file = NULL;

  if (!app_write_text_file(
          generated_h,
          "/* AUTO-GENERATED FILE. DO NOT EDIT. */\n"
          "/* Exporter-owned: regenerated on every export. */\n"
          "/* User logic file locations: ../stygian_editor_layout_shell_host.h and ../stygian_editor_layout_shell_host.c */\n"
          "#pragma once\n"
          "#include \"stygian.h\"\n"
          "#include <stdint.h>\n\n"
          "const void *stygian_editor_generated_behavior_rules(uint32_t *out_count);\n"
          "int stygian_editor_generated_node_index(uint32_t node_id);\n"
          "void stygian_editor_generated_build_scene(StygianContext *ctx,\n"
          "                                        StygianElement *out_elements,\n"
          "                                        uint32_t out_count);\n"
          "void stygian_editor_generated_draw_paths(StygianContext *ctx);\n"
          "void stygian_editor_layout_shell_before_frame(StygianContext *ctx);\n"
          "void stygian_editor_layout_shell_after_frame(StygianContext *ctx);\n")) {
    goto cleanup;
  }

  if (!app_file_exists(host_h)) {
    if (!app_write_text_file(
            host_h,
            "/* HANDWRITTEN FILE. EDIT FREELY. */\n"
            "/* User-owned: exporter may create this file once and never overwrites it. */\n"
            "#pragma once\n"
            "#include \"stygian.h\"\n\n"
            "void stygian_editor_layout_shell_before_frame(StygianContext *ctx);\n"
            "void stygian_editor_layout_shell_after_frame(StygianContext *ctx);\n")) {
      goto cleanup;
    }
  }
  if (!app_file_exists(host_c)) {
    if (!app_write_text_file(
            host_c,
            "/* HANDWRITTEN FILE. EDIT FREELY. */\n"
            "/* User-owned: exporter may create this file once and never overwrites it. */\n"
            "#include \"stygian_editor_layout_shell_host.h\"\n\n"
            "void stygian_editor_layout_shell_before_frame(StygianContext *ctx) {\n"
            "  (void)ctx;\n"
            "}\n\n"
            "void stygian_editor_layout_shell_after_frame(StygianContext *ctx) {\n"
            "  (void)ctx;\n"
            "}\n")) {
      goto cleanup;
    }
  }

  ok = true;

cleanup:
  if (file)
    fclose(file);
  free(code);
  if (editor)
    stygian_editor_destroy(editor);
  return ok;
}

static bool app_seed_self_host_surface_stage(EditorBootstrapApp *app,
                                             AppSelfHostStage stage) {
  if (!app || !app->editor)
    return false;
  stygian_editor_reset(app->editor);
  if (!app_build_self_host_shell_scene(app->editor, stage))
    return false;
  app_reset_interaction_state(app);
  app->camera_pan_x = 24.0f;
  app->camera_pan_y = app_top_chrome_height() + 24.0f;
  app->camera_zoom = 1.0f;
  app_sync_grid_config(app);
  app_sync_snap_flags_from_editor(app);
  app_sync_editor_viewport(app);
  return true;
}

static void app_reset_interaction_state(EditorBootstrapApp *app) {
  if (!app)
    return;
  app->dragging_node = false;
  app->resizing_node = false;
  app->rotating_selection = false;
  app->marquee_selecting = false;
  app->placing_primitive = false;
  app->panning = false;
  app->path_tool_active = false;
  app->active_path = STYGIAN_EDITOR_INVALID_ID;
  app->current_tool = APP_TOOL_SELECT;
  app->path_preview_count = 0u;
  app->drag_selection_count = 0u;
  app->resize_selection_count = 0u;
  app->rotate_selection_count = 0u;
  app->file_menu_open = false;
  app->primitive_menu_open = false;
  app->context_menu_open = false;
  app->timeline_scrubbing = false;
  app->timeline_dragging_key = false;
  app->timeline_dragging_curve = false;
  app->snap_feedback_active = false;
}

static bool app_save_project(EditorBootstrapApp *app) {
  uint64_t started_ms = app_now_ms_system();
  bool ok;
  if (!app || !app->editor || !app->project_path[0])
    return false;
  ok = stygian_editor_save_project_file(app->editor, app->project_path);
  app->perf_last_export_ms = app_elapsed_ms(started_ms, app_now_ms_system());
  if (ok) {
    app_push_recent_project(app, app->project_path);
    snprintf(app->project_path_draft, sizeof(app->project_path_draft), "%s",
             app->project_path);
  }
  return ok;
}

static bool app_load_project(EditorBootstrapApp *app) {
  uint64_t started_ms = app_now_ms_system();
  if (!app || !app->editor || !app->project_path[0])
    return false;
  if (!stygian_editor_load_project_file(app->editor, app->project_path))
    return false;
  app->perf_last_load_ms = app_elapsed_ms(started_ms, app_now_ms_system());
  app_push_recent_project(app, app->project_path);
  snprintf(app->project_path_draft, sizeof(app->project_path_draft), "%s",
           app->project_path);
  app_reset_interaction_state(app);
  app_sync_grid_config(app);
  app_sync_snap_flags_from_editor(app);
  app_sync_editor_viewport(app);
  return true;
}

static bool app_save_project_as(EditorBootstrapApp *app) {
  if (!app || !app->project_path_draft[0])
    return false;
  snprintf(app->project_path, sizeof(app->project_path), "%s",
           app->project_path_draft);
  return app_save_project(app);
}

static bool app_open_recent_project(EditorBootstrapApp *app, uint32_t recent_index) {
  if (!app || recent_index >= app->recent_project_count ||
      !app->recent_project_paths[recent_index][0]) {
    return false;
  }
  snprintf(app->project_path, sizeof(app->project_path), "%s",
           app->recent_project_paths[recent_index]);
  return app_load_project(app);
}

static bool app_import_image_from_draft(EditorBootstrapApp *app) {
  StygianEditorNodeId selected = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorShapeKind kind = STYGIAN_EDITOR_SHAPE_RECT;
  float wx = 0.0f;
  float wy = 0.0f;
  StygianEditorNodeId id = STYGIAN_EDITOR_INVALID_ID;

  if (!app || !app->editor || !app->import_image_source[0])
    return false;

  selected = stygian_editor_selected_node(app->editor);
  if (selected != STYGIAN_EDITOR_INVALID_ID &&
      stygian_editor_node_get_shape_kind(app->editor, selected, &kind) &&
      kind == STYGIAN_EDITOR_SHAPE_IMAGE) {
    return stygian_editor_node_set_image_source(app->editor, selected,
                                                app->import_image_source);
  }

  stygian_editor_view_to_world(app->editor,
                               app->layout.viewport_x +
                                   app->layout.viewport_w * 0.5f - 110.0f,
                               app->layout.viewport_y +
                                   app->layout.viewport_h * 0.42f,
                               &wx, &wy);
  id = app_add_image_node(app, wx, wy, app->import_image_source);
  if (id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  (void)stygian_editor_select_node(app->editor, id, false);
  app_sync_selected_name(app);
  return true;
}

static void app_default_primitive_size(AppPrimitiveKind primitive, float *out_w,
                                       float *out_h, bool *out_is_ellipse) {
  float w = 140.0f;
  float h = 90.0f;
  bool is_ellipse = false;
  switch (primitive) {
  case APP_PRIMITIVE_SQUARE:
    w = 100.0f;
    h = 100.0f;
    break;
  case APP_PRIMITIVE_CIRCLE:
    w = 100.0f;
    h = 100.0f;
    is_ellipse = true;
    break;
  case APP_PRIMITIVE_ELLIPSE:
    is_ellipse = true;
    break;
  case APP_PRIMITIVE_RECTANGLE:
  default:
    break;
  }
  if (out_w)
    *out_w = w;
  if (out_h)
    *out_h = h;
  if (out_is_ellipse)
    *out_is_ellipse = is_ellipse;
}

static StygianEditorNodeId app_add_primitive_with_bounds(EditorBootstrapApp *app,
                                                         AppPrimitiveKind primitive,
                                                         float world_x, float world_y,
                                                         float shape_w,
                                                         float shape_h) {
  StygianEditorNodeId id = STYGIAN_EDITOR_INVALID_ID;
  bool is_ellipse = false;
  if (!app || !app->editor)
    return STYGIAN_EDITOR_INVALID_ID;
  app_default_primitive_size(primitive, NULL, NULL, &is_ellipse);
  if (shape_w < 1.0f || shape_h < 1.0f) {
    app_default_primitive_size(primitive, &shape_w, &shape_h, &is_ellipse);
    world_x -= shape_w * 0.5f;
    world_y -= shape_h * 0.5f;
  }
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
    bool uniform_default = false;
    if (primitive == APP_PRIMITIVE_CIRCLE || primitive == APP_PRIMITIVE_ELLIPSE)
      uniform_default = true;
    app_set_node_uniform_scale(app, id, uniform_default);
    (void)stygian_editor_select_node(app->editor, id, false);
  }
  return id;
}

#if 0
static void app_add_shape_at_cursor(EditorBootstrapApp *app, bool ellipse,
                                    int mouse_x, int mouse_y) {
  app_add_primitive_at_cursor(app,
                              ellipse ? APP_PRIMITIVE_ELLIPSE
                                      : APP_PRIMITIVE_RECTANGLE,
                              mouse_x, mouse_y);
}
#endif

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
  if (app->path_tool_active)
    app->current_tool = APP_TOOL_PATH;
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
  app->current_tool = APP_TOOL_SELECT;
  app->path_preview_count = 0u;
}

static void app_cancel_path_tool(EditorBootstrapApp *app) {
  if (!app || !app->editor || !app->path_tool_active)
    return;
  stygian_editor_path_cancel(app->editor, app->active_path);
  app->path_tool_active = false;
  app->active_path = STYGIAN_EDITOR_INVALID_ID;
  app->current_tool = APP_TOOL_SELECT;
  app->path_preview_count = 0u;
  app_set_status(app, "Path tool cancelled.");
}

#if 0
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
  app->behavior_action_choice = 0;
  snprintf(app->behavior_variable_name, sizeof(app->behavior_variable_name),
           "motion.x");
  snprintf(app->behavior_variable_number, sizeof(app->behavior_variable_number),
           "60");
  snprintf(app->behavior_variable_mode, sizeof(app->behavior_variable_mode), "0");
  app->behavior_variable_use_active_mode = true;
  snprintf(app->behavior_navigate_target, sizeof(app->behavior_navigate_target),
           "screen://details");
  app_set_status(app,
                 "Bootstrap ready. Demo slider press animates the blue box.");
}
#endif

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
    if (event->key.key == STYGIAN_KEY_V) {
      app_set_current_tool(app, APP_TOOL_SELECT);
      app_set_status(app, "Tool: Select");
      return true;
    }
    if (event->key.key == STYGIAN_KEY_R) {
      app->selected_primitive = APP_PRIMITIVE_RECTANGLE;
      app_set_current_tool(app, APP_TOOL_RECT_FRAME);
      app_set_status(app, "Tool: Rect/Frame");
      return true;
    }
    if (event->key.key == STYGIAN_KEY_O) {
      app->selected_primitive = APP_PRIMITIVE_ELLIPSE;
      app_set_current_tool(app, APP_TOOL_ELLIPSE);
      app_set_status(app, "Tool: Ellipse");
      return true;
    }
    if (event->key.key == STYGIAN_KEY_P) {
      app_set_current_tool(app, APP_TOOL_PATH);
      return true;
    }
    if (event->key.key == STYGIAN_KEY_H) {
      app_set_current_tool(app, APP_TOOL_PAN);
      app_set_status(app, "Tool: Pan");
      return true;
    }
    if (event->key.key == STYGIAN_KEY_Z) {
      app_set_current_tool(app, APP_TOOL_ZOOM);
      app_set_status(app, "Tool: Zoom");
      return true;
    }
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
      if (app->file_menu_open || app->primitive_menu_open || app->context_menu_open) {
        app->file_menu_open = false;
        app->primitive_menu_open = false;
        app->context_menu_open = false;
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
      if (app->current_tool != APP_TOOL_SELECT) {
        app_set_current_tool(app, APP_TOOL_SELECT);
        app_set_status(app, "Tool: Select");
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
      (app->file_menu_open || app->primitive_menu_open || app->context_menu_open)) {
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
    if (app->context_menu_open && app_point_in_context_menu(app, mx, my))
      clicked_menu_surface = true;

    if (!clicked_menu_surface) {
      app->file_menu_open = false;
      app->primitive_menu_open = false;
      app->context_menu_open = false;
      mutated = true;
    }
    if (clicked_menu_surface)
      return true;
  }

  if (event->type == STYGIAN_EVENT_MOUSE_DOWN &&
      (event->mouse_button.button == STYGIAN_MOUSE_MIDDLE ||
       (event->mouse_button.button == STYGIAN_MOUSE_LEFT &&
        app->current_tool == APP_TOOL_PAN)) &&
      app_point_in_rect(mx, my, app->layout.viewport_x, app->layout.viewport_y,
                        app->layout.viewport_w, app->layout.viewport_h)) {
    app->panning = true;
    return false;
  }
  if (event->type == STYGIAN_EVENT_MOUSE_UP &&
      (event->mouse_button.button == STYGIAN_MOUSE_MIDDLE ||
       (event->mouse_button.button == STYGIAN_MOUSE_LEFT &&
        app->current_tool == APP_TOOL_PAN))) {
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

  if (event->type == STYGIAN_EVENT_MOUSE_MOVE && app->placing_primitive) {
    app->place_curr_view_x = mx;
    app->place_curr_view_y = my;
    return true;
  }

  if (event->type == STYGIAN_EVENT_MOUSE_MOVE && app->resizing_node &&
      app->resize_selection_count > 0u) {
    return app_apply_resize_drag(app, mx, my);
  }

  if (event->type == STYGIAN_EVENT_MOUSE_MOVE && app->rotating_selection &&
      app->rotate_selection_count > 0u) {
    return app_apply_rotate_drag(app, mx, my);
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
    bool was_rotating = app->rotating_selection;
    bool was_marquee = app->marquee_selecting;
    bool was_placing = app->placing_primitive;
    bool marquee_mutated = false;
    bool placement_mutated = false;
    if (app->placing_primitive) {
      float x0w = 0.0f;
      float y0w = 0.0f;
      float x1w = 0.0f;
      float y1w = 0.0f;
      float min_x;
      float min_y;
      float w;
      float h;
      StygianEditorNodeId new_id;
      stygian_editor_view_to_world(app->editor, app->place_start_view_x,
                                   app->place_start_view_y, &x0w, &y0w);
      stygian_editor_view_to_world(app->editor, mx, my, &x1w, &y1w);
      min_x = x0w < x1w ? x0w : x1w;
      min_y = y0w < y1w ? y0w : y1w;
      w = fabsf(x1w - x0w);
      h = fabsf(y1w - y0w);
      new_id = app_add_primitive_with_bounds(app, app->selected_primitive, min_x, min_y,
                                             w, h);
      if (new_id != STYGIAN_EDITOR_INVALID_ID) {
        app_set_status(app, "Primitive placed.");
        placement_mutated = true;
      } else {
        app_set_status(app, "Primitive add failed (capacity or invalid state).");
      }
      app->placing_primitive = false;
      app_set_current_tool(app, APP_TOOL_SELECT);
    }
    if (app->marquee_selecting)
      marquee_mutated = app_commit_marquee_selection(app);
    app->dragging_node = false;
    app->drag_node_id = STYGIAN_EDITOR_INVALID_ID;
    app->drag_selection_count = 0u;
    app->resizing_node = false;
    app->resize_node_id = STYGIAN_EDITOR_INVALID_ID;
    app->resize_handle = APP_RESIZE_NONE;
    app->resize_selection_count = 0u;
    app->rotating_selection = false;
    app->rotate_selection_count = 0u;
    app->marquee_selecting = false;
    app->marquee_additive = false;
    app->marquee_start_view_x = 0.0f;
    app->marquee_start_view_y = 0.0f;
    app->marquee_curr_view_x = 0.0f;
    app->marquee_curr_view_y = 0.0f;
    return was_resizing || was_dragging || was_rotating || was_marquee ||
           was_placing || marquee_mutated || placement_mutated;
  }

  if (!app_point_in_rect(mx, my, app->layout.viewport_x, app->layout.viewport_y,
                         app->layout.viewport_w, app->layout.viewport_h)) {
    return false;
  }

  if (event->type == STYGIAN_EVENT_MOUSE_MOVE && app->path_tool_active)
    return true;

  if (event->type == STYGIAN_EVENT_MOUSE_DOWN &&
      event->mouse_button.button == STYGIAN_MOUSE_LEFT &&
      app->current_tool == APP_TOOL_ZOOM) {
    app_camera_zoom_about(app, mx, my, app->camera_zoom * 1.15f);
    return true;
  }

  if (event->type == STYGIAN_EVENT_MOUSE_DOWN &&
      event->mouse_button.button == STYGIAN_MOUSE_LEFT) {
    float world_x;
    float world_y;
    stygian_editor_view_to_world(app->editor, mx, my, &world_x, &world_y);

    if (app->current_tool == APP_TOOL_RECT_FRAME ||
        app->current_tool == APP_TOOL_ELLIPSE) {
      app->placing_primitive = true;
      app->place_start_view_x = mx;
      app->place_start_view_y = my;
      app->place_curr_view_x = mx;
      app->place_curr_view_y = my;
      app->context_menu_open = false;
      app_set_status(app, "Drag to size. Click release keeps the default size.");
      return true;
    }

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
      if (app_rotation_handle_hit(app, mx, my)) {
        if (app_capture_rotate_selection(app, world_x, world_y)) {
          app->rotating_selection = true;
          app->dragging_node = false;
          app->resizing_node = false;
          app->marquee_selecting = false;
          app->context_menu_open = false;
          app_set_status(app, "Rotate selection.");
          return true;
        }
      }

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

  if (event->type == STYGIAN_EVENT_MOUSE_DOWN &&
      event->mouse_button.button == STYGIAN_MOUSE_RIGHT) {
    float world_x = 0.0f;
    float world_y = 0.0f;
    StygianEditorNodeId hit_node;
    stygian_editor_view_to_world(app->editor, mx, my, &world_x, &world_y);
    hit_node = stygian_editor_hit_test_at(app->editor, world_x, world_y);
    if (hit_node != STYGIAN_EDITOR_INVALID_ID &&
        !stygian_editor_node_is_selected(app->editor, hit_node)) {
      (void)stygian_editor_select_node(app->editor, hit_node, false);
    }
    app->context_menu_open = true;
    app->context_menu_x = mx;
    app->context_menu_y = my;
    app->context_menu_world_x = world_x;
    app->context_menu_world_y = world_y;
    app->primitive_menu_open = false;
    app->file_menu_open = false;
    app_set_status(app, hit_node != STYGIAN_EDITOR_INVALID_ID
                            ? "Context menu: selection actions."
                            : "Context menu: canvas actions.");
    return true;
  }

  return false;
}

static bool app_handle_timeline_event(EditorBootstrapApp *app,
                                      const StygianEvent *event) {
  float mx;
  float my;
  uint32_t max_time_ms;
  uint32_t start_ms;
  uint32_t span_ms;
  if (!app || !event || !app->editor)
    return false;
  if (event->type == STYGIAN_EVENT_MOUSE_DOWN || event->type == STYGIAN_EVENT_MOUSE_UP) {
    mx = (float)event->mouse_button.x;
    my = (float)event->mouse_button.y;
  } else if (event->type == STYGIAN_EVENT_MOUSE_MOVE) {
    mx = (float)event->mouse_move.x;
    my = (float)event->mouse_move.y;
  } else {
    return false;
  }
  if (app->layout.timeline_w <= 0.0f || app->layout.timeline_h <= 0.0f)
    return false;
  max_time_ms = app->layout.timeline_max_time_ms > 0u ? app->layout.timeline_max_time_ms
                                                      : 1000u;
  start_ms = app_timeline_visible_start_ms(app, max_time_ms);
  span_ms = app_timeline_visible_span_ms(app, max_time_ms);

  if (event->type == STYGIAN_EVENT_MOUSE_DOWN &&
      event->mouse_button.button == STYGIAN_MOUSE_LEFT) {
    if (app_point_in_rect(mx, my, app->layout.timeline_lane_x,
                          app->layout.timeline_y + 8.0f, app->layout.timeline_lane_w,
                          app->layout.timeline_lane_h)) {
      if (app_timeline_select_key_from_lane(app, mx, my)) {
        app->timeline_dragging_key = true;
        app->timeline_scrubbing = false;
        return true;
      }
      app->timeline_scrubbing = true;
      app_timeline_set_cursor(
          app, app_timeline_view_x_to_time(mx, start_ms, span_ms, app->layout.timeline_lane_x,
                                           app->layout.timeline_lane_w));
      return true;
    }
    if (app->layout.timeline_curve_w > 0.0f &&
        app_point_in_rect(mx, my, app->layout.timeline_curve_x, app->layout.timeline_curve_y,
                          app->layout.timeline_curve_w, app->layout.timeline_curve_h)) {
      app->timeline_dragging_curve = true;
      return true;
    }
  }

  if (event->type == STYGIAN_EVENT_MOUSE_MOVE) {
    if (app->timeline_scrubbing) {
      app_timeline_set_cursor(
          app, app_timeline_view_x_to_time(mx, start_ms, span_ms, app->layout.timeline_lane_x,
                                           app->layout.timeline_lane_w));
      return true;
    }
    if (app->timeline_dragging_key) {
      uint32_t new_time = app_timeline_view_x_to_time(mx, start_ms, span_ms,
                                                      app->layout.timeline_lane_x,
                                                      app->layout.timeline_lane_w);
      return app_timeline_update_selected_key(app, new_time, NULL, false);
    }
    if (app->timeline_dragging_curve) {
      StygianEditorTimelineTrack track = {0};
      float min_v;
      float max_v;
      float norm_y;
      float new_value;
      uint32_t new_time;
      if (!stygian_editor_timeline_get_track(app->editor, app->timeline_selected_track_index,
                                             &track))
        return false;
      if (app->timeline_selected_keyframe_index >= track.keyframe_count)
        return false;
      min_v = track.keyframes[0].value;
      max_v = min_v;
      for (uint32_t k = 1u; k < track.keyframe_count; ++k) {
        if (track.keyframes[k].value < min_v)
          min_v = track.keyframes[k].value;
        if (track.keyframes[k].value > max_v)
          max_v = track.keyframes[k].value;
      }
      if (fabsf(max_v - min_v) < 0.0001f)
        max_v = min_v + 1.0f;
      norm_y = 1.0f - ((my - app->layout.timeline_curve_y) / app->layout.timeline_curve_h);
      norm_y = app_clampf(norm_y, 0.0f, 1.0f);
      new_value = min_v + (max_v - min_v) * norm_y;
      new_time = app_timeline_view_x_to_time(mx, start_ms, span_ms, app->layout.timeline_curve_x,
                                             app->layout.timeline_curve_w);
      return app_timeline_update_selected_key(app, new_time, &new_value, true);
    }
  }

  if (event->type == STYGIAN_EVENT_MOUSE_UP &&
      event->mouse_button.button == STYGIAN_MOUSE_LEFT) {
    bool active = app->timeline_scrubbing || app->timeline_dragging_key ||
                  app->timeline_dragging_curve;
    app->timeline_scrubbing = false;
    app->timeline_dragging_key = false;
    app->timeline_dragging_curve = false;
    return active;
  }
  return false;
}

static bool app_handle_command_shortcuts(EditorBootstrapApp *app,
                                         const StygianEvent *event) {
  uint32_t mods = 0u;
  float nudge = 1.0f;
  if (!app || !event || event->type != STYGIAN_EVENT_KEY_DOWN)
    return false;
  if (event->key.repeat)
    return false;
  mods = event->key.mods;
  if (event->key.key == STYGIAN_KEY_F1) {
    app->shortcut_help_open = !app->shortcut_help_open;
    app_set_status(app, app->shortcut_help_open ? "Shortcut help shown."
                                                : "Shortcut help hidden.");
    return true;
  }
  if ((mods & STYGIAN_MOD_CTRL) == 0u) {
    if (event->key.key == STYGIAN_KEY_DELETE ||
        event->key.key == STYGIAN_KEY_BACKSPACE) {
      if (app_delete_selection(app))
        app_set_status(app, "Selection deleted.");
      else
        app_set_status(app, "Nothing to delete.");
      return true;
    }
    if (mods & STYGIAN_MOD_SHIFT)
      nudge = 10.0f;
    if (event->key.key == STYGIAN_KEY_LEFT)
      return app_nudge_selection(app, -nudge, 0.0f);
    if (event->key.key == STYGIAN_KEY_RIGHT)
      return app_nudge_selection(app, nudge, 0.0f);
    if (event->key.key == STYGIAN_KEY_UP)
      return app_nudge_selection(app, 0.0f, -nudge);
    if (event->key.key == STYGIAN_KEY_DOWN)
      return app_nudge_selection(app, 0.0f, nudge);
    return false;
  }
  if (event->key.key == STYGIAN_KEY_Z) {
    bool ok = false;
    if (mods & STYGIAN_MOD_SHIFT)
      ok = app_redo(app);
    else
      ok = app_undo(app);
    app_set_status(app, ok ? ((mods & STYGIAN_MOD_SHIFT) ? "Redo applied."
                                                          : "Undo applied.")
                           : ((mods & STYGIAN_MOD_SHIFT) ? "Nothing to redo."
                                                          : "Nothing to undo."));
    return true;
  }
  if (event->key.key == STYGIAN_KEY_Y) {
    if (app_redo(app))
      app_set_status(app, "Redo applied.");
    else
      app_set_status(app, "Nothing to redo.");
    return true;
  }
  if ((mods & STYGIAN_MOD_CTRL) == 0u)
    return false;
  if (event->key.key == STYGIAN_KEY_A) {
    if (app_select_all_nodes(app))
      app_set_status(app, "Selected all nodes.");
    else
      app_set_status(app, "Nothing to select.");
    return true;
  }
  if (event->key.key == STYGIAN_KEY_C) {
    if (app_copy_selection(app))
      app_set_status(app, "Selection copied.");
    else
      app_set_status(app, "Nothing to copy.");
    return true;
  }
  if (event->key.key == STYGIAN_KEY_X) {
    if (app_cut_selection(app))
      app_set_status(app, "Selection cut.");
    else
      app_set_status(app, "Nothing to cut.");
    return true;
  }
  if (event->key.key == STYGIAN_KEY_V) {
    float wx = 0.0f;
    float wy = 0.0f;
    float view_x = (float)app->last_mouse_x;
    float view_y = (float)app->last_mouse_y;
    if (!app_point_in_rect(view_x, view_y, app->layout.viewport_x, app->layout.viewport_y,
                           app->layout.viewport_w, app->layout.viewport_h)) {
      view_x = app->layout.viewport_x + app->layout.viewport_w * 0.5f;
      view_y = app->layout.viewport_y + app->layout.viewport_h * 0.5f;
    }
    stygian_editor_view_to_world(app->editor, view_x, view_y, &wx, &wy);
    if (app_paste_clipboard(app, wx, wy, true))
      app_set_status(app, "Clipboard pasted.");
    else
      app_set_status(app, "Clipboard is empty.");
    return true;
  }
  if (event->key.key == STYGIAN_KEY_D) {
    if (app_duplicate_selection(app, 24.0f, 24.0f))
      app_set_status(app, "Selection duplicated.");
    else
      app_set_status(app, "Nothing to duplicate.");
    return true;
  }
  if (event->key.key == STYGIAN_KEY_N) {
    app_reset_canvas(app);
    app_set_status(app, "Canvas reset.");
    return true;
  }
  if (event->key.key == STYGIAN_KEY_S) {
    if (app_save_project(app))
      app_set_status(app, "Saved project.");
    else
      app_set_status(app, "Project save failed.");
    return true;
  }
  if (event->key.key == STYGIAN_KEY_O) {
    if (app_load_project(app))
      app_set_status(app, "Loaded project.");
    else
      app_set_status(app, "Project load failed.");
    return true;
  }
  if (event->key.key == STYGIAN_KEY_E) {
    if (app_export_runtime_bundle(
            app, "editor/build/windows/stygian_export",
            "editor/build/windows/stygian_export/stygian_export_report.txt")) {
      app_set_status(app, "Exported runtime bundle.");
    } else {
      app_set_status(app, "Runtime bundle export failed.");
    }
    return true;
  }
  if (event->key.key == STYGIAN_KEY_K) {
    app->command_palette_open = !app->command_palette_open;
    return true;
  }
  if (event->key.key == STYGIAN_KEY_1) {
    if (app_seed_self_host_surface_stage(app, APP_SELF_HOST_STAGE_1))
      app_set_status(app, "Seeded self-host stage 1 surfaces.");
    else
      app_set_status(app, "Self-host stage 1 seed failed.");
    return true;
  }
  if (event->key.key == STYGIAN_KEY_2) {
    if (app_seed_self_host_surface_stage(app, APP_SELF_HOST_STAGE_2))
      app_set_status(app, "Seeded self-host stage 2 surfaces.");
    else
      app_set_status(app, "Self-host stage 2 seed failed.");
    return true;
  }
  if (event->key.key == STYGIAN_KEY_3) {
    if (app_seed_self_host_surface_stage(app, APP_SELF_HOST_STAGE_3))
      app_set_status(app, "Seeded self-host stage 3 surfaces.");
    else
      app_set_status(app, "Self-host stage 3 seed failed.");
    return true;
  }
  return false;
}

#if 0
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

  if (stygian_button(app->ctx, app->font,
                     "Export Runtime Bundle -> editor/build/windows/stygian_export",
                     x, y, w, 28.0f)) {
    if (app_export_runtime_bundle(
            app, "editor/build/windows/stygian_export",
            "editor/build/windows/stygian_export/stygian_export_report.txt")) {
      app_set_status(app, "Exported runtime bundle + report.");
    } else {
      app_set_status(app, "Runtime bundle export failed.");
    }
  }
  y += 34.0f;
  if (stygian_button(app->ctx, app->font, "Run Binding Doctor", x, y, w, 24.0f)) {
    if (app_run_binding_doctor(
            app, "editor/build/windows/stygian_export/stygian_binding_doctor.txt",
            "editor/build/windows/stygian_export/stygian_editor_hooks.c")) {
      app_set_status(app, "Binding doctor report written.");
    } else {
      app_set_status(app, "Binding doctor failed.");
    }
  }
  y += 32.0f;

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
#endif

#if 0
static bool app_draw_right_panel(EditorBootstrapApp *app) {
  float x = app->layout.right_x + 12.0f;
  float y = app->layout.right_y + 10.0f;
  float w = app->layout.right_w - 24.0f;
  bool mutated = false;
  StygianEditorNodeId selected = stygian_editor_selected_node(app->editor);
  char hooks_source[32768];
  const char *hooks_source_path =
      "editor/build/windows/stygian_export/stygian_editor_hooks.c";
  bool has_hook_set_variable = false;
  bool has_hook_navigate = false;
  bool selected_needs_hook_set_variable = false;
  bool selected_needs_hook_navigate = false;

  hooks_source[0] = '\0';
  (void)app_read_text_file(hooks_source_path, hooks_source, sizeof(hooks_source));
  has_hook_set_variable = app_hook_function_exists(
      hooks_source, "stygian_editor_generated_hook_set_variable");
  has_hook_navigate =
      app_hook_function_exists(hooks_source, "stygian_editor_generated_hook_navigate");

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
    char binding_text[96];
    uint32_t binding_connected = 0u;
    uint32_t binding_missing = 0u;
    bool have_bounds =
        stygian_editor_node_get_bounds(app->editor, selected, &nx, &ny, &nw, &nh);

    if (app->complexity_mode == APP_COMPLEXITY_BEGINNER)
      snprintf(id_text, sizeof(id_text), "Selected Item");
    else
      snprintf(id_text, sizeof(id_text), "Node ID %u", selected);
    if (app->font) {
      stygian_text(app->ctx, app->font, id_text, x, y, 14.0f, 0.84f, 0.88f,
                   0.97f, 1.0f);
    }
    y += 20.0f;
    app_node_binding_state(app, selected, &binding_connected, &binding_missing);
    snprintf(binding_text, sizeof(binding_text), "Bindings %u | Missing %u",
             binding_connected, binding_missing);
    if (app->font) {
      stygian_text(app->ctx, app->font, binding_text, x, y, 12.0f, 0.80f, 0.86f,
                   0.94f, 1.0f);
    }
    y += 18.0f;

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

    if (app->font) {
      const char *supported_events =
          app->complexity_mode == APP_COMPLEXITY_BEGINNER
              ? "Events: Press, Release, Hover, Value Changed"
              : "Supported events: press/release/hover_enter/hover_leave/value_changed";
      uint32_t behavior_count = stygian_editor_behavior_count(app->editor);
      uint32_t shown = 0u;
      stygian_text(app->ctx, app->font, "Logic", x, y, 15.0f, 0.93f, 0.95f, 1.0f,
                   1.0f);
      y += 18.0f;
      stygian_text(app->ctx, app->font, supported_events, x, y, 11.0f, 0.76f, 0.84f,
                   0.94f, 1.0f);
      y += 16.0f;
      for (uint32_t i = 0u; i < behavior_count && shown < 5u; ++i) {
        StygianEditorBehaviorRule rule;
        StygianEditorBehaviorId behavior_id = 0u;
        StygianEditorNodeId target = STYGIAN_EDITOR_INVALID_ID;
        bool missing_node = false;
        bool missing_hook = false;
        char line[180];
        if (!stygian_editor_get_behavior_rule(app->editor, i, &behavior_id, &rule))
          continue;
        if (rule.action_kind == STYGIAN_EDITOR_ACTION_ANIMATE)
          target = rule.animate.target_node;
        else if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY)
          target = rule.set_property.target_node;
        else if (rule.action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY)
          target = rule.toggle_visibility.target_node;
        else
          target = rule.trigger_node;
        if (rule.trigger_node != selected && target != selected)
          continue;
        if (!app_node_exists(app, rule.trigger_node) || !app_node_exists(app, target))
          missing_node = true;
        if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_VARIABLE &&
            !has_hook_set_variable)
          missing_hook = true;
        if (rule.action_kind == STYGIAN_EDITOR_ACTION_NAVIGATE && !has_hook_navigate)
          missing_hook = true;
        if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_VARIABLE)
          selected_needs_hook_set_variable = true;
        if (rule.action_kind == STYGIAN_EDITOR_ACTION_NAVIGATE)
          selected_needs_hook_navigate = true;
        if (app->complexity_mode == APP_COMPLEXITY_BEGINNER) {
          snprintf(line, sizeof(line), "%s -> %s [%s]",
                   app_event_kind_name(rule.trigger_event),
                   app_action_kind_name(rule.action_kind),
                   (missing_node || missing_hook) ? "needs fix" : "connected");
        } else {
          snprintf(line, sizeof(line), "#%u %s -> %s [%s%s%s]", behavior_id,
                   app_event_kind_name(rule.trigger_event),
                   app_action_kind_name(rule.action_kind),
                   missing_node ? "missing-node" : "connected",
                   missing_hook ? ",missing-hook" : "",
                   (!missing_node && !missing_hook) ? ",owned:binding" : "");
        }
        stygian_text(app->ctx, app->font, line, x, y, 11.0f, 0.82f, 0.87f, 0.95f,
                     1.0f);
        y += 14.0f;
        shown += 1u;
      }
      if (shown == 0u) {
        stygian_text(app->ctx, app->font, "No logic bound to selection.", x, y,
                     11.0f, 0.74f, 0.81f, 0.91f, 1.0f);
        y += 14.0f;
      }
      y += 8.0f;
    }

    if (app->font) {
      char hook_line[192];
      stygian_text(app->ctx, app->font, "Hook Discovery", x, y, 15.0f, 0.93f,
                   0.95f, 1.0f, 1.0f);
      y += 18.0f;
      if (!selected_needs_hook_set_variable && !selected_needs_hook_navigate) {
        stygian_text(app->ctx, app->font,
                     "No external hooks required by this selection.", x, y,
                     11.0f, 0.74f, 0.81f, 0.91f, 1.0f);
        y += 14.0f;
      } else {
        if (selected_needs_hook_set_variable) {
          snprintf(hook_line, sizeof(hook_line),
                   "stygian_editor_generated_hook_set_variable [%s]",
                   has_hook_set_variable ? "present" : "missing");
          stygian_text(app->ctx, app->font, hook_line, x, y, 11.0f, 0.80f,
                       0.86f, 0.95f, 1.0f);
          y += 14.0f;
        }
        if (selected_needs_hook_navigate) {
          snprintf(hook_line, sizeof(hook_line),
                   "stygian_editor_generated_hook_navigate [%s]",
                   has_hook_navigate ? "present" : "missing");
          stygian_text(app->ctx, app->font, hook_line, x, y, 11.0f, 0.80f,
                       0.86f, 0.95f, 1.0f);
          y += 14.0f;
        }
      }
      stygian_text(app->ctx, app->font, hooks_source_path, x, y, 10.0f, 0.72f,
                   0.80f, 0.90f, 1.0f);
      y += 16.0f;
    }

    if (stygian_button(app->ctx, app->font, "Open Hook Source", x, y, w * 0.48f,
                       24.0f)) {
      if (app_open_text_file_in_default_editor(hooks_source_path))
        app_set_status(app, "Opened hook source file in default editor.");
      else
        app_set_status(app, "Failed to open hook source file.");
    }
    if (stygian_button(app->ctx, app->font, "Create Missing Stubs",
                       x + w * 0.52f, y, w * 0.48f, 24.0f)) {
      bool wrote = false;
      bool ok = true;
      if (selected_needs_hook_set_variable && !has_hook_set_variable) {
        ok = app_append_hook_stub_if_missing(
                 hooks_source_path, "stygian_editor_generated_hook_set_variable",
                 "void stygian_editor_generated_hook_set_variable(\n"
                 "    const char *name, uint32_t kind, float number_value,\n"
                 "    uint32_t mode_index, StygianColor color_value) {\n"
                 "  (void)name;\n"
                 "  (void)kind;\n"
                 "  (void)number_value;\n"
                 "  (void)mode_index;\n"
                 "  (void)color_value;\n"
                 "}\n\n") &&
             ok;
        wrote = true;
      }
      if (selected_needs_hook_navigate && !has_hook_navigate) {
        ok = app_append_hook_stub_if_missing(
                 hooks_source_path, "stygian_editor_generated_hook_navigate",
                 "void stygian_editor_generated_hook_navigate(const char *target) {\n"
                 "  (void)target;\n"
                 "}\n\n") &&
             ok;
        wrote = true;
      }
      if (ok) {
        hooks_source[0] = '\0';
        (void)app_read_text_file(hooks_source_path, hooks_source,
                                 sizeof(hooks_source));
        has_hook_set_variable = app_hook_function_exists(
            hooks_source, "stygian_editor_generated_hook_set_variable");
        has_hook_navigate = app_hook_function_exists(
            hooks_source, "stygian_editor_generated_hook_navigate");
        if (wrote)
          app_set_status(app, "Created missing hook stubs for selection.");
        else
          app_set_status(app, "No missing hooks for current selection.");
        mutated = true;
      } else {
        app_set_status(app, "Failed to create one or more hook stubs.");
      }
    }
    y += 30.0f;
  }

  if (app->font) {
    stygian_text(app->ctx, app->font, "Function Editor", x, y, 17.0f, 0.93f,
                 0.95f, 1.0f, 1.0f);
  }
  y += 22.0f;

  if (app->code_insight_enabled) {
    if (selected != STYGIAN_EDITOR_INVALID_ID && app->code_insight_node != selected) {
      app_refresh_code_insight(app, selected);
    }
    if (app->font) {
      stygian_text(app->ctx, app->font, app->code_insight_text, x, y, 11.0f, 0.78f,
                   0.86f, 0.95f, 1.0f);
    }
    y += 18.0f;
    if (stygian_button(app->ctx, app->font, "Refresh Code Insight", x, y, w,
                       24.0f)) {
      app_refresh_code_insight(app, selected);
      mutated = true;
    }
    y += 30.0f;
  }

  if (app->complexity_mode == APP_COMPLEXITY_BEGINNER) {
    if (app->font) {
      stygian_text(
          app->ctx, app->font,
          "Beginner mode hides raw runtime wiring. Use templates and tabs scaffold.",
          x, y, 11.0f, 0.75f, 0.82f, 0.92f, 1.0f);
      y += 14.0f;
      stygian_text(app->ctx, app->font,
                   "Switch to Systems mode for explicit IDs and contract-level controls.",
                   x, y, 11.0f, 0.75f, 0.82f, 0.92f, 1.0f);
    }
    return mutated;
  }

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
    stygian_text(app->ctx, app->font, "Action", x, y, 13.0f, 0.85f, 0.88f, 0.95f,
                 1.0f);
  y += 16.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Animate", x, y,
                             &app->behavior_action_choice, 0);
  y += 20.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Set Property", x, y,
                             &app->behavior_action_choice, 1);
  y += 20.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Toggle Visibility", x, y,
                             &app->behavior_action_choice, 2);
  y += 20.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Set Variable", x, y,
                             &app->behavior_action_choice, 3);
  y += 20.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Navigate", x, y,
                             &app->behavior_action_choice, 4);
  y += 24.0f;

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

  if (app->behavior_action_choice == 0 || app->behavior_action_choice == 1) {
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

    if (app->behavior_action_choice == 0) {
      if (app->font)
        stygian_text(app->ctx, app->font, "From", x, y, 13.0f, 0.85f, 0.88f,
                     0.95f, 1.0f);
      y += 14.0f;
      stygian_text_input(app->ctx, app->font, x, y, w, 22.0f,
                         app->behavior_from_value,
                         (int)sizeof(app->behavior_from_value));
      y += 28.0f;
    }

    if (app->font)
      stygian_text(app->ctx, app->font,
                   app->behavior_action_choice == 0 ? "To" : "Value", x, y,
                   13.0f, 0.85f, 0.88f, 0.95f, 1.0f);
    y += 14.0f;
    stygian_text_input(app->ctx, app->font, x, y, w, 22.0f, app->behavior_to_value,
                       (int)sizeof(app->behavior_to_value));
    y += 28.0f;

    if (app->behavior_action_choice == 0) {
      if (app->font)
        stygian_text(app->ctx, app->font, "Duration ms", x, y, 13.0f, 0.85f,
                     0.88f, 0.95f, 1.0f);
      y += 14.0f;
      stygian_text_input(app->ctx, app->font, x, y, w, 22.0f,
                         app->behavior_duration_ms,
                         (int)sizeof(app->behavior_duration_ms));
      y += 30.0f;

      if (app->font)
        stygian_text(app->ctx, app->font, "Easing", x, y, 13.0f, 0.85f, 0.88f,
                     0.95f, 1.0f);
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
    }
  } else if (app->behavior_action_choice == 3) {
    if (app->font)
      stygian_text(app->ctx, app->font, "Variable Name", x, y, 13.0f, 0.85f,
                   0.88f, 0.95f, 1.0f);
    y += 14.0f;
    stygian_text_input(app->ctx, app->font, x, y, w, 22.0f,
                       app->behavior_variable_name,
                       (int)sizeof(app->behavior_variable_name));
    y += 28.0f;
    if (app->font)
      stygian_text(app->ctx, app->font, "Number Value", x, y, 13.0f, 0.85f,
                   0.88f, 0.95f, 1.0f);
    y += 14.0f;
    stygian_text_input(app->ctx, app->font, x, y, w, 22.0f,
                       app->behavior_variable_number,
                       (int)sizeof(app->behavior_variable_number));
    y += 26.0f;
    (void)stygian_checkbox(app->ctx, app->font, "Use Active Mode", x, y,
                           &app->behavior_variable_use_active_mode);
    y += 22.0f;
    if (!app->behavior_variable_use_active_mode) {
      if (app->font)
        stygian_text(app->ctx, app->font, "Mode Index", x, y, 13.0f, 0.85f,
                     0.88f, 0.95f, 1.0f);
      y += 14.0f;
      stygian_text_input(app->ctx, app->font, x, y, w, 22.0f,
                         app->behavior_variable_mode,
                         (int)sizeof(app->behavior_variable_mode));
      y += 28.0f;
    }
  } else if (app->behavior_action_choice == 4) {
    if (app->font)
      stygian_text(app->ctx, app->font, "Navigate Target", x, y, 13.0f, 0.85f,
                   0.88f, 0.95f, 1.0f);
    y += 14.0f;
    stygian_text_input(app->ctx, app->font, x, y, w, 22.0f,
                       app->behavior_navigate_target,
                       (int)sizeof(app->behavior_navigate_target));
    y += 28.0f;
  }

  if (stygian_button(app->ctx, app->font, "Add Function Rule", x, y, w, 26.0f)) {
    StygianEditorBehaviorRule rule;
    uint32_t trigger_id = 0u;
    uint32_t target_id = 0u;
    uint32_t mode_index = 0u;
    uint32_t duration_ms = 0u;
    float from_value = NAN;
    float to_value = 0.0f;
    float number_value = 0.0f;
    StygianEditorBehaviorActionKind action_kind;
    bool ok = true;

    memset(&rule, 0, sizeof(rule));
    action_kind = app_action_kind_from_choice(app->behavior_action_choice);
    if (!app_parse_u32(app->behavior_trigger_id, &trigger_id)) {
      app_set_status(app, "Function rule error: trigger node id invalid.");
      ok = false;
    }
    if (action_kind == STYGIAN_EDITOR_ACTION_ANIMATE ||
        action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY ||
        action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY) {
      if (!app_parse_u32(app->behavior_target_id, &target_id)) {
        app_set_status(app, "Function rule error: target node id invalid.");
        ok = false;
      }
    }
    if (action_kind == STYGIAN_EDITOR_ACTION_ANIMATE) {
      if (!app_parse_float_text(app->behavior_from_value, &from_value)) {
        app_set_status(
            app, "Function rule error: from value must be number or nan.");
        ok = false;
      }
      if (!app_parse_float_text(app->behavior_to_value, &to_value)) {
        app_set_status(app, "Function rule error: to value must be a number.");
        ok = false;
      }
      if (!app_parse_u32_or_default(app->behavior_duration_ms, 350u,
                                    &duration_ms) ||
          duration_ms == 0u) {
        app_set_status(app, "Function rule error: duration must be > 0.");
        ok = false;
      }
    } else if (action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY) {
      if (!app_parse_float_text(app->behavior_to_value, &to_value)) {
        app_set_status(
            app, "Function rule error: property value must be a number.");
        ok = false;
      }
    } else if (action_kind == STYGIAN_EDITOR_ACTION_SET_VARIABLE) {
      if (!app->behavior_variable_name[0]) {
        app_set_status(app, "Function rule error: variable name is required.");
        ok = false;
      }
      if (!app_parse_float_text(app->behavior_variable_number, &number_value)) {
        app_set_status(app,
                       "Function rule error: variable number value invalid.");
        ok = false;
      }
      if (!app->behavior_variable_use_active_mode &&
          !app_parse_u32_or_default(app->behavior_variable_mode, 0u,
                                    &mode_index)) {
        app_set_status(app, "Function rule error: mode index invalid.");
        ok = false;
      }
    } else if (action_kind == STYGIAN_EDITOR_ACTION_NAVIGATE) {
      if (!app->behavior_navigate_target[0]) {
        app_set_status(app, "Function rule error: navigate target is required.");
        ok = false;
      }
    }

    if (ok) {
      rule.trigger_node = trigger_id;
      rule.trigger_event = app_event_kind_from_choice(app->behavior_event_choice);
      rule.action_kind = action_kind;
      if (action_kind == STYGIAN_EDITOR_ACTION_ANIMATE) {
        rule.animate.target_node = target_id;
        rule.animate.property =
            app_property_kind_from_choice(app->behavior_property_choice);
        rule.animate.from_value = from_value;
        rule.animate.to_value = to_value;
        rule.animate.duration_ms = duration_ms;
        rule.animate.easing = app_easing_from_choice(app->behavior_easing_choice);
      } else if (action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY) {
        rule.set_property.target_node = target_id;
        rule.set_property.property =
            app_property_kind_from_choice(app->behavior_property_choice);
        rule.set_property.value = to_value;
      } else if (action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY) {
        rule.toggle_visibility.target_node = target_id;
      } else if (action_kind == STYGIAN_EDITOR_ACTION_SET_VARIABLE) {
        snprintf(rule.set_variable.variable_name,
                 sizeof(rule.set_variable.variable_name), "%s",
                 app->behavior_variable_name);
        rule.set_variable.variable_kind = STYGIAN_EDITOR_VARIABLE_NUMBER;
        rule.set_variable.use_active_mode = app->behavior_variable_use_active_mode;
        rule.set_variable.mode_index = mode_index;
        rule.set_variable.number_value = number_value;
      } else if (action_kind == STYGIAN_EDITOR_ACTION_NAVIGATE) {
        snprintf(rule.navigate.target, sizeof(rule.navigate.target), "%s",
                 app->behavior_navigate_target);
      }

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

  y += 30.0f;
  if (app->font) {
    char contract[192];
    snprintf(contract, sizeof(contract),
             "Systems contract: schema %u.%u | generated: read-only | hooks: user-owned",
             STYGIAN_EDITOR_PROJECT_SCHEMA_MAJOR, STYGIAN_EDITOR_PROJECT_SCHEMA_MINOR);
    stygian_text(app->ctx, app->font, contract, x, y, 10.0f, 0.74f, 0.82f, 0.92f,
                 1.0f);
  }

  return mutated;
}
#endif

static bool app_add_tabs_scaffold(EditorBootstrapApp *app, float world_x,
                                  float world_y) {
  StygianEditorNodeId tabs_root = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId headers[8];
  StygianEditorNodeId contents[3];
  StygianEditorFrameDesc content = {0};
  uint32_t header_count = 0u;
  bool ok = true;
  if (!app || !app->editor)
    return false;
  if (!stygian_editor_add_control_recipe(app->editor, STYGIAN_EDITOR_RECIPE_TABS,
                                         world_x, world_y, 300.0f, 34.0f,
                                         "Tabs", &tabs_root) ||
      !tabs_root) {
    return false;
  }
  (void)stygian_editor_node_set_name(app->editor, tabs_root, "tabs_bar");
  header_count = stygian_editor_tree_list_children(app->editor, tabs_root,
                                                   headers, 8u);
  if (header_count < 3u)
    return false;

  memset(contents, 0, sizeof(contents));
  content.x = world_x;
  content.y = world_y + 40.0f;
  content.w = 300.0f;
  content.h = 170.0f;
  content.clip_content = true;
  content.visible = true;
  content.z = 0.0f;
  for (uint32_t i = 0u; i < 3u; ++i) {
    char name[32];
    content.fill = stygian_editor_color_rgba(0.10f + 0.03f * (float)i,
                                             0.12f + 0.02f * (float)i,
                                             0.16f + 0.02f * (float)i, 1.0f);
    content.visible = true;
    contents[i] = stygian_editor_add_frame(app->editor, &content);
    if (!contents[i]) {
      ok = false;
      break;
    }
    (void)stygian_editor_node_set_opacity(app->editor, contents[i],
                                          i == 0u ? 1.0f : 0.08f);
    snprintf(name, sizeof(name), "tabs_content_%u", i + 1u);
    (void)stygian_editor_node_set_name(app->editor, contents[i], name);
  }
  if (!ok)
    return false;

  for (uint32_t i = 0u; i < 3u; ++i) {
    for (uint32_t j = 0u; j < 3u; ++j) {
      StygianEditorBehaviorRule rule = {0};
      rule.trigger_node = headers[i];
      rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
      rule.action_kind = STYGIAN_EDITOR_ACTION_SET_PROPERTY;
      rule.set_property.target_node = headers[j];
      rule.set_property.property = STYGIAN_EDITOR_PROP_OPACITY;
      rule.set_property.value = (i == j) ? 1.0f : 0.62f;
      if (!stygian_editor_add_behavior(app->editor, &rule, NULL))
        ok = false;
    }
    for (uint32_t j = 0u; j < 3u; ++j) {
      StygianEditorBehaviorRule rule = {0};
      rule.trigger_node = headers[i];
      rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
      rule.action_kind = STYGIAN_EDITOR_ACTION_SET_PROPERTY;
      rule.set_property.target_node = contents[j];
      rule.set_property.property = STYGIAN_EDITOR_PROP_OPACITY;
      rule.set_property.value = (i == j) ? 1.0f : 0.08f;
      if (!stygian_editor_add_behavior(app->editor, &rule, NULL))
        ok = false;
    }
  }
  return ok;
}

static bool app_add_sidebar_template(EditorBootstrapApp *app, float world_x,
                                     float world_y) {
  StygianEditorFrameDesc root = {0};
  StygianEditorFrameDesc sidebar = {0};
  StygianEditorFrameDesc content = {0};
  StygianEditorNodeId root_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId side_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId content_id = STYGIAN_EDITOR_INVALID_ID;
  if (!app || !app->editor)
    return false;
  root.x = world_x;
  root.y = world_y;
  root.w = 700.0f;
  root.h = 420.0f;
  root.clip_content = true;
  root.fill = stygian_editor_color_rgba(0.08f, 0.09f, 0.12f, 1.0f);
  root.visible = true;
  root_id = stygian_editor_add_frame(app->editor, &root);
  if (!root_id)
    return false;
  (void)stygian_editor_node_set_name(app->editor, root_id, "template_sidebar_root");

  sidebar.x = world_x;
  sidebar.y = world_y;
  sidebar.w = 220.0f;
  sidebar.h = 420.0f;
  sidebar.clip_content = true;
  sidebar.fill = stygian_editor_color_rgba(0.13f, 0.15f, 0.20f, 1.0f);
  sidebar.visible = true;
  side_id = stygian_editor_add_frame(app->editor, &sidebar);
  if (!side_id || !stygian_editor_reparent_node(app->editor, side_id, root_id, false))
    return false;
  (void)stygian_editor_node_set_name(app->editor, side_id, "template_sidebar_panel");

  content.x = world_x + 220.0f;
  content.y = world_y;
  content.w = 480.0f;
  content.h = 420.0f;
  content.clip_content = true;
  content.fill = stygian_editor_color_rgba(0.10f, 0.12f, 0.16f, 1.0f);
  content.visible = true;
  content_id = stygian_editor_add_frame(app->editor, &content);
  if (!content_id ||
      !stygian_editor_reparent_node(app->editor, content_id, root_id, false)) {
    return false;
  }
  (void)stygian_editor_node_set_name(app->editor, content_id, "template_main_content");
  return true;
}

static bool app_add_modal_template(EditorBootstrapApp *app, float world_x,
                                   float world_y) {
  StygianEditorNodeId open_button = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId modal = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId close_button = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorRectDesc rect = {0};
  StygianEditorBehaviorRule rule = {0};
  if (!app || !app->editor)
    return false;

  rect.x = world_x;
  rect.y = world_y;
  rect.w = 180.0f;
  rect.h = 36.0f;
  rect.radius[0] = rect.radius[1] = rect.radius[2] = rect.radius[3] = 6.0f;
  rect.fill = stygian_editor_color_rgba(0.16f, 0.45f, 0.74f, 1.0f);
  rect.visible = true;
  open_button = stygian_editor_add_rect(app->editor, &rect);
  if (!open_button)
    return false;
  (void)stygian_editor_node_set_name(app->editor, open_button, "template_modal_open");

  rect.x = world_x + 80.0f;
  rect.y = world_y + 60.0f;
  rect.w = 360.0f;
  rect.h = 220.0f;
  rect.radius[0] = rect.radius[1] = rect.radius[2] = rect.radius[3] = 10.0f;
  rect.fill = stygian_editor_color_rgba(0.13f, 0.15f, 0.20f, 1.0f);
  rect.visible = true;
  modal = stygian_editor_add_rect(app->editor, &rect);
  if (!modal)
    return false;
  (void)stygian_editor_node_set_name(app->editor, modal, "template_modal_surface");
  (void)stygian_editor_node_set_opacity(app->editor, modal, 0.08f);

  rect.x = world_x + 380.0f;
  rect.y = world_y + 70.0f;
  rect.w = 48.0f;
  rect.h = 24.0f;
  rect.radius[0] = rect.radius[1] = rect.radius[2] = rect.radius[3] = 4.0f;
  rect.fill = stygian_editor_color_rgba(0.42f, 0.20f, 0.20f, 1.0f);
  rect.visible = true;
  close_button = stygian_editor_add_rect(app->editor, &rect);
  if (!close_button)
    return false;
  (void)stygian_editor_node_set_name(app->editor, close_button, "template_modal_close");

  rule.trigger_node = open_button;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
  rule.action_kind = STYGIAN_EDITOR_ACTION_SET_PROPERTY;
  rule.set_property.target_node = modal;
  rule.set_property.property = STYGIAN_EDITOR_PROP_OPACITY;
  rule.set_property.value = 1.0f;
  if (!stygian_editor_add_behavior(app->editor, &rule, NULL))
    return false;
  memset(&rule, 0, sizeof(rule));
  rule.trigger_node = close_button;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
  rule.action_kind = STYGIAN_EDITOR_ACTION_SET_PROPERTY;
  rule.set_property.target_node = modal;
  rule.set_property.property = STYGIAN_EDITOR_PROP_OPACITY;
  rule.set_property.value = 0.08f;
  if (!stygian_editor_add_behavior(app->editor, &rule, NULL))
    return false;
  return true;
}

static bool app_add_settings_template(EditorBootstrapApp *app, float world_x,
                                      float world_y) {
  StygianEditorFrameDesc panel = {0};
  StygianEditorNodeId panel_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId root = STYGIAN_EDITOR_INVALID_ID;
  if (!app || !app->editor)
    return false;
  panel.x = world_x;
  panel.y = world_y;
  panel.w = 420.0f;
  panel.h = 300.0f;
  panel.clip_content = true;
  panel.fill = stygian_editor_color_rgba(0.10f, 0.12f, 0.16f, 1.0f);
  panel.visible = true;
  panel_id = stygian_editor_add_frame(app->editor, &panel);
  if (!panel_id)
    return false;
  (void)stygian_editor_node_set_name(app->editor, panel_id, "template_settings_panel");

  if (!stygian_editor_add_control_recipe(app->editor, STYGIAN_EDITOR_RECIPE_TOGGLE,
                                         world_x + 24.0f, world_y + 40.0f, 58.0f,
                                         28.0f, "Dark Mode", &root) ||
      !root)
    return false;
  if (!stygian_editor_add_control_recipe(app->editor, STYGIAN_EDITOR_RECIPE_TOGGLE,
                                         world_x + 24.0f, world_y + 84.0f, 58.0f,
                                         28.0f, "Notifications", &root) ||
      !root)
    return false;
  if (!stygian_editor_add_control_recipe(
          app->editor, STYGIAN_EDITOR_RECIPE_INPUT_FIELD, world_x + 24.0f,
          world_y + 132.0f, 280.0f, 34.0f, "Display Name", &root) ||
      !root) {
    return false;
  }
  return true;
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
  float menu_x = 12.0f;
  float menu_group_x = 8.0f;
  float menu_group_w = 0.0f;
  float tool_y = menu_h;
  float button_h = 24.0f;
  float button_y = tool_y + 8.0f;
  float tool_x = 16.0f;
  float tool_w = 68.0f;
  float tool_gap = 6.0f;
  float select_x = tool_x;
  float rect_x = select_x + tool_w + tool_gap;
  float ellipse_x = rect_x + tool_w + tool_gap;
  float path_x = ellipse_x + tool_w + tool_gap;
  float pan_x = path_x + tool_w + tool_gap;
  float zoom_x = pan_x + tool_w + tool_gap;
  float primitive_x = zoom_x + tool_w + 10.0f;
  float primitive_w = 118.0f;
  float cmd_w = 104.0f;
  float mode_w = 112.0f;
  float insight_w = 112.0f;
  float insight_x = app->layout.top_w - 12.0f - insight_w;
  float mode_x = insight_x - 8.0f - mode_w;
  float cmd_x = mode_x - 8.0f - cmd_w;
  float quick_x = primitive_x + primitive_w + 14.0f;
  float quick_w = 72.0f;
  float quick_gap = 8.0f;
  float quick_need = (quick_w + quick_gap) * 4.0f - quick_gap;
  float quick_room = cmd_x - quick_x - 14.0f;
  int quick_count = 4;
  float badge_x = 0.0f;
  float badge_w = 0.0f;
  float file_menu_x = 10.0f;
  float file_menu_y = menu_h + 4.0f;
  float file_menu_w = 236.0f;
  float file_menu_hh = 360.0f;
  float prim_menu_x = 0.0f;
  float prim_menu_y = 0.0f;
  float prim_menu_w = 0.0f;
  float prim_row_h = 0.0f;
  float prim_menu_hh = 0.0f;
  float cmd_menu_x = cmd_x;
  float cmd_menu_y = top_h + 4.0f;
  float cmd_menu_w = 188.0f;
  float cmd_menu_h = 214.0f;
  float interactive_h = top_h;
  bool mutated = false;
  bool left_down;
  uint32_t selection_count = 0u;
  int i;
  char project_line[196];
  char status_line[256];

  if (!app || !window)
    return false;

  if (quick_room < quick_need + 130.0f)
    quick_count = 2;
  if (quick_room < 170.0f)
    quick_count = 0;
  badge_x = quick_x +
            (quick_count > 0 ? (quick_w + quick_gap) * (float)quick_count : 0.0f) +
            (quick_count > 0 ? 12.0f : 2.0f);
  badge_w = cmd_x - badge_x - 12.0f;

  app_primitive_menu_geometry(app, &prim_menu_x, &prim_menu_y, &prim_menu_w,
                              &prim_row_h, &prim_menu_hh);

  selection_count = app_selected_nodes(app, app->selection_ids_cache,
                                       APP_MAX_TRANSFORM_SELECTION);

  stygian_rect(app->ctx, app->layout.top_x, app->layout.top_y, app->layout.top_w,
               menu_h, 0.05f, 0.06f, 0.09f, 1.0f);
  stygian_rect(app->ctx, app->layout.top_x, menu_h, app->layout.top_w, strip_h,
               0.07f, 0.10f, 0.13f, 1.0f);
  stygian_rect(app->ctx, app->layout.top_x, menu_h + strip_h - 12.0f,
               app->layout.top_w, 12.0f, 0.06f, 0.08f, 0.11f, 1.0f);
  app_draw_divider(app, 0.0f, menu_h, app->layout.top_w, menu_h, 0.90f);
  app_draw_divider(app, 0.0f, top_h - 1.0f, app->layout.top_w, top_h - 1.0f,
                   0.90f);

  left_down = stygian_mouse_down(window, STYGIAN_MOUSE_LEFT);
  for (i = 0; i < (int)(sizeof(menu_labels) / sizeof(menu_labels[0])); ++i) {
    float label_w = 52.0f;
    if (app->font)
      label_w = stygian_text_width(app->ctx, app->font, menu_labels[i], 12.0f) +
                18.0f;
    if (label_w < 44.0f)
      label_w = 44.0f;
    menu_group_w += label_w + (i == 0 ? 8.0f : 4.0f);
  }
  menu_group_w += 8.0f;
  stygian_rect_rounded(app->ctx, menu_group_x, 3.0f, menu_group_w, menu_h - 6.0f,
                       0.11f, 0.13f, 0.17f, 0.95f, 6.0f);

  for (i = 0; i < (int)(sizeof(menu_labels) / sizeof(menu_labels[0])); ++i) {
    float label_w = 54.0f;
    bool clicked = false;
    bool hover = false;
    if (app->font) {
      label_w = stygian_text_width(app->ctx, app->font, menu_labels[i], 12.0f) +
                18.0f;
      if (label_w < 46.0f)
        label_w = 46.0f;
    }
    hover = app_point_in_rect((float)mouse_x, (float)mouse_y, menu_x, 4.0f,
                              label_w, menu_h - 8.0f);
    clicked =
        stygian_button(app->ctx, app->font, NULL, menu_x, 4.0f, label_w,
                       menu_h - 8.0f);
    if (app->file_menu_open && app->active_menu_index == i) {
      stygian_rect(app->ctx, menu_x + 7.0f, menu_h - 3.0f, label_w - 14.0f, 2.0f,
                   0.43f, 0.67f, 1.0f, 0.95f);
    }
    if (app->font) {
      stygian_text(app->ctx, app->font, menu_labels[i], menu_x + 8.0f, 6.0f,
                   12.0f,
                   hover || (app->file_menu_open && app->active_menu_index == i)
                       ? 0.97f
                       : 0.92f,
                   hover || (app->file_menu_open && app->active_menu_index == i)
                       ? 0.98f
                       : 0.94f,
                   0.99f, 1.0f);
    }
    if (clicked) {
      app_toggle_top_menu(app, i);
      mutated = true;
    }
    menu_x += label_w + 4.0f;
  }

  if (app->font) {
    const char *session_name = app->project_path[0] ? app->project_path
                                                    : "In-memory session";
    snprintf(project_line, sizeof(project_line), "%s | selection:%u | nodes:%u",
             session_name, selection_count, stygian_editor_node_count(app->editor));
    stygian_text(app->ctx, app->font, project_line, menu_group_x + menu_group_w + 12.0f,
                 6.0f, 11.0f, 0.69f, 0.77f, 0.88f, 1.0f);
  }

  if (app->file_menu_open && app->active_menu_index == 0) {
    float fy = file_menu_y + 10.0f;
    file_menu_hh = 462.0f;
    app_draw_panel_card(app, file_menu_x, file_menu_y, file_menu_w, file_menu_hh);
    if (app->font) {
      stygian_text(app->ctx, app->font, "Project", file_menu_x + 10.0f, fy,
                   11.0f, 0.67f, 0.76f, 0.88f, 1.0f);
    }
    fy += 16.0f;
    stygian_text_input(app->ctx, app->font, file_menu_x + 10.0f, fy,
                       file_menu_w - 20.0f, 20.0f, app->project_path_draft,
                       (int)sizeof(app->project_path_draft));
    fy += 26.0f;
    if (stygian_button(app->ctx, app->font, "New Project", file_menu_x + 10.0f,
                       fy, file_menu_w - 20.0f, 24.0f)) {
      app_reset_canvas(app);
      app_set_status(app, "Canvas reset.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Open Project", file_menu_x + 10.0f,
                       fy, file_menu_w - 20.0f, 24.0f)) {
      snprintf(app->project_path, sizeof(app->project_path), "%s",
               app->project_path_draft);
      if (app_load_project(app)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Loaded %s", app->project_path);
        app_set_status(app, msg);
      } else {
        app_set_status(app, "Project load failed.");
      }
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Save Project", file_menu_x + 10.0f,
                       fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_save_project(app)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Saved %s", app->project_path);
        app_set_status(app, msg);
      } else {
        app_set_status(app, "Project save failed.");
      }
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Save As", file_menu_x + 10.0f, fy,
                       file_menu_w - 20.0f, 24.0f)) {
      if (app_save_project_as(app)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Saved %s", app->project_path);
        app_set_status(app, msg);
      } else {
        app_set_status(app, "Save As failed. Set a valid project path first.");
      }
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 34.0f;
    if (app->font) {
      stygian_text(app->ctx, app->font, "Recent", file_menu_x + 10.0f, fy,
                   11.0f, 0.67f, 0.76f, 0.88f, 1.0f);
    }
    fy += 16.0f;
    if (app->recent_project_count == 0u && app->font) {
      stygian_text(app->ctx, app->font, "No recent projects yet.",
                   file_menu_x + 10.0f, fy + 2.0f, 10.0f, 0.62f, 0.70f, 0.80f,
                   1.0f);
      fy += 22.0f;
    } else {
      for (uint32_t recent_i = 0u;
           recent_i < app->recent_project_count && recent_i < 4u; ++recent_i) {
        if (stygian_button(app->ctx, app->font,
                           app->recent_project_paths[recent_i],
                           file_menu_x + 10.0f, fy, file_menu_w - 20.0f, 20.0f)) {
          if (app_open_recent_project(app, recent_i))
            app_set_status(app, "Loaded recent project.");
          else
            app_set_status(app, "Recent project load failed.");
          app->file_menu_open = false;
          mutated = true;
        }
        fy += 24.0f;
      }
    }
    if (app->font) {
      stygian_text(app->ctx, app->font, "Import", file_menu_x + 10.0f, fy,
                   11.0f, 0.67f, 0.76f, 0.88f, 1.0f);
    }
    fy += 16.0f;
    stygian_text_input(app->ctx, app->font, file_menu_x + 10.0f, fy,
                       file_menu_w - 20.0f, 20.0f, app->import_image_source,
                       (int)sizeof(app->import_image_source));
    fy += 24.0f;
    if (stygian_button(app->ctx, app->font, "Import Image", file_menu_x + 10.0f,
                       fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_import_image_from_draft(app))
        app_set_status(app, "Imported image from path.");
      else
        app_set_status(app, "Image import failed. Pick a path first.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 34.0f;
    if (app->font) {
      stygian_text(app->ctx, app->font, "Export", file_menu_x + 10.0f, fy,
                   11.0f, 0.67f, 0.76f, 0.88f, 1.0f);
    }
    fy += 16.0f;
    if (stygian_button(app->ctx, app->font, "Export Runtime Bundle",
                       file_menu_x + 10.0f, fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_export_runtime_bundle(
              app, "editor/build/windows/stygian_export",
              "editor/build/windows/stygian_export/stygian_export_report.txt")) {
        app_set_status(app, "Exported runtime bundle + report.");
      } else {
        app_set_status(app, "Runtime bundle export failed.");
      }
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Export Editor Shell",
                       file_menu_x + 10.0f, fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_export_editor_layout_shell(
              "editor/build/stygian_editor_layout_shell.generated.c")) {
        app_set_status(
            app,
            "Exported editor/build/stygian_editor_layout_shell.generated.c");
      } else {
        app_set_status(app, "Editor shell export failed.");
      }
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 34.0f;
    if (app->font) {
      stygian_text(app->ctx, app->font, "Session", file_menu_x + 10.0f, fy,
                   11.0f, 0.67f, 0.76f, 0.88f, 1.0f);
    }
    fy += 16.0f;
    if (stygian_button(app->ctx, app->font, "Quit", file_menu_x + 10.0f, fy,
                       file_menu_w - 20.0f, 24.0f)) {
      stygian_window_request_close(window);
    }
  } else if (app->file_menu_open && app->active_menu_index == 1) {
    float fy = file_menu_y + 10.0f;
    float panel_h = 300.0f;
    app_draw_panel_card(app, file_menu_x, file_menu_y, file_menu_w, panel_h);
    if (app->font) {
      stygian_text(app->ctx, app->font, "Edit", file_menu_x + 10.0f, fy, 11.0f,
                   0.67f, 0.76f, 0.88f, 1.0f);
    }
    fy += 16.0f;
    if (stygian_button(app->ctx, app->font, "Undo", file_menu_x + 10.0f, fy,
                       file_menu_w - 20.0f, 24.0f)) {
      if (app_undo(app))
        app_set_status(app, "Undo applied.");
      else
        app_set_status(app, "Nothing to undo.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Redo", file_menu_x + 10.0f, fy,
                       file_menu_w - 20.0f, 24.0f)) {
      if (app_redo(app))
        app_set_status(app, "Redo applied.");
      else
        app_set_status(app, "Nothing to redo.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 34.0f;
    if (app->font) {
      stygian_text(app->ctx, app->font, "Clipboard", file_menu_x + 10.0f, fy,
                   11.0f, 0.67f, 0.76f, 0.88f, 1.0f);
    }
    fy += 16.0f;
    if (stygian_button(app->ctx, app->font, "Cut", file_menu_x + 10.0f, fy,
                       file_menu_w - 20.0f, 24.0f)) {
      if (app_cut_selection(app))
        app_set_status(app, "Selection cut.");
      else
        app_set_status(app, "Nothing to cut.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Copy", file_menu_x + 10.0f, fy,
                       file_menu_w - 20.0f, 24.0f)) {
      if (app_copy_selection(app))
        app_set_status(app, "Selection copied.");
      else
        app_set_status(app, "Nothing to copy.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Paste", file_menu_x + 10.0f, fy,
                       file_menu_w - 20.0f, 24.0f)) {
      float wx = 0.0f;
      float wy = 0.0f;
      stygian_editor_view_to_world(app->editor,
                                   app->layout.viewport_x + app->layout.viewport_w * 0.5f,
                                   app->layout.viewport_y + app->layout.viewport_h * 0.5f,
                                   &wx, &wy);
      if (app_paste_clipboard(app, wx, wy, true))
        app_set_status(app, "Clipboard pasted.");
      else
        app_set_status(app, "Clipboard is empty.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 34.0f;
    if (app->font) {
      stygian_text(app->ctx, app->font, "Objects", file_menu_x + 10.0f, fy,
                   11.0f, 0.67f, 0.76f, 0.88f, 1.0f);
    }
    fy += 16.0f;
    if (stygian_button(app->ctx, app->font, "Duplicate", file_menu_x + 10.0f, fy,
                       file_menu_w - 20.0f, 24.0f)) {
      if (app_duplicate_selection(app, 24.0f, 24.0f))
        app_set_status(app, "Selection duplicated.");
      else
        app_set_status(app, "Nothing to duplicate.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Delete", file_menu_x + 10.0f, fy,
                       file_menu_w - 20.0f, 24.0f)) {
      if (app_delete_selection(app))
        app_set_status(app, "Selection deleted.");
      else
        app_set_status(app, "Nothing to delete.");
      app->file_menu_open = false;
      mutated = true;
    }
  } else if (app->file_menu_open && app->active_menu_index == 2) {
    float fy = file_menu_y + 10.0f;
    float panel_h = 380.0f;
    app_draw_panel_card(app, file_menu_x, file_menu_y, file_menu_w, panel_h);
    if (app->font) {
      stygian_text(app->ctx, app->font, "View", file_menu_x + 10.0f, fy, 11.0f,
                   0.67f, 0.76f, 0.88f, 1.0f);
    }
    fy += 16.0f;
    if (stygian_button(app->ctx, app->font, "Toggle Command Palette",
                       file_menu_x + 10.0f, fy, file_menu_w - 20.0f, 24.0f)) {
      app->command_palette_open = !app->command_palette_open;
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font,
                       app->shortcut_help_open ? "Hide Shortcut Help"
                                               : "Show Shortcut Help",
                       file_menu_x + 10.0f, fy, file_menu_w - 20.0f, 24.0f)) {
      app->shortcut_help_open = !app->shortcut_help_open;
      app_set_status(app, app->shortcut_help_open ? "Shortcut help shown."
                                                  : "Shortcut help hidden.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font,
                       app->code_insight_enabled ? "Disable Code Insight"
                                                 : "Enable Code Insight",
                       file_menu_x + 10.0f, fy, file_menu_w - 20.0f, 24.0f)) {
      app->code_insight_enabled = !app->code_insight_enabled;
      if (!app->code_insight_enabled)
        snprintf(app->code_insight_text, sizeof(app->code_insight_text),
                 "Code insight is off.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(
            app->ctx, app->font,
            app->complexity_mode == APP_COMPLEXITY_BEGINNER ? "Switch To Systems"
                                                            : "Switch To Beginner",
            file_menu_x + 10.0f, fy, file_menu_w - 20.0f, 24.0f)) {
      app->complexity_mode =
          app->complexity_mode == APP_COMPLEXITY_BEGINNER
              ? APP_COMPLEXITY_SYSTEMS
              : APP_COMPLEXITY_BEGINNER;
      app_set_status(app, app->complexity_mode == APP_COMPLEXITY_BEGINNER
                              ? "Beginner mode active."
                              : "Systems mode active.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 34.0f;
    if (app->font) {
      stygian_text(app->ctx, app->font, "Canvas", file_menu_x + 10.0f, fy,
                   11.0f, 0.67f, 0.76f, 0.88f, 1.0f);
    }
    fy += 16.0f;
    if (stygian_button(app->ctx, app->font,
                       app->grid_enabled ? "Hide Major Grid" : "Show Major Grid",
                       file_menu_x + 10.0f, fy, file_menu_w - 20.0f, 24.0f)) {
      app->grid_enabled = !app->grid_enabled;
      app_sync_grid_config(app);
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font,
                       app->sub_snap_enabled ? "Hide Sub Grid" : "Show Sub Grid",
                       file_menu_x + 10.0f, fy, file_menu_w - 20.0f, 24.0f)) {
      app->sub_snap_enabled = !app->sub_snap_enabled;
      app_sync_grid_config(app);
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Zoom In", file_menu_x + 10.0f, fy,
                       file_menu_w - 20.0f, 24.0f)) {
      float focus_x = app->layout.viewport_x + app->layout.viewport_w * 0.5f;
      float focus_y = app->layout.viewport_y + app->layout.viewport_h * 0.5f;
      app_camera_zoom_about(app, focus_x, focus_y, app->camera_zoom * 1.15f);
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Zoom Out", file_menu_x + 10.0f, fy,
                       file_menu_w - 20.0f, 24.0f)) {
      float focus_x = app->layout.viewport_x + app->layout.viewport_w * 0.5f;
      float focus_y = app->layout.viewport_y + app->layout.viewport_h * 0.5f;
      app_camera_zoom_about(app, focus_x, focus_y, app->camera_zoom / 1.15f);
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Reset Zoom", file_menu_x + 10.0f, fy,
                       file_menu_w - 20.0f, 24.0f)) {
      float focus_x = app->layout.viewport_x + app->layout.viewport_w * 0.5f;
      float focus_y = app->layout.viewport_y + app->layout.viewport_h * 0.5f;
      app_camera_zoom_about(app, focus_x, focus_y, 1.0f);
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 34.0f;
    if (app->font) {
      stygian_text(app->ctx, app->font, "Workspace Seeds", file_menu_x + 10.0f, fy,
                   11.0f, 0.67f, 0.76f, 0.88f, 1.0f);
    }
    fy += 16.0f;
    if (stygian_button(app->ctx, app->font, "Seed Stage 1", file_menu_x + 10.0f,
                       fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_seed_self_host_surface_stage(app, APP_SELF_HOST_STAGE_1)) {
        app_set_status(app, "Seeded self-host stage 1 surfaces.");
      } else {
        app_set_status(app, "Self-host stage 1 seed failed.");
      }
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Seed Stage 2", file_menu_x + 10.0f,
                       fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_seed_self_host_surface_stage(app, APP_SELF_HOST_STAGE_2)) {
        app_set_status(app, "Seeded self-host stage 2 surfaces.");
      } else {
        app_set_status(app, "Self-host stage 2 seed failed.");
      }
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Seed Stage 3", file_menu_x + 10.0f,
                       fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_seed_self_host_surface_stage(app, APP_SELF_HOST_STAGE_3)) {
        app_set_status(app, "Seeded self-host stage 3 surfaces.");
      } else {
        app_set_status(app, "Self-host stage 3 seed failed.");
      }
      app->file_menu_open = false;
      mutated = true;
    }
  } else if (app->file_menu_open && app->active_menu_index == 3) {
    float fy = file_menu_y + 10.0f;
    float panel_h = 294.0f;
    app_draw_panel_card(app, file_menu_x, file_menu_y, file_menu_w, panel_h);
    if (app->font) {
      stygian_text(app->ctx, app->font, "Selection", file_menu_x + 10.0f, fy,
                   11.0f, 0.67f, 0.76f, 0.88f, 1.0f);
    }
    fy += 16.0f;
    if (stygian_button(app->ctx, app->font, "Select All", file_menu_x + 10.0f, fy,
                       file_menu_w - 20.0f, 24.0f)) {
      if (app_select_all_nodes(app))
        app_set_status(app, "Selected all nodes.");
      else
        app_set_status(app, "Nothing to select.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Select Focus Branch",
                       file_menu_x + 10.0f, fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_select_focus_children(app))
        app_set_status(app, "Selected nodes in current branch.");
      else
        app_set_status(app, "Nothing to select in current branch.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Select Parent", file_menu_x + 10.0f,
                       fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_select_parent(app))
        app_set_status(app, "Selected parent.");
      else
        app_set_status(app, "Selected node has no parent.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Clear Selection",
                       file_menu_x + 10.0f, fy, file_menu_w - 20.0f, 24.0f)) {
      (void)stygian_editor_select_node(app->editor, STYGIAN_EDITOR_INVALID_ID,
                                       false);
      app_set_status(app, "Selection cleared.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 34.0f;
    if (app->font) {
      stygian_text(app->ctx, app->font, "State", file_menu_x + 10.0f, fy,
                   11.0f, 0.67f, 0.76f, 0.88f, 1.0f);
    }
    fy += 16.0f;
    if (stygian_button(app->ctx, app->font, "Show Selected", file_menu_x + 10.0f,
                       fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_set_selection_visible(app, true))
        app_set_status(app, "Selected nodes shown.");
      else
        app_set_status(app, "Nothing changed.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Hide Selected", file_menu_x + 10.0f,
                       fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_set_selection_visible(app, false))
        app_set_status(app, "Selected nodes hidden.");
      else
        app_set_status(app, "Nothing changed.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Lock Selected", file_menu_x + 10.0f,
                       fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_set_selection_locked(app, true))
        app_set_status(app, "Selected nodes locked.");
      else
        app_set_status(app, "Nothing changed.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Unlock Selected",
                       file_menu_x + 10.0f, fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_set_selection_locked(app, false))
        app_set_status(app, "Selected nodes unlocked.");
      else
        app_set_status(app, "Nothing changed.");
      app->file_menu_open = false;
      mutated = true;
    }
  } else if (app->file_menu_open && app->active_menu_index == 4) {
    float fy = file_menu_y + 10.0f;
    float panel_h = 188.0f;
    app_draw_panel_card(app, file_menu_x, file_menu_y, file_menu_w, panel_h);
    if (app->font) {
      stygian_text(app->ctx, app->font, "Help", file_menu_x + 10.0f, fy, 11.0f,
                   0.67f, 0.76f, 0.88f, 1.0f);
    }
    fy += 16.0f;
    if (stygian_button(app->ctx, app->font, "Open Generated Code",
                       file_menu_x + 10.0f, fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_open_text_file_in_default_editor(app_generated_scene_path()))
        app_set_status(app, "Opened generated code.");
      else
        app_set_status(app, "Failed to open generated code.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Open Hook Source",
                       file_menu_x + 10.0f, fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_open_text_file_in_default_editor(app_hook_source_path()))
        app_set_status(app, "Opened hook source.");
      else
        app_set_status(app, "Failed to open hook source.");
      app->file_menu_open = false;
      mutated = true;
    }
    fy += 28.0f;
    if (stygian_button(app->ctx, app->font, "Run Binding Doctor",
                       file_menu_x + 10.0f, fy, file_menu_w - 20.0f, 24.0f)) {
      if (app_run_binding_doctor(
              app, "editor/build/windows/stygian_export/stygian_binding_doctor.txt",
              "editor/build/windows/stygian_export/stygian_editor_hooks.c")) {
        app_set_status(app, "Binding doctor report written.");
      } else {
        app_set_status(app, "Binding doctor failed.");
      }
      app->file_menu_open = false;
      mutated = true;
    }
  }

  app->primitive_menu_hover_index = -1;
  if (stygian_button(app->ctx, app->font,
                     app->current_tool == APP_TOOL_SELECT ? "Select*" : "Select",
                     select_x, button_y, tool_w, button_h)) {
    app_set_current_tool(app, APP_TOOL_SELECT);
    app->file_menu_open = false;
    app_set_status(app, "Tool: Select");
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font,
                     app->current_tool == APP_TOOL_RECT_FRAME ? "Rect*" : "Rect",
                     rect_x, button_y, tool_w, button_h)) {
    app->selected_primitive = APP_PRIMITIVE_RECTANGLE;
    app_set_current_tool(app, APP_TOOL_RECT_FRAME);
    app->file_menu_open = false;
    app_set_status(app, "Tool: Rect/Frame");
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font,
                     app->current_tool == APP_TOOL_ELLIPSE ? "Ellipse*" : "Ellipse",
                     ellipse_x, button_y, tool_w, button_h)) {
    app->selected_primitive = APP_PRIMITIVE_ELLIPSE;
    app_set_current_tool(app, APP_TOOL_ELLIPSE);
    app->file_menu_open = false;
    app_set_status(app, "Tool: Ellipse");
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font,
                     app->current_tool == APP_TOOL_PATH ? "Path*" : "Path",
                     path_x, button_y, tool_w, button_h)) {
    app->file_menu_open = false;
    app_set_current_tool(app, APP_TOOL_PATH);
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font,
                     app->current_tool == APP_TOOL_PAN ? "Pan*" : "Pan", pan_x,
                     button_y, tool_w, button_h)) {
    app->file_menu_open = false;
    app_set_current_tool(app, APP_TOOL_PAN);
    app_set_status(app, "Tool: Pan");
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font,
                     app->current_tool == APP_TOOL_ZOOM ? "Zoom*" : "Zoom",
                     zoom_x, button_y, tool_w, button_h)) {
    app->file_menu_open = false;
    app_set_current_tool(app, APP_TOOL_ZOOM);
    app_set_status(app, "Tool: Zoom");
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, app_primitive_name(app->selected_primitive),
                     primitive_x, button_y, primitive_w, button_h)) {
    app->primitive_menu_open = !app->primitive_menu_open;
    app->file_menu_open = false;
    mutated = true;
  }

  if (app->primitive_menu_open) {
    app->primitive_menu_hover_index =
        app_primitive_menu_hover_index_at(app, (float)mouse_x, (float)mouse_y);

    app_draw_panel_card(app, prim_menu_x, prim_menu_y, prim_menu_w, prim_menu_hh);
    for (i = 0; i < 4; ++i) {
      if (app->primitive_menu_hover_index == i) {
        stygian_rect_rounded(app->ctx, prim_menu_x + 6.0f,
                             prim_menu_y + 6.0f + prim_row_h * (float)i,
                             prim_menu_w - 12.0f, prim_row_h - 4.0f, 0.19f, 0.29f,
                             0.42f, 0.96f, 4.0f);
      }
      if (stygian_button(app->ctx, app->font, primitive_labels[i],
                         prim_menu_x + 8.0f,
                         prim_menu_y + 8.0f + prim_row_h * (float)i,
                         prim_menu_w - 16.0f, prim_row_h - 6.0f)) {
        app->selected_primitive = (AppPrimitiveKind)i;
        app_set_current_tool(app, app_tool_from_primitive(app->selected_primitive));
        mutated = true;
        app->primitive_menu_open = false;
        app_set_status(app, "Primitive selected. Place it in the viewport.");
      }
    }
  }

  if (app->font) {
    float badge_y = tool_y + 10.0f;
    float bx = badge_x;
    if (badge_w > 420.0f) {
      app_draw_badge(app, bx, badge_y, 126.0f, 16.0f, 0.19f, 0.28f, 0.42f,
                     "Generated: exporter-owned");
      bx += 132.0f;
      app_draw_badge(app, bx, badge_y, 120.0f, 16.0f, 0.20f, 0.34f, 0.23f,
                     "Hooks: user-owned");
      bx += 126.0f;
      app_draw_badge(app, bx, badge_y, 108.0f, 16.0f, 0.36f, 0.29f, 0.16f,
                     "Bindings: logic");
    } else if (badge_w > 230.0f) {
      app_draw_badge(app, bx, badge_y, 214.0f, 16.0f, 0.18f, 0.24f, 0.31f,
                     "Generated layout | Hook logic");
    }

    if (badge_w > 180.0f) {
      snprintf(status_line, sizeof(status_line),
               "%s | Tool: %s | %s",
               app->status_text[0] ? app->status_text
                                   : "Canvas ready. Pick a tool and work in the viewport.",
               app_tool_name(app->current_tool),
               "Export owns generated code. Hooks stay yours.");
      stygian_text(app->ctx, app->font, status_line, badge_x, tool_y + 31.0f,
                   10.8f, 0.72f, 0.80f, 0.89f, 1.0f);
    }
  }

  if (stygian_button(app->ctx, app->font,
                     app->command_palette_open ? "Cmd: Open" : "Cmd: Closed",
                     cmd_x, button_y, 116.0f, button_h)) {
    app->command_palette_open = !app->command_palette_open;
    app->file_menu_open = false;
    app->primitive_menu_open = false;
    mutated = true;
  }

  if (stygian_button(
          app->ctx, app->font,
          app->complexity_mode == APP_COMPLEXITY_BEGINNER ? "Mode: Beginner"
                                                          : "Mode: Systems",
          mode_x, button_y, 116.0f, button_h)) {
    app->complexity_mode =
        app->complexity_mode == APP_COMPLEXITY_BEGINNER ? APP_COMPLEXITY_SYSTEMS
                                                        : APP_COMPLEXITY_BEGINNER;
    app_set_status(app,
                   app->complexity_mode == APP_COMPLEXITY_BEGINNER
                       ? "Beginner mode: simplified labels and fewer low-level terms."
                       : "Systems mode: explicit IDs/contracts and code insight.");
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font,
                     app->code_insight_enabled ? "Code Insight: On"
                                               : "Code Insight: Off",
                     insight_x, button_y, 116.0f, button_h)) {
    app->code_insight_enabled = !app->code_insight_enabled;
    app->code_insight_node = STYGIAN_EDITOR_INVALID_ID;
    if (!app->code_insight_enabled) {
      snprintf(app->code_insight_text, sizeof(app->code_insight_text),
               "Code insight is off.");
    }
    mutated = true;
  }

  if (app->command_palette_open) {
    app_draw_panel_card(app, cmd_menu_x, cmd_menu_y, cmd_menu_w, cmd_menu_h);
    if (stygian_button(app->ctx, app->font, "Save (Ctrl+S)", cmd_menu_x + 6.0f,
                       cmd_menu_y + 6.0f, cmd_menu_w - 12.0f, 20.0f)) {
      if (app_save_project(app))
        app_set_status(app, "Saved project.");
      else
        app_set_status(app, "Project save failed.");
      mutated = true;
    }
    if (stygian_button(app->ctx, app->font, "Load (Ctrl+O)", cmd_menu_x + 6.0f,
                       cmd_menu_y + 30.0f, cmd_menu_w - 12.0f, 20.0f)) {
      if (app_load_project(app))
        app_set_status(app, "Loaded project.");
      else
        app_set_status(app, "Project load failed.");
      mutated = true;
    }
    if (stygian_button(app->ctx, app->font, "Export Bundle (Ctrl+E)",
                       cmd_menu_x + 6.0f, cmd_menu_y + 54.0f, cmd_menu_w - 12.0f,
                       20.0f)) {
      if (app_export_runtime_bundle(
              app, "editor/build/windows/stygian_export",
              "editor/build/windows/stygian_export/stygian_export_report.txt")) {
        app_set_status(app, "Exported runtime bundle.");
      } else {
        app_set_status(app, "Runtime bundle export failed.");
      }
      mutated = true;
    }
    if (stygian_button(app->ctx, app->font, "Binding Doctor", cmd_menu_x + 6.0f,
                       cmd_menu_y + 78.0f, cmd_menu_w - 12.0f, 20.0f)) {
      if (app_run_binding_doctor(
              app, "editor/build/windows/stygian_export/stygian_binding_doctor.txt",
              "editor/build/windows/stygian_export/stygian_editor_hooks.c")) {
        app_set_status(app, "Binding doctor report written.");
      } else {
        app_set_status(app, "Binding doctor failed.");
      }
      mutated = true;
    }
    if (stygian_button(app->ctx, app->font,
                       app->shortcut_help_open ? "Hide Shortcut Help (F1)"
                                               : "Show Shortcut Help (F1)",
                       cmd_menu_x + 6.0f, cmd_menu_y + 102.0f, cmd_menu_w - 12.0f,
                       20.0f)) {
      app->shortcut_help_open = !app->shortcut_help_open;
      mutated = true;
    }
    if (stygian_button(app->ctx, app->font, "New Canvas (Ctrl+N)",
                       cmd_menu_x + 6.0f, cmd_menu_y + 126.0f, cmd_menu_w - 12.0f,
                       20.0f)) {
      app_reset_canvas(app);
      app_set_status(app, "Canvas reset.");
      mutated = true;
    }
    if (stygian_button(app->ctx, app->font, "Seed Dogfood Stage 1 (Ctrl+1)",
                       cmd_menu_x + 6.0f, cmd_menu_y + 150.0f, cmd_menu_w - 12.0f,
                       20.0f)) {
      if (app_seed_self_host_surface_stage(app, APP_SELF_HOST_STAGE_1))
        app_set_status(app, "Seeded self-host stage 1 surfaces.");
      else
        app_set_status(app, "Self-host stage 1 seed failed.");
      mutated = true;
    }
    if (stygian_button(app->ctx, app->font, "Seed Dogfood Stage 2 (Ctrl+2)",
                       cmd_menu_x + 6.0f, cmd_menu_y + 174.0f, cmd_menu_w - 12.0f,
                       20.0f)) {
      if (app_seed_self_host_surface_stage(app, APP_SELF_HOST_STAGE_2))
        app_set_status(app, "Seeded self-host stage 2 surfaces.");
      else
        app_set_status(app, "Self-host stage 2 seed failed.");
      mutated = true;
    }
    if (stygian_button(app->ctx, app->font, "Seed Dogfood Stage 3 (Ctrl+3)",
                       cmd_menu_x + 6.0f, cmd_menu_y + 198.0f, cmd_menu_w - 12.0f,
                       20.0f)) {
      if (app_seed_self_host_surface_stage(app, APP_SELF_HOST_STAGE_3))
        app_set_status(app, "Seeded self-host stage 3 surfaces.");
      else
        app_set_status(app, "Self-host stage 3 seed failed.");
      mutated = true;
    }
  }

  if (quick_count > 0 &&
      stygian_button(app->ctx, app->font, "Tabs", quick_x, button_y, quick_w,
                     button_h)) {
    float vx = app->layout.viewport_x + app->layout.viewport_w * 0.5f - 150.0f;
    float vy = app->layout.viewport_y + 80.0f;
    float wx = 0.0f;
    float wy = 0.0f;
    stygian_editor_view_to_world(app->editor, vx, vy, &wx, &wy);
    if (app_add_tabs_scaffold(app, wx, wy)) {
      app_set_status(app, "Tabs scaffold added.");
    } else {
      app_set_status(app, "Tabs scaffold add failed.");
    }
    mutated = true;
  }
  if (quick_count > 1 &&
      stygian_button(app->ctx, app->font, "Sidebar", quick_x + (quick_w + quick_gap),
                     button_y, quick_w, button_h)) {
    float wx = 0.0f;
    float wy = 0.0f;
    stygian_editor_view_to_world(
        app->editor, app->layout.viewport_x + 100.0f, app->layout.viewport_y + 90.0f,
        &wx, &wy);
    if (app_add_sidebar_template(app, wx, wy))
      app_set_status(app, "Sidebar template added.");
    else
      app_set_status(app, "Sidebar template failed.");
    mutated = true;
  }
  if (quick_count > 2 &&
      stygian_button(app->ctx, app->font, "Modal",
                     quick_x + (quick_w + quick_gap) * 2.0f, button_y, quick_w,
                     button_h)) {
    float wx = 0.0f;
    float wy = 0.0f;
    stygian_editor_view_to_world(
        app->editor, app->layout.viewport_x + 130.0f, app->layout.viewport_y + 110.0f,
        &wx, &wy);
    if (app_add_modal_template(app, wx, wy))
      app_set_status(app, "Modal template added.");
    else
      app_set_status(app, "Modal template failed.");
    mutated = true;
  }
  if (quick_count > 3 &&
      stygian_button(app->ctx, app->font, "Settings",
                     quick_x + (quick_w + quick_gap) * 3.0f, button_y, quick_w,
                     button_h)) {
    float wx = 0.0f;
    float wy = 0.0f;
    stygian_editor_view_to_world(
        app->editor, app->layout.viewport_x + 150.0f, app->layout.viewport_y + 140.0f,
        &wx, &wy);
    if (app_add_settings_template(app, wx, wy))
      app_set_status(app, "Settings template added.");
    else
      app_set_status(app, "Settings template failed.");
    mutated = true;
  }

  if (app->file_menu_open) {
    if (file_menu_y + file_menu_hh > interactive_h)
      interactive_h = file_menu_y + file_menu_hh;
  }
  if (app->primitive_menu_open) {
    if (prim_menu_y + prim_menu_hh > interactive_h)
      interactive_h = prim_menu_y + prim_menu_hh;
  }
  if (app->command_palette_open) {
    if (cmd_menu_y + cmd_menu_h > interactive_h)
      interactive_h = cmd_menu_y + cmd_menu_h;
  }
  if (app->shortcut_help_open && app->font) {
    stygian_text(app->ctx, app->font,
                 "Shortcuts: V/R/O/P/H/Z tools | Ctrl+Z/Y undo redo | Ctrl+C/X/V copy cut paste | Ctrl+D duplicate | Ctrl+A select all | Delete remove | Arrows nudge",
                 16.0f, top_h - 12.0f, 10.0f, 0.66f, 0.74f, 0.84f, 1.0f);
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

static void app_draw_rotation_handle(EditorBootstrapApp *app) {
  float cx = 0.0f;
  float cy = 0.0f;
  float hs = app_rotation_handle_size_px();
  bool hot;
  if (!app || !app->editor)
    return;
  if (!app_rotation_handle_center_view(app, &cx, &cy))
    return;
  hot = app_rotation_handle_hit(app, (float)app->last_mouse_x,
                                (float)app->last_mouse_y) ||
        app->rotating_selection;
  stygian_line(app->ctx, cx, cy + hs * 0.5f, cx,
               cy + hs * 0.5f + 12.0f, 1.3f, 0.48f, 0.72f, 1.0f, 0.95f);
  stygian_rect_rounded(app->ctx, cx - hs * 0.5f, cy - hs * 0.5f, hs, hs,
                       hot ? 0.15f : 0.10f, hot ? 0.70f : 0.56f, 0.98f, 1.0f,
                       hs * 0.5f);
  stygian_rect_rounded(app->ctx, cx - hs * 0.5f + 1.0f, cy - hs * 0.5f + 1.0f,
                       hs - 2.0f, hs - 2.0f, hot ? 0.86f : 0.72f,
                       hot ? 0.94f : 0.86f, 1.0f, 1.0f, hs * 0.5f);
}

static void app_draw_context_menu(EditorBootstrapApp *app) {
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
  float row_y;
  bool has_selection;
  if (!app || !app->font || !app->context_menu_open)
    return;
  app_context_menu_geometry(app, &x, &y, &w, &h);
  has_selection = stygian_editor_selected_count(app->editor) > 0u;
  app_draw_panel_card(app, x, y, w, h);
  stygian_text(app->ctx, app->font, has_selection ? "Selection" : "Canvas",
               x + 10.0f, y + 8.0f, 11.0f, 0.71f, 0.79f, 0.89f, 1.0f);
  row_y = y + 26.0f;
  if (stygian_button(app->ctx, app->font, "Copy", x + 8.0f, row_y, w - 16.0f, 20.0f)) {
    if (app_copy_selection(app))
      app_set_status(app, "Selection copied.");
    else
      app_set_status(app, "Nothing to copy.");
    app->context_menu_open = false;
  }
  row_y += 22.0f;
  if (stygian_button(app->ctx, app->font, "Cut", x + 8.0f, row_y, w - 16.0f, 20.0f)) {
    if (app_cut_selection(app))
      app_set_status(app, "Selection cut.");
    else
      app_set_status(app, "Nothing to cut.");
    app->context_menu_open = false;
  }
  row_y += 22.0f;
  if (stygian_button(app->ctx, app->font, "Paste Here", x + 8.0f, row_y, w - 16.0f,
                     20.0f)) {
    if (app_paste_clipboard(app, app->context_menu_world_x, app->context_menu_world_y,
                            true))
      app_set_status(app, "Clipboard pasted.");
    else
      app_set_status(app, "Clipboard is empty.");
    app->context_menu_open = false;
  }
  row_y += 22.0f;
  if (stygian_button(app->ctx, app->font, "Duplicate", x + 8.0f, row_y, w - 16.0f,
                     20.0f)) {
    if (app_duplicate_selection(app, 24.0f, 24.0f))
      app_set_status(app, "Selection duplicated.");
    else
      app_set_status(app, "Nothing to duplicate.");
    app->context_menu_open = false;
  }
  row_y += 22.0f;
  if (stygian_button(app->ctx, app->font, "Delete", x + 8.0f, row_y, w - 16.0f,
                     20.0f)) {
    if (app_delete_selection(app))
      app_set_status(app, "Selection deleted.");
    else
      app_set_status(app, "Nothing to delete.");
    app->context_menu_open = false;
  }
  row_y += 26.0f;
  if (stygian_button(app->ctx, app->font, "Select All", x + 8.0f, row_y, w - 16.0f,
                     20.0f)) {
    if (app_select_all_nodes(app))
      app_set_status(app, "Selected all nodes.");
    else
      app_set_status(app, "Nothing to select.");
    app->context_menu_open = false;
  }
  row_y += 22.0f;
  if (stygian_button(app->ctx, app->font, "Clear Selection", x + 8.0f, row_y,
                     w - 16.0f, 20.0f)) {
    (void)stygian_editor_select_node(app->editor, STYGIAN_EDITOR_INVALID_ID, false);
    app_set_status(app, "Selection cleared.");
    app->context_menu_open = false;
  }
  row_y += 22.0f;
  if (stygian_button(app->ctx, app->font, "Reset Zoom", x + 8.0f, row_y, w - 16.0f,
                     20.0f)) {
    float focus_x = app->layout.viewport_x + app->layout.viewport_w * 0.5f;
    float focus_y = app->layout.viewport_y + app->layout.viewport_h * 0.5f;
    app_camera_zoom_about(app, focus_x, focus_y, 1.0f);
    app_set_status(app, "Zoom reset.");
    app->context_menu_open = false;
  }
}

#if 0
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

  if (app->font) {
    stygian_text(app->ctx, app->font, "Data Repeater", x, y, 15.0f, 0.93f, 0.95f,
                 1.0f, 1.0f);
  }
  y += 18.0f;
  if (app->font)
    stygian_text(app->ctx, app->font, "Count", x, y, 12.0f, 0.84f, 0.88f, 0.92f,
                 1.0f);
  stygian_text_input(app->ctx, app->font, x + 52.0f, y - 2.0f, 54.0f, 20.0f,
                     app->repeater_count, (int)sizeof(app->repeater_count));
  if (app->font)
    stygian_text(app->ctx, app->font, "Step X", x + 114.0f, y, 12.0f, 0.84f, 0.88f,
                 0.92f, 1.0f);
  stygian_text_input(app->ctx, app->font, x + 160.0f, y - 2.0f, 64.0f, 20.0f,
                     app->repeater_step_x, (int)sizeof(app->repeater_step_x));
  if (app->font)
    stygian_text(app->ctx, app->font, "Y", x + 230.0f, y, 12.0f, 0.84f, 0.88f, 0.92f,
                 1.0f);
  stygian_text_input(app->ctx, app->font, x + 242.0f, y - 2.0f, content_w - 242.0f,
                     20.0f, app->repeater_step_y,
                     (int)sizeof(app->repeater_step_y));
  y += 26.0f;
  if (stygian_button(app->ctx, app->font, "Apply Repeater", x, y, content_w, 22.0f)) {
    if (app_apply_repeater_from_selection(app))
      mutated = true;
  }
  y += 30.0f;

  if (app->font) {
    stygian_text(app->ctx, app->font, "SDF Boolean", x, y, 15.0f, 0.93f, 0.95f,
                 1.0f, 1.0f);
  }
  y += 16.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Union", x, y,
                             &app->boolean_op_choice, 0);
  y += 18.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Subtract", x, y,
                             &app->boolean_op_choice, 1);
  y += 18.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Intersect", x, y,
                             &app->boolean_op_choice, 2);
  y += 18.0f;
  (void)stygian_radio_button(app->ctx, app->font, "Exclude", x, y,
                             &app->boolean_op_choice, 3);
  y += 20.0f;
  (void)stygian_checkbox(app->ctx, app->font, "Flatten Result",
                         x, y, &app->boolean_flatten_after);
  y += 20.0f;
  (void)stygian_checkbox(app->ctx, app->font,
                         "Smooth Merge Intent (fallback diag)",
                         x, y, &app->boolean_request_smooth_fallback);
  y += 24.0f;
  if (stygian_button(app->ctx, app->font, "Compose Boolean", x, y, content_w,
                     22.0f)) {
    if (app_apply_boolean_from_selection(app))
      mutated = true;
  }
  y += 30.0f;

  if (app->font) {
    stygian_text(app->ctx, app->font, "Snap Hierarchy", x, y, 15.0f, 0.93f, 0.95f,
                 1.0f, 1.0f);
  }
  y += 18.0f;
  app_sync_snap_flags_from_editor(app);
  if (stygian_checkbox(app->ctx, app->font, "Grid", x, y, &app->snap_use_grid)) {
    app_push_snap_flags_to_editor(app);
    mutated = true;
  }
  y += 18.0f;
  if (stygian_checkbox(app->ctx, app->font, "Guides", x, y, &app->snap_use_guides)) {
    app_push_snap_flags_to_editor(app);
    mutated = true;
  }
  y += 18.0f;
  if (stygian_checkbox(app->ctx, app->font, "Bounds", x, y, &app->snap_use_bounds)) {
    app_push_snap_flags_to_editor(app);
    mutated = true;
  }
  y += 18.0f;
  if (stygian_checkbox(app->ctx, app->font, "Parent Edges", x, y,
                       &app->snap_use_parent_edges)) {
    app_push_snap_flags_to_editor(app);
    mutated = true;
  }
  y += 24.0f;

  {
    uint32_t track_count = stygian_editor_timeline_track_count(app->editor);
    uint32_t clip_count = stygian_editor_timeline_clip_count(app->editor);
    StygianEditorTimelineTrack selected_track = {0};
    StygianEditorTimelineClip selected_clip = {0};
    bool have_track = false;
    bool have_clip = false;
    if (app->timeline_selected_track_index >= track_count && track_count > 0u) {
      app->timeline_selected_track_index = track_count - 1u;
      app->timeline_selected_keyframe_index = 0u;
    }
    if (app->timeline_selected_clip_index >= clip_count && clip_count > 0u) {
      app->timeline_selected_clip_index = clip_count - 1u;
    }
    if (track_count > 0u) {
      have_track = stygian_editor_timeline_get_track(
          app->editor, app->timeline_selected_track_index, &selected_track);
      if (have_track &&
          app->timeline_selected_keyframe_index >= selected_track.keyframe_count &&
          selected_track.keyframe_count > 0u) {
        app->timeline_selected_keyframe_index = selected_track.keyframe_count - 1u;
      }
    }
    if (clip_count > 0u) {
      have_clip = stygian_editor_timeline_get_clip(
          app->editor, app->timeline_selected_clip_index, &selected_clip);
    }

    if (app->font) {
      stygian_text(app->ctx, app->font, "Timeline", x, y, 15.0f, 0.93f, 0.95f,
                   1.0f, 1.0f);
    }
    y += 18.0f;

    if (app->font)
      stygian_text(app->ctx, app->font, "Track Name", x, y, 12.0f, 0.84f, 0.88f,
                   0.92f, 1.0f);
    stygian_text_input(app->ctx, app->font, x + 68.0f, y - 2.0f, content_w - 68.0f,
                       20.0f, app->timeline_track_name,
                       (int)sizeof(app->timeline_track_name));
    y += 22.0f;

    if (app->font)
      stygian_text(app->ctx, app->font, "Target", x, y, 12.0f, 0.84f, 0.88f, 0.92f,
                   1.0f);
    stygian_text_input(app->ctx, app->font, x + 48.0f, y - 2.0f, 56.0f, 20.0f,
                       app->timeline_track_target_id,
                       (int)sizeof(app->timeline_track_target_id));
    if (stygian_button(app->ctx, app->font, "Use Selected", x + 110.0f, y - 2.0f,
                       94.0f, 20.0f)) {
      StygianEditorNodeId sel = stygian_editor_selected_node(app->editor);
      if (sel != STYGIAN_EDITOR_INVALID_ID) {
        snprintf(app->timeline_track_target_id,
                 sizeof(app->timeline_track_target_id), "%u", sel);
        mutated = true;
      }
    }
    if (app->font) {
      stygian_text(app->ctx, app->font,
                   app_timeline_property_label_from_choice(
                       app->timeline_track_property_choice),
                   x + 212.0f, y + 2.0f, 11.0f, 0.80f, 0.87f, 0.95f, 1.0f);
    }
    y += 22.0f;

    (void)stygian_radio_button(app->ctx, app->font, "X", x, y,
                               &app->timeline_track_property_choice, 0);
    (void)stygian_radio_button(app->ctx, app->font, "Y", x + 44.0f, y,
                               &app->timeline_track_property_choice, 1);
    (void)stygian_radio_button(app->ctx, app->font, "W", x + 88.0f, y,
                               &app->timeline_track_property_choice, 2);
    (void)stygian_radio_button(app->ctx, app->font, "H", x + 132.0f, y,
                               &app->timeline_track_property_choice, 3);
    (void)stygian_radio_button(app->ctx, app->font, "Opacity", x + 176.0f, y,
                               &app->timeline_track_property_choice, 4);
    y += 20.0f;

    if (stygian_button(app->ctx, app->font, "Add Track", x, y, 88.0f, 20.0f)) {
      StygianEditorTimelineTrack t = {0};
      uint32_t target_id = 0u;
      if (!app_parse_u32(app->timeline_track_target_id, &target_id) ||
          target_id == STYGIAN_EDITOR_INVALID_ID) {
        app_set_status(app, "Timeline track target id invalid.");
      } else {
        t.target_node = target_id;
        t.property =
            app_timeline_property_from_choice(app->timeline_track_property_choice);
        t.layer = 0u;
        snprintf(t.name, sizeof(t.name), "%s",
                 app->timeline_track_name[0] ? app->timeline_track_name
                                             : "timeline_track");
        if (stygian_editor_timeline_add_track(app->editor, &t, NULL)) {
          track_count = stygian_editor_timeline_track_count(app->editor);
          if (track_count > 0u)
            app->timeline_selected_track_index = track_count - 1u;
          app->timeline_selected_keyframe_index = 0u;
          app_set_status(app, "Timeline track added.");
          mutated = true;
        } else {
          app_set_status(app, "Timeline track add failed.");
        }
      }
    }
    if (stygian_button(app->ctx, app->font, "Delete Track", x + 96.0f, y, 96.0f,
                       20.0f)) {
      if (have_track &&
          stygian_editor_timeline_remove_track(app->editor, selected_track.id)) {
        track_count = stygian_editor_timeline_track_count(app->editor);
        if (track_count == 0u)
          app->timeline_selected_track_index = 0u;
        else if (app->timeline_selected_track_index >= track_count)
          app->timeline_selected_track_index = track_count - 1u;
        app->timeline_selected_keyframe_index = 0u;
        app_set_status(app, "Timeline track deleted.");
        mutated = true;
      }
    }
    y += 24.0f;

    if (app->font) {
      char track_hdr[96];
      snprintf(track_hdr, sizeof(track_hdr), "Tracks (%u)", track_count);
      stygian_text(app->ctx, app->font, track_hdr, x, y, 12.0f, 0.82f, 0.88f, 0.95f,
                   1.0f);
    }
    y += 16.0f;
    for (uint32_t i = 0u; i < track_count && i < 6u; ++i) {
      StygianEditorTimelineTrack t = {0};
      char label[80];
      if (!stygian_editor_timeline_get_track(app->editor, i, &t))
        continue;
      snprintf(label, sizeof(label), "#%u %s", t.id, t.name[0] ? t.name : "track");
      if (stygian_button(app->ctx, app->font, label, x, y, content_w, 18.0f)) {
        app->timeline_selected_track_index = i;
        app->timeline_selected_keyframe_index = 0u;
        mutated = true;
      }
      y += 20.0f;
    }

    if (have_track) {
      if (app->font) {
        char kf_hdr[128];
        snprintf(kf_hdr, sizeof(kf_hdr), "Keyframes: %u",
                 selected_track.keyframe_count);
        stygian_text(app->ctx, app->font, kf_hdr, x, y, 12.0f, 0.82f, 0.88f, 0.95f,
                     1.0f);
      }
      y += 16.0f;

      stygian_text_input(app->ctx, app->font, x, y, 58.0f, 20.0f,
                         app->timeline_cursor_ms,
                         (int)sizeof(app->timeline_cursor_ms));
      stygian_text_input(app->ctx, app->font, x + 64.0f, y, 72.0f, 20.0f,
                         app->timeline_key_value,
                         (int)sizeof(app->timeline_key_value));
      (void)stygian_radio_button(app->ctx, app->font, "Linear", x + 142.0f, y + 2.0f,
                                 &app->timeline_key_easing_choice, 0);
      (void)stygian_radio_button(app->ctx, app->font, "Out", x + 206.0f, y + 2.0f,
                                 &app->timeline_key_easing_choice, 1);
      (void)stygian_radio_button(app->ctx, app->font, "InOut", x + 252.0f, y + 2.0f,
                                 &app->timeline_key_easing_choice, 2);
      y += 22.0f;

      if (stygian_button(app->ctx, app->font, "Add Key", x, y, 72.0f, 18.0f)) {
        uint32_t t_ms = 0u;
        float v = 0.0f;
        if (!app_parse_u32_or_default(app->timeline_cursor_ms, 0u, &t_ms) ||
            !app_parse_float_text(app->timeline_key_value, &v)) {
          app_set_status(app, "Timeline key parse failed.");
        } else if (selected_track.keyframe_count <
                   STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP) {
          StygianEditorTimelineKeyframe
              keys[STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP];
          uint32_t n = selected_track.keyframe_count;
          memcpy(keys, selected_track.keyframes,
                 sizeof(StygianEditorTimelineKeyframe) * (size_t)n);
          keys[n].time_ms = t_ms;
          keys[n].value = v;
          keys[n].easing = app_easing_from_choice(app->timeline_key_easing_choice);
          if (stygian_editor_timeline_set_track_keyframes(app->editor,
                                                          selected_track.id, keys,
                                                          n + 1u)) {
            app_set_status(app, "Keyframe added.");
            mutated = true;
          }
        }
      }
      if (stygian_button(app->ctx, app->font, "Update", x + 78.0f, y, 70.0f,
                         18.0f)) {
        if (selected_track.keyframe_count > 0u &&
            app->timeline_selected_keyframe_index < selected_track.keyframe_count) {
          uint32_t t_ms = 0u;
          float v = 0.0f;
          if (!app_parse_u32_or_default(app->timeline_cursor_ms, 0u, &t_ms) ||
              !app_parse_float_text(app->timeline_key_value, &v)) {
            app_set_status(app, "Timeline key parse failed.");
          } else {
            StygianEditorTimelineKeyframe
                keys[STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP];
            memcpy(keys, selected_track.keyframes,
                   sizeof(StygianEditorTimelineKeyframe) *
                       (size_t)selected_track.keyframe_count);
            keys[app->timeline_selected_keyframe_index].time_ms = t_ms;
            keys[app->timeline_selected_keyframe_index].value = v;
            keys[app->timeline_selected_keyframe_index].easing =
                app_easing_from_choice(app->timeline_key_easing_choice);
            if (stygian_editor_timeline_set_track_keyframes(
                    app->editor, selected_track.id, keys,
                    selected_track.keyframe_count)) {
              app_set_status(app, "Keyframe updated.");
              mutated = true;
            }
          }
        }
      }
      if (stygian_button(app->ctx, app->font, "Delete", x + 154.0f, y, 70.0f,
                         18.0f)) {
        if (selected_track.keyframe_count > 0u &&
            app->timeline_selected_keyframe_index < selected_track.keyframe_count) {
          StygianEditorTimelineKeyframe keys[STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP];
          uint32_t n = 0u;
          for (uint32_t k = 0u; k < selected_track.keyframe_count; ++k) {
            if (k == app->timeline_selected_keyframe_index)
              continue;
            keys[n++] = selected_track.keyframes[k];
          }
          if (stygian_editor_timeline_set_track_keyframes(app->editor,
                                                          selected_track.id, keys,
                                                          n)) {
            if (app->timeline_selected_keyframe_index >= n && n > 0u)
              app->timeline_selected_keyframe_index = n - 1u;
            app_set_status(app, "Keyframe deleted.");
            mutated = true;
          }
        }
      }
      y += 22.0f;

      for (uint32_t k = 0u; k < selected_track.keyframe_count && k < 6u; ++k) {
        char label[92];
        snprintf(label, sizeof(label), "K%u t=%u v=%.2f", k,
                 selected_track.keyframes[k].time_ms,
                 selected_track.keyframes[k].value);
        if (stygian_button(app->ctx, app->font, label, x, y, content_w, 18.0f)) {
          app->timeline_selected_keyframe_index = k;
          snprintf(app->timeline_cursor_ms, sizeof(app->timeline_cursor_ms), "%u",
                   selected_track.keyframes[k].time_ms);
          snprintf(app->timeline_key_value, sizeof(app->timeline_key_value), "%.3f",
                   selected_track.keyframes[k].value);
          app->timeline_key_easing_choice =
              (int)selected_track.keyframes[k].easing;
          mutated = true;
        }
        y += 20.0f;
      }

      if (selected_track.keyframe_count >= 2u) {
        float graph_x = x;
        float graph_y = y;
        float graph_w = content_w;
        float graph_h = 54.0f;
        uint32_t k;
        float min_v = selected_track.keyframes[0].value;
        float max_v = min_v;
        uint32_t max_t =
            selected_track.keyframes[selected_track.keyframe_count - 1u].time_ms;
        stygian_rect(app->ctx, graph_x, graph_y, graph_w, graph_h, 0.06f, 0.08f,
                     0.11f, 1.0f);
        stygian_rect(app->ctx, graph_x + 1.0f, graph_y + 1.0f, graph_w - 2.0f,
                     graph_h - 2.0f, 0.08f, 0.11f, 0.15f, 1.0f);
        for (k = 1u; k < selected_track.keyframe_count; ++k) {
          if (selected_track.keyframes[k].value < min_v)
            min_v = selected_track.keyframes[k].value;
          if (selected_track.keyframes[k].value > max_v)
            max_v = selected_track.keyframes[k].value;
        }
        if (max_t == 0u)
          max_t = 1u;
        if (fabsf(max_v - min_v) < 0.0001f)
          max_v = min_v + 1.0f;
        for (k = 1u; k < selected_track.keyframe_count; ++k) {
          float x0 = graph_x + (graph_w - 8.0f) *
                                   ((float)selected_track.keyframes[k - 1u].time_ms /
                                    (float)max_t) +
                     4.0f;
          float x1 = graph_x +
                     (graph_w - 8.0f) *
                         ((float)selected_track.keyframes[k].time_ms /
                          (float)max_t) +
                     4.0f;
          float y0 = graph_y + graph_h - 4.0f -
                     (graph_h - 8.0f) *
                         ((selected_track.keyframes[k - 1u].value - min_v) /
                          (max_v - min_v));
          float y1 = graph_y + graph_h - 4.0f -
                     (graph_h - 8.0f) *
                         ((selected_track.keyframes[k].value - min_v) /
                          (max_v - min_v));
          stygian_line(app->ctx, x0, y0, x1, y1, 1.2f, 0.23f, 0.76f, 0.96f, 1.0f);
        }
        y += graph_h + 6.0f;
      }
    }

    if (app->font) {
      stygian_text(app->ctx, app->font, "Clips", x, y, 13.0f, 0.84f, 0.88f, 0.92f,
                   1.0f);
    }
    y += 16.0f;
    stygian_text_input(app->ctx, app->font, x, y, 88.0f, 20.0f,
                       app->timeline_clip_name,
                       (int)sizeof(app->timeline_clip_name));
    stygian_text_input(app->ctx, app->font, x + 94.0f, y, 52.0f, 20.0f,
                       app->timeline_clip_start_ms,
                       (int)sizeof(app->timeline_clip_start_ms));
    stygian_text_input(app->ctx, app->font, x + 150.0f, y, 56.0f, 20.0f,
                       app->timeline_clip_duration_ms,
                       (int)sizeof(app->timeline_clip_duration_ms));
    stygian_text_input(app->ctx, app->font, x + 210.0f, y, 44.0f, 20.0f,
                       app->timeline_clip_layer,
                       (int)sizeof(app->timeline_clip_layer));
    if (stygian_button(app->ctx, app->font, "Add", x + 258.0f, y, content_w - 258.0f,
                       20.0f)) {
      StygianEditorTimelineClip c = {0};
      uint32_t start_ms = 0u;
      uint32_t dur_ms = 0u;
      uint32_t layer = 0u;
      if (!app_parse_u32_or_default(app->timeline_clip_start_ms, 0u, &start_ms) ||
          !app_parse_u32_or_default(app->timeline_clip_duration_ms, 300u,
                                    &dur_ms) ||
          !app_parse_u32_or_default(app->timeline_clip_layer, 0u, &layer)) {
        app_set_status(app, "Timeline clip fields invalid.");
      } else {
        snprintf(c.name, sizeof(c.name), "%s",
                 app->timeline_clip_name[0] ? app->timeline_clip_name : "clip");
        c.start_ms = start_ms;
        c.duration_ms = dur_ms;
        c.layer = layer;
        if (stygian_editor_timeline_add_clip(app->editor, &c, NULL)) {
          clip_count = stygian_editor_timeline_clip_count(app->editor);
          if (clip_count > 0u)
            app->timeline_selected_clip_index = clip_count - 1u;
          app_set_status(app, "Timeline clip added.");
          mutated = true;
        }
      }
    }
    y += 22.0f;

    for (uint32_t i = 0u; i < clip_count && i < 4u; ++i) {
      StygianEditorTimelineClip c = {0};
      char label[84];
      if (!stygian_editor_timeline_get_clip(app->editor, i, &c))
        continue;
      snprintf(label, sizeof(label), "#%u %s L%u", c.id,
               c.name[0] ? c.name : "clip", c.layer);
      if (stygian_button(app->ctx, app->font, label, x, y, content_w, 18.0f)) {
        app->timeline_selected_clip_index = i;
        mutated = true;
      }
      y += 20.0f;
    }

    if (have_clip && have_track) {
      bool found = false;
      for (uint32_t i = 0u; i < selected_clip.track_count; ++i) {
        if (selected_clip.track_ids[i] == selected_track.id) {
          found = true;
          break;
        }
      }
      if (stygian_button(app->ctx, app->font,
                         found ? "Unassign Selected Track"
                               : "Assign Selected Track",
                         x, y, content_w, 20.0f)) {
        StygianEditorTimelineTrackId ids[STYGIAN_EDITOR_TIMELINE_CLIP_TRACK_CAP];
        uint32_t n = 0u;
        for (uint32_t i = 0u; i < selected_clip.track_count; ++i) {
          if (selected_clip.track_ids[i] == selected_track.id)
            continue;
          ids[n++] = selected_clip.track_ids[i];
        }
        if (!found && n < STYGIAN_EDITOR_TIMELINE_CLIP_TRACK_CAP)
          ids[n++] = selected_track.id;
        if (stygian_editor_timeline_set_clip_tracks(app->editor, selected_clip.id,
                                                    ids, n)) {
          app_set_status(app, found ? "Track unassigned from clip."
                                    : "Track assigned to clip.");
          mutated = true;
        }
      }
      y += 22.0f;
    }
    if (have_clip && stygian_button(app->ctx, app->font, "Delete Clip", x, y,
                                    content_w, 20.0f)) {
      if (stygian_editor_timeline_remove_clip(app->editor, selected_clip.id)) {
        clip_count = stygian_editor_timeline_clip_count(app->editor);
        if (clip_count == 0u)
          app->timeline_selected_clip_index = 0u;
        else if (app->timeline_selected_clip_index >= clip_count)
          app->timeline_selected_clip_index = clip_count - 1u;
        app_set_status(app, "Timeline clip deleted.");
        mutated = true;
      }
      y += 22.0f;
    }
  }

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

  y += 24.0f;
  if (app->font) {
    char perf_line[192];
    float d_load = app->perf_last_load_ms - APP_PERF_LOAD_BUDGET_MS;
    float d_layout = app->perf_last_layout_ms - APP_PERF_LAYOUT_BUDGET_MS;
    float d_export = app->perf_last_export_ms - APP_PERF_EXPORT_BUDGET_MS;
    float d_interaction = app->perf_last_interaction_ms - APP_PERF_INTERACTION_BUDGET_MS;
    stygian_text(app->ctx, app->font, "Performance Surface", x, y, 15.0f, 0.93f,
                 0.95f, 1.0f, 1.0f);
    y += 16.0f;
    snprintf(perf_line, sizeof(perf_line),
             "load %.1fms (d%.1f) | layout %.1fms (d%.1f)",
             app->perf_last_load_ms, d_load, app->perf_last_layout_ms, d_layout);
    stygian_text(app->ctx, app->font, perf_line, x, y, 11.0f, 0.80f, 0.86f, 0.93f,
                 1.0f);
    y += 14.0f;
    snprintf(perf_line, sizeof(perf_line),
             "export %.1fms (d%.1f) | interaction %.1fms (d%.1f)",
             app->perf_last_export_ms, d_export, app->perf_last_interaction_ms,
             d_interaction);
    stygian_text(app->ctx, app->font, perf_line, x, y, 11.0f, 0.80f, 0.86f, 0.93f,
                 1.0f);
  }

  return mutated;
}

#endif

static void app_draw_placement_preview(EditorBootstrapApp *app) {
  float mx;
  float my;
  float w = 140.0f;
  float h = 90.0f;
  float x;
  float y;
  float radius = 8.0f;
  const char *label = NULL;

  if (!app || !app->font)
    return;
  if (app->current_tool != APP_TOOL_RECT_FRAME &&
      app->current_tool != APP_TOOL_ELLIPSE)
    return;

  mx = (float)app->last_mouse_x;
  my = (float)app->last_mouse_y;
  if (!app_point_in_rect(mx, my, app->layout.viewport_x, app->layout.viewport_y,
                         app->layout.viewport_w, app->layout.viewport_h)) {
    return;
  }

  if (app->selected_primitive == APP_PRIMITIVE_SQUARE ||
      app->selected_primitive == APP_PRIMITIVE_CIRCLE) {
    w = 120.0f;
    h = 120.0f;
  }

  if (app->placing_primitive) {
    x = app->place_start_view_x < app->place_curr_view_x ? app->place_start_view_x
                                                         : app->place_curr_view_x;
    y = app->place_start_view_y < app->place_curr_view_y ? app->place_start_view_y
                                                         : app->place_curr_view_y;
    w = fabsf(app->place_curr_view_x - app->place_start_view_x);
    h = fabsf(app->place_curr_view_y - app->place_start_view_y);
  } else {
    x = mx - w * 0.5f;
    y = my - h * 0.5f;
  }
  if (app->current_tool == APP_TOOL_ELLIPSE ||
      app->selected_primitive == APP_PRIMITIVE_CIRCLE ||
      app->selected_primitive == APP_PRIMITIVE_ELLIPSE) {
    radius = (w < h ? w : h) * 0.5f;
    label = app->placing_primitive ? "Release to place ellipse"
                                   : "Click or drag to place ellipse";
  } else {
    label = app->placing_primitive ? "Release to place rect"
                                   : "Click or drag to place rect";
  }

  stygian_rect_rounded(app->ctx, x, y, w, h, 0.18f, 0.42f, 0.72f, 0.18f,
                       radius);
  stygian_line(app->ctx, x, y, x + w, y, 1.6f, 0.36f, 0.68f, 1.0f, 0.95f);
  stygian_line(app->ctx, x + w, y, x + w, y + h, 1.6f, 0.36f, 0.68f, 1.0f,
               0.95f);
  stygian_line(app->ctx, x + w, y + h, x, y + h, 1.6f, 0.36f, 0.68f, 1.0f,
               0.95f);
  stygian_line(app->ctx, x, y + h, x, y, 1.6f, 0.36f, 0.68f, 1.0f, 0.95f);
  stygian_text(app->ctx, app->font, label, x, y - 12.0f, 10.6f, 0.72f, 0.83f,
               0.95f, 1.0f);
}

static bool app_draw_viewport_text_overlay(EditorBootstrapApp *app) {
  static StygianTextArea text_area = {0};
  StygianEditorNodeId selected = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorShapeKind kind = STYGIAN_EDITOR_SHAPE_RECT;
  float wx = 0.0f, wy = 0.0f, ww = 0.0f, wh = 0.0f;
  float vx0 = 0.0f, vy0 = 0.0f, vx1 = 0.0f, vy1 = 0.0f;

  if (!app || !app->editor || !app->font ||
      stygian_editor_selected_count(app->editor) != 1u) {
    return false;
  }

  selected = stygian_editor_selected_node(app->editor);
  if (selected == STYGIAN_EDITOR_INVALID_ID ||
      !stygian_editor_node_get_shape_kind(app->editor, selected, &kind) ||
      kind != STYGIAN_EDITOR_SHAPE_TEXT ||
      !stygian_editor_node_get_bounds(app->editor, selected, &wx, &wy, &ww, &wh)) {
    return false;
  }

  app_sync_text_editor_buffer(app);
  stygian_editor_world_to_view(app->editor, wx, wy, &vx0, &vy0);
  stygian_editor_world_to_view(app->editor, wx + ww, wy + wh, &vx1, &vy1);

  if (vx1 < vx0) {
    float tmp = vx0;
    vx0 = vx1;
    vx1 = tmp;
  }
  if (vy1 < vy0) {
    float tmp = vy0;
    vy0 = vy1;
    vy1 = tmp;
  }
  if (vx1 - vx0 < 120.0f)
    vx1 = vx0 + 120.0f;
  if (vy1 - vy0 < 36.0f)
    vy1 = vy0 + 36.0f;

  if (text_area.buffer != app->text_edit_buffer) {
    memset(&text_area, 0, sizeof(text_area));
    text_area.buffer = app->text_edit_buffer;
    text_area.buffer_size = (int)sizeof(app->text_edit_buffer);
  }
  text_area.read_only = false;
  text_area.x = vx0;
  text_area.y = vy0;
  text_area.w = vx1 - vx0;
  text_area.h = vy1 - vy0;

  stygian_rect_rounded(app->ctx, vx0 - 2.0f, vy0 - 2.0f, text_area.w + 4.0f,
                       text_area.h + 4.0f, 0.10f, 0.14f, 0.21f, 0.78f, 6.0f);
  if (stygian_text_area(app->ctx, app->font, &text_area)) {
    if (stygian_editor_node_set_text_content(app->editor, selected,
                                             app->text_edit_buffer)) {
      return true;
    }
  }
  return false;
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
  app_draw_smart_guides(app);
  app_draw_placement_preview(app);
  app_draw_path_preview(app);
  app_draw_marquee_selection(app);
  app_draw_resize_handles(app);
  app_draw_rotation_handle(app);
  (void)app_draw_viewport_text_overlay(app);
  app_draw_context_menu(app);
  if (app->snap_feedback_active && app->font) {
    stygian_rect_rounded(app->ctx, app->snap_feedback_view_x - 4.0f,
                         app->snap_feedback_view_y - 2.0f, 120.0f, 16.0f, 0.09f,
                         0.12f, 0.17f, 0.94f, 5.0f);
    stygian_text(app->ctx, app->font, app->snap_feedback_text,
                 app->snap_feedback_view_x, app->snap_feedback_view_y, 10.0f,
                 0.83f, 0.91f, 0.99f, 1.0f);
  }
  stygian_clip_pop(app->ctx);

  return false;
}

static bool app_draw_behavior_graph_panel(EditorBootstrapApp *app, float panel_x,
                                          float panel_y, float panel_w,
                                          float panel_h) {
  uint32_t behavior_count = stygian_editor_behavior_count(app->editor);
  uint32_t i;
  uint32_t missing_node_count = 0u;
  uint32_t missing_hook_count = 0u;
  uint32_t connected_count = 0u;
  bool mutated = false;
  char hooks_source[32768];
  bool has_hook_set_variable = false;
  bool has_hook_navigate = false;
  float x = panel_x + 10.0f;
  float y = panel_y + 8.0f;
  float content_w = panel_w - 20.0f;
  float lane_h = 92.0f;

  app_draw_panel_shell(app, panel_x, panel_y, panel_w, panel_h, 0.28f, 0.55f,
                       0.39f);
  stygian_widgets_register_region(
      panel_x, panel_y, panel_w, panel_h,
      STYGIAN_WIDGET_REGION_POINTER_LEFT_MUTATES |
          STYGIAN_WIDGET_REGION_POINTER_RIGHT_MUTATES |
          STYGIAN_WIDGET_REGION_SCROLL);

  hooks_source[0] = '\0';
  (void)app_read_text_file(app_hook_source_path(), hooks_source,
                           sizeof(hooks_source));
  has_hook_set_variable = app_hook_function_exists(
      hooks_source, "stygian_editor_generated_hook_set_variable");
  has_hook_navigate =
      app_hook_function_exists(hooks_source, "stygian_editor_generated_hook_navigate");

  if (app->font) {
    char header[96];
    snprintf(header, sizeof(header), "Behavior Graph (%u rules)", behavior_count);
    stygian_text(app->ctx, app->font, header, x, y, 16.0f, 0.93f, 0.95f, 1.0f,
                 1.0f);
    stygian_text(app->ctx, app->font,
                 "Declarative rules first. Missing hooks stay visible instead of hiding.",
                 x, y + 16.0f, 10.8f, 0.74f, 0.82f, 0.90f, 1.0f);
  }
  y += 30.0f;

  app_draw_panel_card(app, x, y, content_w, 56.0f);
  app_draw_badge(app, x + 10.0f, y + 10.0f, 94.0f, 16.0f, 0.20f, 0.34f, 0.23f,
                 has_hook_set_variable || has_hook_navigate ? "Hooks found"
                                                            : "Hooks missing");
  stygian_text(app->ctx, app->font,
               app->behavior_graph_message[0] ? app->behavior_graph_message
                                              : "Use this to inspect event flow and fire rules on purpose.",
               x + 10.0f, y + 31.0f, 10.5f, 0.76f, 0.84f, 0.93f, 1.0f);
  if (stygian_button(app->ctx, app->font, "Open Hooks", x + content_w - 118.0f,
                     y + 24.0f, 52.0f, 20.0f)) {
    if (app_open_text_file_in_default_editor(app_hook_source_path()))
      app_set_status(app, "Opened hook source.");
    else
      app_set_status(app, "Failed to open hook source.");
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, "Export", x + content_w - 60.0f,
                     y + 24.0f, 50.0f, 20.0f)) {
    if (app_export_runtime_bundle(
            app, "editor/build/windows/stygian_export",
            "editor/build/windows/stygian_export/stygian_export_report.txt")) {
      app_set_status(app, "Exported runtime bundle.");
    } else {
      app_set_status(app, "Runtime bundle export failed.");
    }
    mutated = true;
  }
  y += 66.0f;

  if (behavior_count == 0u) {
    app_draw_panel_card(app, x, y, content_w, 48.0f);
    if (app->font) {
      stygian_text(
          app->ctx, app->font,
          "No rules yet. Add one in the Function Editor panel.", x + 10.0f,
          y + 12.0f, 13.0f, 0.80f, 0.84f, 0.92f, 1.0f);
      stygian_text(app->ctx, app->font,
                   "Static layout is fine. This panel only matters once behavior exists.",
                   x + 10.0f, y + 28.0f, 10.2f, 0.70f, 0.80f, 0.90f, 1.0f);
    }
    return mutated;
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
    char action_label[144];
    char easing_label[64];
    char button_label[64];
    char status_label[80];
    StygianEditorNodeId target_node = STYGIAN_EDITOR_INVALID_ID;
    bool missing_node = false;
    bool missing_hook = false;
    float action_r = 0.18f;
    float action_g = 0.35f;
    float action_b = 0.22f;
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

    if (rule.action_kind == STYGIAN_EDITOR_ACTION_ANIMATE)
      target_node = rule.animate.target_node;
    else if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY)
      target_node = rule.set_property.target_node;
    else if (rule.action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY)
      target_node = rule.toggle_visibility.target_node;
    else
      target_node = rule.trigger_node;

    if (!app_node_exists(app, rule.trigger_node) || !app_node_exists(app, target_node))
      missing_node = true;
    if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_VARIABLE &&
        !has_hook_set_variable)
      missing_hook = true;
    if (rule.action_kind == STYGIAN_EDITOR_ACTION_NAVIGATE && !has_hook_navigate)
      missing_hook = true;

    if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY) {
      action_r = 0.24f;
      action_g = 0.31f;
      action_b = 0.56f;
    } else if (rule.action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY) {
      action_r = 0.44f;
      action_g = 0.33f;
      action_b = 0.16f;
    } else if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_VARIABLE) {
      action_r = 0.18f;
      action_g = 0.45f;
      action_b = 0.45f;
    } else if (rule.action_kind == STYGIAN_EDITOR_ACTION_NAVIGATE) {
      action_r = 0.44f;
      action_g = 0.23f;
      action_b = 0.49f;
    }

    if (missing_node)
      missing_node_count += 1u;
    if (missing_hook)
      missing_hook_count += 1u;
    if (!missing_node && !missing_hook)
      connected_count += 1u;

    app_draw_panel_card(app, x, row_top, content_w, lane_h - 8.0f);

    stygian_rect_rounded(app->ctx, left_x, row_top + 8.0f, node_w, node_h, 0.13f,
                         0.29f, 0.48f, 0.98f, 8.0f);
    stygian_rect_rounded(app->ctx, right_x, row_top + 8.0f, node_w, node_h, action_r,
                         action_g, action_b, 0.98f, 8.0f);

    stygian_line(app->ctx, left_x + node_w, center_y, right_x, center_y, 2.0f,
                 0.42f, 0.72f, 0.96f, 1.0f);

    snprintf(trigger_label, sizeof(trigger_label), "N%u %s", rule.trigger_node,
             app_event_kind_name(rule.trigger_event));
    if (rule.action_kind == STYGIAN_EDITOR_ACTION_ANIMATE) {
      snprintf(action_label, sizeof(action_label), "N%u Animate %s -> %.2f",
               rule.animate.target_node,
               app_property_kind_name(rule.animate.property), rule.animate.to_value);
      snprintf(easing_label, sizeof(easing_label), "%ums %s",
               rule.animate.duration_ms, app_easing_name(rule.animate.easing));
    } else if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY) {
      snprintf(action_label, sizeof(action_label), "N%u Set %s = %.2f",
               rule.set_property.target_node,
               app_property_kind_name(rule.set_property.property),
               rule.set_property.value);
      snprintf(easing_label, sizeof(easing_label), "Immediate property set");
    } else if (rule.action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY) {
      snprintf(action_label, sizeof(action_label), "N%u Toggle Visibility",
               rule.toggle_visibility.target_node);
      snprintf(easing_label, sizeof(easing_label), "Immediate visibility flip");
    } else if (rule.action_kind == STYGIAN_EDITOR_ACTION_SET_VARIABLE) {
      snprintf(action_label, sizeof(action_label), "Set Variable %s",
               rule.set_variable.variable_name);
      snprintf(easing_label, sizeof(easing_label), "Hook: set_variable");
    } else if (rule.action_kind == STYGIAN_EDITOR_ACTION_NAVIGATE) {
      snprintf(action_label, sizeof(action_label), "Navigate -> %s",
               rule.navigate.target);
      snprintf(easing_label, sizeof(easing_label), "Hook: navigate");
    } else {
      snprintf(action_label, sizeof(action_label), "Unknown action");
      snprintf(easing_label, sizeof(easing_label), "Unsupported");
    }
    if (missing_node && missing_hook)
      snprintf(status_label, sizeof(status_label), "Status: missing-node + missing-hook");
    else if (missing_node)
      snprintf(status_label, sizeof(status_label), "Status: missing-node");
    else if (missing_hook)
      snprintf(status_label, sizeof(status_label), "Status: missing-hook");
    else
      snprintf(status_label, sizeof(status_label), "Status: connected");

    if (app->font) {
      stygian_text(app->ctx, app->font, trigger_label, left_x + 8.0f,
                   row_top + 14.0f, 12.5f, 0.90f, 0.95f, 1.0f, 1.0f);
      stygian_text(app->ctx, app->font, action_label, right_x + 8.0f,
                   row_top + 14.0f, 12.5f, 0.91f, 1.0f, 0.91f, 1.0f);
      stygian_text(app->ctx, app->font, easing_label, right_x + 8.0f,
                   row_top + 30.0f, 11.5f, 0.83f, 0.95f, 0.87f, 1.0f);
      stygian_text(app->ctx, app->font, status_label, left_x + 8.0f, row_top + 62.0f,
                   11.0f, missing_node || missing_hook ? 0.96f : 0.74f,
                   missing_node || missing_hook ? 0.68f : 0.90f,
                   missing_node || missing_hook ? 0.70f : 0.96f, 1.0f);
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

  if (app->font) {
    char summary[144];
    snprintf(summary, sizeof(summary), "Connected %u | Missing node %u | Missing hook %u",
             connected_count, missing_node_count, missing_hook_count);
    stygian_text(app->ctx, app->font, summary, x, panel_y + panel_h - 34.0f, 11.0f,
                 0.78f, 0.86f, 0.95f, 1.0f);
  }
  if (app->behavior_graph_message[0] && app->font) {
    stygian_text(app->ctx, app->font, app->behavior_graph_message, x,
                 panel_y + panel_h - 18.0f, 11.5f, 0.72f, 0.87f, 0.96f, 1.0f);
  }

  return mutated;
}

#if 0
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
#endif

#if 0
static void app_draw_simple_panel_note(EditorBootstrapApp *app, float panel_x,
                                       float panel_y, float panel_w,
                                       float panel_h, const char *title,
                                       const char *line_a, const char *line_b) {
  app_draw_panel_shell(app, panel_x, panel_y, panel_w, panel_h, 0.39f, 0.56f,
                       0.83f);
  app_draw_panel_card(app, panel_x + 10.0f, panel_y + 34.0f, panel_w - 20.0f,
                      54.0f);
  if (!app->font)
    return;
  if (title)
    stygian_text(app->ctx, app->font, title, panel_x + 10.0f, panel_y + 8.0f,
                 16.0f, 0.93f, 0.95f, 1.0f, 1.0f);
  if (line_a)
    stygian_text(app->ctx, app->font, line_a, panel_x + 20.0f, panel_y + 46.0f,
                 12.0f, 0.80f, 0.86f, 0.94f, 1.0f);
  if (line_b)
    stygian_text(app->ctx, app->font, line_b, panel_x + 20.0f, panel_y + 62.0f,
                 11.0f, 0.74f, 0.82f, 0.92f, 1.0f);
}
#endif

static bool app_draw_assets_panel(EditorBootstrapApp *app, float panel_x,
                                  float panel_y, float panel_w, float panel_h) {
  float x = panel_x + 10.0f;
  float y = panel_y + 8.0f;
  float w = panel_w - 20.0f;
  bool mutated = false;
  float half_w;
  if (!app || !app->ctx || !app->editor || !app->font)
    return false;
  app_sync_project_drafts(app);

  app_draw_panel_shell(app, panel_x, panel_y, panel_w, panel_h, 0.24f, 0.55f,
                       0.54f);
  stygian_widgets_register_region(
      panel_x, panel_y, panel_w, panel_h,
      STYGIAN_WIDGET_REGION_POINTER_LEFT_MUTATES |
          STYGIAN_WIDGET_REGION_POINTER_RIGHT_MUTATES |
          STYGIAN_WIDGET_REGION_SCROLL);

  stygian_text(app->ctx, app->font, "Insert", x, y, 16.0f, 0.93f, 0.95f,
               1.0f, 1.0f);
  stygian_text(app->ctx, app->font,
               "Starter structures and export actions live here. No fake filler.",
               x, y + 16.0f, 10.8f, 0.72f, 0.80f, 0.90f, 1.0f);
  y += 30.0f;

  half_w = (w - 28.0f) * 0.5f;
  app_draw_panel_card(app, x, y, w, 158.0f);
  stygian_text(app->ctx, app->font, "Scene Starters", x + 10.0f, y + 10.0f, 12.5f,
               0.90f, 0.94f, 1.0f, 1.0f);
  stygian_text(app->ctx, app->font, "Drop editable shells into the viewport.",
               x + 10.0f, y + 24.0f, 10.0f, 0.66f, 0.74f, 0.84f, 1.0f);
  if (stygian_button(app->ctx, app->font, "Tabs", x + 10.0f, y + 32.0f, half_w,
                     22.0f)) {
    float wx = 0.0f;
    float wy = 0.0f;
    stygian_editor_view_to_world(app->editor,
                                 app->layout.viewport_x +
                                     app->layout.viewport_w * 0.5f - 150.0f,
                                 app->layout.viewport_y + 90.0f, &wx, &wy);
    if (app_add_tabs_scaffold(app, wx, wy))
      app_set_status(app, "Tabs scaffold added.");
    else
      app_set_status(app, "Tabs scaffold add failed.");
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, "Sidebar", x + 18.0f + half_w,
                     y + 32.0f, half_w, 22.0f)) {
    float wx = 0.0f;
    float wy = 0.0f;
    stygian_editor_view_to_world(app->editor, app->layout.viewport_x + 100.0f,
                                 app->layout.viewport_y + 96.0f, &wx, &wy);
    if (app_add_sidebar_template(app, wx, wy))
      app_set_status(app, "Sidebar template added.");
    else
      app_set_status(app, "Sidebar template failed.");
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, "Modal", x + 10.0f, y + 58.0f, half_w,
                     22.0f)) {
    float wx = 0.0f;
    float wy = 0.0f;
    stygian_editor_view_to_world(app->editor, app->layout.viewport_x + 130.0f,
                                 app->layout.viewport_y + 110.0f, &wx, &wy);
    if (app_add_modal_template(app, wx, wy))
      app_set_status(app, "Modal template added.");
    else
      app_set_status(app, "Modal template failed.");
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, "Settings", x + 18.0f + half_w,
                     y + 58.0f, half_w, 22.0f)) {
    float wx = 0.0f;
    float wy = 0.0f;
    stygian_editor_view_to_world(app->editor, app->layout.viewport_x + 160.0f,
                                 app->layout.viewport_y + 130.0f, &wx, &wy);
    if (app_add_settings_template(app, wx, wy))
      app_set_status(app, "Settings template added.");
    else
      app_set_status(app, "Settings template failed.");
    mutated = true;
  }
  stygian_text(app->ctx, app->font, "Content", x + 10.0f, y + 92.0f, 12.0f,
               0.88f, 0.92f, 0.98f, 1.0f);
  if (stygian_button(app->ctx, app->font, "Text", x + 10.0f, y + 112.0f, half_w,
                     22.0f)) {
    float wx = 0.0f, wy = 0.0f;
    StygianEditorNodeId id;
    stygian_editor_view_to_world(app->editor,
                                 app->layout.viewport_x + app->layout.viewport_w * 0.5f - 120.0f,
                                 app->layout.viewport_y + app->layout.viewport_h * 0.35f,
                                 &wx, &wy);
    id = app_add_text_node(app, wx, wy, "Text");
    if (id != STYGIAN_EDITOR_INVALID_ID) {
      (void)stygian_editor_select_node(app->editor, id, false);
      app_sync_selected_name(app);
      app_sync_text_editor_buffer(app);
      app_set_status(app, "Text node added.");
      mutated = true;
    }
  }
  if (stygian_button(app->ctx, app->font, "Image", x + 18.0f + half_w,
                     y + 112.0f, half_w, 22.0f)) {
    float wx = 0.0f, wy = 0.0f;
    StygianEditorNodeId id;
    stygian_editor_view_to_world(app->editor,
                                 app->layout.viewport_x + app->layout.viewport_w * 0.5f - 110.0f,
                                 app->layout.viewport_y + app->layout.viewport_h * 0.42f,
                                 &wx, &wy);
    id = app_add_image_node(app, wx, wy, app->import_image_source);
    if (id != STYGIAN_EDITOR_INVALID_ID) {
      (void)stygian_editor_select_node(app->editor, id, false);
      app_sync_selected_name(app);
      app_set_status(app, "Image node added.");
      mutated = true;
    }
  }
  if (stygian_button(app->ctx, app->font, "Component", x + 10.0f, y + 138.0f,
                     half_w, 22.0f)) {
    float wx = 0.0f, wy = 0.0f;
    StygianEditorNodeId id;
    stygian_editor_view_to_world(app->editor,
                                 app->layout.viewport_x +
                                     app->layout.viewport_w * 0.5f - 140.0f,
                                 app->layout.viewport_y +
                                     app->layout.viewport_h * 0.50f,
                                 &wx, &wy);
    id = app_add_component_def_node(app, wx, wy, NULL);
    if (id != STYGIAN_EDITOR_INVALID_ID) {
      (void)stygian_editor_select_node(app->editor, id, false);
      app_sync_selected_name(app);
      app_sync_component_editor_buffer(app);
      app_set_status(app, "Component definition added.");
      mutated = true;
    }
  }
  if (stygian_button(app->ctx, app->font, "Instance", x + 18.0f + half_w,
                     y + 138.0f, half_w, 22.0f)) {
    float wx = 0.0f, wy = 0.0f;
    StygianEditorNodeId def_id = app_component_def_for_selection(app);
    StygianEditorNodeId id = STYGIAN_EDITOR_INVALID_ID;
    if (def_id != STYGIAN_EDITOR_INVALID_ID) {
      stygian_editor_view_to_world(app->editor,
                                   app->layout.viewport_x +
                                       app->layout.viewport_w * 0.5f + 40.0f,
                                   app->layout.viewport_y +
                                       app->layout.viewport_h * 0.50f,
                                   &wx, &wy);
      id = app_add_component_instance_node(app, def_id, wx, wy);
    }
    if (id != STYGIAN_EDITOR_INVALID_ID) {
      (void)stygian_editor_select_node(app->editor, id, false);
      app_sync_selected_name(app);
      app_sync_component_editor_buffer(app);
      app_set_status(app, "Component instance added.");
      mutated = true;
    } else {
      app_set_status(app, "Select a component definition or instance first.");
    }
  }
  if (stygian_button(app->ctx, app->font, "Tabs Family", x + 10.0f, y + 164.0f,
                     half_w, 22.0f)) {
    float wx = 0.0f, wy = 0.0f;
    stygian_editor_view_to_world(app->editor,
                                 app->layout.viewport_x + 80.0f,
                                 app->layout.viewport_y +
                                     app->layout.viewport_h * 0.58f,
                                 &wx, &wy);
    if (app_seed_component_variant_family(app, wx, wy, true)) {
      app_set_status(app, "Tab variant family seeded.");
      mutated = true;
    } else {
      app_set_status(app, "Tab variant family seed failed.");
    }
  }
  if (stygian_button(app->ctx, app->font, "Toggle Family",
                     x + 18.0f + half_w, y + 164.0f, half_w, 22.0f)) {
    float wx = 0.0f, wy = 0.0f;
    stygian_editor_view_to_world(app->editor,
                                 app->layout.viewport_x + 120.0f,
                                 app->layout.viewport_y +
                                     app->layout.viewport_h * 0.66f,
                                 &wx, &wy);
    if (app_seed_component_variant_family(app, wx, wy, false)) {
      app_set_status(app, "Toggle variant family seeded.");
      mutated = true;
    } else {
      app_set_status(app, "Toggle variant family seed failed.");
    }
  }
  stygian_text_input(app->ctx, app->font, x + 10.0f, y + 190.0f, w - 20.0f, 20.0f,
                     app->import_image_source, (int)sizeof(app->import_image_source));
  y += 218.0f;

  app_draw_panel_card(app, x, y, w, 82.0f);
  stygian_text(app->ctx, app->font, "Project", x + 10.0f, y + 10.0f, 12.5f,
               0.90f, 0.94f, 1.0f, 1.0f);
  stygian_text(app->ctx, app->font,
               app->project_path_draft[0] ? app->project_path_draft : "In-memory session",
               x + 10.0f, y + 27.0f, 10.0f, 0.70f, 0.79f, 0.89f, 1.0f);
  if (stygian_button(app->ctx, app->font, "Save", x + 10.0f, y + 48.0f, 52.0f,
                     22.0f)) {
    if (app_save_project(app))
      app_set_status(app, "Saved project.");
    else
      app_set_status(app, "Project save failed.");
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, "Load", x + 68.0f, y + 48.0f, 52.0f,
                     22.0f)) {
    if (app_load_project(app))
      app_set_status(app, "Loaded project.");
    else
      app_set_status(app, "Project load failed.");
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, "Export", x + 126.0f, y + 48.0f,
                     64.0f, 22.0f)) {
    if (app_export_runtime_bundle(
            app, "editor/build/windows/stygian_export",
            "editor/build/windows/stygian_export/stygian_export_report.txt")) {
      app_set_status(app, "Exported runtime bundle.");
    } else {
      app_set_status(app, "Runtime bundle export failed.");
    }
    mutated = true;
  }
  y += 90.0f;

  app_draw_panel_card(app, x, y, w, 80.0f);
  stygian_text(app->ctx, app->font, "Ownership", x + 10.0f, y + 10.0f,
               12.5f, 0.90f, 0.94f, 1.0f, 1.0f);
  app_draw_badge(app, x + 10.0f, y + 30.0f, 114.0f, 16.0f, 0.19f, 0.28f, 0.42f,
                 "Generated");
  app_draw_badge(app, x + 130.0f, y + 30.0f, 92.0f, 16.0f, 0.20f, 0.34f, 0.23f,
                 "Hooks");
  stygian_text(app->ctx, app->font,
               "Generated code gets replaced. Hook code does not.", x + 10.0f,
               y + 49.0f, 10.0f, 0.68f, 0.76f, 0.86f, 1.0f);
  if (stygian_button(app->ctx, app->font, "Open Gen", x + 10.0f, y + 58.0f,
                     92.0f, 20.0f)) {
    if (app_open_text_file_in_default_editor(app_generated_scene_path()))
      app_set_status(app, "Opened generated code.");
    else
      app_set_status(app, "Failed to open generated code.");
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, "Open Hooks", x + 108.0f, y + 58.0f,
                     100.0f, 20.0f)) {
    if (app_open_text_file_in_default_editor(app_hook_source_path()))
      app_set_status(app, "Opened hook source.");
    else
      app_set_status(app, "Failed to open hook source.");
    mutated = true;
  }
  return mutated;
}

static const char *app_scene_kind_label(StygianEditorShapeKind kind) {
  switch (kind) {
  case STYGIAN_EDITOR_SHAPE_RECT:
    return "Rect";
  case STYGIAN_EDITOR_SHAPE_ELLIPSE:
    return "Ellipse";
  case STYGIAN_EDITOR_SHAPE_FRAME:
    return "Frame";
  case STYGIAN_EDITOR_SHAPE_TEXT:
    return "Text";
  case STYGIAN_EDITOR_SHAPE_PATH:
    return "Path";
  case STYGIAN_EDITOR_SHAPE_IMAGE:
    return "Image";
  case STYGIAN_EDITOR_SHAPE_COMPONENT_DEF:
    return "Component";
  case STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE:
    return "Instance";
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
  default:
    return "Node";
  }
}

static void app_draw_layer_tree_rows(EditorBootstrapApp *app,
                                     StygianEditorNodeId parent_id, uint32_t depth,
                                     float x, float *io_y, float w,
                                     int *io_rows_left, bool *io_mutated) {
  StygianEditorNodeId children[64];
  uint32_t child_count;
  if (!app || !io_y || !io_rows_left || *io_rows_left <= 0)
    return;
  child_count = stygian_editor_tree_list_children(app->editor, parent_id, children,
                                                  (uint32_t)(sizeof(children) / sizeof(children[0])));
  for (uint32_t i = 0u; i < child_count && *io_rows_left > 0; ++i) {
    StygianEditorNodeId id = children[i];
    StygianEditorShapeKind kind = STYGIAN_EDITOR_SHAPE_RECT;
    StygianEditorNodeId component_def_id = STYGIAN_EDITOR_INVALID_ID;
    bool visible = true;
    bool locked = false;
    bool is_selected = stygian_editor_node_is_selected(app->editor, id);
    char row[96];
    char name[32];
    char meta[64];
    float row_h = 28.0f;
    float indent = 14.0f * (float)depth;
    float y = *io_y;
    (void)stygian_editor_node_get_shape_kind(app->editor, id, &kind);
    (void)stygian_editor_node_get_visible(app->editor, id, &visible);
    (void)stygian_editor_node_get_locked(app->editor, id, &locked);
    if (!stygian_editor_node_get_name(app->editor, id, name, sizeof(name)) || !name[0])
      snprintf(name, sizeof(name), "%s", app_scene_kind_label(kind));

    app_draw_panel_card(app, x, y, w, row_h);
    if (is_selected) {
      stygian_rect_rounded(app->ctx, x + 2.0f, y + 2.0f, w - 4.0f, row_h - 4.0f,
                           0.15f, 0.22f, 0.31f, 0.72f, 5.0f);
      stygian_rect(app->ctx, x + 1.0f, y + 5.0f, 3.0f, row_h - 10.0f, 0.43f, 0.67f,
                   1.0f, 0.95f);
    }
    if (stygian_button(app->ctx, app->font, NULL, x + 6.0f + indent, y + 3.0f,
                       w - 150.0f - indent, row_h - 6.0f)) {
      (void)stygian_editor_select_node(app->editor, id, false);
      app_sync_selected_name(app);
      app_sync_text_editor_buffer(app);
      *io_mutated = true;
    }
    snprintf(row, sizeof(row), "#%u %s", id, name);
    stygian_text(app->ctx, app->font, row, x + 12.0f + indent, y + 6.0f, 11.2f,
                 0.92f, 0.95f, 0.98f, 1.0f);
    meta[0] = '\0';
    if (kind == STYGIAN_EDITOR_SHAPE_COMPONENT_DEF) {
      char group[32];
      char variant[32];
      group[0] = '\0';
      variant[0] = '\0';
      if (stygian_editor_component_def_get_variant(app->editor, id, group,
                                                   sizeof(group), variant,
                                                   sizeof(variant)) &&
          group[0]) {
        snprintf(meta, sizeof(meta), "%s:%s", group, variant[0] ? variant : "-");
      } else {
        snprintf(meta, sizeof(meta), "def");
      }
    } else if (kind == STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE &&
               stygian_editor_component_instance_get_def(app->editor, id,
                                                         &component_def_id)) {
      char symbol[64];
      symbol[0] = '\0';
      if (stygian_editor_component_def_get_symbol(app->editor, component_def_id,
                                                  symbol, sizeof(symbol)) &&
          symbol[0]) {
        snprintf(meta, sizeof(meta), "->%s", symbol);
      }
    }
    stygian_text(app->ctx, app->font, meta[0] ? meta : app_scene_kind_label(kind),
                 x + w - 142.0f, y + 6.0f, 10.0f, 0.67f, 0.76f, 0.88f, 1.0f);
    if (stygian_button(app->ctx, app->font, visible ? "Show" : "Hide",
                       x + w - 106.0f, y + 3.0f, 42.0f, row_h - 6.0f)) {
      (void)stygian_editor_node_set_visible(app->editor, id, !visible);
      *io_mutated = true;
    }
    if (stygian_button(app->ctx, app->font, locked ? "Lock" : "Edit",
                       x + w - 58.0f, y + 3.0f, 44.0f, row_h - 6.0f)) {
      (void)stygian_editor_node_set_locked(app->editor, id, !locked);
      *io_mutated = true;
    }
    if (is_selected) {
      if (stygian_button(app->ctx, app->font, "^", x + w - 150.0f, y + 3.0f,
                         14.0f, row_h - 14.0f)) {
        if (i > 0u) {
          (void)stygian_editor_tree_reorder_child(app->editor, id, i - 1u);
          *io_mutated = true;
        }
      }
      if (stygian_button(app->ctx, app->font, "v", x + w - 150.0f,
                         y + row_h - 12.0f, 12.0f, 9.0f)) {
        (void)stygian_editor_tree_reorder_child(app->editor, id, i + 1u);
        *io_mutated = true;
      }
      if (stygian_button(app->ctx, app->font, "Focus", x + w - 196.0f, y + 3.0f,
                         40.0f, row_h - 6.0f)) {
        app->scene_parent_focus = id;
        *io_mutated = true;
      }
    }

    *io_y += row_h + 4.0f;
    *io_rows_left -= 1;
    app_draw_layer_tree_rows(app, id, depth + 1u, x, io_y, w, io_rows_left,
                             io_mutated);
  }
}

static bool app_draw_scene_layers_panel(EditorBootstrapApp *app, float panel_x,
                                        float panel_y, float panel_w,
                                        float panel_h) {
  StygianEditorNodeId selected = stygian_editor_selected_node(app->editor);
  StygianEditorNodeId selected_parent = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId focused = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorShapeKind selected_kind = STYGIAN_EDITOR_SHAPE_RECT;
  int rows_left = 18;
  uint32_t selected_count = 0u;
  float x = panel_x + 10.0f;
  float y = panel_y + 8.0f;
  float w = panel_w - 20.0f;
  bool mutated = false;
  char header[128];
  char path[192];

  app_draw_panel_shell(app, panel_x, panel_y, panel_w, panel_h, 0.28f, 0.44f,
                       0.68f);
  stygian_widgets_register_region(
      panel_x, panel_y, panel_w, panel_h,
      STYGIAN_WIDGET_REGION_POINTER_LEFT_MUTATES |
          STYGIAN_WIDGET_REGION_POINTER_RIGHT_MUTATES |
          STYGIAN_WIDGET_REGION_SCROLL);

  if (!app->font)
    return false;

  stygian_text(app->ctx, app->font, "Layers", x, y, 16.0f, 0.93f, 0.95f,
               1.0f, 1.0f);
  selected_count = app_selected_nodes(app, app->selection_ids_cache,
                                      APP_MAX_TRANSFORM_SELECTION);
  app_sync_selected_name(app);
  snprintf(header, sizeof(header), "%u selected | focus %s", selected_count,
           app->scene_parent_focus == STYGIAN_EDITOR_INVALID_ID ? "root"
                                                                : "subtree");
  stygian_text(app->ctx, app->font, header, x, y + 16.0f, 10.8f, 0.70f, 0.79f,
               0.89f, 1.0f);
  y += 30.0f;

  if (selected != STYGIAN_EDITOR_INVALID_ID &&
      stygian_editor_node_get_parent(app->editor, selected, &selected_parent)) {
    if (app->scene_parent_focus == STYGIAN_EDITOR_INVALID_ID)
      app->scene_parent_focus = selected_parent;
  }
  focused = app->scene_parent_focus;
  if (selected != STYGIAN_EDITOR_INVALID_ID)
    (void)stygian_editor_node_get_shape_kind(app->editor, selected, &selected_kind);

  app_draw_panel_card(app, x, y, w, 62.0f);
  app_node_path(app,
                selected != STYGIAN_EDITOR_INVALID_ID ? selected
                                                      : app->scene_parent_focus,
                path, sizeof(path));
  snprintf(header, sizeof(header), "Branch view | %s", path);
  stygian_text(app->ctx, app->font, header, x + 10.0f, y + 9.0f, 11.0f, 0.81f,
               0.87f, 0.95f, 1.0f);
  stygian_text(app->ctx, app->font, "Visibility, lock, order, and focus live on the rows.",
               x + 10.0f, y + 24.0f, 10.0f, 0.64f, 0.72f, 0.83f, 1.0f);

  if (stygian_button(app->ctx, app->font, "Root", x + 10.0f, y + 38.0f, 68.0f,
                     20.0f)) {
    app->scene_parent_focus = STYGIAN_EDITOR_INVALID_ID;
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, "Focus Parent", x + 84.0f, y + 38.0f,
                     104.0f, 20.0f)) {
    if (selected != STYGIAN_EDITOR_INVALID_ID &&
        stygian_editor_node_get_parent(app->editor, selected, &selected_parent)) {
      app->scene_parent_focus = selected_parent;
      mutated = true;
    }
  }
  y += 72.0f;

  if (selected != STYGIAN_EDITOR_INVALID_ID) {
    app_draw_panel_card(app, x, y, w, 78.0f);
    snprintf(header, sizeof(header), "Selected #%u %s", selected,
             app_scene_kind_label(selected_kind));
    stygian_text(app->ctx, app->font, header, x + 10.0f, y + 10.0f, 12.0f,
                 0.90f, 0.94f, 1.0f, 1.0f);
    stygian_text_input(app->ctx, app->font, x + 10.0f, y + 30.0f, w - 20.0f,
                       20.0f, app->selected_name_text,
                       (int)sizeof(app->selected_name_text));
    if (stygian_button(app->ctx, app->font, "Rename", x + 10.0f, y + 54.0f,
                       64.0f, 20.0f)) {
      if (stygian_editor_node_set_name(app->editor, selected,
                                       app->selected_name_text)) {
        app_set_status(app, "Node renamed.");
        mutated = true;
      }
    }
    if (stygian_button(app->ctx, app->font, "Focus", x + 80.0f, y + 54.0f,
                       60.0f, 20.0f)) {
      app->scene_parent_focus = selected;
      mutated = true;
    }
    y += 86.0f;
  }

  app_draw_panel_card(app, x, y, w, panel_h - (y - panel_y) - 12.0f);
  y += 8.0f;
  if (stygian_editor_tree_list_children(app->editor, focused, app->selection_ids_cache, 1u) == 0u &&
      focused == STYGIAN_EDITOR_INVALID_ID &&
      stygian_editor_node_count(app->editor) == 0u) {
    stygian_text(app->ctx, app->font, "No scene content yet.", x + 10.0f, y + 10.0f,
                 11.5f, 0.78f, 0.84f, 0.93f, 1.0f);
    stygian_text(app->ctx, app->font,
                 "Drop a shape in the viewport or seed a template.",
                 x + 10.0f, y + 26.0f, 10.5f, 0.64f, 0.72f, 0.83f, 1.0f);
    y += 56.0f;
  } else {
    app_draw_layer_tree_rows(app, focused, 0u, x + 6.0f, &y, w - 12.0f, &rows_left,
                             &mutated);
  }

  if (selected != STYGIAN_EDITOR_INVALID_ID) {
    y += 4.0f;
    if (stygian_button(app->ctx, app->font, "Reparent Selected To Focus", x, y, w,
                       24.0f)) {
      (void)stygian_editor_reparent_node(app->editor, selected,
                                         app->scene_parent_focus, true);
      mutated = true;
    }
  }
  return mutated;
}

static bool app_draw_inspector_v2_panel(EditorBootstrapApp *app, float panel_x,
                                        float panel_y, float panel_w,
                                        float panel_h) {
  float x = panel_x + 10.0f;
  float y = panel_y + 8.0f;
  float content_w = panel_w - 20.0f;
  StygianEditorNodeId ids[APP_MAX_TRANSFORM_SELECTION];
  StygianEditorNodeId selected = STYGIAN_EDITOR_INVALID_ID;
  uint32_t count = app_selected_nodes(app, ids, APP_MAX_TRANSFORM_SELECTION);
  bool mutated = false;
  bool uniform_any = false;
  bool uniform_all = true;
  bool have_first_color = false;
  bool have_rect = false;
  bool have_rotation = false;
  bool mixed_rotation = false;
  bool single_text = false;
  bool single_image = false;
  bool single_frame = false;
  bool single_component_def = false;
  bool single_component_instance = false;
  float rotation_degrees = 0.0f;
  float rtl = 0.0f, rtr = 0.0f, rbr = 0.0f, rbl = 0.0f;
  float gx = 0.0f, gy = 0.0f, gw = 0.0f, gh = 0.0f;
  StygianEditorColor first_color = {0};
  StygianEditorNodeFill first_fill = {0};
  StygianEditorNodeStroke first_stroke = {0};
  bool have_fill = false;
  bool have_stroke = false;
  float font_size = 24.0f;
  float line_height = 28.0f;
  float letter_spacing = 0.0f;
  uint32_t font_weight = 400u;
  StygianEditorTextBoxMode box_mode = STYGIAN_EDITOR_TEXT_BOX_AREA;
  StygianEditorTextHAlign align_h = STYGIAN_EDITOR_TEXT_ALIGN_LEFT;
  StygianEditorTextVAlign align_v = STYGIAN_EDITOR_TEXT_ALIGN_TOP;
  StygianEditorTextAutoSize auto_size = STYGIAN_EDITOR_TEXT_AUTOSIZE_HEIGHT;
  uint32_t image_fit_mode = 0u;
  bool frame_clip_content = false;
  StygianEditorFrameOverflowPolicy overflow_policy =
      STYGIAN_EDITOR_FRAME_OVERFLOW_VISIBLE;
  StygianEditorFrameAutoLayout frame_layout = {0};
  StygianEditorNodeLayoutOptions child_layout_opts = {0};
  StygianEditorNodeId parent_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorShapeKind parent_kind = STYGIAN_EDITOR_SHAPE_RECT;
  StygianEditorConstraintH constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
  StygianEditorConstraintV constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
  float min_w = 0.0f, max_w = 0.0f, min_h = 0.0f, max_h = 0.0f;
  StygianEditorNodeId component_def_id = STYGIAN_EDITOR_INVALID_ID;
  bool parent_auto_layout = false;
  char path[192];
  char hdr[128];
  uint32_t i;

  if (!app || !app->editor || panel_w <= 0.0f || panel_h <= 0.0f)
    return false;

  app_draw_panel_shell(app, panel_x, panel_y, panel_w, panel_h, 0.32f, 0.52f,
                       0.78f);
  stygian_widgets_register_region(
      panel_x, panel_y, panel_w, panel_h,
      STYGIAN_WIDGET_REGION_POINTER_LEFT_MUTATES |
          STYGIAN_WIDGET_REGION_POINTER_RIGHT_MUTATES |
          STYGIAN_WIDGET_REGION_SCROLL);

  if (app->font) {
    stygian_text(app->ctx, app->font, "Inspector", x, y, 16.0f, 0.93f, 0.95f,
                 1.0f, 1.0f);
  }
  y += 22.0f;

  if (count == 0u) {
    app_draw_panel_card(app, x, y, content_w, 118.0f);
    if (app->font) {
      stygian_text(app->ctx, app->font, "Document", x + 10.0f, y + 11.0f, 12.0f,
                   0.82f, 0.88f, 0.95f, 1.0f);
      stygian_text(app->ctx, app->font,
                   "No active selection. This is the document-level fallback surface.",
                   x + 10.0f, y + 28.0f, 10.5f, 0.66f, 0.74f, 0.84f, 1.0f);
      stygian_text(app->ctx, app->font, app->project_path, x + 10.0f, y + 46.0f,
                   10.0f, 0.66f, 0.74f, 0.84f, 1.0f);
    }
    stygian_text_input(app->ctx, app->font, x + 10.0f, y + 64.0f, content_w - 20.0f,
                       20.0f, app->project_path_draft,
                       (int)sizeof(app->project_path_draft));
    if (stygian_button(app->ctx, app->font, "Use Path", x + 10.0f, y + 88.0f, 70.0f,
                       20.0f)) {
      snprintf(app->project_path, sizeof(app->project_path), "%s",
               app->project_path_draft);
      app_set_status(app, "Project path updated.");
      mutated = true;
    }
    return false;
  }

  selected = stygian_editor_selected_node(app->editor);
  app_sync_component_editor_buffer(app);
  app_sync_fx_editor_buffers(app);
  if (app_bounds_for_nodes_world(app, ids, count, &gx, &gy, &gw, &gh)) {
    app->panel_group_x = gx;
    app->panel_group_y = gy;
    app->panel_group_w = gw;
    app->panel_group_h = gh;
  }
  app_node_path(app, selected, path, sizeof(path));

  for (i = 0u; i < count; ++i) {
    float r = 0.0f;
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
      StygianEditorShapeKind kind = STYGIAN_EDITOR_SHAPE_RECT;
      if (stygian_editor_node_get_shape_kind(app->editor, ids[i], &kind) &&
          kind == STYGIAN_EDITOR_SHAPE_RECT &&
          stygian_editor_node_get_corner_radii(app->editor, ids[i], &rtl, &rtr,
                                               &rbr, &rbl)) {
        have_rect = true;
      }
    }
    if (!stygian_editor_node_get_rotation(app->editor, ids[i], &r))
      continue;
    if (!have_rotation) {
      rotation_degrees = r;
      have_rotation = true;
    } else if (fabsf(r - rotation_degrees) > 0.001f) {
      mixed_rotation = true;
    }
  }
  app->panel_uniform_scale = uniform_any;
  app->panel_opacity = have_first_color ? first_color.a : 1.0f;
  if (count == 1u) {
    StygianEditorShapeKind selected_kind = STYGIAN_EDITOR_SHAPE_RECT;
    (void)stygian_editor_node_get_shape_kind(app->editor, selected, &selected_kind);
    single_text = selected_kind == STYGIAN_EDITOR_SHAPE_TEXT;
    single_image = selected_kind == STYGIAN_EDITOR_SHAPE_IMAGE;
    single_frame = selected_kind == STYGIAN_EDITOR_SHAPE_FRAME;
    single_component_def = selected_kind == STYGIAN_EDITOR_SHAPE_COMPONENT_DEF;
    single_component_instance =
        selected_kind == STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE;
    have_fill = stygian_editor_node_fill_count(app->editor, selected) > 0u &&
                stygian_editor_node_get_fill(app->editor, selected, 0u, &first_fill);
    have_stroke = stygian_editor_node_stroke_count(app->editor, selected) > 0u &&
                  stygian_editor_node_get_stroke(app->editor, selected, 0u, &first_stroke);
    if (single_text) {
      app_sync_text_editor_buffer(app);
      (void)stygian_editor_node_get_text_typography(app->editor, selected, &font_size,
                                                    &line_height, &letter_spacing,
                                                    &font_weight);
      (void)stygian_editor_node_get_text_layout(app->editor, selected, &box_mode,
                                                &align_h, &align_v, &auto_size);
    } else if (single_image) {
      (void)stygian_editor_node_get_image_source(app->editor, selected,
                                                 app->import_image_source,
                                                 sizeof(app->import_image_source));
      (void)stygian_editor_node_get_image_fit_mode(app->editor, selected,
                                                   &image_fit_mode);
    } else if (single_frame) {
      (void)stygian_editor_frame_get_auto_layout(app->editor, selected,
                                                 &frame_layout);
      (void)stygian_editor_frame_get_overflow_policy(app->editor, selected,
                                                     &overflow_policy);
      frame_clip_content = overflow_policy == STYGIAN_EDITOR_FRAME_OVERFLOW_CLIP ||
                           overflow_policy == STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_X ||
                           overflow_policy == STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_Y ||
                           overflow_policy == STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_BOTH;
    } else if (single_component_def) {
      component_def_id = selected;
    } else if (single_component_instance) {
      (void)stygian_editor_component_instance_get_def(app->editor, selected,
                                                      &component_def_id);
    }
    if (stygian_editor_node_get_parent(app->editor, selected, &parent_id) &&
        parent_id != STYGIAN_EDITOR_INVALID_ID &&
        stygian_editor_node_get_shape_kind(app->editor, parent_id, &parent_kind)) {
      if (parent_kind == STYGIAN_EDITOR_SHAPE_FRAME &&
          stygian_editor_frame_get_auto_layout(app->editor, parent_id,
                                               &frame_layout)) {
        parent_auto_layout =
            frame_layout.mode != STYGIAN_EDITOR_AUTO_LAYOUT_OFF;
      }
    }
    (void)stygian_editor_node_get_layout_options(app->editor, selected,
                                                 &child_layout_opts);
    (void)stygian_editor_node_get_constraints(app->editor, selected, &constraint_h,
                                              &constraint_v);
    (void)stygian_editor_node_get_size_limits(app->editor, selected, &min_w, &max_w,
                                              &min_h, &max_h);
  }

  app_draw_panel_card(app, x, y, content_w, 58.0f);
  snprintf(hdr, sizeof(hdr), "Selection %u | primary #%u", count, selected);
  if (app->font) {
    stygian_text(app->ctx, app->font, hdr, x + 10.0f, y + 9.0f, 11.5f, 0.88f,
                 0.92f, 0.97f, 1.0f);
    stygian_text(app->ctx, app->font, path, x + 10.0f, y + 25.0f, 10.3f, 0.66f,
                 0.74f, 0.84f, 1.0f);
    stygian_text(app->ctx, app->font,
                 count > 1u ? "Group edit is live for transform and appearance."
                            : "Single-node edit surface.",
                 x + 10.0f, y + 40.0f, 9.9f, 0.63f, 0.71f, 0.81f, 1.0f);
  }
  y += 68.0f;

  app_draw_panel_card(app, x, y, content_w, 124.0f);
  if (app->font) {
    stygian_text(app->ctx, app->font, "Transform", x + 10.0f, y + 10.0f, 13.0f,
                 0.90f, 0.94f, 1.0f, 1.0f);
  }
  y += 28.0f;

  if (app->font)
    stygian_text(app->ctx, app->font, "X", x + 10.0f, y, 11.0f, 0.85f, 0.88f,
                 0.92f, 1.0f);
  {
    float v = app->panel_group_x;
    if (stygian_slider(app->ctx, x + 22.0f, y, content_w - 32.0f, 14.0f, &v,
                       -4000.0f, 4000.0f) &&
        app_apply_group_translation(app, v, app->panel_group_y)) {
      mutated = true;
      (void)app_selected_bounds_world(app, &app->panel_group_x, &app->panel_group_y,
                                      &app->panel_group_w, &app->panel_group_h);
    }
  }
  y += 18.0f;
  if (app->font)
    stygian_text(app->ctx, app->font, "Y", x + 10.0f, y, 11.0f, 0.85f, 0.88f,
                 0.92f, 1.0f);
  {
    float v = app->panel_group_y;
    if (stygian_slider(app->ctx, x + 22.0f, y, content_w - 32.0f, 14.0f, &v,
                       -4000.0f, 4000.0f) &&
        app_apply_group_translation(app, app->panel_group_x, v)) {
      mutated = true;
      (void)app_selected_bounds_world(app, &app->panel_group_x, &app->panel_group_y,
                                      &app->panel_group_w, &app->panel_group_h);
    }
  }
  y += 18.0f;
  if (app->font)
    stygian_text(app->ctx, app->font, "W", x + 10.0f, y, 11.0f, 0.85f, 0.88f,
                 0.92f, 1.0f);
  {
    float v = app->panel_group_w;
    if (stygian_slider(app->ctx, x + 22.0f, y, content_w - 32.0f, 14.0f, &v, 1.0f,
                       4000.0f) &&
        app_apply_group_resize(app, v, app->panel_group_h, true, false)) {
      mutated = true;
      (void)app_selected_bounds_world(app, &app->panel_group_x, &app->panel_group_y,
                                      &app->panel_group_w, &app->panel_group_h);
    }
  }
  y += 18.0f;
  if (app->font)
    stygian_text(app->ctx, app->font, "H", x + 10.0f, y, 11.0f, 0.85f, 0.88f,
                 0.92f, 1.0f);
  {
    float v = app->panel_group_h;
    if (stygian_slider(app->ctx, x + 22.0f, y, content_w - 32.0f, 14.0f, &v, 1.0f,
                       4000.0f) &&
        app_apply_group_resize(app, app->panel_group_w, v, false, true)) {
      mutated = true;
      (void)app_selected_bounds_world(app, &app->panel_group_x, &app->panel_group_y,
                                      &app->panel_group_w, &app->panel_group_h);
    }
  }
  y += 22.0f;
  if (app->font)
    stygian_text(app->ctx, app->font, "Rotation", x + 10.0f, y, 11.0f, 0.85f,
                 0.88f, 0.92f, 1.0f);
  if (have_rotation) {
    float r = rotation_degrees;
    if (stygian_slider(app->ctx, x + 72.0f, y, content_w - 82.0f, 14.0f, &r,
                       -180.0f, 180.0f)) {
      if (app_apply_rotation_to_selection(app, r))
        mutated = true;
      rotation_degrees = r;
      mixed_rotation = false;
    }
  }
  y += 42.0f;

  app_draw_panel_card(app, x, y, content_w, have_rect ? 126.0f : 76.0f);
  if (app->font) {
    stygian_text(app->ctx, app->font, "Appearance", x + 10.0f, y + 10.0f, 13.0f,
                 0.90f, 0.94f, 1.0f, 1.0f);
  }
  y += 30.0f;
  if (app->font)
    stygian_text(app->ctx, app->font, "Opacity", x + 10.0f, y, 11.0f, 0.85f, 0.88f,
                 0.92f, 1.0f);
  {
    float opacity = app->panel_opacity;
    if (stygian_slider(app->ctx, x + 62.0f, y, content_w - 72.0f, 14.0f, &opacity,
                       0.0f, 1.0f)) {
      if (app_apply_opacity_to_selection(app, opacity))
        mutated = true;
      app->panel_opacity = opacity;
    }
  }
  y += 24.0f;
  if (stygian_button(app->ctx, app->font, "Apply Paint Color", x + 10.0f, y,
                     content_w - 20.0f, 22.0f)) {
    if (app_apply_color_to_selection(app, app->paint_color))
      mutated = true;
  }
  y += 32.0f;
  if (have_rect) {
    app->panel_radius_tl = rtl;
    app->panel_radius_tr = rtr;
    app->panel_radius_br = rbr;
    app->panel_radius_bl = rbl;
    if (app->font)
      stygian_text(app->ctx, app->font, "Corner Radius", x + 10.0f, y, 11.0f,
                   0.85f, 0.88f, 0.92f, 1.0f);
    y += 16.0f;
    if (app->font)
      stygian_text(app->ctx, app->font, "TL", x + 10.0f, y, 10.5f, 0.82f, 0.87f,
                   0.94f, 1.0f);
    {
      float v = app->panel_radius_tl;
      if (stygian_slider(app->ctx, x + 34.0f, y, content_w - 44.0f, 14.0f, &v,
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
    y += 18.0f;
    if (app->font)
      stygian_text(app->ctx, app->font, "TR", x + 10.0f, y, 10.5f, 0.82f, 0.87f,
                   0.94f, 1.0f);
    {
      float v = app->panel_radius_tr;
      if (stygian_slider(app->ctx, x + 34.0f, y, content_w - 44.0f, 14.0f, &v,
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
    y += 18.0f;
    if (app->font)
      stygian_text(app->ctx, app->font, "BR", x + 10.0f, y, 10.5f, 0.82f, 0.87f,
                   0.94f, 1.0f);
    {
      float v = app->panel_radius_br;
      if (stygian_slider(app->ctx, x + 34.0f, y, content_w - 44.0f, 14.0f, &v,
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
    y += 18.0f;
    if (app->font)
      stygian_text(app->ctx, app->font, "BL", x + 10.0f, y, 10.5f, 0.82f, 0.87f,
                   0.94f, 1.0f);
    {
      float v = app->panel_radius_bl;
      if (stygian_slider(app->ctx, x + 34.0f, y, content_w - 44.0f, 14.0f, &v,
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
    y += 22.0f;
  }
  app_draw_panel_card(app, x, y, content_w, 84.0f);
  if (app->font) {
    stygian_text(app->ctx, app->font, "Primitive Rules", x + 10.0f, y + 10.0f,
                 13.0f, 0.90f, 0.94f, 1.0f, 1.0f);
    if (mixed_rotation) {
      stygian_text(app->ctx, app->font, "Rotation is mixed across selection.",
                   x + 10.0f, y + 27.0f, 10.5f, 0.82f, 0.74f, 0.66f, 1.0f);
    }
    if (uniform_any && !uniform_all) {
      stygian_text(app->ctx, app->font, "Uniform scale is mixed.", x + 10.0f,
                   y + 42.0f, 10.5f, 0.82f, 0.74f, 0.66f, 1.0f);
    }
  }
  if (stygian_checkbox(app->ctx, app->font, "Uniform Scale (per primitive)",
                       x + 10.0f, y + 58.0f, &app->panel_uniform_scale)) {
    for (i = 0u; i < count; ++i)
      app_set_node_uniform_scale(app, ids[i], app->panel_uniform_scale);
    mutated = true;
  }
  y += 94.0f;

  app_draw_panel_card(app, x, y, content_w, 66.0f);
  if (app->font) {
    stygian_text(app->ctx, app->font, "Ownership", x + 10.0f, y + 10.0f, 13.0f,
                 0.90f, 0.94f, 1.0f, 1.0f);
    stygian_text(app->ctx, app->font,
                 "Layout exports as generated C23. Hooks stay hand-owned.",
                 x + 10.0f, y + 28.0f, 10.5f, 0.66f, 0.74f, 0.84f, 1.0f);
    stygian_text(app->ctx, app->font,
                 "This panel edits authored state, not generated files.",
                 x + 10.0f, y + 43.0f, 10.0f, 0.62f, 0.70f, 0.80f, 1.0f);
  }
  y += 76.0f;

  if (count == 1u && single_text) {
    static StygianTextArea text_area = {0};
    app_draw_panel_card(app, x, y, content_w, 230.0f);
    stygian_text(app->ctx, app->font, "Content", x + 10.0f, y + 10.0f, 13.0f,
                 0.90f, 0.94f, 1.0f, 1.0f);
    if (text_area.buffer != app->text_edit_buffer) {
      memset(&text_area, 0, sizeof(text_area));
      text_area.buffer = app->text_edit_buffer;
      text_area.buffer_size = (int)sizeof(app->text_edit_buffer);
    }
    text_area.read_only = false;
    text_area.x = x + 10.0f;
    text_area.y = y + 28.0f;
    text_area.w = content_w - 20.0f;
    text_area.h = 92.0f;
    if (stygian_text_area(app->ctx, app->font, &text_area)) {
      (void)stygian_editor_node_set_text_content(app->editor, selected,
                                                 app->text_edit_buffer);
      mutated = true;
    }
    stygian_text(app->ctx, app->font, "Typography", x + 10.0f, y + 128.0f,
                 12.0f, 0.88f, 0.92f, 0.98f, 1.0f);
    if (stygian_slider(app->ctx, x + 68.0f, y + 128.0f, content_w - 78.0f, 14.0f,
                       &font_size, 8.0f, 72.0f)) {
      (void)stygian_editor_node_set_text_typography(app->editor, selected, font_size,
                                                    line_height, letter_spacing,
                                                    font_weight);
      mutated = true;
    }
    if (stygian_slider(app->ctx, x + 68.0f, y + 146.0f, content_w - 78.0f, 14.0f,
                       &line_height, 10.0f, 96.0f)) {
      (void)stygian_editor_node_set_text_typography(app->editor, selected, font_size,
                                                    line_height, letter_spacing,
                                                    font_weight);
      mutated = true;
    }
    if (stygian_slider(app->ctx, x + 68.0f, y + 164.0f, content_w - 78.0f, 14.0f,
                       &letter_spacing, -4.0f, 16.0f)) {
      (void)stygian_editor_node_set_text_typography(app->editor, selected, font_size,
                                                    line_height, letter_spacing,
                                                    font_weight);
      mutated = true;
    }
    if (stygian_radio_button(app->ctx, app->font, "Point", x + 10.0f, y + 186.0f,
                             (int *)&box_mode, STYGIAN_EDITOR_TEXT_BOX_POINT) ||
        stygian_radio_button(app->ctx, app->font, "Area", x + 74.0f, y + 186.0f,
                             (int *)&box_mode, STYGIAN_EDITOR_TEXT_BOX_AREA)) {
      (void)stygian_editor_node_set_text_layout(app->editor, selected, box_mode,
                                                align_h, align_v, auto_size);
      mutated = true;
    }
    y += 240.0f;
  } else if (count == 1u && single_image) {
    app_draw_panel_card(app, x, y, content_w, 128.0f);
    stygian_text(app->ctx, app->font, "Image", x + 10.0f, y + 10.0f, 13.0f,
                 0.90f, 0.94f, 1.0f, 1.0f);
    stygian_text_input(app->ctx, app->font, x + 10.0f, y + 30.0f, content_w - 20.0f,
                       20.0f, app->import_image_source,
                       (int)sizeof(app->import_image_source));
    if (stygian_button(app->ctx, app->font, "Replace", x + 10.0f, y + 56.0f, 72.0f,
                       20.0f)) {
      if (stygian_editor_node_set_image_source(app->editor, selected,
                                               app->import_image_source)) {
        app_set_status(app, "Image source updated.");
        mutated = true;
      }
    }
    stygian_text(app->ctx, app->font, app_image_fit_name(image_fit_mode), x + 90.0f,
                 y + 60.0f, 10.8f, 0.68f, 0.76f, 0.86f, 1.0f);
    if (stygian_radio_button(app->ctx, app->font, "Fill", x + 10.0f, y + 84.0f,
                             (int *)&image_fit_mode, 0) ||
        stygian_radio_button(app->ctx, app->font, "Contain", x + 70.0f, y + 84.0f,
                             (int *)&image_fit_mode, 1) ||
        stygian_radio_button(app->ctx, app->font, "Cover", x + 154.0f, y + 84.0f,
                             (int *)&image_fit_mode, 2)) {
      (void)stygian_editor_node_set_image_fit_mode(app->editor, selected,
                                                   image_fit_mode);
      mutated = true;
    }
    y += 138.0f;
  } else if (count == 1u && single_frame) {
    app_draw_panel_card(app, x, y, content_w, 188.0f);
    stygian_text(app->ctx, app->font, "Layout", x + 10.0f, y + 10.0f, 13.0f,
                 0.90f, 0.94f, 1.0f, 1.0f);
    if (stygian_radio_button(app->ctx, app->font, "Off", x + 10.0f, y + 30.0f,
                             (int *)&frame_layout.mode,
                             STYGIAN_EDITOR_AUTO_LAYOUT_OFF) ||
        stygian_radio_button(app->ctx, app->font, "H", x + 62.0f, y + 30.0f,
                             (int *)&frame_layout.mode,
                             STYGIAN_EDITOR_AUTO_LAYOUT_HORIZONTAL) ||
        stygian_radio_button(app->ctx, app->font, "V", x + 104.0f, y + 30.0f,
                             (int *)&frame_layout.mode,
                             STYGIAN_EDITOR_AUTO_LAYOUT_VERTICAL)) {
      (void)stygian_editor_frame_set_auto_layout(app->editor, selected,
                                                 &frame_layout);
      mutated = true;
    }
    if (stygian_checkbox(app->ctx, app->font, "Clip Content", x + 168.0f, y + 30.0f,
                         &frame_clip_content)) {
      overflow_policy = frame_clip_content ? STYGIAN_EDITOR_FRAME_OVERFLOW_CLIP
                                           : STYGIAN_EDITOR_FRAME_OVERFLOW_VISIBLE;
      (void)stygian_editor_frame_set_overflow_policy(app->editor, selected,
                                                     overflow_policy);
      mutated = true;
    }
    {
      bool wrap_enabled = frame_layout.wrap == STYGIAN_EDITOR_AUTO_LAYOUT_WRAP;
      if (stygian_checkbox(app->ctx, app->font, "Wrap", x + 126.0f, y + 54.0f,
                           &wrap_enabled)) {
        frame_layout.wrap = wrap_enabled ? STYGIAN_EDITOR_AUTO_LAYOUT_WRAP
                                         : STYGIAN_EDITOR_AUTO_LAYOUT_NO_WRAP;
        (void)stygian_editor_frame_set_auto_layout(app->editor, selected,
                                                   &frame_layout);
        mutated = true;
      }
    }
    if (stygian_slider(app->ctx, x + 54.0f, y + 80.0f, content_w - 64.0f, 14.0f,
                       &frame_layout.gap, 0.0f, 64.0f)) {
      (void)stygian_editor_frame_set_auto_layout(app->editor, selected,
                                                 &frame_layout);
      mutated = true;
    }
    stygian_text(app->ctx, app->font, "Gap", x + 10.0f, y + 80.0f, 10.6f, 0.82f,
                 0.88f, 0.94f, 1.0f);
    if (stygian_slider(app->ctx, x + 54.0f, y + 98.0f, content_w - 64.0f, 14.0f,
                       &frame_layout.padding_left, 0.0f, 96.0f)) {
      frame_layout.padding_right = frame_layout.padding_left;
      frame_layout.padding_top = frame_layout.padding_left;
      frame_layout.padding_bottom = frame_layout.padding_left;
      (void)stygian_editor_frame_set_auto_layout(app->editor, selected,
                                                 &frame_layout);
      mutated = true;
    }
    stygian_text(app->ctx, app->font, "Pad", x + 10.0f, y + 98.0f, 10.6f, 0.82f,
                 0.88f, 0.94f, 1.0f);
    if (stygian_radio_button(app->ctx, app->font, "Start", x + 10.0f, y + 122.0f,
                             (int *)&frame_layout.primary_align,
                             STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START) ||
        stygian_radio_button(app->ctx, app->font, "Center", x + 76.0f, y + 122.0f,
                             (int *)&frame_layout.primary_align,
                             STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_CENTER) ||
        stygian_radio_button(app->ctx, app->font, "End", x + 154.0f, y + 122.0f,
                             (int *)&frame_layout.primary_align,
                             STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_END)) {
      (void)stygian_editor_frame_set_auto_layout(app->editor, selected,
                                                 &frame_layout);
      mutated = true;
    }
    if (stygian_radio_button(app->ctx, app->font, "Cross Start", x + 10.0f,
                             y + 146.0f, (int *)&frame_layout.cross_align,
                             STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START) ||
        stygian_radio_button(app->ctx, app->font, "Cross Center", x + 102.0f,
                             y + 146.0f, (int *)&frame_layout.cross_align,
                             STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_CENTER) ||
        stygian_radio_button(app->ctx, app->font, "Stretch", x + 206.0f,
                             y + 146.0f, (int *)&frame_layout.cross_align,
                             STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_STRETCH)) {
      (void)stygian_editor_frame_set_auto_layout(app->editor, selected,
                                                 &frame_layout);
      mutated = true;
    }
    y += 198.0f;
  }

  if (count == 1u && !single_frame &&
      parent_id != STYGIAN_EDITOR_INVALID_ID) {
    app_draw_panel_card(app, x, y, content_w, parent_auto_layout ? 108.0f : 154.0f);
    stygian_text(app->ctx, app->font, "Responsive", x + 10.0f, y + 10.0f, 13.0f,
                 0.90f, 0.94f, 1.0f, 1.0f);
    if (parent_auto_layout) {
      if (stygian_checkbox(app->ctx, app->font, "Absolute In Parent", x + 10.0f,
                           y + 32.0f, &child_layout_opts.absolute_position)) {
        (void)stygian_editor_node_set_layout_options(app->editor, selected,
                                                     &child_layout_opts);
        mutated = true;
      }
      if (stygian_radio_button(app->ctx, app->font, "W Fixed", x + 10.0f, y + 56.0f,
                               (int *)&child_layout_opts.sizing_h,
                               STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED) ||
          stygian_radio_button(app->ctx, app->font, "Hug", x + 82.0f, y + 56.0f,
                               (int *)&child_layout_opts.sizing_h,
                               STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_HUG) ||
          stygian_radio_button(app->ctx, app->font, "Fill", x + 138.0f, y + 56.0f,
                               (int *)&child_layout_opts.sizing_h,
                               STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FILL)) {
        (void)stygian_editor_node_set_layout_options(app->editor, selected,
                                                     &child_layout_opts);
        mutated = true;
      }
      if (stygian_radio_button(app->ctx, app->font, "H Fixed", x + 10.0f, y + 80.0f,
                               (int *)&child_layout_opts.sizing_v,
                               STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED) ||
          stygian_radio_button(app->ctx, app->font, "Hug", x + 82.0f, y + 80.0f,
                               (int *)&child_layout_opts.sizing_v,
                               STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_HUG) ||
          stygian_radio_button(app->ctx, app->font, "Fill", x + 138.0f, y + 80.0f,
                               (int *)&child_layout_opts.sizing_v,
                               STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FILL)) {
        (void)stygian_editor_node_set_layout_options(app->editor, selected,
                                                     &child_layout_opts);
        mutated = true;
      }
    } else {
      if (stygian_radio_button(app->ctx, app->font, "Left", x + 10.0f, y + 34.0f,
                               (int *)&constraint_h,
                               STYGIAN_EDITOR_CONSTRAINT_H_LEFT) ||
          stygian_radio_button(app->ctx, app->font, "Right", x + 62.0f, y + 34.0f,
                               (int *)&constraint_h,
                               STYGIAN_EDITOR_CONSTRAINT_H_RIGHT) ||
          stygian_radio_button(app->ctx, app->font, "L+R", x + 122.0f, y + 34.0f,
                               (int *)&constraint_h,
                               STYGIAN_EDITOR_CONSTRAINT_H_LEFT_RIGHT) ||
          stygian_radio_button(app->ctx, app->font, "Center", x + 180.0f, y + 34.0f,
                               (int *)&constraint_h,
                               STYGIAN_EDITOR_CONSTRAINT_H_CENTER) ||
          stygian_radio_button(app->ctx, app->font, "Scale", x + 252.0f, y + 34.0f,
                               (int *)&constraint_h,
                               STYGIAN_EDITOR_CONSTRAINT_H_SCALE)) {
        (void)stygian_editor_node_set_constraints(app->editor, selected, constraint_h,
                                                  constraint_v);
        mutated = true;
      }
      if (stygian_radio_button(app->ctx, app->font, "Top", x + 10.0f, y + 58.0f,
                               (int *)&constraint_v,
                               STYGIAN_EDITOR_CONSTRAINT_V_TOP) ||
          stygian_radio_button(app->ctx, app->font, "Bottom", x + 56.0f, y + 58.0f,
                               (int *)&constraint_v,
                               STYGIAN_EDITOR_CONSTRAINT_V_BOTTOM) ||
          stygian_radio_button(app->ctx, app->font, "T+B", x + 122.0f, y + 58.0f,
                               (int *)&constraint_v,
                               STYGIAN_EDITOR_CONSTRAINT_V_TOP_BOTTOM) ||
          stygian_radio_button(app->ctx, app->font, "Center", x + 180.0f, y + 58.0f,
                               (int *)&constraint_v,
                               STYGIAN_EDITOR_CONSTRAINT_V_CENTER) ||
          stygian_radio_button(app->ctx, app->font, "Scale", x + 252.0f, y + 58.0f,
                               (int *)&constraint_v,
                               STYGIAN_EDITOR_CONSTRAINT_V_SCALE)) {
        (void)stygian_editor_node_set_constraints(app->editor, selected, constraint_h,
                                                  constraint_v);
        mutated = true;
      }
      stygian_text(app->ctx, app->font, "Min W", x + 10.0f, y + 86.0f, 10.6f, 0.82f,
                   0.88f, 0.94f, 1.0f);
      if (stygian_slider(app->ctx, x + 58.0f, y + 86.0f, content_w - 68.0f, 14.0f,
                         &min_w, 0.0f, 800.0f)) {
        (void)stygian_editor_node_set_size_limits(app->editor, selected, min_w, max_w,
                                                  min_h, max_h);
        mutated = true;
      }
      stygian_text(app->ctx, app->font, "Min H", x + 10.0f, y + 110.0f, 10.6f, 0.82f,
                   0.88f, 0.94f, 1.0f);
      if (stygian_slider(app->ctx, x + 58.0f, y + 110.0f, content_w - 68.0f, 14.0f,
                         &min_h, 0.0f, 800.0f)) {
        (void)stygian_editor_node_set_size_limits(app->editor, selected, min_w, max_w,
                                                  min_h, max_h);
        mutated = true;
      }
    }
    y += parent_auto_layout ? 118.0f : 164.0f;
  }

  if (count == 1u && (single_component_def || single_component_instance)) {
    float card_h = single_component_def ? 220.0f : 196.0f;
    app_draw_panel_card(app, x, y, content_w, card_h);
    stygian_text(app->ctx, app->font, "Component", x + 10.0f, y + 10.0f, 13.0f,
                 0.90f, 0.94f, 1.0f, 1.0f);
    if (component_def_id != STYGIAN_EDITOR_INVALID_ID) {
      char symbol[64];
      char group[32];
      char variant[32];
      uint32_t prop_count = stygian_editor_component_def_property_count(
          app->editor, component_def_id);
      symbol[0] = '\0';
      group[0] = '\0';
      variant[0] = '\0';
      (void)stygian_editor_component_def_get_symbol(app->editor, component_def_id,
                                                    symbol, sizeof(symbol));
      (void)stygian_editor_component_def_get_variant(app->editor, component_def_id,
                                                     group, sizeof(group), variant,
                                                     sizeof(variant));
      if (single_component_instance) {
        bool detached = false;
        char info[160];
        (void)stygian_editor_component_instance_is_detached(app->editor, selected,
                                                            &detached);
        snprintf(info, sizeof(info), "resolved -> %s | set %s:%s | %s",
                 symbol[0] ? symbol : "(unnamed)", group[0] ? group : "-",
                 variant[0] ? variant : "-", detached ? "detached" : "linked");
        stygian_text(app->ctx, app->font, info, x + 10.0f, y + 28.0f, 10.4f,
                     0.70f, 0.79f, 0.89f, 1.0f);
        if (stygian_button(app->ctx, app->font, "Reset Overrides", x + 10.0f,
                           y + 46.0f, 102.0f, 20.0f)) {
          if (stygian_editor_component_instance_reset_overrides(app->editor,
                                                                selected)) {
            app_set_status(app, "Component overrides reset.");
            mutated = true;
          }
        }
        if (stygian_button(app->ctx, app->font, detached ? "Repair" : "Detach",
                           x + 118.0f, y + 46.0f, 74.0f, 20.0f)) {
          bool ok = detached
                        ? stygian_editor_component_instance_repair(
                              app->editor, selected, component_def_id, true)
                        : stygian_editor_component_instance_detach(app->editor,
                                                                   selected);
          if (ok) {
            app_set_status(app, detached ? "Component instance repaired."
                                         : "Component instance detached.");
            mutated = true;
          }
        }
        stygian_text(app->ctx, app->font, "Runtime mapping", x + 10.0f, y + 74.0f,
                     11.0f, 0.86f, 0.90f, 0.96f, 1.0f);
        stygian_text(app->ctx, app->font,
                     "Generated C23 records the resolved symbol and active override set.",
                     x + 10.0f, y + 89.0f, 10.0f, 0.64f, 0.72f, 0.83f, 1.0f);
        for (uint32_t pi = 0u; pi < prop_count && pi < 4u; ++pi) {
          StygianEditorComponentPropertyDef prop = {0};
          StygianEditorComponentPropertyValue ov = {0};
          bool has_override = false;
          float row_y = y + 112.0f + 20.0f * (float)pi;
          if (!stygian_editor_component_def_get_property(app->editor, component_def_id,
                                                         pi, &prop)) {
            continue;
          }
          has_override = stygian_editor_component_instance_get_property_override(
              app->editor, selected, prop.name, &ov);
          if (prop.type == STYGIAN_EDITOR_COMPONENT_PROPERTY_BOOL) {
            bool value = has_override ? ov.bool_value : prop.default_bool;
            char label[96];
            snprintf(label, sizeof(label), "%s [%s]", prop.name,
                     has_override ? "override" : "default");
            if (stygian_checkbox(app->ctx, app->font, label, x + 10.0f, row_y,
                                 &value)) {
              StygianEditorComponentPropertyValue pv = {0};
              snprintf(pv.name, sizeof(pv.name), "%s", prop.name);
              pv.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_BOOL;
              pv.bool_value = value;
              if (stygian_editor_component_instance_set_property_override(
                      app->editor, selected, &pv)) {
                mutated = true;
              }
            }
          } else {
            char label[128];
            const char *value =
                has_override && ov.enum_value[0] ? ov.enum_value : prop.default_enum;
            snprintf(label, sizeof(label), "%s = %s [%s]", prop.name,
                     value && value[0] ? value : "-", has_override ? "override" : "default");
            stygian_text(app->ctx, app->font, label, x + 10.0f, row_y, 10.4f,
                         0.78f, 0.86f, 0.95f, 1.0f);
            if (stygian_button(app->ctx, app->font, "Next", x + content_w - 90.0f,
                               row_y - 3.0f, 36.0f, 18.0f)) {
              StygianEditorComponentPropertyValue pv = {0};
              uint32_t idx = 0u;
              const char *current = value;
              for (; idx < prop.enum_option_count; ++idx) {
                if (strncmp(prop.enum_options[idx], current,
                            sizeof(prop.enum_options[idx])) == 0)
                  break;
              }
              if (prop.enum_option_count > 0u)
                idx = (idx + 1u) % prop.enum_option_count;
              snprintf(pv.name, sizeof(pv.name), "%s", prop.name);
              pv.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
              snprintf(pv.enum_value, sizeof(pv.enum_value), "%s",
                       prop.enum_option_count > 0u ? prop.enum_options[idx] : "");
              if (stygian_editor_component_instance_set_property_override(
                      app->editor, selected, &pv)) {
                mutated = true;
              }
            }
          }
          if (stygian_button(app->ctx, app->font, "Clr", x + content_w - 48.0f,
                             row_y - 3.0f, 30.0f, 18.0f)) {
            if (stygian_editor_component_instance_clear_property_override(
                    app->editor, selected, prop.name)) {
              mutated = true;
            }
          }
        }
      } else {
        stygian_text_input(app->ctx, app->font, x + 10.0f, y + 28.0f,
                           content_w - 84.0f, 20.0f, app->component_symbol_text,
                           (int)sizeof(app->component_symbol_text));
        if (stygian_button(app->ctx, app->font, "Apply", x + content_w - 66.0f,
                           y + 28.0f, 56.0f, 20.0f)) {
          if (stygian_editor_component_def_set_symbol(app->editor, selected,
                                                      app->component_symbol_text)) {
            app_set_status(app, "Component symbol updated.");
            mutated = true;
          }
        }
        stygian_text_input(app->ctx, app->font, x + 10.0f, y + 54.0f,
                           (content_w - 28.0f) * 0.5f, 20.0f,
                           app->component_variant_group_text,
                           (int)sizeof(app->component_variant_group_text));
        stygian_text_input(app->ctx, app->font, x + 18.0f + (content_w - 28.0f) * 0.5f,
                           y + 54.0f, (content_w - 28.0f) * 0.5f, 20.0f,
                           app->component_variant_name_text,
                           (int)sizeof(app->component_variant_name_text));
        if (stygian_button(app->ctx, app->font, "Set Variant", x + 10.0f, y + 80.0f,
                           86.0f, 20.0f)) {
          if (stygian_editor_component_def_set_variant(
                  app->editor, selected, app->component_variant_group_text,
                  app->component_variant_name_text)) {
            app_set_status(app, "Component variant updated.");
            mutated = true;
          }
        }
        if (stygian_button(app->ctx, app->font, "Create Instance", x + 104.0f,
                           y + 80.0f, 100.0f, 20.0f)) {
          float wx = 0.0f, wy = 0.0f;
          StygianEditorNodeId id;
          stygian_editor_view_to_world(app->editor,
                                       app->layout.viewport_x +
                                           app->layout.viewport_w * 0.5f + 60.0f,
                                       app->layout.viewport_y +
                                           app->layout.viewport_h * 0.58f,
                                       &wx, &wy);
          id = app_add_component_instance_node(app, selected, wx, wy);
          if (id != STYGIAN_EDITOR_INVALID_ID) {
            (void)stygian_editor_select_node(app->editor, id, false);
            app_sync_selected_name(app);
            app_sync_component_editor_buffer(app);
            app_set_status(app, "Component instance created.");
            mutated = true;
          }
        }
        stygian_text(app->ctx, app->font, "Property", x + 10.0f, y + 110.0f, 11.0f,
                     0.86f, 0.90f, 0.96f, 1.0f);
        stygian_text_input(app->ctx, app->font, x + 62.0f, y + 106.0f, 82.0f, 20.0f,
                           app->component_property_name_text,
                           (int)sizeof(app->component_property_name_text));
        if (stygian_radio_button(app->ctx, app->font, "Bool", x + 152.0f, y + 108.0f,
                                 &app->component_property_type_choice, 0) ||
            stygian_radio_button(app->ctx, app->font, "Enum", x + 212.0f, y + 108.0f,
                                 &app->component_property_type_choice, 1)) {
          mutated = true;
        }
        if (app->component_property_type_choice == 0) {
          (void)stygian_checkbox(app->ctx, app->font, "Default true", x + 10.0f,
                                 y + 134.0f, &app->component_property_default_bool);
        } else {
          stygian_text_input(app->ctx, app->font, x + 10.0f, y + 132.0f,
                             content_w - 20.0f, 20.0f,
                             app->component_property_options_text,
                             (int)sizeof(app->component_property_options_text));
          stygian_text_input(app->ctx, app->font, x + 10.0f, y + 158.0f, 110.0f,
                             20.0f, app->component_property_default_enum_text,
                             (int)sizeof(app->component_property_default_enum_text));
        }
        if (stygian_button(app->ctx, app->font, "Set Property", x + 10.0f, y + 184.0f,
                           88.0f, 20.0f)) {
          StygianEditorComponentPropertyDef prop = {0};
          snprintf(prop.name, sizeof(prop.name), "%s",
                   app->component_property_name_text);
          prop.type = app->component_property_type_choice == 1
                          ? STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM
                          : STYGIAN_EDITOR_COMPONENT_PROPERTY_BOOL;
          prop.default_bool = app->component_property_default_bool;
          snprintf(prop.default_enum, sizeof(prop.default_enum), "%s",
                   app->component_property_default_enum_text);
          app_component_fill_options_from_csv(&prop,
                                              app->component_property_options_text);
          if (stygian_editor_component_def_set_property(app->editor, selected,
                                                        &prop)) {
            app_set_status(app, "Component property updated.");
            mutated = true;
          }
        }
        {
          char summary[192];
          char schema[96];
          schema[0] = '\0';
          for (uint32_t pi = 0u; pi < prop_count && pi < 3u; ++pi) {
            StygianEditorComponentPropertyDef prop = {0};
            if (!stygian_editor_component_def_get_property(app->editor, selected,
                                                           pi, &prop)) {
              continue;
            }
            if (schema[0]) {
              strncat(schema, ", ", sizeof(schema) - strlen(schema) - 1u);
            }
            strncat(schema, prop.name, sizeof(schema) - strlen(schema) - 1u);
            strncat(schema, "(", sizeof(schema) - strlen(schema) - 1u);
            strncat(schema, app_component_property_type_name(prop.type),
                    sizeof(schema) - strlen(schema) - 1u);
            strncat(schema, ")", sizeof(schema) - strlen(schema) - 1u);
          }
          snprintf(summary, sizeof(summary), "runtime symbol %s | schema %s",
                   symbol[0] ? symbol : "(unnamed)",
                   schema[0] ? schema : "none");
          stygian_text(app->ctx, app->font, summary, x + 106.0f, y + 188.0f,
                       10.0f, 0.64f, 0.72f, 0.83f, 1.0f);
        }
      }
    }
    y += card_h + 10.0f;
  }

  if (count == 1u) {
    StygianEditorNodeEffect effects[STYGIAN_EDITOR_NODE_EFFECT_CAP];
    StygianEditorEffectTransform effect_xforms[STYGIAN_EDITOR_NODE_EFFECT_CAP];
    uint32_t effect_count = stygian_editor_node_effect_count(app->editor, selected);
    StygianEditorNodeId mask_id = STYGIAN_EDITOR_INVALID_ID;
    StygianEditorMaskMode mask_mode = STYGIAN_EDITOR_MASK_ALPHA;
    bool mask_invert = false;
    StygianEditorShaderAttachment shader = {0};
    uint32_t ei;
    float card_h = 356.0f;
    for (ei = 0u; ei < effect_count && ei < STYGIAN_EDITOR_NODE_EFFECT_CAP; ++ei) {
      (void)stygian_editor_node_get_effect(app->editor, selected, ei, &effects[ei]);
      (void)stygian_editor_node_get_effect_transform(app->editor, selected, ei,
                                                     &effect_xforms[ei]);
    }
    (void)stygian_editor_node_get_mask(app->editor, selected, &mask_id, &mask_mode,
                                       &mask_invert);
    (void)stygian_editor_node_get_shader_attachment(app->editor, selected, &shader);

    app_draw_panel_card(app, x, y, content_w, card_h);
    stygian_text(app->ctx, app->font, "Paint / FX", x + 10.0f, y + 10.0f, 13.0f,
                 0.90f, 0.94f, 1.0f, 1.0f);
    stygian_text(app->ctx, app->font, have_fill ? app_fill_kind_name(first_fill.kind)
                                                : "No fill",
                 x + 10.0f, y + 28.0f, 10.8f, 0.68f, 0.76f, 0.86f, 1.0f);
    if (stygian_button(app->ctx, app->font, "Use Paint As Fill", x + 10.0f, y + 44.0f,
                       110.0f, 20.0f)) {
      StygianEditorNodeFill fill = {0};
      fill.kind = STYGIAN_EDITOR_FILL_SOLID;
      fill.visible = true;
      fill.opacity = 1.0f;
      fill.solid = app->paint_color;
      (void)stygian_editor_node_set_fills(app->editor, selected, &fill, 1u);
      mutated = true;
    }
    if (stygian_button(app->ctx, app->font, "Use Paint As Stroke", x + 126.0f, y + 44.0f,
                       118.0f, 20.0f)) {
      StygianEditorNodeStroke stroke = {0};
      stroke.visible = true;
      stroke.opacity = 1.0f;
      stroke.thickness = have_stroke ? first_stroke.thickness : 2.0f;
      stroke.color = app->paint_color;
      stroke.cap = STYGIAN_EDITOR_STROKE_CAP_ROUND;
      stroke.join = STYGIAN_EDITOR_STROKE_JOIN_ROUND;
      stroke.align = STYGIAN_EDITOR_STROKE_ALIGN_CENTER;
      stroke.miter_limit = 4.0f;
      (void)stygian_editor_node_set_strokes(app->editor, selected, &stroke, 1u);
      mutated = true;
    }
    if (have_stroke) {
      float thickness = first_stroke.thickness;
      if (stygian_slider(app->ctx, x + 78.0f, y + 72.0f, content_w - 88.0f, 14.0f,
                         &thickness, 0.5f, 24.0f)) {
        first_stroke.thickness = thickness;
        (void)stygian_editor_node_set_strokes(app->editor, selected, &first_stroke, 1u);
        mutated = true;
      }
    }
    stygian_text(app->ctx, app->font, "Effect stack", x + 10.0f, y + 96.0f, 11.0f,
                 0.86f, 0.90f, 0.96f, 1.0f);
    if (stygian_button(app->ctx, app->font, "+ Glow", x + content_w - 78.0f, y + 92.0f,
                       68.0f, 20.0f)) {
      if (effect_count < STYGIAN_EDITOR_NODE_EFFECT_CAP) {
        StygianEditorNodeEffect effect = {0};
        StygianEditorEffectTransform xform = {0};
        effect.kind = STYGIAN_EDITOR_EFFECT_GLOW;
        effect.visible = true;
        effect.opacity = 0.7f;
        effect.radius = 16.0f;
        effect.intensity = 0.8f;
        effect.color = app->paint_color;
        xform.scale_x = 1.0f;
        xform.scale_y = 1.0f;
        effects[effect_count] = effect;
        effect_xforms[effect_count] = xform;
        effect_count += 1u;
        if (stygian_editor_node_set_effects(app->editor, selected, effects, effect_count)) {
          for (ei = 0u; ei < effect_count; ++ei) {
            (void)stygian_editor_node_set_effect_transform(app->editor, selected, ei,
                                                           &effect_xforms[ei]);
          }
          app->fx_selected_effect_index = effect_count - 1u;
          mutated = true;
        }
      }
    }
    {
      float row_y = y + 118.0f;
      for (ei = 0u; ei < effect_count && ei < STYGIAN_EDITOR_NODE_EFFECT_CAP; ++ei) {
        char label[64];
        float btn_x = x + 10.0f;
        bool selected_fx = app->fx_selected_effect_index == ei;
        snprintf(label, sizeof(label), "%c %u %s", selected_fx ? '>' : ' ', ei + 1u,
                 app_effect_kind_name(effects[ei].kind));
        if (stygian_button(app->ctx, app->font, label, btn_x, row_y,
                           content_w - 116.0f, 20.0f)) {
          app->fx_selected_effect_index = ei;
          mutated = true;
        }
        if (stygian_button(app->ctx, app->font, "^", x + content_w - 100.0f, row_y,
                           24.0f, 20.0f) && ei > 0u) {
          StygianEditorNodeEffect tmp_e = effects[ei - 1u];
          StygianEditorEffectTransform tmp_x = effect_xforms[ei - 1u];
          effects[ei - 1u] = effects[ei];
          effect_xforms[ei - 1u] = effect_xforms[ei];
          effects[ei] = tmp_e;
          effect_xforms[ei] = tmp_x;
          if (stygian_editor_node_set_effects(app->editor, selected, effects, effect_count)) {
            for (uint32_t ti = 0u; ti < effect_count; ++ti) {
              (void)stygian_editor_node_set_effect_transform(app->editor, selected, ti,
                                                             &effect_xforms[ti]);
            }
            app->fx_selected_effect_index = ei - 1u;
            mutated = true;
          }
        }
        if (stygian_button(app->ctx, app->font, "v", x + content_w - 72.0f, row_y,
                           24.0f, 20.0f) && ei + 1u < effect_count) {
          StygianEditorNodeEffect tmp_e = effects[ei + 1u];
          StygianEditorEffectTransform tmp_x = effect_xforms[ei + 1u];
          effects[ei + 1u] = effects[ei];
          effect_xforms[ei + 1u] = effect_xforms[ei];
          effects[ei] = tmp_e;
          effect_xforms[ei] = tmp_x;
          if (stygian_editor_node_set_effects(app->editor, selected, effects, effect_count)) {
            for (uint32_t ti = 0u; ti < effect_count; ++ti) {
              (void)stygian_editor_node_set_effect_transform(app->editor, selected, ti,
                                                             &effect_xforms[ti]);
            }
            app->fx_selected_effect_index = ei + 1u;
            mutated = true;
          }
        }
        if (stygian_button(app->ctx, app->font, "x", x + content_w - 44.0f, row_y,
                           24.0f, 20.0f)) {
          for (uint32_t ti = ei + 1u; ti < effect_count; ++ti) {
            effects[ti - 1u] = effects[ti];
            effect_xforms[ti - 1u] = effect_xforms[ti];
          }
          if (effect_count > 0u)
            effect_count -= 1u;
          if (stygian_editor_node_set_effects(app->editor, selected, effects, effect_count)) {
            for (uint32_t ti = 0u; ti < effect_count; ++ti) {
              (void)stygian_editor_node_set_effect_transform(app->editor, selected, ti,
                                                             &effect_xforms[ti]);
            }
            if (app->fx_selected_effect_index >= effect_count && effect_count > 0u)
              app->fx_selected_effect_index = effect_count - 1u;
            mutated = true;
          }
          break;
        }
        row_y += 24.0f;
      }

      if (effect_count > 0u && app->fx_selected_effect_index < effect_count) {
        StygianEditorNodeEffect effect = effects[app->fx_selected_effect_index];
        StygianEditorEffectTransform xform =
            effect_xforms[app->fx_selected_effect_index];
        float edit_y = row_y + 2.0f;
        int kind_choice = (int)effect.kind;
        stygian_text(app->ctx, app->font, "Selected effect", x + 10.0f, edit_y,
                     11.0f, 0.86f, 0.90f, 0.96f, 1.0f);
        edit_y += 18.0f;
        if (stygian_radio_button(app->ctx, app->font, "Drop", x + 10.0f, edit_y,
                                 &kind_choice, STYGIAN_EDITOR_EFFECT_DROP_SHADOW) ||
            stygian_radio_button(app->ctx, app->font, "Inner", x + 68.0f, edit_y,
                                 &kind_choice, STYGIAN_EDITOR_EFFECT_INNER_SHADOW) ||
            stygian_radio_button(app->ctx, app->font, "Blur", x + 130.0f, edit_y,
                                 &kind_choice, STYGIAN_EDITOR_EFFECT_LAYER_BLUR) ||
            stygian_radio_button(app->ctx, app->font, "Glow", x + 186.0f, edit_y,
                                 &kind_choice, STYGIAN_EDITOR_EFFECT_GLOW) ||
            stygian_radio_button(app->ctx, app->font, "Noise", x + 246.0f, edit_y,
                                 &kind_choice, STYGIAN_EDITOR_EFFECT_NOISE)) {
          effect.kind = (StygianEditorEffectKind)kind_choice;
          effects[app->fx_selected_effect_index] = effect;
          (void)stygian_editor_node_set_effects(app->editor, selected, effects, effect_count);
          for (uint32_t ti = 0u; ti < effect_count; ++ti)
            (void)stygian_editor_node_set_effect_transform(app->editor, selected, ti,
                                                           &effect_xforms[ti]);
          mutated = true;
        }
        edit_y += 24.0f;
        if (stygian_checkbox(app->ctx, app->font, "Visible", x + 10.0f, edit_y,
                             &effect.visible)) {
          effects[app->fx_selected_effect_index] = effect;
          (void)stygian_editor_node_set_effects(app->editor, selected, effects, effect_count);
          for (uint32_t ti = 0u; ti < effect_count; ++ti)
            (void)stygian_editor_node_set_effect_transform(app->editor, selected, ti,
                                                           &effect_xforms[ti]);
          mutated = true;
        }
        if (stygian_slider(app->ctx, x + 84.0f, edit_y, content_w - 94.0f, 14.0f,
                           &effect.opacity, 0.0f, 1.0f)) {
          effects[app->fx_selected_effect_index] = effect;
          (void)stygian_editor_node_set_effects(app->editor, selected, effects, effect_count);
          for (uint32_t ti = 0u; ti < effect_count; ++ti)
            (void)stygian_editor_node_set_effect_transform(app->editor, selected, ti,
                                                           &effect_xforms[ti]);
          mutated = true;
        }
        edit_y += 22.0f;
        if (stygian_slider(app->ctx, x + 64.0f, edit_y, content_w - 74.0f, 14.0f,
                           &effect.radius, 0.0f, 96.0f)) {
          effects[app->fx_selected_effect_index] = effect;
          (void)stygian_editor_node_set_effects(app->editor, selected, effects, effect_count);
          for (uint32_t ti = 0u; ti < effect_count; ++ti)
            (void)stygian_editor_node_set_effect_transform(app->editor, selected, ti,
                                                           &effect_xforms[ti]);
          mutated = true;
        }
        edit_y += 20.0f;
        if (stygian_slider(app->ctx, x + 64.0f, edit_y, content_w - 74.0f, 14.0f,
                           &effect.spread, -48.0f, 48.0f)) {
          effects[app->fx_selected_effect_index] = effect;
          (void)stygian_editor_node_set_effects(app->editor, selected, effects, effect_count);
          for (uint32_t ti = 0u; ti < effect_count; ++ti)
            (void)stygian_editor_node_set_effect_transform(app->editor, selected, ti,
                                                           &effect_xforms[ti]);
          mutated = true;
        }
        edit_y += 20.0f;
        if (stygian_slider(app->ctx, x + 64.0f, edit_y, content_w - 74.0f, 14.0f,
                           &effect.intensity, 0.0f, 4.0f)) {
          effects[app->fx_selected_effect_index] = effect;
          (void)stygian_editor_node_set_effects(app->editor, selected, effects, effect_count);
          for (uint32_t ti = 0u; ti < effect_count; ++ti)
            (void)stygian_editor_node_set_effect_transform(app->editor, selected, ti,
                                                           &effect_xforms[ti]);
          mutated = true;
        }
        edit_y += 20.0f;
        if (stygian_slider(app->ctx, x + 64.0f, edit_y, (content_w - 80.0f) * 0.5f,
                           14.0f, &effect.offset_x, -128.0f, 128.0f) ||
            stygian_slider(app->ctx, x + 74.0f + (content_w - 80.0f) * 0.5f, edit_y,
                           (content_w - 80.0f) * 0.5f - 10.0f, 14.0f,
                           &effect.offset_y, -128.0f, 128.0f)) {
          effects[app->fx_selected_effect_index] = effect;
          (void)stygian_editor_node_set_effects(app->editor, selected, effects, effect_count);
          for (uint32_t ti = 0u; ti < effect_count; ++ti)
            (void)stygian_editor_node_set_effect_transform(app->editor, selected, ti,
                                                           &effect_xforms[ti]);
          mutated = true;
        }
        edit_y += 22.0f;
        if (stygian_slider(app->ctx, x + 64.0f, edit_y, 82.0f, 14.0f,
                           &xform.scale_x, 0.1f, 4.0f) ||
            stygian_slider(app->ctx, x + 154.0f, edit_y, 82.0f, 14.0f,
                           &xform.scale_y, 0.1f, 4.0f) ||
            stygian_slider(app->ctx, x + 244.0f, edit_y, content_w - 254.0f, 14.0f,
                           &xform.rotation_deg, -180.0f, 180.0f)) {
          effect_xforms[app->fx_selected_effect_index] = xform;
          (void)stygian_editor_node_set_effect_transform(
              app->editor, selected, app->fx_selected_effect_index, &xform);
          mutated = true;
        }
      }
    }

    stygian_text(app->ctx, app->font, "Mask / clip ownership", x + 10.0f, y + 282.0f,
                 11.0f, 0.86f, 0.90f, 0.96f, 1.0f);
    stygian_text_input(app->ctx, app->font, x + 10.0f, y + 300.0f, 72.0f, 20.0f,
                       app->mask_node_id_text, (int)sizeof(app->mask_node_id_text));
    if (stygian_radio_button(app->ctx, app->font, "Alpha", x + 88.0f, y + 302.0f,
                             (int *)&mask_mode, STYGIAN_EDITOR_MASK_ALPHA) ||
        stygian_radio_button(app->ctx, app->font, "Luma", x + 146.0f, y + 302.0f,
                             (int *)&mask_mode, STYGIAN_EDITOR_MASK_LUMINANCE) ||
        stygian_checkbox(app->ctx, app->font, "Invert", x + 208.0f, y + 300.0f,
                         &mask_invert)) {
      uint32_t parsed_mask = STYGIAN_EDITOR_INVALID_ID;
      if (!app->mask_node_id_text[0] ||
          !app_parse_u32_or_default(app->mask_node_id_text, STYGIAN_EDITOR_INVALID_ID,
                                    &parsed_mask)) {
        parsed_mask = STYGIAN_EDITOR_INVALID_ID;
      }
      if (stygian_editor_node_set_mask(app->editor, selected, parsed_mask, mask_mode,
                                       mask_invert)) {
        mutated = true;
      }
    }

    stygian_text(app->ctx, app->font, "Shader slot", x + 10.0f, y + 328.0f, 11.0f,
                 0.86f, 0.90f, 0.96f, 1.0f);
    if (stygian_checkbox(app->ctx, app->font, "Enabled", x + 88.0f, y + 326.0f,
                         &app->shader_enabled)) {
      shader.enabled = app->shader_enabled;
      snprintf(shader.slot_name, sizeof(shader.slot_name), "%s", app->shader_slot_text);
      snprintf(shader.asset_path, sizeof(shader.asset_path), "%s",
               app->shader_asset_path_text);
      snprintf(shader.entry_point, sizeof(shader.entry_point), "%s",
               app->shader_entry_point_text);
      if (stygian_editor_node_set_shader_attachment(app->editor, selected, &shader))
        mutated = true;
    }
    stygian_text_input(app->ctx, app->font, x + 10.0f, y + 350.0f, 72.0f, 20.0f,
                       app->shader_slot_text, (int)sizeof(app->shader_slot_text));
    stygian_text_input(app->ctx, app->font, x + 88.0f, y + 350.0f, content_w - 166.0f,
                       20.0f, app->shader_asset_path_text,
                       (int)sizeof(app->shader_asset_path_text));
    stygian_text_input(app->ctx, app->font, x + content_w - 72.0f, y + 350.0f, 62.0f,
                       20.0f, app->shader_entry_point_text,
                       (int)sizeof(app->shader_entry_point_text));
    if (stygian_button(app->ctx, app->font, "Apply Shader", x + 10.0f, y + 374.0f,
                       88.0f, 20.0f)) {
      shader.enabled = app->shader_enabled;
      snprintf(shader.slot_name, sizeof(shader.slot_name), "%s", app->shader_slot_text);
      snprintf(shader.asset_path, sizeof(shader.asset_path), "%s",
               app->shader_asset_path_text);
      snprintf(shader.entry_point, sizeof(shader.entry_point), "%s",
               app->shader_entry_point_text);
      if (stygian_editor_node_set_shader_attachment(app->editor, selected, &shader)) {
        app_set_status(app, "Shader attachment updated.");
        mutated = true;
      } else {
        app_set_status(app, "Shader attachment update failed.");
      }
    }
    y += card_h + 10.0f;
  }
  return mutated;
}

static bool app_draw_timeline_panel(EditorBootstrapApp *app, float panel_x,
                                    float panel_y, float panel_w,
                                    float panel_h) {
  float x = panel_x + 10.0f;
  float y = panel_y + 8.0f;
  float content_w = panel_w - 20.0f;
  bool mutated = false;
  uint32_t track_count = stygian_editor_timeline_track_count(app->editor);
  uint32_t clip_count = stygian_editor_timeline_clip_count(app->editor);
  StygianEditorTimelineTrack selected_track = {0};
  StygianEditorTimelineClip selected_clip = {0};
  bool have_track = false;
  bool have_clip = false;
  uint32_t max_time_ms = 1000u;
  uint32_t playhead_ms = 0u;
  uint32_t visible_start_ms = 0u;
  uint32_t visible_span_ms = 1000u;
  float workspace_h = panel_h > 230.0f ? 118.0f : 94.0f;
  float lane_h = 20.0f;
  float label_w = content_w > 420.0f ? 124.0f : 96.0f;
  float curve_w = content_w > 500.0f ? 156.0f : 0.0f;
  float lane_w = content_w - label_w - curve_w - 30.0f;
  float ctrl_y;

  app_draw_panel_shell(app, panel_x, panel_y, panel_w, panel_h, 0.63f, 0.48f,
                       0.22f);
  stygian_widgets_register_region(
      panel_x, panel_y, panel_w, panel_h,
      STYGIAN_WIDGET_REGION_POINTER_LEFT_MUTATES |
          STYGIAN_WIDGET_REGION_POINTER_RIGHT_MUTATES |
          STYGIAN_WIDGET_REGION_SCROLL);

  if (lane_w < 90.0f)
    lane_w = 90.0f;

  if (app->timeline_selected_track_index >= track_count && track_count > 0u) {
    app->timeline_selected_track_index = track_count - 1u;
    app->timeline_selected_keyframe_index = 0u;
  }
  if (app->timeline_selected_clip_index >= clip_count && clip_count > 0u) {
    app->timeline_selected_clip_index = clip_count - 1u;
  }
  if (track_count > 0u) {
    have_track = stygian_editor_timeline_get_track(
        app->editor, app->timeline_selected_track_index, &selected_track);
    if (have_track &&
        app->timeline_selected_keyframe_index >= selected_track.keyframe_count &&
        selected_track.keyframe_count > 0u) {
      app->timeline_selected_keyframe_index = selected_track.keyframe_count - 1u;
    }
  }
  if (clip_count > 0u) {
    have_clip = stygian_editor_timeline_get_clip(app->editor,
                                                 app->timeline_selected_clip_index,
                                                 &selected_clip);
  }

  for (uint32_t i = 0u; i < track_count; ++i) {
    StygianEditorTimelineTrack t = {0};
    if (!stygian_editor_timeline_get_track(app->editor, i, &t))
      continue;
    if (t.keyframe_count > 0u &&
        t.keyframes[t.keyframe_count - 1u].time_ms > max_time_ms) {
      max_time_ms = t.keyframes[t.keyframe_count - 1u].time_ms;
    }
  }
  for (uint32_t i = 0u; i < clip_count; ++i) {
    StygianEditorTimelineClip c = {0};
    uint32_t end_ms = 0u;
    if (!stygian_editor_timeline_get_clip(app->editor, i, &c))
      continue;
    end_ms = c.start_ms + c.duration_ms;
    if (end_ms > max_time_ms)
      max_time_ms = end_ms;
  }
  if (!app_parse_u32_or_default(app->timeline_cursor_ms, 0u, &playhead_ms))
    playhead_ms = 0u;
  visible_span_ms = app_timeline_visible_span_ms(app, max_time_ms);
  visible_start_ms = app_timeline_visible_start_ms(app, max_time_ms);
  app->layout.timeline_max_time_ms = max_time_ms;

  if (app->font) {
    char hdr[96];
    char subhdr[160];
    snprintf(hdr, sizeof(hdr), "Timeline (%u tracks | %u clips)", track_count,
             clip_count);
    snprintf(subhdr, sizeof(subhdr), "Playhead %ums | property %s | view %u..%u",
             playhead_ms,
             app_timeline_property_label_from_choice(
                 have_track ? (int)selected_track.property
                            : app->timeline_track_property_choice),
             visible_start_ms, visible_start_ms + visible_span_ms);
    stygian_text(app->ctx, app->font, hdr, x, y, 16.0f, 0.93f, 0.95f, 1.0f,
                 1.0f);
    stygian_text(app->ctx, app->font, subhdr, x, y + 16.0f, 11.0f, 0.78f, 0.85f,
                 0.93f, 1.0f);
  }
  if (stygian_button(app->ctx, app->font, "|<", x + content_w - 162.0f, y + 2.0f,
                     24.0f, 20.0f)) {
    app_timeline_set_cursor(app, 0u);
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, "-100", x + content_w - 132.0f,
                     y + 2.0f, 42.0f, 20.0f)) {
    mutated |= app_timeline_step_cursor(app, -100);
  }
  if (stygian_button(app->ctx, app->font, "+100", x + content_w - 84.0f,
                     y + 2.0f, 42.0f, 20.0f)) {
    mutated |= app_timeline_step_cursor(app, 100);
  }
  if (stygian_button(app->ctx, app->font, "-", x + content_w - 62.0f,
                     y + 2.0f, 22.0f, 20.0f)) {
    app->timeline_view_span_ms = app_timeline_visible_span_ms(app, max_time_ms) * 2u;
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, "+", x + content_w - 36.0f,
                     y + 2.0f, 22.0f, 20.0f)) {
    uint32_t span = app_timeline_visible_span_ms(app, max_time_ms);
    app->timeline_view_span_ms = span > 250u ? span / 2u : 250u;
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, "Fit", x + content_w - 88.0f,
                     y + 2.0f, 22.0f, 20.0f)) {
    app->timeline_view_start_ms = 0u;
    app->timeline_view_span_ms = max_time_ms > 1000u ? max_time_ms : 1000u;
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, "Key", x + content_w - 114.0f,
                     y + 2.0f, 22.0f, 20.0f) &&
      have_track && selected_track.keyframe_count > 0u &&
      app->timeline_selected_keyframe_index < selected_track.keyframe_count) {
    app_timeline_set_cursor(
        app,
        selected_track.keyframes[app->timeline_selected_keyframe_index].time_ms);
    mutated = true;
  }
  y += 30.0f;

  app_draw_panel_card(app, x, y, content_w, workspace_h);
  {
    float lane_x = x + 12.0f + label_w;
    float lane_y = y + 24.0f;
    float ruler_y = y + 10.0f;
    float curve_x = lane_x + lane_w + 10.0f;
    app->layout.timeline_lane_x = lane_x;
    app->layout.timeline_lane_y = y + 8.0f;
    app->layout.timeline_lane_w = lane_w;
    app->layout.timeline_lane_h = workspace_h - 16.0f;
    app->layout.timeline_curve_x = curve_x + 8.0f;
    app->layout.timeline_curve_y = y + 28.0f;
    app->layout.timeline_curve_w = curve_w > 0.0f ? curve_w - 26.0f : 0.0f;
    app->layout.timeline_curve_h = workspace_h - 42.0f;

    if (app->font) {
      char label[40];
      for (uint32_t t = 0u; t < 5u; ++t) {
        float fx = lane_x + lane_w * ((float)t / 4.0f);
        uint32_t stamp = visible_start_ms +
                         (uint32_t)((float)visible_span_ms * ((float)t / 4.0f));
        snprintf(label, sizeof(label), "%u",
                 stamp);
        stygian_text(app->ctx, app->font, label, fx - 8.0f, ruler_y, 9.5f, 0.69f,
                     0.78f, 0.88f, 1.0f);
        app_draw_divider(app, fx, ruler_y + 14.0f, fx, y + workspace_h - 10.0f,
                         0.40f);
      }
    }

    for (uint32_t i = 0u; i < track_count && i < 5u; ++i) {
      StygianEditorTimelineTrack t = {0};
      float row_y = lane_y + lane_h * (float)i;
      char row_label[80];
      bool selected_row = (have_track && i == app->timeline_selected_track_index);
      if (!stygian_editor_timeline_get_track(app->editor, i, &t))
        continue;
      stygian_rect_rounded(app->ctx, x + 8.0f, row_y - 1.0f, content_w - 16.0f,
                           lane_h - 1.0f, selected_row ? 0.10f : 0.07f,
                           selected_row ? 0.16f : 0.09f,
                           selected_row ? 0.24f : 0.13f, 0.92f, 4.0f);
      snprintf(row_label, sizeof(row_label), "#%u %s", t.id,
               t.name[0] ? t.name : "track");
      if (stygian_button(app->ctx, app->font, row_label, x + 12.0f, row_y,
                         label_w - 8.0f, 16.0f)) {
        app->timeline_selected_track_index = i;
        app->timeline_selected_keyframe_index = 0u;
        mutated = true;
      }
      for (uint32_t cidx = 0u; cidx < clip_count; ++cidx) {
        StygianEditorTimelineClip c = {0};
        bool clip_uses_track = false;
        if (!stygian_editor_timeline_get_clip(app->editor, cidx, &c))
          continue;
        for (uint32_t link = 0u; link < c.track_count; ++link) {
          if (c.track_ids[link] == t.id) {
            clip_uses_track = true;
            break;
          }
        }
        if (clip_uses_track) {
          float clip_x =
              app_timeline_time_to_view_x(c.start_ms, visible_start_ms, visible_span_ms,
                                          lane_x, lane_w);
          float clip_w = lane_w * ((float)c.duration_ms / (float)visible_span_ms);
          if (clip_w < 8.0f)
            clip_w = 8.0f;
          if (c.start_ms + c.duration_ms < visible_start_ms ||
              c.start_ms > visible_start_ms + visible_span_ms)
            continue;
          stygian_rect_rounded(app->ctx, clip_x, row_y + 2.0f, clip_w, 11.0f,
                               0.21f, 0.35f, 0.52f, 0.92f, 3.0f);
        }
      }
      for (uint32_t k = 0u; k < t.keyframe_count; ++k) {
        float key_x;
        bool selected_key =
            selected_row && k == app->timeline_selected_keyframe_index;
        if (t.keyframes[k].time_ms < visible_start_ms ||
            t.keyframes[k].time_ms > visible_start_ms + visible_span_ms) {
          continue;
        }
        key_x = app_timeline_time_to_view_x(t.keyframes[k].time_ms, visible_start_ms,
                                            visible_span_ms, lane_x, lane_w);
        stygian_rect_rounded(app->ctx, key_x - 3.0f, row_y + 3.0f, 6.0f, 10.0f,
                             selected_key ? 0.98f : 0.95f,
                             selected_key ? 0.68f : 0.83f,
                             selected_key ? 0.34f : 0.53f, 0.98f, 2.0f);
      }
    }

    {
      float playhead_x = app_timeline_time_to_view_x(
          playhead_ms, visible_start_ms, visible_span_ms, lane_x, lane_w);
      app_draw_divider(app, playhead_x, y + 8.0f, playhead_x,
                       y + workspace_h - 8.0f, 0.95f);
    }

    if (curve_w > 0.0f) {
      app_draw_panel_card(app, curve_x, y + 8.0f, curve_w - 10.0f,
                          workspace_h - 16.0f);
      if (app->font) {
        stygian_text(app->ctx, app->font, "Curve", curve_x + 8.0f, y + 16.0f,
                     11.0f, 0.84f, 0.90f, 0.96f, 1.0f);
      }
      if (have_track && selected_track.keyframe_count >= 2u) {
        float graph_x = curve_x + 8.0f;
        float graph_y = y + 28.0f;
        float graph_w = curve_w - 26.0f;
        float graph_h = workspace_h - 42.0f;
        float min_v = selected_track.keyframes[0].value;
        float max_v = min_v;
        for (uint32_t k = 1u; k < selected_track.keyframe_count; ++k) {
          if (selected_track.keyframes[k].value < min_v)
            min_v = selected_track.keyframes[k].value;
          if (selected_track.keyframes[k].value > max_v)
            max_v = selected_track.keyframes[k].value;
        }
        if (fabsf(max_v - min_v) < 0.0001f)
          max_v = min_v + 1.0f;
        for (uint32_t k = 1u; k < selected_track.keyframe_count; ++k) {
          float x0 = app_timeline_time_to_view_x(selected_track.keyframes[k - 1u].time_ms,
                                                 visible_start_ms, visible_span_ms,
                                                 graph_x, graph_w);
          float x1 = app_timeline_time_to_view_x(selected_track.keyframes[k].time_ms,
                                                 visible_start_ms, visible_span_ms,
                                                 graph_x, graph_w);
          float y0 = graph_y + graph_h -
                     graph_h * ((selected_track.keyframes[k - 1u].value - min_v) /
                                (max_v - min_v));
          float y1 = graph_y + graph_h -
                     graph_h * ((selected_track.keyframes[k].value - min_v) /
                                (max_v - min_v));
          stygian_line(app->ctx, x0, y0, x1, y1, 1.4f, 0.27f, 0.80f, 0.98f,
                       1.0f);
        }
        for (uint32_t k = 0u; k < selected_track.keyframe_count; ++k) {
          float px = app_timeline_time_to_view_x(selected_track.keyframes[k].time_ms,
                                                 visible_start_ms, visible_span_ms,
                                                 graph_x, graph_w);
          float py = graph_y + graph_h -
                     graph_h * ((selected_track.keyframes[k].value - min_v) /
                                (max_v - min_v));
          bool hot = k == app->timeline_selected_keyframe_index;
          stygian_rect_rounded(app->ctx, px - 3.0f, py - 3.0f, 6.0f, 6.0f,
                               hot ? 0.97f : 0.82f, hot ? 0.72f : 0.62f,
                               hot ? 0.34f : 0.50f, 1.0f, 3.0f);
        }
      } else if (app->font) {
        stygian_text(app->ctx, app->font, "Select a track with two keys.",
                     curve_x + 8.0f, y + 36.0f, 10.2f, 0.64f, 0.72f, 0.83f,
                     1.0f);
      }
    }
  }
  y += workspace_h + 10.0f;

  app_draw_panel_card(app, x, y, content_w, 220.0f);
  ctrl_y = y + 10.0f;
  if (app->font) {
    stygian_text(app->ctx, app->font, "Motion Controllers", x + 10.0f,
                 ctrl_y, 12.5f, 0.90f, 0.94f, 1.0f, 1.0f);
    stygian_text(app->ctx, app->font,
                 "Operators build editable keyframe systems. Presets are just shortcuts into the same lane.",
                 x + 10.0f, ctrl_y + 14.0f, 10.0f, 0.68f, 0.76f, 0.86f, 1.0f);
  }
  ctrl_y += 32.0f;

  (void)stygian_radio_button(app->ctx, app->font, "Loop", x + 10.0f, ctrl_y,
                             &app->motion_operator_choice, 0);
  (void)stygian_radio_button(app->ctx, app->font, "PingPong", x + 70.0f, ctrl_y,
                             &app->motion_operator_choice, 1);
  (void)stygian_radio_button(app->ctx, app->font, "Stagger", x + 150.0f, ctrl_y,
                             &app->motion_operator_choice, 2);
  (void)stygian_radio_button(app->ctx, app->font, "Delay", x + 228.0f, ctrl_y,
                             &app->motion_operator_choice, 3);
  (void)stygian_radio_button(app->ctx, app->font, "RepOff", x + 290.0f, ctrl_y,
                             &app->motion_operator_choice, 4);
  ctrl_y += 22.0f;

  stygian_text_input(app->ctx, app->font, x + 10.0f, ctrl_y, 66.0f, 20.0f,
                     app->motion_start_value, (int)sizeof(app->motion_start_value));
  stygian_text_input(app->ctx, app->font, x + 82.0f, ctrl_y, 66.0f, 20.0f,
                     app->motion_end_value, (int)sizeof(app->motion_end_value));
  stygian_text_input(app->ctx, app->font, x + 154.0f, ctrl_y, 56.0f, 20.0f,
                     app->motion_duration_ms, (int)sizeof(app->motion_duration_ms));
  stygian_text_input(app->ctx, app->font, x + 216.0f, ctrl_y, 44.0f, 20.0f,
                     app->motion_cycles, (int)sizeof(app->motion_cycles));
  stygian_text_input(app->ctx, app->font, x + 266.0f, ctrl_y, 50.0f, 20.0f,
                     app->motion_step_ms, (int)sizeof(app->motion_step_ms));
  if (stygian_button(app->ctx, app->font, "Apply", x + 322.0f, ctrl_y,
                     60.0f, 20.0f)) {
    if (app_apply_motion_operator(app, app->motion_operator_choice)) {
      app_set_status(app, "Applied motion controller.");
      mutated = true;
    } else {
      app_set_status(app, "Motion controller apply failed.");
    }
  }
  ctrl_y += 28.0f;

  if (stygian_button(app->ctx, app->font, "Pulse", x + 10.0f, ctrl_y, 68.0f,
                     20.0f)) {
    if (app_apply_motion_preset(app, "pulse")) {
      app_set_status(app, "Applied pulse motion preset.");
      mutated = true;
    } else {
      app_set_status(app, "Pulse preset needs a valid selection.");
    }
  }
  if (stygian_button(app->ctx, app->font, "Slide", x + 84.0f, ctrl_y, 68.0f,
                     20.0f)) {
    if (app_apply_motion_preset(app, "slide")) {
      app_set_status(app, "Applied slide motion preset.");
      mutated = true;
    } else {
      app_set_status(app, "Slide preset needs a valid selection.");
    }
  }
  if (stygian_button(app->ctx, app->font, "Spin", x + 158.0f, ctrl_y, 68.0f,
                     20.0f)) {
    if (app_apply_motion_preset(app, "spin")) {
      app_set_status(app, "Applied spin motion preset.");
      mutated = true;
    } else {
      app_set_status(app, "Spin preset needs a valid selection.");
    }
  }
  ctrl_y += 28.0f;

  stygian_text_input(app->ctx, app->font, x + 10.0f, ctrl_y, 104.0f, 20.0f,
                     app->driver_variable_name,
                     (int)sizeof(app->driver_variable_name));
  stygian_text_input(app->ctx, app->font, x + 120.0f, ctrl_y, 56.0f, 20.0f,
                     app->driver_source_node_id,
                     (int)sizeof(app->driver_source_node_id));
  if (stygian_button(app->ctx, app->font, "Use Sel Src", x + 182.0f, ctrl_y,
                     76.0f, 20.0f)) {
    StygianEditorNodeId sel = stygian_editor_selected_node(app->editor);
    if (sel != STYGIAN_EDITOR_INVALID_ID) {
      snprintf(app->driver_source_node_id, sizeof(app->driver_source_node_id), "%u",
               sel);
      mutated = true;
    }
  }
  if (stygian_button(app->ctx, app->font, "Bind Driver", x + 264.0f, ctrl_y,
                     92.0f, 20.0f)) {
    if (app_bind_driver_variable(app)) {
      app_set_status(app, "Bound driver variable to source control and selection.");
      mutated = true;
    } else {
      app_set_status(app, "Driver bind failed. Source id, variable, and target selection must be valid.");
    }
  }
  ctrl_y += 24.0f;
  if (app->font) {
    stygian_text(app->ctx, app->font,
                 "Driver path: bind a source control VALUE to a named variable, then bind selection to the same variable.",
                 x + 10.0f, ctrl_y, 10.0f, 0.68f, 0.76f, 0.86f, 1.0f);
  }
  ctrl_y += 18.0f;

  stygian_text_input(app->ctx, app->font, x + 10.0f, ctrl_y, 96.0f, 20.0f,
                     app->timeline_track_name,
                     (int)sizeof(app->timeline_track_name));
  stygian_text_input(app->ctx, app->font, x + 112.0f, ctrl_y, 56.0f, 20.0f,
                     app->timeline_track_target_id,
                     (int)sizeof(app->timeline_track_target_id));
  if (stygian_button(app->ctx, app->font, "Use Sel", x + 174.0f, ctrl_y,
                     64.0f, 20.0f)) {
    StygianEditorNodeId sel = stygian_editor_selected_node(app->editor);
    if (sel != STYGIAN_EDITOR_INVALID_ID) {
      snprintf(app->timeline_track_target_id, sizeof(app->timeline_track_target_id),
               "%u", sel);
      mutated = true;
    }
  }
  if (stygian_button(app->ctx, app->font, "Add Track", x + 244.0f, ctrl_y,
                     76.0f, 20.0f)) {
    StygianEditorTimelineTrack t = {0};
    uint32_t target_id = 0u;
    if (!app_parse_u32(app->timeline_track_target_id, &target_id) ||
        target_id == STYGIAN_EDITOR_INVALID_ID) {
      app_set_status(app, "Timeline track target id invalid.");
    } else {
      t.target_node = target_id;
      t.property =
          app_timeline_property_from_choice(app->timeline_track_property_choice);
      t.layer = 0u;
      snprintf(t.name, sizeof(t.name), "%s",
               app->timeline_track_name[0] ? app->timeline_track_name
                                           : "timeline_track");
      if (stygian_editor_timeline_add_track(app->editor, &t, NULL)) {
        track_count = stygian_editor_timeline_track_count(app->editor);
        if (track_count > 0u)
          app->timeline_selected_track_index = track_count - 1u;
        app->timeline_selected_keyframe_index = 0u;
        app_set_status(app, "Timeline track added.");
        mutated = true;
      } else {
        app_set_status(app, "Timeline track add failed.");
      }
    }
  }
  if (stygian_button(app->ctx, app->font, "Delete", x + 326.0f, ctrl_y,
                     64.0f, 20.0f)) {
    if (have_track &&
        stygian_editor_timeline_remove_track(app->editor, selected_track.id)) {
      track_count = stygian_editor_timeline_track_count(app->editor);
      if (track_count == 0u)
        app->timeline_selected_track_index = 0u;
      else if (app->timeline_selected_track_index >= track_count)
        app->timeline_selected_track_index = track_count - 1u;
      app->timeline_selected_keyframe_index = 0u;
      app_set_status(app, "Timeline track deleted.");
      mutated = true;
    }
  }
  ctrl_y += 24.0f;

  (void)stygian_radio_button(app->ctx, app->font, "X", x + 10.0f, ctrl_y,
                             &app->timeline_track_property_choice, 0);
  (void)stygian_radio_button(app->ctx, app->font, "Y", x + 52.0f, ctrl_y,
                             &app->timeline_track_property_choice, 1);
  (void)stygian_radio_button(app->ctx, app->font, "W", x + 94.0f, ctrl_y,
                             &app->timeline_track_property_choice, 2);
  (void)stygian_radio_button(app->ctx, app->font, "H", x + 136.0f, ctrl_y,
                             &app->timeline_track_property_choice, 3);
  (void)stygian_radio_button(app->ctx, app->font, "Opacity", x + 178.0f,
                             ctrl_y, &app->timeline_track_property_choice, 4);
  ctrl_y += 22.0f;

  stygian_text_input(app->ctx, app->font, x + 10.0f, ctrl_y, 58.0f, 20.0f,
                     app->timeline_cursor_ms,
                     (int)sizeof(app->timeline_cursor_ms));
  stygian_text_input(app->ctx, app->font, x + 74.0f, ctrl_y, 72.0f, 20.0f,
                     app->timeline_key_value,
                     (int)sizeof(app->timeline_key_value));
  if (stygian_button(app->ctx, app->font, "Add Key", x + 152.0f, ctrl_y,
                     66.0f, 20.0f)) {
    if (have_track) {
      uint32_t t_ms = 0u;
      float v = 0.0f;
      if (!app_parse_u32_or_default(app->timeline_cursor_ms, 0u, &t_ms) ||
          !app_parse_float_text(app->timeline_key_value, &v)) {
        app_set_status(app, "Timeline key parse failed.");
      } else if (selected_track.keyframe_count <
                 STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP) {
        StygianEditorTimelineKeyframe keys[STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP];
        uint32_t n = selected_track.keyframe_count;
        for (uint32_t k = 0u; k < n; ++k)
          keys[k] = selected_track.keyframes[k];
        keys[n].time_ms = t_ms;
        keys[n].value = v;
        keys[n].easing = app_easing_from_choice(app->timeline_key_easing_choice);
        if (stygian_editor_timeline_set_track_keyframes(app->editor,
                                                        selected_track.id, keys,
                                                        n + 1u)) {
          app->timeline_selected_keyframe_index = n;
          app_set_status(app, "Timeline keyframe added.");
          mutated = true;
        }
      }
    }
  }
  if (stygian_button(app->ctx, app->font, "Update", x + 224.0f, ctrl_y,
                     62.0f, 20.0f)) {
    if (have_track && selected_track.keyframe_count > 0u &&
        app->timeline_selected_keyframe_index < selected_track.keyframe_count) {
      uint32_t t_ms = 0u;
      float v = 0.0f;
      if (!app_parse_u32_or_default(app->timeline_cursor_ms, 0u, &t_ms) ||
          !app_parse_float_text(app->timeline_key_value, &v)) {
        app_set_status(app, "Timeline key parse failed.");
      } else {
        StygianEditorTimelineKeyframe keys[STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP];
        uint32_t n = selected_track.keyframe_count;
        for (uint32_t k = 0u; k < n; ++k)
          keys[k] = selected_track.keyframes[k];
        keys[app->timeline_selected_keyframe_index].time_ms = t_ms;
        keys[app->timeline_selected_keyframe_index].value = v;
        keys[app->timeline_selected_keyframe_index].easing =
            app_easing_from_choice(app->timeline_key_easing_choice);
        if (stygian_editor_timeline_set_track_keyframes(app->editor,
                                                        selected_track.id, keys,
                                                        n)) {
          app_set_status(app, "Timeline keyframe updated.");
          mutated = true;
        }
      }
    }
  }
  if (stygian_button(app->ctx, app->font, "Delete", x + 292.0f, ctrl_y,
                     62.0f, 20.0f)) {
    if (have_track && selected_track.keyframe_count > 0u &&
        app->timeline_selected_keyframe_index < selected_track.keyframe_count) {
      StygianEditorTimelineKeyframe keys[STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP];
      uint32_t n = 0u;
      for (uint32_t k = 0u; k < selected_track.keyframe_count; ++k) {
        if (k == app->timeline_selected_keyframe_index)
          continue;
        keys[n++] = selected_track.keyframes[k];
      }
      if (stygian_editor_timeline_set_track_keyframes(app->editor,
                                                      selected_track.id, keys,
                                                      n)) {
        if (app->timeline_selected_keyframe_index >= n && n > 0u)
          app->timeline_selected_keyframe_index = n - 1u;
        app_set_status(app, "Timeline keyframe deleted.");
        mutated = true;
      }
    }
  }
  ctrl_y += 24.0f;

  stygian_text_input(app->ctx, app->font, x + 10.0f, ctrl_y, 76.0f, 20.0f,
                     app->timeline_clip_name,
                     (int)sizeof(app->timeline_clip_name));
  stygian_text_input(app->ctx, app->font, x + 92.0f, ctrl_y, 50.0f, 20.0f,
                     app->timeline_clip_start_ms,
                     (int)sizeof(app->timeline_clip_start_ms));
  stygian_text_input(app->ctx, app->font, x + 148.0f, ctrl_y, 56.0f, 20.0f,
                     app->timeline_clip_duration_ms,
                     (int)sizeof(app->timeline_clip_duration_ms));
  stygian_text_input(app->ctx, app->font, x + 210.0f, ctrl_y, 42.0f, 20.0f,
                     app->timeline_clip_layer,
                     (int)sizeof(app->timeline_clip_layer));
  if (stygian_button(app->ctx, app->font, "Add Clip", x + 258.0f, ctrl_y,
                     66.0f, 20.0f)) {
    uint32_t start_ms = 0u;
    uint32_t dur_ms = 0u;
    uint32_t layer = 0u;
    if (!app_parse_u32_or_default(app->timeline_clip_start_ms, 0u, &start_ms) ||
        !app_parse_u32_or_default(app->timeline_clip_duration_ms, 300u, &dur_ms) ||
        !app_parse_u32_or_default(app->timeline_clip_layer, 0u, &layer)) {
      app_set_status(app, "Timeline clip parse failed.");
    } else {
      StygianEditorTimelineClip c = {0};
      c.start_ms = start_ms;
      c.duration_ms = dur_ms;
      c.layer = layer;
      snprintf(c.name, sizeof(c.name), "%s",
               app->timeline_clip_name[0] ? app->timeline_clip_name : "clip");
      if (have_track) {
        c.track_count = 1u;
        c.track_ids[0] = selected_track.id;
      }
      if (stygian_editor_timeline_add_clip(app->editor, &c, NULL)) {
        clip_count = stygian_editor_timeline_clip_count(app->editor);
        if (clip_count > 0u)
          app->timeline_selected_clip_index = clip_count - 1u;
        app_set_status(app, "Timeline clip added.");
        mutated = true;
      }
    }
  }
  if (stygian_button(app->ctx, app->font, "Link Sel", x + 330.0f, ctrl_y,
                     68.0f, 20.0f)) {
    if (have_clip && have_track) {
      StygianEditorTimelineTrackId track_ids[STYGIAN_EDITOR_TIMELINE_CLIP_TRACK_CAP];
      uint32_t n = selected_clip.track_count;
      bool already = false;
      for (uint32_t idx = 0u; idx < n; ++idx)
        track_ids[idx] = selected_clip.track_ids[idx];
      for (uint32_t idx = 0u; idx < n; ++idx) {
        if (track_ids[idx] == selected_track.id) {
          already = true;
          break;
        }
      }
      if (!already && n < STYGIAN_EDITOR_TIMELINE_CLIP_TRACK_CAP)
        track_ids[n++] = selected_track.id;
      if (stygian_editor_timeline_set_clip_tracks(app->editor, selected_clip.id,
                                                  track_ids, n)) {
        app_set_status(app, "Timeline clip track links updated.");
        mutated = true;
      }
    }
  }
  if (stygian_button(app->ctx, app->font, "Delete Clip", x + 404.0f, ctrl_y,
                     82.0f, 20.0f)) {
    if (have_clip &&
        stygian_editor_timeline_remove_clip(app->editor, selected_clip.id)) {
      clip_count = stygian_editor_timeline_clip_count(app->editor);
      if (clip_count == 0u)
        app->timeline_selected_clip_index = 0u;
      else if (app->timeline_selected_clip_index >= clip_count)
        app->timeline_selected_clip_index = clip_count - 1u;
      app_set_status(app, "Timeline clip deleted.");
      mutated = true;
    }
  }
  y += 228.0f;

  if (have_track) {
    for (uint32_t k = 0u; k < selected_track.keyframe_count && k < 4u; ++k) {
      char key_label[96];
      const StygianEditorTimelineKeyframe *kf = &selected_track.keyframes[k];
      snprintf(key_label, sizeof(key_label), "%u ms | %.2f | %s", kf->time_ms,
               kf->value, app_easing_name(kf->easing));
      if (stygian_button(app->ctx, app->font, key_label, x + 10.0f, y,
                         content_w - 20.0f, 18.0f)) {
        app->timeline_selected_keyframe_index = k;
        snprintf(app->timeline_cursor_ms, sizeof(app->timeline_cursor_ms), "%u",
                 kf->time_ms);
        snprintf(app->timeline_key_value, sizeof(app->timeline_key_value), "%.3f",
                 kf->value);
        app->timeline_key_easing_choice = (int)kf->easing;
        mutated = true;
      }
      y += 20.0f;
    }
  }

  for (uint32_t i = 0u; i < clip_count && i < 4u; ++i) {
    StygianEditorTimelineClip c = {0};
    char label[120];
    if (!stygian_editor_timeline_get_clip(app->editor, i, &c))
      continue;
    snprintf(label, sizeof(label), "#%u L%u %s [%u..%u] tracks:%u", c.id, c.layer,
             c.name[0] ? c.name : "clip", c.start_ms, c.start_ms + c.duration_ms,
             c.track_count);
    if (stygian_button(app->ctx, app->font, label, x + 10.0f, y,
                       content_w - 20.0f, 18.0f)) {
      app->timeline_selected_clip_index = i;
      mutated = true;
    }
    y += 20.0f;
  }

  if (app->font) {
    uint32_t overlap_count = 0u;
    char diag[128];
    for (uint32_t i = 0u; i < clip_count; ++i) {
      StygianEditorTimelineClip a = {0};
      if (!stygian_editor_timeline_get_clip(app->editor, i, &a))
        continue;
      for (uint32_t j = i + 1u; j < clip_count; ++j) {
        StygianEditorTimelineClip b = {0};
        uint32_t a_end = a.start_ms + a.duration_ms;
        uint32_t b_end;
        bool overlap;
        if (!stygian_editor_timeline_get_clip(app->editor, j, &b))
          continue;
        b_end = b.start_ms + b.duration_ms;
        overlap = !(a_end <= b.start_ms || b_end <= a.start_ms);
        if (overlap && a.layer == b.layer)
          overlap_count += 1u;
      }
    }
    snprintf(diag, sizeof(diag), "Layer overlap diagnostics: %u conflicts",
             overlap_count);
    stygian_text(app->ctx, app->font, diag, x + 10.0f, y, 11.0f,
                 overlap_count ? 0.94f : 0.78f, overlap_count ? 0.52f : 0.84f,
                 0.90f, 1.0f);
  }
  return mutated;
}

static void app_panel_scene_layers_cb(StygianDockPanel *panel, StygianContext *ctx,
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
  mutated = app_draw_scene_layers_panel(app, x, y, w, h);
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

static void app_panel_assets_cb(StygianDockPanel *panel, StygianContext *ctx,
                                StygianFont font, float x, float y, float w,
                                float h) {
  EditorBootstrapApp *app = g_editor_bootstrap_app;
  bool mutated;
  (void)panel;
  if (!app)
    return;
  app->ctx = ctx;
  app->font = font;
  mutated = app_draw_assets_panel(app, x, y, w, h);
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
  mutated = app_draw_inspector_v2_panel(app, x, y, w, h);
  if (mutated)
    stygian_request_repaint_after_ms(ctx, 0u);
}

static void app_panel_timeline_cb(StygianDockPanel *panel, StygianContext *ctx,
                                  StygianFont font, float x, float y, float w,
                                  float h) {
  EditorBootstrapApp *app = g_editor_bootstrap_app;
  bool mutated;
  (void)panel;
  if (!app)
    return;
  app->ctx = ctx;
  app->font = font;
  app->layout.timeline_x = x;
  app->layout.timeline_y = y;
  app->layout.timeline_w = w;
  app->layout.timeline_h = h;
  mutated = app_draw_timeline_panel(app, x, y, w, h);
  if (mutated)
    stygian_request_repaint_after_ms(ctx, 0u);
}

static void app_panel_logic_cb(StygianDockPanel *panel, StygianContext *ctx,
                               StygianFont font, float x, float y, float w,
                               float h) {
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

static void app_panel_diagnostics_cb(StygianDockPanel *panel, StygianContext *ctx,
                                     StygianFont font, float x, float y, float w,
                                     float h) {
  EditorBootstrapApp *app = g_editor_bootstrap_app;
  bool mutated;
  (void)panel;
  if (!app)
    return;
  app->ctx = ctx;
  app->font = font;
  mutated = app_draw_diagnostics_panel(app, x, y, w, h);
  if (mutated)
    stygian_request_repaint_after_ms(ctx, 0u);
}

static void app_panel_code_insight_cb(StygianDockPanel *panel, StygianContext *ctx,
                                      StygianFont font, float x, float y, float w,
                                      float h) {
  EditorBootstrapApp *app = g_editor_bootstrap_app;
  static char summary_text[4096];
  static char generated_text[262144];
  static char hook_text[131072];
  static StygianTextArea summary_area = {0};
  static StygianTextArea generated_area = {0};
  static StygianTextArea hook_area = {0};
  StygianEditorNodeId selected = STYGIAN_EDITOR_INVALID_ID;
  StygianTextArea *area = NULL;
  const char *open_path = NULL;
  bool mutated = false;
  bool compact_controls = false;
  float tab_w = 68.0f;
  float gen_w = 78.0f;
  float hook_w = 60.0f;
  float action_w = 54.0f;
  float open_w = 52.0f;
  float row_y = 0.0f;
  (void)panel;
  if (!app)
    return;
  app->ctx = ctx;
  app->font = font;
  app_draw_panel_shell(app, x, y, w, h, 0.74f, 0.58f, 0.30f);
  if (!app->font)
    return;
  stygian_text(app->ctx, app->font, "Code", x + 10.0f, y + 8.0f, 16.0f, 0.93f,
               0.95f, 1.0f, 1.0f);
  stygian_text(app->ctx, app->font,
               "Generated files are replaced. Hooks stay yours. Project data stays authored.",
               x + 10.0f, y + 24.0f, 10.5f, 0.74f, 0.82f, 0.92f, 1.0f);
  compact_controls = w < 280.0f;
  if (compact_controls) {
    tab_w = 42.0f;
    gen_w = 38.0f;
    hook_w = 34.0f;
    action_w = 46.0f;
    open_w = 40.0f;
  }
  row_y = y + 42.0f;

  selected = stygian_editor_selected_node(app->editor);
  if (app->code_insight_enabled &&
      selected != STYGIAN_EDITOR_INVALID_ID &&
      app->code_insight_node != selected) {
    app_refresh_code_insight(app, selected);
  }
  snprintf(summary_text, sizeof(summary_text),
           "Summary\n"
           "=======\n"
           "%s\n\n"
           "Generated zone\n"
           "--------------\n"
           "%s\n\n"
           "Hook zone\n"
           "---------\n"
           "%s\n\n"
           "Authored zone\n"
           "------------\n"
           "%s\n\n"
           "Status\n"
           "------\n"
           "%s\n",
           app->code_insight_enabled ? app->code_insight_text
                                     : "Code insight is off. Toggle it from the top strip for per-selection export notes.",
           app_generated_scene_path(), app_hook_source_path(),
           app->project_path[0] ? app->project_path : "in-memory session",
           app->status_text[0] ? app->status_text : "Idle");

  if (summary_area.buffer != summary_text) {
    memset(&summary_area, 0, sizeof(summary_area));
    summary_area.buffer = summary_text;
    summary_area.buffer_size = (int)sizeof(summary_text);
    summary_area.read_only = true;
  }
  stygian_text_area_mark_text_dirty(&summary_area);

  if (generated_area.buffer != generated_text) {
    memset(&generated_area, 0, sizeof(generated_area));
    generated_area.buffer = generated_text;
    generated_area.buffer_size = (int)sizeof(generated_text);
    generated_area.read_only = true;
  }
  if (hook_area.buffer != hook_text) {
    memset(&hook_area, 0, sizeof(hook_area));
    hook_area.buffer = hook_text;
    hook_area.buffer_size = (int)sizeof(hook_text);
    hook_area.read_only = true;
  }

  if (stygian_button(app->ctx, app->font, compact_controls ? "Sur." : "Summary",
                     x + 10.0f, row_y, tab_w,
                     20.0f)) {
    app->code_view_mode = 0;
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, compact_controls ? "Gen." : "Generated",
                     x + 16.0f + tab_w, row_y, gen_w, 20.0f)) {
    app->code_view_mode = 1;
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, compact_controls ? "H." : "Hooks",
                     x + 22.0f + tab_w + gen_w, row_y, hook_w,
                     20.0f)) {
    app->code_view_mode = 2;
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, compact_controls ? "Ref." : "Refresh",
                     x + w - (open_w + action_w + 20.0f), row_y, action_w, 20.0f)) {
    if (selected != STYGIAN_EDITOR_INVALID_ID)
      app_refresh_code_insight(app, selected);
    generated_text[0] = '\0';
    hook_text[0] = '\0';
    stygian_text_area_mark_text_dirty(&generated_area);
    stygian_text_area_mark_text_dirty(&hook_area);
    mutated = true;
  }
  if (stygian_button(app->ctx, app->font, compact_controls ? "Open" : "Open File",
                     x + w - (open_w + 10.0f), row_y, open_w, 20.0f)) {
    open_path = app->code_view_mode == 2 ? app_hook_source_path()
                                         : app_generated_scene_path();
    if (app->code_view_mode == 0)
      open_path = app_generated_scene_path();
    if (app_open_text_file_in_default_editor(open_path))
      app_set_status(app, "Opened source file in default editor.");
    else
      app_set_status(app, "Failed to open source file.");
    mutated = true;
  }

  if (app->code_view_mode == 1 && generated_text[0] == '\0') {
    if (!app_read_text_file(app_generated_scene_path(), generated_text,
                            sizeof(generated_text))) {
      snprintf(generated_text, sizeof(generated_text),
               "Generated file is not available yet.\n\nRun Export Runtime Bundle to populate:\n%s\n",
               app_generated_scene_path());
    }
    stygian_text_area_mark_text_dirty(&generated_area);
  }
  if (app->code_view_mode == 2 && hook_text[0] == '\0') {
    if (!app_read_text_file(app_hook_source_path(), hook_text,
                            sizeof(hook_text))) {
      snprintf(hook_text, sizeof(hook_text),
               "Hook file is not available yet.\n\nExport or run Binding Doctor to populate:\n%s\n",
               app_hook_source_path());
    }
    stygian_text_area_mark_text_dirty(&hook_area);
  }

  area = &summary_area;
  if (app->code_view_mode == 1)
    area = &generated_area;
  else if (app->code_view_mode == 2)
    area = &hook_area;
  area->read_only = true;
  area->x = x + 10.0f;
  area->y = y + 70.0f;
  area->w = w - 20.0f;
  area->h = h - 80.0f;
  (void)stygian_text_area(app->ctx, app->font, area);

  if (mutated)
    stygian_request_repaint_after_ms(ctx, 0u);
}

static bool app_init_dock(EditorBootstrapApp *app) {
  StygianWindow *window;
  StygianDockNode *left = NULL;
  StygianDockNode *right = NULL;
  StygianDockNode *center = NULL;
  StygianDockNode *right_stack = NULL;
  StygianDockNode *left_bottom = NULL;
  StygianDockNode *center_bottom = NULL;

  if (!app || !app->ctx)
    return false;

  window = stygian_get_window(app->ctx);
  app->dock = stygian_dock_create(stygian_window_native_context(window),
                                  stygian_window_native_handle(window));
  if (!app->dock)
    return false;

  app->panel_scene_layers_id = stygian_dock_register_panel(
      app->dock, "Scene/Layers", false, app_panel_scene_layers_cb, NULL);
  app->panel_assets_id = stygian_dock_register_panel(
      app->dock, "Assets", true, app_panel_assets_cb, NULL);
  app->panel_viewport_id = stygian_dock_register_panel(
      app->dock, "Viewport", false, app_panel_viewport_cb, NULL);
  app->panel_inspector_id = stygian_dock_register_panel(
      app->dock, "Inspector", false, app_panel_inspector_cb, NULL);
  app->panel_timeline_id = stygian_dock_register_panel(
      app->dock, "Timeline", false, app_panel_timeline_cb, NULL);
  app->panel_logic_id =
      stygian_dock_register_panel(app->dock, "Logic", false, app_panel_logic_cb, NULL);
  app->panel_diagnostics_id = stygian_dock_register_panel(
      app->dock, "Diagnostics", true, app_panel_diagnostics_cb, NULL);
  app->panel_code_insight_id = stygian_dock_register_panel(
      app->dock, "Code Insight", true, app_panel_code_insight_cb, NULL);

  if (!app->panel_scene_layers_id || !app->panel_assets_id ||
      !app->panel_viewport_id || !app->panel_inspector_id ||
      !app->panel_timeline_id || !app->panel_logic_id ||
      !app->panel_diagnostics_id || !app->panel_code_insight_id) {
    return false;
  }

  {
    StygianDockNode *root = stygian_dock_get_root(app->dock);
    stygian_dock_add_panel_to_node(app->dock, root, app->panel_scene_layers_id);
    stygian_dock_split(app->dock, root, STYGIAN_DOCK_SPLIT_VERTICAL, 0.22f, &left,
                       &right);
    if (!left || !right)
      return false;

    stygian_dock_add_panel_to_node(app->dock, right, app->panel_viewport_id);
    stygian_dock_split(app->dock, right, STYGIAN_DOCK_SPLIT_VERTICAL, 0.74f,
                       &center, &right_stack);
    if (!center || !right_stack)
      return false;
    stygian_dock_split(app->dock, center, STYGIAN_DOCK_SPLIT_HORIZONTAL, 0.74f,
                       &center, &center_bottom);
    if (!center || !center_bottom)
      return false;
    stygian_dock_add_panel_to_node(app->dock, center_bottom, app->panel_timeline_id);

    stygian_dock_add_panel_to_node(app->dock, right_stack, app->panel_inspector_id);
    stygian_dock_add_panel_to_node(app->dock, right_stack, app->panel_logic_id);
    stygian_dock_add_panel_to_node(app->dock, right_stack,
                                   app->panel_diagnostics_id);
    stygian_dock_add_panel_to_node(app->dock, right_stack,
                                   app->panel_code_insight_id);
    stygian_dock_split(app->dock, left, STYGIAN_DOCK_SPLIT_HORIZONTAL, 0.70f, &left,
                       &left_bottom);
    if (left_bottom) {
      stygian_dock_add_panel_to_node(app->dock, left_bottom, app->panel_assets_id);
    }
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
  EditorBootstrapApp *app = NULL;
  StygianEditorConfig editor_cfg;
  StygianEditorHost editor_host;
  bool first_frame = true;
  bool has_repo_paths = false;
  char shader_dir[1024];
  char atlas_png[1024];
  char atlas_json[1024];

  memset(&cfg, 0, sizeof(cfg));
  memset(&editor_host, 0, sizeof(editor_host));
  app = (EditorBootstrapApp *)calloc(1u, sizeof(*app));
  if (!app)
    return 1;

  window = stygian_window_create(&win_cfg);
  if (!window) {
    free(app);
    return 1;
  }

  has_repo_paths =
      app_resolve_repo_resource_paths(shader_dir, sizeof(shader_dir), atlas_png,
                                      sizeof(atlas_png), atlas_json,
                                      sizeof(atlas_json));

  cfg.backend = STYGIAN_EDITOR_BOOTSTRAP_BACKEND;
  cfg.window = window;
  if (has_repo_paths) {
    cfg.shader_dir = shader_dir;
    cfg.default_font_atlas_png = atlas_png;
    cfg.default_font_atlas_json = atlas_json;
  }
  app->ctx = stygian_create(&cfg);
  if (!app->ctx) {
    stygian_window_destroy(window);
    free(app);
    return 1;
  }

  app->font = stygian_get_default_font(app->ctx);
  app->paint_color = stygian_editor_color_rgba(0.16f, 0.61f, 0.95f, 1.0f);
  app->grid_enabled = true;
  app->sub_snap_enabled = true;
  app->major_step_px = 96.0f;
  app->snap_tolerance_px = -1.0f;
  app->camera_zoom = 1.0f;
  app->selected_primitive = APP_PRIMITIVE_RECTANGLE;
  app->current_tool = APP_TOOL_SELECT;
  snprintf(app->token_name, sizeof(app->token_name), "primary");
  app->behavior_event_choice = 0;
  app->behavior_action_choice = 0;
  app->behavior_property_choice = 1;
  app->behavior_easing_choice = 1;
  app->behavior_variable_kind_choice = 0;
  app->behavior_variable_use_active_mode = true;
  snprintf(app->behavior_variable_name, sizeof(app->behavior_variable_name),
           "motion.x");
  snprintf(app->behavior_variable_number, sizeof(app->behavior_variable_number),
           "120");
  snprintf(app->behavior_variable_mode, sizeof(app->behavior_variable_mode), "0");
  snprintf(app->behavior_navigate_target, sizeof(app->behavior_navigate_target),
           "screen://next");
  snprintf(app->repeater_count, sizeof(app->repeater_count), "3");
  snprintf(app->repeater_step_x, sizeof(app->repeater_step_x), "24");
  snprintf(app->repeater_step_y, sizeof(app->repeater_step_y), "24");
  app->boolean_op_choice = 0;
  app->boolean_flatten_after = false;
  app->boolean_request_smooth_fallback = false;
  app->snap_use_grid = true;
  app->snap_use_guides = true;
  app->snap_use_bounds = true;
  app->snap_use_parent_edges = true;
  app->perf_last_load_ms = 0.0f;
  app->perf_last_layout_ms = 0.0f;
  app->perf_last_export_ms = 0.0f;
  app->perf_last_interaction_ms = 0.0f;
  app->scene_parent_focus = STYGIAN_EDITOR_INVALID_ID;
  app->timeline_selected_track_index = 0u;
  app->timeline_selected_clip_index = 0u;
  app->timeline_selected_keyframe_index = 0u;
  app->motion_operator_choice = 0;
  snprintf(app->motion_start_value, sizeof(app->motion_start_value), "0");
  snprintf(app->motion_end_value, sizeof(app->motion_end_value), "120");
  snprintf(app->motion_duration_ms, sizeof(app->motion_duration_ms), "600");
  snprintf(app->motion_cycles, sizeof(app->motion_cycles), "2");
  snprintf(app->motion_step_ms, sizeof(app->motion_step_ms), "120");
  snprintf(app->driver_variable_name, sizeof(app->driver_variable_name),
           "motion.driver");
  snprintf(app->driver_source_node_id, sizeof(app->driver_source_node_id), "0");
  app->driver_target_property_choice = 0;
  snprintf(app->timeline_track_name, sizeof(app->timeline_track_name), "track");
  snprintf(app->timeline_track_target_id, sizeof(app->timeline_track_target_id),
           "0");
  app->timeline_track_property_choice = 0;
  snprintf(app->timeline_cursor_ms, sizeof(app->timeline_cursor_ms), "0");
  snprintf(app->timeline_key_value, sizeof(app->timeline_key_value), "0");
  app->timeline_key_easing_choice = 0;
  snprintf(app->timeline_clip_name, sizeof(app->timeline_clip_name), "clip");
  snprintf(app->timeline_clip_start_ms, sizeof(app->timeline_clip_start_ms), "0");
  snprintf(app->timeline_clip_duration_ms, sizeof(app->timeline_clip_duration_ms),
           "300");
  snprintf(app->timeline_clip_layer, sizeof(app->timeline_clip_layer), "0");
  app->timeline_view_start_ms = 0u;
  app->timeline_view_span_ms = 2000u;
  app->complexity_mode = APP_COMPLEXITY_SYSTEMS;
  app->code_insight_enabled = false;
  app->code_view_mode = 0;
  app->code_insight_node = STYGIAN_EDITOR_INVALID_ID;
  snprintf(app->code_insight_text, sizeof(app->code_insight_text),
           "Code insight is off.");
  snprintf(app->project_path, sizeof(app->project_path),
           "editor/build/stygian_editor_session.project.json");
  app->now_ms = app_now_ms_system();

  editor_cfg = stygian_editor_config_default();
  editor_cfg.max_nodes = 8192u;
  editor_cfg.max_path_points = 65536u;
  editor_cfg.max_behaviors = 4096u;
  editor_host.user_data = app;
  editor_host.now_ms = app_host_now_ms;
  editor_host.log = app_host_log;
  editor_host.request_repaint_hz = app_host_request_repaint_hz;
  app->editor = stygian_editor_create(&editor_cfg, &editor_host);
  if (!app->editor) {
    if (app->font)
      stygian_font_destroy(app->ctx, app->font);
    stygian_destroy(app->ctx);
    stygian_window_destroy(window);
    free(app);
    return 1;
  }
  g_editor_bootstrap_app = app;
  if (!app_init_dock(app)) {
    stygian_editor_destroy(app->editor);
    if (app->font)
      stygian_font_destroy(app->ctx, app->font);
    stygian_destroy(app->ctx);
    stygian_window_destroy(window);
    g_editor_bootstrap_app = NULL;
    free(app);
    return 1;
  }

  {
    int width = 0;
    int height = 0;
    stygian_window_get_size(window, &width, &height);
    app_layout_set_canvas(app, width, height);
    app->camera_pan_x = 24.0f;
    app->camera_pan_y = app_top_chrome_height() + 24.0f;
    app_sync_grid_config(app);
    app_sync_snap_flags_from_editor(app);
    app_sync_editor_viewport(app);
    app_set_status(app, "Canvas ready.");
  }

  while (!stygian_window_should_close(window)) {
    StygianEvent event;
    bool event_mutated = false;
    bool event_requested = false;
    bool event_eval_requested = false;
    uint32_t wait_ms = stygian_next_repaint_wait_ms(app->ctx, 250u);
    int win_w = 0;
    int win_h = 0;

    app->now_ms = app_now_ms_system();
    stygian_window_get_size(window, &win_w, &win_h);
    app_layout_set_canvas(app, win_w, win_h);
    app_sync_editor_viewport(app);

    stygian_widgets_begin_frame(app->ctx);

    while (stygian_window_poll_event(window, &event)) {
      StygianWidgetEventImpact impact =
          stygian_widgets_process_event_ex(app->ctx, &event);
      if (impact & STYGIAN_IMPACT_MUTATED_STATE)
        event_mutated = true;
      if (impact & STYGIAN_IMPACT_REQUEST_REPAINT)
        event_requested = true;
      if (impact & STYGIAN_IMPACT_REQUEST_EVAL)
        event_eval_requested = true;

      if (event.type == STYGIAN_EVENT_CLOSE)
        stygian_window_request_close(window);

      {
        uint64_t interaction_start = app_now_ms_system();
        if (app_handle_command_shortcuts(app, &event))
          event_mutated = true;
        if (app_handle_timeline_event(app, &event))
          event_mutated = true;
        if (app_handle_viewport_event(app, &event))
          event_mutated = true;
        app->perf_last_interaction_ms =
            app_elapsed_ms(interaction_start, app_now_ms_system());
      }
    }

    if (!first_frame && !event_mutated && !event_requested &&
        !event_eval_requested) {
      if (stygian_window_wait_event_timeout(window, &event, wait_ms)) {
        StygianWidgetEventImpact impact =
            stygian_widgets_process_event_ex(app->ctx, &event);
        if (impact & STYGIAN_IMPACT_MUTATED_STATE)
          event_mutated = true;
        if (impact & STYGIAN_IMPACT_REQUEST_REPAINT)
          event_requested = true;
        if (impact & STYGIAN_IMPACT_REQUEST_EVAL)
          event_eval_requested = true;
        if (event.type == STYGIAN_EVENT_CLOSE)
          stygian_window_request_close(window);
        {
          uint64_t interaction_start = app_now_ms_system();
          if (app_handle_command_shortcuts(app, &event))
            event_mutated = true;
          if (app_handle_timeline_event(app, &event))
            event_mutated = true;
          if (app_handle_viewport_event(app, &event))
            event_mutated = true;
          app->perf_last_interaction_ms =
              app_elapsed_ms(interaction_start, app_now_ms_system());
        }
      }
    }

    stygian_editor_tick(app->editor, app->now_ms);
    app_sync_driver_rules(app);

    {
      bool repaint_pending = stygian_has_pending_repaint(app->ctx);
      bool render_frame = first_frame || event_mutated || repaint_pending;
      bool eval_only_frame =
          (!render_frame && (event_requested || event_eval_requested));
      bool frame_mutated = event_mutated;

      if (!render_frame && !eval_only_frame)
        continue;
      first_frame = false;

      stygian_mouse_pos(window, &app->last_mouse_x, &app->last_mouse_y);
      app_layout_set_canvas(app, win_w, win_h);
      stygian_begin_frame_intent(
          app->ctx, win_w, win_h,
          eval_only_frame ? STYGIAN_FRAME_EVAL_ONLY : STYGIAN_FRAME_RENDER);

      if (app->dock) {
        float dock_y = app->layout.top_h;
        float dock_h = (float)win_h - dock_y;
        if (dock_h < 0.0f)
          dock_h = 0.0f;
        stygian_dock_update(app->ctx, app->font, app->dock, 0.0f, dock_y,
                            (float)win_w, dock_h);
        stygian_dock_composite_main(app->dock);
      } else {
        (void)app_draw_viewport(app);
      }
      // Menus and popovers need to land after dock compositing or the dock
      // just paints right over them.
      if (app_draw_top_chrome(app, window, app->last_mouse_x, app->last_mouse_y))
        frame_mutated = true;

      if (frame_mutated) {
        stygian_set_repaint_source(app->ctx, "editor_mutation");
        stygian_request_repaint_after_ms(app->ctx, 0u);
      }

      stygian_widgets_commit_regions();
      stygian_end_frame(app->ctx);
    }
  }

  if (app->editor)
    stygian_editor_destroy(app->editor);
  if (app->dock)
    stygian_dock_destroy(app->dock);
  if (app->font)
    stygian_font_destroy(app->ctx, app->font);
  stygian_destroy(app->ctx);
  stygian_window_destroy(window);
  g_editor_bootstrap_app = NULL;
  free(app);
  return 0;
}
