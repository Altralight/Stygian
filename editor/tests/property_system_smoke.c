#include "../include/stygian_editor.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool nearf(float a, float b) { return fabsf(a - b) <= 0.01f; }

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorEllipseDesc ellipse = {0};
  StygianEditorNodeId r = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId e = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorPropertyDescriptor desc;
  StygianEditorPropertyValue v = {0};
  bool mixed = false;
  uint32_t supported = 0u;
  uint32_t applied = 0u;

  cfg.max_nodes = 128u;
  cfg.max_path_points = 1024u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 16u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("editor create failed");

  rect.x = 10.0f;
  rect.y = 12.0f;
  rect.w = 40.0f;
  rect.h = 24.0f;
  rect.fill = stygian_editor_color_rgba(0.2f, 0.3f, 0.4f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;
  r = stygian_editor_add_rect(editor, &rect);
  if (r == STYGIAN_EDITOR_INVALID_ID)
    return fail("add rect failed");

  ellipse.x = 30.0f;
  ellipse.y = 20.0f;
  ellipse.w = 30.0f;
  ellipse.h = 30.0f;
  ellipse.fill = stygian_editor_color_rgba(0.7f, 0.1f, 0.2f, 1.0f);
  ellipse.visible = true;
  ellipse.z = 2.0f;
  e = stygian_editor_add_ellipse(editor, &ellipse);
  if (e == STYGIAN_EDITOR_INVALID_ID)
    return fail("add ellipse failed");

  if (!stygian_editor_property_get_descriptor(STYGIAN_EDITOR_PROP_OPACITY, &desc))
    return fail("descriptor get for opacity failed");
  if (desc.value_type != STYGIAN_EDITOR_VALUE_FLOAT || !desc.writable ||
      !desc.has_numeric_range || !nearf(desc.max_value, 1.0f)) {
    return fail("opacity descriptor mismatch");
  }
  if (!stygian_editor_property_get_descriptor(STYGIAN_EDITOR_PROP_SHAPE_KIND,
                                               &desc))
    return fail("descriptor get for shape kind failed");
  if (desc.writable)
    return fail("shape kind should be read-only");

  if (!stygian_editor_select_node(editor, r, false) ||
      !stygian_editor_select_node(editor, e, true)) {
    return fail("selection setup failed");
  }

  if (!stygian_editor_selection_get_property_value(
          editor, STYGIAN_EDITOR_PROP_OPACITY, &v, &mixed, &supported)) {
    return fail("selection get opacity failed");
  }
  if (supported != 2u || mixed || v.type != STYGIAN_EDITOR_VALUE_FLOAT ||
      !nearf(v.as.number, 1.0f)) {
    return fail("selection opacity baseline mismatch");
  }

  v.type = STYGIAN_EDITOR_VALUE_FLOAT;
  v.as.number = 0.4f;
  if (!stygian_editor_selection_set_property_value(
          editor, STYGIAN_EDITOR_PROP_OPACITY, &v, &applied)) {
    return fail("selection set opacity failed");
  }
  if (applied != 2u)
    return fail("selection opacity applied count mismatch");

  if (!stygian_editor_selection_get_property_value(
          editor, STYGIAN_EDITOR_PROP_OPACITY, &v, &mixed, &supported) ||
      mixed || supported != 2u || !nearf(v.as.number, 0.4f)) {
    return fail("selection opacity after set mismatch");
  }

  if (!stygian_editor_node_set_position(editor, r, 10.0f, 12.0f, false) ||
      !stygian_editor_node_set_position(editor, e, 55.0f, 20.0f, false)) {
    return fail("position setup for mixed check failed");
  }
  if (!stygian_editor_selection_get_property_value(editor, STYGIAN_EDITOR_PROP_X,
                                                   &v, &mixed, &supported) ||
      !mixed || supported != 2u) {
    return fail("selection mixed X should report mixed");
  }

  v.type = STYGIAN_EDITOR_VALUE_FLOAT;
  v.as.number = 8.0f;
  if (!stygian_editor_selection_set_property_value(
          editor, STYGIAN_EDITOR_PROP_RADIUS_TL, &v, &applied)) {
    return fail("selection set radius_tl failed");
  }
  if (applied != 1u)
    return fail("radius_tl should apply only to rect");

  v.type = STYGIAN_EDITOR_VALUE_FLOAT;
  v.as.number = 1.5f;
  if (stygian_editor_selection_set_property_value(editor, STYGIAN_EDITOR_PROP_OPACITY,
                                                  &v, &applied)) {
    return fail("invalid opacity should fail");
  }
  if (!stygian_editor_selection_get_property_value(
          editor, STYGIAN_EDITOR_PROP_OPACITY, &v, &mixed, &supported) ||
      !nearf(v.as.number, 0.4f)) {
    return fail("invalid opacity should not change state");
  }

  v.type = STYGIAN_EDITOR_VALUE_BOOL;
  v.as.boolean = false;
  if (!stygian_editor_selection_set_property_value(
          editor, STYGIAN_EDITOR_PROP_VISIBLE, &v, &applied)) {
    return fail("selection set visible failed");
  }
  if (applied != 2u)
    return fail("visible should apply to both selected nodes");
  if (!stygian_editor_undo(editor))
    return fail("undo after visible multi-set failed");
  if (!stygian_editor_selection_get_property_value(
          editor, STYGIAN_EDITOR_PROP_VISIBLE, &v, &mixed, &supported) ||
      v.type != STYGIAN_EDITOR_VALUE_BOOL || !v.as.boolean) {
    return fail("undo should restore visible state");
  }

  if (!stygian_editor_set_color_token(
          editor, "primary", stygian_editor_color_rgba(0.9f, 0.9f, 0.2f, 1.0f))) {
    return fail("set color token failed");
  }
  v.type = STYGIAN_EDITOR_VALUE_STRING;
  snprintf(v.as.string, sizeof(v.as.string), "primary");
  if (!stygian_editor_selection_set_property_value(
          editor, STYGIAN_EDITOR_PROP_COLOR_TOKEN, &v, &applied)) {
    return fail("selection set color token failed");
  }
  if (applied != 2u)
    return fail("color token should apply to both selected nodes");

  if (!stygian_editor_selection_get_property_value(
          editor, STYGIAN_EDITOR_PROP_SHAPE_KIND, &v, &mixed, &supported) ||
      !mixed || supported != 2u || v.type != STYGIAN_EDITOR_VALUE_ENUM) {
    return fail("shape kind mixed enum check failed");
  }
  if (stygian_editor_selection_set_property_value(editor,
                                                  STYGIAN_EDITOR_PROP_SHAPE_KIND,
                                                  &v, &applied)) {
    return fail("shape kind set should fail (read-only)");
  }

  stygian_editor_destroy(editor);
  printf("[PASS] editor property system smoke\n");
  return 0;
}
