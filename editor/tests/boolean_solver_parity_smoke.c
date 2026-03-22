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

static bool nearly(float a, float b) {
  float d = a - b;
  if (d < 0.0f)
    d = -d;
  return d <= 0.01f;
}

static bool check_bounds(const StygianEditor *editor, StygianEditorNodeId id,
                         float x, float y, float w, float h) {
  float ax = 0.0f;
  float ay = 0.0f;
  float aw = 0.0f;
  float ah = 0.0f;
  if (!stygian_editor_node_get_bounds(editor, id, &ax, &ay, &aw, &ah))
    return false;
  return nearly(ax, x) && nearly(ay, y) && nearly(aw, w) && nearly(ah, h);
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorRectDesc r = {0};
  StygianEditorNodeId ida = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId idb = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId ids[2];
  StygianEditorNodeId bool_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId path_id = STYGIAN_EDITOR_INVALID_ID;
  uint32_t point_count = 0u;
  static char json[262144];
  static char c23[524288];
  StygianEditorExportDiagnostic diags[64];
  uint32_t diag_count = 0u;
  bool has_error = false;
  size_t need = 0u;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 4096u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 32u;
  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("create editor failed");

  r.visible = true;
  r.fill = stygian_editor_color_rgba(0.8f, 0.2f, 0.2f, 1.0f);
  r.x = 20.0f;
  r.y = 30.0f;
  r.w = 110.0f;
  r.h = 90.0f;
  ida = stygian_editor_add_rect(a, &r);
  if (ida == STYGIAN_EDITOR_INVALID_ID)
    return fail("add rect A failed");

  r.fill = stygian_editor_color_rgba(0.2f, 0.4f, 0.8f, 1.0f);
  r.x = 70.0f;
  r.y = 55.0f;
  r.w = 100.0f;
  r.h = 80.0f;
  idb = stygian_editor_add_rect(a, &r);
  if (idb == STYGIAN_EDITOR_INVALID_ID)
    return fail("add rect B failed");

  ids[0] = ida;
  ids[1] = idb;

  bool_id = stygian_editor_boolean_compose(a, STYGIAN_EDITOR_BOOLEAN_INTERSECT,
                                           ids, 2u, true);
  if (bool_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("intersect compose failed");
  if (!stygian_editor_boolean_flatten(a, bool_id, &path_id))
    return fail("intersect flatten failed");
  if (!check_bounds(a, path_id, 70.0f, 55.0f, 60.0f, 65.0f))
    return fail("intersect flatten bounds mismatch");
  point_count = stygian_editor_path_point_count(a, path_id);
  if (point_count != 4u)
    return fail("intersect flatten point count mismatch");

  bool_id = stygian_editor_boolean_compose(a, STYGIAN_EDITOR_BOOLEAN_SUBTRACT,
                                           ids, 2u, true);
  if (bool_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("subtract compose failed");
  if (!stygian_editor_boolean_flatten(a, bool_id, &path_id))
    return fail("subtract flatten failed");
  if (!check_bounds(a, path_id, 20.0f, 30.0f, 110.0f, 90.0f))
    return fail("subtract flatten bounds mismatch");
  point_count = stygian_editor_path_point_count(a, path_id);
  if (point_count <= 4u)
    return fail("subtract flatten should produce non-rect geometry");

  need = stygian_editor_build_project_json(a, NULL, 0u);
  if (need < 2u || need > sizeof(json))
    return fail("build json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need)
    return fail("build json failed");
  if (!stygian_editor_load_project_json(b, json))
    return fail("load json failed");

  bool_id = stygian_editor_boolean_compose(b, STYGIAN_EDITOR_BOOLEAN_EXCLUDE,
                                           ids, 2u, true);
  if (bool_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("exclude compose failed after reload");
  if (!stygian_editor_boolean_flatten(b, bool_id, &path_id))
    return fail("exclude flatten failed after reload");
  point_count = stygian_editor_path_point_count(b, path_id);
  if (point_count <= 4u)
    return fail("exclude flatten should keep solver polygon detail");

  need = stygian_editor_build_c23_with_diagnostics(
      b, c23, sizeof(c23), diags, 64u, &diag_count, &has_error);
  if (need < 2u || need > sizeof(c23) || has_error)
    return fail("build c23 with diagnostics failed");
  if (contains(c23, "boolean_export_fallback"))
    return fail("legacy boolean fallback marker still present in export");
  for (uint32_t i = 0u; i < diag_count; ++i) {
    if (strcmp(diags[i].feature, "boolean_export_fallback") == 0)
      return fail("legacy boolean_export_fallback diagnostic still emitted");
  }

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor boolean solver parity smoke\n");
  return 0;
}
