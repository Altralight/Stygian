#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
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
  StygianEditorNodeFill fills[3];
  StygianEditorNodeFill out_fill = {0};
  char json[262144];
  char c23[524288];
  size_t need_json = 0u;
  uint32_t diag_count = 0u;
  bool has_error = false;
  StygianEditorExportDiagnostic diags[32];

  cfg.max_nodes = 128u;
  cfg.max_path_points = 2048u;
  cfg.max_behaviors = 32u;
  cfg.max_color_tokens = 16u;
  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  rect.x = 50.0f;
  rect.y = 70.0f;
  rect.w = 260.0f;
  rect.h = 180.0f;
  rect.fill = stygian_editor_color_rgba(0.4f, 0.4f, 0.6f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;
  rect_id = stygian_editor_add_rect(a, &rect);
  if (!rect_id)
    return fail("add rect failed");

  memset(fills, 0, sizeof(fills));
  fills[0].kind = STYGIAN_EDITOR_FILL_SOLID;
  fills[0].visible = true;
  fills[0].opacity = 1.0f;
  fills[0].solid = stygian_editor_color_rgba(0.2f, 0.7f, 1.0f, 1.0f);

  fills[1].kind = STYGIAN_EDITOR_FILL_LINEAR_GRADIENT;
  fills[1].visible = true;
  fills[1].opacity = 0.8f;
  fills[1].stops[0].position = 0.0f;
  fills[1].stops[0].color = stygian_editor_color_rgba(1.0f, 0.2f, 0.3f, 1.0f);
  fills[1].stops[1].position = 1.0f;
  fills[1].stops[1].color = stygian_editor_color_rgba(0.1f, 0.1f, 0.9f, 1.0f);
  fills[1].gradient_angle_deg = 35.0f;

  fills[2].kind = STYGIAN_EDITOR_FILL_IMAGE;
  fills[2].visible = true;
  fills[2].opacity = 1.0f;
  snprintf(fills[2].image_asset, sizeof(fills[2].image_asset), "%s",
           "checker_asset");

  if (!stygian_editor_node_set_fills(a, rect_id, fills, 3u))
    return fail("set fills failed");
  if (stygian_editor_node_fill_count(a, rect_id) != 3u)
    return fail("fill count mismatch after set");
  if (!stygian_editor_node_get_fill(a, rect_id, 1u, &out_fill))
    return fail("get fill failed");
  if (out_fill.kind != STYGIAN_EDITOR_FILL_LINEAR_GRADIENT)
    return fail("wrong fill kind at index 1");

  need_json = stygian_editor_build_project_json(a, NULL, 0u);
  if (need_json == 0u || need_json > sizeof(json))
    return fail("build json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need_json)
    return fail("build json failed");
  if (!strstr(json, "fills="))
    return fail("fills not persisted");

  if (!stygian_editor_load_project_json(b, json))
    return fail("reload json failed");
  if (stygian_editor_node_fill_count(b, rect_id) != 3u)
    return fail("fill count mismatch after reload");

  if (stygian_editor_build_c23_with_diagnostics(b, c23, sizeof(c23), diags, 32u,
                                                &diag_count, &has_error) == 0u) {
    return fail("build c23 failed");
  }
  if (has_error)
    return fail("build c23 reported error");
  if (!has_diag_feature(diags, diag_count, "fill_stack_partial_export"))
    return fail("missing partial export fill-stack warning");
  if (!has_diag_feature(diags, diag_count, "fill_stack_unsupported_kind"))
    return fail("missing unsupported fill kind warning");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor fill stack smoke\n");
  return 0;
}
