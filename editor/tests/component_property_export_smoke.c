#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorComponentDefDesc def = {0};
  StygianEditorComponentInstanceDesc inst = {0};
  StygianEditorComponentPropertyDef prop = {0};
  StygianEditorComponentPropertyValue ov = {0};
  StygianEditorNodeId d0 = 0u;
  StygianEditorNodeId d1 = 0u;
  StygianEditorNodeId instance_id = 0u;
  char code[524288];
  size_t need = 0u;

  cfg.max_nodes = 128u;
  cfg.max_path_points = 256u;
  cfg.max_behaviors = 32u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("create editor failed");

  def.x = 10.0f;
  def.y = 10.0f;
  def.w = 180.0f;
  def.h = 56.0f;
  def.symbol = "tab_default";
  def.visible = true;
  d0 = stygian_editor_add_component_def(editor, &def);
  def.symbol = "tab_active";
  d1 = stygian_editor_add_component_def(editor, &def);
  if (!d0 || !d1)
    return fail("add component defs failed");

  if (!stygian_editor_component_def_set_variant(editor, d0, "tab_variant",
                                                "default") ||
      !stygian_editor_component_def_set_variant(editor, d1, "tab_variant",
                                                "active")) {
    return fail("set variants failed");
  }

  snprintf(prop.name, sizeof(prop.name), "state");
  prop.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
  prop.enum_option_count = 2u;
  snprintf(prop.enum_options[0], sizeof(prop.enum_options[0]), "default");
  snprintf(prop.enum_options[1], sizeof(prop.enum_options[1]), "active");
  snprintf(prop.default_enum, sizeof(prop.default_enum), "default");
  if (!stygian_editor_component_def_set_property(editor, d0, &prop))
    return fail("set property d0 failed");
  snprintf(prop.default_enum, sizeof(prop.default_enum), "active");
  if (!stygian_editor_component_def_set_property(editor, d1, &prop))
    return fail("set property d1 failed");

  inst.component_def_id = d0;
  inst.x = 220.0f;
  inst.y = 80.0f;
  inst.visible = true;
  instance_id = stygian_editor_add_component_instance(editor, &inst);
  if (!instance_id)
    return fail("add component instance failed");

  snprintf(ov.name, sizeof(ov.name), "state");
  ov.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
  snprintf(ov.enum_value, sizeof(ov.enum_value), "active");
  if (!stygian_editor_component_instance_set_property_override(editor, instance_id,
                                                               &ov)) {
    return fail("set property override failed");
  }

  need = stygian_editor_build_c23(editor, NULL, 0u);
  if (need == 0u || need > sizeof(code))
    return fail("build c23 size failed");
  if (stygian_editor_build_c23(editor, code, sizeof(code)) != need)
    return fail("build c23 failed");
  if (!strstr(code, "kStygianEditorGeneratedComponentDefs"))
    return fail("missing component def export table");
  if (!strstr(code, "kStygianEditorGeneratedComponentProperties"))
    return fail("missing component property export table");
  if (!strstr(code, "kStygianEditorGeneratedComponentInstances"))
    return fail("missing component instance export table");
  if (!strstr(code, "kStygianEditorGeneratedComponentOverrides"))
    return fail("missing component override export table");
  if (!strstr(code, "tab_variant"))
    return fail("missing exported variant group");
  if (!strstr(code, "tab_active"))
    return fail("missing exported resolved symbol");
  if (!strstr(code, "component-instance resolved-symbol="))
    return fail("missing component instance mapping comment");

  stygian_editor_destroy(editor);
  printf("[PASS] editor component property export smoke\n");
  return 0;
}
