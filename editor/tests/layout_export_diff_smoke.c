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

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorFrameAutoLayout layout = {0};
  StygianEditorNodeLayoutOptions opts = {0};
  StygianEditorNodeId frame_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId child_a = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId child_b = STYGIAN_EDITOR_INVALID_ID;
  char base[262144];
  char gap_edit[262144];
  char wrap_edit[262144];

  cfg.max_nodes = 128u;
  cfg.max_path_points = 1024u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 16u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("editor create failed");

  frame.visible = true;
  frame.w = 320.0f;
  frame.h = 160.0f;
  frame.fill = stygian_editor_color_rgba(0.1f, 0.12f, 0.18f, 1.0f);
  frame_id = stygian_editor_add_frame(editor, &frame);
  if (!frame_id)
    return fail("add frame failed");

  rect.visible = true;
  rect.w = 64.0f;
  rect.h = 32.0f;
  rect.fill = stygian_editor_color_rgba(0.8f, 0.8f, 0.8f, 1.0f);
  child_a = stygian_editor_add_rect(editor, &rect);
  rect.w = 84.0f;
  child_b = stygian_editor_add_rect(editor, &rect);
  if (!child_a || !child_b)
    return fail("add children failed");
  if (!stygian_editor_reparent_node(editor, child_a, frame_id, false) ||
      !stygian_editor_reparent_node(editor, child_b, frame_id, false)) {
    return fail("reparent failed");
  }

  opts.absolute_position = false;
  opts.sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  opts.sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  if (!stygian_editor_node_set_layout_options(editor, child_a, &opts) ||
      !stygian_editor_node_set_layout_options(editor, child_b, &opts)) {
    return fail("set child layout options failed");
  }

  layout.mode = STYGIAN_EDITOR_AUTO_LAYOUT_HORIZONTAL;
  layout.wrap = STYGIAN_EDITOR_AUTO_LAYOUT_NO_WRAP;
  layout.padding_left = 12.0f;
  layout.padding_right = 12.0f;
  layout.padding_top = 12.0f;
  layout.padding_bottom = 12.0f;
  layout.gap = 10.0f;
  layout.primary_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  layout.cross_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  if (!stygian_editor_frame_set_auto_layout(editor, frame_id, &layout))
    return fail("set auto layout failed");
  if (!build_code(editor, base, sizeof(base)))
    return fail("base export failed");

  layout.gap = 14.0f;
  if (!stygian_editor_frame_set_auto_layout(editor, frame_id, &layout))
    return fail("set gap edit failed");
  if (!build_code(editor, gap_edit, sizeof(gap_edit)))
    return fail("gap edit export failed");
  if (changed_line_count(base, gap_edit) > 24u)
    return fail("layout gap edit churned too much output");

  layout.wrap = STYGIAN_EDITOR_AUTO_LAYOUT_WRAP;
  if (!stygian_editor_frame_set_auto_layout(editor, frame_id, &layout))
    return fail("set wrap edit failed");
  if (!build_code(editor, wrap_edit, sizeof(wrap_edit)))
    return fail("wrap edit export failed");
  if (changed_line_count(gap_edit, wrap_edit) > 24u)
    return fail("layout wrap edit churned too much output");
  if (!strstr(wrap_edit, "kStygianEditorGeneratedAutoLayoutFrames"))
    return fail("missing auto-layout frame table");
  if (!strstr(wrap_edit, "{ 1u, 1u, 1u,"))
    return fail("wrap flag missing from exported layout frame record");

  stygian_editor_destroy(editor);
  printf("[PASS] editor layout export diff smoke\n");
  return 0;
}
