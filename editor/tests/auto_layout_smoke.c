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
  StygianEditorFrameAutoLayout layout = {0};
  StygianEditorNodeLayoutOptions child_opts = {0};
  StygianEditorNodeId frame_id = 0u;
  StygianEditorNodeId c1 = 0u;
  StygianEditorNodeId c2 = 0u;
  StygianEditorNodeId c3 = 0u;
  StygianEditorNodeId abs_id = 0u;
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
  frame.w = 300.0f;
  frame.h = 200.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.1f, 0.1f, 0.1f, 1.0f);
  frame.visible = true;
  frame.z = 0.0f;
  frame_id = stygian_editor_add_frame(a, &frame);
  if (!frame_id)
    return fail("add frame failed");

  rect.fill = stygian_editor_color_rgba(1.0f, 1.0f, 1.0f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;

  rect.x = 0.0f;
  rect.y = 0.0f;
  rect.w = 50.0f;
  rect.h = 20.0f;
  c1 = stygian_editor_add_rect(a, &rect);
  rect.w = 40.0f;
  rect.h = 20.0f;
  c2 = stygian_editor_add_rect(a, &rect);
  rect.w = 20.0f;
  rect.h = 20.0f;
  c3 = stygian_editor_add_rect(a, &rect);
  rect.x = 250.0f;
  rect.y = 150.0f;
  rect.w = 30.0f;
  rect.h = 30.0f;
  abs_id = stygian_editor_add_rect(a, &rect);
  if (!c1 || !c2 || !c3 || !abs_id)
    return fail("add children failed");
  if (!stygian_editor_reparent_node(a, c1, frame_id, false) ||
      !stygian_editor_reparent_node(a, c2, frame_id, false) ||
      !stygian_editor_reparent_node(a, c3, frame_id, false) ||
      !stygian_editor_reparent_node(a, abs_id, frame_id, false)) {
    return fail("reparent failed");
  }

  child_opts.absolute_position = false;
  child_opts.sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  child_opts.sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  if (!stygian_editor_node_set_layout_options(a, c1, &child_opts))
    return fail("set c1 layout options failed");
  child_opts.sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_HUG;
  if (!stygian_editor_node_set_layout_options(a, c2, &child_opts))
    return fail("set c2 layout options failed");
  child_opts.sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FILL;
  if (!stygian_editor_node_set_layout_options(a, c3, &child_opts))
    return fail("set c3 layout options failed");
  child_opts.absolute_position = true;
  child_opts.sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  if (!stygian_editor_node_set_layout_options(a, abs_id, &child_opts))
    return fail("set absolute options failed");

  layout.mode = STYGIAN_EDITOR_AUTO_LAYOUT_HORIZONTAL;
  layout.padding_left = 10.0f;
  layout.padding_right = 10.0f;
  layout.padding_top = 10.0f;
  layout.padding_bottom = 10.0f;
  layout.gap = 10.0f;
  layout.primary_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  layout.cross_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  if (!stygian_editor_frame_set_auto_layout(a, frame_id, &layout))
    return fail("set frame auto layout failed");
  if (!stygian_editor_frame_recompute_layout(a, frame_id))
    return fail("recompute layout failed");

  if (!stygian_editor_node_get_bounds(a, c1, &x, &y, &w, &h))
    return fail("bounds c1 failed");
  if (!nearf(x, 10.0f) || !nearf(y, 10.0f) || !nearf(w, 50.0f) || !nearf(h, 20.0f))
    return fail("c1 layout mismatch");
  if (!stygian_editor_node_get_bounds(a, c2, &x, &y, &w, &h))
    return fail("bounds c2 failed");
  if (!nearf(x, 70.0f) || !nearf(y, 10.0f) || !nearf(w, 40.0f) || !nearf(h, 20.0f))
    return fail("c2 layout mismatch");
  if (!stygian_editor_node_get_bounds(a, c3, &x, &y, &w, &h))
    return fail("bounds c3 failed");
  if (!nearf(x, 120.0f) || !nearf(y, 10.0f) || !nearf(w, 170.0f) ||
      !nearf(h, 20.0f)) {
    return fail("c3 fill layout mismatch");
  }
  if (!stygian_editor_node_get_bounds(a, abs_id, &x, &y, &w, &h))
    return fail("bounds abs failed");
  if (!nearf(x, 250.0f) || !nearf(y, 150.0f))
    return fail("absolute child should not move");

  if (!stygian_editor_frame_recompute_layout(a, frame_id))
    return fail("recompute layout second run failed");
  if (!stygian_editor_node_get_bounds(a, c3, &x, &y, &w, &h))
    return fail("bounds c3 second run failed");
  if (!nearf(x, 120.0f) || !nearf(w, 170.0f))
    return fail("layout recompute not deterministic");

  need = stygian_editor_build_project_json(a, NULL, 0u);
  if (need == 0u || need > sizeof(json))
    return fail("json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need)
    return fail("json build failed");
  if (!stygian_editor_load_project_json(b, json))
    return fail("json load failed");
  if (!stygian_editor_frame_recompute_layout(b, frame_id))
    return fail("recompute after load failed");
  if (!stygian_editor_node_get_bounds(b, c3, &x, &y, &w, &h))
    return fail("bounds c3 after load failed");
  if (!nearf(x, 120.0f) || !nearf(w, 170.0f))
    return fail("layout after load mismatch");

  rect.w = 60.0f;
  rect.h = 20.0f;
  if (!stygian_editor_node_set_size(a, c1, 60.0f, 20.0f) ||
      !stygian_editor_node_set_size(a, c2, 60.0f, 20.0f) ||
      !stygian_editor_node_set_size(a, c3, 60.0f, 20.0f)) {
    return fail("resize wrap children failed");
  }
  child_opts.absolute_position = false;
  child_opts.sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  child_opts.sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  if (!stygian_editor_node_set_layout_options(a, c1, &child_opts) ||
      !stygian_editor_node_set_layout_options(a, c2, &child_opts) ||
      !stygian_editor_node_set_layout_options(a, c3, &child_opts)) {
    return fail("set fixed wrap child options failed");
  }
  layout.wrap = STYGIAN_EDITOR_AUTO_LAYOUT_WRAP;
  if (!stygian_editor_frame_set_auto_layout(a, frame_id, &layout))
    return fail("set wrap auto layout failed");
  if (!stygian_editor_resize_frame(a, frame_id, 150.0f, 200.0f))
    return fail("resize wrap frame failed");
  if (!stygian_editor_frame_recompute_layout(a, frame_id))
    return fail("recompute wrap layout failed");
  if (!stygian_editor_node_get_bounds(a, c1, &x, &y, &w, &h) ||
      !nearf(x, 10.0f) || !nearf(y, 10.0f))
    return fail("wrap c1 mismatch");
  if (!stygian_editor_node_get_bounds(a, c2, &x, &y, &w, &h) ||
      !nearf(x, 80.0f) || !nearf(y, 10.0f))
    return fail("wrap c2 mismatch");
  if (!stygian_editor_node_get_bounds(a, c3, &x, &y, &w, &h) ||
      !nearf(x, 10.0f) || !nearf(y, 40.0f))
    return fail("wrap c3 mismatch");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor auto layout smoke\n");
  return 0;
}
