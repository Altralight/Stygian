#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PERF_NODE_COUNT 80u
#define PERF_LOAD_BUDGET_MS 80.0
#define PERF_LAYOUT_BUDGET_MS 380.0
#define PERF_EXPORT_BUDGET_MS 80.0
#define PERF_INTERACTION_BUDGET_MS 80.0

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static double now_ms(void) {
  return (double)clock() * (1000.0 / (double)CLOCKS_PER_SEC);
}

static bool seed_perf_scene(StygianEditor *editor,
                            StygianEditorNodeId *out_frame_id) {
  uint32_t i = 0u;
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorFrameAutoLayout layout = {0};
  StygianEditorNodeId frame_id = 0u;

  if (!editor || !out_frame_id)
    return false;

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 1280.0f;
  frame.h = 720.0f;
  frame.visible = true;
  frame.clip_content = false;
  frame.fill = stygian_editor_color_rgba(0.06f, 0.08f, 0.11f, 1.0f);
  frame_id = stygian_editor_add_frame(editor, &frame);
  if (!frame_id)
    return false;

  layout.mode = STYGIAN_EDITOR_AUTO_LAYOUT_HORIZONTAL;
  layout.padding_left = 10.0f;
  layout.padding_right = 10.0f;
  layout.padding_top = 10.0f;
  layout.padding_bottom = 10.0f;
  layout.gap = 6.0f;
  layout.primary_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  layout.cross_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  if (!stygian_editor_frame_set_auto_layout(editor, frame_id, &layout))
    return false;

  memset(&rect, 0, sizeof(rect));
  rect.w = 72.0f;
  rect.h = 32.0f;
  rect.visible = true;
  rect.fill = stygian_editor_color_rgba(0.20f, 0.45f, 0.95f, 1.0f);

  for (i = 0u; i < PERF_NODE_COUNT; ++i) {
    StygianEditorNodeId node_id = stygian_editor_add_rect(editor, &rect);
    if (!node_id)
      return false;
    if (!stygian_editor_reparent_node(editor, node_id, frame_id, true))
      return false;
  }

  if (!stygian_editor_frame_recompute_layout(editor, frame_id))
    return false;
  *out_frame_id = frame_id;
  return true;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  size_t project_need = 0u;
  char *project_json = NULL;
  double t0 = 0.0;
  double t1 = 0.0;
  double load_ms = 0.0;
  double layout_ms = 0.0;
  double export_ms = 0.0;
  double interaction_ms = 0.0;
  uint32_t i = 0u;
  float x = 0.0f;
  float y = 0.0f;
  StygianEditorNodeId frame_id = 0u;

  cfg.max_nodes = 2048u;
  cfg.max_path_points = 8192u;
  cfg.max_behaviors = 256u;
  cfg.max_color_tokens = 128u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("create editor failed");

  if (!seed_perf_scene(editor, &frame_id))
    return fail("seed scene failed");

  project_need = stygian_editor_build_project_json(editor, NULL, 0u);
  if (project_need < 2u)
    return fail("build project json size failed");
  project_json = (char *)malloc(project_need);
  if (!project_json)
    return fail("project json alloc failed");
  if (stygian_editor_build_project_json(editor, project_json, project_need) !=
      project_need) {
    return fail("build project json failed");
  }

  t0 = now_ms();
  if (!stygian_editor_load_project_json(editor, project_json))
    return fail("load project json failed");
  t1 = now_ms();
  load_ms = t1 - t0;

  t0 = now_ms();
  for (i = 0u; i < 4u; ++i) {
    if (!stygian_editor_resize_frame(editor, frame_id,
                                     1280.0f + (float)(i % 7u),
                                     720.0f + (float)(i % 5u))) {
      return fail("resize frame failed");
    }
    if (!stygian_editor_frame_recompute_layout(editor, frame_id))
      return fail("recompute layout failed");
  }
  t1 = now_ms();
  layout_ms = t1 - t0;

  t0 = now_ms();
  if (stygian_editor_build_c23(editor, NULL, 0u) < 2u)
    return fail("build c23 size failed");
  t1 = now_ms();
  export_ms = t1 - t0;

  t0 = now_ms();
  for (i = 0u; i < 500u; ++i) {
    x = (float)((i * 11u) % 1200u);
    y = (float)((i * 7u) % 680u);
    (void)stygian_editor_hit_test_at(editor, x, y);
    (void)stygian_editor_select_in_rect(editor, x, y, x + 24.0f, y + 18.0f,
                                        (i % 2u) != 0u);
  }
  t1 = now_ms();
  interaction_ms = t1 - t0;

  printf("[perf] load_ms=%.3f layout_ms=%.3f export_ms=%.3f interaction_ms=%.3f\n",
         load_ms, layout_ms, export_ms, interaction_ms);

  if (load_ms > PERF_LOAD_BUDGET_MS)
    return fail("load budget exceeded");
  if (layout_ms > PERF_LAYOUT_BUDGET_MS)
    return fail("layout budget exceeded");
  if (export_ms > PERF_EXPORT_BUDGET_MS)
    return fail("export budget exceeded");
  if (interaction_ms > PERF_INTERACTION_BUDGET_MS)
    return fail("interaction budget exceeded");

  free(project_json);
  stygian_editor_destroy(editor);
  printf("[PASS] editor performance budget smoke\n");
  return 0;
}
