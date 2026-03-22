#include "../include/stygian_editor.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool nearf(float a, float b) { return fabsf(a - b) <= 0.05f; }

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorSnapSources sources = {0};
  StygianEditorGuide guide = {0};
  StygianEditorNodeId frame_id = 0u;
  StygianEditorNodeId selected_id = 0u;
  StygianEditorNodeId other_id = 0u;
  uint32_t gid_v = 0u;
  uint32_t gid_h = 0u;
  float sx = 0.0f, sy = 0.0f;
  char json[262144];
  size_t need = 0u;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 4096u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 16u;
  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  frame.x = 10.0f;
  frame.y = 20.0f;
  frame.w = 300.0f;
  frame.h = 200.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.2f, 0.2f, 0.2f, 1.0f);
  frame.visible = true;
  frame.z = 0.0f;
  frame_id = stygian_editor_add_frame(a, &frame);
  if (!frame_id)
    return fail("add frame failed");

  rect.fill = stygian_editor_color_rgba(1.0f, 1.0f, 1.0f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;
  rect.x = 0.0f;
  rect.y = 0.0f;
  rect.w = 40.0f;
  rect.h = 20.0f;
  selected_id = stygian_editor_add_rect(a, &rect);
  rect.x = 200.0f;
  rect.y = 100.0f;
  rect.w = 40.0f;
  rect.h = 20.0f;
  other_id = stygian_editor_add_rect(a, &rect);
  if (!selected_id || !other_id)
    return fail("add rect failed");
  if (!stygian_editor_reparent_node(a, selected_id, frame_id, false) ||
      !stygian_editor_reparent_node(a, other_id, frame_id, false)) {
    return fail("reparent failed");
  }
  if (!stygian_editor_select_node(a, selected_id, false))
    return fail("select failed");

  sources.use_grid = false;
  sources.use_guides = true;
  sources.use_bounds = false;
  sources.use_parent_edges = false;
  stygian_editor_set_snap_sources(a, &sources);

  guide.id = 0u;
  guide.axis = STYGIAN_EDITOR_GUIDE_VERTICAL;
  guide.position = 100.0f;
  guide.parent_id = STYGIAN_EDITOR_INVALID_ID;
  gid_v = stygian_editor_add_guide(a, &guide);
  guide.axis = STYGIAN_EDITOR_GUIDE_HORIZONTAL;
  guide.position = 50.0f;
  gid_h = stygian_editor_add_guide(a, &guide);
  if (!gid_v || !gid_h)
    return fail("add guide failed");

  stygian_editor_snap_world_point(a, 97.0f, 49.0f, &sx, &sy);
  if (!nearf(sx, 100.0f) || !nearf(sy, 50.0f))
    return fail("guide snap failed");

  sources.use_guides = false;
  sources.use_bounds = true;
  stygian_editor_set_snap_sources(a, &sources);
  stygian_editor_snap_world_point(a, 241.0f, 101.0f, &sx, &sy);
  if (!nearf(sx, 250.0f) || !nearf(sy, 120.0f)) {
    fprintf(stderr, "bounds snap debug: sx=%.3f sy=%.3f\n", sx, sy);
    return fail("bounds snap failed");
  }

  sources.use_bounds = false;
  sources.use_parent_edges = true;
  stygian_editor_set_snap_sources(a, &sources);
  stygian_editor_snap_world_point(a, 311.0f, 221.0f, &sx, &sy);
  if (!nearf(sx, 310.0f) || !nearf(sy, 220.0f))
    return fail("parent-edge snap failed");

  stygian_editor_set_ruler_unit(a, STYGIAN_EDITOR_RULER_UNIT_PX);

  need = stygian_editor_build_project_json(a, NULL, 0u);
  if (need == 0u || need > sizeof(json))
    return fail("json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need)
    return fail("json build failed");
  if (!stygian_editor_load_project_json(b, json))
    return fail("json load failed");
  if (stygian_editor_guide_count(b) != 2u)
    return fail("guide count persistence failed");
  {
    StygianEditorGuide g = {0};
    if (!stygian_editor_get_guide(b, 0u, &g))
      return fail("guide read failed");
    if (g.id == 0u)
      return fail("guide id persistence failed");
  }
  if (stygian_editor_get_ruler_unit(b) != STYGIAN_EDITOR_RULER_UNIT_PX)
    return fail("ruler persistence failed");
  sources = stygian_editor_get_snap_sources(b);
  if (!sources.use_parent_edges || sources.use_grid || sources.use_guides ||
      sources.use_bounds) {
    return fail("snap source persistence failed");
  }
  if (!stygian_editor_remove_guide(b, gid_v))
    return fail("remove guide failed");
  if (stygian_editor_guide_count(b) != 1u)
    return fail("guide remove count failed");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor guides snap smoke\n");
  return 0;
}
