#include "../include/stygian_editor.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool nearf(float a, float b) { return fabsf(a - b) <= 0.01f; }

static bool build_code(const StygianEditor *editor, char *dst, size_t cap) {
  size_t need = stygian_editor_build_c23(editor, NULL, 0u);
  if (need == 0u || need > cap)
    return false;
  return stygian_editor_build_c23(editor, dst, cap) == need;
}

static int count_substring(const char *haystack, const char *needle) {
  int count = 0;
  size_t needle_len = 0u;
  const char *cursor = NULL;
  if (!haystack || !needle || !needle[0])
    return 0;
  needle_len = strlen(needle);
  cursor = haystack;
  while ((cursor = strstr(cursor, needle)) != NULL) {
    ++count;
    cursor += needle_len;
  }
  return count;
}

static int fail_first_diff(const char *label, const char *a, const char *b) {
  size_t i = 0u;
  while (a[i] && b[i] && a[i] == b[i])
    ++i;
  fprintf(stderr, "[FAIL] %s at byte %zu\n", label, i);
  fprintf(stderr, "  a: %.80s\n", a + i);
  fprintf(stderr, "  b: %.80s\n", b + i);
  return 1;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor_a = NULL;
  StygianEditor *editor_b = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorLineDesc line = {0};
  StygianEditorTextDesc text = {0};
  StygianEditorNodeId rect_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId line_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId text_id = STYGIAN_EDITOR_INVALID_ID;
  float rotation = 0.0f;
  char code_a[131072];
  char code_b[131072];
  char json[131072];
  size_t json_need = 0u;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 2048u;
  cfg.max_behaviors = 128u;
  cfg.max_color_tokens = 32u;

  editor_a = stygian_editor_create(&cfg, NULL);
  editor_b = stygian_editor_create(&cfg, NULL);
  if (!editor_a || !editor_b)
    return fail("editor create failed");

  rect.x = 24.0f;
  rect.y = 32.0f;
  rect.w = 160.0f;
  rect.h = 56.0f;
  rect.radius[0] = 8.0f;
  rect.radius[1] = 8.0f;
  rect.radius[2] = 8.0f;
  rect.radius[3] = 8.0f;
  rect.fill = stygian_editor_color_rgba(0.2f, 0.4f, 0.8f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;
  rect_id = stygian_editor_add_rect(editor_a, &rect);
  if (rect_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add rect failed");

  line.x1 = 220.0f;
  line.y1 = 40.0f;
  line.x2 = 360.0f;
  line.y2 = 92.0f;
  line.thickness = 3.0f;
  line.stroke = stygian_editor_color_rgba(0.9f, 0.8f, 0.2f, 1.0f);
  line.visible = true;
  line.z = 2.0f;
  line_id = stygian_editor_add_line(editor_a, &line);
  if (line_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add line failed");

  text.x = 48.0f;
  text.y = 132.0f;
  text.w = 180.0f;
  text.h = 32.0f;
  text.font_size = 20.0f;
  text.line_height = 24.0f;
  text.letter_spacing = 0.0f;
  text.box_mode = STYGIAN_EDITOR_TEXT_BOX_POINT;
  text.align_h = STYGIAN_EDITOR_TEXT_ALIGN_LEFT;
  text.align_v = STYGIAN_EDITOR_TEXT_ALIGN_TOP;
  text.auto_size = STYGIAN_EDITOR_TEXT_AUTOSIZE_WIDTH;
  text.fill = stygian_editor_color_rgba(0.95f, 0.95f, 0.98f, 1.0f);
  text.text = "Rotate Me";
  text.visible = true;
  text.z = 3.0f;
  text_id = stygian_editor_add_text(editor_a, &text);
  if (text_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add text failed");

  if (!stygian_editor_node_set_rotation(editor_a, rect_id, 30.0f))
    return fail("set rect rotation failed");
  if (!stygian_editor_node_set_rotation(editor_a, line_id, -15.0f))
    return fail("set line rotation failed");
  if (!stygian_editor_node_set_rotation(editor_a, text_id, 12.0f))
    return fail("set text rotation failed");

  if (!build_code(editor_a, code_a, sizeof(code_a)))
    return fail("build code failed");
  if (count_substring(code_a, "stygian_editor_generated_apply_rotation(") < 2)
    return fail("retained rotation helper missing from export");
  if (count_substring(code_a, "stygian_editor_generated_push_rotation(") < 1)
    return fail("vector rotation helper missing from export");
  if (strstr(code_a, "Rotate Me") == NULL)
    return fail("text export missing");

  json_need = stygian_editor_build_project_json(editor_a, NULL, 0u);
  if (json_need == 0u || json_need > sizeof(json))
    return fail("project json size failed");
  if (stygian_editor_build_project_json(editor_a, json, sizeof(json)) != json_need)
    return fail("project json write failed");
  if (!stygian_editor_load_project_json(editor_b, json))
    return fail("project load failed");

  if (!stygian_editor_node_get_rotation(editor_b, rect_id, &rotation) ||
      !nearf(rotation, 30.0f)) {
    return fail("rect rotation lost after roundtrip");
  }
  if (!stygian_editor_node_get_rotation(editor_b, line_id, &rotation) ||
      !nearf(rotation, -15.0f)) {
    return fail("line rotation lost after roundtrip");
  }
  if (!stygian_editor_node_get_rotation(editor_b, text_id, &rotation) ||
      !nearf(rotation, 12.0f)) {
    return fail("text rotation lost after roundtrip");
  }

  if (!build_code(editor_b, code_b, sizeof(code_b)))
    return fail("roundtrip code build failed");
  if (strcmp(code_a, code_b) != 0)
    return fail_first_diff("roundtrip changed rotated export", code_a, code_b);

  stygian_editor_destroy(editor_a);
  stygian_editor_destroy(editor_b);
  printf("[PASS] editor rotation export smoke\n");
  return 0;
}
