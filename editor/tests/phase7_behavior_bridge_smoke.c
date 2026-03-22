#include "../include/stygian_editor.h"

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
  StygianEditor *editor = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorBehaviorRule rule = {0};
  StygianEditorNodeId a = 0u;
  StygianEditorNodeId b = 0u;
  StygianEditorNodeId root = 0u;
  StygianEditorPropertyValue pv = {0};
  char json[262144];
  char code[524288];
  size_t need = 0u;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 1024u;
  cfg.max_behaviors = 128u;
  cfg.max_color_tokens = 64u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("create editor failed");

  rect.x = 20.0f;
  rect.y = 20.0f;
  rect.w = 100.0f;
  rect.h = 40.0f;
  rect.fill = stygian_editor_color_rgba(0.3f, 0.4f, 0.5f, 1.0f);
  rect.visible = true;
  a = stygian_editor_add_rect(editor, &rect);
  rect.x = 200.0f;
  b = stygian_editor_add_rect(editor, &rect);
  if (!a || !b)
    return fail("add nodes failed");

  rule.trigger_node = a;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_FOCUS_ENTER;
  rule.action_kind = STYGIAN_EDITOR_ACTION_SET_PROPERTY;
  rule.set_property.target_node = b;
  rule.set_property.property = STYGIAN_EDITOR_PROP_X;
  rule.set_property.value = 260.0f;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return fail("add set_property behavior failed");

  memset(&rule, 0, sizeof(rule));
  rule.trigger_node = a;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
  rule.action_kind = STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY;
  rule.toggle_visibility.target_node = b;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return fail("add toggle behavior failed");

  memset(&rule, 0, sizeof(rule));
  rule.trigger_node = a;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_VALUE_CHANGED;
  rule.action_kind = STYGIAN_EDITOR_ACTION_SET_VARIABLE;
  snprintf(rule.set_variable.variable_name,
           sizeof(rule.set_variable.variable_name), "motion.x");
  rule.set_variable.variable_kind = STYGIAN_EDITOR_VARIABLE_NUMBER;
  rule.set_variable.use_active_mode = true;
  rule.set_variable.number_value = 330.0f;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return fail("add set_variable behavior failed");

  memset(&rule, 0, sizeof(rule));
  rule.trigger_node = a;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_RELEASE;
  rule.action_kind = STYGIAN_EDITOR_ACTION_NAVIGATE;
  snprintf(rule.navigate.target, sizeof(rule.navigate.target), "screen://details");
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return fail("add navigate behavior failed");

  stygian_editor_trigger_event(editor, a, STYGIAN_EDITOR_EVENT_FOCUS_ENTER);
  if (!stygian_editor_node_get_property_value(editor, b, STYGIAN_EDITOR_PROP_X, &pv) ||
      pv.type != STYGIAN_EDITOR_VALUE_FLOAT || pv.as.number < 259.0f ||
      pv.as.number > 261.0f) {
    return fail("set_property action did not apply");
  }

  stygian_editor_trigger_event(editor, a, STYGIAN_EDITOR_EVENT_PRESS);
  if (!stygian_editor_node_get_property_value(editor, b, STYGIAN_EDITOR_PROP_VISIBLE,
                                              &pv) ||
      pv.type != STYGIAN_EDITOR_VALUE_BOOL || pv.as.boolean != false) {
    return fail("toggle visibility action did not apply");
  }

  if (!stygian_editor_set_variable_mode(editor, 0u, "default") ||
      !stygian_editor_set_number_variable(editor, "motion.x", 0u, 260.0f) ||
      !stygian_editor_set_active_variable_mode(editor, 0u) ||
      !stygian_editor_bind_node_number_variable(editor, b, STYGIAN_EDITOR_PROP_X,
                                                "motion.x")) {
    return fail("variable bind setup failed");
  }
  stygian_editor_trigger_event(editor, a, STYGIAN_EDITOR_EVENT_VALUE_CHANGED);
  if (!stygian_editor_node_get_property_value(editor, b, STYGIAN_EDITOR_PROP_X, &pv) ||
      pv.as.number < 329.0f || pv.as.number > 331.0f) {
    return fail("set_variable action did not apply");
  }

  if (!stygian_editor_add_control_recipe(editor, STYGIAN_EDITOR_RECIPE_BUTTON,
                                         10.0f, 120.0f, 180.0f, 44.0f,
                                         "Save", &root) ||
      !root) {
    return fail("button recipe failed");
  }
  if (!stygian_editor_add_control_recipe(editor, STYGIAN_EDITOR_RECIPE_SLIDER,
                                         10.0f, 180.0f, 200.0f, 24.0f,
                                         "", &root) ||
      !root) {
    return fail("slider recipe failed");
  }
  if (!stygian_editor_add_control_recipe(editor, STYGIAN_EDITOR_RECIPE_TOGGLE,
                                         10.0f, 220.0f, 56.0f, 28.0f,
                                         "", &root) ||
      !root) {
    return fail("toggle recipe failed");
  }

  need = stygian_editor_build_project_json(editor, NULL, 0u);
  if (need == 0u || need > sizeof(json))
    return fail("build project json size failed");
  if (stygian_editor_build_project_json(editor, json, sizeof(json)) != need)
    return fail("build project json failed");
  if (!strstr(json, "act="))
    return fail("behavior action kind did not persist");

  need = stygian_editor_build_c23(editor, NULL, 0u);
  if (need == 0u || need > sizeof(code))
    return fail("build c23 size failed");
  if (stygian_editor_build_c23(editor, code, sizeof(code)) != need)
    return fail("build c23 failed");
  if (!strstr(code, "stygian_editor_generated_dispatch_event"))
    return fail("dispatch export missing");
  if (!strstr(code, "stygian_editor_generated_hook_set_variable"))
    return fail("set_variable hook missing");
  if (!strstr(code, "STYGIAN_EDITOR_GENERATED_EVENT_FOCUS_ENTER"))
    return fail("focus event export missing");

  stygian_editor_destroy(editor);
  printf("[PASS] editor phase7 behavior bridge smoke\n");
  return 0;
}
