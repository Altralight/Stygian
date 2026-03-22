#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define MKDIR(path) mkdir(path, 0777)
#endif

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool path_exists(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;
  fclose(f);
  return true;
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

static bool read_text_file(const char *path, char *out, size_t out_cap) {
  FILE *f = NULL;
  size_t n = 0u;
  if (!path || !out || out_cap < 2u)
    return false;
  f = fopen(path, "rb");
  if (!f)
    return false;
  n = fread(out, 1u, out_cap - 1u, f);
  out[n] = '\0';
  fclose(f);
  return true;
}

static bool contains_text(const char *haystack, const char *needle) {
  if (!haystack || !needle || !needle[0])
    return false;
  return strstr(haystack, needle) != NULL;
}

static bool mkdir_if_missing(const char *path) {
  if (!path || !path[0])
    return false;
  if (MKDIR(path) == 0)
    return true;
  return true;
}

static bool export_generated_layout(const StygianEditor *editor,
                                    const char *out_root) {
  char generated_dir[512];
  char scene_h[512];
  char scene_c[512];
  char manifest[512];
  char hooks_h[512];
  char hooks_c[512];
  char code[262144];
  size_t code_need = 0u;
  FILE *f = NULL;

  if (!editor || !out_root || !out_root[0])
    return false;

  snprintf(generated_dir, sizeof(generated_dir), "%s/generated", out_root);
  snprintf(scene_h, sizeof(scene_h), "%s/stygian_editor_scene.generated.h",
           generated_dir);
  snprintf(scene_c, sizeof(scene_c), "%s/stygian_editor_scene.generated.c",
           generated_dir);
  snprintf(manifest, sizeof(manifest),
           "%s/stygian_editor_manifest.generated.json", generated_dir);
  snprintf(hooks_h, sizeof(hooks_h), "%s/stygian_editor_hooks.h", out_root);
  snprintf(hooks_c, sizeof(hooks_c), "%s/stygian_editor_hooks.c", out_root);

  if (!mkdir_if_missing(out_root) || !mkdir_if_missing(generated_dir))
    return false;

  code_need = stygian_editor_build_c23(editor, NULL, 0u);
  if (code_need == 0u || code_need > sizeof(code))
    return false;
  if (stygian_editor_build_c23(editor, code, sizeof(code)) != code_need)
    return false;

  if (!write_text_file(
          scene_h,
          "/* AUTO-GENERATED FILE. DO NOT EDIT. */\n"
          "/* Exporter-owned: regenerated on every export. */\n"
          "/* User logic file locations: ../stygian_editor_hooks.h and ../stygian_editor_hooks.c */\n"
          "#pragma once\n"
          "#include \"stygian.h\"\n"
          "#include <stdint.h>\n\n"
          "const void *stygian_editor_generated_behavior_rules(uint32_t *out_count);\n"
          "int stygian_editor_generated_node_index(uint32_t node_id);\n"
          "void stygian_editor_generated_build_scene(StygianContext *ctx,\n"
          "                                        StygianElement *out_elements,\n"
          "                                        uint32_t out_count);\n"
          "void stygian_editor_generated_draw_paths(StygianContext *ctx);\n")) {
    return false;
  }

  f = fopen(scene_c, "wb");
  if (!f)
    return false;
  fputs("/* AUTO-GENERATED FILE. DO NOT EDIT. */\n", f);
  fputs("/* Exporter-owned: regenerated on every export. */\n", f);
  fputs(
      "/* User logic file locations: ../stygian_editor_hooks.h and "
      "../stygian_editor_hooks.c */\n",
      f);
  fputs("#include \"stygian_editor_scene.generated.h\"\n\n", f);
  fputs(code, f);
  fclose(f);

  if (!write_text_file(
          manifest,
          "{\n"
          "  \"schema\": \"stygian-editor-generated-layout\",\n"
          "  \"version\": 1,\n"
          "  \"ownership\": {\n"
          "    \"generated\": [\n"
          "      \"generated/stygian_editor_scene.generated.h\",\n"
          "      \"generated/stygian_editor_scene.generated.c\",\n"
          "      \"generated/stygian_editor_manifest.generated.json\"\n"
          "    ],\n"
          "    \"hooks\": [\"stygian_editor_hooks.h\", \"stygian_editor_hooks.c\"],\n"
          "    \"rules\": \"generated files are exporter-owned; hooks are user-owned and never overwritten\"\n"
          "  }\n"
          "}\n")) {
    return false;
  }

  if (!path_exists(hooks_h)) {
    if (!write_text_file(hooks_h,
                         "/* HANDWRITTEN FILE. EDIT FREELY. */\n"
                         "/* User-owned: exporter may create this file once and never overwrites it. */\n"
                         "#pragma once\n"
                         "#include \"stygian.h\"\n\n"
                         "void stygian_editor_hooks_before_frame(StygianContext *ctx);\n"
                         "void stygian_editor_hooks_after_frame(StygianContext *ctx);\n")) {
      return false;
    }
  }

  if (!path_exists(hooks_c)) {
    if (!write_text_file(hooks_c,
                         "/* HANDWRITTEN FILE. EDIT FREELY. */\n"
                         "/* User-owned: exporter may create this file once and never overwrites it. */\n"
                         "#include \"stygian_editor_hooks.h\"\n\n"
                         "void stygian_editor_hooks_before_frame(StygianContext *ctx) {\n"
                         "  (void)ctx;\n"
                         "}\n\n"
                         "void stygian_editor_hooks_after_frame(StygianContext *ctx) {\n"
                         "  (void)ctx;\n"
                         "}\n")) {
      return false;
    }
  }

  return true;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorEllipseDesc ellipse = {0};
  const char *out_root = "editor/build/windows/generated_layout_smoke_v2";
  char hooks_c_path[512];
  char scene_h_path[512];
  char scene_h_text[4096];
  char scene_c_path[512];
  char scene_c_text[8192];
  char hooks_h_path[512];
  char hooks_h_text[2048];
  char hooks_before[2048];
  char hooks_after[2048];
  const char *sentinel =
      "/* custom hook content: should survive regeneration */\n";

  cfg.max_nodes = 64u;
  cfg.max_path_points = 512u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 32u;

  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("editor create failed");

  rect.x = 12.0f;
  rect.y = 24.0f;
  rect.w = 160.0f;
  rect.h = 80.0f;
  rect.fill = stygian_editor_color_rgba(0.1f, 0.2f, 0.7f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;
  if (stygian_editor_add_rect(editor, &rect) == STYGIAN_EDITOR_INVALID_ID)
    return fail("add rect failed");

  ellipse.x = 260.0f;
  ellipse.y = 80.0f;
  ellipse.w = 96.0f;
  ellipse.h = 96.0f;
  ellipse.fill = stygian_editor_color_rgba(0.8f, 0.3f, 0.1f, 0.9f);
  ellipse.visible = true;
  ellipse.z = 2.0f;
  if (stygian_editor_add_ellipse(editor, &ellipse) == STYGIAN_EDITOR_INVALID_ID)
    return fail("add ellipse failed");

  if (!export_generated_layout(editor, out_root))
    return fail("first export failed");

  snprintf(scene_h_path, sizeof(scene_h_path),
           "%s/generated/stygian_editor_scene.generated.h", out_root);
  if (!read_text_file(scene_h_path, scene_h_text, sizeof(scene_h_text)))
    return fail("failed reading generated scene header");
  if (!contains_text(scene_h_text, "Exporter-owned: regenerated on every export."))
    return fail("generated scene header missing ownership marker");
  if (!contains_text(scene_h_text, "../stygian_editor_hooks.h"))
    return fail("generated scene header missing hook path marker");

  snprintf(scene_c_path, sizeof(scene_c_path),
           "%s/generated/stygian_editor_scene.generated.c", out_root);
  if (!read_text_file(scene_c_path, scene_c_text, sizeof(scene_c_text)))
    return fail("failed reading generated scene source");
  if (!contains_text(scene_c_text,
                     "Exporter-owned generated zone: regenerated on every export.")) {
    return fail("generated scene source missing generated-zone marker");
  }
  if (!contains_text(scene_c_text,
                     "Hook zone lives in stygian_editor_hooks.h/.c and stays user-owned.")) {
    return fail("generated scene source missing hook-zone marker");
  }

  snprintf(hooks_h_path, sizeof(hooks_h_path), "%s/stygian_editor_hooks.h",
           out_root);
  if (!read_text_file(hooks_h_path, hooks_h_text, sizeof(hooks_h_text)))
    return fail("failed reading hooks header");
  if (!contains_text(hooks_h_text,
                     "User-owned: exporter may create this file once and never overwrites it.")) {
    return fail("hooks header missing user-owned preservation marker");
  }

  snprintf(hooks_c_path, sizeof(hooks_c_path), "%s/stygian_editor_hooks.c",
           out_root);
  if (!write_text_file(hooks_c_path, sentinel))
    return fail("failed to write custom hook sentinel");
  if (!read_text_file(hooks_c_path, hooks_before, sizeof(hooks_before)))
    return fail("failed reading hook file before second export");

  if (!export_generated_layout(editor, out_root))
    return fail("second export failed");
  if (!read_text_file(hooks_c_path, hooks_after, sizeof(hooks_after)))
    return fail("failed reading hook file after second export");
  if (strcmp(hooks_before, hooks_after) != 0)
    return fail("handwritten hooks were overwritten during regeneration");

  stygian_editor_destroy(editor);
  printf("[PASS] editor generated file layout smoke\n");
  return 0;
}
