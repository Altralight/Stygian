#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool contains_text(const char *haystack, const char *needle) {
  return haystack && needle && strstr(haystack, needle) != NULL;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorFrameDesc root = {0};
  StygianEditorFrameDesc child = {0};
  StygianEditorNodeId root_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId child_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorExportDiagnostic diags[16];
  uint32_t diag_count = 0u;
  uint32_t i;
  bool saw_frame_fallback = false;
  char code_a[262144];
  char code_b[262144];
  size_t code_need = 0u;
  char json[262144];
  size_t json_need = 0u;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 2048u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 16u;

  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  root.x = 0.0f;
  root.y = 0.0f;
  root.w = 640.0f;
  root.h = 360.0f;
  root.clip_content = true;
  root.fill = stygian_editor_color_rgba(0.08f, 0.10f, 0.12f, 1.0f);
  root.visible = true;
  root.z = 0.0f;

  child.x = 40.0f;
  child.y = 50.0f;
  child.w = 320.0f;
  child.h = 200.0f;
  child.clip_content = false;
  child.fill = stygian_editor_color_rgba(0.18f, 0.22f, 0.26f, 0.95f);
  child.visible = true;
  child.z = 1.0f;

  root_id = stygian_editor_add_frame(a, &root);
  child_id = stygian_editor_add_frame(a, &child);
  if (root_id == STYGIAN_EDITOR_INVALID_ID || child_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add frame failed");
  if (!stygian_editor_reparent_node(a, child_id, root_id, false))
    return fail("frame reparent failed");

  memset(diags, 0, sizeof(diags));
  diag_count = stygian_editor_collect_export_diagnostics(a, diags, 16u);
  for (i = 0u; i < diag_count && i < 16u; ++i) {
    if (diags[i].severity == STYGIAN_EDITOR_DIAG_WARNING &&
        strcmp(diags[i].feature, "codegen_shape_fallback") == 0 &&
        (diags[i].node_id == root_id || diags[i].node_id == child_id)) {
      saw_frame_fallback = true;
      break;
    }
  }
  if (saw_frame_fallback)
    return fail("frame should not use generic codegen fallback");

  code_need = stygian_editor_build_c23(a, NULL, 0u);
  if (code_need == 0u || code_need > sizeof(code_a))
    return fail("code size failed");
  if (stygian_editor_build_c23(a, code_a, sizeof(code_a)) != code_need)
    return fail("build code A failed");
  if (!contains_text(code_a, "stygian_editor_generated_frame_records"))
    return fail("frame record export function missing");
  if (!contains_text(code_a, "frame_clip_content=true"))
    return fail("clip true marker missing from export");
  if (!contains_text(code_a, "frame_clip_content=false"))
    return fail("clip false marker missing from export");

  json_need = stygian_editor_build_project_json(a, NULL, 0u);
  if (json_need == 0u || json_need > sizeof(json))
    return fail("json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != json_need)
    return fail("build json failed");
  if (!stygian_editor_load_project_json(b, json))
    return fail("load json failed");

  code_need = stygian_editor_build_c23(b, NULL, 0u);
  if (code_need == 0u || code_need > sizeof(code_b))
    return fail("code B size failed");
  if (stygian_editor_build_c23(b, code_b, sizeof(code_b)) != code_need)
    return fail("build code B failed");
  if (strcmp(code_a, code_b) != 0)
    return fail("frame export changed after roundtrip");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor frame clip export smoke\n");
  return 0;
}
