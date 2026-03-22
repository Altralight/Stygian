#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct FixtureIds {
  StygianEditorNodeId primary_edit_id;
  StygianEditorNodeId secondary_edit_id;
} FixtureIds;

static int fail_case(const char *case_name, const char *msg) {
  fprintf(stderr, "[FAIL] %s: %s\n", case_name, msg);
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
  uint32_t changed = 0u;
  const char *ea = NULL;
  const char *eb = NULL;
  size_t lena = 0u;
  size_t lenb = 0u;
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

static bool has_symbol(const char *code, const char *symbol) {
  return code && symbol && strstr(code, symbol) != NULL;
}

static bool seed_dashboard_shell(StygianEditor *editor, FixtureIds *ids) {
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorBehaviorRule rule = {0};
  StygianEditorTimelineTrack track = {0};
  StygianEditorTimelineClip clip = {0};
  StygianEditorTimelineTrackId track_id = 0u;
  StygianEditorTimelineKeyframe keys[2];
  StygianEditorNodeId root_id = 0u;
  StygianEditorNodeId topbar_id = 0u;
  StygianEditorNodeId sidebar_id = 0u;
  StygianEditorNodeId content_id = 0u;
  StygianEditorNodeId toggle_id = 0u;
  StygianEditorNodeId card_id = 0u;

  if (!editor || !ids)
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
  (void)stygian_editor_node_set_name(editor, root_id, "dashboard_root");

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 1440.0f;
  frame.h = 56.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.14f, 0.15f, 0.18f, 1.0f);
  frame.z = 1.0f;
  topbar_id = stygian_editor_add_frame(editor, &frame);
  if (!topbar_id ||
      !stygian_editor_reparent_node(editor, topbar_id, root_id, false)) {
    return false;
  }
  (void)stygian_editor_node_set_name(editor, topbar_id, "topbar");
  if (!stygian_editor_node_set_constraints(editor, topbar_id,
                                           STYGIAN_EDITOR_CONSTRAINT_H_LEFT_RIGHT,
                                           STYGIAN_EDITOR_CONSTRAINT_V_TOP)) {
    return false;
  }

  frame.x = 0.0f;
  frame.y = 56.0f;
  frame.w = 260.0f;
  frame.h = 844.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.09f, 0.11f, 0.14f, 1.0f);
  frame.z = 1.0f;
  sidebar_id = stygian_editor_add_frame(editor, &frame);
  if (!sidebar_id ||
      !stygian_editor_reparent_node(editor, sidebar_id, root_id, false)) {
    return false;
  }
  (void)stygian_editor_node_set_name(editor, sidebar_id, "sidebar");
  if (!stygian_editor_node_set_constraints(
          editor, sidebar_id, STYGIAN_EDITOR_CONSTRAINT_H_LEFT,
          STYGIAN_EDITOR_CONSTRAINT_V_TOP_BOTTOM)) {
    return false;
  }

  frame.x = 260.0f;
  frame.y = 56.0f;
  frame.w = 1180.0f;
  frame.h = 844.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.07f, 0.08f, 0.11f, 1.0f);
  frame.z = 1.0f;
  content_id = stygian_editor_add_frame(editor, &frame);
  if (!content_id ||
      !stygian_editor_reparent_node(editor, content_id, root_id, false)) {
    return false;
  }
  (void)stygian_editor_node_set_name(editor, content_id, "content");
  if (!stygian_editor_node_set_constraints(
          editor, content_id, STYGIAN_EDITOR_CONSTRAINT_H_LEFT_RIGHT,
          STYGIAN_EDITOR_CONSTRAINT_V_TOP_BOTTOM)) {
    return false;
  }

  rect.x = 12.0f;
  rect.y = 10.0f;
  rect.w = 92.0f;
  rect.h = 34.0f;
  rect.radius[0] = 8.0f;
  rect.radius[1] = 8.0f;
  rect.radius[2] = 8.0f;
  rect.radius[3] = 8.0f;
  rect.fill = stygian_editor_color_rgba(0.20f, 0.52f, 0.95f, 1.0f);
  rect.visible = true;
  rect.z = 2.0f;
  toggle_id = stygian_editor_add_rect(editor, &rect);
  if (!toggle_id || !stygian_editor_reparent_node(editor, toggle_id, topbar_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, toggle_id, "sidebar_toggle_button");

  rect.x = 24.0f;
  rect.y = 24.0f;
  rect.w = 420.0f;
  rect.h = 240.0f;
  rect.radius[0] = 14.0f;
  rect.radius[1] = 14.0f;
  rect.radius[2] = 14.0f;
  rect.radius[3] = 14.0f;
  rect.fill = stygian_editor_color_rgba(0.12f, 0.16f, 0.24f, 1.0f);
  rect.visible = true;
  rect.z = 2.0f;
  card_id = stygian_editor_add_rect(editor, &rect);
  if (!card_id || !stygian_editor_reparent_node(editor, card_id, content_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, card_id, "main_card");

  rule.trigger_node = toggle_id;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
  rule.action_kind = STYGIAN_EDITOR_ACTION_ANIMATE;
  rule.animate.target_node = sidebar_id;
  rule.animate.property = STYGIAN_EDITOR_PROP_X;
  rule.animate.from_value = 0.0f;
  rule.animate.to_value = -220.0f;
  rule.animate.duration_ms = 250u;
  rule.animate.easing = STYGIAN_EDITOR_EASING_OUT_CUBIC;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return false;

  memset(&track, 0, sizeof(track));
  track.target_node = card_id;
  track.property = STYGIAN_EDITOR_PROP_OPACITY;
  track.layer = 1u;
  snprintf(track.name, sizeof(track.name), "card_pulse");
  if (!stygian_editor_timeline_add_track(editor, &track, &track_id))
    return false;

  keys[0].time_ms = 0u;
  keys[0].value = 0.65f;
  keys[0].easing = STYGIAN_EDITOR_EASING_LINEAR;
  keys[1].time_ms = 320u;
  keys[1].value = 1.0f;
  keys[1].easing = STYGIAN_EDITOR_EASING_OUT_CUBIC;
  if (!stygian_editor_timeline_set_track_keyframes(editor, track_id, keys, 2u))
    return false;

  memset(&clip, 0, sizeof(clip));
  snprintf(clip.name, sizeof(clip.name), "boot");
  clip.start_ms = 0u;
  clip.duration_ms = 320u;
  clip.layer = 1u;
  if (!stygian_editor_timeline_add_clip(editor, &clip, NULL))
    return false;

  ids->primary_edit_id = card_id;
  ids->secondary_edit_id = toggle_id;
  return true;
}

static bool seed_tabs_settings(StygianEditor *editor, FixtureIds *ids) {
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorBehaviorRule rule = {0};
  StygianEditorNodeId root_id = 0u;
  StygianEditorNodeId strip_id = 0u;
  StygianEditorNodeId tab_a_id = 0u;
  StygianEditorNodeId tab_b_id = 0u;
  StygianEditorNodeId panel_a_id = 0u;
  StygianEditorNodeId panel_b_id = 0u;

  if (!editor || !ids)
    return false;

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 1024.0f;
  frame.h = 720.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.08f, 0.09f, 0.12f, 1.0f);
  frame.visible = true;
  frame.z = 0.0f;
  root_id = stygian_editor_add_frame(editor, &frame);
  if (!root_id)
    return false;
  (void)stygian_editor_node_set_name(editor, root_id, "settings_root");

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 1024.0f;
  frame.h = 56.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.16f, 0.17f, 0.20f, 1.0f);
  frame.z = 1.0f;
  strip_id = stygian_editor_add_frame(editor, &frame);
  if (!strip_id ||
      !stygian_editor_reparent_node(editor, strip_id, root_id, false)) {
    return false;
  }
  (void)stygian_editor_node_set_name(editor, strip_id, "tab_strip");

  rect.x = 8.0f;
  rect.y = 10.0f;
  rect.w = 120.0f;
  rect.h = 36.0f;
  rect.radius[0] = 8.0f;
  rect.radius[1] = 8.0f;
  rect.radius[2] = 8.0f;
  rect.radius[3] = 8.0f;
  rect.fill = stygian_editor_color_rgba(0.20f, 0.52f, 0.95f, 1.0f);
  rect.visible = true;
  rect.z = 2.0f;
  tab_a_id = stygian_editor_add_rect(editor, &rect);
  if (!tab_a_id || !stygian_editor_reparent_node(editor, tab_a_id, strip_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, tab_a_id, "tab_general");

  rect.x = 136.0f;
  rect.fill = stygian_editor_color_rgba(0.30f, 0.33f, 0.39f, 1.0f);
  tab_b_id = stygian_editor_add_rect(editor, &rect);
  if (!tab_b_id || !stygian_editor_reparent_node(editor, tab_b_id, strip_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, tab_b_id, "tab_advanced");

  rect.x = 16.0f;
  rect.y = 76.0f;
  rect.w = 992.0f;
  rect.h = 628.0f;
  rect.radius[0] = 10.0f;
  rect.radius[1] = 10.0f;
  rect.radius[2] = 10.0f;
  rect.radius[3] = 10.0f;
  rect.fill = stygian_editor_color_rgba(0.12f, 0.14f, 0.19f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;
  panel_a_id = stygian_editor_add_rect(editor, &rect);
  if (!panel_a_id || !stygian_editor_reparent_node(editor, panel_a_id, root_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, panel_a_id, "panel_general");

  rect.fill = stygian_editor_color_rgba(0.11f, 0.13f, 0.16f, 1.0f);
  panel_b_id = stygian_editor_add_rect(editor, &rect);
  if (!panel_b_id || !stygian_editor_reparent_node(editor, panel_b_id, root_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, panel_b_id, "panel_advanced");
  if (!stygian_editor_node_set_visible(editor, panel_b_id, false))
    return false;

  rule.trigger_node = tab_a_id;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
  rule.action_kind = STYGIAN_EDITOR_ACTION_SET_PROPERTY;
  rule.set_property.target_node = panel_a_id;
  rule.set_property.property = STYGIAN_EDITOR_PROP_VISIBLE;
  rule.set_property.value = 1.0f;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return false;
  rule.set_property.target_node = panel_b_id;
  rule.set_property.value = 0.0f;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return false;

  rule.trigger_node = tab_b_id;
  rule.set_property.target_node = panel_a_id;
  rule.set_property.value = 0.0f;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return false;
  rule.set_property.target_node = panel_b_id;
  rule.set_property.value = 1.0f;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return false;

  ids->primary_edit_id = panel_a_id;
  ids->secondary_edit_id = tab_b_id;
  return true;
}

static bool seed_modal_flow(StygianEditor *editor, FixtureIds *ids) {
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorBehaviorRule rule = {0};
  StygianEditorNodeId root_id = 0u;
  StygianEditorNodeId open_btn_id = 0u;
  StygianEditorNodeId backdrop_id = 0u;
  StygianEditorNodeId modal_id = 0u;
  StygianEditorNodeId close_btn_id = 0u;

  if (!editor || !ids)
    return false;

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 1280.0f;
  frame.h = 720.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.07f, 0.08f, 0.11f, 1.0f);
  frame.visible = true;
  frame.z = 0.0f;
  root_id = stygian_editor_add_frame(editor, &frame);
  if (!root_id)
    return false;
  (void)stygian_editor_node_set_name(editor, root_id, "modal_demo_root");

  rect.x = 24.0f;
  rect.y = 24.0f;
  rect.w = 170.0f;
  rect.h = 44.0f;
  rect.radius[0] = 8.0f;
  rect.radius[1] = 8.0f;
  rect.radius[2] = 8.0f;
  rect.radius[3] = 8.0f;
  rect.fill = stygian_editor_color_rgba(0.20f, 0.52f, 0.95f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;
  open_btn_id = stygian_editor_add_rect(editor, &rect);
  if (!open_btn_id || !stygian_editor_reparent_node(editor, open_btn_id, root_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, open_btn_id, "open_modal_button");

  rect.x = 0.0f;
  rect.y = 0.0f;
  rect.w = 1280.0f;
  rect.h = 720.0f;
  rect.radius[0] = 0.0f;
  rect.radius[1] = 0.0f;
  rect.radius[2] = 0.0f;
  rect.radius[3] = 0.0f;
  rect.fill = stygian_editor_color_rgba(0.0f, 0.0f, 0.0f, 0.55f);
  rect.visible = false;
  rect.z = 2.0f;
  backdrop_id = stygian_editor_add_rect(editor, &rect);
  if (!backdrop_id ||
      !stygian_editor_reparent_node(editor, backdrop_id, root_id, false)) {
    return false;
  }
  (void)stygian_editor_node_set_name(editor, backdrop_id, "modal_backdrop");

  rect.x = 380.0f;
  rect.y = 220.0f;
  rect.w = 520.0f;
  rect.h = 280.0f;
  rect.radius[0] = 16.0f;
  rect.radius[1] = 16.0f;
  rect.radius[2] = 16.0f;
  rect.radius[3] = 16.0f;
  rect.fill = stygian_editor_color_rgba(0.14f, 0.16f, 0.22f, 1.0f);
  rect.visible = false;
  rect.z = 3.0f;
  modal_id = stygian_editor_add_rect(editor, &rect);
  if (!modal_id || !stygian_editor_reparent_node(editor, modal_id, root_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, modal_id, "modal_card");

  rect.x = 864.0f;
  rect.y = 234.0f;
  rect.w = 24.0f;
  rect.h = 24.0f;
  rect.radius[0] = 4.0f;
  rect.radius[1] = 4.0f;
  rect.radius[2] = 4.0f;
  rect.radius[3] = 4.0f;
  rect.fill = stygian_editor_color_rgba(0.72f, 0.24f, 0.24f, 1.0f);
  rect.visible = false;
  rect.z = 4.0f;
  close_btn_id = stygian_editor_add_rect(editor, &rect);
  if (!close_btn_id ||
      !stygian_editor_reparent_node(editor, close_btn_id, root_id, false)) {
    return false;
  }
  (void)stygian_editor_node_set_name(editor, close_btn_id, "close_modal_button");

  rule.trigger_node = open_btn_id;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_PRESS;
  rule.action_kind = STYGIAN_EDITOR_ACTION_SET_PROPERTY;
  rule.set_property.target_node = backdrop_id;
  rule.set_property.property = STYGIAN_EDITOR_PROP_VISIBLE;
  rule.set_property.value = 1.0f;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return false;

  rule.set_property.target_node = modal_id;
  rule.set_property.value = 1.0f;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return false;

  rule.set_property.target_node = close_btn_id;
  rule.set_property.value = 1.0f;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return false;

  rule.action_kind = STYGIAN_EDITOR_ACTION_ANIMATE;
  rule.animate.target_node = modal_id;
  rule.animate.property = STYGIAN_EDITOR_PROP_Y;
  rule.animate.from_value = 260.0f;
  rule.animate.to_value = 220.0f;
  rule.animate.duration_ms = 220u;
  rule.animate.easing = STYGIAN_EDITOR_EASING_OUT_CUBIC;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return false;

  rule.trigger_node = close_btn_id;
  rule.action_kind = STYGIAN_EDITOR_ACTION_SET_PROPERTY;
  rule.set_property.target_node = backdrop_id;
  rule.set_property.property = STYGIAN_EDITOR_PROP_VISIBLE;
  rule.set_property.value = 0.0f;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return false;
  rule.set_property.target_node = modal_id;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return false;
  rule.set_property.target_node = close_btn_id;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return false;

  ids->primary_edit_id = modal_id;
  ids->secondary_edit_id = open_btn_id;
  return true;
}

static int run_case(const char *case_name,
                    bool (*seed_fn)(StygianEditor *, FixtureIds *),
                    uint32_t min_behaviors, uint32_t min_tracks) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditor *c = NULL;
  FixtureIds ids = {0};
  char fixture_path[512];
  char edited_path[512];
  char *json_a = NULL;
  char *json_b = NULL;
  char *json_c = NULL;
  char *code_a = NULL;
  char *code_b = NULL;
  char *code_c = NULL;
  size_t buf_cap = 262144u;
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
  uint32_t diff_lines = 0u;
  int rc = 0;

  cfg.max_nodes = 2048u;
  cfg.max_path_points = 16384u;
  cfg.max_behaviors = 4096u;
  cfg.max_color_tokens = 256u;
  cfg.max_timeline_tracks = 256u;
  cfg.max_timeline_clips = 256u;

  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  c = stygian_editor_create(&cfg, NULL);
  if (!a || !b || !c) {
    rc = fail_case(case_name, "editor create failed");
    goto cleanup;
  }
  json_a = (char *)malloc(buf_cap);
  json_b = (char *)malloc(buf_cap);
  json_c = (char *)malloc(buf_cap);
  code_a = (char *)malloc(buf_cap);
  code_b = (char *)malloc(buf_cap);
  code_c = (char *)malloc(buf_cap);
  if (!json_a || !json_b || !json_c || !code_a || !code_b || !code_c) {
    rc = fail_case(case_name, "buffer allocation failed");
    goto cleanup;
  }

  if (!seed_fn(a, &ids)) {
    rc = fail_case(case_name, "fixture seed failed");
    goto cleanup;
  }

  if (stygian_editor_behavior_count(a) < min_behaviors) {
    rc = fail_case(case_name, "behavior count below fixture floor");
    goto cleanup;
  }
  if (stygian_editor_timeline_track_count(a) < min_tracks) {
    rc = fail_case(case_name, "timeline track count below fixture floor");
    goto cleanup;
  }

  if (!build_json(a, json_a, buf_cap) || !build_c23(a, code_a, buf_cap)) {
    rc = fail_case(case_name, "baseline build failed");
    goto cleanup;
  }
  if (!has_symbol(code_a, "stygian_editor_generated_build_scene")) {
    rc = fail_case(case_name, "missing generated scene symbol");
    goto cleanup;
  }

  snprintf(fixture_path, sizeof(fixture_path), "editor/fixtures/self_host_daily_%s.project.json",
           case_name);
  if (!stygian_editor_save_project_file(a, fixture_path)) {
    rc = fail_case(case_name, "save fixture project failed");
    goto cleanup;
  }

  if (!stygian_editor_load_project_file(b, fixture_path)) {
    rc = fail_case(case_name, "reload fixture project failed");
    goto cleanup;
  }
  if (!build_json(b, json_b, buf_cap) || !build_c23(b, code_b, buf_cap)) {
    rc = fail_case(case_name, "reload build failed");
    goto cleanup;
  }
  if (strcmp(json_a, json_b) != 0) {
    rc = fail_case(case_name, "json mismatch after file reload");
    goto cleanup;
  }
  if (strcmp(code_a, code_b) != 0) {
    rc = fail_case(case_name, "c23 mismatch after file reload");
    goto cleanup;
  }

  if (!stygian_editor_node_get_bounds(b, ids.primary_edit_id, &x, &y, &w, &h)) {
    rc = fail_case(case_name, "failed to read primary node bounds");
    goto cleanup;
  }
  if (!stygian_editor_node_set_position(b, ids.primary_edit_id, x + 1.0f, y, false)) {
    rc = fail_case(case_name, "tiny edit failed");
    goto cleanup;
  }

  if (!build_c23(b, code_c, buf_cap) || !build_json(b, json_c, buf_cap)) {
    rc = fail_case(case_name, "post-edit build failed");
    goto cleanup;
  }
  diff_lines = changed_line_count(code_b, code_c);
  if (diff_lines == 0u || diff_lines > 48u) {
    rc = fail_case(case_name, "tiny edit diff budget exceeded");
    goto cleanup;
  }

  snprintf(edited_path, sizeof(edited_path),
           "editor/build/windows/self_host_daily_%s.edited.project.json", case_name);
  if (!stygian_editor_save_project_file(b, edited_path)) {
    rc = fail_case(case_name, "save edited project failed");
    goto cleanup;
  }

  if (!stygian_editor_load_project_file(c, edited_path)) {
    rc = fail_case(case_name, "reload edited project failed");
    goto cleanup;
  }
  if (!build_json(c, json_b, buf_cap)) {
    rc = fail_case(case_name, "build json from edited reload failed");
    goto cleanup;
  }
  if (strcmp(json_c, json_b) != 0) {
    rc = fail_case(case_name, "edited save/load roundtrip mismatch");
    goto cleanup;
  }

cleanup:
  free(json_a);
  free(json_b);
  free(json_c);
  free(code_a);
  free(code_b);
  free(code_c);
  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  stygian_editor_destroy(c);
  return rc;
}

int main(void) {
  int rc = 0;
  rc = run_case("dashboard_shell", seed_dashboard_shell, 1u, 1u);
  if (rc != 0)
    return rc;
  rc = run_case("tabs_settings", seed_tabs_settings, 4u, 0u);
  if (rc != 0)
    return rc;
  rc = run_case("modal_flow", seed_modal_flow, 6u, 0u);
  if (rc != 0)
    return rc;
  printf("[PASS] editor self-host daily flow smoke\n");
  return 0;
}
