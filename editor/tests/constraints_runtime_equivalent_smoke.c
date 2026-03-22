#include "../include/stygian_editor.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct ParsedConstraintRecord {
  uint32_t parent_id;
  uint32_t child_id;
  uint32_t h_mode;
  uint32_t v_mode;
  float left;
  float right;
  float top;
  float bottom;
  float center_dx;
  float center_dy;
  float x_ratio;
  float y_ratio;
  float w_ratio;
  float h_ratio;
  float base_w;
  float base_h;
} ParsedConstraintRecord;

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool nearf(float a, float b) { return fabsf(a - b) <= 0.05f; }

static bool parse_constraint_records(const char *code, ParsedConstraintRecord *out,
                                     uint32_t max_out, uint32_t *out_count) {
  const char *table = strstr(code, "kStygianEditorGeneratedConstraints");
  const char *scan;
  uint32_t count = 0u;
  if (!table || !out || !out_count || max_out == 0u)
    return false;
  scan = strstr(table, "{");
  if (!scan)
    return false;
  while (scan && *scan && count < max_out) {
    ParsedConstraintRecord r = {0};
    int consumed = 0;
    int n = sscanf(scan,
                   " { %uu, %uu, %uu, %uu, %f, %f, %f, %f, %f, %f, %f, %f, %f, "
                   "%f, %f, %f },%n",
                   &r.parent_id, &r.child_id, &r.h_mode, &r.v_mode, &r.left,
                   &r.right, &r.top, &r.bottom, &r.center_dx, &r.center_dy,
                   &r.x_ratio, &r.y_ratio, &r.w_ratio, &r.h_ratio, &r.base_w,
                   &r.base_h, &consumed);
    if (n == 16) {
      out[count++] = r;
      scan += consumed;
    } else {
      scan = strstr(scan + 1, "{");
      if (!scan)
        break;
      if (strstr(scan, "};") == scan)
        break;
    }
    if (strstr(scan, "};") == scan)
      break;
  }
  *out_count = count;
  return count > 0u;
}

static void solve_record(const ParsedConstraintRecord *c, float parent_w,
                         float parent_h, float *out_x, float *out_y, float *out_w,
                         float *out_h) {
  float x = 0.0f, y = 0.0f, w = c->base_w, h = c->base_h;
  switch (c->h_mode) {
  case 1u:
    x = parent_w - c->right - c->base_w;
    break;
  case 2u:
    x = c->left;
    w = parent_w - c->left - c->right;
    break;
  case 3u:
    x = (parent_w - c->base_w) * 0.5f + c->center_dx;
    break;
  case 4u:
    x = parent_w * c->x_ratio;
    w = parent_w * c->w_ratio;
    break;
  default:
    x = c->left;
    break;
  }
  if (w < 0.0f)
    w = 0.0f;

  switch (c->v_mode) {
  case 1u:
    y = parent_h - c->bottom - c->base_h;
    break;
  case 2u:
    y = c->top;
    h = parent_h - c->top - c->bottom;
    break;
  case 3u:
    y = (parent_h - c->base_h) * 0.5f + c->center_dy;
    break;
  case 4u:
    y = parent_h * c->y_ratio;
    h = parent_h * c->h_ratio;
    break;
  default:
    y = c->top;
    break;
  }
  if (h < 0.0f)
    h = 0.0f;

  *out_x = x;
  *out_y = y;
  *out_w = w;
  *out_h = h;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorNodeId frame_id = 0u;
  StygianEditorNodeId right_id = 0u;
  StygianEditorNodeId stretch_id = 0u;
  StygianEditorNodeId center_id = 0u;
  StygianEditorNodeId scale_id = 0u;
  StygianEditorNodeId left_id = 0u;
  StygianEditorNodeId top_bottom_id = 0u;
  ParsedConstraintRecord records[64];
  uint32_t record_count = 0u;
  uint32_t i;
  char code[262144];
  size_t need = 0u;
  float ex_x[6], ex_y[6], ex_w[6], ex_h[6];
  StygianEditorNodeId ids[6];

  cfg.max_nodes = 256u;
  cfg.max_path_points = 4096u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 16u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("editor create failed");

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 200.0f;
  frame.h = 100.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.1f, 0.1f, 0.1f, 1.0f);
  frame.visible = true;
  frame.z = 0.0f;
  frame_id = stygian_editor_add_frame(editor, &frame);
  if (!frame_id)
    return fail("add frame failed");

  rect.w = 20.0f;
  rect.h = 10.0f;
  rect.fill = stygian_editor_color_rgba(1.0f, 1.0f, 1.0f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;

  rect.x = 170.0f;
  rect.y = 80.0f;
  right_id = stygian_editor_add_rect(editor, &rect);
  rect.x = 10.0f;
  rect.y = 30.0f;
  rect.w = 180.0f;
  rect.h = 20.0f;
  stretch_id = stygian_editor_add_rect(editor, &rect);
  rect.x = 75.0f;
  rect.y = 35.0f;
  rect.w = 50.0f;
  rect.h = 30.0f;
  center_id = stygian_editor_add_rect(editor, &rect);
  rect.x = 20.0f;
  rect.y = 10.0f;
  rect.w = 40.0f;
  rect.h = 20.0f;
  scale_id = stygian_editor_add_rect(editor, &rect);
  rect.x = 12.0f;
  rect.y = 22.0f;
  rect.w = 30.0f;
  rect.h = 15.0f;
  left_id = stygian_editor_add_rect(editor, &rect);
  rect.x = 150.0f;
  rect.y = 10.0f;
  rect.w = 20.0f;
  rect.h = 80.0f;
  top_bottom_id = stygian_editor_add_rect(editor, &rect);
  if (!right_id || !stretch_id || !center_id || !scale_id || !left_id ||
      !top_bottom_id)
    return fail("add children failed");

  if (!stygian_editor_reparent_node(editor, right_id, frame_id, false) ||
      !stygian_editor_reparent_node(editor, stretch_id, frame_id, false) ||
      !stygian_editor_reparent_node(editor, center_id, frame_id, false) ||
      !stygian_editor_reparent_node(editor, scale_id, frame_id, false) ||
      !stygian_editor_reparent_node(editor, left_id, frame_id, false) ||
      !stygian_editor_reparent_node(editor, top_bottom_id, frame_id, false)) {
    return fail("reparent failed");
  }

  if (!stygian_editor_node_set_constraints(
          editor, right_id, STYGIAN_EDITOR_CONSTRAINT_H_RIGHT,
          STYGIAN_EDITOR_CONSTRAINT_V_BOTTOM) ||
      !stygian_editor_node_set_constraints(
          editor, stretch_id, STYGIAN_EDITOR_CONSTRAINT_H_LEFT_RIGHT,
          STYGIAN_EDITOR_CONSTRAINT_V_TOP) ||
      !stygian_editor_node_set_constraints(
          editor, center_id, STYGIAN_EDITOR_CONSTRAINT_H_CENTER,
          STYGIAN_EDITOR_CONSTRAINT_V_CENTER) ||
      !stygian_editor_node_set_constraints(
          editor, scale_id, STYGIAN_EDITOR_CONSTRAINT_H_SCALE,
          STYGIAN_EDITOR_CONSTRAINT_V_SCALE) ||
      !stygian_editor_node_set_constraints(
          editor, left_id, STYGIAN_EDITOR_CONSTRAINT_H_LEFT,
          STYGIAN_EDITOR_CONSTRAINT_V_TOP) ||
      !stygian_editor_node_set_constraints(
          editor, top_bottom_id, STYGIAN_EDITOR_CONSTRAINT_H_RIGHT,
          STYGIAN_EDITOR_CONSTRAINT_V_TOP_BOTTOM)) {
    return fail("set constraints failed");
  }

  if (!stygian_editor_resize_frame(editor, frame_id, 400.0f, 200.0f))
    return fail("resize frame failed");

  ids[0] = right_id;
  ids[1] = stretch_id;
  ids[2] = center_id;
  ids[3] = scale_id;
  ids[4] = left_id;
  ids[5] = top_bottom_id;
  for (i = 0u; i < 6u; ++i) {
    if (!stygian_editor_node_get_bounds(editor, ids[i], &ex_x[i], &ex_y[i], &ex_w[i],
                                        &ex_h[i])) {
      return fail("expected bounds fetch failed");
    }
  }

  need = stygian_editor_build_c23(editor, NULL, 0u);
  if (need == 0u || need > sizeof(code))
    return fail("build_c23 size failed");
  if (stygian_editor_build_c23(editor, code, sizeof(code)) != need)
    return fail("build_c23 failed");
  if (!strstr(code, "stygian_editor_generated_apply_constraints_for_parent"))
    return fail("runtime-equivalent solver symbol missing");
  if (!parse_constraint_records(code, records, 64u, &record_count))
    return fail("failed to parse exported constraint records");

  for (i = 0u; i < record_count; ++i) {
    const ParsedConstraintRecord *r = &records[i];
    float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
    uint32_t idx = 0u;
    bool tracked = false;
    if (r->parent_id != frame_id)
      continue;
    solve_record(r, 400.0f, 200.0f, &x, &y, &w, &h);
    for (idx = 0u; idx < 6u; ++idx) {
      if (ids[idx] == r->child_id) {
        tracked = true;
        break;
      }
    }
    if (!tracked)
      continue;
    if (!nearf(x, ex_x[idx]) || !nearf(y, ex_y[idx]) || !nearf(w, ex_w[idx]) ||
        !nearf(h, ex_h[idx])) {
      return fail("runtime-equivalent solver mismatch vs editor resize result");
    }
  }

  stygian_editor_destroy(editor);
  printf("[PASS] editor constraints runtime equivalent smoke\n");
  return 0;
}
