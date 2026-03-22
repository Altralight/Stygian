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

static bool nearf(float a, float b) {
  float d = a - b;
  if (d < 0.0f)
    d = -d;
  return d <= 0.0001f;
}

static bool has_diag_feature(const StygianEditorExportDiagnostic *diags,
                             uint32_t count, const char *feature) {
  uint32_t i;
  for (i = 0u; i < count; ++i) {
    if (strcmp(diags[i].feature, feature) == 0)
      return true;
  }
  return false;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorNodeId rect_id = 0u;
  StygianEditorNodeFill fill = {0};
  StygianEditorNodeEffect effect = {0};
  StygianEditorGradientTransform gx = {0};
  StygianEditorGradientTransform gx_out = {0};
  StygianEditorEffectTransform ex = {0};
  StygianEditorEffectTransform ex_out = {0};
  char json[262144];
  char c23[524288];
  size_t need_json = 0u;
  uint32_t diag_count = 0u;
  bool has_error = false;
  StygianEditorExportDiagnostic diags[32];

  cfg.max_nodes = 128u;
  cfg.max_path_points = 1024u;
  cfg.max_behaviors = 32u;
  cfg.max_color_tokens = 16u;
  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("create editor failed");

  rect.x = 100.0f;
  rect.y = 90.0f;
  rect.w = 280.0f;
  rect.h = 190.0f;
  rect.fill = stygian_editor_color_rgba(0.20f, 0.30f, 0.40f, 1.0f);
  rect.visible = true;
  rect_id = stygian_editor_add_rect(a, &rect);
  if (!rect_id)
    return fail("add rect failed");

  memset(&fill, 0, sizeof(fill));
  fill.kind = STYGIAN_EDITOR_FILL_LINEAR_GRADIENT;
  fill.visible = true;
  fill.opacity = 0.9f;
  fill.stops[0].position = 0.0f;
  fill.stops[0].color = stygian_editor_color_rgba(1.0f, 0.2f, 0.2f, 1.0f);
  fill.stops[1].position = 1.0f;
  fill.stops[1].color = stygian_editor_color_rgba(0.2f, 0.4f, 1.0f, 1.0f);
  fill.gradient_angle_deg = 20.0f;
  if (!stygian_editor_node_set_fills(a, rect_id, &fill, 1u))
    return fail("set gradient fill failed");

  gx.origin_x = 0.25f;
  gx.origin_y = 0.75f;
  gx.scale_x = 1.30f;
  gx.scale_y = 0.60f;
  gx.rotation_deg = 15.0f;
  if (!stygian_editor_node_set_fill_gradient_transform(a, rect_id, 0u, &gx))
    return fail("set fill gradient transform failed");
  if (!stygian_editor_node_get_fill_gradient_transform(a, rect_id, 0u, &gx_out))
    return fail("get fill gradient transform failed");
  if (!nearf(gx_out.origin_x, gx.origin_x) || !nearf(gx_out.origin_y, gx.origin_y) ||
      !nearf(gx_out.scale_x, gx.scale_x) || !nearf(gx_out.scale_y, gx.scale_y) ||
      !nearf(gx_out.rotation_deg, gx.rotation_deg)) {
    return fail("fill gradient transform mismatch");
  }

  memset(&effect, 0, sizeof(effect));
  effect.kind = STYGIAN_EDITOR_EFFECT_DROP_SHADOW;
  effect.visible = true;
  effect.opacity = 0.8f;
  effect.radius = 10.0f;
  effect.spread = 2.0f;
  effect.offset_x = 5.0f;
  effect.offset_y = 7.0f;
  effect.color = stygian_editor_color_rgba(0.0f, 0.0f, 0.0f, 1.0f);
  if (!stygian_editor_node_set_effects(a, rect_id, &effect, 1u))
    return fail("set effect failed");

  ex.scale_x = 1.20f;
  ex.scale_y = 0.75f;
  ex.rotation_deg = 35.0f;
  if (!stygian_editor_node_set_effect_transform(a, rect_id, 0u, &ex))
    return fail("set effect transform failed");
  if (!stygian_editor_node_get_effect_transform(a, rect_id, 0u, &ex_out))
    return fail("get effect transform failed");
  if (!nearf(ex_out.scale_x, ex.scale_x) || !nearf(ex_out.scale_y, ex.scale_y) ||
      !nearf(ex_out.rotation_deg, ex.rotation_deg)) {
    return fail("effect transform mismatch");
  }

  need_json = stygian_editor_build_project_json(a, NULL, 0u);
  if (need_json == 0u || need_json > sizeof(json))
    return fail("build json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need_json)
    return fail("build json failed");
  if (!contains(json, "fills=") || !contains(json, "effects="))
    return fail("missing fills/effects serialization");
  if (!contains(json, ",0.250000,0.750000,1.300000,0.600000,15.000000"))
    return fail("missing gradient transform serialization");
  if (!contains(json, ",1.200000,0.750000,35.000000"))
    return fail("missing effect transform serialization");

  if (!stygian_editor_load_project_json(b, json))
    return fail("reload failed");
  if (!stygian_editor_node_get_fill_gradient_transform(b, rect_id, 0u, &gx_out))
    return fail("reload fill transform read failed");
  if (!stygian_editor_node_get_effect_transform(b, rect_id, 0u, &ex_out))
    return fail("reload effect transform read failed");
  if (!nearf(gx_out.origin_x, gx.origin_x) || !nearf(gx_out.scale_y, gx.scale_y) ||
      !nearf(ex_out.scale_x, ex.scale_x) || !nearf(ex_out.rotation_deg, ex.rotation_deg)) {
    return fail("reload transform mismatch");
  }

  if (stygian_editor_build_c23_with_diagnostics(b, c23, sizeof(c23), diags, 32u,
                                                &diag_count, &has_error) == 0u) {
    return fail("build c23 failed");
  }
  if (has_error)
    return fail("c23 build reported error");
  if (!contains(c23, "gradient-xform"))
    return fail("missing gradient transform export marker");
  if (!contains(c23, "effect-xform"))
    return fail("missing effect transform export marker");
  if (!has_diag_feature(diags, diag_count, "gradient_transform_partial_export"))
    return fail("missing gradient transform partial-export diagnostic");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor gradient/effect parity smoke\n");
  return 0;
}
