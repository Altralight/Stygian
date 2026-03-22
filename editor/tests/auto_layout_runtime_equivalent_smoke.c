#include "../include/stygian_editor.h"

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct ParsedAutoLayoutFrame {
  uint32_t node_id;
  uint32_t mode;
  uint32_t wrap;
  float padding_left;
  float padding_right;
  float padding_top;
  float padding_bottom;
  float gap;
  uint32_t primary_align;
  uint32_t cross_align;
  uint32_t overflow_policy;
  float scroll_x;
  float scroll_y;
} ParsedAutoLayoutFrame;

typedef struct ParsedAutoLayoutChild {
  uint32_t parent_id;
  uint32_t child_id;
  uint32_t order_index;
  bool absolute;
  uint32_t sizing_h;
  uint32_t sizing_v;
  float min_w;
  float max_w;
  float min_h;
  float max_h;
  float base_x;
  float base_y;
  float base_w;
  float base_h;
} ParsedAutoLayoutChild;

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool nearf(float a, float b) { return fabsf(a - b) <= 0.05f; }

static float clamp_size(float v, float min_v, float max_v) {
  if (v < min_v)
    v = min_v;
  if (max_v > 0.0f && v > max_v)
    v = max_v;
  if (v < 0.0f)
    v = 0.0f;
  return v;
}

static bool parse_auto_layout_frames(const char *code, ParsedAutoLayoutFrame *out,
                                     uint32_t max_out, uint32_t *out_count) {
  const char *table = strstr(code, "kStygianEditorGeneratedAutoLayoutFrames");
  const char *scan;
  uint32_t count = 0u;
  if (!table || !out || !out_count || max_out == 0u)
    return false;
  scan = strstr(table, "= {");
  if (!scan)
    return false;
  scan = strchr(scan, '{');
  if (!scan)
    return false;
  scan += 1;
  while (scan && *scan && count < max_out) {
    ParsedAutoLayoutFrame f = {0};
    int consumed = 0;
    int n;
    while (*scan && isspace((unsigned char)*scan))
      scan += 1;
    if (*scan == '}')
      break;
    if (*scan != '{') {
      scan += 1;
      continue;
    }
    n = sscanf(scan,
                   "{ %u%*1[u], %u%*1[u], %u%*1[u], %f, %f, %f, %f, %f, %u%*1[u], %u%*1[u], %u%*1[u], %f, %f }%*[, ]%n",
                   &f.node_id, &f.mode, &f.wrap, &f.padding_left, &f.padding_right,
                   &f.padding_top, &f.padding_bottom, &f.gap, &f.primary_align,
                   &f.cross_align, &f.overflow_policy, &f.scroll_x, &f.scroll_y,
                   &consumed);
    if (n == 13) {
      out[count++] = f;
      scan += consumed;
    } else {
      scan += 1;
    }
  }
  *out_count = count;
  return count > 0u;
}

static bool parse_auto_layout_children(const char *code, ParsedAutoLayoutChild *out,
                                       uint32_t max_out, uint32_t *out_count) {
  const char *table = strstr(code, "kStygianEditorGeneratedAutoLayoutChildren");
  const char *scan;
  uint32_t count = 0u;
  if (!table || !out || !out_count || max_out == 0u)
    return false;
  scan = strstr(table, "= {");
  if (!scan)
    return false;
  scan = strchr(scan, '{');
  if (!scan)
    return false;
  scan += 1;
  while (scan && *scan && count < max_out) {
    ParsedAutoLayoutChild c = {0};
    char absolute_text[16];
    int consumed = 0;
    int n;
    while (*scan && isspace((unsigned char)*scan))
      scan += 1;
    if (*scan == '}')
      break;
    if (*scan != '{') {
      scan += 1;
      continue;
    }
    n = sscanf(scan,
                   "{ %u%*1[u], %u%*1[u], %u%*1[u], %15[^,], %u%*1[u], %u%*1[u], %f, %f, %f, %f, %f, %f, "
                   "%f, %f }%*[, ]%n",
                   &c.parent_id, &c.child_id, &c.order_index, absolute_text,
                   &c.sizing_h, &c.sizing_v, &c.min_w, &c.max_w, &c.min_h,
                   &c.max_h, &c.base_x, &c.base_y, &c.base_w, &c.base_h,
                   &consumed);
    if (n == 14) {
      c.absolute = strstr(absolute_text, "true") != NULL;
      out[count++] = c;
      scan += consumed;
    } else {
      scan += 1;
    }
  }
  *out_count = count;
  return count > 0u;
}

static uint32_t solve_auto_layout(const ParsedAutoLayoutFrame *frame,
                                  const ParsedAutoLayoutChild *children,
                                  uint32_t child_count, float parent_w,
                                  float parent_h, uint32_t *out_child_ids,
                                  float *out_x, float *out_y, float *out_w,
                                  float *out_h, uint32_t max_out) {
  uint32_t i;
  uint32_t resolved = 0u;
  uint32_t flow_count = 0u;
  uint32_t fill_count = 0u;
  float fixed_primary = 0.0f;
  float inner_w;
  float inner_h;
  float gap;
  float fill_each = 0.0f;
  float total_primary = 0.0f;
  float cursor;
  float scroll = 0.0f;
  bool horizontal;
  if (!frame || !children || !out_child_ids || !out_x || !out_y || !out_w ||
      !out_h || max_out == 0u)
    return 0u;
  if (frame->mode == 0u)
    return 0u;
  horizontal = frame->mode == 1u;
  inner_w = parent_w - frame->padding_left - frame->padding_right;
  inner_h = parent_h - frame->padding_top - frame->padding_bottom;
  if (inner_w < 0.0f)
    inner_w = 0.0f;
  if (inner_h < 0.0f)
    inner_h = 0.0f;
  gap = frame->gap < 0.0f ? 0.0f : frame->gap;

  for (i = 0u; i < child_count; ++i) {
    float w, h;
    const ParsedAutoLayoutChild *c = &children[i];
    if (c->absolute)
      continue;
    w = clamp_size(c->base_w, c->min_w, c->max_w);
    h = clamp_size(c->base_h, c->min_h, c->max_h);
    flow_count += 1u;
    if (horizontal) {
      if (c->sizing_h == 2u)
        fill_count += 1u;
      else
        fixed_primary += w;
    } else {
      if (c->sizing_v == 2u)
        fill_count += 1u;
      else
        fixed_primary += h;
    }
  }
  if (flow_count > 1u)
    fixed_primary += gap * (float)(flow_count - 1u);
  {
    float avail = (horizontal ? inner_w : inner_h) - fixed_primary;
    if (avail < 0.0f)
      avail = 0.0f;
    if (fill_count > 0u)
      fill_each = avail / (float)fill_count;
  }

  for (i = 0u; i < child_count; ++i) {
    float primary;
    const ParsedAutoLayoutChild *c = &children[i];
    if (c->absolute)
      continue;
    primary = horizontal ? c->base_w : c->base_h;
    if (horizontal && c->sizing_h == 2u)
      primary = fill_each;
    if (!horizontal && c->sizing_v == 2u)
      primary = fill_each;
    primary = clamp_size(primary, horizontal ? c->min_w : c->min_h,
                         horizontal ? c->max_w : c->max_h);
    total_primary += primary;
  }
  if (flow_count > 1u)
    total_primary += gap * (float)(flow_count - 1u);

  cursor = horizontal ? frame->padding_left : frame->padding_top;
  if (frame->primary_align == 1u)
    cursor += ((horizontal ? inner_w : inner_h) - total_primary) * 0.5f;
  else if (frame->primary_align == 2u)
    cursor += (horizontal ? inner_w : inner_h) - total_primary;

  if (horizontal && (frame->overflow_policy == 2u || frame->overflow_policy == 4u)) {
    float max_scroll = total_primary - inner_w;
    if (max_scroll < 0.0f)
      max_scroll = 0.0f;
    scroll = frame->scroll_x;
    if (scroll < 0.0f)
      scroll = 0.0f;
    if (scroll > max_scroll)
      scroll = max_scroll;
  } else if (!horizontal &&
             (frame->overflow_policy == 3u || frame->overflow_policy == 4u)) {
    float max_scroll = total_primary - inner_h;
    if (max_scroll < 0.0f)
      max_scroll = 0.0f;
    scroll = frame->scroll_y;
    if (scroll < 0.0f)
      scroll = 0.0f;
    if (scroll > max_scroll)
      scroll = max_scroll;
  }

  for (i = 0u; i < child_count && resolved < max_out; ++i) {
    float x, y, w, h;
    const ParsedAutoLayoutChild *c = &children[i];
    x = c->base_x;
    y = c->base_y;
    w = clamp_size(c->base_w, c->min_w, c->max_w);
    h = clamp_size(c->base_h, c->min_h, c->max_h);
    if (!c->absolute) {
      float cross;
      float cross_offset = 0.0f;
      if (horizontal) {
        if (c->sizing_h == 2u)
          w = fill_each;
        if (c->sizing_v == 2u || frame->cross_align == 3u)
          h = inner_h;
        w = clamp_size(w, c->min_w, c->max_w);
        h = clamp_size(h, c->min_h, c->max_h);
        cross = inner_h - h;
        if (cross < 0.0f)
          cross = 0.0f;
        if (frame->cross_align == 1u)
          cross_offset = cross * 0.5f;
        else if (frame->cross_align == 2u)
          cross_offset = cross;
        x = cursor - scroll;
        y = frame->padding_top + cross_offset;
        cursor += w + gap;
      } else {
        if (c->sizing_v == 2u)
          h = fill_each;
        if (c->sizing_h == 2u || frame->cross_align == 3u)
          w = inner_w;
        w = clamp_size(w, c->min_w, c->max_w);
        h = clamp_size(h, c->min_h, c->max_h);
        cross = inner_w - w;
        if (cross < 0.0f)
          cross = 0.0f;
        if (frame->cross_align == 1u)
          cross_offset = cross * 0.5f;
        else if (frame->cross_align == 2u)
          cross_offset = cross;
        x = frame->padding_left + cross_offset;
        y = cursor - scroll;
        cursor += h + gap;
      }
    }
    out_child_ids[resolved] = c->child_id;
    out_x[resolved] = x;
    out_y[resolved] = y;
    out_w[resolved] = w;
    out_h[resolved] = h;
    resolved += 1u;
  }
  return resolved;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorFrameDesc frame = {0};
  StygianEditorRectDesc rect = {0};
  StygianEditorFrameAutoLayout layout = {0};
  StygianEditorNodeLayoutOptions opts = {0};
  StygianEditorNodeId frame_id = 0u;
  StygianEditorNodeId ids[4] = {0};
  ParsedAutoLayoutFrame frames[16];
  ParsedAutoLayoutChild children[64];
  ParsedAutoLayoutChild frame_children[16];
  uint32_t frame_count = 0u;
  uint32_t child_count = 0u;
  uint32_t local_child_count = 0u;
  uint32_t i;
  char code[262144];
  size_t need = 0u;
  float ex_x[4], ex_y[4], ex_w[4], ex_h[4];
  uint32_t rs_ids[16];
  float rs_x[16], rs_y[16], rs_w[16], rs_h[16];
  uint32_t rs_count;
  const ParsedAutoLayoutFrame *target_frame = NULL;

  cfg.max_nodes = 256u;
  cfg.max_path_points = 4096u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 16u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("editor create failed");

  frame.x = 0.0f;
  frame.y = 0.0f;
  frame.w = 300.0f;
  frame.h = 100.0f;
  frame.clip_content = true;
  frame.fill = stygian_editor_color_rgba(0.1f, 0.1f, 0.1f, 1.0f);
  frame.visible = true;
  frame.z = 0.0f;
  frame_id = stygian_editor_add_frame(editor, &frame);
  if (!frame_id)
    return fail("add frame failed");

  rect.x = 0.0f;
  rect.y = 0.0f;
  rect.fill = stygian_editor_color_rgba(1.0f, 1.0f, 1.0f, 1.0f);
  rect.visible = true;
  rect.z = 1.0f;

  rect.w = 50.0f;
  rect.h = 20.0f;
  ids[0] = stygian_editor_add_rect(editor, &rect);
  rect.w = 40.0f;
  rect.h = 20.0f;
  ids[1] = stygian_editor_add_rect(editor, &rect);
  rect.w = 20.0f;
  rect.h = 20.0f;
  ids[2] = stygian_editor_add_rect(editor, &rect);
  rect.x = 240.0f;
  rect.y = 70.0f;
  rect.w = 30.0f;
  rect.h = 20.0f;
  ids[3] = stygian_editor_add_rect(editor, &rect);
  for (i = 0u; i < 4u; ++i) {
    if (!ids[i])
      return fail("add rect failed");
    if (!stygian_editor_reparent_node(editor, ids[i], frame_id, false))
      return fail("reparent failed");
  }

  opts.absolute_position = false;
  opts.sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  opts.sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  if (!stygian_editor_node_set_layout_options(editor, ids[0], &opts))
    return fail("set layout options child 0 failed");
  opts.sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_HUG;
  if (!stygian_editor_node_set_layout_options(editor, ids[1], &opts))
    return fail("set layout options child 1 failed");
  opts.sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FILL;
  if (!stygian_editor_node_set_layout_options(editor, ids[2], &opts))
    return fail("set layout options child 2 failed");
  opts.absolute_position = true;
  opts.sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  if (!stygian_editor_node_set_layout_options(editor, ids[3], &opts))
    return fail("set layout options child 3 failed");

  if (!stygian_editor_node_set_size_limits(editor, ids[2], 120.0f, 200.0f, 0.0f,
                                           0.0f)) {
    return fail("set size limits failed");
  }

  layout.mode = STYGIAN_EDITOR_AUTO_LAYOUT_HORIZONTAL;
  layout.padding_left = 10.0f;
  layout.padding_right = 10.0f;
  layout.padding_top = 10.0f;
  layout.padding_bottom = 10.0f;
  layout.gap = 10.0f;
  layout.primary_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  layout.cross_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
  if (!stygian_editor_frame_set_auto_layout(editor, frame_id, &layout))
    return fail("set auto layout failed");
  if (!stygian_editor_frame_set_overflow_policy(
          editor, frame_id, STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_X))
    return fail("set overflow failed");
  if (!stygian_editor_resize_frame(editor, frame_id, 400.0f, 100.0f))
    return fail("resize frame failed");
  if (!stygian_editor_frame_set_scroll(editor, frame_id, 20.0f, 0.0f))
    return fail("set scroll failed");

  for (i = 0u; i < 4u; ++i) {
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
  if (!strstr(code, "stygian_editor_generated_apply_auto_layout_for_parent"))
    return fail("generated auto-layout solver symbol missing");
  if (!parse_auto_layout_frames(code, frames, 16u, &frame_count))
    return fail("parse auto-layout frames failed");
  if (!parse_auto_layout_children(code, children, 64u, &child_count))
    return fail("parse auto-layout children failed");

  for (i = 0u; i < frame_count; ++i) {
    if (frames[i].node_id == frame_id) {
      target_frame = &frames[i];
      break;
    }
  }
  if (!target_frame)
    return fail("target frame record missing");

  for (i = 0u; i < child_count; ++i) {
    if (children[i].parent_id != frame_id)
      continue;
    if (local_child_count >= 16u)
      return fail("too many local children");
    frame_children[local_child_count++] = children[i];
  }
  if (local_child_count == 0u)
    return fail("no local children in exported layout records");

  rs_count = solve_auto_layout(target_frame, frame_children, local_child_count, 400.0f,
                               100.0f, rs_ids, rs_x, rs_y, rs_w, rs_h, 16u);
  if (rs_count == 0u)
    return fail("runtime-equivalent auto-layout solve produced no results");

  for (i = 0u; i < rs_count; ++i) {
    uint32_t j;
    bool matched = false;
    for (j = 0u; j < 4u; ++j) {
      if (ids[j] != rs_ids[i])
        continue;
      matched = true;
      if (!nearf(rs_x[i], ex_x[j]) || !nearf(rs_y[i], ex_y[j]) ||
          !nearf(rs_w[i], ex_w[j]) || !nearf(rs_h[i], ex_h[j])) {
        return fail("runtime-equivalent auto-layout mismatch vs editor result");
      }
      break;
    }
    if (!matched)
      return fail("untracked child id in runtime solve");
  }

  stygian_editor_destroy(editor);
  printf("[PASS] editor auto layout runtime equivalent smoke\n");
  return 0;
}
