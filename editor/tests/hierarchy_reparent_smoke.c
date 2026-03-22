#include "../include/stygian_editor.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool nearf(float a, float b) { return fabsf(a - b) <= 0.01f; }

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorNodeId root = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId parent = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId child = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId grand = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId pid = STYGIAN_EDITOR_INVALID_ID;
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
  char json[131072];
  size_t need = 0u;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 4096u;
  cfg.max_behaviors = 256u;
  cfg.max_color_tokens = 64u;

  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  rect.w = 200.0f;
  rect.h = 120.0f;
  rect.fill = stygian_editor_color_rgba(0.1f, 0.2f, 0.3f, 1.0f);
  rect.visible = true;
  rect.z = 0.0f;

  rect.x = 100.0f;
  rect.y = 100.0f;
  root = stygian_editor_add_rect(a, &rect);
  if (root == STYGIAN_EDITOR_INVALID_ID)
    return fail("add root failed");

  rect.x = 50.0f;
  rect.y = 25.0f;
  parent = stygian_editor_add_rect(a, &rect);
  if (parent == STYGIAN_EDITOR_INVALID_ID)
    return fail("add parent failed");
  if (!stygian_editor_reparent_node(a, parent, root, false))
    return fail("reparent parent->root failed");

  rect.x = 10.0f;
  rect.y = 5.0f;
  child = stygian_editor_add_rect(a, &rect);
  if (child == STYGIAN_EDITOR_INVALID_ID)
    return fail("add child failed");
  if (!stygian_editor_reparent_node(a, child, parent, false))
    return fail("reparent child->parent failed");

  if (!stygian_editor_node_get_bounds(a, child, &x, &y, &w, &h))
    return fail("child bounds failed");
  if (!nearf(x, 160.0f) || !nearf(y, 130.0f))
    return fail("child world bounds incorrect under hierarchy");

  rect.x = 400.0f;
  rect.y = 300.0f;
  grand = stygian_editor_add_rect(a, &rect);
  if (grand == STYGIAN_EDITOR_INVALID_ID)
    return fail("add grand failed");
  if (!stygian_editor_reparent_node(a, grand, root, false))
    return fail("reparent grand->root failed");
  if (!stygian_editor_node_get_bounds(a, child, &x, &y, &w, &h))
    return fail("child bounds pre keep-world failed");
  if (!stygian_editor_reparent_node(a, child, grand, true))
    return fail("reparent child->grand keep_world failed");
  if (!stygian_editor_node_get_bounds(a, child, &x, &y, &w, &h))
    return fail("child bounds post keep-world failed");
  if (!nearf(x, 160.0f) || !nearf(y, 130.0f))
    return fail("keep_world did not preserve world position");

  if (!stygian_editor_reparent_node(a, child, parent, false))
    return fail("reparent child back to parent failed");
  if (!stygian_editor_node_get_bounds(a, child, &x, &y, &w, &h))
    return fail("child bounds after local reparent failed");
  if (!nearf(x, -190.0f) || !nearf(y, -145.0f)) {
    return fail("local reparent did not move world position as expected");
  }

  if (stygian_editor_reparent_node(a, root, child, false))
    return fail("cycle reparent should fail but succeeded");

  need = stygian_editor_build_project_json(a, NULL, 0u);
  if (need == 0u || need > sizeof(json))
    return fail("json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need)
    return fail("json build failed");
  if (!stygian_editor_load_project_json(b, json))
    return fail("json load failed");
  if (!stygian_editor_node_get_parent(b, parent, &pid) || pid != root)
    return fail("reloaded parent link (parent->root) mismatch");
  if (!stygian_editor_node_get_parent(b, child, &pid) || pid != parent)
    return fail("reloaded parent link (child->parent) mismatch");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor hierarchy reparent smoke\n");
  return 0;
}
