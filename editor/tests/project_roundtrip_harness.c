#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(const char *case_name, const char *msg) {
  fprintf(stderr, "[FAIL] %s: %s\n", case_name, msg);
  return 1;
}

static int build_json(const StygianEditor *editor, char *dst, size_t cap) {
  size_t need = stygian_editor_build_project_json(editor, NULL, 0u);
  if (need == 0u || need > cap)
    return 0;
  if (stygian_editor_build_project_json(editor, dst, cap) != need)
    return 0;
  return 1;
}

static int build_code(const StygianEditor *editor, char *dst, size_t cap) {
  size_t need = stygian_editor_build_c23(editor, NULL, 0u);
  if (need == 0u || need > cap)
    return 0;
  if (stygian_editor_build_c23(editor, dst, cap) != need)
    return 0;
  return 1;
}

static bool seed_minimal(StygianEditor *editor) {
  StygianEditorRectDesc rect = {0};
  rect.x = 20.0f;
  rect.y = 20.0f;
  rect.w = 120.0f;
  rect.h = 48.0f;
  rect.fill = stygian_editor_color_rgba(0.3f, 0.5f, 0.8f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;
  return stygian_editor_add_rect(editor, &rect) != STYGIAN_EDITOR_INVALID_ID;
}

static bool seed_medium(StygianEditor *editor) {
  StygianEditorRectDesc rect = {0};
  StygianEditorEllipseDesc ellipse = {0};
  StygianEditorPathDesc path = {0};
  StygianEditorBehaviorRule rule = {0};
  StygianEditorNodeId rect_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId ellipse_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorPathId path_id = STYGIAN_EDITOR_INVALID_ID;

  rect.x = 40.0f;
  rect.y = 52.0f;
  rect.w = 220.0f;
  rect.h = 84.0f;
  rect.radius[0] = 10.0f;
  rect.radius[1] = 14.0f;
  rect.radius[2] = 10.0f;
  rect.radius[3] = 14.0f;
  rect.fill = stygian_editor_color_rgba(0.1f, 0.2f, 0.7f, 0.95f);
  rect.visible = true;
  rect.z = 1.0f;
  rect_id = stygian_editor_add_rect(editor, &rect);
  if (rect_id == STYGIAN_EDITOR_INVALID_ID)
    return false;

  ellipse.x = 310.0f;
  ellipse.y = 128.0f;
  ellipse.w = 110.0f;
  ellipse.h = 110.0f;
  ellipse.fill = stygian_editor_color_rgba(0.85f, 0.2f, 0.3f, 0.9f);
  ellipse.visible = true;
  ellipse.z = 2.0f;
  ellipse_id = stygian_editor_add_ellipse(editor, &ellipse);
  if (ellipse_id == STYGIAN_EDITOR_INVALID_ID)
    return false;

  path.stroke = stygian_editor_color_rgba(0.9f, 0.9f, 0.2f, 1.0f);
  path.thickness = 3.0f;
  path.closed = false;
  path.visible = true;
  path.z = 0.5f;
  path_id = stygian_editor_path_begin(editor, &path);
  if (path_id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  if (!stygian_editor_path_add_point(editor, path_id, 12.0f, 18.0f, false) ||
      !stygian_editor_path_add_point(editor, path_id, 22.0f, 78.0f, false) ||
      !stygian_editor_path_add_point(editor, path_id, 96.0f, 30.0f, false)) {
    return false;
  }
  if (stygian_editor_path_commit(editor, path_id) == STYGIAN_EDITOR_INVALID_ID)
    return false;

  if (!stygian_editor_set_color_token(
          editor, "primary",
          stygian_editor_color_rgba(0.18f, 0.42f, 0.73f, 1.0f)))
    return false;
  if (!stygian_editor_apply_color_token(editor, rect_id, "primary"))
    return false;

  rule.trigger_node = rect_id;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
  rule.action_kind = STYGIAN_EDITOR_ACTION_ANIMATE;
  rule.animate.target_node = ellipse_id;
  rule.animate.property = STYGIAN_EDITOR_PROP_Y;
  rule.animate.from_value = 128.0f;
  rule.animate.to_value = 24.0f;
  rule.animate.duration_ms = 280u;
  rule.animate.easing = STYGIAN_EDITOR_EASING_OUT_CUBIC;
  return stygian_editor_add_behavior(editor, &rule, NULL);
}

static bool seed_deep_nested(StygianEditor *editor) {
  StygianEditorRectDesc rect = {0};
  StygianEditorNodeId ids[12];
  uint32_t i;
  memset(ids, 0, sizeof(ids));
  for (i = 0u; i < 12u; ++i) {
    rect.x = 24.0f + (float)(i % 4u) * 88.0f;
    rect.y = 40.0f + (float)(i / 4u) * 56.0f;
    rect.w = 72.0f;
    rect.h = 38.0f;
    rect.fill = stygian_editor_color_rgba(0.15f + 0.03f * (float)i, 0.2f, 0.3f,
                                          0.95f);
    rect.visible = true;
    rect.z = 1.0f + 0.01f * (float)i;
    ids[i] = stygian_editor_add_rect(editor, &rect);
    if (ids[i] == STYGIAN_EDITOR_INVALID_ID)
      return false;
  }
  // Build a real hierarchy chain and siblings.
  if (!stygian_editor_reparent_node(editor, ids[1], ids[0], false))
    return false;
  if (!stygian_editor_reparent_node(editor, ids[2], ids[1], false))
    return false;
  if (!stygian_editor_reparent_node(editor, ids[3], ids[2], false))
    return false;
  if (!stygian_editor_reparent_node(editor, ids[4], ids[2], false))
    return false;
  if (!stygian_editor_reparent_node(editor, ids[5], ids[1], false))
    return false;
  if (!stygian_editor_reparent_node(editor, ids[6], ids[0], false))
    return false;
  if (!stygian_editor_reparent_node(editor, ids[7], ids[6], false))
    return false;
  if (!stygian_editor_reparent_node(editor, ids[8], ids[6], false))
    return false;
  if (!stygian_editor_reparent_node(editor, ids[9], ids[8], false))
    return false;
  if (!stygian_editor_reparent_node(editor, ids[10], ids[9], false))
    return false;
  if (!stygian_editor_reparent_node(editor, ids[11], ids[10], false))
    return false;

  for (i = 1u; i < 12u; ++i) {
    StygianEditorBehaviorRule rule = {0};
    rule.trigger_node = ids[0];
    rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
    rule.action_kind = STYGIAN_EDITOR_ACTION_ANIMATE;
    rule.animate.target_node = ids[i];
    rule.animate.property = STYGIAN_EDITOR_PROP_OPACITY;
    rule.animate.from_value = 0.95f;
    rule.animate.to_value = 0.35f;
    rule.animate.duration_ms = 240u + i * 10u;
    rule.animate.easing = STYGIAN_EDITOR_EASING_IN_OUT_CUBIC;
    if (!stygian_editor_add_behavior(editor, &rule, NULL))
      return false;
  }
  return true;
}

static int run_case(const char *case_name, bool (*seed_fn)(StygianEditor *)) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditor *c = NULL;
  char *json_a = NULL;
  char *json_b = NULL;
  char *json_c = NULL;
  char *c_a = NULL;
  char *c_b = NULL;
  char file_path[512];
  size_t buf_cap = 262144u;
  int rc = 0;

  cfg.max_nodes = 1024u;
  cfg.max_path_points = 16384u;
  cfg.max_behaviors = 2048u;
  cfg.max_color_tokens = 128u;

  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  c = stygian_editor_create(&cfg, NULL);
  if (!a || !b || !c)
    return fail(case_name, "editor create failed");
  json_a = (char *)malloc(buf_cap);
  json_b = (char *)malloc(buf_cap);
  json_c = (char *)malloc(buf_cap);
  c_a = (char *)malloc(buf_cap);
  c_b = (char *)malloc(buf_cap);
  if (!json_a || !json_b || !json_c || !c_a || !c_b) {
    rc = fail(case_name, "buffer allocation failed");
    goto cleanup;
  }
  if (!seed_fn(a))
    {
      rc = fail(case_name, "fixture seed failed");
      goto cleanup;
    }

  if (!build_json(a, json_a, buf_cap)) {
    rc = fail(case_name, "build json (A) failed");
    goto cleanup;
  }
  if (!stygian_editor_load_project_json(b, json_a))
    {
      rc = fail(case_name, "load json into B failed");
      goto cleanup;
    }
  if (!build_json(b, json_b, buf_cap)) {
    rc = fail(case_name, "build json (B) failed");
    goto cleanup;
  }
  if (strcmp(json_a, json_b) != 0) {
    rc = fail(case_name, "normalized project json mismatch");
    goto cleanup;
  }

  if (!build_code(a, c_a, buf_cap) || !build_code(b, c_b, buf_cap)) {
    rc = fail(case_name, "build code failed");
    goto cleanup;
  }
  if (strcmp(c_a, c_b) != 0) {
    rc = fail(case_name, "generated C23 mismatch after load");
    goto cleanup;
  }

  snprintf(file_path, sizeof(file_path),
           "editor/fixtures/roundtrip_harness_%s.project.json", case_name);
  if (!stygian_editor_save_project_file(a, file_path)) {
    rc = fail(case_name, "save project file failed");
    goto cleanup;
  }
  if (!stygian_editor_load_project_file(c, file_path)) {
    rc = fail(case_name, "load project file failed");
    goto cleanup;
  }
  if (!build_json(c, json_c, buf_cap)) {
    rc = fail(case_name, "build json (C) failed");
    goto cleanup;
  }
  if (strcmp(json_a, json_c) != 0) {
    rc = fail(case_name, "saved file roundtrip mismatch");
    goto cleanup;
  }

cleanup:
  free(json_a);
  free(json_b);
  free(json_c);
  free(c_a);
  free(c_b);
  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  stygian_editor_destroy(c);
  return rc;
}

int main(void) {
  int rc = 0;
  rc = run_case("minimal", seed_minimal);
  if (rc != 0)
    return rc;
  rc = run_case("medium", seed_medium);
  if (rc != 0)
    return rc;
  rc = run_case("deep_nested", seed_deep_nested);
  if (rc != 0)
    return rc;
  printf("[PASS] editor project roundtrip harness\n");
  return 0;
}
