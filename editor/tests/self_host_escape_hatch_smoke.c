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

static bool path_exists(const char *path) {
  FILE *f = NULL;
  if (!path)
    return false;
  f = fopen(path, "rb");
  if (!f)
    return false;
  fclose(f);
  return true;
}

static bool mkdir_if_missing(const char *path) {
  if (!path || !path[0])
    return false;
  if (MKDIR(path) == 0)
    return true;
  return true;
}

static bool export_shell_bundle(const StygianEditor *editor, const char *out_root) {
  char generated_h[512];
  char generated_c[512];
  char host_h[512];
  char host_c[512];
  char *code = NULL;
  size_t need = 0u;
  FILE *f = NULL;

  if (!editor || !out_root || !out_root[0])
    return false;
  if (!mkdir_if_missing(out_root))
    return false;

  snprintf(generated_h, sizeof(generated_h), "%s/stygian_editor_layout_shell.generated.h",
           out_root);
  snprintf(generated_c, sizeof(generated_c), "%s/stygian_editor_layout_shell.generated.c",
           out_root);
  snprintf(host_h, sizeof(host_h), "%s/stygian_editor_layout_shell_host.h", out_root);
  snprintf(host_c, sizeof(host_c), "%s/stygian_editor_layout_shell_host.c", out_root);

  need = stygian_editor_build_c23(editor, NULL, 0u);
  if (need < 2u)
    return false;
  code = (char *)malloc(need);
  if (!code)
    return false;
  if (stygian_editor_build_c23(editor, code, need) != need)
    return false;

  if (!write_text_file(
          generated_h,
          "/* AUTO-GENERATED FILE. DO NOT EDIT. */\n"
          "#pragma once\n"
          "#include \"stygian.h\"\n"
          "#include <stdint.h>\n\n"
          "void stygian_editor_generated_build_scene(StygianContext *ctx,\n"
          "                                        StygianElement *out_elements,\n"
          "                                        uint32_t out_count);\n"
          "void stygian_editor_generated_draw_paths(StygianContext *ctx);\n"
          "void stygian_editor_layout_shell_before_frame(StygianContext *ctx);\n"
          "void stygian_editor_layout_shell_after_frame(StygianContext *ctx);\n")) {
    free(code);
    return false;
  }

  f = fopen(generated_c, "wb");
  if (!f) {
    free(code);
    return false;
  }
  fputs("/* AUTO-GENERATED FILE. DO NOT EDIT. */\n", f);
  fputs("#include \"stygian_editor_layout_shell.generated.h\"\n\n", f);
  fputs(code, f);
  fclose(f);
  free(code);

  if (!path_exists(host_h)) {
    if (!write_text_file(
            host_h,
            "/* HANDWRITTEN FILE. EDIT FREELY. */\n"
            "#pragma once\n"
            "#include \"stygian.h\"\n\n"
            "void stygian_editor_layout_shell_before_frame(StygianContext *ctx);\n"
            "void stygian_editor_layout_shell_after_frame(StygianContext *ctx);\n")) {
      return false;
    }
  }
  if (!path_exists(host_c)) {
    if (!write_text_file(
            host_c,
            "/* HANDWRITTEN FILE. EDIT FREELY. */\n"
            "#include \"stygian_editor_layout_shell_host.h\"\n\n"
            "void stygian_editor_layout_shell_before_frame(StygianContext *ctx) {\n"
            "  (void)ctx;\n"
            "}\n\n"
            "void stygian_editor_layout_shell_after_frame(StygianContext *ctx) {\n"
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
  StygianEditorFrameDesc frame = {0};
  const char *out_root = "editor/build/windows/self_host_escape_hatch";
  char host_c_path[512];
  char before[2048];
  char after[2048];
  const char *sentinel = "/* handwritten escape hatch sentinel */\n";

  cfg.max_nodes = 64u;
  cfg.max_path_points = 1024u;
  cfg.max_behaviors = 32u;
  cfg.max_color_tokens = 16u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("create editor failed");

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 640.0f;
  frame.h = 360.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.06f, 0.07f, 0.09f, 1.0f);
  frame.visible = true;
  frame.z = 0.0f;
  if (!stygian_editor_add_frame(editor, &frame))
    return fail("add frame failed");

  if (!export_shell_bundle(editor, out_root))
    return fail("first export failed");

  snprintf(host_c_path, sizeof(host_c_path), "%s/stygian_editor_layout_shell_host.c",
           out_root);
  if (!write_text_file(host_c_path, sentinel))
    return fail("write sentinel failed");
  if (!read_text_file(host_c_path, before, sizeof(before)))
    return fail("read before failed");

  if (!export_shell_bundle(editor, out_root))
    return fail("second export failed");
  if (!read_text_file(host_c_path, after, sizeof(after)))
    return fail("read after failed");
  if (strcmp(before, after) != 0)
    return fail("handwritten host slice overwritten by regeneration");

  stygian_editor_destroy(editor);
  printf("[PASS] editor self-host escape hatch smoke\n");
  return 0;
}
