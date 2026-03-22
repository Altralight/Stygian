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

static bool replace_first(const char *src, const char *needle,
                          const char *replacement, char *out, size_t out_cap) {
  const char *at = NULL;
  size_t prefix = 0u;
  size_t suffix = 0u;
  size_t needed = 0u;
  if (!src || !needle || !replacement || !out || out_cap < 2u)
    return false;
  at = strstr(src, needle);
  if (!at)
    return false;
  prefix = (size_t)(at - src);
  suffix = strlen(at + strlen(needle));
  needed = prefix + strlen(replacement) + suffix + 1u;
  if (needed > out_cap)
    return false;
  memcpy(out, src, prefix);
  memcpy(out + prefix, replacement, strlen(replacement));
  memcpy(out + prefix + strlen(replacement), at + strlen(needle), suffix + 1u);
  return true;
}

static bool replace_schema_minor_value(const char *src, uint32_t new_minor,
                                       char *out, size_t out_cap) {
  const char *needle = "\"schema_minor\":";
  const char *at = strstr(src, needle);
  const char *val = NULL;
  const char *end = NULL;
  char minor_text[32];
  size_t prefix = 0u;
  size_t suffix = 0u;
  size_t needed = 0u;
  if (!src || !out || out_cap < 2u)
    return false;
  if (!at)
    return false;
  val = at + strlen(needle);
  while (*val == ' ' || *val == '\t')
    ++val;
  end = val;
  while (*end >= '0' && *end <= '9')
    ++end;
  snprintf(minor_text, sizeof(minor_text), "%u", new_minor);
  prefix = (size_t)(val - src);
  suffix = strlen(end);
  needed = prefix + strlen(minor_text) + suffix + 1u;
  if (needed > out_cap)
    return false;
  memcpy(out, src, prefix);
  memcpy(out + prefix, minor_text, strlen(minor_text));
  memcpy(out + prefix + strlen(minor_text), end, suffix + 1u);
  return true;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorRectDesc rect = {0};
  size_t need = 0u;
  char *good = NULL;
  char bad[65536];
  uint32_t baseline_nodes = 0u;

  cfg.max_nodes = 128u;
  cfg.max_path_points = 1024u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 16u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("create editor failed");

  rect.w = 40.0f;
  rect.h = 20.0f;
  rect.visible = true;
  rect.fill = stygian_editor_color_rgba(1.0f, 1.0f, 1.0f, 1.0f);
  if (!stygian_editor_add_rect(editor, &rect))
    return fail("seed rect failed");
  baseline_nodes = stygian_editor_node_count(editor);

  need = stygian_editor_build_project_json(editor, NULL, 0u);
  if (need < 2u)
    return fail("build json size failed");
  good = (char *)malloc(need);
  if (!good)
    return fail("alloc good json failed");
  if (stygian_editor_build_project_json(editor, good, need) != need)
    return fail("build json failed");

  if (!stygian_editor_load_project_json(editor, good))
    return fail("good load unexpectedly failed");

  if (stygian_editor_load_project_json(editor, ""))
    return fail("empty string unexpectedly accepted");
  if (stygian_editor_load_project_json(editor, "{}"))
    return fail("empty object unexpectedly accepted");
  if (stygian_editor_load_project_json(
          editor,
          "{\"schema_name\":\"nope\",\"schema_major\":0,\"schema_minor\":0}")) {
    return fail("wrong schema unexpectedly accepted");
  }
  if (stygian_editor_load_project_json(
          editor,
          "{\"schema_name\":\"stygian-editor-project\",\"schema_major\":99,\"schema_minor\":0}")) {
    return fail("unsupported major unexpectedly accepted");
  }

  if (!replace_first(good, "\"schema_name\": \"stygian-editor-project\"",
                     "\"schema_name\": \"broken-schema\"", bad, sizeof(bad))) {
    return fail("replace schema failed");
  }
  if (stygian_editor_load_project_json(editor, bad))
    return fail("mutated schema unexpectedly accepted");

  if (!replace_first(good, "pid=0", "pid=9999", bad, sizeof(bad)))
    return fail("replace pid failed");
  if (stygian_editor_load_project_json(editor, bad))
    return fail("broken parent link unexpectedly accepted");

  if (!replace_schema_minor_value(
          good,
          STYGIAN_EDITOR_PROJECT_SCHEMA_MINOR > 0u
              ? (STYGIAN_EDITOR_PROJECT_SCHEMA_MINOR - 1u)
              : 0u,
          bad, sizeof(bad))) {
    return fail("replace schema minor down failed");
  }
  if (!stygian_editor_load_project_json(editor, bad))
    return fail("older schema minor should be backward compatible");

  if (!replace_schema_minor_value(good,
                                  STYGIAN_EDITOR_PROJECT_SCHEMA_MINOR + 1u, bad,
                                  sizeof(bad))) {
    return fail("replace schema minor up failed");
  }
  if (stygian_editor_load_project_json(editor, bad))
    return fail("newer schema minor unexpectedly accepted");

  if (stygian_editor_node_count(editor) != baseline_nodes)
    return fail("failed load mutated editor state");

  free(good);
  stygian_editor_destroy(editor);
  printf("[PASS] editor project hostile load smoke\n");
  return 0;
}
