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
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorPathDesc path_desc = {0};
  StygianEditorNodeId rect_id = 0u;
  StygianEditorNodeId path_id = 0u;
  StygianEditorNodeId bool_id = 0u;
  StygianEditorNodeId flat_id = 0u;
  StygianEditorNodeFill fills[1];
  StygianEditorNodeStroke strokes[1];
  StygianEditorNodeEffect effects[2];
  StygianEditorPoint point = {0};
  StygianEditorPoint point_out = {0};
  StygianEditorNodeId operands[2];
  char json[262144];
  char c23[524288];
  size_t need = 0u;
  uint32_t bool_count = 0u;
  StygianEditorNodeId bool_ops[8];
  StygianEditorBooleanOp bool_op = STYGIAN_EDITOR_BOOLEAN_UNION;
  StygianEditorNodeStroke stroke_out = {0};
  StygianEditorNodeEffect effect_out = {0};
  StygianEditorNodeId mask_id = 0u;
  StygianEditorMaskMode mask_mode = STYGIAN_EDITOR_MASK_ALPHA;
  bool mask_invert = false;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 8192u;
  cfg.max_behaviors = 32u;
  cfg.max_color_tokens = 32u;
  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  rect.x = 64.0f;
  rect.y = 72.0f;
  rect.w = 180.0f;
  rect.h = 120.0f;
  rect.fill = stygian_editor_color_rgba(0.2f, 0.3f, 0.5f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;
  rect_id = stygian_editor_add_rect(a, &rect);
  if (!rect_id)
    return fail("add rect failed");

  memset(fills, 0, sizeof(fills));
  fills[0].kind = STYGIAN_EDITOR_FILL_LINEAR_GRADIENT;
  fills[0].visible = true;
  fills[0].opacity = 1.0f;
  fills[0].stops[0].color = stygian_editor_color_rgba(1.0f, 0.1f, 0.2f, 1.0f);
  fills[0].stops[1].color = stygian_editor_color_rgba(0.1f, 0.8f, 1.0f, 1.0f);
  fills[0].gradient_angle_deg = 30.0f;
  if (!stygian_editor_node_set_fills(a, rect_id, fills, 1u))
    return fail("set linear gradient fill failed");

  memset(strokes, 0, sizeof(strokes));
  strokes[0].visible = true;
  strokes[0].opacity = 0.9f;
  strokes[0].thickness = 6.0f;
  strokes[0].color = stygian_editor_color_rgba(0.9f, 0.9f, 0.2f, 1.0f);
  strokes[0].cap = STYGIAN_EDITOR_STROKE_CAP_ROUND;
  strokes[0].join = STYGIAN_EDITOR_STROKE_JOIN_BEVEL;
  strokes[0].align = STYGIAN_EDITOR_STROKE_ALIGN_OUTSIDE;
  strokes[0].miter_limit = 7.0f;
  strokes[0].dash_count = 2u;
  strokes[0].dash_pattern[0] = 12.0f;
  strokes[0].dash_pattern[1] = 8.0f;
  if (!stygian_editor_node_set_strokes(a, rect_id, strokes, 1u))
    return fail("set strokes failed");
  if (!stygian_editor_node_get_stroke(a, rect_id, 0u, &stroke_out))
    return fail("get stroke failed");
  if (stroke_out.dash_count != 2u || stroke_out.cap != STYGIAN_EDITOR_STROKE_CAP_ROUND)
    return fail("stroke state mismatch");

  memset(effects, 0, sizeof(effects));
  effects[0].kind = STYGIAN_EDITOR_EFFECT_DROP_SHADOW;
  effects[0].visible = true;
  effects[0].opacity = 0.8f;
  effects[0].radius = 12.0f;
  effects[0].spread = 2.0f;
  effects[0].offset_x = 3.0f;
  effects[0].offset_y = 5.0f;
  effects[0].color = stygian_editor_color_rgba(0.0f, 0.0f, 0.0f, 1.0f);
  effects[1].kind = STYGIAN_EDITOR_EFFECT_LAYER_BLUR;
  effects[1].visible = true;
  effects[1].opacity = 1.0f;
  effects[1].radius = 4.0f;
  if (!stygian_editor_node_set_effects(a, rect_id, effects, 2u))
    return fail("set effects failed");
  if (!stygian_editor_node_get_effect(a, rect_id, 0u, &effect_out))
    return fail("get effect failed");
  if (effect_out.kind != STYGIAN_EDITOR_EFFECT_DROP_SHADOW)
    return fail("effect kind mismatch");

  path_desc.stroke = stygian_editor_color_rgba(0.4f, 1.0f, 0.6f, 1.0f);
  path_desc.thickness = 3.0f;
  path_desc.closed = false;
  path_desc.visible = true;
  path_desc.z = 2.0f;
  {
    StygianEditorPathId pb = stygian_editor_path_begin(a, &path_desc);
    if (!pb)
      return fail("path begin failed");
    if (!stygian_editor_path_add_point(a, pb, 80.0f, 220.0f, false) ||
        !stygian_editor_path_add_point(a, pb, 180.0f, 240.0f, false) ||
        !stygian_editor_path_add_point(a, pb, 140.0f, 320.0f, false)) {
      return fail("path add point failed");
    }
    path_id = stygian_editor_path_commit(a, pb);
  }
  if (!path_id)
    return fail("path commit failed");
  if (stygian_editor_path_point_count(a, path_id) != 3u)
    return fail("path count mismatch");

  point.x = 160.0f;
  point.y = 280.0f;
  point.in_x = 145.0f;
  point.in_y = 278.0f;
  point.out_x = 174.0f;
  point.out_y = 282.0f;
  point.has_in_tangent = true;
  point.has_out_tangent = true;
  point.kind = STYGIAN_EDITOR_PATH_POINT_SMOOTH;
  if (!stygian_editor_path_insert_point(a, path_id, 2u, &point))
    return fail("path insert failed");
  if (!stygian_editor_path_set_closed(a, path_id, true))
    return fail("path set closed failed");
  if (!stygian_editor_path_get_point(a, path_id, 2u, &point_out))
    return fail("path get inserted point failed");
  if (point_out.kind != STYGIAN_EDITOR_PATH_POINT_SMOOTH)
    return fail("path point kind mismatch");
  if (!stygian_editor_path_remove_point(a, path_id, 1u))
    return fail("path remove failed");
  if (stygian_editor_path_point_count(a, path_id) != 3u)
    return fail("path count mismatch after remove");

  if (!stygian_editor_node_set_mask(a, rect_id, path_id, STYGIAN_EDITOR_MASK_ALPHA,
                                    true)) {
    return fail("set mask failed");
  }
  if (!stygian_editor_node_get_mask(a, rect_id, &mask_id, &mask_mode, &mask_invert))
    return fail("get mask failed");
  if (mask_id != path_id || mask_mode != STYGIAN_EDITOR_MASK_ALPHA || !mask_invert)
    return fail("mask state mismatch");

  operands[0] = rect_id;
  operands[1] = path_id;
  bool_id = stygian_editor_boolean_compose(a, STYGIAN_EDITOR_BOOLEAN_UNION, operands,
                                           2u, true);
  if (!bool_id)
    return fail("boolean compose failed");
  if (!stygian_editor_node_get_boolean(a, bool_id, &bool_op, bool_ops, 8u, &bool_count))
    return fail("get boolean failed");
  if (bool_op != STYGIAN_EDITOR_BOOLEAN_UNION || bool_count != 2u)
    return fail("boolean state mismatch");
  if (!stygian_editor_boolean_flatten(a, bool_id, &flat_id))
    return fail("boolean flatten failed");
  if (!flat_id)
    return fail("flatten id missing");

  need = stygian_editor_build_project_json(a, NULL, 0u);
  if (need == 0u || need > sizeof(json))
    return fail("build json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need)
    return fail("build json failed");
  if (!contains(json, "strokes=") || !contains(json, "effects=") ||
      !contains(json, "mask=") || !contains(json, "boolop=")) {
    return fail("new fields missing from project json");
  }

  if (!stygian_editor_load_project_json(b, json))
    return fail("reload json failed");
  if (!stygian_editor_node_get_boolean(b, bool_id, &bool_op, bool_ops, 8u, &bool_count))
    return fail("reload boolean failed");
  if (bool_count != 2u)
    return fail("reload boolean operand count mismatch");
  if (!stygian_editor_node_get_mask(b, rect_id, &mask_id, &mask_mode, &mask_invert))
    return fail("reload mask failed");
  if (mask_id != path_id || !mask_invert)
    return fail("reload mask mismatch");
  if (stygian_editor_node_stroke_count(b, rect_id) != 1u)
    return fail("reload stroke count mismatch");
  if (stygian_editor_node_effect_count(b, rect_id) != 2u)
    return fail("reload effect count mismatch");

  if (stygian_editor_build_c23(b, c23, sizeof(c23)) == 0u)
    return fail("build c23 failed");
  if (!contains(c23, "stygian_set_gradient("))
    return fail("missing gradient export");
  if (!contains(c23, "stygian_set_shadow(") || !contains(c23, "stygian_set_blur("))
    return fail("missing effect export");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor paint-geometry Batch B smoke\n");
  return 0;
}
