#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool build_code(const StygianEditor *editor, char *dst, size_t cap) {
  size_t need = 0u;
  if (!editor || !dst || cap < 2u)
    return false;
  need = stygian_editor_build_c23(editor, NULL, 0u);
  if (need == 0u || need > cap)
    return false;
  return stygian_editor_build_c23(editor, dst, cap) == need;
}

static uint32_t count_remaining_lines(const char *p) {
  uint32_t count = 0u;
  if (!p || !p[0])
    return 0u;
  count = 1u;
  while (*p) {
    if (*p == '\n' && p[1] != '\0')
      count += 1u;
    p += 1;
  }
  return count;
}

static uint32_t changed_line_count(const char *a, const char *b) {
  uint32_t changed = 0u;
  const char *ea = NULL;
  const char *eb = NULL;
  size_t lena = 0u;
  size_t lenb = 0u;
  if (!a || !b)
    return 0u;
  while (*a && *b) {
    ea = strchr(a, '\n');
    eb = strchr(b, '\n');
    if (!ea)
      ea = a + strlen(a);
    if (!eb)
      eb = b + strlen(b);
    lena = (size_t)(ea - a);
    lenb = (size_t)(eb - b);
    if (lena != lenb || memcmp(a, b, lena) != 0)
      changed += 1u;
    a = (*ea == '\n') ? ea + 1 : ea;
    b = (*eb == '\n') ? eb + 1 : eb;
  }
  changed += count_remaining_lines(a);
  changed += count_remaining_lines(b);
  return changed;
}

static bool collect_node_markers(const char *code, char *out, size_t out_cap) {
  const char *line = code;
  const char *end = NULL;
  size_t len = 0u;
  size_t used = 0u;
  if (!code || !out || out_cap < 2u)
    return false;
  out[0] = '\0';
  while (*line) {
    end = strchr(line, '\n');
    if (!end)
      end = line + strlen(line);
    len = (size_t)(end - line);
    if (len >= 10u && memcmp(line, "  /* Node ", 10u) == 0u) {
      if (used + len + 2u > out_cap)
        return false;
      memcpy(out + used, line, len);
      used += len;
      out[used++] = '\n';
      out[used] = '\0';
    }
    line = (*end == '\n') ? end + 1 : end;
  }
  return true;
}

static uint32_t count_substring(const char *text, const char *needle) {
  const char *at = text;
  size_t needle_len = 0u;
  uint32_t count = 0u;
  if (!text || !needle || !needle[0])
    return 0u;
  needle_len = strlen(needle);
  while ((at = strstr(at, needle)) != NULL) {
    count += 1u;
    at += needle_len;
  }
  return count;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorEllipseDesc ellipse = {0};
  StygianEditorNodeId card_id = 0u;
  StygianEditorNodeId dot_id = 0u;
  StygianEditorNodeId btn_id = 0u;
  char code_base[196608];
  char code_tiny[196608];
  char code_rename[196608];
  char markers_base[4096];
  char markers_tiny[4096];
  char markers_rename[4096];
  uint32_t tiny_diff_lines = 0u;
  uint32_t rename_diff_lines = 0u;
  uint32_t symbols_base = 0u;
  uint32_t symbols_rename = 0u;

  cfg.max_nodes = 128u;
  cfg.max_path_points = 1024u;
  cfg.max_behaviors = 128u;
  cfg.max_color_tokens = 32u;

  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("editor create failed");

  rect.x = 20.0f;
  rect.y = 24.0f;
  rect.w = 220.0f;
  rect.h = 140.0f;
  rect.radius[0] = 12.0f;
  rect.radius[1] = 12.0f;
  rect.radius[2] = 12.0f;
  rect.radius[3] = 12.0f;
  rect.fill = stygian_editor_color_rgba(0.12f, 0.16f, 0.24f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;
  card_id = stygian_editor_add_rect(editor, &rect);
  if (card_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add card rect failed");
  if (!stygian_editor_node_set_name(editor, card_id, "card"))
    return fail("set card name failed");

  ellipse.x = 48.0f;
  ellipse.y = 64.0f;
  ellipse.w = 14.0f;
  ellipse.h = 14.0f;
  ellipse.fill = stygian_editor_color_rgba(0.95f, 0.42f, 0.18f, 1.0f);
  ellipse.visible = true;
  ellipse.z = 2.0f;
  dot_id = stygian_editor_add_ellipse(editor, &ellipse);
  if (dot_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add dot ellipse failed");
  if (!stygian_editor_node_set_name(editor, dot_id, "status_dot"))
    return fail("set dot name failed");

  rect.x = 96.0f;
  rect.y = 126.0f;
  rect.w = 112.0f;
  rect.h = 28.0f;
  rect.radius[0] = 8.0f;
  rect.radius[1] = 8.0f;
  rect.radius[2] = 8.0f;
  rect.radius[3] = 8.0f;
  rect.fill = stygian_editor_color_rgba(0.20f, 0.52f, 0.95f, 1.0f);
  rect.visible = true;
  rect.z = 3.0f;
  btn_id = stygian_editor_add_rect(editor, &rect);
  if (btn_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add button rect failed");
  if (!stygian_editor_node_set_name(editor, btn_id, "cta_button"))
    return fail("set button name failed");

  if (!build_code(editor, code_base, sizeof(code_base)))
    return fail("base export failed");
  if (!collect_node_markers(code_base, markers_base, sizeof(markers_base)))
    return fail("collect base node markers failed");

  if (!stygian_editor_node_set_position(editor, dot_id, 49.0f, 64.0f, false))
    return fail("tiny move edit failed");
  if (!build_code(editor, code_tiny, sizeof(code_tiny)))
    return fail("tiny edit export failed");
  if (!collect_node_markers(code_tiny, markers_tiny, sizeof(markers_tiny)))
    return fail("collect tiny-edit node markers failed");

  tiny_diff_lines = changed_line_count(code_base, code_tiny);
  if (tiny_diff_lines == 0u || tiny_diff_lines > 32u)
    return fail("tiny edit caused non-local textual diff");
  if (strcmp(markers_base, markers_tiny) != 0)
    return fail("node marker order changed after tiny edit");

  if (!stygian_editor_node_set_name(editor, btn_id, "cta_button_v2"))
    return fail("rename edit failed");
  if (!build_code(editor, code_rename, sizeof(code_rename)))
    return fail("rename edit export failed");
  if (!collect_node_markers(code_rename, markers_rename, sizeof(markers_rename)))
    return fail("collect rename node markers failed");

  rename_diff_lines = changed_line_count(code_tiny, code_rename);
  if (rename_diff_lines > 8u)
    return fail("rename-only edit churned too much output");
  if (strcmp(markers_tiny, markers_rename) != 0)
    return fail("node marker order changed after rename-only edit");

  symbols_base = count_substring(code_tiny, "stygian_editor_generated_build_scene");
  symbols_rename =
      count_substring(code_rename, "stygian_editor_generated_build_scene");
  if (symbols_base != symbols_rename)
    return fail("generated symbol footprint changed after rename-only edit");

  stygian_editor_destroy(editor);
  printf("[PASS] editor export diff quality smoke\n");
  return 0;
}
