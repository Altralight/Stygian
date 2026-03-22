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

static bool read_text_file(const char *path, char *out, size_t cap) {
  FILE *f = NULL;
  size_t n = 0u;
  if (!path || !out || cap < 2u)
    return false;
  f = fopen(path, "rb");
  if (!f)
    return false;
  n = fread(out, 1u, cap - 1u, f);
  fclose(f);
  out[n] = '\0';
  return true;
}

static bool extract_line(const char *text, const char *needle, char *out,
                         size_t out_cap) {
  const char *p = NULL;
  const char *end = NULL;
  size_t len = 0u;
  if (!text || !needle || !out || out_cap < 2u)
    return false;
  p = strstr(text, needle);
  if (!p)
    return false;
  end = strchr(p, '\n');
  if (!end)
    end = p + strlen(p);
  len = (size_t)(end - p);
  if (len + 2u > out_cap)
    return false;
  memcpy(out, p, len);
  out[len] = '\n';
  out[len + 1u] = '\0';
  return true;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorEllipseDesc ellipse = {0};
  size_t need = 0u;
  char *code = NULL;
  char signature[2048] = {0};
  char line[256];
  char golden[2048] = {0};

  cfg.max_nodes = 64u;
  cfg.max_path_points = 512u;
  cfg.max_behaviors = 32u;
  cfg.max_color_tokens = 16u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("create editor failed");

  rect.x = 12.0f;
  rect.y = 24.0f;
  rect.w = 160.0f;
  rect.h = 80.0f;
  rect.fill = stygian_editor_color_rgba(0.1f, 0.2f, 0.7f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;
  if (!stygian_editor_add_rect(editor, &rect))
    return fail("add rect failed");

  ellipse.x = 260.0f;
  ellipse.y = 80.0f;
  ellipse.w = 96.0f;
  ellipse.h = 96.0f;
  ellipse.fill = stygian_editor_color_rgba(0.8f, 0.3f, 0.1f, 0.9f);
  ellipse.visible = true;
  ellipse.z = 2.0f;
  if (!stygian_editor_add_ellipse(editor, &ellipse))
    return fail("add ellipse failed");

  need = stygian_editor_build_c23(editor, NULL, 0u);
  if (need < 2u)
    return fail("build c23 size failed");
  code = (char *)malloc(need);
  if (!code)
    return fail("alloc code failed");
  if (stygian_editor_build_c23(editor, code, need) != need)
    return fail("build c23 failed");

  if (!extract_line(code, "#define STYGIAN_EDITOR_GENERATED_NODE_COUNT", line,
                    sizeof(line)))
    return fail("extract node count line failed");
  strncat(signature, line, sizeof(signature) - strlen(signature) - 1u);
  if (!extract_line(code, "#define STYGIAN_EDITOR_GENERATED_BEHAVIOR_COUNT", line,
                    sizeof(line)))
    return fail("extract behavior count line failed");
  strncat(signature, line, sizeof(signature) - strlen(signature) - 1u);
  if (!extract_line(code, "case 1u: return 0;", line, sizeof(line)))
    return fail("extract node index line 1 failed");
  strncat(signature, line, sizeof(signature) - strlen(signature) - 1u);
  if (!extract_line(code, "case 2u: return 1;", line, sizeof(line)))
    return fail("extract node index line 2 failed");
  strncat(signature, line, sizeof(signature) - strlen(signature) - 1u);
  if (!extract_line(code,
                    "stygian_set_bounds(ctx, out_elements[0u], 12.000000f, "
                    "24.000000f, 160.000000f, 80.000000f);",
                    line, sizeof(line)))
    return fail("extract rect bounds line failed");
  strncat(signature, line, sizeof(signature) - strlen(signature) - 1u);
  if (!extract_line(code,
                    "stygian_set_bounds(ctx, out_elements[1u], 260.000000f, "
                    "80.000000f, 96.000000f, 96.000000f);",
                    line, sizeof(line)))
    return fail("extract ellipse bounds line failed");
  strncat(signature, line, sizeof(signature) - strlen(signature) - 1u);

  if (!read_text_file("editor/fixtures/export_c23_golden_signature.txt", golden,
                      sizeof(golden))) {
    return fail("read golden signature failed");
  }
  if (strcmp(signature, golden) != 0)
    return fail("golden signature mismatch");

  free(code);
  stygian_editor_destroy(editor);
  printf("[PASS] editor export c23 golden smoke\n");
  return 0;
}
