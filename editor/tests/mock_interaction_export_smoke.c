#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool build_json(const StygianEditor *editor, char *dst, size_t cap) {
  size_t need = 0u;
  if (!editor || !dst || cap < 2u)
    return false;
  need = stygian_editor_build_project_json(editor, NULL, 0u);
  if (need < 2u || need > cap)
    return false;
  return stygian_editor_build_project_json(editor, dst, cap) == need;
}

static bool build_c23(const StygianEditor *editor, char *dst, size_t cap) {
  size_t need = 0u;
  if (!editor || !dst || cap < 2u)
    return false;
  need = stygian_editor_build_c23(editor, NULL, 0u);
  if (need < 2u || need > cap)
    return false;
  return stygian_editor_build_c23(editor, dst, cap) == need;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorTimelineTrack track = {0};
  StygianEditorTimelineClip clip = {0};
  StygianEditorTimelineTrackId track_id = 0u;
  StygianEditorTimelineKeyframe keys[2];
  StygianEditorBehaviorRule rule = {0};
  StygianEditorNodeId root_id = 0u;
  StygianEditorNodeId sidebar_id = 0u;
  StygianEditorNodeId toggle_id = 0u;
  StygianEditorNodeId tabs_root = 0u;
  StygianEditorNodeId headers[8];
  StygianEditorNodeId page_nodes[3];
  uint32_t header_count = 0u;
  char *json_a = NULL;
  char *json_b = NULL;
  char *code_a = NULL;
  char *code_b = NULL;
  const size_t cap = 524288u;

  cfg.max_nodes = 512u;
  cfg.max_path_points = 4096u;
  cfg.max_behaviors = 512u;
  cfg.max_color_tokens = 64u;
  cfg.max_timeline_tracks = 64u;
  cfg.max_timeline_clips = 32u;

  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  json_a = (char *)malloc(cap);
  json_b = (char *)malloc(cap);
  code_a = (char *)malloc(cap);
  code_b = (char *)malloc(cap);
  if (!json_a || !json_b || !code_a || !code_b)
    return fail("buffer allocation failed");

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 1280.0f;
  frame.h = 720.0f;
  frame.clip_content = false;
  frame.fill = stygian_editor_color_rgba(0.07f, 0.08f, 0.10f, 1.0f);
  frame.visible = true;
  root_id = stygian_editor_add_frame(a, &frame);
  if (!root_id)
    return fail("root frame add failed");
  (void)stygian_editor_node_set_name(a, root_id, "mock_root");

  frame.x = -220.0f;
  frame.y = 56.0f;
  frame.w = 220.0f;
  frame.h = 620.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.10f, 0.12f, 0.15f, 1.0f);
  sidebar_id = stygian_editor_add_frame(a, &frame);
  if (!sidebar_id || !stygian_editor_reparent_node(a, sidebar_id, root_id, false))
    return fail("sidebar frame add failed");
  (void)stygian_editor_node_set_name(a, sidebar_id, "sidebar_panel");

  rect.x = 16.0f;
  rect.y = 14.0f;
  rect.w = 100.0f;
  rect.h = 30.0f;
  rect.radius[0] = rect.radius[1] = rect.radius[2] = rect.radius[3] = 8.0f;
  rect.fill = stygian_editor_color_rgba(0.18f, 0.49f, 0.95f, 1.0f);
  rect.visible = true;
  toggle_id = stygian_editor_add_rect(a, &rect);
  if (!toggle_id || !stygian_editor_reparent_node(a, toggle_id, root_id, false))
    return fail("toggle add failed");
  (void)stygian_editor_node_set_name(a, toggle_id, "sidebar_toggle");

  memset(&rule, 0, sizeof(rule));
  rule.trigger_node = toggle_id;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
  rule.action_kind = STYGIAN_EDITOR_ACTION_ANIMATE;
  rule.animate.target_node = sidebar_id;
  rule.animate.property = STYGIAN_EDITOR_PROP_X;
  rule.animate.from_value = -220.0f;
  rule.animate.to_value = 0.0f;
  rule.animate.duration_ms = 260u;
  rule.animate.easing = STYGIAN_EDITOR_EASING_OUT_CUBIC;
  if (!stygian_editor_add_behavior(a, &rule, NULL))
    return fail("sidebar behavior add failed");

  if (!stygian_editor_add_control_recipe(a, STYGIAN_EDITOR_RECIPE_TABS, 280.0f,
                                         18.0f, 300.0f, 34.0f, "Settings Tabs",
                                         &tabs_root) ||
      !tabs_root) {
    return fail("tabs recipe failed");
  }
  header_count = stygian_editor_tree_list_children(a, tabs_root, headers, 8u);
  if (header_count < 3u)
    return fail("tab headers missing");

  frame.x = 280.0f;
  frame.y = 70.0f;
  frame.w = 420.0f;
  frame.h = 220.0f;
  frame.clip_content = true;
  frame.visible = true;
  for (uint32_t i = 0u; i < 3u; ++i) {
    frame.fill = stygian_editor_color_rgba(0.11f + 0.03f * (float)i,
                                           0.14f + 0.02f * (float)i,
                                           0.20f + 0.03f * (float)i, 1.0f);
    page_nodes[i] = stygian_editor_add_frame(a, &frame);
    if (!page_nodes[i] || !stygian_editor_reparent_node(a, page_nodes[i], root_id, false))
      return fail("tab page add failed");
    (void)stygian_editor_node_set_name(a, page_nodes[i],
                                       i == 0u ? "page_general"
                                       : i == 1u ? "page_advanced"
                                                 : "page_theme");
    (void)stygian_editor_node_set_opacity(a, page_nodes[i], i == 0u ? 1.0f : 0.08f);
  }

  for (uint32_t i = 0u; i < 3u; ++i) {
    for (uint32_t j = 0u; j < 3u; ++j) {
      memset(&rule, 0, sizeof(rule));
      rule.trigger_node = headers[i];
      rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
      rule.action_kind = STYGIAN_EDITOR_ACTION_SET_PROPERTY;
      rule.set_property.target_node = page_nodes[j];
      rule.set_property.property = STYGIAN_EDITOR_PROP_OPACITY;
      rule.set_property.value = i == j ? 1.0f : 0.08f;
      if (!stygian_editor_add_behavior(a, &rule, NULL))
        return fail("tab page behavior add failed");
    }
  }

  memset(&track, 0, sizeof(track));
  track.target_node = page_nodes[0];
  track.property = STYGIAN_EDITOR_PROP_OPACITY;
  track.layer = 1u;
  snprintf(track.name, sizeof(track.name), "page_intro_fade");
  if (!stygian_editor_timeline_add_track(a, &track, &track_id) || !track_id)
    return fail("timeline track add failed");

  keys[0].time_ms = 0u;
  keys[0].value = 0.35f;
  keys[0].easing = STYGIAN_EDITOR_EASING_LINEAR;
  keys[1].time_ms = 280u;
  keys[1].value = 1.0f;
  keys[1].easing = STYGIAN_EDITOR_EASING_OUT_CUBIC;
  if (!stygian_editor_timeline_set_track_keyframes(a, track_id, keys, 2u))
    return fail("timeline keyframe set failed");

  memset(&clip, 0, sizeof(clip));
  snprintf(clip.name, sizeof(clip.name), "intro");
  clip.start_ms = 0u;
  clip.duration_ms = 280u;
  clip.layer = 1u;
  if (!stygian_editor_timeline_add_clip(a, &clip, &clip.id) || !clip.id)
    return fail("timeline clip add failed");
  if (!stygian_editor_timeline_set_clip_tracks(a, clip.id, &track_id, 1u))
    return fail("timeline clip track assign failed");

  if (!build_json(a, json_a, cap) || !build_c23(a, code_a, cap))
    return fail("build baseline export failed");
  if (!strstr(json_a, "\"timeline_tracks\": [") || !strstr(json_a, "\"behaviors\": ["))
    return fail("json missing motion sections");
  if (!strstr(code_a, "stygian_editor_generated_dispatch_event") ||
      !strstr(code_a, "kStygianEditorGeneratedRules") ||
      !strstr(code_a, "STYGIAN_EDITOR_GENERATED_BEHAVIOR_COUNT"))
    return fail("c23 missing behavior dispatch");

  if (!stygian_editor_load_project_json(b, json_a))
    return fail("roundtrip load failed");
  if (!build_json(b, json_b, cap) || !build_c23(b, code_b, cap))
    return fail("roundtrip build failed");
  if (strcmp(code_a, code_b) != 0)
    return fail("c23 changed after roundtrip");

  free(json_a);
  free(json_b);
  free(code_a);
  free(code_b);
  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor mock interaction export smoke\n");
  return 0;
}
