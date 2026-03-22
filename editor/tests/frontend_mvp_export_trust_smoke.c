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

static bool build_code_with_diags(const StygianEditor *editor, char *dst,
                                  size_t cap,
                                  StygianEditorExportDiagnostic *diags,
                                  uint32_t max_diags, uint32_t *out_diag_count,
                                  bool *out_has_error) {
  size_t need = 0u;
  if (!editor || !dst || cap < 2u || !out_diag_count || !out_has_error)
    return false;
  need = stygian_editor_build_c23_with_diagnostics(
      editor, dst, cap, diags, max_diags, out_diag_count, out_has_error);
  return need > 0u && need < cap;
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
  const char *ea = NULL;
  const char *eb = NULL;
  size_t lena = 0u;
  size_t lenb = 0u;
  uint32_t changed = 0u;
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
  const char *line = NULL;
  const char *end = NULL;
  size_t len = 0u;
  size_t used = 0u;
  if (!code || !out || out_cap < 2u)
    return false;
  out[0] = '\0';
  line = code;
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

static bool has_diag_feature_for_node(const StygianEditorExportDiagnostic *diags,
                                      uint32_t count, StygianEditorNodeId node_id,
                                      const char *feature) {
  uint32_t i = 0u;
  if (!diags || !feature || node_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  for (i = 0u; i < count; ++i) {
    if (diags[i].node_id == node_id && strcmp(diags[i].feature, feature) == 0)
      return true;
  }
  return false;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorEllipseDesc ellipse = {0};
  StygianEditorTextDesc text = {0};
  StygianEditorImageDesc image = {0};
  StygianEditorNodeFill gradient = {0};
  StygianEditorNodeEffect shadow = {0};
  StygianEditorPathDesc path = {0};
  StygianEditorPathId path_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId panel_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId dot_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId text_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId image_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId path_node_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorExportDiagnostic diags[32];
  uint32_t diag_count_a = 0u;
  uint32_t diag_count_b = 0u;
  bool has_error = false;
  size_t json_need = 0u;
  char json[262144];
  char code_a[524288];
  char code_b[524288];
  char code_roundtrip[524288];
  char code_tiny[524288];
  char markers_a[4096];
  char markers_tiny[4096];
  uint32_t tiny_diff = 0u;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 2048u;
  cfg.max_behaviors = 128u;
  cfg.max_color_tokens = 64u;

  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  rect.x = 20.0f;
  rect.y = 24.0f;
  rect.w = 260.0f;
  rect.h = 160.0f;
  rect.radius[0] = 16.0f;
  rect.radius[1] = 16.0f;
  rect.radius[2] = 16.0f;
  rect.radius[3] = 16.0f;
  rect.fill = stygian_editor_color_rgba(0.13f, 0.17f, 0.24f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;
  panel_id = stygian_editor_add_rect(a, &rect);
  if (panel_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add panel failed");
  if (!stygian_editor_node_set_name(a, panel_id, "shell_panel"))
    return fail("set panel name failed");
  if (!stygian_editor_node_set_rotation(a, panel_id, 8.0f))
    return fail("set panel rotation failed");

  memset(&gradient, 0, sizeof(gradient));
  gradient.kind = STYGIAN_EDITOR_FILL_LINEAR_GRADIENT;
  gradient.visible = true;
  gradient.opacity = 1.0f;
  gradient.stops[0].position = 0.0f;
  gradient.stops[0].color = stygian_editor_color_rgba(0.20f, 0.46f, 0.94f, 1.0f);
  gradient.stops[1].position = 1.0f;
  gradient.stops[1].color = stygian_editor_color_rgba(0.12f, 0.18f, 0.30f, 1.0f);
  gradient.gradient_angle_deg = 25.0f;
  if (!stygian_editor_node_set_fills(a, panel_id, &gradient, 1u))
    return fail("set panel gradient failed");

  memset(&shadow, 0, sizeof(shadow));
  shadow.kind = STYGIAN_EDITOR_EFFECT_DROP_SHADOW;
  shadow.visible = true;
  shadow.opacity = 0.7f;
  shadow.radius = 12.0f;
  shadow.spread = 2.0f;
  shadow.offset_x = 5.0f;
  shadow.offset_y = 9.0f;
  shadow.color = stygian_editor_color_rgba(0.0f, 0.0f, 0.0f, 1.0f);
  if (!stygian_editor_node_set_effects(a, panel_id, &shadow, 1u))
    return fail("set panel effect failed");

  ellipse.x = 44.0f;
  ellipse.y = 52.0f;
  ellipse.w = 12.0f;
  ellipse.h = 12.0f;
  ellipse.fill = stygian_editor_color_rgba(0.95f, 0.62f, 0.18f, 1.0f);
  ellipse.visible = true;
  ellipse.z = 2.0f;
  dot_id = stygian_editor_add_ellipse(a, &ellipse);
  if (dot_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add status dot failed");
  if (!stygian_editor_node_set_name(a, dot_id, "status_dot"))
    return fail("set status dot name failed");

  text.x = 72.0f;
  text.y = 48.0f;
  text.w = 180.0f;
  text.h = 42.0f;
  text.font_size = 18.0f;
  text.line_height = 22.0f;
  text.letter_spacing = 0.5f;
  text.font_weight = 600u;
  text.box_mode = STYGIAN_EDITOR_TEXT_BOX_AREA;
  text.align_h = STYGIAN_EDITOR_TEXT_ALIGN_LEFT;
  text.align_v = STYGIAN_EDITOR_TEXT_ALIGN_TOP;
  text.auto_size = STYGIAN_EDITOR_TEXT_AUTOSIZE_NONE;
  text.fill = stygian_editor_color_rgba(0.96f, 0.97f, 0.99f, 1.0f);
  text.text = "Project Files";
  text.visible = true;
  text.z = 3.0f;
  text_id = stygian_editor_add_text(a, &text);
  if (text_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add text failed");
  if (!stygian_editor_node_set_name(a, text_id, "title_text"))
    return fail("set text name failed");

  image.x = 32.0f;
  image.y = 104.0f;
  image.w = 216.0f;
  image.h = 96.0f;
  image.fit_mode = 2u;
  image.source = "assets/ui/hero.png";
  image.visible = true;
  image.z = 4.0f;
  image_id = stygian_editor_add_image(a, &image);
  if (image_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("add image failed");
  if (!stygian_editor_node_set_name(a, image_id, "hero_image"))
    return fail("set image name failed");

  path.stroke = stygian_editor_color_rgba(0.96f, 0.86f, 0.28f, 1.0f);
  path.thickness = 2.0f;
  path.closed = true;
  path.visible = true;
  path.z = 5.0f;
  path_id = stygian_editor_path_begin(a, &path);
  if (path_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("path begin failed");
  if (!stygian_editor_path_add_point(a, path_id, 308.0f, 72.0f, false) ||
      !stygian_editor_path_add_point(a, path_id, 364.0f, 88.0f, false) ||
      !stygian_editor_path_add_point(a, path_id, 336.0f, 136.0f, false)) {
    return fail("path add point failed");
  }
  path_node_id = stygian_editor_path_commit(a, path_id);
  if (path_node_id == STYGIAN_EDITOR_INVALID_ID)
    return fail("path commit failed");
  if (!stygian_editor_node_set_name(a, path_node_id, "warning_path"))
    return fail("set path name failed");

  diag_count_a = stygian_editor_collect_export_diagnostics(a, diags, 32u);
  if (!has_diag_feature_for_node(diags, diag_count_a, path_node_id,
                                 "closed_path_fill")) {
    return fail("missing node-local unsupported-case diagnostic");
  }

  if (!build_code_with_diags(a, code_a, sizeof(code_a), diags, 32u, &diag_count_a,
                             &has_error)) {
    return fail("first export failed");
  }
  if (has_error)
    return fail("supported mockup export should not hard fail");
  if (!contains(code_a, "Exporter-owned generated zone: regenerated on every export."))
    return fail("missing generated ownership marker");
  if (!contains(code_a, "Hook zone lives in stygian_editor_hooks.h/.c and stays user-owned."))
    return fail("missing hook ownership marker");
  if (!contains(code_a, "stygian_editor_generated_draw_image(ctx, _tex,"))
    return fail("missing image helper emission");
  if (!contains(code_a, "stygian_editor_generated_draw_text_block(ctx,"))
    return fail("missing text block helper emission");

  if (!build_code_with_diags(a, code_b, sizeof(code_b), diags, 32u, &diag_count_b,
                             &has_error)) {
    return fail("second export failed");
  }
  if (strcmp(code_a, code_b) != 0)
    return fail("repeated export was not deterministic");
  if (!collect_node_markers(code_a, markers_a, sizeof(markers_a)))
    return fail("collect base markers failed");

  json_need = stygian_editor_build_project_json(a, NULL, 0u);
  if (json_need == 0u || json_need > sizeof(json))
    return fail("project json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != json_need)
    return fail("project json build failed");
  if (!stygian_editor_load_project_json(b, json))
    return fail("project json reload failed");
  if (!build_code_with_diags(b, code_roundtrip, sizeof(code_roundtrip), diags, 32u,
                             &diag_count_b, &has_error)) {
    return fail("roundtrip export failed");
  }
  if (strcmp(code_a, code_roundtrip) != 0)
    return fail("build/load/build changed generated code");

  if (!stygian_editor_node_set_position(a, dot_id, 45.0f, 52.0f, false))
    return fail("tiny position edit failed");
  if (!build_code_with_diags(a, code_tiny, sizeof(code_tiny), diags, 32u,
                             &diag_count_b, &has_error)) {
    return fail("tiny edit export failed");
  }
  if (!collect_node_markers(code_tiny, markers_tiny, sizeof(markers_tiny)))
    return fail("collect tiny markers failed");
  if (strcmp(markers_a, markers_tiny) != 0)
    return fail("tiny edit changed node marker order");
  tiny_diff = changed_line_count(code_a, code_tiny);
  if (tiny_diff == 0u || tiny_diff > 32u)
    return fail("tiny edit churned too much generated output");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] frontend MVP export trust smoke\n");
  return 0;
}
