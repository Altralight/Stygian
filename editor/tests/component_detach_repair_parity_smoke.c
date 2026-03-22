#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool nearly(float a, float b) {
  float d = a - b;
  if (d < 0.0f)
    d = -d;
  return d <= 0.01f;
}

static bool has_diag_feature(const StygianEditorExportDiagnostic *diags,
                             uint32_t count, const char *feature) {
  uint32_t i;
  for (i = 0u; i < count; ++i) {
    if (strcmp(diags[i].feature, feature) == 0)
      return true;
  }
  return false;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorComponentDefDesc def = {0};
  StygianEditorComponentInstanceDesc inst = {0};
  StygianEditorComponentPropertyDef prop = {0};
  StygianEditorComponentOverrideState state = {0};
  StygianEditorExportDiagnostic diags[64];
  StygianEditorNodeId d1 = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId d2 = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId i1 = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId got_def = STYGIAN_EDITOR_INVALID_ID;
  bool detached = false;
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
  float old_w = 0.0f;
  size_t need = 0u;
  uint32_t diag_count = 0u;
  bool has_error = false;
  static char json[262144];
  static char c23[524288];

  cfg.max_nodes = 256u;
  cfg.max_path_points = 1024u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 16u;
  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("create editor failed");

  def.x = 0.0f;
  def.y = 0.0f;
  def.w = 120.0f;
  def.h = 44.0f;
  def.visible = true;
  def.symbol = "chip_default";
  d1 = stygian_editor_add_component_def(a, &def);
  def.w = 180.0f;
  def.h = 52.0f;
  def.symbol = "chip_alt";
  d2 = stygian_editor_add_component_def(a, &def);
  if (!d1 || !d2)
    return fail("add component defs failed");

  snprintf(prop.name, sizeof(prop.name), "state");
  prop.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
  snprintf(prop.default_enum, sizeof(prop.default_enum), "idle");
  prop.enum_option_count = 2u;
  snprintf(prop.enum_options[0], sizeof(prop.enum_options[0]), "idle");
  snprintf(prop.enum_options[1], sizeof(prop.enum_options[1]), "active");
  if (!stygian_editor_component_def_set_property(a, d1, &prop) ||
      !stygian_editor_component_def_set_property(a, d2, &prop)) {
    return fail("set component property failed");
  }
  if (!stygian_editor_component_defs_compatible(a, d1, d2))
    return fail("defs should be compatible");

  inst.component_def_id = d1;
  inst.x = 40.0f;
  inst.y = 80.0f;
  inst.visible = true;
  i1 = stygian_editor_add_component_instance(a, &inst);
  if (!i1)
    return fail("add component instance failed");

  state.mask = STYGIAN_EDITOR_COMPONENT_OVERRIDE_W;
  state.w = 144.0f;
  if (!stygian_editor_component_instance_set_override_state(a, i1, &state))
    return fail("set override state failed");

  if (!stygian_editor_node_get_bounds(a, i1, &x, &y, &w, &h))
    return fail("get instance bounds failed");
  old_w = w;
  if (!stygian_editor_component_instance_detach(a, i1))
    return fail("detach failed");
  if (!stygian_editor_component_instance_is_detached(a, i1, &detached) ||
      !detached) {
    return fail("detached state not set");
  }

  if (!stygian_editor_node_set_size(a, d1, 260.0f, 66.0f))
    return fail("resize d1 failed");
  if (!stygian_editor_component_apply_definition(a, d1))
    return fail("apply d1 failed");
  if (!stygian_editor_node_get_bounds(a, i1, &x, &y, &w, &h))
    return fail("get detached bounds failed");
  if (!nearly(w, old_w))
    return fail("detached instance changed on definition apply");

  need = stygian_editor_build_project_json(a, NULL, 0u);
  if (need < 2u || need > sizeof(json))
    return fail("build json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need)
    return fail("build json failed");
  if (!stygian_editor_load_project_json(b, json))
    return fail("load json failed");
  if (!stygian_editor_component_instance_is_detached(b, i1, &detached) ||
      !detached) {
    return fail("detached state did not roundtrip");
  }

  if (!stygian_editor_component_instance_repair(b, i1, d2, true))
    return fail("repair failed");
  if (!stygian_editor_component_instance_is_detached(b, i1, &detached) ||
      detached) {
    return fail("repair did not clear detached state");
  }
  if (!stygian_editor_component_instance_get_def(b, i1, &got_def) ||
      got_def != d2) {
    return fail("repair did not bind requested def");
  }
  if (!stygian_editor_node_get_bounds(b, i1, &x, &y, &w, &h))
    return fail("get repaired bounds failed");
  if (!nearly(w, 144.0f)) {
    return fail("repair did not preserve overrides");
  }

  need = stygian_editor_build_c23_with_diagnostics(
      b, c23, sizeof(c23), diags, 64u, &diag_count, &has_error);
  if (need < 2u || need > sizeof(c23) || has_error)
    return fail("build c23 diagnostics failed");
  if (has_diag_feature(diags, diag_count, "component_instance_missing_def"))
    return fail("component missing def diagnostic should not fire after repair");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor component detach/repair parity smoke\n");
  return 0;
}
