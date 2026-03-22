#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool has_fallback_diag(const StygianEditorExportDiagnostic *diags,
                              uint32_t count) {
  uint32_t i;
  for (i = 0u; i < count; ++i) {
    if (strcmp(diags[i].feature, "codegen_shape_fallback") == 0)
      return true;
  }
  return false;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor_a = NULL;
  StygianEditor *editor_b = NULL;
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorEllipseDesc ellipse = {0};
  StygianEditorLineDesc line = {0};
  StygianEditorArrowDesc arrow = {0};
  StygianEditorPolygonDesc polygon = {0};
  StygianEditorStarDesc star = {0};
  StygianEditorTextDesc text = {0};
  StygianEditorImageDesc image = {0};
  StygianEditorPathDesc path_desc = {0};
  StygianEditorPathId draft = 0u;
  StygianEditorNodeId path_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorPoint point = {0};
  char project_json[262144];
  char c23[524288];
  size_t need_json = 0u;
  size_t need_c23 = 0u;
  StygianEditorExportDiagnostic diags[64];
  uint32_t diag_count = 0u;
  bool has_error = false;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 4096u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 32u;

  editor_a = stygian_editor_create(&cfg, NULL);
  editor_b = stygian_editor_create(&cfg, NULL);
  if (!editor_a || !editor_b)
    return fail("editor create failed");

  frame.x = 10.0f;
  frame.y = 10.0f;
  frame.w = 900.0f;
  frame.h = 580.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.08f, 0.10f, 0.14f, 1.0f);
  frame.visible = true;
  frame.z = 0.0f;
  if (!stygian_editor_add_frame(editor_a, &frame))
    return fail("add frame failed");

  rect.x = 24.0f;
  rect.y = 24.0f;
  rect.w = 220.0f;
  rect.h = 80.0f;
  rect.radius[0] = 14.0f;
  rect.radius[1] = 14.0f;
  rect.radius[2] = 14.0f;
  rect.radius[3] = 14.0f;
  rect.fill = stygian_editor_color_rgba(0.20f, 0.48f, 0.88f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;
  if (!stygian_editor_add_rect(editor_a, &rect))
    return fail("add rect failed");

  ellipse.x = 280.0f;
  ellipse.y = 24.0f;
  ellipse.w = 96.0f;
  ellipse.h = 96.0f;
  ellipse.fill = stygian_editor_color_rgba(0.95f, 0.45f, 0.24f, 0.92f);
  ellipse.visible = true;
  ellipse.z = 2.0f;
  if (!stygian_editor_add_ellipse(editor_a, &ellipse))
    return fail("add ellipse failed");

  line.x1 = 40.0f;
  line.y1 = 150.0f;
  line.x2 = 280.0f;
  line.y2 = 178.0f;
  line.thickness = 3.0f;
  line.stroke = stygian_editor_color_rgba(0.25f, 0.80f, 1.00f, 1.0f);
  line.visible = true;
  line.z = 3.0f;
  if (!stygian_editor_add_line(editor_a, &line))
    return fail("add line failed");

  arrow.x1 = 40.0f;
  arrow.y1 = 220.0f;
  arrow.x2 = 320.0f;
  arrow.y2 = 250.0f;
  arrow.thickness = 2.0f;
  arrow.head_size = 18.0f;
  arrow.stroke = stygian_editor_color_rgba(0.98f, 0.80f, 0.22f, 1.0f);
  arrow.visible = true;
  arrow.z = 4.0f;
  if (!stygian_editor_add_arrow(editor_a, &arrow))
    return fail("add arrow failed");

  polygon.x = 420.0f;
  polygon.y = 32.0f;
  polygon.w = 120.0f;
  polygon.h = 120.0f;
  polygon.sides = 6u;
  polygon.corner_radius = 0.0f;
  polygon.fill = stygian_editor_color_rgba(0.55f, 0.66f, 1.0f, 0.92f);
  polygon.visible = true;
  polygon.z = 5.0f;
  if (!stygian_editor_add_polygon(editor_a, &polygon))
    return fail("add polygon failed");

  star.x = 580.0f;
  star.y = 32.0f;
  star.w = 120.0f;
  star.h = 120.0f;
  star.points = 5u;
  star.inner_ratio = 0.42f;
  star.fill = stygian_editor_color_rgba(1.0f, 0.62f, 0.18f, 0.92f);
  star.visible = true;
  star.z = 6.0f;
  if (!stygian_editor_add_star(editor_a, &star))
    return fail("add star failed");

  text.x = 40.0f;
  text.y = 320.0f;
  text.w = 220.0f;
  text.h = 36.0f;
  text.font_size = 22.0f;
  text.line_height = 26.0f;
  text.letter_spacing = 0.0f;
  text.font_weight = 500u;
  text.box_mode = STYGIAN_EDITOR_TEXT_BOX_POINT;
  text.align_h = STYGIAN_EDITOR_TEXT_ALIGN_LEFT;
  text.align_v = STYGIAN_EDITOR_TEXT_ALIGN_TOP;
  text.auto_size = STYGIAN_EDITOR_TEXT_AUTOSIZE_WIDTH;
  text.fill = stygian_editor_color_rgba(0.96f, 0.96f, 0.98f, 1.0f);
  text.text = "Mockup text";
  text.visible = true;
  text.z = 7.0f;
  if (!stygian_editor_add_text(editor_a, &text))
    return fail("add text failed");

  image.x = 300.0f;
  image.y = 300.0f;
  image.w = 128.0f;
  image.h = 96.0f;
  image.fit_mode = 1u;
  image.source = "assets/mockup.png";
  image.visible = true;
  image.z = 8.0f;
  if (!stygian_editor_add_image(editor_a, &image))
    return fail("add image failed");

  path_desc.stroke = stygian_editor_color_rgba(0.78f, 0.22f, 0.98f, 1.0f);
  path_desc.thickness = 3.0f;
  path_desc.closed = false;
  path_desc.visible = true;
  path_desc.z = 9.0f;
  draft = stygian_editor_path_begin(editor_a, &path_desc);
  if (draft == 0u)
    return fail("path begin failed");
  if (!stygian_editor_path_add_point(editor_a, draft, 520.0f, 250.0f, false) ||
      !stygian_editor_path_add_point(editor_a, draft, 620.0f, 300.0f, false) ||
      !stygian_editor_path_add_point(editor_a, draft, 680.0f, 360.0f, false)) {
    return fail("path add point failed");
  }
  path_id = stygian_editor_path_commit(editor_a, draft);
  if (path_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("path commit failed");
  if (stygian_editor_path_point_count(editor_a, path_id) != 3u)
    return fail("path point count mismatch");

  point.in_x = 590.0f;
  point.in_y = 268.0f;
  point.out_x = 590.0f;
  point.out_y = 268.0f;
  point.kind = STYGIAN_EDITOR_PATH_POINT_SMOOTH;
  if (!stygian_editor_path_insert_point(editor_a, path_id, 1u, &point))
    return fail("path insert failed");
  if (!stygian_editor_path_remove_point(editor_a, path_id, 2u))
    return fail("path remove failed");
  if (!stygian_editor_path_set_closed(editor_a, path_id, true))
    return fail("path close failed");
  if (!stygian_editor_path_get_point(editor_a, path_id, 1u, &point))
    return fail("path point read failed");

  need_json = stygian_editor_build_project_json(editor_a, NULL, 0u);
  if (need_json == 0u || need_json > sizeof(project_json))
    return fail("project json size failed");
  if (stygian_editor_build_project_json(editor_a, project_json,
                                        sizeof(project_json)) != need_json) {
    return fail("project json build failed");
  }
  if (!strstr(project_json, "k=frame") || !strstr(project_json, "k=rect") ||
      !strstr(project_json, "k=ellipse") || !strstr(project_json, "k=line") ||
      !strstr(project_json, "k=arrow") || !strstr(project_json, "k=polygon") ||
      !strstr(project_json, "k=star") || !strstr(project_json, "k=path") ||
      !strstr(project_json, "k=text") || !strstr(project_json, "k=image")) {
    return fail("project json missing one or more primitive records");
  }

  if (!stygian_editor_load_project_json(editor_b, project_json))
    return fail("project json reload failed");
  if (stygian_editor_path_point_count(editor_b, path_id) != 3u)
    return fail("roundtrip path point count mismatch");

  need_c23 = stygian_editor_build_c23_with_diagnostics(
      editor_b, c23, sizeof(c23), diags, 64u, &diag_count, &has_error);
  if (need_c23 == 0u || has_error)
    return fail("c23 export failed");
  if (has_fallback_diag(diags, diag_count))
    return fail("fallback diagnostic present for mvp primitive export");
  if (!strstr(c23, "stygian_set_type(ctx, out_elements[1u], STYGIAN_RECT);") ||
      !strstr(c23,
              "stygian_set_type(ctx, out_elements[2u], STYGIAN_CIRCLE);") ||
      !strstr(c23, "/* Line node") || !strstr(c23, "/* Arrow node") ||
      !strstr(c23, "/* Polygon node") || !strstr(c23, "/* Star node") ||
      !strstr(c23, "/* Path node") ||
      !strstr(c23, "stygian_editor_generated_text_run(") ||
      !strstr(c23, "stygian_editor_generated_resolve_texture(")) {
    return fail("c23 output missing one or more primitive export paths");
  }

  stygian_editor_destroy(editor_a);
  stygian_editor_destroy(editor_b);
  printf("[PASS] editor content primitives mvp smoke\n");
  return 0;
}
