#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool contains(const char *text, const char *needle) {
  return text && needle && strstr(text, needle) != NULL;
}

static uint32_t count_occurs(const char *text, const char *needle) {
  uint32_t count = 0u;
  const char *at = text;
  size_t nlen = strlen(needle);
  if (!text || !needle || nlen == 0u)
    return 0u;
  while ((at = strstr(at, needle)) != NULL) {
    ++count;
    at += nlen;
  }
  return count;
}

static bool nearf(float a, float b) {
  float d = a - b;
  if (d < 0.0f)
    d = -d;
  return d <= 0.01f;
}

static bool has_diag_feature(const StygianEditorExportDiagnostic *diags,
                             uint32_t count, const char *feature) {
  uint32_t i;
  for (i = 0u; i < count; ++i) {
    if (strcmp(diags[i].feature, feature) == 0)
      return true;
  }
  return false;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorTextDesc text = {0};
  StygianEditorTextStyleSpan spans[3];
  StygianEditorNodeId text_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorExportDiagnostic diags[64];
  uint32_t diag_count = 0u;
  bool has_error = false;
  float mw_a = 0.0f, mh_a = 0.0f, mw_b = 0.0f, mh_b = 0.0f;
  size_t need_json = 0u;
  size_t need_c23 = 0u;
  static char json[262144];
  static char c23[524288];

  cfg.max_nodes = 128u;
  cfg.max_path_points = 1024u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 16u;
  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("create editor failed");

  text.x = 48.0f;
  text.y = 64.0f;
  text.w = 120.0f;
  text.h = 42.0f;
  text.font_size = 16.0f;
  text.line_height = 20.0f;
  text.letter_spacing = 0.0f;
  text.font_weight = 400u;
  text.box_mode = STYGIAN_EDITOR_TEXT_BOX_AREA;
  text.align_h = STYGIAN_EDITOR_TEXT_ALIGN_LEFT;
  text.align_v = STYGIAN_EDITOR_TEXT_ALIGN_TOP;
  text.auto_size = STYGIAN_EDITOR_TEXT_AUTOSIZE_NONE;
  text.fill = stygian_editor_color_rgba(0.90f, 0.90f, 0.95f, 1.0f);
  text.text = "ABCD\nEFGH";
  text.visible = true;
  text.z = 2.0f;
  text_id = stygian_editor_add_text(a, &text);
  if (text_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add text failed");

  memset(spans, 0, sizeof(spans));
  spans[0].start = 0u;
  spans[0].length = 2u;
  spans[0].font_size = 22.0f;
  spans[0].line_height = 26.0f;
  spans[0].letter_spacing = 1.0f;
  spans[0].weight = 700u;
  spans[0].color = stygian_editor_color_rgba(1.0f, 0.4f, 0.3f, 1.0f);
  spans[1].start = 2u;
  spans[1].length = 2u;
  spans[1].font_size = 14.0f;
  spans[1].line_height = 18.0f;
  spans[1].letter_spacing = 0.5f;
  spans[1].weight = 500u;
  spans[1].color = stygian_editor_color_rgba(0.3f, 0.9f, 0.5f, 1.0f);
  spans[2].start = 5u;
  spans[2].length = 4u;
  spans[2].font_size = 18.0f;
  spans[2].line_height = 22.0f;
  spans[2].letter_spacing = 0.0f;
  spans[2].weight = 400u;
  spans[2].color = stygian_editor_color_rgba(0.5f, 0.6f, 1.0f, 1.0f);
  if (!stygian_editor_node_set_text_spans(a, text_id, spans, 3u))
    return fail("set text spans failed");
  if (!stygian_editor_node_measure_text(a, text_id, &mw_a, &mh_a))
    return fail("measure text A failed");

  need_json = stygian_editor_build_project_json(a, NULL, 0u);
  if (need_json < 2u || need_json > sizeof(json))
    return fail("build json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need_json)
    return fail("build json failed");
  if (!stygian_editor_load_project_json(b, json))
    return fail("load json failed");
  if (!stygian_editor_node_measure_text(b, text_id, &mw_b, &mh_b))
    return fail("measure text B failed");
  if (!nearf(mw_a, mw_b) || !nearf(mh_a, mh_b))
    return fail("measure mismatch after roundtrip");

  need_c23 = stygian_editor_build_c23_with_diagnostics(
      b, c23, sizeof(c23), diags, 64u, &diag_count, &has_error);
  if (need_c23 < 2u || need_c23 > sizeof(c23) || has_error)
    return fail("build c23 with diagnostics failed");
  if (has_diag_feature(diags, diag_count, "text_span_partial_export"))
    return fail("legacy text span partial diagnostic still present");
  if (!contains(c23, "stygian_clip_push(ctx"))
    return fail("missing text area clip push in export");
  if (count_occurs(c23, "stygian_text(ctx, 0u, ") < 2u)
    return fail("mixed span export did not emit multiple text runs");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor text shaping/overflow parity smoke\n");
  return 0;
}
