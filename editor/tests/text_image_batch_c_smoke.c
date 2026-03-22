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

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorTextDesc text_desc = {0};
  StygianEditorTextDesc plain_desc = {0};
  StygianEditorImageDesc image_desc = {0};
  StygianEditorRectDesc rect_desc = {0};
  StygianEditorNodeFill image_fill = {0};
  StygianEditorTextStyleSpan spans[2];
  StygianEditorNodeId text_id = 0u;
  StygianEditorNodeId plain_id = 0u;
  StygianEditorNodeId image_id = 0u;
  StygianEditorNodeId rect_id = 0u;
  StygianEditorExportDiagnostic diags[32];
  uint32_t diag_count = 0u;
  bool has_error = false;
  char text_out[96];
  char image_source[128];
  char json[262144];
  char c23[524288];
  size_t need = 0u;
  float mw = 0.0f, mh = 0.0f;
  uint32_t fit = 0u;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 4096u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 64u;
  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  text_desc.x = 40.0f;
  text_desc.y = 52.0f;
  text_desc.w = 220.0f;
  text_desc.h = 60.0f;
  text_desc.font_size = 20.0f;
  text_desc.line_height = 26.0f;
  text_desc.letter_spacing = 1.0f;
  text_desc.font_weight = 500u;
  text_desc.box_mode = STYGIAN_EDITOR_TEXT_BOX_AREA;
  text_desc.align_h = STYGIAN_EDITOR_TEXT_ALIGN_LEFT;
  text_desc.align_v = STYGIAN_EDITOR_TEXT_ALIGN_TOP;
  text_desc.auto_size = STYGIAN_EDITOR_TEXT_AUTOSIZE_BOTH;
  text_desc.fill = stygian_editor_color_rgba(0.95f, 0.95f, 0.98f, 1.0f);
  text_desc.text = "Batch C\nText";
  text_desc.visible = true;
  text_desc.z = 1.0f;
  text_id = stygian_editor_add_text(a, &text_desc);
  if (!text_id)
    return fail("add text failed");

  memset(spans, 0, sizeof(spans));
  spans[0].start = 0u;
  spans[0].length = 5u;
  spans[0].font_size = 22.0f;
  spans[0].line_height = 28.0f;
  spans[0].letter_spacing = 1.5f;
  spans[0].weight = 700u;
  spans[0].color = stygian_editor_color_rgba(1.0f, 0.6f, 0.2f, 1.0f);
  spans[1].start = 6u;
  spans[1].length = 4u;
  spans[1].font_size = 18.0f;
  spans[1].line_height = 24.0f;
  spans[1].letter_spacing = 0.5f;
  spans[1].weight = 400u;
  spans[1].color = stygian_editor_color_rgba(0.5f, 0.9f, 1.0f, 1.0f);
  if (!stygian_editor_node_set_text_spans(a, text_id, spans, 2u))
    return fail("set text spans failed");
  if (stygian_editor_node_text_span_count(a, text_id) != 2u)
    return fail("text span count mismatch");
  if (!stygian_editor_node_measure_text(a, text_id, &mw, &mh))
    return fail("measure text failed");
  if (mw <= 0.0f || mh <= 0.0f)
    return fail("measure text invalid size");

  plain_desc.x = 40.0f;
  plain_desc.y = 140.0f;
  plain_desc.w = 220.0f;
  plain_desc.h = 80.0f;
  plain_desc.font_size = 18.0f;
  plain_desc.line_height = 24.0f;
  plain_desc.letter_spacing = 1.0f;
  plain_desc.font_weight = 700u;
  plain_desc.box_mode = STYGIAN_EDITOR_TEXT_BOX_AREA;
  plain_desc.align_h = STYGIAN_EDITOR_TEXT_ALIGN_CENTER;
  plain_desc.align_v = STYGIAN_EDITOR_TEXT_ALIGN_MIDDLE;
  plain_desc.auto_size = STYGIAN_EDITOR_TEXT_AUTOSIZE_NONE;
  plain_desc.fill = stygian_editor_color_rgba(0.98f, 0.98f, 1.0f, 1.0f);
  plain_desc.text = "Centered\nTitle";
  plain_desc.visible = true;
  plain_desc.z = 1.2f;
  plain_id = stygian_editor_add_text(a, &plain_desc);
  if (!plain_id)
    return fail("add plain text failed");

  image_desc.x = 320.0f;
  image_desc.y = 88.0f;
  image_desc.w = 180.0f;
  image_desc.h = 100.0f;
  image_desc.fit_mode = 1u;
  image_desc.source = "assets/ui/hero.png";
  image_desc.visible = true;
  image_desc.z = 1.5f;
  image_id = stygian_editor_add_image(a, &image_desc);
  if (!image_id)
    return fail("add image failed");
  if (!stygian_editor_node_set_image_fit_mode(a, image_id, 2u))
    return fail("set image fit failed");
  if (!stygian_editor_node_get_image_fit_mode(a, image_id, &fit) || fit != 2u)
    return fail("get image fit failed");

  rect_desc.x = 320.0f;
  rect_desc.y = 220.0f;
  rect_desc.w = 180.0f;
  rect_desc.h = 100.0f;
  rect_desc.fill = stygian_editor_color_rgba(0.3f, 0.3f, 0.4f, 1.0f);
  rect_desc.visible = true;
  rect_desc.z = 1.0f;
  rect_id = stygian_editor_add_rect(a, &rect_desc);
  if (!rect_id)
    return fail("add rect failed");
  memset(&image_fill, 0, sizeof(image_fill));
  image_fill.kind = STYGIAN_EDITOR_FILL_IMAGE;
  image_fill.visible = true;
  image_fill.opacity = 1.0f;
  snprintf(image_fill.image_asset, sizeof(image_fill.image_asset), "%s",
           "assets/ui/pattern.png");
  if (!stygian_editor_node_set_fills(a, rect_id, &image_fill, 1u))
    return fail("set image fill failed");

  need = stygian_editor_build_project_json(a, NULL, 0u);
  if (need == 0u || need > sizeof(json))
    return fail("build json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need)
    return fail("build json failed");
  if (!contains(json, "tspans="))
    return fail("spans not persisted");
  if (!contains(json, "src="))
    return fail("image source not persisted");

  if (!stygian_editor_load_project_json(b, json))
    return fail("reload project failed");
  if (!stygian_editor_node_get_text_content(b, text_id, text_out, sizeof(text_out)))
    return fail("get text content failed");
  if (strcmp(text_out, "Batch C\nText") != 0)
    return fail("text content mismatch");
  if (stygian_editor_node_text_span_count(b, text_id) != 2u)
    return fail("reload text span count mismatch");
  if (!stygian_editor_node_get_image_source(b, image_id, image_source,
                                            sizeof(image_source)))
    return fail("get image source failed");
  if (strcmp(image_source, "assets/ui/hero.png") != 0)
    return fail("image source mismatch");

  if (stygian_editor_build_c23_with_diagnostics(b, c23, sizeof(c23), diags, 32u,
                                                &diag_count, &has_error) == 0u)
    return fail("build c23 failed");
  if (has_error)
    return fail("build c23 reported error");
  if (!contains(c23, "stygian_text(ctx, 0u,"))
    return fail("missing text emission");
  if (!contains(c23, "stygian_editor_generated_draw_text_block(ctx,"))
    return fail("missing text block helper emission");
  if (!contains(c23, "stygian_editor_generated_resolve_texture("))
    return fail("missing image resolver hook");
  if (!contains(c23, "stygian_editor_generated_draw_image(ctx, _tex,"))
    return fail("missing fitted image helper emission");
  if (!contains(c23, "image_fit=2u"))
    return fail("missing image fit export marker");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor text/image Batch C smoke\n");
  return 0;
}
