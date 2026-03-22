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
  StygianEditorNodeId id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId bad_ids[2];
  StygianEditorTransformCommand bad_cmd = {0};
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
  float rotation = 0.0f;
  uint32_t i;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 4096u;
  cfg.max_behaviors = 256u;
  cfg.max_color_tokens = 64u;

  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("editor create failed");

  rect.x = 20.0f;
  rect.y = 25.0f;
  rect.w = 80.0f;
  rect.h = 40.0f;
  rect.fill = stygian_editor_color_rgba(0.1f, 0.2f, 0.8f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;

  id = stygian_editor_add_rect(editor, &rect);
  if (id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add rect failed");
  if (stygian_editor_node_count(editor) != 1u)
    return fail("node count after add mismatch");

  if (!stygian_editor_undo(editor))
    return fail("undo add failed");
  if (stygian_editor_node_count(editor) != 0u)
    return fail("node count after undo add mismatch");
  if (!stygian_editor_redo(editor))
    return fail("redo add failed");
  if (!stygian_editor_node_get_bounds(editor, id, &x, &y, &w, &h))
    return fail("node missing after redo add");
  if (!nearf(x, 20.0f) || !nearf(y, 25.0f))
    return fail("redo add changed bounds");

  if (!stygian_editor_begin_transaction(editor))
    return fail("begin transaction failed");
  if (stygian_editor_begin_transaction(editor))
    return fail("nested transaction should fail");
  if (stygian_editor_undo(editor))
    return fail("undo should fail while transaction active");
  if (stygian_editor_redo(editor))
    return fail("redo should fail while transaction active");
  if (!stygian_editor_node_set_position(editor, id, 100.0f, 150.0f, false))
    return fail("position edit in transaction failed");
  if (!stygian_editor_node_set_rotation(editor, id, 35.0f))
    return fail("rotation edit in transaction failed");
  if (!stygian_editor_end_transaction(editor, true))
    return fail("commit transaction failed");

  if (!stygian_editor_node_get_bounds(editor, id, &x, &y, &w, &h))
    return fail("bounds after transaction missing");
  if (!nearf(x, 100.0f) || !nearf(y, 150.0f))
    return fail("bounds after transaction mismatch");
  if (!stygian_editor_node_get_rotation(editor, id, &rotation) ||
      !nearf(rotation, 35.0f)) {
    return fail("rotation after transaction mismatch");
  }

  if (!stygian_editor_undo(editor))
    return fail("undo transaction failed");
  if (!stygian_editor_node_get_bounds(editor, id, &x, &y, &w, &h))
    return fail("bounds after undo transaction missing");
  if (!nearf(x, 20.0f) || !nearf(y, 25.0f))
    return fail("undo transaction bounds mismatch");
  if (!stygian_editor_node_get_rotation(editor, id, &rotation) ||
      !nearf(rotation, 0.0f)) {
    return fail("undo transaction rotation mismatch");
  }

  if (!stygian_editor_redo(editor))
    return fail("redo transaction failed");
  if (!stygian_editor_node_get_bounds(editor, id, &x, &y, &w, &h))
    return fail("bounds after redo transaction missing");
  if (!nearf(x, 100.0f) || !nearf(y, 150.0f))
    return fail("redo transaction bounds mismatch");

  if (!stygian_editor_undo(editor))
    return fail("second undo transaction failed");
  if (!stygian_editor_begin_transaction(editor))
    return fail("begin cancel transaction failed");
  if (!stygian_editor_node_set_position(editor, id, 222.0f, 333.0f, false))
    return fail("cancel transaction move failed");
  if (!stygian_editor_end_transaction(editor, false))
    return fail("cancel transaction failed");
  if (!stygian_editor_node_get_bounds(editor, id, &x, &y, &w, &h))
    return fail("bounds after cancel missing");
  if (!nearf(x, 20.0f) || !nearf(y, 25.0f))
    return fail("cancel transaction did not roll back state");

  bad_ids[0] = id;
  bad_ids[1] = 999999u;
  bad_cmd.kind = STYGIAN_EDITOR_TRANSFORM_MOVE;
  bad_cmd.dx = 10.0f;
  bad_cmd.dy = 10.0f;
  if (stygian_editor_apply_transform(editor, bad_ids, 2u, &bad_cmd))
    return fail("transform with invalid id should fail");
  if (!stygian_editor_node_get_bounds(editor, id, &x, &y, &w, &h))
    return fail("bounds after failed transform missing");
  if (!nearf(x, 20.0f) || !nearf(y, 25.0f))
    return fail("failed transform should have rolled back");

  if (!stygian_editor_node_set_position(editor, id, 44.0f, 55.0f, false))
    return fail("post-undo move failed");
  if (stygian_editor_redo(editor))
    return fail("redo should be cleared after new mutation");

  // History cap is 128 entries; push well beyond and make sure undo depth is bounded.
  for (i = 0u; i < 160u; ++i) {
    if (!stygian_editor_node_set_position(editor, id, 300.0f + (float)i, 60.0f,
                                          false)) {
      return fail("history cap setup moves failed");
    }
  }
  for (i = 0u; i < 128u; ++i) {
    if (!stygian_editor_undo(editor))
      return fail("undo should succeed within history cap");
  }
  if (stygian_editor_undo(editor))
    return fail("undo should stop after history cap is exhausted");

  stygian_editor_destroy(editor);
  printf("[PASS] editor undo redo smoke\n");
  return 0;
}
