#ifndef STYGIAN_EDITOR_H
#define STYGIAN_EDITOR_H

#include "stygian.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STYGIAN_EDITOR_INVALID_ID 0u

typedef struct StygianEditor StygianEditor;
typedef uint32_t StygianEditorNodeId;
typedef uint32_t StygianEditorPathId;
typedef uint32_t StygianEditorBehaviorId;

typedef enum StygianEditorLogLevel {
  STYGIAN_EDITOR_LOG_DEBUG = 0,
  STYGIAN_EDITOR_LOG_INFO = 1,
  STYGIAN_EDITOR_LOG_WARN = 2,
  STYGIAN_EDITOR_LOG_ERROR = 3,
} StygianEditorLogLevel;

typedef enum StygianEditorShapeKind {
  STYGIAN_EDITOR_SHAPE_RECT = 0,
  STYGIAN_EDITOR_SHAPE_ELLIPSE = 1,
  STYGIAN_EDITOR_SHAPE_PATH = 2,
} StygianEditorShapeKind;

typedef enum StygianEditorEventKind {
  STYGIAN_EDITOR_EVENT_PRESS = 0,
  STYGIAN_EDITOR_EVENT_RELEASE = 1,
  STYGIAN_EDITOR_EVENT_HOVER_ENTER = 2,
  STYGIAN_EDITOR_EVENT_HOVER_LEAVE = 3,
  STYGIAN_EDITOR_EVENT_DRAG_START = 4,
  STYGIAN_EDITOR_EVENT_DRAG_MOVE = 5,
  STYGIAN_EDITOR_EVENT_DRAG_END = 6,
  STYGIAN_EDITOR_EVENT_SCROLL = 7,
  STYGIAN_EDITOR_EVENT_VALUE_CHANGED = 8,
} StygianEditorEventKind;

typedef enum StygianEditorBehaviorActionKind {
  STYGIAN_EDITOR_ACTION_ANIMATE = 0,
} StygianEditorBehaviorActionKind;

typedef enum StygianEditorPropertyKind {
  STYGIAN_EDITOR_PROP_X = 0,
  STYGIAN_EDITOR_PROP_Y = 1,
  STYGIAN_EDITOR_PROP_WIDTH = 2,
  STYGIAN_EDITOR_PROP_HEIGHT = 3,
  STYGIAN_EDITOR_PROP_OPACITY = 4,
  STYGIAN_EDITOR_PROP_RADIUS_TL = 5,
  STYGIAN_EDITOR_PROP_RADIUS_TR = 6,
  STYGIAN_EDITOR_PROP_RADIUS_BR = 7,
  STYGIAN_EDITOR_PROP_RADIUS_BL = 8,
  STYGIAN_EDITOR_PROP_VALUE = 9,
} StygianEditorPropertyKind;

typedef enum StygianEditorEasing {
  STYGIAN_EDITOR_EASING_LINEAR = 0,
  STYGIAN_EDITOR_EASING_OUT_CUBIC = 1,
  STYGIAN_EDITOR_EASING_IN_OUT_CUBIC = 2,
} StygianEditorEasing;

typedef struct StygianEditorHost {
  void *user_data;
  uint64_t (*now_ms)(void *user_data);
  void (*log)(void *user_data, StygianEditorLogLevel level,
              const char *message);
  void (*request_repaint_hz)(void *user_data, uint32_t hz);
} StygianEditorHost;

typedef struct StygianEditorColor {
  float r;
  float g;
  float b;
  float a;
} StygianEditorColor;

typedef struct StygianEditorGridConfig {
  bool enabled;
  bool sub_snap_enabled;
  float major_step_px;
  uint32_t sub_divisions;
  float min_minor_px;
  float snap_tolerance_px;
} StygianEditorGridConfig;

typedef struct StygianEditorViewport2D {
  float width;
  float height;
  float pan_x;
  float pan_y;
  float zoom;
} StygianEditorViewport2D;

typedef struct StygianEditorGridLevel {
  float step_world;
  float step_screen;
  float alpha;
  bool major;
} StygianEditorGridLevel;

typedef struct StygianEditorPoint {
  float x;
  float y;
} StygianEditorPoint;

typedef struct StygianEditorRectDesc {
  float x;
  float y;
  float w;
  float h;
  float radius[4];
  StygianEditorColor fill;
  bool visible;
  float z;
} StygianEditorRectDesc;

typedef struct StygianEditorEllipseDesc {
  float x;
  float y;
  float w;
  float h;
  StygianEditorColor fill;
  bool visible;
  float z;
} StygianEditorEllipseDesc;

typedef struct StygianEditorPathDesc {
  StygianEditorColor stroke;
  float thickness;
  bool closed;
  bool visible;
  float z;
} StygianEditorPathDesc;

typedef struct StygianEditorAnimateAction {
  StygianEditorNodeId target_node;
  StygianEditorPropertyKind property;
  float from_value;
  float to_value;
  uint32_t duration_ms;
  StygianEditorEasing easing;
} StygianEditorAnimateAction;

typedef struct StygianEditorBehaviorRule {
  StygianEditorNodeId trigger_node;
  StygianEditorEventKind trigger_event;
  StygianEditorBehaviorActionKind action_kind;
  StygianEditorAnimateAction animate;
} StygianEditorBehaviorRule;

typedef struct StygianEditorConfig {
  uint32_t max_nodes;
  uint32_t max_path_points;
  uint32_t max_behaviors;
  uint32_t max_color_tokens;
  StygianEditorGridConfig grid;
} StygianEditorConfig;

StygianEditorConfig stygian_editor_config_default(void);
StygianEditorColor stygian_editor_color_rgba(float r, float g, float b,
                                             float a);

StygianEditor *stygian_editor_create(const StygianEditorConfig *config,
                                     const StygianEditorHost *host);
void stygian_editor_destroy(StygianEditor *editor);
void stygian_editor_reset(StygianEditor *editor);

void stygian_editor_set_host(StygianEditor *editor,
                             const StygianEditorHost *host);

void stygian_editor_set_viewport(StygianEditor *editor,
                                 const StygianEditorViewport2D *viewport);
StygianEditorViewport2D stygian_editor_get_viewport(const StygianEditor *editor);

void stygian_editor_set_grid_config(StygianEditor *editor,
                                    const StygianEditorGridConfig *grid);
StygianEditorGridConfig
stygian_editor_get_grid_config(const StygianEditor *editor);
uint32_t stygian_editor_get_grid_levels(const StygianEditor *editor,
                                        StygianEditorGridLevel *out_levels,
                                        uint32_t max_levels);

void stygian_editor_world_to_view(const StygianEditor *editor, float world_x,
                                  float world_y, float *out_view_x,
                                  float *out_view_y);
void stygian_editor_view_to_world(const StygianEditor *editor, float view_x,
                                  float view_y, float *out_world_x,
                                  float *out_world_y);

float stygian_editor_snap_world_scalar(const StygianEditor *editor,
                                       float world_value);
void stygian_editor_snap_world_point(const StygianEditor *editor,
                                     float world_x, float world_y,
                                     float *out_world_x, float *out_world_y);

StygianEditorNodeId stygian_editor_add_rect(StygianEditor *editor,
                                            const StygianEditorRectDesc *desc);
StygianEditorNodeId
stygian_editor_add_ellipse(StygianEditor *editor,
                           const StygianEditorEllipseDesc *desc);

StygianEditorPathId stygian_editor_path_begin(StygianEditor *editor,
                                              const StygianEditorPathDesc *desc);
bool stygian_editor_path_add_point(StygianEditor *editor, StygianEditorPathId id,
                                   float x, float y, bool snap_to_grid);
StygianEditorNodeId stygian_editor_path_commit(StygianEditor *editor,
                                               StygianEditorPathId id);
void stygian_editor_path_cancel(StygianEditor *editor, StygianEditorPathId id);

uint32_t stygian_editor_node_count(const StygianEditor *editor);
bool stygian_editor_node_set_position(StygianEditor *editor,
                                      StygianEditorNodeId node_id, float x,
                                      float y, bool snap_to_grid);
bool stygian_editor_node_set_size(StygianEditor *editor,
                                  StygianEditorNodeId node_id, float w,
                                  float h);
bool stygian_editor_node_set_color(StygianEditor *editor,
                                   StygianEditorNodeId node_id,
                                   StygianEditorColor color);
bool stygian_editor_node_get_color(const StygianEditor *editor,
                                   StygianEditorNodeId node_id,
                                   StygianEditorColor *out_color);
bool stygian_editor_node_set_opacity(StygianEditor *editor,
                                     StygianEditorNodeId node_id, float opacity);
bool stygian_editor_node_get_shape_kind(const StygianEditor *editor,
                                        StygianEditorNodeId node_id,
                                        StygianEditorShapeKind *out_kind);
bool stygian_editor_node_set_corner_radii(StygianEditor *editor,
                                          StygianEditorNodeId node_id,
                                          float top_left, float top_right,
                                          float bottom_right,
                                          float bottom_left);
bool stygian_editor_node_get_corner_radii(const StygianEditor *editor,
                                          StygianEditorNodeId node_id,
                                          float *out_top_left,
                                          float *out_top_right,
                                          float *out_bottom_right,
                                          float *out_bottom_left);
bool stygian_editor_node_get_bounds(const StygianEditor *editor,
                                    StygianEditorNodeId node_id, float *out_x,
                                    float *out_y, float *out_w, float *out_h);
bool stygian_editor_delete_node(StygianEditor *editor,
                                StygianEditorNodeId node_id);

StygianEditorNodeId stygian_editor_select_at(StygianEditor *editor, float x,
                                             float y, bool additive);
bool stygian_editor_select_node(StygianEditor *editor,
                                StygianEditorNodeId node_id, bool additive);
StygianEditorNodeId stygian_editor_selected_node(const StygianEditor *editor);
uint32_t stygian_editor_selected_count(const StygianEditor *editor);
bool stygian_editor_node_is_selected(const StygianEditor *editor,
                                     StygianEditorNodeId node_id);
uint32_t stygian_editor_selected_nodes(const StygianEditor *editor,
                                       StygianEditorNodeId *out_ids,
                                       uint32_t max_ids);
StygianEditorNodeId stygian_editor_hit_test_at(const StygianEditor *editor,
                                               float x, float y);
uint32_t stygian_editor_select_in_rect(StygianEditor *editor, float x0, float y0,
                                       float x1, float y1, bool additive);

bool stygian_editor_set_color_token(StygianEditor *editor, const char *name,
                                    StygianEditorColor color);
bool stygian_editor_get_color_token(const StygianEditor *editor,
                                    const char *name,
                                    StygianEditorColor *out_color);
bool stygian_editor_apply_color_token(StygianEditor *editor,
                                      StygianEditorNodeId node_id,
                                      const char *name);

bool stygian_editor_add_behavior(StygianEditor *editor,
                                 const StygianEditorBehaviorRule *rule,
                                 StygianEditorBehaviorId *out_behavior_id);
bool stygian_editor_remove_behavior(StygianEditor *editor,
                                    StygianEditorBehaviorId behavior_id);
uint32_t stygian_editor_behavior_count(const StygianEditor *editor);
bool stygian_editor_get_behavior_rule(const StygianEditor *editor,
                                      uint32_t index,
                                      StygianEditorBehaviorId *out_behavior_id,
                                      StygianEditorBehaviorRule *out_rule);
void stygian_editor_trigger_event(StygianEditor *editor,
                                  StygianEditorNodeId trigger_node,
                                  StygianEditorEventKind event_kind);
void stygian_editor_tick(StygianEditor *editor, uint64_t now_ms);

void stygian_editor_render_viewport2d(const StygianEditor *editor,
                                      StygianContext *ctx, bool draw_grid,
                                      bool draw_selection);

size_t stygian_editor_build_c23(const StygianEditor *editor, char *out_code,
                                size_t out_capacity);

#ifdef __cplusplus
}
#endif

#endif // STYGIAN_EDITOR_H
