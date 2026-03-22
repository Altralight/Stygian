#include "../include/stygian_editor.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool nearf(float a, float b) { return fabsf(a - b) <= 0.05f; }

static int test_serialization_roundtrip(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorNodeId id = STYGIAN_EDITOR_INVALID_ID;
  size_t need = 0u;
  char *json = NULL;

  cfg.max_nodes = 128u;
  cfg.max_path_points = 512u;
  cfg.max_behaviors = 32u;
  cfg.max_color_tokens = 16u;
  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("serialization create failed");

  rect.x = 12.0f;
  rect.y = 34.0f;
  rect.w = 80.0f;
  rect.h = 28.0f;
  rect.fill = stygian_editor_color_rgba(0.2f, 0.5f, 0.8f, 1.0f);
  rect.visible = true;
  rect.z = 2.0f;
  id = stygian_editor_add_rect(a, &rect);
  if (!id)
    return fail("serialization add rect failed");
  if (!stygian_editor_node_set_name(a, id, "serial_rect"))
    return fail("serialization set name failed");

  need = stygian_editor_build_project_json(a, NULL, 0u);
  if (need < 2u)
    return fail("serialization json size failed");
  json = (char *)malloc(need);
  if (!json)
    return fail("serialization alloc failed");
  if (stygian_editor_build_project_json(a, json, need) != need)
    return fail("serialization json build failed");
  if (!stygian_editor_load_project_json(b, json))
    return fail("serialization json load failed");
  free(json);

  if (stygian_editor_node_count(b) != stygian_editor_node_count(a))
    return fail("serialization node count mismatch");

  stygian_editor_destroy(b);
  stygian_editor_destroy(a);
  return 0;
}

static int test_scene_graph_reparent(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *e = NULL;
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorNodeId f1 = 0u, f2 = 0u, child = 0u, parent = 0u;

  cfg.max_nodes = 128u;
  e = stygian_editor_create(&cfg, NULL);
  if (!e)
    return fail("scene graph create failed");

  frame.w = 400.0f;
  frame.h = 300.0f;
  frame.visible = true;
  frame.fill = stygian_editor_color_rgba(0.1f, 0.1f, 0.1f, 1.0f);
  f1 = stygian_editor_add_frame(e, &frame);
  frame.x = 500.0f;
  frame.y = 0.0f;
  f2 = stygian_editor_add_frame(e, &frame);
  rect.x = 40.0f;
  rect.y = 30.0f;
  rect.w = 80.0f;
  rect.h = 24.0f;
  rect.visible = true;
  rect.fill = stygian_editor_color_rgba(1.0f, 1.0f, 1.0f, 1.0f);
  child = stygian_editor_add_rect(e, &rect);
  if (!f1 || !f2 || !child)
    return fail("scene graph add nodes failed");
  if (!stygian_editor_reparent_node(e, child, f1, false))
    return fail("scene graph first reparent failed");
  if (!stygian_editor_reparent_node(e, child, f2, true))
    return fail("scene graph keep-world reparent failed");
  if (!stygian_editor_node_get_parent(e, child, &parent) || parent != f2)
    return fail("scene graph parent mismatch");

  stygian_editor_destroy(e);
  return 0;
}

static int test_layout_solver(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *e = NULL;
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorFrameAutoLayout layout = {0};
  StygianEditorNodeLayoutOptions opts = {0};
  StygianEditorNodeId root = 0u, a = 0u, b = 0u;
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;

  cfg.max_nodes = 128u;
  e = stygian_editor_create(&cfg, NULL);
  if (!e)
    return fail("layout create failed");

  frame.w = 300.0f;
  frame.h = 140.0f;
  frame.visible = true;
  frame.fill = stygian_editor_color_rgba(0.1f, 0.1f, 0.1f, 1.0f);
  root = stygian_editor_add_frame(e, &frame);
  if (!root)
    return fail("layout add frame failed");

  rect.w = 40.0f;
  rect.h = 20.0f;
  rect.visible = true;
  rect.fill = stygian_editor_color_rgba(1.0f, 1.0f, 1.0f, 1.0f);
  a = stygian_editor_add_rect(e, &rect);
  rect.w = 60.0f;
  b = stygian_editor_add_rect(e, &rect);
  if (!a || !b)
    return fail("layout add children failed");
  if (!stygian_editor_reparent_node(e, a, root, false) ||
      !stygian_editor_reparent_node(e, b, root, false)) {
    return fail("layout reparent failed");
  }

  opts.absolute_position = false;
  opts.sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  opts.sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  if (!stygian_editor_node_set_layout_options(e, a, &opts) ||
      !stygian_editor_node_set_layout_options(e, b, &opts)) {
    return fail("layout set options failed");
  }

  layout.mode = STYGIAN_EDITOR_AUTO_LAYOUT_HORIZONTAL;
  layout.padding_left = 8.0f;
  layout.padding_top = 10.0f;
  layout.padding_right = 8.0f;
  layout.padding_bottom = 10.0f;
  layout.gap = 6.0f;
  layout.primary_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  layout.cross_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  if (!stygian_editor_frame_set_auto_layout(e, root, &layout) ||
      !stygian_editor_frame_recompute_layout(e, root)) {
    return fail("layout recompute failed");
  }

  if (!stygian_editor_node_get_bounds(e, a, &x, &y, &w, &h))
    return fail("layout bounds a failed");
  if (!nearf(x, 8.0f) || !nearf(y, 10.0f))
    return fail("layout a position mismatch");
  if (!stygian_editor_node_get_bounds(e, b, &x, &y, &w, &h))
    return fail("layout bounds b failed");
  if (!nearf(x, 54.0f) || !nearf(y, 10.0f))
    return fail("layout b position mismatch");

  stygian_editor_destroy(e);
  return 0;
}

static int test_property_rules(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *e = NULL;
  StygianEditorRectDesc r = {0};
  StygianEditorNodeId a = 0u, b = 0u;
  StygianEditorPropertyValue value = {0};
  StygianEditorPropertyValue out = {0};
  bool mixed = false;
  uint32_t supported = 0u;
  uint32_t applied = 0u;

  cfg.max_nodes = 64u;
  e = stygian_editor_create(&cfg, NULL);
  if (!e)
    return fail("property create failed");

  r.w = 40.0f;
  r.h = 20.0f;
  r.visible = true;
  r.fill = stygian_editor_color_rgba(1.0f, 1.0f, 1.0f, 1.0f);
  a = stygian_editor_add_rect(e, &r);
  r.w = 90.0f;
  b = stygian_editor_add_rect(e, &r);
  if (!a || !b)
    return fail("property add rects failed");

  if (!stygian_editor_select_node(e, a, false) ||
      !stygian_editor_select_node(e, b, true)) {
    return fail("property select failed");
  }

  if (!stygian_editor_selection_get_property_value(
          e, STYGIAN_EDITOR_PROP_WIDTH, &out, &mixed, &supported))
    return fail("property read mixed failed");
  if (!mixed || supported != 2u)
    return fail("property mixed contract failed");

  value.type = STYGIAN_EDITOR_VALUE_FLOAT;
  value.as.number = 120.0f;
  if (!stygian_editor_selection_set_property_value(
          e, STYGIAN_EDITOR_PROP_WIDTH, &value, &applied))
    return fail("property apply failed");
  if (applied != 2u)
    return fail("property applied count failed");

  stygian_editor_destroy(e);
  return 0;
}

int main(void) {
  if (test_serialization_roundtrip() != 0)
    return 1;
  if (test_scene_graph_reparent() != 0)
    return 1;
  if (test_layout_solver() != 0)
    return 1;
  if (test_property_rules() != 0)
    return 1;
  printf("[PASS] editor core unit suite\n");
  return 0;
}
