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
  StygianEditorRectDesc rect = {0};
  StygianEditorPathDesc path_desc = {0};
  StygianEditorNodeEffect effects[3] = {0};
  StygianEditorEffectTransform ex = {0};
  StygianEditorShaderAttachment shader = {0};
  StygianEditorShaderAttachment shader_out = {0};
  StygianEditorExportDiagnostic diags[32];
  StygianEditorNodeId rect_id = 0u;
  StygianEditorNodeId path_id = 0u;
  StygianEditorNodeId mask_id = 0u;
  StygianEditorMaskMode mask_mode = STYGIAN_EDITOR_MASK_ALPHA;
  bool mask_invert = false;
  uint32_t diag_count = 0u;
  char json[262144];
  char c23[524288];

  cfg.max_nodes = 128u;
  cfg.max_path_points = 1024u;
  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  rect.x = 40.0f;
  rect.y = 48.0f;
  rect.w = 180.0f;
  rect.h = 120.0f;
  rect.fill = stygian_editor_color_rgba(0.8f, 0.82f, 0.86f, 1.0f);
  rect.visible = true;
  rect_id = stygian_editor_add_rect(a, &rect);
  if (!rect_id)
    return fail("add rect failed");

  path_desc.stroke = stygian_editor_color_rgba(1.0f, 1.0f, 1.0f, 1.0f);
  path_desc.thickness = 2.0f;
  path_desc.closed = false;
  path_desc.visible = true;
  {
    StygianEditorPathId pb = stygian_editor_path_begin(a, &path_desc);
    if (!pb)
      return fail("path begin failed");
    if (!stygian_editor_path_add_point(a, pb, 44.0f, 56.0f, false) ||
        !stygian_editor_path_add_point(a, pb, 188.0f, 66.0f, false) ||
        !stygian_editor_path_add_point(a, pb, 176.0f, 154.0f, false)) {
      return fail("path points failed");
    }
    path_id = stygian_editor_path_commit(a, pb);
  }
  if (!path_id)
    return fail("path commit failed");

  effects[0].kind = STYGIAN_EDITOR_EFFECT_DROP_SHADOW;
  effects[0].visible = true;
  effects[0].opacity = 0.8f;
  effects[0].radius = 12.0f;
  effects[0].spread = 2.0f;
  effects[0].offset_x = 4.0f;
  effects[0].offset_y = 6.0f;
  effects[0].intensity = 1.0f;
  effects[0].color = stygian_editor_color_rgba(0.0f, 0.0f, 0.0f, 1.0f);
  effects[1].kind = STYGIAN_EDITOR_EFFECT_LAYER_BLUR;
  effects[1].visible = true;
  effects[1].opacity = 1.0f;
  effects[1].radius = 6.0f;
  effects[1].intensity = 1.0f;
  effects[2].kind = STYGIAN_EDITOR_EFFECT_NOISE;
  effects[2].visible = true;
  effects[2].opacity = 0.4f;
  effects[2].radius = 2.0f;
  effects[2].intensity = 0.6f;
  if (!stygian_editor_node_set_effects(a, rect_id, effects, 3u))
    return fail("set effects failed");

  ex.scale_x = 1.25f;
  ex.scale_y = 0.8f;
  ex.rotation_deg = 18.0f;
  if (!stygian_editor_node_set_effect_transform(a, rect_id, 2u, &ex))
    return fail("set effect transform failed");

  if (!stygian_editor_node_set_mask(a, rect_id, path_id, STYGIAN_EDITOR_MASK_ALPHA,
                                    false)) {
    return fail("set mask failed");
  }

  shader.enabled = true;
  snprintf(shader.slot_name, sizeof(shader.slot_name), "%s", "surface");
  snprintf(shader.asset_path, sizeof(shader.asset_path), "%s",
           "shaders/fx/card_glow.frag");
  snprintf(shader.entry_point, sizeof(shader.entry_point), "%s", "main");
  if (!stygian_editor_node_set_shader_attachment(a, rect_id, &shader))
    return fail("set shader attachment failed");
  if (!stygian_editor_node_get_shader_attachment(a, rect_id, &shader_out))
    return fail("get shader attachment failed");
  if (!shader_out.enabled || strcmp(shader_out.asset_path, shader.asset_path) != 0)
    return fail("shader attachment mismatch");

  if (stygian_editor_build_project_json(a, json, sizeof(json)) == 0u)
    return fail("build json failed");
  if (!contains(json, "effects=") || !contains(json, "mask=") ||
      !contains(json, "shasset=") || !contains(json, "shentry=")) {
    return fail("batch e fields missing from json");
  }

  if (!stygian_editor_load_project_json(b, json))
    return fail("reload json failed");
  if (stygian_editor_node_effect_count(b, rect_id) != 3u)
    return fail("reload effect count mismatch");
  if (!stygian_editor_node_get_mask(b, rect_id, &mask_id, &mask_mode, &mask_invert))
    return fail("reload mask failed");
  if (mask_id != path_id || mask_mode != STYGIAN_EDITOR_MASK_ALPHA || mask_invert)
    return fail("reload mask mismatch");
  if (!stygian_editor_node_get_shader_attachment(b, rect_id, &shader_out))
    return fail("reload shader attachment failed");
  if (!shader_out.enabled || strcmp(shader_out.slot_name, "surface") != 0 ||
      strcmp(shader_out.entry_point, "main") != 0) {
    return fail("reload shader attachment mismatch");
  }

  diag_count = stygian_editor_collect_export_diagnostics(
      b, diags, (uint32_t)(sizeof(diags) / sizeof(diags[0])));
  if (!has_diag_feature(diags, diag_count, "effect_stack_partial_export"))
    return fail("missing effect stack diagnostic");
  if (!has_diag_feature(diags, diag_count, "shader_attachment_host_binding"))
    return fail("missing shader host binding diagnostic");

  if (stygian_editor_build_c23(b, c23, sizeof(c23)) == 0u)
    return fail("build c23 failed");
  if (!contains(c23, "authored shader attachment"))
    return fail("missing shader attachment export marker");
  if (!contains(c23, "stygian_set_shadow(") || !contains(c23, "stygian_set_blur("))
    return fail("missing effect export marker");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor shader/effect Batch E smoke\n");
  return 0;
}
