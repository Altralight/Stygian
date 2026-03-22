#include "../include/stygian_editor_module.h"

static const char *stygian_editor_module_name_impl(void) {
  return "stygian-editor-module";
}

static const char *stygian_editor_module_version_impl(void) {
  return "0.1.0";
}

static const StygianEditorModuleApi g_stygian_editor_module_api = {
    .abi_version = STYGIAN_EDITOR_MODULE_ABI_VERSION,
    .struct_size = sizeof(StygianEditorModuleApi),
    .module_name = stygian_editor_module_name_impl,
    .module_version = stygian_editor_module_version_impl,
    .config_default = stygian_editor_config_default,
    .color_rgba = stygian_editor_color_rgba,
    .create = stygian_editor_create,
    .destroy = stygian_editor_destroy,
    .reset = stygian_editor_reset,
    .set_host = stygian_editor_set_host,
    .set_viewport = stygian_editor_set_viewport,
    .get_viewport = stygian_editor_get_viewport,
    .set_grid_config = stygian_editor_set_grid_config,
    .get_grid_config = stygian_editor_get_grid_config,
    .get_grid_levels = stygian_editor_get_grid_levels,
    .world_to_view = stygian_editor_world_to_view,
    .view_to_world = stygian_editor_view_to_world,
    .snap_world_scalar = stygian_editor_snap_world_scalar,
    .snap_world_point = stygian_editor_snap_world_point,
    .set_ruler_unit = stygian_editor_set_ruler_unit,
    .get_ruler_unit = stygian_editor_get_ruler_unit,
    .set_snap_sources = stygian_editor_set_snap_sources,
    .get_snap_sources = stygian_editor_get_snap_sources,
    .add_guide = stygian_editor_add_guide,
    .remove_guide = stygian_editor_remove_guide,
    .guide_count = stygian_editor_guide_count,
    .get_guide = stygian_editor_get_guide,
    .add_rect = stygian_editor_add_rect,
    .add_ellipse = stygian_editor_add_ellipse,
    .add_frame = stygian_editor_add_frame,
    .path_begin = stygian_editor_path_begin,
    .path_add_point = stygian_editor_path_add_point,
    .path_commit = stygian_editor_path_commit,
    .path_cancel = stygian_editor_path_cancel,
    .node_count = stygian_editor_node_count,
    .node_set_position = stygian_editor_node_set_position,
    .node_set_size = stygian_editor_node_set_size,
    .node_set_color = stygian_editor_node_set_color,
    .node_get_color = stygian_editor_node_get_color,
    .node_set_opacity = stygian_editor_node_set_opacity,
    .node_set_rotation = stygian_editor_node_set_rotation,
    .node_get_rotation = stygian_editor_node_get_rotation,
    .node_get_shape_kind = stygian_editor_node_get_shape_kind,
    .node_set_corner_radii = stygian_editor_node_set_corner_radii,
    .node_get_corner_radii = stygian_editor_node_get_corner_radii,
    .node_get_bounds = stygian_editor_node_get_bounds,
    .node_get_parent = stygian_editor_node_get_parent,
    .reparent_node = stygian_editor_reparent_node,
    .apply_transform = stygian_editor_apply_transform,
    .delete_node = stygian_editor_delete_node,
    .property_get_descriptor = stygian_editor_property_get_descriptor,
    .node_get_property_value = stygian_editor_node_get_property_value,
    .node_set_property_value = stygian_editor_node_set_property_value,
    .selection_get_property_value = stygian_editor_selection_get_property_value,
    .selection_set_property_value = stygian_editor_selection_set_property_value,
    .node_set_name = stygian_editor_node_set_name,
    .node_get_name = stygian_editor_node_get_name,
    .node_set_locked = stygian_editor_node_set_locked,
    .node_get_locked = stygian_editor_node_get_locked,
    .node_set_visible = stygian_editor_node_set_visible,
    .node_get_visible = stygian_editor_node_get_visible,
    .tree_list_children = stygian_editor_tree_list_children,
    .tree_reorder_child = stygian_editor_tree_reorder_child,
    .node_set_constraints = stygian_editor_node_set_constraints,
    .node_get_constraints = stygian_editor_node_get_constraints,
    .resize_frame = stygian_editor_resize_frame,
    .frame_set_auto_layout = stygian_editor_frame_set_auto_layout,
    .frame_get_auto_layout = stygian_editor_frame_get_auto_layout,
    .node_set_layout_options = stygian_editor_node_set_layout_options,
    .node_get_layout_options = stygian_editor_node_get_layout_options,
    .frame_recompute_layout = stygian_editor_frame_recompute_layout,
    .node_set_size_limits = stygian_editor_node_set_size_limits,
    .node_get_size_limits = stygian_editor_node_get_size_limits,
    .frame_set_overflow_policy = stygian_editor_frame_set_overflow_policy,
    .frame_get_overflow_policy = stygian_editor_frame_get_overflow_policy,
    .frame_set_scroll = stygian_editor_frame_set_scroll,
    .frame_get_scroll = stygian_editor_frame_get_scroll,
    .select_at = stygian_editor_select_at,
    .select_node = stygian_editor_select_node,
    .selected_node = stygian_editor_selected_node,
    .selected_count = stygian_editor_selected_count,
    .node_is_selected = stygian_editor_node_is_selected,
    .selected_nodes = stygian_editor_selected_nodes,
    .hit_test_at = stygian_editor_hit_test_at,
    .select_in_rect = stygian_editor_select_in_rect,
    .set_color_token = stygian_editor_set_color_token,
    .get_color_token = stygian_editor_get_color_token,
    .apply_color_token = stygian_editor_apply_color_token,
    .add_behavior = stygian_editor_add_behavior,
    .remove_behavior = stygian_editor_remove_behavior,
    .behavior_count = stygian_editor_behavior_count,
    .get_behavior_rule = stygian_editor_get_behavior_rule,
    .trigger_event = stygian_editor_trigger_event,
    .tick = stygian_editor_tick,
    .begin_transaction = stygian_editor_begin_transaction,
    .end_transaction = stygian_editor_end_transaction,
    .undo = stygian_editor_undo,
    .redo = stygian_editor_redo,
    .render_viewport2d = stygian_editor_render_viewport2d,
    .build_project_json = stygian_editor_build_project_json,
    .load_project_json = stygian_editor_load_project_json,
    .save_project_file = stygian_editor_save_project_file,
    .load_project_file = stygian_editor_load_project_file,
    .build_c23 = stygian_editor_build_c23,
    .collect_export_diagnostics = stygian_editor_collect_export_diagnostics,
    .build_c23_with_diagnostics = stygian_editor_build_c23_with_diagnostics,
    .add_line = stygian_editor_add_line,
    .add_arrow = stygian_editor_add_arrow,
    .add_polygon = stygian_editor_add_polygon,
    .add_star = stygian_editor_add_star,
    .add_arc = stygian_editor_add_arc,
    .node_fill_count = stygian_editor_node_fill_count,
    .node_get_fill = stygian_editor_node_get_fill,
    .node_set_fills = stygian_editor_node_set_fills,
    .node_set_fill_gradient_transform =
        stygian_editor_node_set_fill_gradient_transform,
    .node_get_fill_gradient_transform =
        stygian_editor_node_get_fill_gradient_transform,
    .node_stroke_count = stygian_editor_node_stroke_count,
    .node_get_stroke = stygian_editor_node_get_stroke,
    .node_set_strokes = stygian_editor_node_set_strokes,
    .node_effect_count = stygian_editor_node_effect_count,
    .node_get_effect = stygian_editor_node_get_effect,
    .node_set_effects = stygian_editor_node_set_effects,
    .node_set_effect_transform = stygian_editor_node_set_effect_transform,
    .node_get_effect_transform = stygian_editor_node_get_effect_transform,
    .component_instance_detach = stygian_editor_component_instance_detach,
    .component_instance_is_detached =
        stygian_editor_component_instance_is_detached,
    .component_instance_repair = stygian_editor_component_instance_repair,
    .node_set_mask = stygian_editor_node_set_mask,
    .node_get_mask = stygian_editor_node_get_mask,
    .path_point_count = stygian_editor_path_point_count,
    .path_get_point = stygian_editor_path_get_point,
    .path_set_point = stygian_editor_path_set_point,
    .path_insert_point = stygian_editor_path_insert_point,
    .path_remove_point = stygian_editor_path_remove_point,
    .path_set_closed = stygian_editor_path_set_closed,
    .boolean_compose = stygian_editor_boolean_compose,
    .boolean_flatten = stygian_editor_boolean_flatten,
    .node_get_boolean = stygian_editor_node_get_boolean,
    .add_text = stygian_editor_add_text,
    .add_image = stygian_editor_add_image,
    .node_set_text_content = stygian_editor_node_set_text_content,
    .node_get_text_content = stygian_editor_node_get_text_content,
    .node_set_text_layout = stygian_editor_node_set_text_layout,
    .node_get_text_layout = stygian_editor_node_get_text_layout,
    .node_set_text_typography = stygian_editor_node_set_text_typography,
    .node_get_text_typography = stygian_editor_node_get_text_typography,
    .node_text_span_count = stygian_editor_node_text_span_count,
    .node_get_text_span = stygian_editor_node_get_text_span,
    .node_set_text_spans = stygian_editor_node_set_text_spans,
    .node_measure_text = stygian_editor_node_measure_text,
    .node_set_image_source = stygian_editor_node_set_image_source,
    .node_get_image_source = stygian_editor_node_get_image_source,
    .node_set_image_fit_mode = stygian_editor_node_set_image_fit_mode,
    .node_get_image_fit_mode = stygian_editor_node_get_image_fit_mode,
    .add_component_def = stygian_editor_add_component_def,
    .add_component_instance = stygian_editor_add_component_instance,
    .component_def_get_symbol = stygian_editor_component_def_get_symbol,
    .component_def_set_symbol = stygian_editor_component_def_set_symbol,
    .component_instance_get_def = stygian_editor_component_instance_get_def,
    .component_instance_set_def = stygian_editor_component_instance_set_def,
    .component_apply_definition = stygian_editor_component_apply_definition,
    .component_instance_set_override_state =
        stygian_editor_component_instance_set_override_state,
    .component_instance_get_override_state =
        stygian_editor_component_instance_get_override_state,
    .component_instance_reset_overrides =
        stygian_editor_component_instance_reset_overrides,
    .component_def_set_variant = stygian_editor_component_def_set_variant,
    .component_def_get_variant = stygian_editor_component_def_get_variant,
    .component_def_set_property = stygian_editor_component_def_set_property,
    .component_def_property_count = stygian_editor_component_def_property_count,
    .component_def_get_property = stygian_editor_component_def_get_property,
    .set_text_style = stygian_editor_set_text_style,
    .get_text_style = stygian_editor_get_text_style,
    .apply_text_style = stygian_editor_apply_text_style,
    .set_effect_style = stygian_editor_set_effect_style,
    .get_effect_style = stygian_editor_get_effect_style,
    .apply_effect_style = stygian_editor_apply_effect_style,
    .set_layout_style = stygian_editor_set_layout_style,
    .get_layout_style = stygian_editor_get_layout_style,
    .apply_layout_style = stygian_editor_apply_layout_style,
    .style_usage_count = stygian_editor_style_usage_count,
    .set_variable_mode = stygian_editor_set_variable_mode,
    .variable_mode_count = stygian_editor_variable_mode_count,
    .get_variable_mode = stygian_editor_get_variable_mode,
    .set_active_variable_mode = stygian_editor_set_active_variable_mode,
    .get_active_variable_mode = stygian_editor_get_active_variable_mode,
    .set_color_variable = stygian_editor_set_color_variable,
    .get_color_variable = stygian_editor_get_color_variable,
    .set_number_variable = stygian_editor_set_number_variable,
    .get_number_variable = stygian_editor_get_number_variable,
    .bind_node_color_variable = stygian_editor_bind_node_color_variable,
    .bind_node_number_variable = stygian_editor_bind_node_number_variable,
    .apply_active_variable_mode = stygian_editor_apply_active_variable_mode,
    .add_control_recipe = stygian_editor_add_control_recipe,
    .component_resolve_variant = stygian_editor_component_resolve_variant,
    .component_instance_set_property_override =
        stygian_editor_component_instance_set_property_override,
    .component_instance_get_property_override =
        stygian_editor_component_instance_get_property_override,
    .component_instance_clear_property_override =
        stygian_editor_component_instance_clear_property_override,
    .component_defs_compatible = stygian_editor_component_defs_compatible,
    .add_driver = stygian_editor_add_driver,
    .remove_driver = stygian_editor_remove_driver,
    .driver_count = stygian_editor_driver_count,
    .get_driver_rule = stygian_editor_get_driver_rule,
    .apply_driver_sample = stygian_editor_apply_driver_sample,
    .node_set_shader_attachment = stygian_editor_node_set_shader_attachment,
    .node_get_shader_attachment = stygian_editor_node_get_shader_attachment,
};

uint32_t stygian_editor_module_abi_version(void) {
  return STYGIAN_EDITOR_MODULE_ABI_VERSION;
}

bool stygian_editor_module_get_info(StygianEditorModuleInfo *out_info) {
  if (!out_info || out_info->struct_size < sizeof(StygianEditorModuleInfo))
    return false;

  out_info->struct_size = sizeof(StygianEditorModuleInfo);
  out_info->abi_version = STYGIAN_EDITOR_MODULE_ABI_VERSION;
  out_info->module_name = stygian_editor_module_name_impl();
  out_info->module_version = stygian_editor_module_version_impl();
  out_info->build_language = "C23";
  return true;
}

const StygianEditorModuleApi *
stygian_editor_module_get_api(uint32_t requested_abi_version) {
  uint32_t requested_major =
      STYGIAN_EDITOR_MODULE_ABI_VERSION_MAJOR(requested_abi_version);
  uint32_t module_major =
      STYGIAN_EDITOR_MODULE_ABI_VERSION_MAJOR(STYGIAN_EDITOR_MODULE_ABI_VERSION);

  if (requested_major != module_major)
    return NULL;

  if (requested_abi_version > STYGIAN_EDITOR_MODULE_ABI_VERSION)
    return NULL;

  return &g_stygian_editor_module_api;
}

