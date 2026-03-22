#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool contains(const char *text, const char *needle) {
  return text && needle && strstr(text, needle) != NULL;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorBehaviorRule rule = {0};
  StygianEditorDriverRule driver = {0};
  StygianEditorDriverId driver_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId trigger = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId paint_node = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId interaction_node = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorPropertyValue pv = {0};
  StygianEditorColor c = {0};
  static char c23[524288];
  size_t need = 0u;

  cfg.max_nodes = 128u;
  cfg.max_path_points = 512u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 16u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("create editor failed");

  rect.visible = true;
  rect.fill = stygian_editor_color_rgba(0.3f, 0.4f, 0.5f, 1.0f);
  rect.x = 20.0f;
  rect.y = 24.0f;
  rect.w = 80.0f;
  rect.h = 40.0f;
  trigger = stygian_editor_add_rect(editor, &rect);
  rect.x = 150.0f;
  paint_node = stygian_editor_add_rect(editor, &rect);
  rect.x = 260.0f;
  interaction_node = stygian_editor_add_rect(editor, &rect);
  if (!trigger || !paint_node || !interaction_node)
    return fail("add nodes failed");

  if (!stygian_editor_set_variable_mode(editor, 0u, "light") ||
      !stygian_editor_set_variable_mode(editor, 1u, "dark")) {
    return fail("set variable modes failed");
  }
  if (!stygian_editor_set_color_variable(
          editor, "theme.panel", 0u,
          stygian_editor_color_rgba(0.85f, 0.85f, 0.85f, 1.0f)) ||
      !stygian_editor_set_color_variable(
          editor, "theme.panel", 1u,
          stygian_editor_color_rgba(0.16f, 0.18f, 0.20f, 1.0f))) {
    return fail("set color variable failed");
  }
  if (!stygian_editor_set_number_variable(editor, "layout.w", 0u, 100.0f) ||
      !stygian_editor_set_number_variable(editor, "layout.w", 1u, 180.0f) ||
      !stygian_editor_set_number_variable(editor, "motion.x", 0u, 260.0f) ||
      !stygian_editor_set_number_variable(editor, "motion.x", 1u, 320.0f)) {
    return fail("set number variable failed");
  }
  if (!stygian_editor_bind_node_color_variable(editor, paint_node,
                                               "theme.panel") ||
      !stygian_editor_bind_node_number_variable(editor, paint_node,
                                                STYGIAN_EDITOR_PROP_WIDTH,
                                                "layout.w") ||
      !stygian_editor_bind_node_number_variable(editor, interaction_node,
                                                STYGIAN_EDITOR_PROP_X,
                                                "motion.x")) {
    return fail("bind variables failed");
  }
  if (!stygian_editor_set_active_variable_mode(editor, 1u) ||
      !stygian_editor_apply_active_variable_mode(editor)) {
    return fail("apply dark mode failed");
  }
  if (!stygian_editor_node_get_color(editor, paint_node, &c))
    return fail("get bound color failed");
  if (c.r > 0.3f)
    return fail("dark mode paint variable not applied");
  if (!stygian_editor_node_get_property_value(editor, paint_node,
                                              STYGIAN_EDITOR_PROP_WIDTH, &pv) ||
      pv.type != STYGIAN_EDITOR_VALUE_FLOAT || pv.as.number < 179.0f ||
      pv.as.number > 181.0f) {
    return fail("layout width variable not applied");
  }

  memset(&rule, 0, sizeof(rule));
  rule.trigger_node = trigger;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
  rule.action_kind = STYGIAN_EDITOR_ACTION_SET_VARIABLE;
  snprintf(rule.set_variable.variable_name,
           sizeof(rule.set_variable.variable_name), "motion.x");
  rule.set_variable.variable_kind = STYGIAN_EDITOR_VARIABLE_NUMBER;
  rule.set_variable.use_active_mode = true;
  rule.set_variable.number_value = 333.0f;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return fail("add set_variable behavior failed");
  stygian_editor_trigger_event(editor, trigger, STYGIAN_EDITOR_EVENT_PRESS);
  if (!stygian_editor_node_get_property_value(editor, interaction_node,
                                              STYGIAN_EDITOR_PROP_X, &pv) ||
      pv.type != STYGIAN_EDITOR_VALUE_FLOAT || pv.as.number < 332.0f ||
      pv.as.number > 334.0f) {
    return fail("interaction variable assignment not applied");
  }

  memset(&driver, 0, sizeof(driver));
  driver.source_node = trigger;
  driver.source_property = STYGIAN_EDITOR_PROP_VALUE;
  snprintf(driver.variable_name, sizeof(driver.variable_name), "motion.x");
  driver.use_active_mode = true;
  driver.in_min = 0.0f;
  driver.in_max = 1.0f;
  driver.out_min = 200.0f;
  driver.out_max = 420.0f;
  driver.clamp_output = true;
  if (!stygian_editor_add_driver(editor, &driver, &driver_id))
    return fail("add driver failed");
  if (!stygian_editor_apply_driver_sample(editor, driver_id, 0.5f))
    return fail("apply driver sample failed");
  if (!stygian_editor_node_get_property_value(editor, interaction_node,
                                              STYGIAN_EDITOR_PROP_X, &pv) ||
      pv.type != STYGIAN_EDITOR_VALUE_FLOAT || pv.as.number < 309.0f ||
      pv.as.number > 311.0f) {
    return fail("driver sample did not update bound property");
  }

  need = stygian_editor_build_c23(editor, NULL, 0u);
  if (need < 2u || need > sizeof(c23))
    return fail("build c23 size failed");
  if (stygian_editor_build_c23(editor, c23, sizeof(c23)) != need)
    return fail("build c23 failed");
  if (!contains(c23, "kStygianEditorGeneratedVariableBindings"))
    return fail("missing exported variable bindings");
  if (!contains(c23, "stygian_editor_generated_apply_variable_mode"))
    return fail("missing exported variable mode helper");
  if (!contains(c23, "stygian_editor_generated_apply_variable_assignment"))
    return fail("missing exported variable assignment helper");
  if (!contains(c23, "kStygianEditorGeneratedDrivers"))
    return fail("missing exported driver table");
  if (!contains(c23, "stygian_editor_generated_apply_driver_sample"))
    return fail("missing exported driver helper");
  if (!contains(c23, "gStygianEditorGeneratedActiveVariableMode"))
    return fail("missing exported active variable mode state");

  stygian_editor_destroy(editor);
  printf("[PASS] editor variable binding parity smoke\n");
  return 0;
}
