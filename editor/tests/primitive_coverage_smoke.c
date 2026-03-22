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
  StygianEditorLineDesc line = {0};
  StygianEditorArrowDesc arrow = {0};
  StygianEditorPolygonDesc polygon = {0};
  StygianEditorStarDesc star = {0};
  StygianEditorArcDesc arc = {0};
  char project_json[262144];
  char c23[524288];
  size_t need_json = 0u;
  size_t need_c23 = 0u;
  StygianEditorExportDiagnostic diags[32];
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
  frame.w = 720.0f;
  frame.h = 420.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.08f, 0.10f, 0.15f, 1.0f);
  frame.visible = true;
  frame.z = 0.0f;
  if (!stygian_editor_add_frame(editor_a, &frame))
    return fail("add frame failed");

  line.x1 = 40.0f;
  line.y1 = 80.0f;
  line.x2 = 320.0f;
  line.y2 = 120.0f;
  line.thickness = 3.0f;
  line.stroke = stygian_editor_color_rgba(0.3f, 0.8f, 1.0f, 1.0f);
  line.visible = true;
  line.z = 2.0f;
  if (!stygian_editor_add_line(editor_a, &line))
    return fail("add line failed");

  arrow.x1 = 60.0f;
  arrow.y1 = 180.0f;
  arrow.x2 = 360.0f;
  arrow.y2 = 220.0f;
  arrow.thickness = 2.0f;
  arrow.head_size = 18.0f;
  arrow.stroke = stygian_editor_color_rgba(1.0f, 0.75f, 0.2f, 1.0f);
  arrow.visible = true;
  arrow.z = 3.0f;
  if (!stygian_editor_add_arrow(editor_a, &arrow))
    return fail("add arrow failed");

  polygon.x = 420.0f;
  polygon.y = 70.0f;
  polygon.w = 140.0f;
  polygon.h = 140.0f;
  polygon.sides = 6u;
  polygon.corner_radius = 0.0f;
  polygon.fill = stygian_editor_color_rgba(0.55f, 0.65f, 1.0f, 0.9f);
  polygon.visible = true;
  polygon.z = 4.0f;
  if (!stygian_editor_add_polygon(editor_a, &polygon))
    return fail("add polygon failed");

  star.x = 580.0f;
  star.y = 70.0f;
  star.w = 120.0f;
  star.h = 120.0f;
  star.points = 5u;
  star.inner_ratio = 0.42f;
  star.fill = stygian_editor_color_rgba(1.0f, 0.6f, 0.2f, 0.9f);
  star.visible = true;
  star.z = 5.0f;
  if (!stygian_editor_add_star(editor_a, &star))
    return fail("add star failed");

  arc.x = 420.0f;
  arc.y = 260.0f;
  arc.w = 220.0f;
  arc.h = 120.0f;
  arc.start_angle = -30.0f;
  arc.sweep_angle = 240.0f;
  arc.thickness = 4.0f;
  arc.stroke = stygian_editor_color_rgba(0.2f, 1.0f, 0.75f, 1.0f);
  arc.visible = true;
  arc.z = 6.0f;
  if (!stygian_editor_add_arc(editor_a, &arc))
    return fail("add arc failed");

  need_json = stygian_editor_build_project_json(editor_a, NULL, 0u);
  if (need_json == 0u || need_json > sizeof(project_json))
    return fail("project json size failed");
  if (stygian_editor_build_project_json(editor_a, project_json,
                                        sizeof(project_json)) != need_json) {
    return fail("project json build failed");
  }
  if (!strstr(project_json, "k=line") || !strstr(project_json, "k=arrow") ||
      !strstr(project_json, "k=polygon") || !strstr(project_json, "k=star") ||
      !strstr(project_json, "k=arc")) {
    return fail("primitive records missing from project json");
  }

  if (!stygian_editor_load_project_json(editor_b, project_json))
    return fail("project json reload failed");

  need_c23 = stygian_editor_build_c23_with_diagnostics(
      editor_b, c23, sizeof(c23), diags, 32u, &diag_count, &has_error);
  if (need_c23 == 0u || has_error)
    return fail("c23 export failed");
  if (has_fallback_diag(diags, diag_count))
    return fail("fallback diagnostic present for primitive export");
  if (!strstr(c23, "/* Line node") || !strstr(c23, "/* Arrow node") ||
      !strstr(c23, "/* Polygon node") || !strstr(c23, "/* Star node") ||
      !strstr(c23, "/* Arc node")) {
    return fail("c23 output missing primitive draw sections");
  }

  stygian_editor_destroy(editor_a);
  stygian_editor_destroy(editor_b);
  printf("[PASS] editor primitive coverage smoke\n");
  return 0;
}
