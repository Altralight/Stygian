#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool contains_text(const char *haystack, const char *needle) {
  return haystack && needle && strstr(haystack, needle) != NULL;
}

static bool write_text_file(const char *path, const char *text) {
  FILE *f = NULL;
  if (!path || !text)
    return false;
  f = fopen(path, "wb");
  if (!f)
    return false;
  fwrite(text, 1u, strlen(text), f);
  fclose(f);
  return true;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorNodeId root_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId top_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId viewport_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId inspector_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId parent_check = STYGIAN_EDITOR_INVALID_ID;
  size_t need_a = 0u;
  size_t need_b = 0u;
  char *code_a = NULL;
  char *code_b = NULL;
  char line[128];

  cfg.max_nodes = 256u;
  cfg.max_path_points = 2048u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 32u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("create editor failed");

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 1440.0f;
  frame.h = 900.0f;
  frame.clip_content = false;
  frame.fill = stygian_editor_color_rgba(0.06f, 0.07f, 0.09f, 1.0f);
  frame.visible = true;
  frame.z = 0.0f;
  root_id = stygian_editor_add_frame(editor, &frame);
  if (!root_id)
    return fail("add root frame failed");
  (void)stygian_editor_node_set_name(editor, root_id, "editor_root");

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 1440.0f;
  frame.h = 58.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.20f, 0.20f, 0.22f, 1.0f);
  frame.z = 1.0f;
  top_id = stygian_editor_add_frame(editor, &frame);
  if (!top_id || !stygian_editor_reparent_node(editor, top_id, root_id, false))
    return fail("add top frame failed");
  (void)stygian_editor_node_set_name(editor, top_id, "top_chrome");
  if (!stygian_editor_node_set_constraints(editor, top_id,
                                           STYGIAN_EDITOR_CONSTRAINT_H_LEFT_RIGHT,
                                           STYGIAN_EDITOR_CONSTRAINT_V_TOP)) {
    return fail("set top constraints failed");
  }

  frame.x = 0.0f;
  frame.y = 58.0f;
  frame.w = 1088.0f;
  frame.h = 842.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.05f, 0.06f, 0.08f, 1.0f);
  frame.z = 1.0f;
  viewport_id = stygian_editor_add_frame(editor, &frame);
  if (!viewport_id ||
      !stygian_editor_reparent_node(editor, viewport_id, root_id, false)) {
    return fail("add viewport frame failed");
  }
  (void)stygian_editor_node_set_name(editor, viewport_id, "viewport_shell");
  if (!stygian_editor_node_set_constraints(
          editor, viewport_id, STYGIAN_EDITOR_CONSTRAINT_H_LEFT_RIGHT,
          STYGIAN_EDITOR_CONSTRAINT_V_TOP_BOTTOM)) {
    return fail("set viewport constraints failed");
  }

  frame.x = 1088.0f;
  frame.y = 58.0f;
  frame.w = 352.0f;
  frame.h = 842.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.10f, 0.12f, 0.15f, 1.0f);
  frame.z = 1.0f;
  inspector_id = stygian_editor_add_frame(editor, &frame);
  if (!inspector_id ||
      !stygian_editor_reparent_node(editor, inspector_id, root_id, false)) {
    return fail("add inspector frame failed");
  }
  (void)stygian_editor_node_set_name(editor, inspector_id, "inspector_shell");
  if (!stygian_editor_node_set_constraints(
          editor, inspector_id, STYGIAN_EDITOR_CONSTRAINT_H_RIGHT,
          STYGIAN_EDITOR_CONSTRAINT_V_TOP_BOTTOM)) {
    return fail("set inspector constraints failed");
  }

  rect.x = 10.0f;
  rect.y = 6.0f;
  rect.w = 54.0f;
  rect.h = 22.0f;
  rect.fill = stygian_editor_color_rgba(0.24f, 0.24f, 0.26f, 1.0f);
  rect.visible = true;
  rect.z = 2.0f;
  if (!stygian_editor_add_rect(editor, &rect))
    return fail("add top slot rect failed");

  if (!stygian_editor_node_get_parent(editor, top_id, &parent_check) ||
      parent_check != root_id)
    return fail("source parent check top failed");
  if (!stygian_editor_node_get_parent(editor, viewport_id, &parent_check) ||
      parent_check != root_id)
    return fail("source parent check viewport failed");
  if (!stygian_editor_node_get_parent(editor, inspector_id, &parent_check) ||
      parent_check != root_id)
    return fail("source parent check inspector failed");

  need_a = stygian_editor_build_c23(editor, NULL, 0u);
  if (need_a < 2u)
    return fail("build c23 size failed");
  code_a = (char *)malloc(need_a);
  if (!code_a)
    return fail("alloc code_a failed");
  if (stygian_editor_build_c23(editor, code_a, need_a) != need_a)
    return fail("build c23 failed");

  snprintf(line, sizeof(line), "  { %uu, %uu, %uu, %uu,", root_id, top_id,
           (uint32_t)STYGIAN_EDITOR_CONSTRAINT_H_LEFT_RIGHT,
           (uint32_t)STYGIAN_EDITOR_CONSTRAINT_V_TOP);
  if (!contains_text(code_a, line))
    return fail("top constraint record missing");
  snprintf(line, sizeof(line), "  { %uu, %uu, %uu, %uu,", root_id, viewport_id,
           (uint32_t)STYGIAN_EDITOR_CONSTRAINT_H_LEFT_RIGHT,
           (uint32_t)STYGIAN_EDITOR_CONSTRAINT_V_TOP_BOTTOM);
  if (!contains_text(code_a, line))
    return fail("viewport constraint record missing");
  snprintf(line, sizeof(line), "  { %uu, %uu, %uu, %uu,", root_id, inspector_id,
           (uint32_t)STYGIAN_EDITOR_CONSTRAINT_H_RIGHT,
           (uint32_t)STYGIAN_EDITOR_CONSTRAINT_V_TOP_BOTTOM);
  if (!contains_text(code_a, line))
    return fail("inspector constraint record missing");

  if (!write_text_file("editor/build/windows/stygian_editor_layout_shell.generated.c",
                       code_a)) {
    return fail("write generated shell artifact failed");
  }

  need_b = stygian_editor_build_c23(editor, NULL, 0u);
  if (need_b != need_a)
    return fail("determinism size mismatch");
  code_b = (char *)malloc(need_b);
  if (!code_b)
    return fail("alloc code_b failed");
  if (stygian_editor_build_c23(editor, code_b, need_b) != need_b)
    return fail("second build c23 failed");
  if (strcmp(code_a, code_b) != 0)
    return fail("determinism content mismatch");

  free(code_b);
  free(code_a);
  stygian_editor_destroy(editor);
  printf("[PASS] editor self-host layout shell smoke\n");
  return 0;
}
