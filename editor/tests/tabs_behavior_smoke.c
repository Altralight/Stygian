#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool get_float_prop(const StygianEditor *editor, StygianEditorNodeId node_id,
                           StygianEditorPropertyKind property, float *out_value) {
  StygianEditorPropertyValue value = {0};
  if (!out_value)
    return false;
  if (!stygian_editor_node_get_property_value(editor, node_id, property, &value))
    return false;
  if (value.type != STYGIAN_EDITOR_VALUE_FLOAT)
    return false;
  *out_value = value.as.number;
  return true;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorNodeId tabs_root = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId headers[8];
  StygianEditorNodeId content_nodes[3];
  StygianEditorFrameDesc content = {0};
  uint32_t header_count = 0u;
  static char code[524288];
  size_t need = 0u;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 1024u;
  cfg.max_behaviors = 256u;
  cfg.max_color_tokens = 32u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("create editor failed");

  if (!stygian_editor_add_control_recipe(editor, STYGIAN_EDITOR_RECIPE_TABS,
                                         20.0f, 20.0f, 300.0f, 34.0f, "Tabs",
                                         &tabs_root) ||
      !tabs_root) {
    return fail("tabs recipe failed");
  }

  header_count = stygian_editor_tree_list_children(editor, tabs_root, headers, 8u);
  if (header_count < 3u)
    return fail("tabs recipe did not emit expected header children");

  content.x = 20.0f;
  content.y = 62.0f;
  content.w = 300.0f;
  content.h = 180.0f;
  content.clip_content = true;
  content.z = 0.0f;
  for (uint32_t i = 0u; i < 3u; ++i) {
    content.fill = stygian_editor_color_rgba(0.10f + 0.03f * (float)i,
                                             0.12f + 0.02f * (float)i,
                                             0.16f + 0.02f * (float)i, 1.0f);
    content.visible = true;
    content_nodes[i] = stygian_editor_add_frame(editor, &content);
    if (!content_nodes[i])
      return fail("content frame add failed");
    (void)stygian_editor_node_set_opacity(editor, content_nodes[i],
                                          i == 0u ? 1.0f : 0.08f);
  }

  for (uint32_t i = 0u; i < 3u; ++i) {
    for (uint32_t j = 0u; j < 3u; ++j) {
      StygianEditorBehaviorRule rule = {0};
      rule.trigger_node = headers[i];
      rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
      rule.action_kind = STYGIAN_EDITOR_ACTION_SET_PROPERTY;
      rule.set_property.target_node = headers[j];
      rule.set_property.property = STYGIAN_EDITOR_PROP_OPACITY;
      rule.set_property.value = (i == j) ? 1.0f : 0.62f;
      if (!stygian_editor_add_behavior(editor, &rule, NULL))
        return fail("header behavior add failed");
    }
    for (uint32_t j = 0u; j < 3u; ++j) {
      StygianEditorBehaviorRule rule = {0};
      rule.trigger_node = headers[i];
      rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
      rule.action_kind = STYGIAN_EDITOR_ACTION_SET_PROPERTY;
      rule.set_property.target_node = content_nodes[j];
      rule.set_property.property = STYGIAN_EDITOR_PROP_OPACITY;
      rule.set_property.value = (i == j) ? 1.0f : 0.08f;
      if (!stygian_editor_add_behavior(editor, &rule, NULL))
        return fail("content behavior add failed");
    }
  }

  stygian_editor_trigger_event(editor, headers[1], STYGIAN_EDITOR_EVENT_PRESS);
  for (uint32_t i = 0u; i < 3u; ++i) {
    float content_opacity = 0.0f;
    float opacity = 0.0f;
    if (!get_float_prop(editor, content_nodes[i], STYGIAN_EDITOR_PROP_OPACITY,
                        &content_opacity))
      return fail("content opacity property read failed");
    if (!get_float_prop(editor, headers[i], STYGIAN_EDITOR_PROP_OPACITY, &opacity))
      return fail("header opacity property read failed");
    if ((i == 1u && content_opacity < 0.99f) ||
        (i != 1u && content_opacity > 0.20f))
      return fail("tab content opacity switching failed");
    if ((i == 1u && opacity < 0.99f) || (i != 1u && opacity > 0.70f))
      return fail("tab header opacity switching failed");
  }

  need = stygian_editor_build_c23(editor, NULL, 0u);
  if (need < 2u || need > sizeof(code))
    return fail("build c23 size failed");
  if (stygian_editor_build_c23(editor, code, sizeof(code)) != need)
    return fail("build c23 failed");
  if (!strstr(code, "stygian_editor_generated_dispatch_event"))
    return fail("behavior dispatch export missing");

  stygian_editor_destroy(editor);
  printf("[PASS] editor tabs behavior smoke\n");
  return 0;
}
