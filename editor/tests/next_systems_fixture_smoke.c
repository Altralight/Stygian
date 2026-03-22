#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct FixtureCase {
  const char *name;
  bool (*seed_fn)(StygianEditor *editor);
  uint32_t min_behaviors;
  uint32_t min_tracks;
  uint32_t min_drivers;
  const char *json_needles[8];
  const char *code_needles[8];
} FixtureCase;

static int fail_case(const char *case_name, const char *msg) {
  fprintf(stderr, "[FAIL] %s: %s\n", case_name, msg);
  return 1;
}

static bool contains(const char *text, const char *needle) {
  return text && needle && needle[0] && strstr(text, needle) != NULL;
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

static bool assert_needles(const char *text, const char *const *needles,
                           const char *case_name, const char *label) {
  uint32_t i;
  for (i = 0u; i < 8u; ++i) {
    if (!needles[i])
      break;
    if (!contains(text, needles[i])) {
      fprintf(stderr, "[FAIL] %s: missing %s signature `%s`\n", case_name,
              label, needles[i]);
      return false;
    }
  }
  return true;
}

static bool seed_motion_layout_driver(StygianEditor *editor) {
  StygianEditorFrameDesc root = {0};
  StygianEditorRectDesc card = {0};
  StygianEditorNodeLayoutOptions opts = {0};
  StygianEditorFrameAutoLayout layout = {0};
  StygianEditorTimelineTrack track = {0};
  StygianEditorTimelineTrackId track_id = 0u;
  StygianEditorTimelineClip clip = {0};
  StygianEditorTimelineKeyframe keys[2];
  StygianEditorBehaviorRule rule = {0};
  StygianEditorDriverRule driver = {0};
  StygianEditorNodeEffect effect = {0};
  StygianEditorShaderAttachment shader = {0};
  StygianEditorNodeId root_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId slider_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId card_id = STYGIAN_EDITOR_INVALID_ID;
  StygianEditorNodeId sidebar_id = STYGIAN_EDITOR_INVALID_ID;

  if (!editor)
    return false;

  root.w = 960.0f;
  root.h = 640.0f;
  root.fill = stygian_editor_color_rgba(0.07f, 0.08f, 0.11f, 1.0f);
  root.visible = true;
  root_id = stygian_editor_add_frame(editor, &root);
  if (!root_id)
    return false;
  (void)stygian_editor_node_set_name(editor, root_id, "motion_layout_root");

  layout.mode = STYGIAN_EDITOR_AUTO_LAYOUT_HORIZONTAL;
  layout.wrap = STYGIAN_EDITOR_AUTO_LAYOUT_WRAP;
  layout.padding_left = 20.0f;
  layout.padding_right = 20.0f;
  layout.padding_top = 20.0f;
  layout.padding_bottom = 20.0f;
  layout.gap = 18.0f;
  layout.primary_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  layout.cross_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  if (!stygian_editor_frame_set_auto_layout(editor, root_id, &layout))
    return false;
  if (!stygian_editor_frame_set_overflow_policy(
          editor, root_id, STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_Y)) {
    return false;
  }

  if (!stygian_editor_add_control_recipe(editor, STYGIAN_EDITOR_RECIPE_SLIDER,
                                         20.0f, 24.0f, 260.0f, 24.0f, "Progress",
                                         &slider_id) ||
      !slider_id) {
    return false;
  }
  if (!stygian_editor_reparent_node(editor, slider_id, root_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, slider_id, "progress_slider");

  card.w = 320.0f;
  card.h = 180.0f;
  card.radius[0] = 18.0f;
  card.radius[1] = 18.0f;
  card.radius[2] = 18.0f;
  card.radius[3] = 18.0f;
  card.fill = stygian_editor_color_rgba(0.16f, 0.20f, 0.28f, 1.0f);
  card.visible = true;
  card_id = stygian_editor_add_rect(editor, &card);
  if (!card_id || !stygian_editor_reparent_node(editor, card_id, root_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, card_id, "hero_card");

  card.w = 180.0f;
  card.h = 460.0f;
  card.radius[0] = 14.0f;
  card.radius[1] = 14.0f;
  card.radius[2] = 14.0f;
  card.radius[3] = 14.0f;
  card.fill = stygian_editor_color_rgba(0.10f, 0.12f, 0.16f, 1.0f);
  sidebar_id = stygian_editor_add_rect(editor, &card);
  if (!sidebar_id ||
      !stygian_editor_reparent_node(editor, sidebar_id, root_id, false))
    return false;
  (void)stygian_editor_node_set_name(editor, sidebar_id, "sidebar_panel");

  opts.absolute_position = false;
  opts.sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  opts.sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  if (!stygian_editor_node_set_layout_options(editor, slider_id, &opts) ||
      !stygian_editor_node_set_layout_options(editor, card_id, &opts) ||
      !stygian_editor_node_set_layout_options(editor, sidebar_id, &opts)) {
    return false;
  }

  if (!stygian_editor_set_variable_mode(editor, 0u, "default") ||
      !stygian_editor_set_number_variable(editor, "hero.opacity", 0u, 0.35f) ||
      !stygian_editor_bind_node_number_variable(editor, card_id,
                                                STYGIAN_EDITOR_PROP_OPACITY,
                                                "hero.opacity")) {
    return false;
  }

  driver.source_node = slider_id;
  driver.source_property = STYGIAN_EDITOR_PROP_VALUE;
  snprintf(driver.variable_name, sizeof(driver.variable_name), "%s", "hero.opacity");
  driver.use_active_mode = true;
  driver.in_min = 0.0f;
  driver.in_max = 1.0f;
  driver.out_min = 0.25f;
  driver.out_max = 1.0f;
  driver.clamp_output = true;
  if (!stygian_editor_add_driver(editor, &driver, NULL))
    return false;

  track.target_node = card_id;
  track.property = STYGIAN_EDITOR_PROP_Y;
  track.layer = 1u;
  snprintf(track.name, sizeof(track.name), "%s", "hero_lift");
  if (!stygian_editor_timeline_add_track(editor, &track, &track_id))
    return false;
  keys[0].time_ms = 0u;
  keys[0].value = 40.0f;
  keys[0].easing = STYGIAN_EDITOR_EASING_LINEAR;
  keys[1].time_ms = 420u;
  keys[1].value = 12.0f;
  keys[1].easing = STYGIAN_EDITOR_EASING_OUT_CUBIC;
  if (!stygian_editor_timeline_set_track_keyframes(editor, track_id, keys, 2u))
    return false;
  snprintf(clip.name, sizeof(clip.name), "%s", "intro");
  clip.start_ms = 0u;
  clip.duration_ms = 420u;
  clip.layer = 1u;
  if (!stygian_editor_timeline_add_clip(editor, &clip, NULL))
    return false;

  rule.trigger_node = slider_id;
  rule.trigger_event = STYGIAN_EDITOR_EVENT_VALUE_CHANGED;
  rule.action_kind = STYGIAN_EDITOR_ACTION_ANIMATE;
  rule.animate.target_node = sidebar_id;
  rule.animate.property = STYGIAN_EDITOR_PROP_X;
  rule.animate.from_value = 0.0f;
  rule.animate.to_value = 16.0f;
  rule.animate.duration_ms = 180u;
  rule.animate.easing = STYGIAN_EDITOR_EASING_OUT_CUBIC;
  if (!stygian_editor_add_behavior(editor, &rule, NULL))
    return false;

  effect.kind = STYGIAN_EDITOR_EFFECT_GLOW;
  effect.visible = true;
  effect.opacity = 0.55f;
  effect.radius = 18.0f;
  effect.intensity = 1.0f;
  effect.color = stygian_editor_color_rgba(0.18f, 0.61f, 0.96f, 1.0f);
  if (!stygian_editor_node_set_effects(editor, card_id, &effect, 1u))
    return false;

  shader.enabled = true;
  snprintf(shader.slot_name, sizeof(shader.slot_name), "%s", "surface");
  snprintf(shader.asset_path, sizeof(shader.asset_path), "%s",
           "shaders/fx/hero_card.frag");
  snprintf(shader.entry_point, sizeof(shader.entry_point), "%s", "main");
  if (!stygian_editor_node_set_shader_attachment(editor, card_id, &shader))
    return false;

  return true;
}

static bool seed_component_fixture(StygianEditor *editor) {
  StygianEditorComponentDefDesc def = {0};
  StygianEditorComponentInstanceDesc inst = {0};
  StygianEditorComponentPropertyDef prop = {0};
  StygianEditorComponentPropertyValue ov = {0};
  StygianEditorNodeId d0 = 0u;
  StygianEditorNodeId d1 = 0u;
  StygianEditorNodeId instance_id = 0u;

  if (!editor)
    return false;

  def.x = 40.0f;
  def.y = 40.0f;
  def.w = 180.0f;
  def.h = 52.0f;
  def.symbol = "tab_default";
  def.visible = true;
  d0 = stygian_editor_add_component_def(editor, &def);
  def.symbol = "tab_active";
  d1 = stygian_editor_add_component_def(editor, &def);
  if (!d0 || !d1)
    return false;
  if (!stygian_editor_component_def_set_variant(editor, d0, "tab_variant",
                                                "default") ||
      !stygian_editor_component_def_set_variant(editor, d1, "tab_variant",
                                                "active")) {
    return false;
  }

  snprintf(prop.name, sizeof(prop.name), "%s", "state");
  prop.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
  prop.enum_option_count = 2u;
  snprintf(prop.enum_options[0], sizeof(prop.enum_options[0]), "%s", "default");
  snprintf(prop.enum_options[1], sizeof(prop.enum_options[1]), "%s", "active");
  snprintf(prop.default_enum, sizeof(prop.default_enum), "%s", "default");
  if (!stygian_editor_component_def_set_property(editor, d0, &prop))
    return false;
  snprintf(prop.default_enum, sizeof(prop.default_enum), "%s", "active");
  if (!stygian_editor_component_def_set_property(editor, d1, &prop))
    return false;

  inst.component_def_id = d0;
  inst.x = 260.0f;
  inst.y = 84.0f;
  inst.w = 180.0f;
  inst.h = 52.0f;
  inst.visible = true;
  instance_id = stygian_editor_add_component_instance(editor, &inst);
  if (!instance_id)
    return false;

  snprintf(ov.name, sizeof(ov.name), "%s", "state");
  ov.type = STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM;
  snprintf(ov.enum_value, sizeof(ov.enum_value), "%s", "active");
  if (!stygian_editor_component_instance_set_property_override(editor, instance_id,
                                                               &ov)) {
    return false;
  }
  return true;
}

static bool seed_ownership_fixture(StygianEditor *editor) {
  StygianEditorRectDesc rect = {0};
  StygianEditorPathDesc path = {0};
  StygianEditorNodeEffect effects[3] = {0};
  StygianEditorShaderAttachment shader = {0};
  StygianEditorNodeId rect_id = 0u;
  StygianEditorNodeId path_id = 0u;
  StygianEditorPathId pb = 0u;

  if (!editor)
    return false;

  rect.x = 80.0f;
  rect.y = 96.0f;
  rect.w = 260.0f;
  rect.h = 180.0f;
  rect.radius[0] = 16.0f;
  rect.radius[1] = 16.0f;
  rect.radius[2] = 16.0f;
  rect.radius[3] = 16.0f;
  rect.fill = stygian_editor_color_rgba(0.86f, 0.88f, 0.91f, 1.0f);
  rect.visible = true;
  rect_id = stygian_editor_add_rect(editor, &rect);
  if (!rect_id)
    return false;

  path.stroke = stygian_editor_color_rgba(1.0f, 1.0f, 1.0f, 1.0f);
  path.thickness = 2.0f;
  path.visible = true;
  pb = stygian_editor_path_begin(editor, &path);
  if (!pb)
    return false;
  if (!stygian_editor_path_add_point(editor, pb, 86.0f, 104.0f, false) ||
      !stygian_editor_path_add_point(editor, pb, 316.0f, 118.0f, false) ||
      !stygian_editor_path_add_point(editor, pb, 298.0f, 248.0f, false)) {
    return false;
  }
  path_id = stygian_editor_path_commit(editor, pb);
  if (!path_id)
    return false;

  effects[0].kind = STYGIAN_EDITOR_EFFECT_DROP_SHADOW;
  effects[0].visible = true;
  effects[0].opacity = 0.8f;
  effects[0].radius = 12.0f;
  effects[0].spread = 2.0f;
  effects[0].offset_x = 3.0f;
  effects[0].offset_y = 5.0f;
  effects[0].intensity = 1.0f;
  effects[0].color = stygian_editor_color_rgba(0.0f, 0.0f, 0.0f, 1.0f);
  effects[1].kind = STYGIAN_EDITOR_EFFECT_LAYER_BLUR;
  effects[1].visible = true;
  effects[1].opacity = 1.0f;
  effects[1].radius = 6.0f;
  effects[1].intensity = 1.0f;
  effects[2].kind = STYGIAN_EDITOR_EFFECT_NOISE;
  effects[2].visible = true;
  effects[2].opacity = 0.35f;
  effects[2].radius = 2.0f;
  effects[2].intensity = 0.75f;
  if (!stygian_editor_node_set_effects(editor, rect_id, effects, 3u))
    return false;
  if (!stygian_editor_node_set_mask(editor, rect_id, path_id,
                                    STYGIAN_EDITOR_MASK_ALPHA, false)) {
    return false;
  }

  shader.enabled = true;
  snprintf(shader.slot_name, sizeof(shader.slot_name), "%s", "surface");
  snprintf(shader.asset_path, sizeof(shader.asset_path), "%s",
           "shaders/fx/card_noise.frag");
  snprintf(shader.entry_point, sizeof(shader.entry_point), "%s", "main");
  if (!stygian_editor_node_set_shader_attachment(editor, rect_id, &shader))
    return false;

  return true;
}

static int run_case(const FixtureCase *fc) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditor *c = NULL;
  char *json_a = NULL;
  char *json_b = NULL;
  char *json_c = NULL;
  char *code_a = NULL;
  char *code_b = NULL;
  char fixture_path[512];
  char edited_path[512];
  size_t cap = 262144u;
  int rc = 0;
  StygianEditorNodeId selected = STYGIAN_EDITOR_INVALID_ID;
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;

  cfg.max_nodes = 2048u;
  cfg.max_path_points = 16384u;
  cfg.max_behaviors = 4096u;
  cfg.max_color_tokens = 256u;
  cfg.max_timeline_tracks = 256u;
  cfg.max_timeline_clips = 256u;
  cfg.max_drivers = 256u;

  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  c = stygian_editor_create(&cfg, NULL);
  if (!a || !b || !c) {
    rc = fail_case(fc->name, "editor create failed");
    goto cleanup;
  }

  json_a = (char *)malloc(cap);
  json_b = (char *)malloc(cap);
  json_c = (char *)malloc(cap);
  code_a = (char *)malloc(cap * 2u);
  code_b = (char *)malloc(cap * 2u);
  if (!json_a || !json_b || !json_c || !code_a || !code_b) {
    rc = fail_case(fc->name, "buffer allocation failed");
    goto cleanup;
  }

  if (!fc->seed_fn(a)) {
    rc = fail_case(fc->name, "fixture seed failed");
    goto cleanup;
  }
  if (stygian_editor_behavior_count(a) < fc->min_behaviors) {
    rc = fail_case(fc->name, "behavior count below fixture floor");
    goto cleanup;
  }
  if (stygian_editor_timeline_track_count(a) < fc->min_tracks) {
    rc = fail_case(fc->name, "timeline track count below fixture floor");
    goto cleanup;
  }
  if (stygian_editor_driver_count(a) < fc->min_drivers) {
    rc = fail_case(fc->name, "driver count below fixture floor");
    goto cleanup;
  }
  if (!build_json(a, json_a, cap) || !build_c23(a, code_a, cap * 2u)) {
    rc = fail_case(fc->name, "baseline build failed");
    goto cleanup;
  }
  if (!assert_needles(json_a, fc->json_needles, fc->name, "json")) {
    rc = 1;
    goto cleanup;
  }
  if (!assert_needles(code_a, fc->code_needles, fc->name, "code")) {
    rc = 1;
    goto cleanup;
  }

  snprintf(fixture_path, sizeof(fixture_path),
           "editor/fixtures/next_systems_%s.project.json", fc->name);
  if (!stygian_editor_save_project_file(a, fixture_path)) {
    rc = fail_case(fc->name, "save fixture project failed");
    goto cleanup;
  }
  if (!stygian_editor_load_project_file(b, fixture_path)) {
    rc = fail_case(fc->name, "reload fixture project failed");
    goto cleanup;
  }
  if (!build_json(b, json_b, cap) || !build_c23(b, code_b, cap * 2u)) {
    rc = fail_case(fc->name, "reload build failed");
    goto cleanup;
  }
  if (!assert_needles(json_b, fc->json_needles, fc->name, "reload json") ||
      !assert_needles(code_b, fc->code_needles, fc->name, "reload code")) {
    rc = 1;
    goto cleanup;
  }

  selected = stygian_editor_selected_node(b);
  if (selected == STYGIAN_EDITOR_INVALID_ID)
    selected = 1u;
  if (!stygian_editor_node_get_bounds(b, selected, &x, &y, &w, &h) ||
      !stygian_editor_node_set_position(b, selected, x + 1.0f, y, false)) {
    rc = fail_case(fc->name, "edited save sweep failed");
    goto cleanup;
  }
  snprintf(edited_path, sizeof(edited_path),
           "editor/build/windows/next_systems_%s.edited.project.json", fc->name);
  if (!stygian_editor_save_project_file(b, edited_path)) {
    rc = fail_case(fc->name, "save edited project failed");
    goto cleanup;
  }
  if (!stygian_editor_load_project_file(c, edited_path)) {
    rc = fail_case(fc->name, "reload edited project failed");
    goto cleanup;
  }
  if (!build_json(c, json_c, cap)) {
    rc = fail_case(fc->name, "build edited reload json failed");
    goto cleanup;
  }

cleanup:
  free(json_a);
  free(json_b);
  free(json_c);
  free(code_a);
  free(code_b);
  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  stygian_editor_destroy(c);
  return rc;
}

int main(void) {
  static const FixtureCase cases[] = {
      {"motion_layout_driver",
       seed_motion_layout_driver,
       1u,
       1u,
       1u,
       {"\"drivers\": [", "\"timeline_tracks\": [", NULL},
       {"kStygianEditorGeneratedAutoLayoutFrames",
        "kStygianEditorGeneratedDrivers",
        "stygian_editor_generated_apply_driver_sample",
        "authored shader attachment",
        NULL}},
      {"component_states",
       seed_component_fixture,
       0u,
       0u,
       0u,
       {"cprops=", NULL},
       {"kStygianEditorGeneratedComponentDefs",
        "kStygianEditorGeneratedComponentProperties",
        "kStygianEditorGeneratedComponentInstances",
        NULL}},
      {"ownership_effects",
       seed_ownership_fixture,
       0u,
       0u,
       0u,
       {"effects=", "mask=", "shasset=", NULL},
       {"authored shader attachment", "effect-xform", NULL}},
  };
  uint32_t i;
  for (i = 0u; i < (uint32_t)(sizeof(cases) / sizeof(cases[0])); ++i) {
    int rc = run_case(&cases[i]);
    if (rc != 0)
      return rc;
  }
  printf("[PASS] editor next systems fixture smoke\n");
  return 0;
}
