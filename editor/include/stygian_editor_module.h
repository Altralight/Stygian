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
#define STYGIAN_EDITOR_MODULE_ABI_MINOR 28u
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
  void (*set_ruler_unit)(StygianEditor *editor, StygianEditorRulerUnit unit);
  StygianEditorRulerUnit (*get_ruler_unit)(const StygianEditor *editor);
  void (*set_snap_sources)(StygianEditor *editor,
                           const StygianEditorSnapSources *sources);
  StygianEditorSnapSources (*get_snap_sources)(const StygianEditor *editor);
  uint32_t (*add_guide)(StygianEditor *editor, const StygianEditorGuide *guide);
  bool (*remove_guide)(StygianEditor *editor, uint32_t guide_id);
  uint32_t (*guide_count)(const StygianEditor *editor);
  bool (*get_guide)(const StygianEditor *editor, uint32_t index,
                    StygianEditorGuide *out_guide);

  StygianEditorNodeId (*add_rect)(StygianEditor *editor,
                                  const StygianEditorRectDesc *desc);
  StygianEditorNodeId (*add_ellipse)(StygianEditor *editor,
                                     const StygianEditorEllipseDesc *desc);
  StygianEditorNodeId (*add_frame)(StygianEditor *editor,
                                   const StygianEditorFrameDesc *desc);

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
  bool (*node_set_rotation)(StygianEditor *editor, StygianEditorNodeId node_id,
                            float rotation_degrees);
  bool (*node_get_rotation)(const StygianEditor *editor,
                            StygianEditorNodeId node_id,
                            float *out_rotation_degrees);
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
  bool (*node_get_parent)(const StygianEditor *editor,
                          StygianEditorNodeId node_id,
                          StygianEditorNodeId *out_parent_id);
  bool (*reparent_node)(StygianEditor *editor, StygianEditorNodeId node_id,
                        StygianEditorNodeId new_parent_id,
                        bool keep_world_transform);
  bool (*apply_transform)(StygianEditor *editor,
                          const StygianEditorNodeId *node_ids,
                          uint32_t node_count,
                          const StygianEditorTransformCommand *command);
  bool (*delete_node)(StygianEditor *editor, StygianEditorNodeId node_id);
  bool (*property_get_descriptor)(StygianEditorPropertyKind kind,
                                  StygianEditorPropertyDescriptor *out_desc);
  bool (*node_get_property_value)(const StygianEditor *editor,
                                  StygianEditorNodeId node_id,
                                  StygianEditorPropertyKind property,
                                  StygianEditorPropertyValue *out_value);
  bool (*node_set_property_value)(StygianEditor *editor,
                                  StygianEditorNodeId node_id,
                                  StygianEditorPropertyKind property,
                                  const StygianEditorPropertyValue *value);
  bool (*selection_get_property_value)(const StygianEditor *editor,
                                       StygianEditorPropertyKind property,
                                       StygianEditorPropertyValue *out_value,
                                       bool *out_mixed,
                                       uint32_t *out_supported_count);
  bool (*selection_set_property_value)(StygianEditor *editor,
                                       StygianEditorPropertyKind property,
                                       const StygianEditorPropertyValue *value,
                                       uint32_t *out_applied_count);
  bool (*node_set_name)(StygianEditor *editor, StygianEditorNodeId node_id,
                        const char *name);
  bool (*node_get_name)(const StygianEditor *editor,
                        StygianEditorNodeId node_id, char *out_name,
                        size_t out_name_capacity);
  bool (*node_set_locked)(StygianEditor *editor, StygianEditorNodeId node_id,
                          bool locked);
  bool (*node_get_locked)(const StygianEditor *editor,
                          StygianEditorNodeId node_id, bool *out_locked);
  bool (*node_set_visible)(StygianEditor *editor, StygianEditorNodeId node_id,
                           bool visible);
  bool (*node_get_visible)(const StygianEditor *editor,
                           StygianEditorNodeId node_id, bool *out_visible);
  uint32_t (*tree_list_children)(const StygianEditor *editor,
                                 StygianEditorNodeId parent_id,
                                 StygianEditorNodeId *out_ids,
                                 uint32_t max_ids);
  bool (*tree_reorder_child)(StygianEditor *editor, StygianEditorNodeId node_id,
                             uint32_t new_sibling_index);
  bool (*node_set_constraints)(StygianEditor *editor,
                               StygianEditorNodeId node_id,
                               StygianEditorConstraintH horizontal,
                               StygianEditorConstraintV vertical);
  bool (*node_get_constraints)(const StygianEditor *editor,
                               StygianEditorNodeId node_id,
                               StygianEditorConstraintH *out_horizontal,
                               StygianEditorConstraintV *out_vertical);
  bool (*resize_frame)(StygianEditor *editor, StygianEditorNodeId frame_node_id,
                       float new_width, float new_height);
  bool (*frame_set_auto_layout)(StygianEditor *editor,
                                StygianEditorNodeId frame_node_id,
                                const StygianEditorFrameAutoLayout *layout);
  bool (*frame_get_auto_layout)(const StygianEditor *editor,
                                StygianEditorNodeId frame_node_id,
                                StygianEditorFrameAutoLayout *out_layout);
  bool (*node_set_layout_options)(
      StygianEditor *editor, StygianEditorNodeId node_id,
      const StygianEditorNodeLayoutOptions *options);
  bool (*node_get_layout_options)(
      const StygianEditor *editor, StygianEditorNodeId node_id,
      StygianEditorNodeLayoutOptions *out_options);
  bool (*frame_recompute_layout)(StygianEditor *editor,
                                 StygianEditorNodeId frame_node_id);
  bool (*node_set_size_limits)(StygianEditor *editor,
                               StygianEditorNodeId node_id, float min_width,
                               float max_width, float min_height,
                               float max_height);
  bool (*node_get_size_limits)(const StygianEditor *editor,
                               StygianEditorNodeId node_id,
                               float *out_min_width, float *out_max_width,
                               float *out_min_height, float *out_max_height);
  bool (*frame_set_overflow_policy)(
      StygianEditor *editor, StygianEditorNodeId frame_node_id,
      StygianEditorFrameOverflowPolicy policy);
  bool (*frame_get_overflow_policy)(
      const StygianEditor *editor, StygianEditorNodeId frame_node_id,
      StygianEditorFrameOverflowPolicy *out_policy);
  bool (*frame_set_scroll)(StygianEditor *editor,
                           StygianEditorNodeId frame_node_id, float scroll_x,
                           float scroll_y);
  bool (*frame_get_scroll)(const StygianEditor *editor,
                           StygianEditorNodeId frame_node_id,
                           float *out_scroll_x, float *out_scroll_y);

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
  bool (*begin_transaction)(StygianEditor *editor);
  bool (*end_transaction)(StygianEditor *editor, bool commit);
  bool (*undo)(StygianEditor *editor);
  bool (*redo)(StygianEditor *editor);

  void (*render_viewport2d)(const StygianEditor *editor, StygianContext *ctx,
                            bool draw_grid, bool draw_selection);
  size_t (*build_project_json)(const StygianEditor *editor, char *out_json,
                               size_t out_capacity);
  bool (*load_project_json)(StygianEditor *editor, const char *json);
  bool (*save_project_file)(const StygianEditor *editor, const char *path);
  bool (*load_project_file)(StygianEditor *editor, const char *path);
  size_t (*build_c23)(const StygianEditor *editor, char *out_code,
                      size_t out_capacity);
  uint32_t (*collect_export_diagnostics)(
      const StygianEditor *editor, StygianEditorExportDiagnostic *out_diags,
      uint32_t max_diags);
  size_t (*build_c23_with_diagnostics)(
      const StygianEditor *editor, char *out_code, size_t out_capacity,
      StygianEditorExportDiagnostic *out_diags, uint32_t max_diags,
      uint32_t *out_diag_count, bool *out_has_error);

  // Added in ABI minor 15. Appended to keep existing offsets stable.
  StygianEditorNodeId (*add_line)(StygianEditor *editor,
                                  const StygianEditorLineDesc *desc);
  StygianEditorNodeId (*add_arrow)(StygianEditor *editor,
                                   const StygianEditorArrowDesc *desc);
  StygianEditorNodeId (*add_polygon)(StygianEditor *editor,
                                     const StygianEditorPolygonDesc *desc);
  StygianEditorNodeId (*add_star)(StygianEditor *editor,
                                  const StygianEditorStarDesc *desc);
  StygianEditorNodeId (*add_arc)(StygianEditor *editor,
                                 const StygianEditorArcDesc *desc);
  uint32_t (*node_fill_count)(const StygianEditor *editor,
                              StygianEditorNodeId node_id);
  bool (*node_get_fill)(const StygianEditor *editor, StygianEditorNodeId node_id,
                        uint32_t index, StygianEditorNodeFill *out_fill);
  bool (*node_set_fills)(StygianEditor *editor, StygianEditorNodeId node_id,
                         const StygianEditorNodeFill *fills,
                         uint32_t fill_count);
  // Added in ABI minor 17. Appended to keep existing offsets stable.
  uint32_t (*node_stroke_count)(const StygianEditor *editor,
                                StygianEditorNodeId node_id);
  bool (*node_get_stroke)(const StygianEditor *editor,
                          StygianEditorNodeId node_id, uint32_t index,
                          StygianEditorNodeStroke *out_stroke);
  bool (*node_set_strokes)(StygianEditor *editor, StygianEditorNodeId node_id,
                           const StygianEditorNodeStroke *strokes,
                           uint32_t stroke_count);
  uint32_t (*node_effect_count)(const StygianEditor *editor,
                                StygianEditorNodeId node_id);
  bool (*node_get_effect)(const StygianEditor *editor,
                          StygianEditorNodeId node_id, uint32_t index,
                          StygianEditorNodeEffect *out_effect);
  bool (*node_set_effects)(StygianEditor *editor, StygianEditorNodeId node_id,
                           const StygianEditorNodeEffect *effects,
                           uint32_t effect_count);
  bool (*node_set_mask)(StygianEditor *editor, StygianEditorNodeId node_id,
                        StygianEditorNodeId mask_node_id,
                        StygianEditorMaskMode mode, bool invert);
  bool (*node_get_mask)(const StygianEditor *editor,
                        StygianEditorNodeId node_id,
                        StygianEditorNodeId *out_mask_node_id,
                        StygianEditorMaskMode *out_mode, bool *out_invert);
  uint32_t (*path_point_count)(const StygianEditor *editor,
                               StygianEditorNodeId path_node_id);
  bool (*path_get_point)(const StygianEditor *editor,
                         StygianEditorNodeId path_node_id, uint32_t point_index,
                         StygianEditorPoint *out_point);
  bool (*path_set_point)(StygianEditor *editor,
                         StygianEditorNodeId path_node_id, uint32_t point_index,
                         const StygianEditorPoint *point);
  bool (*path_insert_point)(StygianEditor *editor,
                            StygianEditorNodeId path_node_id,
                            uint32_t point_index,
                            const StygianEditorPoint *point);
  bool (*path_remove_point)(StygianEditor *editor,
                            StygianEditorNodeId path_node_id,
                            uint32_t point_index);
  bool (*path_set_closed)(StygianEditor *editor,
                          StygianEditorNodeId path_node_id, bool closed);
  StygianEditorNodeId (*boolean_compose)(
      StygianEditor *editor, StygianEditorBooleanOp op,
      const StygianEditorNodeId *operand_ids, uint32_t operand_count,
      bool keep_sources);
  bool (*boolean_flatten)(StygianEditor *editor,
                          StygianEditorNodeId boolean_node_id,
                          StygianEditorNodeId *out_path_node_id);
  bool (*node_get_boolean)(const StygianEditor *editor,
                           StygianEditorNodeId node_id,
                           StygianEditorBooleanOp *out_op,
                           StygianEditorNodeId *out_operand_ids,
                           uint32_t max_operands,
                           uint32_t *out_operand_count);
  // Added in ABI minor 18. Appended to keep existing offsets stable.
  StygianEditorNodeId (*add_text)(StygianEditor *editor,
                                  const StygianEditorTextDesc *desc);
  StygianEditorNodeId (*add_image)(StygianEditor *editor,
                                   const StygianEditorImageDesc *desc);
  bool (*node_set_text_content)(StygianEditor *editor,
                                StygianEditorNodeId node_id, const char *text);
  bool (*node_get_text_content)(const StygianEditor *editor,
                                StygianEditorNodeId node_id, char *out_text,
                                size_t out_text_capacity);
  bool (*node_set_text_layout)(StygianEditor *editor,
                               StygianEditorNodeId node_id,
                               StygianEditorTextBoxMode box_mode,
                               StygianEditorTextHAlign align_h,
                               StygianEditorTextVAlign align_v,
                               StygianEditorTextAutoSize auto_size);
  bool (*node_get_text_layout)(
      const StygianEditor *editor, StygianEditorNodeId node_id,
      StygianEditorTextBoxMode *out_box_mode, StygianEditorTextHAlign *out_align_h,
      StygianEditorTextVAlign *out_align_v,
      StygianEditorTextAutoSize *out_auto_size);
  bool (*node_set_text_typography)(StygianEditor *editor,
                                   StygianEditorNodeId node_id, float font_size,
                                   float line_height, float letter_spacing,
                                   uint32_t font_weight);
  bool (*node_get_text_typography)(
      const StygianEditor *editor, StygianEditorNodeId node_id,
      float *out_font_size, float *out_line_height, float *out_letter_spacing,
      uint32_t *out_font_weight);
  uint32_t (*node_text_span_count)(const StygianEditor *editor,
                                   StygianEditorNodeId node_id);
  bool (*node_get_text_span)(const StygianEditor *editor,
                             StygianEditorNodeId node_id, uint32_t index,
                             StygianEditorTextStyleSpan *out_span);
  bool (*node_set_text_spans)(StygianEditor *editor, StygianEditorNodeId node_id,
                              const StygianEditorTextStyleSpan *spans,
                              uint32_t span_count);
  bool (*node_measure_text)(const StygianEditor *editor,
                            StygianEditorNodeId node_id, float *out_width,
                            float *out_height);
  bool (*node_set_image_source)(StygianEditor *editor,
                                StygianEditorNodeId node_id,
                                const char *source);
  bool (*node_get_image_source)(const StygianEditor *editor,
                                StygianEditorNodeId node_id, char *out_source,
                                size_t out_source_capacity);
  bool (*node_set_image_fit_mode)(StygianEditor *editor,
                                  StygianEditorNodeId node_id,
                                  uint32_t fit_mode);
  bool (*node_get_image_fit_mode)(const StygianEditor *editor,
                                  StygianEditorNodeId node_id,
                                  uint32_t *out_fit_mode);
  // Added in ABI minor 19. Appended to keep existing offsets stable.
  StygianEditorNodeId (*add_component_def)(
      StygianEditor *editor, const StygianEditorComponentDefDesc *desc);
  StygianEditorNodeId (*add_component_instance)(
      StygianEditor *editor, const StygianEditorComponentInstanceDesc *desc);
  bool (*component_def_get_symbol)(
      const StygianEditor *editor, StygianEditorNodeId component_def_id,
      char *out_symbol, size_t out_symbol_capacity);
  bool (*component_def_set_symbol)(
      StygianEditor *editor, StygianEditorNodeId component_def_id,
      const char *symbol);
  bool (*component_instance_get_def)(const StygianEditor *editor,
                                     StygianEditorNodeId instance_id,
                                     StygianEditorNodeId *out_component_def_id);
  bool (*component_instance_set_def)(StygianEditor *editor,
                                     StygianEditorNodeId instance_id,
                                     StygianEditorNodeId component_def_id);
  bool (*component_apply_definition)(StygianEditor *editor,
                                     StygianEditorNodeId component_def_id);
  // Added in ABI minor 20. Appended to keep existing offsets stable.
  bool (*component_instance_set_override_state)(
      StygianEditor *editor, StygianEditorNodeId instance_id,
      const StygianEditorComponentOverrideState *state);
  bool (*component_instance_get_override_state)(
      const StygianEditor *editor, StygianEditorNodeId instance_id,
      StygianEditorComponentOverrideState *out_state);
  bool (*component_instance_reset_overrides)(StygianEditor *editor,
                                             StygianEditorNodeId instance_id);
  bool (*component_def_set_variant)(StygianEditor *editor,
                                    StygianEditorNodeId component_def_id,
                                    const char *group_name,
                                    const char *variant_name);
  bool (*component_def_get_variant)(const StygianEditor *editor,
                                    StygianEditorNodeId component_def_id,
                                    char *out_group_name,
                                    size_t out_group_name_capacity,
                                    char *out_variant_name,
                                    size_t out_variant_name_capacity);
  bool (*component_def_set_property)(
      StygianEditor *editor, StygianEditorNodeId component_def_id,
      const StygianEditorComponentPropertyDef *property_def);
  uint32_t (*component_def_property_count)(const StygianEditor *editor,
                                           StygianEditorNodeId component_def_id);
  bool (*component_def_get_property)(
      const StygianEditor *editor, StygianEditorNodeId component_def_id,
      uint32_t index, StygianEditorComponentPropertyDef *out_property_def);
  bool (*set_text_style)(StygianEditor *editor,
                         const StygianEditorTextStyleDef *style_def);
  bool (*get_text_style)(const StygianEditor *editor, const char *name,
                         StygianEditorTextStyleDef *out_style_def);
  bool (*apply_text_style)(StygianEditor *editor, StygianEditorNodeId node_id,
                           const char *name);
  bool (*set_effect_style)(StygianEditor *editor,
                           const StygianEditorEffectStyleDef *style_def);
  bool (*get_effect_style)(const StygianEditor *editor, const char *name,
                           StygianEditorEffectStyleDef *out_style_def);
  bool (*apply_effect_style)(StygianEditor *editor, StygianEditorNodeId node_id,
                             const char *name);
  bool (*set_layout_style)(StygianEditor *editor,
                           const StygianEditorLayoutStyleDef *style_def);
  bool (*get_layout_style)(const StygianEditor *editor, const char *name,
                           StygianEditorLayoutStyleDef *out_style_def);
  bool (*apply_layout_style)(StygianEditor *editor,
                             StygianEditorNodeId frame_node_id,
                             const char *name);
  uint32_t (*style_usage_count)(const StygianEditor *editor,
                                StygianEditorStyleKind kind, const char *name);
  bool (*set_variable_mode)(StygianEditor *editor, uint32_t mode_index,
                            const char *mode_name);
  uint32_t (*variable_mode_count)(const StygianEditor *editor);
  bool (*get_variable_mode)(const StygianEditor *editor, uint32_t mode_index,
                            char *out_mode_name,
                            size_t out_mode_name_capacity);
  bool (*set_active_variable_mode)(StygianEditor *editor, uint32_t mode_index);
  uint32_t (*get_active_variable_mode)(const StygianEditor *editor);
  bool (*set_color_variable)(StygianEditor *editor, const char *name,
                             uint32_t mode_index, StygianEditorColor value);
  bool (*get_color_variable)(const StygianEditor *editor, const char *name,
                             uint32_t mode_index, StygianEditorColor *out_value);
  bool (*set_number_variable)(StygianEditor *editor, const char *name,
                              uint32_t mode_index, float value);
  bool (*get_number_variable)(const StygianEditor *editor, const char *name,
                              uint32_t mode_index, float *out_value);
  bool (*bind_node_color_variable)(StygianEditor *editor,
                                   StygianEditorNodeId node_id,
                                   const char *variable_name);
  bool (*bind_node_number_variable)(StygianEditor *editor,
                                    StygianEditorNodeId node_id,
                                    StygianEditorPropertyKind property,
                                    const char *variable_name);
  bool (*apply_active_variable_mode)(StygianEditor *editor);
  bool (*add_control_recipe)(StygianEditor *editor,
                             StygianEditorControlRecipeKind kind, float x,
                             float y, float w, float h, const char *name,
                             StygianEditorNodeId *out_root_id);
  // Added in ABI minor 22. Appended to keep existing offsets stable.
  bool (*component_resolve_variant)(
      const StygianEditor *editor, StygianEditorNodeId seed_component_def_id,
      const StygianEditorComponentPropertyValue *properties,
      uint32_t property_count, StygianEditorNodeId *out_component_def_id,
      uint32_t *out_match_score, bool *out_exact_match);
  bool (*component_instance_set_property_override)(
      StygianEditor *editor, StygianEditorNodeId instance_id,
      const StygianEditorComponentPropertyValue *property_override);
  bool (*component_instance_get_property_override)(
      const StygianEditor *editor, StygianEditorNodeId instance_id,
      const char *name, StygianEditorComponentPropertyValue *out_property_override);
  bool (*component_instance_clear_property_override)(
      StygianEditor *editor, StygianEditorNodeId instance_id, const char *name);
  bool (*component_defs_compatible)(
      const StygianEditor *editor, StygianEditorNodeId component_def_a,
      StygianEditorNodeId component_def_b);
  // Added in ABI minor 23. Appended to keep existing offsets stable.
  bool (*node_set_fill_gradient_transform)(
      StygianEditor *editor, StygianEditorNodeId node_id, uint32_t fill_index,
      const StygianEditorGradientTransform *transform);
  bool (*node_get_fill_gradient_transform)(
      const StygianEditor *editor, StygianEditorNodeId node_id,
      uint32_t fill_index, StygianEditorGradientTransform *out_transform);
  bool (*node_set_effect_transform)(
      StygianEditor *editor, StygianEditorNodeId node_id,
      uint32_t effect_index, const StygianEditorEffectTransform *transform);
  bool (*node_get_effect_transform)(
      const StygianEditor *editor, StygianEditorNodeId node_id,
      uint32_t effect_index, StygianEditorEffectTransform *out_transform);
  // Added in ABI minor 24. Appended to keep existing offsets stable.
  bool (*component_instance_detach)(StygianEditor *editor,
                                    StygianEditorNodeId instance_id);
  bool (*component_instance_is_detached)(
      const StygianEditor *editor, StygianEditorNodeId instance_id,
      bool *out_detached);
  bool (*component_instance_repair)(
      StygianEditor *editor, StygianEditorNodeId instance_id,
      StygianEditorNodeId preferred_component_def_id, bool preserve_overrides);
  // Added in ABI minor 25. Appended to keep existing offsets stable.
  bool (*add_driver)(StygianEditor *editor,
                     const StygianEditorDriverRule *rule,
                     StygianEditorDriverId *out_driver_id);
  bool (*remove_driver)(StygianEditor *editor, StygianEditorDriverId driver_id);
  uint32_t (*driver_count)(const StygianEditor *editor);
  bool (*get_driver_rule)(const StygianEditor *editor, uint32_t index,
                          StygianEditorDriverId *out_driver_id,
                          StygianEditorDriverRule *out_rule);
  bool (*apply_driver_sample)(StygianEditor *editor,
                              StygianEditorDriverId driver_id,
                              float source_value);
  // Added in ABI minor 28. Appended to keep existing offsets stable.
  bool (*node_set_shader_attachment)(
      StygianEditor *editor, StygianEditorNodeId node_id,
      const StygianEditorShaderAttachment *attachment);
  bool (*node_get_shader_attachment)(
      const StygianEditor *editor, StygianEditorNodeId node_id,
      StygianEditorShaderAttachment *out_attachment);
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

