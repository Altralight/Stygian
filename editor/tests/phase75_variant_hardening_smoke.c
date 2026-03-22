#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool read_file(const char *path, char *out, size_t cap) {
  FILE *f = fopen(path, "rb");
  size_t n = 0u;
  if (!f || !out || cap == 0u)
    return false;
  n = fread(out, 1u, cap - 1u, f);
  fclose(f);
  out[n] = '\0';
  return true;
}

static void normalize_newlines(char *text) {
  size_t read_i = 0u;
  size_t write_i = 0u;
  if (!text)
    return;
  while (text[read_i] != '\0') {
    if (text[read_i] == '\r' && text[read_i + 1u] == '\n') {
      text[write_i++] = '\n';
      read_i += 2u;
      continue;
    }
    text[write_i++] = text[read_i++];
  }
  text[write_i] = '\0';
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorComponentDefDesc def = {0};
  StygianEditorComponentPropertyDef prop = {0};
  StygianEditorComponentPropertyDef bool_prop = {0};
  StygianEditorComponentPropertyValue pv = {0};
  StygianEditorComponentPropertyValue query[2] = {0};
  StygianEditorComponentPropertyValue got = {0};
  StygianEditorRectDesc child_rect = {0};
  StygianEditorNodeId c1 = 0u, c2 = 0u, c3 = 0u;
  StygianEditorNodeId d1 = 0u, d2 = 0u, d3 = 0u, inst = 0u;
  StygianEditorNodeId resolved = 0u;
  StygianEditorNodeId m_state = 0u, m_dense = 0u, m_conflict = 0u, m_unknown = 0u,
                     m_last_wins = 0u;
  uint32_t score = 0u;
  bool exact = false, conflict_exact = false;
  char summary[512] = {0};
  char golden[512] = {0};

  cfg.max_nodes = 128u;
  cfg.max_path_points = 512u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 32u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("create editor failed");

  def.w = 120.0f;
  def.h = 40.0f;
  def.visible = true;
  def.symbol = "button";
  d1 = stygian_editor_add_component_def(editor, &def);
  def.symbol = "button_pressed";
  d2 = stygian_editor_add_component_def(editor, &def);
  def.symbol = "button_disabled";
  d3 = stygian_editor_add_component_def(editor, &def);
  if (!d1 || !d2 || !d3)
    return fail("add defs failed");

  if (!stygian_editor_component_def_set_variant(editor, d1, "button_variant", "default") ||
      !stygian_editor_component_def_set_variant(editor, d2, "button_variant", "pressed") ||
      !stygian_editor_component_def_set_variant(editor, d3, "button_variant", "disabled")) {
    return fail("set variants failed");
  }

  snprintf(prop.name, sizeof(prop.name), "state");
  prop.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
  prop.enum_option_count = 3u;
  snprintf(prop.enum_options[0], sizeof(prop.enum_options[0]), "default");
  snprintf(prop.enum_options[1], sizeof(prop.enum_options[1]), "pressed");
  snprintf(prop.enum_options[2], sizeof(prop.enum_options[2]), "disabled");

  snprintf(prop.default_enum, sizeof(prop.default_enum), "default");
  if (!stygian_editor_component_def_set_property(editor, d1, &prop))
    return fail("set property d1 failed");
  snprintf(prop.default_enum, sizeof(prop.default_enum), "pressed");
  if (!stygian_editor_component_def_set_property(editor, d2, &prop))
    return fail("set property d2 failed");
  snprintf(prop.default_enum, sizeof(prop.default_enum), "disabled");
  if (!stygian_editor_component_def_set_property(editor, d3, &prop))
    return fail("set property d3 failed");

  snprintf(bool_prop.name, sizeof(bool_prop.name), "dense");
  bool_prop.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_BOOL;
  bool_prop.default_bool = false;
  if (!stygian_editor_component_def_set_property(editor, d1, &bool_prop))
    return fail("set bool property d1 failed");
  if (!stygian_editor_component_def_set_property(editor, d2, &bool_prop))
    return fail("set bool property d2 failed");
  bool_prop.default_bool = true;
  if (!stygian_editor_component_def_set_property(editor, d3, &bool_prop))
    return fail("set bool property d3 failed");

  if (!stygian_editor_component_defs_compatible(editor, d1, d2) ||
      !stygian_editor_component_defs_compatible(editor, d1, d3)) {
    return fail("compatibility baseline failed");
  }

  child_rect.x = 6.0f;
  child_rect.y = 6.0f;
  child_rect.w = 24.0f;
  child_rect.h = 12.0f;
  child_rect.visible = true;
  c1 = stygian_editor_add_rect(editor, &child_rect);
  c2 = stygian_editor_add_rect(editor, &child_rect);
  c3 = stygian_editor_add_rect(editor, &child_rect);
  if (!c1 || !c2 || !c3)
    return fail("add component children failed");
  if (!stygian_editor_reparent_node(editor, c1, d1, false) ||
      !stygian_editor_reparent_node(editor, c2, d2, false) ||
      !stygian_editor_reparent_node(editor, c3, d3, false)) {
    return fail("reparent component children failed");
  }
  if (!stygian_editor_node_set_name(editor, c1, "label") ||
      !stygian_editor_node_set_name(editor, c2, "label") ||
      !stygian_editor_node_set_name(editor, c3, "label")) {
    return fail("set child names failed");
  }

  {
    StygianEditorComponentInstanceDesc idesc = {0};
    idesc.component_def_id = d1;
    idesc.x = 10.0f;
    idesc.y = 10.0f;
    idesc.visible = true;
    inst = stygian_editor_add_component_instance(editor, &idesc);
    if (!inst)
      return fail("add instance failed");
  }

  memset(&pv, 0, sizeof(pv));
  snprintf(pv.name, sizeof(pv.name), "state");
  pv.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
  snprintf(pv.enum_value, sizeof(pv.enum_value), "pressed");
  if (!stygian_editor_component_resolve_variant(editor, d1, &pv, 1u, &resolved, &score,
                                                &exact) ||
      resolved != d2 || !exact || score == 0u) {
    return fail("resolve pressed variant failed");
  }
  if (!stygian_editor_component_instance_set_property_override(editor, inst, &pv))
    return fail("set pressed override failed");

  snprintf(pv.enum_value, sizeof(pv.enum_value), "disabled");
  if (!stygian_editor_component_resolve_variant(editor, d1, &pv, 1u, &resolved, &score,
                                                &exact) ||
      resolved != d3 || !exact) {
    return fail("resolve disabled variant failed");
  }
  if (!stygian_editor_component_instance_set_property_override(editor, inst, &pv))
    return fail("set disabled override failed");

  memset(query, 0, sizeof(query));
  snprintf(query[0].name, sizeof(query[0].name), "state");
  query[0].type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
  snprintf(query[0].enum_value, sizeof(query[0].enum_value), "pressed");
  if (!stygian_editor_component_resolve_variant(editor, d1, query, 1u, &m_state,
                                                &score, &exact) ||
      m_state != d2 || !exact) {
    return fail("matrix resolve state failed");
  }

  memset(query, 0, sizeof(query));
  snprintf(query[0].name, sizeof(query[0].name), "dense");
  query[0].type = STYGIAN_EDITOR_COMPONENT_PROPERTY_BOOL;
  query[0].bool_value = true;
  if (!stygian_editor_component_resolve_variant(editor, d1, query, 1u, &m_dense,
                                                &score, &exact) ||
      m_dense != d3 || !exact) {
    return fail("matrix resolve bool failed");
  }

  memset(query, 0, sizeof(query));
  snprintf(query[0].name, sizeof(query[0].name), "state");
  query[0].type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
  snprintf(query[0].enum_value, sizeof(query[0].enum_value), "pressed");
  snprintf(query[1].name, sizeof(query[1].name), "dense");
  query[1].type = STYGIAN_EDITOR_COMPONENT_PROPERTY_BOOL;
  query[1].bool_value = true;
  if (!stygian_editor_component_resolve_variant(editor, d1, query, 2u,
                                                &m_conflict, &score,
                                                &conflict_exact) ||
      m_conflict != d2 || conflict_exact) {
    return fail("matrix resolve conflict fallback failed");
  }

  memset(query, 0, sizeof(query));
  snprintf(query[0].name, sizeof(query[0].name), "size");
  query[0].type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
  snprintf(query[0].enum_value, sizeof(query[0].enum_value), "lg");
  if (!stygian_editor_component_resolve_variant(editor, d1, query, 1u,
                                                &m_unknown, &score, &exact) ||
      m_unknown != d1 || exact) {
    return fail("matrix resolve unknown fallback failed");
  }

  memset(query, 0, sizeof(query));
  snprintf(query[0].name, sizeof(query[0].name), "state");
  query[0].type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
  snprintf(query[0].enum_value, sizeof(query[0].enum_value), "disabled");
  snprintf(query[1].name, sizeof(query[1].name), "state");
  query[1].type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
  snprintf(query[1].enum_value, sizeof(query[1].enum_value), "pressed");
  if (!stygian_editor_component_resolve_variant(editor, d1, query, 2u,
                                                &m_last_wins, &score, &exact) ||
      m_last_wins != d2 || !exact) {
    return fail("matrix resolve duplicate key failed");
  }

  snprintf(pv.name, sizeof(pv.name), "label.visible");
  pv.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_BOOL;
  pv.bool_value = true;
  if (!stygian_editor_component_instance_set_property_override(editor, inst, &pv))
    return fail("set scoped subtree override failed");
  if (!stygian_editor_component_instance_get_property_override(editor, inst, "label.visible",
                                                               &got)) {
    return fail("get scoped subtree override failed");
  }
  snprintf(pv.name, sizeof(pv.name), "ghost.visible");
  pv.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_BOOL;
  pv.bool_value = true;
  if (stygian_editor_component_instance_set_property_override(editor, inst, &pv))
    return fail("invalid scoped subtree override unexpectedly accepted");

  snprintf(pv.name, sizeof(pv.name), "state");
  pv.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
  snprintf(pv.enum_value, sizeof(pv.enum_value), "default");
  if (!stygian_editor_component_instance_set_property_override(editor, inst, &pv))
    return fail("set default override failed");
  if (!stygian_editor_component_instance_clear_property_override(editor, inst,
                                                                 "label.visible")) {
    return fail("clear scoped subtree override failed");
  }

  {
    StygianEditorComponentDefDesc bad_def = {0};
    StygianEditorComponentPropertyDef bad_prop = {0};
    StygianEditorComponentOverrideState ov = {0};
    StygianEditorNodeId bad_id = 0u;

    bad_def.w = 120.0f;
    bad_def.h = 40.0f;
    bad_def.visible = true;
    bad_def.symbol = "chip";
    bad_id = stygian_editor_add_component_def(editor, &bad_def);
    if (!bad_id)
      return fail("add bad def failed");
    if (!stygian_editor_component_def_set_variant(editor, bad_id, "chip_variant",
                                                  "default")) {
      return fail("set bad variant failed");
    }
    snprintf(bad_prop.name, sizeof(bad_prop.name), "mode");
    bad_prop.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
    bad_prop.enum_option_count = 2u;
    snprintf(bad_prop.enum_options[0], sizeof(bad_prop.enum_options[0]), "a");
    snprintf(bad_prop.enum_options[1], sizeof(bad_prop.enum_options[1]), "b");
    snprintf(bad_prop.default_enum, sizeof(bad_prop.default_enum), "a");
    if (!stygian_editor_component_def_set_property(editor, bad_id, &bad_prop))
      return fail("set bad property failed");

    ov.mask = STYGIAN_EDITOR_COMPONENT_OVERRIDE_SWAP;
    ov.swap_component_def_id = bad_id;
    if (stygian_editor_component_instance_set_override_state(editor, inst, &ov))
      return fail("incompatible swap unexpectedly accepted");
  }

  snprintf(pv.name, sizeof(pv.name), "state");
  pv.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
  snprintf(pv.enum_value, sizeof(pv.enum_value), "disabled");
  if (!stygian_editor_component_instance_set_property_override(editor, inst, &pv))
    return fail("set override before reset failed");
  if (!stygian_editor_component_instance_reset_overrides(editor, inst))
    return fail("reset overrides failed");
  if (stygian_editor_component_instance_get_property_override(editor, inst, "state",
                                                              &got)) {
    return fail("reset override unexpectedly retained state");
  }

  snprintf(summary, sizeof(summary),
           "resolved_ids=%u,%u,%u\nmatrix=%u,%u,%u,%u,%u\nscoped_override=ok\noverride_reset=ok\ncompatibility_guard=ok\n",
           d2, d3, d1, m_state, m_dense, m_conflict, m_unknown, m_last_wins);
  if (!read_file("D:\\Projects\\Code\\Stygian\\editor\\fixtures\\variant_resolution_golden.txt",
                 golden, sizeof(golden))) {
    return fail("read golden failed");
  }
  normalize_newlines(summary);
  normalize_newlines(golden);
  if (strcmp(summary, golden) != 0)
    return fail("golden mismatch");

  stygian_editor_destroy(editor);
  printf("[PASS] editor phase75 variant hardening smoke\n");
  return 0;
}
