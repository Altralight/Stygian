#include "../include/stygian_editor.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool nearf(float a, float b) { return fabsf(a - b) <= 0.02f; }

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorNodeId a = 0u;
  StygianEditorNodeId b = 0u;
  StygianEditorNodeId parent = 0u;
  StygianEditorNodeId child = 0u;
  StygianEditorNodeId ids[2];
  StygianEditorTransformCommand cmd = {0};
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
  float rotation = 0.0f;
  float before_wx = 0.0f, before_wy = 0.0f;

  cfg.max_nodes = 128u;
  cfg.max_path_points = 1024u;
  cfg.max_behaviors = 128u;
  cfg.max_color_tokens = 32u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("editor create failed");

  rect.w = 100.0f;
  rect.h = 50.0f;
  rect.fill = stygian_editor_color_rgba(0.2f, 0.3f, 0.4f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;

  rect.x = 10.0f;
  rect.y = 10.0f;
  a = stygian_editor_add_rect(editor, &rect);
  rect.x = 200.0f;
  rect.y = 100.0f;
  rect.w = 40.0f;
  rect.h = 30.0f;
  b = stygian_editor_add_rect(editor, &rect);
  if (a == STYGIAN_EDITOR_INVALID_ID || b == STYGIAN_EDITOR_INVALID_ID)
    return fail("add rects failed");

  ids[0] = a;
  ids[1] = b;
  cmd.kind = STYGIAN_EDITOR_TRANSFORM_MOVE;
  cmd.dx = 15.0f;
  cmd.dy = -5.0f;
  if (!stygian_editor_apply_transform(editor, ids, 2u, &cmd))
    return fail("multi-move command failed");
  if (!stygian_editor_node_get_bounds(editor, a, &x, &y, &w, &h))
    return fail("bounds for A after move failed");
  if (!nearf(x, 25.0f) || !nearf(y, 5.0f))
    return fail("A bounds mismatch after move");
  if (!stygian_editor_node_get_bounds(editor, b, &x, &y, &w, &h))
    return fail("bounds for B after move failed");
  if (!nearf(x, 215.0f) || !nearf(y, 95.0f))
    return fail("B bounds mismatch after move");

  memset(&cmd, 0, sizeof(cmd));
  cmd.kind = STYGIAN_EDITOR_TRANSFORM_SCALE;
  cmd.sx = 2.0f;
  cmd.sy = 2.0f;
  cmd.pivot_world_x = 0.0f;
  cmd.pivot_world_y = 0.0f;
  if (!stygian_editor_apply_transform(editor, ids, 2u, &cmd))
    return fail("multi-scale command failed");
  if (!stygian_editor_node_get_bounds(editor, a, &x, &y, &w, &h))
    return fail("bounds for A after scale failed");
  if (!nearf(x, 50.0f) || !nearf(y, 10.0f) || !nearf(w, 200.0f) ||
      !nearf(h, 100.0f))
    return fail("A bounds mismatch after scale");

  memset(&cmd, 0, sizeof(cmd));
  cmd.kind = STYGIAN_EDITOR_TRANSFORM_ROTATE;
  cmd.degrees = 30.0f;
  cmd.pivot_world_x = 0.0f;
  cmd.pivot_world_y = 0.0f;
  if (!stygian_editor_apply_transform(editor, ids, 2u, &cmd))
    return fail("multi-rotate command failed");
  if (!stygian_editor_node_get_rotation(editor, a, &rotation) ||
      !nearf(rotation, 30.0f))
    return fail("rotation for A missing after rotate");
  if (!stygian_editor_node_get_rotation(editor, b, &rotation) ||
      !nearf(rotation, 30.0f))
    return fail("rotation for B missing after rotate");

  rect.x = 100.0f;
  rect.y = 100.0f;
  rect.w = 150.0f;
  rect.h = 90.0f;
  parent = stygian_editor_add_rect(editor, &rect);
  rect.x = 8.0f;
  rect.y = 6.0f;
  rect.w = 40.0f;
  rect.h = 20.0f;
  child = stygian_editor_add_rect(editor, &rect);
  if (parent == STYGIAN_EDITOR_INVALID_ID || child == STYGIAN_EDITOR_INVALID_ID)
    return fail("add parent/child failed");
  if (!stygian_editor_reparent_node(editor, child, parent, false))
    return fail("initial child->parent reparent failed");
  if (!stygian_editor_node_get_bounds(editor, child, &before_wx, &before_wy, &w, &h))
    return fail("child world bounds before reparent command failed");

  ids[0] = child;
  memset(&cmd, 0, sizeof(cmd));
  cmd.kind = STYGIAN_EDITOR_TRANSFORM_REPARENT;
  cmd.new_parent_id = STYGIAN_EDITOR_INVALID_ID;
  cmd.keep_world_transform = true;
  if (!stygian_editor_apply_transform(editor, ids, 1u, &cmd))
    return fail("reparent command failed");
  if (!stygian_editor_node_get_bounds(editor, child, &x, &y, &w, &h))
    return fail("child world bounds after reparent command failed");
  if (!nearf(x, before_wx) || !nearf(y, before_wy))
    return fail("keep-world reparent command changed world position");

  stygian_editor_destroy(editor);
  printf("[PASS] editor transform commands smoke\n");
  return 0;
}
