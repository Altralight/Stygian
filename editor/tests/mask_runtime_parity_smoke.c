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
  StygianEditorPathDesc path = {0};
  StygianEditorPathId path_id = 0u;
  StygianEditorNodeId mask_root = 0u;
  StygianEditorNodeId mask_mid = 0u;
  StygianEditorNodeId target = 0u;
  StygianEditorNodeId vector_target = 0u;
  StygianEditorNodeId path_node = 0u;
  static char json[262144];
  static char c23_a[524288];
  static char c23_b[524288];
  size_t need_json = 0u;
  size_t need_c23_a = 0u;
  size_t need_c23_b = 0u;
  bool has_error = false;
  uint32_t diag_count = 0u;
  StygianEditorExportDiagnostic diags[64];
  StygianEditorNodeId mask_id = 0u;
  StygianEditorMaskMode mode = STYGIAN_EDITOR_MASK_ALPHA;
  bool invert = false;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 2048u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 32u;
  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("create editor failed");

  rect.visible = true;
  rect.fill = stygian_editor_color_rgba(0.8f, 0.2f, 0.3f, 1.0f);
  rect.x = 100.0f;
  rect.y = 100.0f;
  rect.w = 260.0f;
  rect.h = 220.0f;
  mask_root = stygian_editor_add_rect(a, &rect);
  if (!mask_root)
    return fail("add mask_root failed");

  rect.fill = stygian_editor_color_rgba(0.3f, 0.8f, 0.4f, 1.0f);
  rect.x = 160.0f;
  rect.y = 130.0f;
  rect.w = 210.0f;
  rect.h = 160.0f;
  mask_mid = stygian_editor_add_rect(a, &rect);
  if (!mask_mid)
    return fail("add mask_mid failed");

  rect.fill = stygian_editor_color_rgba(0.2f, 0.4f, 0.9f, 1.0f);
  rect.x = 180.0f;
  rect.y = 150.0f;
  rect.w = 240.0f;
  rect.h = 180.0f;
  target = stygian_editor_add_rect(a, &rect);
  if (!target)
    return fail("add target failed");

  rect.fill = stygian_editor_color_rgba(1.0f, 0.8f, 0.2f, 1.0f);
  rect.x = 210.0f;
  rect.y = 200.0f;
  rect.w = 120.0f;
  rect.h = 90.0f;
  vector_target = stygian_editor_add_rect(a, &rect);
  if (!vector_target)
    return fail("add vector_target failed");

  if (!stygian_editor_node_set_mask(a, mask_mid, mask_root,
                                    STYGIAN_EDITOR_MASK_ALPHA, false)) {
    return fail("set mid->root mask failed");
  }
  if (!stygian_editor_node_set_mask(a, target, mask_mid, STYGIAN_EDITOR_MASK_ALPHA,
                                    false)) {
    return fail("set target->mid mask failed");
  }
  if (stygian_editor_node_set_mask(a, mask_root, target, STYGIAN_EDITOR_MASK_ALPHA,
                                   false)) {
    return fail("cycle mask accepted unexpectedly");
  }

  path.thickness = 3.0f;
  path.stroke = stygian_editor_color_rgba(0.9f, 0.9f, 0.9f, 1.0f);
  path.closed = false;
  path_id = stygian_editor_path_begin(a, &path);
  if (!path_id)
    return fail("path begin failed");
  if (!stygian_editor_path_add_point(a, path_id, 120.0f, 120.0f, false) ||
      !stygian_editor_path_add_point(a, path_id, 420.0f, 260.0f, false)) {
    return fail("path points failed");
  }
  path_node = stygian_editor_path_commit(a, path_id);
  if (path_node == STYGIAN_EDITOR_INVALID_ID)
    return fail("path commit failed");
  if (!stygian_editor_node_set_mask(a, path_node, target,
                                    STYGIAN_EDITOR_MASK_ALPHA, false)) {
    return fail("set path mask failed");
  }

  need_json = stygian_editor_build_project_json(a, NULL, 0u);
  if (need_json < 2u || need_json > sizeof(json))
    return fail("build json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need_json)
    return fail("build json failed");
  if (!contains(json, "mask="))
    return fail("mask serialization missing");

  if (!stygian_editor_load_project_json(b, json))
    return fail("reload json failed");
  if (!stygian_editor_node_get_mask(b, target, &mask_id, &mode, &invert))
    return fail("reload target mask failed");
  if (mask_id != mask_mid || mode != STYGIAN_EDITOR_MASK_ALPHA || invert)
    return fail("reload target mask mismatch");

  need_c23_a = stygian_editor_build_c23_with_diagnostics(
      b, NULL, 0u, diags, 64u, &diag_count, &has_error);
  if (need_c23_a < 2u || need_c23_a > sizeof(c23_a) || has_error)
    return fail("build c23 diagnostics failed");
  if (stygian_editor_build_c23_with_diagnostics(
          b, c23_a, sizeof(c23_a), diags, 64u, &diag_count, &has_error) !=
      need_c23_a) {
    return fail("build c23 diagnostics write failed");
  }
  if (has_diag_feature(diags, diag_count, "mask_export_fallback"))
    return fail("legacy mask fallback diagnostic still present");
  if (has_diag_feature(diags, diag_count, "mask_mode_partial_export"))
    return fail("mask mode partial diag unexpectedly present for alpha chain");

  if (!contains(c23_a, "kStygianEditorGeneratedMaskRecords"))
    return fail("mask records missing from export");
  if (!contains(c23_a, "stygian_editor_generated_mask_rect_for_owner"))
    return fail("mask rect helper missing from export");
  if (!contains(c23_a, "stygian_set_clip(ctx, out_elements"))
    return fail("element clip application missing from export");
  if (!contains(c23_a, "stygian_clip_push(ctx, _mx, _my, _mw, _mh)"))
    return fail("vector clip push missing from export");

  need_c23_b = stygian_editor_build_c23(b, NULL, 0u);
  if (need_c23_b < 2u || need_c23_b > sizeof(c23_b))
    return fail("build c23 deterministic pass failed");
  if (stygian_editor_build_c23(b, c23_b, sizeof(c23_b)) != need_c23_b)
    return fail("build c23 deterministic write failed");
  if (need_c23_a != need_c23_b ||
      memcmp(c23_a, c23_b, need_c23_a) != 0)
    return fail("c23 determinism mismatch");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor mask runtime parity smoke\n");
  return 0;
}
