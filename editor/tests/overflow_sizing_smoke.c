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

static bool nearf(float a, float b) { return fabsf(a - b) <= 0.05f; }

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorFrameAutoLayout layout = {0};
  StygianEditorNodeLayoutOptions opts = {0};
  StygianEditorNodeId frame_id = 0u;
  StygianEditorNodeId c1 = 0u;
  StygianEditorNodeId c2 = 0u;
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
  float sx = 0.0f, sy = 0.0f;
  char json[262144];
  char code[262144];
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

  rect.x = 0.0f;
  rect.y = 0.0f;
  rect.w = 80.0f;
  rect.h = 20.0f;
  rect.fill = stygian_editor_color_rgba(1.0f, 1.0f, 1.0f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;
  c1 = stygian_editor_add_rect(a, &rect);
  rect.w = 20.0f;
  c2 = stygian_editor_add_rect(a, &rect);
  if (!c1 || !c2)
    return fail("add child failed");
  if (!stygian_editor_reparent_node(a, c1, frame_id, false) ||
      !stygian_editor_reparent_node(a, c2, frame_id, false)) {
    return fail("reparent failed");
  }

  opts.absolute_position = false;
  opts.sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  opts.sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  if (!stygian_editor_node_set_layout_options(a, c1, &opts))
    return fail("set opts c1 failed");
  opts.sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FILL;
  if (!stygian_editor_node_set_layout_options(a, c2, &opts))
    return fail("set opts c2 failed");

  if (!stygian_editor_node_set_size_limits(a, c2, 140.0f, 160.0f, 0.0f, 0.0f))
    return fail("set size limits failed");

  layout.mode = STYGIAN_EDITOR_AUTO_LAYOUT_HORIZONTAL;
  layout.padding_left = 10.0f;
  layout.padding_right = 10.0f;
  layout.padding_top = 10.0f;
  layout.padding_bottom = 10.0f;
  layout.gap = 10.0f;
  layout.primary_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  layout.cross_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  if (!stygian_editor_frame_set_auto_layout(a, frame_id, &layout))
    return fail("set auto layout failed");
  if (!stygian_editor_frame_set_overflow_policy(
          a, frame_id, STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_X)) {
    return fail("set overflow policy failed");
  }

  if (!stygian_editor_node_get_bounds(a, c2, &x, &y, &w, &h))
    return fail("bounds c2 failed");
  if (!nearf(x, 100.0f) || !nearf(w, 140.0f))
    return fail("min width clamp not applied");

  if (!stygian_editor_frame_set_scroll(a, frame_id, 30.0f, 0.0f))
    return fail("set scroll failed");
  if (!stygian_editor_node_get_bounds(a, c1, &x, &y, &w, &h))
    return fail("bounds c1 scroll failed");
  if (!nearf(x, -20.0f))
    return fail("scroll offset not applied");

  if (!stygian_editor_frame_set_scroll(a, frame_id, 999.0f, 0.0f))
    return fail("set oversized scroll failed");
  if (!stygian_editor_frame_get_scroll(a, frame_id, &sx, &sy))
    return fail("get scroll failed");
  if (!nearf(sx, 50.0f) || !nearf(sy, 0.0f))
    return fail("scroll clamp mismatch");

  if (!stygian_editor_resize_frame(a, frame_id, 400.0f, 100.0f))
    return fail("resize frame failed");
  if (!stygian_editor_node_get_bounds(a, c2, &x, &y, &w, &h))
    return fail("bounds c2 after resize failed");
  if (!nearf(w, 160.0f))
    return fail("max width clamp not applied");

  need = stygian_editor_build_project_json(a, NULL, 0u);
  if (need == 0u || need > sizeof(json))
    return fail("json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need)
    return fail("json build failed");
  if (!stygian_editor_load_project_json(b, json))
    return fail("json load failed");
  if (!stygian_editor_frame_recompute_layout(b, frame_id))
    return fail("recompute after load failed");
  if (!stygian_editor_node_get_bounds(b, c2, &x, &y, &w, &h))
    return fail("bounds c2 after load failed");
  if (!nearf(w, 160.0f))
    return fail("size limits did not persist");

  need = stygian_editor_build_c23(a, NULL, 0u);
  if (need == 0u || need > sizeof(code))
    return fail("build_c23 size failed");
  if (stygian_editor_build_c23(a, code, sizeof(code)) != need)
    return fail("build_c23 failed");
  if (!strstr(code, "stygian_editor_generated_size_limit_records"))
    return fail("generated size-limit records symbol missing");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor overflow sizing smoke\n");
  return 0;
}
