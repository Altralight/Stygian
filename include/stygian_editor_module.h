#ifndef STYGIAN_EDITOR_MODULE_H
#define STYGIAN_EDITOR_MODULE_H

#include "stygian_editor.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#if defined(STYGIAN_EDITOR_MODULE_BUILD_DLL)
#define STYGIAN_EDITOR_MODULE_API __declspec(dllexport)
#elif defined(STYGIAN_EDITOR_MODULE_USE_DLL)
#define STYGIAN_EDITOR_MODULE_API __declspec(dllimport)
#else
#define STYGIAN_EDITOR_MODULE_API
#endif
#elif defined(__GNUC__) && (__GNUC__ >= 4)
#define STYGIAN_EDITOR_MODULE_API __attribute__((visibility("default")))
#else
#define STYGIAN_EDITOR_MODULE_API
#endif

#define STYGIAN_EDITOR_MODULE_ABI_MAJOR 1u
#define STYGIAN_EDITOR_MODULE_ABI_MINOR 2u
#define STYGIAN_EDITOR_MODULE_ABI_VERSION                                       \
  ((STYGIAN_EDITOR_MODULE_ABI_MAJOR << 16) | STYGIAN_EDITOR_MODULE_ABI_MINOR)

#define STYGIAN_EDITOR_MODULE_ABI_VERSION_MAJOR(v) (((v) >> 16) & 0xFFFFu)
#define STYGIAN_EDITOR_MODULE_ABI_VERSION_MINOR(v) ((v)&0xFFFFu)

typedef struct StygianEditorModuleInfo {
  uint32_t struct_size;
  uint32_t abi_version;
  const char *module_name;
  const char *module_version;
  const char *build_language;
} StygianEditorModuleInfo;

typedef struct StygianEditorModuleApi {
  uint32_t abi_version;
  uint32_t struct_size;
  const char *(*module_name)(void);
  const char *(*module_version)(void);

  StygianEditorConfig (*config_default)(void);
  StygianEditorColor (*color_rgba)(float r, float g, float b, float a);

  StygianEditor *(*create)(const StygianEditorConfig *config,
                           const StygianEditorHost *host);
  void (*destroy)(StygianEditor *editor);
  void (*reset)(StygianEditor *editor);
  void (*set_host)(StygianEditor *editor, const StygianEditorHost *host);

  void (*set_viewport)(StygianEditor *editor,
                       const StygianEditorViewport2D *viewport);
  StygianEditorViewport2D (*get_viewport)(const StygianEditor *editor);

  void (*set_grid_config)(StygianEditor *editor,
                          const StygianEditorGridConfig *grid);
  StygianEditorGridConfig (*get_grid_config)(const StygianEditor *editor);
  uint32_t (*get_grid_levels)(const StygianEditor *editor,
                              StygianEditorGridLevel *out_levels,
                              uint32_t max_levels);

  void (*world_to_view)(const StygianEditor *editor, float world_x,
                        float world_y, float *out_view_x, float *out_view_y);
  void (*view_to_world)(const StygianEditor *editor, float view_x, float view_y,
                        float *out_world_x, float *out_world_y);
  float (*snap_world_scalar)(const StygianEditor *editor, float world_value);
  void (*snap_world_point)(const StygianEditor *editor, float world_x,
                           float world_y, float *out_world_x,
                           float *out_world_y);

  StygianEditorNodeId (*add_rect)(StygianEditor *editor,
                                  const StygianEditorRectDesc *desc);
  StygianEditorNodeId (*add_ellipse)(StygianEditor *editor,
                                     const StygianEditorEllipseDesc *desc);

  StygianEditorPathId (*path_begin)(StygianEditor *editor,
                                    const StygianEditorPathDesc *desc);
  bool (*path_add_point)(StygianEditor *editor, StygianEditorPathId id,
                         float x, float y, bool snap_to_grid);
  StygianEditorNodeId (*path_commit)(StygianEditor *editor,
                                     StygianEditorPathId id);
  void (*path_cancel)(StygianEditor *editor, StygianEditorPathId id);

  uint32_t (*node_count)(const StygianEditor *editor);
  bool (*node_set_position)(StygianEditor *editor, StygianEditorNodeId node_id,
                            float x, float y, bool snap_to_grid);
  bool (*node_set_size)(StygianEditor *editor, StygianEditorNodeId node_id,
                        float w, float h);
  bool (*node_set_color)(StygianEditor *editor, StygianEditorNodeId node_id,
                         StygianEditorColor color);
  bool (*node_get_color)(const StygianEditor *editor,
                         StygianEditorNodeId node_id,
                         StygianEditorColor *out_color);
  bool (*node_set_opacity)(StygianEditor *editor, StygianEditorNodeId node_id,
                           float opacity);
  bool (*node_get_shape_kind)(const StygianEditor *editor,
                              StygianEditorNodeId node_id,
                              StygianEditorShapeKind *out_kind);
  bool (*node_set_corner_radii)(StygianEditor *editor,
                                StygianEditorNodeId node_id, float top_left,
                                float top_right, float bottom_right,
                                float bottom_left);
  bool (*node_get_corner_radii)(const StygianEditor *editor,
                                StygianEditorNodeId node_id,
                                float *out_top_left, float *out_top_right,
                                float *out_bottom_right,
                                float *out_bottom_left);
  bool (*node_get_bounds)(const StygianEditor *editor,
                          StygianEditorNodeId node_id, float *out_x,
                          float *out_y, float *out_w, float *out_h);
  bool (*delete_node)(StygianEditor *editor, StygianEditorNodeId node_id);

  StygianEditorNodeId (*select_at)(StygianEditor *editor, float x, float y,
                                   bool additive);
  bool (*select_node)(StygianEditor *editor, StygianEditorNodeId node_id,
                      bool additive);
  StygianEditorNodeId (*selected_node)(const StygianEditor *editor);
  uint32_t (*selected_count)(const StygianEditor *editor);
  bool (*node_is_selected)(const StygianEditor *editor,
                           StygianEditorNodeId node_id);
  uint32_t (*selected_nodes)(const StygianEditor *editor,
                             StygianEditorNodeId *out_ids, uint32_t max_ids);
  StygianEditorNodeId (*hit_test_at)(const StygianEditor *editor, float x,
                                     float y);
  uint32_t (*select_in_rect)(StygianEditor *editor, float x0, float y0,
                             float x1, float y1, bool additive);

  bool (*set_color_token)(StygianEditor *editor, const char *name,
                          StygianEditorColor color);
  bool (*get_color_token)(const StygianEditor *editor, const char *name,
                          StygianEditorColor *out_color);
  bool (*apply_color_token)(StygianEditor *editor, StygianEditorNodeId node_id,
                            const char *name);

  bool (*add_behavior)(StygianEditor *editor,
                       const StygianEditorBehaviorRule *rule,
                       StygianEditorBehaviorId *out_behavior_id);
  bool (*remove_behavior)(StygianEditor *editor,
                          StygianEditorBehaviorId behavior_id);
  uint32_t (*behavior_count)(const StygianEditor *editor);
  bool (*get_behavior_rule)(const StygianEditor *editor, uint32_t index,
                            StygianEditorBehaviorId *out_behavior_id,
                            StygianEditorBehaviorRule *out_rule);
  void (*trigger_event)(StygianEditor *editor, StygianEditorNodeId trigger_node,
                        StygianEditorEventKind event_kind);
  void (*tick)(StygianEditor *editor, uint64_t now_ms);

  void (*render_viewport2d)(const StygianEditor *editor, StygianContext *ctx,
                            bool draw_grid, bool draw_selection);
  size_t (*build_c23)(const StygianEditor *editor, char *out_code,
                      size_t out_capacity);
} StygianEditorModuleApi;

STYGIAN_EDITOR_MODULE_API uint32_t stygian_editor_module_abi_version(void);

STYGIAN_EDITOR_MODULE_API bool
stygian_editor_module_get_info(StygianEditorModuleInfo *out_info);

STYGIAN_EDITOR_MODULE_API const StygianEditorModuleApi *
stygian_editor_module_get_api(uint32_t requested_abi_version);

#ifdef __cplusplus
}
#endif

#endif // STYGIAN_EDITOR_MODULE_H
