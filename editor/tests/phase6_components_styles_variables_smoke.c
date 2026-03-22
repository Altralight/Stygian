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
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorComponentDefDesc def_desc = {0};
  StygianEditorComponentInstanceDesc inst_desc = {0};
  StygianEditorTextDesc text_desc = {0};
  StygianEditorRectDesc rect_desc = {0};
  StygianEditorFrameDesc frame_desc = {0};
  StygianEditorNodeId def_id = 0u;
  StygianEditorNodeId inst_id = 0u;
  StygianEditorNodeId text_id = 0u;
  StygianEditorNodeId rect_id = 0u;
  StygianEditorNodeId frame_id = 0u;
  StygianEditorComponentOverrideState ov = {0};
  StygianEditorComponentPropertyDef prop = {0};
  StygianEditorTextStyleDef text_style = {0};
  StygianEditorEffectStyleDef effect_style = {0};
  StygianEditorLayoutStyleDef layout_style = {0};
  StygianEditorColor color = {0};
  char json[262144];
  char code[524288];
  size_t need = 0u;
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 1024u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 64u;

  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  def_desc.x = 0.0f;
  def_desc.y = 0.0f;
  def_desc.w = 200.0f;
  def_desc.h = 50.0f;
  def_desc.symbol = "button";
  def_desc.visible = true;
  def_id = stygian_editor_add_component_def(a, &def_desc);
  if (!def_id)
    return fail("add component def failed");

  inst_desc.component_def_id = def_id;
  inst_desc.x = 10.0f;
  inst_desc.y = 20.0f;
  inst_desc.w = 0.0f;
  inst_desc.h = 0.0f;
  inst_desc.visible = true;
  inst_id = stygian_editor_add_component_instance(a, &inst_desc);
  if (!inst_id)
    return fail("add component instance failed");

  ov.mask = STYGIAN_EDITOR_COMPONENT_OVERRIDE_W |
            STYGIAN_EDITOR_COMPONENT_OVERRIDE_VISIBLE |
            STYGIAN_EDITOR_COMPONENT_OVERRIDE_STYLE_BINDING;
  ov.w = 260.0f;
  ov.visible = false;
  snprintf(ov.style_binding, sizeof(ov.style_binding), "brand.primary");
  if (!stygian_editor_component_instance_set_override_state(a, inst_id, &ov))
    return fail("set override state failed");

  if (!stygian_editor_node_get_bounds(a, inst_id, &x, &y, &w, &h))
    return fail("get instance bounds failed");
  if (w < 259.0f || w > 261.0f)
    return fail("override width not applied");

  if (!stygian_editor_node_set_size(a, def_id, 320.0f, 50.0f))
    return fail("resize component def failed");
  if (!stygian_editor_node_get_bounds(a, inst_id, &x, &y, &w, &h))
    return fail("get instance bounds after def resize failed");
  if (w < 259.0f || w > 261.0f)
    return fail("override width was not preserved");

  if (!stygian_editor_component_instance_reset_overrides(a, inst_id))
    return fail("reset overrides failed");
  if (!stygian_editor_node_get_bounds(a, inst_id, &x, &y, &w, &h))
    return fail("get instance bounds after reset failed");
  if (w < 319.0f || w > 321.0f)
    return fail("reset override did not restore definition width");

  if (!stygian_editor_component_def_set_variant(a, def_id, "size", "md"))
    return fail("set variant failed");
  snprintf(prop.name, sizeof(prop.name), "state");
  prop.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
  snprintf(prop.default_enum, sizeof(prop.default_enum), "idle");
  prop.enum_option_count = 2u;
  snprintf(prop.enum_options[0], sizeof(prop.enum_options[0]), "idle");
  snprintf(prop.enum_options[1], sizeof(prop.enum_options[1]), "pressed");
  if (!stygian_editor_component_def_set_property(a, def_id, &prop))
    return fail("set component property failed");
  if (stygian_editor_component_def_property_count(a, def_id) == 0u)
    return fail("component property count not updated");

  text_desc.x = 0.0f;
  text_desc.y = 0.0f;
  text_desc.w = 100.0f;
  text_desc.h = 40.0f;
  text_desc.font_size = 12.0f;
  text_desc.fill = stygian_editor_color_rgba(1, 1, 1, 1);
  text_desc.text = "Label";
  text_desc.visible = true;
  text_id = stygian_editor_add_text(a, &text_desc);
  if (!text_id)
    return fail("add text failed");

  snprintf(text_style.name, sizeof(text_style.name), "Body");
  text_style.font_size = 16.0f;
  text_style.line_height = 20.0f;
  text_style.letter_spacing = 0.5f;
  text_style.font_weight = 500u;
  text_style.color = stygian_editor_color_rgba(0.2f, 0.6f, 0.9f, 1.0f);
  if (!stygian_editor_set_text_style(a, &text_style) ||
      !stygian_editor_apply_text_style(a, text_id, "Body")) {
    return fail("text style apply failed");
  }

  rect_desc.x = 0.0f;
  rect_desc.y = 0.0f;
  rect_desc.w = 80.0f;
  rect_desc.h = 80.0f;
  rect_desc.fill = stygian_editor_color_rgba(0.4f, 0.4f, 0.4f, 1.0f);
  rect_desc.visible = true;
  rect_id = stygian_editor_add_rect(a, &rect_desc);
  if (!rect_id)
    return fail("add rect failed");

  snprintf(effect_style.name, sizeof(effect_style.name), "Card");
  effect_style.effect_count = 1u;
  effect_style.effects[0].kind = STYGIAN_EDITOR_EFFECT_DROP_SHADOW;
  effect_style.effects[0].visible = true;
  effect_style.effects[0].opacity = 0.7f;
  effect_style.effects[0].radius = 6.0f;
  effect_style.effects[0].color = stygian_editor_color_rgba(0, 0, 0, 1);
  if (!stygian_editor_set_effect_style(a, &effect_style) ||
      !stygian_editor_apply_effect_style(a, rect_id, "Card")) {
    return fail("effect style apply failed");
  }

  frame_desc.x = 0.0f;
  frame_desc.y = 0.0f;
  frame_desc.w = 320.0f;
  frame_desc.h = 200.0f;
  frame_desc.fill = stygian_editor_color_rgba(0.1f, 0.1f, 0.1f, 1.0f);
  frame_desc.visible = true;
  frame_id = stygian_editor_add_frame(a, &frame_desc);
  if (!frame_id)
    return fail("add frame failed");

  snprintf(layout_style.name, sizeof(layout_style.name), "VStack");
  layout_style.layout.mode = STYGIAN_EDITOR_AUTO_LAYOUT_VERTICAL;
  layout_style.layout.padding_left = 8.0f;
  layout_style.layout.padding_right = 8.0f;
  layout_style.layout.padding_top = 8.0f;
  layout_style.layout.padding_bottom = 8.0f;
  layout_style.layout.gap = 6.0f;
  layout_style.layout.primary_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  layout_style.layout.cross_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_STRETCH;
  if (!stygian_editor_set_layout_style(a, &layout_style) ||
      !stygian_editor_apply_layout_style(a, frame_id, "VStack")) {
    return fail("layout style apply failed");
  }

  if (stygian_editor_style_usage_count(a, STYGIAN_EDITOR_STYLE_TEXT, "Body") == 0u)
    return fail("text style usage count failed");

  if (!stygian_editor_set_variable_mode(a, 0u, "light") ||
      !stygian_editor_set_variable_mode(a, 1u, "dark")) {
    return fail("set variable modes failed");
  }
  if (!stygian_editor_set_color_variable(a, "theme.surface", 0u,
                                         stygian_editor_color_rgba(0.92f, 0.92f, 0.92f, 1.0f)) ||
      !stygian_editor_set_color_variable(a, "theme.surface", 1u,
                                         stygian_editor_color_rgba(0.16f, 0.16f, 0.18f, 1.0f)) ||
      !stygian_editor_bind_node_color_variable(a, rect_id, "theme.surface")) {
    return fail("color variable setup failed");
  }
  if (!stygian_editor_set_number_variable(a, "anim.opacity", 0u, 0.3f) ||
      !stygian_editor_set_number_variable(a, "anim.opacity", 1u, 0.8f) ||
      !stygian_editor_bind_node_number_variable(a, rect_id,
                                                STYGIAN_EDITOR_PROP_OPACITY,
                                                "anim.opacity")) {
    return fail("number variable setup failed");
  }

  if (!stygian_editor_set_active_variable_mode(a, 1u) ||
      !stygian_editor_apply_active_variable_mode(a)) {
    return fail("apply active variable mode failed");
  }
  if (!stygian_editor_node_get_color(a, rect_id, &color))
    return fail("get rect color failed");
  if (color.r > 0.3f)
    return fail("dark mode color variable not applied");

  need = stygian_editor_build_project_json(a, NULL, 0u);
  if (need == 0u || need > sizeof(json))
    return fail("build project json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need)
    return fail("build project json failed");
  if (!stygian_editor_load_project_json(b, json))
    return fail("reload project json failed");
  if (stygian_editor_style_usage_count(b, STYGIAN_EDITOR_STYLE_TEXT, "Body") == 0u)
    return fail("style usage did not round-trip");

  need = stygian_editor_build_c23(a, NULL, 0u);
  if (need == 0u || need > sizeof(code))
    return fail("build c23 size failed");
  if (stygian_editor_build_c23(a, code, sizeof(code)) != need)
    return fail("build c23 failed");
  if (!strstr(code, "kStygianEditorGeneratedColorTokens"))
    return fail("export missing color token table");
  if (!strstr(code, "kStygianEditorGeneratedVariables"))
    return fail("export missing variable table");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor phase6 components/styles/variables smoke\n");
  return 0;
}
