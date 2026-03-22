#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorTimelineTrack track = {0};
  StygianEditorTimelineClip clip = {0};
  StygianEditorTimelineKeyframe keyframes[3];
  StygianEditorTimelineTrackId t1 = 0u;
  StygianEditorTimelineTrackId t2 = 0u;
  StygianEditorTimelineTrackId ids[2];
  StygianEditorTimelineTrack out_track = {0};
  StygianEditorTimelineClip out_clip = {0};
  StygianEditorNodeId n1 = 0u;
  StygianEditorNodeId n2 = 0u;
  size_t need = 0u;
  char json[262144];

  cfg.max_nodes = 128u;
  cfg.max_behaviors = 64u;
  cfg.max_timeline_tracks = 32u;
  cfg.max_timeline_clips = 16u;

  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");

  rect.x = 10.0f;
  rect.y = 10.0f;
  rect.w = 100.0f;
  rect.h = 36.0f;
  rect.visible = true;
  rect.fill = stygian_editor_color_rgba(0.2f, 0.3f, 0.4f, 1.0f);
  n1 = stygian_editor_add_rect(a, &rect);
  rect.x = 180.0f;
  n2 = stygian_editor_add_rect(a, &rect);
  if (!n1 || !n2)
    return fail("seed nodes failed");

  memset(&track, 0, sizeof(track));
  track.target_node = n1;
  track.property = STYGIAN_EDITOR_PROP_X;
  track.layer = 0u;
  snprintf(track.name, sizeof(track.name), "move_x");
  if (!stygian_editor_timeline_add_track(a, &track, &t1) || !t1)
    return fail("timeline track add #1 failed");

  keyframes[0].time_ms = 240u;
  keyframes[0].value = 320.0f;
  keyframes[0].easing = STYGIAN_EDITOR_EASING_OUT_CUBIC;
  keyframes[1].time_ms = 0u;
  keyframes[1].value = 10.0f;
  keyframes[1].easing = STYGIAN_EDITOR_EASING_LINEAR;
  keyframes[2].time_ms = 120u;
  keyframes[2].value = 180.0f;
  keyframes[2].easing = STYGIAN_EDITOR_EASING_IN_OUT_CUBIC;
  if (!stygian_editor_timeline_set_track_keyframes(a, t1, keyframes, 3u))
    return fail("set keyframes #1 failed");

  memset(&track, 0, sizeof(track));
  track.target_node = n2;
  track.property = STYGIAN_EDITOR_PROP_OPACITY;
  track.layer = 1u;
  snprintf(track.name, sizeof(track.name), "fade");
  if (!stygian_editor_timeline_add_track(a, &track, &t2) || !t2)
    return fail("timeline track add #2 failed");

  keyframes[0].time_ms = 0u;
  keyframes[0].value = 1.0f;
  keyframes[0].easing = STYGIAN_EDITOR_EASING_LINEAR;
  keyframes[1].time_ms = 180u;
  keyframes[1].value = 0.25f;
  keyframes[1].easing = STYGIAN_EDITOR_EASING_OUT_CUBIC;
  if (!stygian_editor_timeline_set_track_keyframes(a, t2, keyframes, 2u))
    return fail("set keyframes #2 failed");

  memset(&clip, 0, sizeof(clip));
  snprintf(clip.name, sizeof(clip.name), "hover_transition");
  clip.start_ms = 0u;
  clip.duration_ms = 240u;
  clip.layer = 3u;
  if (!stygian_editor_timeline_add_clip(a, &clip, &clip.id) || !clip.id)
    return fail("timeline clip add failed");
  ids[0] = t1;
  ids[1] = t2;
  if (!stygian_editor_timeline_set_clip_tracks(a, clip.id, ids, 2u))
    return fail("timeline clip track assign failed");

  if (stygian_editor_timeline_track_count(a) != 2u)
    return fail("timeline track count mismatch");
  if (stygian_editor_timeline_clip_count(a) != 1u)
    return fail("timeline clip count mismatch");

  if (!stygian_editor_timeline_get_track(a, 0u, &out_track))
    return fail("timeline get track failed");
  if (out_track.keyframe_count != 3u)
    return fail("timeline keyframe count mismatch");
  if (out_track.keyframes[0].time_ms != 0u || out_track.keyframes[1].time_ms != 120u ||
      out_track.keyframes[2].time_ms != 240u) {
    return fail("timeline keyframes not sorted");
  }

  if (!stygian_editor_timeline_get_clip(a, 0u, &out_clip))
    return fail("timeline get clip failed");
  if (out_clip.track_count != 2u)
    return fail("timeline clip track_count mismatch");

  need = stygian_editor_build_project_json(a, NULL, 0u);
  if (need < 2u || need > sizeof(json))
    return fail("build project json size failed");
  if (stygian_editor_build_project_json(a, json, sizeof(json)) != need)
    return fail("build project json failed");
  if (!strstr(json, "\"timeline_tracks\": ["))
    return fail("timeline_tracks missing from json");
  if (!strstr(json, "\"timeline_clips\": ["))
    return fail("timeline_clips missing from json");

  if (!stygian_editor_load_project_json(b, json))
    return fail("load project json with timeline failed");
  if (stygian_editor_timeline_track_count(b) != 2u)
    return fail("loaded timeline track count mismatch");
  if (stygian_editor_timeline_clip_count(b) != 1u)
    return fail("loaded timeline clip count mismatch");

  if (!stygian_editor_delete_node(b, n1))
    return fail("delete node for timeline cleanup failed");
  if (stygian_editor_timeline_track_count(b) != 1u)
    return fail("timeline track cleanup on node delete failed");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor timeline multitrack smoke\n");
  return 0;
}
