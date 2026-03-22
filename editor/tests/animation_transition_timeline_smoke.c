#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool read_opacity(const StygianEditor *editor, StygianEditorNodeId node_id,
                         float *out_opacity) {
  StygianEditorPropertyValue pv = {0};
  if (!out_opacity)
    return false;
  if (!stygian_editor_node_get_property_value(editor, node_id,
                                              STYGIAN_EDITOR_PROP_OPACITY, &pv))
    return false;
  if (pv.type != STYGIAN_EDITOR_VALUE_FLOAT)
    return false;
  *out_opacity = pv.as.number;
  return true;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorBehaviorRule rule = {0};
  StygianEditorNodeId trigger = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId target = STYGIAN_EDITOR_INVALID_ID;
  float opacity = 0.0f;
  char json[262144];
  char code[524288];
  size_t need = 0u;

  cfg.max_nodes = 128u;
  cfg.max_path_points = 512u;
  cfg.max_behaviors = 128u;
  cfg.max_color_tokens = 16u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("create editor failed");

  rect.x = 10.0f;
  rect.y = 10.0f;
  rect.w = 120.0f;
  rect.h = 40.0f;
  rect.visible = true;
  rect.fill = stygian_editor_color_rgba(0.2f, 0.3f, 0.4f, 1.0f);
  trigger = stygian_editor_add_rect(editor, &rect);
  rect.x = 220.0f;
  rect.fill = stygian_editor_color_rgba(0.7f, 0.4f, 0.3f, 1.0f);
  target = stygian_editor_add_rect(editor, &rect);
  if (!trigger || !target)
    return fail("seed nodes failed");

  rule.trigger_node = trigger;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_HOVER_ENTER;
  rule.action_kind = STYGIAN_EDITOR_ACTION_ANIMATE;
  rule.animate.target_node = target;
  rule.animate.property = STYGIAN_EDITOR_PROP_OPACITY;
  rule.animate.from_value = NAN;
  rule.animate.to_value = 0.20f;
  rule.animate.duration_ms = 200u;
  rule.animate.easing = STYGIAN_EDITOR_EASING_OUT_CUBIC;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return fail("add hover enter transition failed");

  memset(&rule, 0, sizeof(rule));
  rule.trigger_node = trigger;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_HOVER_LEAVE;
  rule.action_kind = STYGIAN_EDITOR_ACTION_ANIMATE;
  rule.animate.target_node = target;
  rule.animate.property = STYGIAN_EDITOR_PROP_OPACITY;
  rule.animate.from_value = NAN;
  rule.animate.to_value = 1.0f;
  rule.animate.duration_ms = 120u;
  rule.animate.easing = STYGIAN_EDITOR_EASING_LINEAR;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return fail("add hover leave transition failed");

  stygian_editor_trigger_event(editor, trigger, STYGIAN_EDITOR_EVENT_HOVER_ENTER);
  stygian_editor_tick(editor, 0u);
  stygian_editor_tick(editor, 100u);
  if (!read_opacity(editor, target, &opacity))
    return fail("opacity read failed at half");
  if (!(opacity < 1.0f && opacity > 0.20f))
    return fail("halfway transition opacity out of expected range");
  stygian_editor_tick(editor, 240u);
  if (!read_opacity(editor, target, &opacity))
    return fail("opacity read failed at end");
  if (opacity < 0.18f || opacity > 0.22f)
    return fail("transition end opacity mismatch");

  stygian_editor_trigger_event(editor, trigger, STYGIAN_EDITOR_EVENT_HOVER_LEAVE);
  stygian_editor_tick(editor, 360u);
  if (!read_opacity(editor, target, &opacity))
    return fail("opacity read failed after leave");
  if (opacity < 0.98f || opacity > 1.02f)
    return fail("leave transition did not restore opacity");

  need = stygian_editor_build_project_json(editor, NULL, 0u);
  if (need < 2u || need > sizeof(json))
    return fail("build project json size failed");
  if (stygian_editor_build_project_json(editor, json, sizeof(json)) != need)
    return fail("build project json failed");
  if (!strstr(json, "\"transition_clips\": ["))
    return fail("transition clips section missing");
  if (!strstr(json, "ev=hover_enter") || !strstr(json, "ev=hover_leave"))
    return fail("transition clip events missing");
  if (!strstr(json, "ease=out_cubic") || !strstr(json, "ease=linear"))
    return fail("transition easing labels missing");

  need = stygian_editor_build_c23(editor, NULL, 0u);
  if (need < 2u || need > sizeof(code))
    return fail("build c23 size failed");
  if (stygian_editor_build_c23(editor, code, sizeof(code)) != need)
    return fail("build c23 failed");
  if (!strstr(code, "stygian_editor_generated_dispatch_event"))
    return fail("dispatch export missing");

  stygian_editor_destroy(editor);
  printf("[PASS] editor animation transition timeline smoke\n");
  return 0;
}
