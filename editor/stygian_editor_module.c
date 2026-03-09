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
    .add_rect = stygian_editor_add_rect,
    .add_ellipse = stygian_editor_add_ellipse,
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
    .node_get_shape_kind = stygian_editor_node_get_shape_kind,
    .node_set_corner_radii = stygian_editor_node_set_corner_radii,
    .node_get_corner_radii = stygian_editor_node_get_corner_radii,
    .node_get_bounds = stygian_editor_node_get_bounds,
    .delete_node = stygian_editor_delete_node,
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
    .render_viewport2d = stygian_editor_render_viewport2d,
    .build_c23 = stygian_editor_build_c23,
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
