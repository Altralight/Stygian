#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static int build_code(const StygianEditor *editor, char *dst, size_t cap) {
  size_t need = stygian_editor_build_c23(editor, NULL, 0u);
  if (need == 0u || need > cap)
    return 0;
  if (stygian_editor_build_c23(editor, dst, cap) != need)
    return 0;
  return 1;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorEllipseDesc ellipse = {0};
  StygianEditorBehaviorRule rule = {0};
  StygianEditorNodeId rect_id = 0u;
  StygianEditorNodeId ellipse_id = 0u;
  StygianEditorNodeId replacement_id = 0u;
  char code_a_1[131072];
  char code_a_2[131072];
  char code_b[131072];
  char json[131072];
  size_t json_need = 0u;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 2048u;
  cfg.max_behaviors = 256u;
  cfg.max_color_tokens = 64u;

  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  rect.x = 24.0f;
  rect.y = 40.0f;
  rect.w = 180.0f;
  rect.h = 64.0f;
  rect.radius[0] = 4.0f;
  rect.radius[1] = 4.0f;
  rect.radius[2] = 4.0f;
  rect.radius[3] = 4.0f;
  rect.fill = stygian_editor_color_rgba(0.2f, 0.4f, 0.8f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;
  rect_id = stygian_editor_add_rect(a, &rect);
  if (rect_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add rect failed");

  ellipse.x = 280.0f;
  ellipse.y = 120.0f;
  ellipse.w = 96.0f;
  ellipse.h = 96.0f;
  ellipse.fill = stygian_editor_color_rgba(0.9f, 0.2f, 0.3f, 0.75f);
  ellipse.visible = true;
  ellipse.z = 2.0f;
  ellipse_id = stygian_editor_add_ellipse(a, &ellipse);
  if (ellipse_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add ellipse failed");

  if (!stygian_editor_set_color_token(
          a, "primary",
          stygian_editor_color_rgba(0.1f, 0.2f, 0.3f, 1.0f)) ||
      !stygian_editor_apply_color_token(a, rect_id, "primary")) {
    return fail("set/apply token failed");
  }

  rule.trigger_node = rect_id;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
  rule.action_kind = STYGIAN_EDITOR_ACTION_ANIMATE;
  rule.animate.target_node = ellipse_id;
  rule.animate.property = STYGIAN_EDITOR_PROP_Y;
  rule.animate.from_value = 120.0f;
  rule.animate.to_value = 200.0f;
  rule.animate.duration_ms = 300u;
  rule.animate.easing = STYGIAN_EDITOR_EASING_LINEAR;
  if (!stygian_editor_add_behavior(a, &rule, NULL))
    return fail("add behavior failed");

  if (!build_code(a, code_a_1, sizeof(code_a_1)))
    return fail("first codegen failed");
  if (!build_code(a, code_a_2, sizeof(code_a_2)))
    return fail("second codegen failed");
  if (strcmp(code_a_1, code_a_2) != 0)
    return fail("codegen is not deterministic across repeated exports");

  json_need = stygian_editor_build_project_json(a, NULL, 0u);
  if (json_need == 0u || json_need > sizeof(json))
    return fail("project json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != json_need)
    return fail("project json write failed");
  if (!stygian_editor_load_project_json(b, json))
    return fail("project load failed");

  if (!build_code(b, code_b, sizeof(code_b)))
    return fail("codegen after load failed");
  if (strcmp(code_a_1, code_b) != 0)
    return fail("export changed after save/load roundtrip");

  if (!stygian_editor_delete_node(a, rect_id))
    return fail("delete node failed");
  replacement_id = stygian_editor_add_rect(a, &rect);
  if (replacement_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("re-add rect failed");
  if (replacement_id <= ellipse_id)
    return fail("node id reuse/regression detected after delete and re-add");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor export determinism smoke\n");
  return 0;
}
