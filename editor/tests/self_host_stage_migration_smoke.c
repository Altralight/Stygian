#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum SmokeSelfHostStage {
  SMOKE_STAGE_1 = 1,
  SMOKE_STAGE_2 = 2,
  SMOKE_STAGE_3 = 3,
} SmokeSelfHostStage;

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool build_stage_scene(StygianEditor *editor, SmokeSelfHostStage stage) {
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorNodeId root_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId top_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId viewport_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId inspector_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId id = STYGIAN_EDITOR_INVALID_ID;

  if (!editor || stage < SMOKE_STAGE_1 || stage > SMOKE_STAGE_3)
    return false;

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 1440.0f;
  frame.h = 900.0f;
  frame.clip_content = false;
  frame.fill = stygian_editor_color_rgba(0.06f, 0.07f, 0.09f, 1.0f);
  frame.visible = true;
  frame.z = 0.0f;
  root_id = stygian_editor_add_frame(editor, &frame);
  if (!root_id)
    return false;
  (void)stygian_editor_node_set_name(editor, root_id, "editor_root");

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 1440.0f;
  frame.h = 58.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.20f, 0.20f, 0.22f, 1.0f);
  frame.z = 1.0f;
  top_id = stygian_editor_add_frame(editor, &frame);
  if (!top_id || !stygian_editor_reparent_node(editor, top_id, root_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, top_id, "top_chrome");

  frame.x = 0.0f;
  frame.y = 58.0f;
  frame.w = 1088.0f;
  frame.h = 842.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.05f, 0.06f, 0.08f, 1.0f);
  frame.z = 1.0f;
  viewport_id = stygian_editor_add_frame(editor, &frame);
  if (!viewport_id ||
      !stygian_editor_reparent_node(editor, viewport_id, root_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, viewport_id, "viewport_shell");

  frame.x = 1088.0f;
  frame.y = 58.0f;
  frame.w = 352.0f;
  frame.h = 842.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.10f, 0.12f, 0.15f, 1.0f);
  frame.z = 1.0f;
  inspector_id = stygian_editor_add_frame(editor, &frame);
  if (!inspector_id ||
      !stygian_editor_reparent_node(editor, inspector_id, root_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, inspector_id, "inspector_shell");

  rect.x = 10.0f;
  rect.y = 6.0f;
  rect.w = 54.0f;
  rect.h = 22.0f;
  rect.fill = stygian_editor_color_rgba(0.24f, 0.24f, 0.26f, 1.0f);
  rect.visible = true;
  rect.z = 2.0f;
  id = stygian_editor_add_rect(editor, &rect);
  if (!id || !stygian_editor_reparent_node(editor, id, top_id, false))
    return false;

  rect.x = 80.0f;
  rect.y = 6.0f;
  rect.w = 110.0f;
  rect.h = 22.0f;
  rect.fill = stygian_editor_color_rgba(0.14f, 0.38f, 0.61f, 1.0f);
  id = stygian_editor_add_rect(editor, &rect);
  if (!id || !stygian_editor_reparent_node(editor, id, top_id, false))
    return false;

  if (stage >= SMOKE_STAGE_2) {
    frame.x = 1088.0f;
    frame.y = 58.0f;
    frame.w = 352.0f;
    frame.h = 380.0f;
    frame.clip_content = true;
    frame.fill = stygian_editor_color_rgba(0.08f, 0.10f, 0.13f, 1.0f);
    frame.z = 2.0f;
    id = stygian_editor_add_frame(editor, &frame);
    if (!id || !stygian_editor_reparent_node(editor, id, inspector_id, false))
      return false;

    frame.x = 1088.0f;
    frame.y = 438.0f;
    frame.w = 352.0f;
    frame.h = 462.0f;
    frame.clip_content = true;
    frame.fill = stygian_editor_color_rgba(0.07f, 0.08f, 0.10f, 1.0f);
    frame.z = 2.0f;
    id = stygian_editor_add_frame(editor, &frame);
    if (!id || !stygian_editor_reparent_node(editor, id, inspector_id, false))
      return false;
  }

  if (stage >= SMOKE_STAGE_3) {
    frame.x = 8.0f;
    frame.y = 8.0f;
    frame.w = 320.0f;
    frame.h = 44.0f;
    frame.clip_content = true;
    frame.fill = stygian_editor_color_rgba(0.12f, 0.13f, 0.16f, 1.0f);
    frame.z = 2.0f;
    id = stygian_editor_add_frame(editor, &frame);
    if (!id || !stygian_editor_reparent_node(editor, id, viewport_id, false))
      return false;

    rect.x = 16.0f;
    rect.y = 14.0f;
    rect.w = 132.0f;
    rect.h = 22.0f;
    rect.fill = stygian_editor_color_rgba(0.16f, 0.45f, 0.72f, 1.0f);
    id = stygian_editor_add_rect(editor, &rect);
    if (!id || !stygian_editor_reparent_node(editor, id, viewport_id, false))
      return false;
  }

  return true;
}

static int verify_stage_roundtrip(StygianEditor *a, StygianEditor *b,
                                  SmokeSelfHostStage stage) {
  size_t need_json = 0u;
  size_t need_c23 = 0u;
  size_t need_c23_b = 0u;
  char *json = NULL;
  char *code_a = NULL;
  char *code_b = NULL;

  stygian_editor_reset(a);
  if (!build_stage_scene(a, stage))
    return fail("build stage scene failed");

  need_json = stygian_editor_build_project_json(a, NULL, 0u);
  if (need_json < 2u)
    return fail("json size failed");
  json = (char *)malloc(need_json);
  if (!json)
    return fail("alloc json failed");
  if (stygian_editor_build_project_json(a, json, need_json) != need_json)
    return fail("json build failed");

  stygian_editor_reset(b);
  if (!stygian_editor_load_project_json(b, json))
    return fail("json load failed");

  need_c23 = stygian_editor_build_c23(a, NULL, 0u);
  need_c23_b = stygian_editor_build_c23(b, NULL, 0u);
  if (need_c23 < 2u || need_c23_b < 2u || need_c23 != need_c23_b)
    return fail("c23 size mismatch");

  code_a = (char *)malloc(need_c23);
  code_b = (char *)malloc(need_c23_b);
  if (!code_a || !code_b)
    return fail("alloc c23 buffers failed");
  if (stygian_editor_build_c23(a, code_a, need_c23) != need_c23)
    return fail("c23 build a failed");
  if (stygian_editor_build_c23(b, code_b, need_c23_b) != need_c23_b)
    return fail("c23 build b failed");
  if (strcmp(code_a, code_b) != 0)
    return fail("c23 roundtrip mismatch");

  free(code_b);
  free(code_a);
  free(json);
  return 0;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  uint32_t count1 = 0u;
  uint32_t count2 = 0u;
  uint32_t count3 = 0u;

  cfg.max_nodes = 512u;
  cfg.max_path_points = 4096u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 32u;

  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("create editors failed");

  if (verify_stage_roundtrip(a, b, SMOKE_STAGE_1) != 0)
    return 1;
  count1 = stygian_editor_node_count(a);

  if (verify_stage_roundtrip(a, b, SMOKE_STAGE_2) != 0)
    return 1;
  count2 = stygian_editor_node_count(a);

  if (verify_stage_roundtrip(a, b, SMOKE_STAGE_3) != 0)
    return 1;
  count3 = stygian_editor_node_count(a);

  if (!(count1 < count2 && count2 < count3))
    return fail("stage complexity ordering mismatch");

  stygian_editor_destroy(b);
  stygian_editor_destroy(a);
  printf("[PASS] editor self-host stage migration smoke\n");
  return 0;
}
