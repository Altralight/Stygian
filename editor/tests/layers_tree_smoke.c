#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool ids_equal(const StygianEditorNodeId *a, const StygianEditorNodeId *b,
                      uint32_t count) {
  uint32_t i;
  for (i = 0u; i < count; ++i) {
    if (a[i] != b[i])
      return false;
  }
  return true;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorNodeId root = 0u;
  StygianEditorNodeId pa = 0u;
  StygianEditorNodeId pb = 0u;
  StygianEditorNodeId c1 = 0u;
  StygianEditorNodeId c2 = 0u;
  StygianEditorNodeId got[8];
  StygianEditorNodeId expect[8];
  uint32_t count = 0u;
  StygianEditorNodeId pid = 0u;
  bool flag = false;
  char name[64];
  char json[262144];
  char code[262144];
  size_t need = 0u;
  size_t code_need = 0u;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 4096u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 16u;
  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  rect.w = 120.0f;
  rect.h = 80.0f;
  rect.fill = stygian_editor_color_rgba(0.2f, 0.2f, 0.2f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;

  rect.x = 0.0f;
  rect.y = 0.0f;
  root = stygian_editor_add_rect(a, &rect);
  rect.x = 20.0f;
  rect.y = 20.0f;
  pa = stygian_editor_add_rect(a, &rect);
  rect.x = 50.0f;
  rect.y = 20.0f;
  pb = stygian_editor_add_rect(a, &rect);
  rect.x = 4.0f;
  rect.y = 5.0f;
  c1 = stygian_editor_add_rect(a, &rect);
  rect.x = 10.0f;
  rect.y = 12.0f;
  c2 = stygian_editor_add_rect(a, &rect);
  if (!root || !pa || !pb || !c1 || !c2)
    return fail("add nodes failed");

  if (!stygian_editor_reparent_node(a, pa, root, false) ||
      !stygian_editor_reparent_node(a, pb, root, false) ||
      !stygian_editor_reparent_node(a, c1, pa, false) ||
      !stygian_editor_reparent_node(a, c2, pa, false)) {
    return fail("initial reparent failed");
  }

  if (!stygian_editor_node_set_name(a, c1, "CTA Label"))
    return fail("set name failed");
  if (!stygian_editor_node_get_name(a, c1, name, sizeof(name)))
    return fail("get name failed");
  if (strcmp(name, "CTA Label") != 0)
    return fail("name mismatch");

  if (!stygian_editor_node_set_locked(a, c2, true))
    return fail("set locked failed");
  if (!stygian_editor_node_get_locked(a, c2, &flag) || !flag)
    return fail("get locked failed");
  if (stygian_editor_reparent_node(a, c2, pb, false))
    return fail("locked node should reject reparent");

  if (!stygian_editor_node_set_visible(a, pb, false))
    return fail("set visible false failed");
  if (!stygian_editor_node_get_visible(a, pb, &flag) || flag)
    return fail("get visible false failed");

  count = stygian_editor_tree_list_children(a, pa, got, 8u);
  if (count != 2u)
    return fail("children count before reorder mismatch");
  expect[0] = c1;
  expect[1] = c2;
  if (!ids_equal(got, expect, 2u))
    return fail("children order before reorder mismatch");

  if (!stygian_editor_tree_reorder_child(a, c2, 0u))
    return fail("reorder child failed");
  count = stygian_editor_tree_list_children(a, pa, got, 8u);
  expect[0] = c2;
  expect[1] = c1;
  if (count != 2u || !ids_equal(got, expect, 2u))
    return fail("children order after reorder mismatch");

  code_need = stygian_editor_build_c23(a, NULL, 0u);
  if (code_need == 0u || code_need > sizeof(code))
    return fail("export code size failed");
  if (stygian_editor_build_c23(a, code, sizeof(code)) != code_need)
    return fail("export code build failed");

  need = stygian_editor_build_project_json(a, NULL, 0u);
  if (need == 0u || need > sizeof(json))
    return fail("project json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need)
    return fail("project json build failed");
  if (!stygian_editor_load_project_json(b, json))
    return fail("project json load failed");

  count = stygian_editor_tree_list_children(b, pa, got, 8u);
  if (count != 2u || !ids_equal(got, expect, 2u))
    return fail("children order after reload mismatch");
  if (!stygian_editor_node_get_parent(b, c1, &pid) || pid != pa)
    return fail("parent link after reload mismatch");
  if (!stygian_editor_node_get_name(b, c1, name, sizeof(name)) ||
      strcmp(name, "CTA Label") != 0) {
    return fail("name after reload mismatch");
  }
  if (!stygian_editor_node_get_locked(b, c2, &flag) || !flag)
    return fail("locked after reload mismatch");
  if (!stygian_editor_node_get_visible(b, pb, &flag) || flag)
    return fail("visible after reload mismatch");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor layers tree smoke\n");
  return 0;
}
