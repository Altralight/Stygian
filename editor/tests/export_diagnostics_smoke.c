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
  StygianEditor *editor = NULL;
  StygianEditorPathDesc path = {0};
  StygianEditorPathId path_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId path_node_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorExportDiagnostic diags[8];
  uint32_t diag_count = 0u;
  bool has_error = false;
  size_t code_need = 0u;
  char code[131072];
  uint32_t i;
  bool saw_closed_path_warning = false;

  cfg.max_nodes = 128u;
  cfg.max_path_points = 1024u;
  cfg.max_behaviors = 128u;
  cfg.max_color_tokens = 32u;

  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("editor create failed");

  path.stroke = stygian_editor_color_rgba(0.9f, 0.7f, 0.2f, 1.0f);
  path.thickness = 2.0f;
  path.closed = true;
  path.visible = true;
  path.z = 1.0f;

  path_id = stygian_editor_path_begin(editor, &path);
  if (path_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("path begin failed");
  if (!stygian_editor_path_add_point(editor, path_id, 10.0f, 10.0f, false) ||
      !stygian_editor_path_add_point(editor, path_id, 80.0f, 20.0f, false) ||
      !stygian_editor_path_add_point(editor, path_id, 40.0f, 70.0f, false)) {
    return fail("path add point failed");
  }
  path_node_id = stygian_editor_path_commit(editor, path_id);
  if (path_node_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("path commit failed");

  memset(diags, 0, sizeof(diags));
  diag_count = stygian_editor_collect_export_diagnostics(editor, diags, 8u);
  if (diag_count == 0u)
    return fail("expected export diagnostics but got none");

  for (i = 0u; i < diag_count && i < 8u; ++i) {
    if (diags[i].severity == STYGIAN_EDITOR_DIAG_WARNING &&
        diags[i].node_id == path_node_id &&
        strcmp(diags[i].feature, "closed_path_fill") == 0) {
      saw_closed_path_warning = true;
      break;
    }
  }
  if (!saw_closed_path_warning)
    return fail("missing actionable closed_path_fill warning");

  code_need = stygian_editor_build_c23_with_diagnostics(
      editor, code, sizeof(code), diags, 8u, &diag_count, &has_error);
  if (code_need == 0u)
    return fail("build_c23_with_diagnostics failed unexpectedly");
  if (has_error)
    return fail("closed path warning should not be flagged as hard error");

  stygian_editor_destroy(editor);
  printf("[PASS] editor export diagnostics smoke\n");
  return 0;
}
