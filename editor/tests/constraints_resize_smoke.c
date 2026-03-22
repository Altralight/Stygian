#include "../include/stygian_editor.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool nearf(float a, float b) { return fabsf(a - b) <= 0.05f; }

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorNodeId frame_id = 0u;
  StygianEditorNodeId right_id = 0u;
  StygianEditorNodeId stretch_id = 0u;
  StygianEditorNodeId center_id = 0u;
  StygianEditorNodeId scale_id = 0u;
  StygianEditorNodeId left_id = 0u;
  StygianEditorNodeId top_bottom_id = 0u;
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
  char json[262144];
  size_t need = 0u;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 4096u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 16u;
  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 200.0f;
  frame.h = 100.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.1f, 0.1f, 0.1f, 1.0f);
  frame.visible = true;
  frame.z = 0.0f;
  frame_id = stygian_editor_add_frame(a, &frame);
  if (!frame_id)
    return fail("add frame failed");

  rect.w = 20.0f;
  rect.h = 10.0f;
  rect.fill = stygian_editor_color_rgba(1.0f, 1.0f, 1.0f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;

  rect.x = 170.0f;
  rect.y = 80.0f;
  right_id = stygian_editor_add_rect(a, &rect);
  rect.x = 10.0f;
  rect.y = 30.0f;
  rect.w = 180.0f;
  rect.h = 20.0f;
  stretch_id = stygian_editor_add_rect(a, &rect);
  rect.x = 75.0f;
  rect.y = 35.0f;
  rect.w = 50.0f;
  rect.h = 30.0f;
  center_id = stygian_editor_add_rect(a, &rect);
  rect.x = 20.0f;
  rect.y = 10.0f;
  rect.w = 40.0f;
  rect.h = 20.0f;
  scale_id = stygian_editor_add_rect(a, &rect);
  rect.x = 12.0f;
  rect.y = 22.0f;
  rect.w = 30.0f;
  rect.h = 15.0f;
  left_id = stygian_editor_add_rect(a, &rect);
  rect.x = 150.0f;
  rect.y = 10.0f;
  rect.w = 20.0f;
  rect.h = 80.0f;
  top_bottom_id = stygian_editor_add_rect(a, &rect);
  if (!right_id || !stretch_id || !center_id || !scale_id || !left_id ||
      !top_bottom_id)
    return fail("add children failed");

  if (!stygian_editor_reparent_node(a, right_id, frame_id, false) ||
      !stygian_editor_reparent_node(a, stretch_id, frame_id, false) ||
      !stygian_editor_reparent_node(a, center_id, frame_id, false) ||
      !stygian_editor_reparent_node(a, scale_id, frame_id, false) ||
      !stygian_editor_reparent_node(a, left_id, frame_id, false) ||
      !stygian_editor_reparent_node(a, top_bottom_id, frame_id, false)) {
    return fail("reparent failed");
  }

  if (!stygian_editor_node_set_constraints(
          a, right_id, STYGIAN_EDITOR_CONSTRAINT_H_RIGHT,
          STYGIAN_EDITOR_CONSTRAINT_V_BOTTOM) ||
      !stygian_editor_node_set_constraints(
          a, stretch_id, STYGIAN_EDITOR_CONSTRAINT_H_LEFT_RIGHT,
          STYGIAN_EDITOR_CONSTRAINT_V_TOP) ||
      !stygian_editor_node_set_constraints(
          a, center_id, STYGIAN_EDITOR_CONSTRAINT_H_CENTER,
          STYGIAN_EDITOR_CONSTRAINT_V_CENTER) ||
      !stygian_editor_node_set_constraints(
          a, scale_id, STYGIAN_EDITOR_CONSTRAINT_H_SCALE,
          STYGIAN_EDITOR_CONSTRAINT_V_SCALE) ||
      !stygian_editor_node_set_constraints(
          a, left_id, STYGIAN_EDITOR_CONSTRAINT_H_LEFT,
          STYGIAN_EDITOR_CONSTRAINT_V_TOP) ||
      !stygian_editor_node_set_constraints(
          a, top_bottom_id, STYGIAN_EDITOR_CONSTRAINT_H_RIGHT,
          STYGIAN_EDITOR_CONSTRAINT_V_TOP_BOTTOM)) {
    return fail("set constraints failed");
  }

  if (!stygian_editor_resize_frame(a, frame_id, 400.0f, 200.0f))
    return fail("resize frame failed");

  if (!stygian_editor_node_get_bounds(a, left_id, &x, &y, &w, &h))
    return fail("left bounds failed");
  if (!nearf(x, 12.0f) || !nearf(y, 22.0f))
    return fail("left/top constraints mismatch");

  if (!stygian_editor_node_get_bounds(a, top_bottom_id, &x, &y, &w, &h))
    return fail("top-bottom bounds failed");
  if (!nearf(y, 10.0f) || !nearf(h, 180.0f))
    return fail("top-bottom stretch mismatch");

  if (!stygian_editor_node_get_bounds(a, right_id, &x, &y, &w, &h))
    return fail("right bounds failed");
  if (!nearf(x, 370.0f) || !nearf(y, 180.0f))
    return fail("right/bottom constraints mismatch");

  if (!stygian_editor_node_get_bounds(a, stretch_id, &x, &y, &w, &h))
    return fail("stretch bounds failed");
  if (!nearf(x, 10.0f) || !nearf(w, 380.0f))
    return fail("left-right stretch mismatch");

  if (!stygian_editor_node_get_bounds(a, center_id, &x, &y, &w, &h))
    return fail("center bounds failed");
  if (!nearf(x, 175.0f) || !nearf(y, 85.0f))
    return fail("center constraints mismatch");

  if (!stygian_editor_node_get_bounds(a, scale_id, &x, &y, &w, &h))
    return fail("scale bounds failed");
  if (!nearf(x, 40.0f) || !nearf(y, 20.0f) || !nearf(w, 80.0f) ||
      !nearf(h, 40.0f)) {
    return fail("scale constraints mismatch");
  }

  need = stygian_editor_build_project_json(a, NULL, 0u);
  if (need == 0u || need > sizeof(json))
    return fail("json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need)
    return fail("json build failed");
  if (!stygian_editor_load_project_json(b, json))
    return fail("json load failed");

  if (!stygian_editor_resize_frame(b, frame_id, 300.0f, 150.0f))
    return fail("resize frame after load failed");
  if (!stygian_editor_node_get_bounds(b, right_id, &x, &y, &w, &h))
    return fail("right bounds after load failed");
  if (!nearf(x, 270.0f) || !nearf(y, 130.0f))
    return fail("right/bottom after load mismatch");
  if (!stygian_editor_node_get_bounds(b, left_id, &x, &y, &w, &h))
    return fail("left bounds after load failed");
  if (!nearf(x, 12.0f) || !nearf(y, 22.0f))
    return fail("left/top after load mismatch");
  if (!stygian_editor_node_get_bounds(b, top_bottom_id, &x, &y, &w, &h))
    return fail("top-bottom bounds after load failed");
  if (!nearf(y, 10.0f) || !nearf(h, 130.0f))
    return fail("top-bottom after load mismatch");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor constraints resize smoke\n");
  return 0;
}
