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

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditorRectDesc rect = {0};
  StygianEditorEllipseDesc ellipse = {0};
  StygianEditorPathDesc path = {0};
  StygianEditorBehaviorRule rule = {0};
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorNodeId rect_id = 0u;
  StygianEditorNodeId ellipse_id = 0u;
  StygianEditorPathId path_id = 0u;
  StygianEditorNodeId path_node_id = 0u;
  StygianEditorBehaviorId behavior_id = 0u;
  StygianEditorDriverRule driver = {0};
  StygianEditorDriverId driver_id = 0u;
  char json[65536];
  size_t required = 0u;
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
  uint32_t ids[8];
  uint32_t id_count = 0u;
  StygianEditorColor c = {0};
  float r0 = 0.0f, r1 = 0.0f, r2 = 0.0f, r3 = 0.0f;

  cfg.max_nodes = 128u;
  cfg.max_path_points = 1024u;
  cfg.max_behaviors = 128u;
  cfg.max_color_tokens = 32u;

  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  rect.x = 42.0f;
  rect.y = 84.0f;
  rect.w = 260.0f;
  rect.h = 96.0f;
  rect.radius[0] = 8.0f;
  rect.radius[1] = 12.0f;
  rect.radius[2] = 16.0f;
  rect.radius[3] = 20.0f;
  rect.fill = stygian_editor_color_rgba(0.2f, 0.3f, 0.4f, 0.9f);
  rect.visible = true;
  rect.z = 2.0f;
  rect_id = stygian_editor_add_rect(a, &rect);
  if (rect_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add rect failed");

  ellipse.x = 300.0f;
  ellipse.y = 140.0f;
  ellipse.w = 120.0f;
  ellipse.h = 120.0f;
  ellipse.fill = stygian_editor_color_rgba(0.7f, 0.1f, 0.2f, 1.0f);
  ellipse.visible = true;
  ellipse.z = 3.0f;
  ellipse_id = stygian_editor_add_ellipse(a, &ellipse);
  if (ellipse_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add ellipse failed");

  path.stroke = stygian_editor_color_rgba(0.9f, 0.9f, 0.1f, 1.0f);
  path.thickness = 3.0f;
  path.closed = true;
  path.visible = true;
  path.z = 1.0f;
  path_id = stygian_editor_path_begin(a, &path);
  if (path_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("path begin failed");
  if (!stygian_editor_path_add_point(a, path_id, 10.0f, 10.0f, false) ||
      !stygian_editor_path_add_point(a, path_id, 20.0f, 60.0f, false) ||
      !stygian_editor_path_add_point(a, path_id, 60.0f, 20.0f, false)) {
    return fail("path add point failed");
  }
  path_node_id = stygian_editor_path_commit(a, path_id);
  if (path_node_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("path commit failed");

  if (!stygian_editor_set_color_token(
          a, "primary",
          stygian_editor_color_rgba(0.12f, 0.34f, 0.56f, 0.78f))) {
    return fail("set color token failed");
  }
  if (!stygian_editor_apply_color_token(a, rect_id, "primary"))
    return fail("apply color token failed");

  rule.trigger_node = rect_id;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
  rule.action_kind = STYGIAN_EDITOR_ACTION_ANIMATE;
  rule.animate.target_node = ellipse_id;
  rule.animate.property = STYGIAN_EDITOR_PROP_Y;
  rule.animate.from_value = NAN;
  rule.animate.to_value = 10.0f;
  rule.animate.duration_ms = 250u;
  rule.animate.easing = STYGIAN_EDITOR_EASING_OUT_CUBIC;
  if (!stygian_editor_add_behavior(a, &rule, &behavior_id))
    return fail("add behavior failed");
  if (!stygian_editor_set_variable_mode(a, 0u, "default") ||
      !stygian_editor_set_number_variable(a, "motion.y", 0u, ellipse.y) ||
      !stygian_editor_bind_node_number_variable(a, ellipse_id,
                                                STYGIAN_EDITOR_PROP_Y,
                                                "motion.y")) {
    return fail("driver variable setup failed");
  }
  driver.source_node = rect_id;
  driver.source_property = STYGIAN_EDITOR_PROP_VALUE;
  snprintf(driver.variable_name, sizeof(driver.variable_name), "motion.y");
  driver.use_active_mode = true;
  driver.mode_index = 0u;
  driver.in_min = 0.0f;
  driver.in_max = 1.0f;
  driver.out_min = 100.0f;
  driver.out_max = 220.0f;
  driver.clamp_output = true;
  if (!stygian_editor_add_driver(a, &driver, &driver_id))
    return fail("add driver failed");

  stygian_editor_select_node(a, rect_id, false);
  stygian_editor_select_node(a, ellipse_id, true);

  required = stygian_editor_build_project_json(a, NULL, 0u);
  if (required == 0u || required > sizeof(json))
    return fail("build_project_json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != required)
    return fail("build_project_json output failed");
  if (!strstr(json, "\"transition_clips\": ["))
    return fail("transition clips section missing from project json");
  if (!strstr(json, "\"drivers\": ["))
    return fail("drivers section missing from project json");
  if (!stygian_editor_load_project_json(b, json))
    return fail("load_project_json failed");

  if (stygian_editor_node_count(b) != 3u)
    return fail("node count mismatch after roundtrip");
  if (stygian_editor_behavior_count(b) != 1u)
    return fail("behavior count mismatch after roundtrip");
  if (stygian_editor_driver_count(b) != 1u)
    return fail("driver count mismatch after roundtrip");
  if (!stygian_editor_get_color_token(b, "primary", &c))
    return fail("color token missing after roundtrip");
  if (!stygian_editor_node_get_bounds(b, rect_id, &x, &y, &w, &h))
    return fail("rect bounds missing after roundtrip");
  if (fabsf(x - rect.x) > 0.01f || fabsf(y - rect.y) > 0.01f)
    return fail("rect position mismatch after roundtrip");
  if (!stygian_editor_node_get_corner_radii(b, rect_id, &r0, &r1, &r2, &r3))
    return fail("rect radii missing after roundtrip");
  if (fabsf(r0 - rect.radius[0]) > 0.01f || fabsf(r3 - rect.radius[3]) > 0.01f)
    return fail("rect radii mismatch after roundtrip");

  id_count = stygian_editor_selected_nodes(b, ids, 8u);
  if (id_count != 2u)
    return fail("selection count mismatch after roundtrip");
  if (!stygian_editor_apply_driver_sample(b, driver_id, 0.5f))
    return fail("driver sample apply failed after roundtrip");
  if (!stygian_editor_node_get_bounds(b, ellipse_id, &x, &y, &w, &h))
    return fail("ellipse bounds missing after driver sample");
  if (fabsf(y - 160.0f) > 0.5f)
    return fail("driver output mismatch after roundtrip");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor project roundtrip smoke\n");
  return 0;
}
