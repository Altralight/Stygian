#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorComponentDefDesc def_desc = {0};
  StygianEditorComponentInstanceDesc inst_desc = {0};
  StygianEditorNodeId def_id = 0u;
  StygianEditorNodeId inst_a = 0u;
  StygianEditorNodeId inst_b = 0u;
  StygianEditorNodeId linked = 0u;
  StygianEditorPropertyValue v = {0};
  char json[262144];
  size_t need = 0u;
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
  bool visible = false;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 4096u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 64u;
  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  def_desc.x = 40.0f;
  def_desc.y = 50.0f;
  def_desc.w = 180.0f;
  def_desc.h = 60.0f;
  def_desc.symbol = "button_primary";
  def_desc.visible = true;
  def_desc.z = 1.0f;
  def_id = stygian_editor_add_component_def(a, &def_desc);
  if (!def_id)
    return fail("add component def failed");

  inst_desc.component_def_id = def_id;
  inst_desc.x = 300.0f;
  inst_desc.y = 200.0f;
  inst_desc.w = 0.0f;
  inst_desc.h = 0.0f;
  inst_desc.visible = true;
  inst_desc.z = 2.0f;
  inst_a = stygian_editor_add_component_instance(a, &inst_desc);
  if (!inst_a)
    return fail("add instance a failed");
  inst_desc.x = 300.0f;
  inst_desc.y = 300.0f;
  inst_b = stygian_editor_add_component_instance(a, &inst_desc);
  if (!inst_b)
    return fail("add instance b failed");

  if (!stygian_editor_component_instance_get_def(a, inst_a, &linked) ||
      linked != def_id) {
    return fail("instance def linkage failed");
  }

  v.type = STYGIAN_EDITOR_VALUE_FLOAT;
  v.as.number = 240.0f;
  if (!stygian_editor_node_set_property_value(a, def_id, STYGIAN_EDITOR_PROP_WIDTH,
                                              &v)) {
    return fail("set def width failed");
  }
  if (!stygian_editor_node_get_bounds(a, inst_a, &x, &y, &w, &h))
    return fail("get instance a bounds failed");
  if (w < 239.9f || w > 240.1f)
    return fail("instance width did not propagate");

  v.type = STYGIAN_EDITOR_VALUE_BOOL;
  v.as.boolean = false;
  if (!stygian_editor_node_set_property_value(a, def_id, STYGIAN_EDITOR_PROP_VISIBLE,
                                              &v)) {
    return fail("set def visible failed");
  }
  if (!stygian_editor_node_get_visible(a, inst_b, &visible) || visible)
    return fail("instance visibility did not propagate");

  if (!stygian_editor_component_apply_definition(a, def_id))
    return fail("component apply definition failed");

  need = stygian_editor_build_project_json(a, NULL, 0u);
  if (need == 0u || need > sizeof(json))
    return fail("build json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need)
    return fail("build json failed");
  if (!stygian_editor_load_project_json(b, json))
    return fail("reload json failed");
  if (!stygian_editor_component_instance_get_def(b, inst_a, &linked) ||
      linked != def_id) {
    return fail("reloaded def linkage failed");
  }

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor component linkage smoke\n");
  return 0;
}
