#include "../../../../include/stygian.h"
#include "../../../../widgets/stygian_widgets.h"
#include "../../../../window/stygian_input.h"
#include "../../../../window/stygian_window.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef STYGIAN_TEMPLATE_APP_TITLE
#define STYGIAN_TEMPLATE_APP_TITLE "FadeTerm"
#endif

#ifdef STYGIAN_DEMO_VULKAN
#define STYGIAN_TEMPLATE_BACKEND STYGIAN_BACKEND_VULKAN
#define STYGIAN_TEMPLATE_WINDOW_FLAGS                                           \
  (STYGIAN_WINDOW_RESIZABLE | STYGIAN_WINDOW_BORDERLESS |                      \
   STYGIAN_WINDOW_VULKAN)
#else
#define STYGIAN_TEMPLATE_BACKEND STYGIAN_BACKEND_OPENGL
#define STYGIAN_TEMPLATE_WINDOW_FLAGS                                           \
  (STYGIAN_WINDOW_RESIZABLE | STYGIAN_WINDOW_BORDERLESS |                      \
   STYGIAN_WINDOW_OPENGL)
#endif

#define FADETERM_MAX_TABS 24

typedef struct TemplateLayout {
  float shell_x, shell_y, shell_w, shell_h;
  float titlebar_h;
  float button_w, button_h, button_gap;
  float button_y;
  float menu_x;
  float minimize_x;
  float maximize_x;
  float close_x;
  float tabs_x;
  float tabs_y;
  float tabs_w;
  float tabs_h;
  float tabs_scroll_x;
  float tabs_scroll_w;
  float tabs_add_x;
  float tabs_add_y;
  float tabs_add_w;
  float tabs_add_h;
  float body_x;
  float body_y;
  float body_w;
  float body_h;
  float viewport_x;
  float viewport_y;
  float viewport_w;
  float viewport_h;
  float input_x;
  float input_y;
  float input_w;
  float input_h;
} TemplateLayout;

typedef struct FadeTermState {
  float mouse_x;
  float mouse_y;
  float tab_scroll;
  bool mouse_down;
  bool shutting_down;
  bool hover_state_valid;
  bool hovered_menu_prev;
  bool hovered_minimize_prev;
  bool hovered_maximize_prev;
  bool hovered_close_prev;
  bool hovered_add_prev;
  int hovered_tab_prev;
  int active_tab;
  int tab_count;
  int next_session_id;
  char tabs[FADETERM_MAX_TABS][32];
} FadeTermState;

static bool template_point_in_rect(float px, float py, float x, float y, float w,
                                   float h) {
  return px >= x && px < (x + w) && py >= y && py < (y + h);
}

static bool template_point_in_button(const TemplateLayout *layout, float px,
                                     float py, float x) {
  return template_point_in_rect(px, py, x, layout->button_y, layout->button_w,
                                layout->button_h);
}

static bool fadeterm_point_in_tab_strip(const TemplateLayout *layout, float px,
                                        float py) {
  return template_point_in_rect(px, py, layout->tabs_scroll_x, layout->tabs_y,
                                layout->tabs_scroll_w, layout->tabs_h);
}

static bool fadeterm_point_in_add_button(const TemplateLayout *layout, float px,
                                         float py) {
  return template_point_in_rect(px, py, layout->tabs_add_x, layout->tabs_add_y,
                                layout->tabs_add_w, layout->tabs_add_h);
}

static float template_button_tint(bool hovered, bool pressed) {
  if (pressed)
    return 0.28f;
  if (hovered)
    return 0.24f;
  return 0.205f;
}

static void template_button_fill(bool hovered, bool pressed, bool is_close,
                                 float *out_r, float *out_g, float *out_b) {
  if (!out_r || !out_g || !out_b)
    return;
  if (is_close && hovered) {
    if (pressed) {
      *out_r = 0.84f;
      *out_g = 0.23f;
      *out_b = 0.23f;
    } else {
      *out_r = 0.66f;
      *out_g = 0.18f;
      *out_b = 0.18f;
    }
    return;
  }

  {
    float bg = template_button_tint(hovered, pressed);
    *out_r = bg;
    *out_g = bg;
    *out_b = bg + 0.005f;
  }
}

static float template_snap(float v) { return floorf(v + 0.5f); }

static float template_clamp(float v, float lo, float hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static void template_draw_frame_glyph(StygianContext *ctx, float x, float y,
                                      float w, float h, float radius, float c,
                                      float bg_r, float bg_g, float bg_b,
                                      float stroke) {
  stygian_rect_rounded(ctx, x, y, w, h, c, c, c, 1.0f, radius);
  stygian_rect_rounded(ctx, x + stroke, y + stroke, w - stroke * 2.0f,
                       h - stroke * 2.0f, bg_r, bg_g, bg_b, 1.0f,
                       radius - stroke * 0.35f);
}

static void template_draw_builtin_icon(StygianContext *ctx, float x, float y,
                                       float w, float h, StygianType type,
                                       float c) {
  StygianElement icon = stygian_element_transient(ctx);
  if (!icon)
    return;
  stygian_set_bounds(ctx, icon, template_snap(x), template_snap(y), w, h);
  stygian_set_color(ctx, icon, c, c, c, 1.0f);
  stygian_set_type(ctx, icon, type);
}

static void template_draw_minimize_glyph(StygianContext *ctx, float x, float y,
                                         float w, float h, float c) {
  float gx = x + w * 0.24f;
  float gy = y + h * 0.24f;
  float gw = w * 0.52f;
  float gh = h * 0.52f;
  template_draw_builtin_icon(ctx, gx, gy, gw, gh, STYGIAN_ICON_CHEVRON, c);
}

static void template_draw_maximize_glyph(StygianContext *ctx, float x, float y,
                                         float w, float h, float c, float bg_r,
                                         float bg_g, float bg_b) {
  float size = template_snap(w * 0.42f);
  float left = template_snap(x + (w - size) * 0.5f);
  float top = template_snap(y + (h - size) * 0.5f - 0.5f);

  template_draw_frame_glyph(ctx, left, top, size, size, 1.6f, c, bg_r, bg_g,
                            bg_b, 2.0f);
}

static void template_draw_restore_glyph(StygianContext *ctx, float x, float y,
                                        float w, float h, float c, float bg_r,
                                        float bg_g, float bg_b) {
  float size = template_snap(w * 0.24f);
  float stroke = 1.8f;
  float back_left = template_snap(x + w * 0.42f);
  float back_top = template_snap(y + h * 0.30f);
  float front_left = template_snap(x + w * 0.30f);
  float front_top = template_snap(y + h * 0.42f);

  template_draw_frame_glyph(ctx, back_left, back_top, size, size, 1.4f, c,
                            bg_r, bg_g, bg_b, stroke);
  template_draw_frame_glyph(ctx, front_left, front_top, size, size, 1.4f, c,
                            bg_r, bg_g, bg_b, stroke);
}

static void template_draw_close_glyph(StygianContext *ctx, float x, float y,
                                      float w, float h, float c) {
  float gx = x + w * 0.24f;
  float gy = y + h * 0.24f;
  float gw = w * 0.52f;
  float gh = h * 0.52f;
  template_draw_builtin_icon(ctx, gx, gy, gw, gh, STYGIAN_ICON_CLOSE, c);
}

static void template_draw_menu_glyph(StygianContext *ctx, float x, float y,
                                     float w, float h, float c) {
  float glyph_w = template_snap(w * 0.42f);
  float glyph_h = 2.0f;
  float gap = 4.0f;
  float left = template_snap(x + (w - glyph_w) * 0.5f);
  float top = template_snap(y + (h - (glyph_h * 3.0f + gap * 2.0f)) * 0.5f);
  float radius = 1.0f;

  stygian_rect_rounded(ctx, left, top, glyph_w, glyph_h, c, c, c, 1.0f,
                       radius);
  stygian_rect_rounded(ctx, left, top + glyph_h + gap, glyph_w, glyph_h, c, c,
                       c, 1.0f, radius);
  stygian_rect_rounded(ctx, left, top + (glyph_h + gap) * 2.0f, glyph_w,
                       glyph_h, c, c, c, 1.0f, radius);
}

static void template_draw_title(StygianContext *ctx, StygianFont font,
                                const TemplateLayout *layout,
                                const char *title) {
  float text_size = 14.0f;
  float left_bound;
  float right_bound;
  float avail_w;
  float title_w;
  float title_x;
  float title_y;

  if (!ctx || !font || !layout || !title || !title[0])
    return;

  left_bound = layout->menu_x + layout->button_w + 18.0f;
  right_bound = layout->minimize_x - 18.0f;
  avail_w = right_bound - left_bound;
  if (avail_w <= 24.0f)
    return;

  title_w = stygian_text_width(ctx, font, title, text_size);
  title_x = template_clamp(layout->shell_x + (layout->shell_w - title_w) * 0.5f,
                           left_bound, right_bound - title_w);
  title_y = template_snap(layout->button_y + 5.0f);
  stygian_text(ctx, font, title, title_x, title_y, text_size, 0.92f, 0.93f,
               0.96f, 0.98f);
}

static void template_draw_caption_button_bg(StygianContext *ctx,
                                            const TemplateLayout *layout,
                                            float x, bool hovered, bool pressed,
                                            bool is_close) {
  float r, g, b;
  template_button_fill(hovered, pressed, is_close, &r, &g, &b);
  if (is_close && hovered) {
    float mid_pad = pressed ? 5.0f : 4.0f;
    float outer_pad = pressed ? 8.0f : 7.0f;
    float mid_a = pressed ? 0.55f : 0.35f;
    float outer_a = pressed ? 0.34f : 0.20f;
    stygian_rect_rounded(ctx, x - outer_pad, layout->button_y - outer_pad,
                         layout->button_w + outer_pad * 2.0f,
                         layout->button_h + outer_pad * 2.0f, r, g, b, outer_a,
                         11.0f);
    stygian_rect_rounded(ctx, x - mid_pad, layout->button_y - mid_pad,
                         layout->button_w + mid_pad * 2.0f,
                         layout->button_h + mid_pad * 2.0f, r, g, b, mid_a,
                         9.0f);
  }
  stygian_rect_rounded(ctx, x, layout->button_y, layout->button_w,
                       layout->button_h, r, g, b, 1.0f, 7.0f);
}

static void fadeterm_seed_tabs(FadeTermState *state) {
  memset(state, 0, sizeof(*state));
  state->hovered_tab_prev = -1;
  state->active_tab = 0;
  state->tab_count = 3;
  state->next_session_id = 4;
  strcpy(state->tabs[0], "shell");
  strcpy(state->tabs[1], "server");
  strcpy(state->tabs[2], "build");
}

static float fadeterm_tab_width(StygianContext *ctx, StygianFont font,
                                const char *label) {
  float text_w = 62.0f;
  if (ctx && font && label)
    text_w = stygian_text_width(ctx, font, label, 13.0f);
  return template_clamp(text_w + 34.0f, 86.0f, 164.0f);
}

static float fadeterm_tab_total_width(StygianContext *ctx, StygianFont font,
                                      const FadeTermState *state) {
  int i;
  float total = 8.0f;
  for (i = 0; i < state->tab_count; ++i)
    total += fadeterm_tab_width(ctx, font, state->tabs[i]) + 8.0f;
  return total;
}

static float fadeterm_tab_max_scroll(StygianContext *ctx, StygianFont font,
                                     const TemplateLayout *layout,
                                     const FadeTermState *state) {
  float overflow =
      fadeterm_tab_total_width(ctx, font, state) - layout->tabs_scroll_w;
  return overflow > 0.0f ? overflow : 0.0f;
}

static void fadeterm_clamp_scroll(StygianContext *ctx, StygianFont font,
                                  const TemplateLayout *layout,
                                  FadeTermState *state) {
  state->tab_scroll = template_clamp(
      state->tab_scroll, 0.0f, fadeterm_tab_max_scroll(ctx, font, layout, state));
}

static void fadeterm_tab_rect(StygianContext *ctx, StygianFont font,
                              const TemplateLayout *layout,
                              const FadeTermState *state, int index, float *out_x,
                              float *out_w) {
  int i;
  float x = layout->tabs_scroll_x + 6.0f - state->tab_scroll;
  for (i = 0; i < index; ++i)
    x += fadeterm_tab_width(ctx, font, state->tabs[i]) + 8.0f;
  *out_x = x;
  *out_w = fadeterm_tab_width(ctx, font, state->tabs[index]);
}

static int fadeterm_hovered_tab(StygianContext *ctx, StygianFont font,
                                const TemplateLayout *layout,
                                const FadeTermState *state, float px, float py) {
  int i;
  if (!fadeterm_point_in_tab_strip(layout, px, py))
    return -1;
  for (i = 0; i < state->tab_count; ++i) {
    float tab_x, tab_w;
    fadeterm_tab_rect(ctx, font, layout, state, i, &tab_x, &tab_w);
    if (template_point_in_rect(px, py, tab_x, layout->tabs_y + 2.0f, tab_w,
                               layout->tabs_h - 4.0f))
      return i;
  }
  return -1;
}

static void fadeterm_reveal_tab(StygianContext *ctx, StygianFont font,
                                const TemplateLayout *layout,
                                FadeTermState *state, int index) {
  float tab_x, tab_w;
  float visible_left = layout->tabs_scroll_x + 6.0f;
  float visible_right = layout->tabs_scroll_x + layout->tabs_scroll_w - 6.0f;
  if (index < 0 || index >= state->tab_count)
    return;
  fadeterm_tab_rect(ctx, font, layout, state, index, &tab_x, &tab_w);
  if (tab_x < visible_left)
    state->tab_scroll -= visible_left - tab_x;
  else if (tab_x + tab_w > visible_right)
    state->tab_scroll += (tab_x + tab_w) - visible_right;
  fadeterm_clamp_scroll(ctx, font, layout, state);
}

static bool fadeterm_add_tab(FadeTermState *state) {
  if (state->tab_count >= FADETERM_MAX_TABS)
    return false;
  snprintf(state->tabs[state->tab_count], sizeof(state->tabs[state->tab_count]),
           "session-%d", state->next_session_id++);
  state->active_tab = state->tab_count;
  state->tab_count++;
  return true;
}

static void fadeterm_scroll_tabs(StygianContext *ctx, StygianFont font,
                                 const TemplateLayout *layout,
                                 FadeTermState *state, float delta) {
  state->tab_scroll += delta;
  fadeterm_clamp_scroll(ctx, font, layout, state);
}

static void template_compute_layout(TemplateLayout *layout, int width, int height,
                                    const StygianTitlebarHints *hints) {
  memset(layout, 0, sizeof(*layout));
  layout->shell_x = 0.0f;
  layout->shell_y = 0.0f;
  layout->shell_w = (float)width;
  layout->shell_h = (float)height;
  layout->titlebar_h = hints->recommended_titlebar_height > 0.0f
                           ? hints->recommended_titlebar_height + 4.0f
                           : 42.0f;
  layout->button_w = hints->recommended_button_width > 0.0f
                         ? hints->recommended_button_width + 6.0f
                         : 34.0f;
  layout->button_h = hints->recommended_button_height > 0.0f
                         ? hints->recommended_button_height + 2.0f
                         : 26.0f;
  layout->button_gap = hints->recommended_button_gap > 0.0f
                           ? hints->recommended_button_gap + 2.0f
                           : 7.0f;
  layout->button_y = layout->shell_y + 7.0f;
  layout->menu_x = layout->shell_x + 12.0f;
  layout->close_x =
      layout->shell_x + layout->shell_w - layout->button_w - 8.0f;
  layout->maximize_x =
      layout->close_x - layout->button_gap - layout->button_w;
  layout->minimize_x =
      layout->maximize_x - layout->button_gap - layout->button_w;
  layout->tabs_x = layout->menu_x + layout->button_w + 18.0f;
  layout->tabs_y = layout->button_y - 1.0f;
  layout->tabs_h = layout->button_h + 4.0f;
  layout->tabs_w = layout->minimize_x - layout->tabs_x - 12.0f;
  layout->tabs_add_w = 28.0f;
  layout->tabs_add_h = 28.0f;
  layout->tabs_add_x = layout->tabs_x + layout->tabs_w - layout->tabs_add_w;
  layout->tabs_add_y = layout->button_y - 1.0f;
  layout->tabs_scroll_x = layout->tabs_x;
  layout->tabs_scroll_w = layout->tabs_w - layout->tabs_add_w - 8.0f;
  layout->body_x = layout->shell_x + 14.0f;
  layout->body_y = layout->titlebar_h + 14.0f;
  layout->body_w = layout->shell_w - 28.0f;
  layout->body_h = layout->shell_h - layout->body_y - 14.0f;
  layout->viewport_x = layout->body_x + 12.0f;
  layout->viewport_y = layout->body_y + 52.0f;
  layout->viewport_w = layout->body_w - 24.0f;
  layout->viewport_h = layout->body_h - 116.0f;
  layout->input_x = layout->body_x + 12.0f;
  layout->input_y = layout->body_y + layout->body_h - 52.0f;
  layout->input_w = layout->body_w - 24.0f;
  layout->input_h = 40.0f;
}

static void fadeterm_update_pointer(FadeTermState *state,
                                    const StygianEvent *event) {
  if (event->type == STYGIAN_EVENT_MOUSE_MOVE) {
    state->mouse_x = (float)event->mouse_move.x;
    state->mouse_y = (float)event->mouse_move.y;
  } else if (event->type == STYGIAN_EVENT_MOUSE_DOWN ||
             event->type == STYGIAN_EVENT_MOUSE_UP) {
    state->mouse_x = (float)event->mouse_button.x;
    state->mouse_y = (float)event->mouse_button.y;
    state->mouse_down =
        (event->type == STYGIAN_EVENT_MOUSE_DOWN) &&
        (event->mouse_button.button == STYGIAN_MOUSE_LEFT);
  } else if (event->type == STYGIAN_EVENT_SCROLL) {
    state->mouse_x = (float)event->scroll.x;
    state->mouse_y = (float)event->scroll.y;
  }
}

static void fadeterm_handle_left_click(StygianWindow *window, StygianContext *ctx,
                                       StygianFont font,
                                       const TemplateLayout *layout,
                                       FadeTermState *state,
                                       const StygianEvent *event,
                                       bool *event_mutated,
                                       bool *event_requested) {
  bool in_menu =
      template_point_in_button(layout, state->mouse_x, state->mouse_y,
                               layout->menu_x);
  bool in_minimize =
      template_point_in_button(layout, state->mouse_x, state->mouse_y,
                               layout->minimize_x);
  bool in_maximize =
      template_point_in_button(layout, state->mouse_x, state->mouse_y,
                               layout->maximize_x);
  bool in_close =
      template_point_in_button(layout, state->mouse_x, state->mouse_y,
                               layout->close_x);
  bool in_add =
      fadeterm_point_in_add_button(layout, state->mouse_x, state->mouse_y);
  int hovered_tab =
      fadeterm_hovered_tab(ctx, font, layout, state, state->mouse_x, state->mouse_y);
  bool in_titlebar = template_point_in_rect(state->mouse_x, state->mouse_y,
                                            layout->shell_x, layout->shell_y,
                                            layout->shell_w,
                                            layout->titlebar_h + 10.0f);

  if (in_menu) {
    // This stays a stub for now. Real menu logic can show up when the app earns it.
    *event_requested = true;
  } else if (hovered_tab >= 0) {
    state->active_tab = hovered_tab;
    fadeterm_reveal_tab(ctx, font, layout, state, hovered_tab);
    *event_mutated = true;
    *event_requested = true;
  } else if (in_add) {
    if (fadeterm_add_tab(state)) {
      fadeterm_reveal_tab(ctx, font, layout, state, state->active_tab);
      *event_mutated = true;
    }
    *event_requested = true;
  } else if (in_minimize) {
    stygian_window_minimize(window);
    *event_mutated = true;
    *event_requested = true;
  } else if (in_maximize) {
    if (stygian_window_is_maximized(window))
      stygian_window_restore(window);
    else
      stygian_window_maximize(window);
    *event_mutated = true;
    *event_requested = true;
  } else if (in_close) {
    stygian_set_present_enabled(ctx, false);
    state->shutting_down = true;
    stygian_window_request_close(window);
    *event_mutated = true;
    *event_requested = true;
  } else if (in_titlebar) {
    if (event->mouse_button.clicks >= 2)
      stygian_window_titlebar_double_click(window);
    else
      stygian_window_begin_system_move(window);
    *event_mutated = true;
    *event_requested = true;
  }
}

static void fadeterm_process_event(StygianWindow *window, StygianContext *ctx,
                                   StygianFont font,
                                   const TemplateLayout *layout,
                                   FadeTermState *state,
                                   const StygianEvent *event,
                                   bool *event_mutated,
                                   bool *event_requested) {
  fadeterm_update_pointer(state, event);

  if (event->type == STYGIAN_EVENT_SCROLL &&
      fadeterm_point_in_tab_strip(layout, state->mouse_x, state->mouse_y)) {
    float delta = (-event->scroll.dy * 48.0f) - (event->scroll.dx * 36.0f);
    fadeterm_scroll_tabs(ctx, font, layout, state, delta);
    *event_requested = true;
  }

  if (event->type == STYGIAN_EVENT_MOUSE_DOWN &&
      event->mouse_button.button == STYGIAN_MOUSE_LEFT) {
    fadeterm_handle_left_click(window, ctx, font, layout, state, event,
                               event_mutated, event_requested);
  }

  if (event->type == STYGIAN_EVENT_CLOSE) {
    stygian_set_present_enabled(ctx, false);
    state->shutting_down = true;
    stygian_window_request_close(window);
  }
}

static void fadeterm_draw_tab_strip(StygianContext *ctx, StygianFont font,
                                    const TemplateLayout *layout,
                                    const FadeTermState *state) {
  int i;
  float strip_r = 0.135f;
  float strip_g = 0.14f;
  float strip_b = 0.15f;
  float plus_bg = fadeterm_point_in_add_button(layout, state->mouse_x,
                                               state->mouse_y)
                      ? 0.25f
                      : 0.205f;

  stygian_rect_rounded(ctx, layout->tabs_scroll_x, layout->tabs_y,
                       layout->tabs_scroll_w, layout->tabs_h, strip_r, strip_g,
                       strip_b, 0.92f, 12.0f);
  stygian_rect_rounded(ctx, layout->tabs_add_x, layout->tabs_add_y,
                       layout->tabs_add_w, layout->tabs_add_h, plus_bg, plus_bg,
                       plus_bg + 0.01f, 1.0f, 8.0f);
  template_draw_builtin_icon(ctx, layout->tabs_add_x + 6.0f,
                             layout->tabs_add_y + 6.0f, 16.0f, 16.0f,
                             STYGIAN_ICON_PLUS, 0.94f);

  // The strip is clipped because overflowing tabs love to scribble over the
  // window controls when nobody is watching.
  stygian_clip_push(ctx, layout->tabs_scroll_x, layout->tabs_y,
                    layout->tabs_scroll_w, layout->tabs_h);
  for (i = 0; i < state->tab_count; ++i) {
    float tab_x, tab_w;
    float tab_r, tab_g, tab_b;
    float text_r, text_g, text_b, text_a;
    bool hovered =
        (i == fadeterm_hovered_tab(ctx, font, layout, state, state->mouse_x,
                                   state->mouse_y));
    bool active = (i == state->active_tab);

    fadeterm_tab_rect(ctx, font, layout, state, i, &tab_x, &tab_w);
    if (tab_x + tab_w < layout->tabs_scroll_x - 2.0f ||
        tab_x > layout->tabs_scroll_x + layout->tabs_scroll_w + 2.0f)
      continue;

    if (active) {
      tab_r = 0.215f;
      tab_g = 0.225f;
      tab_b = 0.245f;
      text_r = 0.96f;
      text_g = 0.97f;
      text_b = 0.99f;
      text_a = 1.0f;
    } else if (hovered) {
      tab_r = 0.19f;
      tab_g = 0.195f;
      tab_b = 0.215f;
      text_r = 0.90f;
      text_g = 0.92f;
      text_b = 0.96f;
      text_a = 0.98f;
    } else {
      tab_r = 0.17f;
      tab_g = 0.175f;
      tab_b = 0.19f;
      text_r = 0.76f;
      text_g = 0.79f;
      text_b = 0.86f;
      text_a = 0.94f;
    }

    stygian_rect_rounded(ctx, tab_x, layout->tabs_y + 2.0f, tab_w,
                         layout->tabs_h - 4.0f, tab_r, tab_g, tab_b, 1.0f,
                         11.0f);
    if (active) {
      stygian_rect_rounded(ctx, tab_x + 12.0f, layout->tabs_y + 4.0f,
                           tab_w - 24.0f, 2.0f, 0.53f, 0.69f, 0.98f, 0.95f,
                           1.0f);
    }
    stygian_text(ctx, font, state->tabs[i], tab_x + 16.0f,
                 template_snap(layout->tabs_y + 9.0f), 13.0f, text_r, text_g,
                 text_b, text_a);
  }
  stygian_clip_pop(ctx);
}

static void fadeterm_draw_terminal_body(StygianContext *ctx, StygianFont font,
                                        const TemplateLayout *layout,
                                        const FadeTermState *state) {
  static const char *k_lines[] = {
      "fade@stygian  ~/workspace/fadeterm                                          09:41",
      "-------------------------------------------------------------------------------",
      "$ git status --short",
      " M showcase/apps/fadeterm/src/fadeterm.c",
      " M compile/targets.json",
      "$ ./tools/check_nlnet_readiness --app fadeterm",
      "[ok] shell chrome is stable",
      "[ok] tabs are scrollable and keyboard-safe",
      "[todo] interactive input, history, and scrollback",
      "[todo] command execution bridge",
      "$ echo \"FadeTerm is back in business\"",
      "FadeTerm is back in business"};
  float body_r = 0.085f;
  float body_g = 0.095f;
  float body_b = 0.125f;
  float panel_r = 0.065f;
  float panel_g = 0.074f;
  float panel_b = 0.10f;
  float line_y = layout->viewport_y + 22.0f;
  float line_h = 18.0f;
  int i;

  stygian_rect_rounded(ctx, layout->body_x, layout->body_y, layout->body_w,
                       layout->body_h, body_r, body_g, body_b, 1.0f, 14.0f);
  stygian_rect_rounded(ctx, layout->body_x + 12.0f, layout->body_y + 12.0f,
                       116.0f, 24.0f, 0.14f, 0.17f, 0.24f, 1.0f, 8.0f);
  stygian_text(ctx, font, state->tabs[state->active_tab], layout->body_x + 26.0f,
               layout->body_y + 18.0f, 13.0f, 0.93f, 0.95f, 0.99f, 0.98f);
  stygian_rect_rounded(ctx, layout->body_x + layout->body_w - 166.0f,
                       layout->body_y + 12.0f, 64.0f, 24.0f, 0.12f, 0.18f,
                       0.14f, 1.0f, 8.0f);
  stygian_text(ctx, font, "secure", layout->body_x + layout->body_w - 148.0f,
               layout->body_y + 18.0f, 12.0f, 0.76f, 0.95f, 0.82f, 0.98f);
  stygian_rect_rounded(ctx, layout->body_x + layout->body_w - 92.0f,
                       layout->body_y + 12.0f, 68.0f, 24.0f, 0.16f, 0.12f,
                       0.11f, 1.0f, 8.0f);
  stygian_text(ctx, font, "UTF-8", layout->body_x + layout->body_w - 73.0f,
               layout->body_y + 18.0f, 12.0f, 0.96f, 0.87f, 0.76f, 0.98f);

  stygian_rect_rounded(ctx, layout->viewport_x, layout->viewport_y,
                       layout->viewport_w, layout->viewport_h, panel_r, panel_g,
                       panel_b, 1.0f, 12.0f);
  stygian_rect_rounded(ctx, layout->viewport_x + 14.0f, layout->viewport_y + 14.0f,
                       8.0f, 8.0f, 0.85f, 0.32f, 0.34f, 0.95f, 4.0f);
  stygian_rect_rounded(ctx, layout->viewport_x + 28.0f, layout->viewport_y + 14.0f,
                       8.0f, 8.0f, 0.89f, 0.70f, 0.32f, 0.95f, 4.0f);
  stygian_rect_rounded(ctx, layout->viewport_x + 42.0f, layout->viewport_y + 14.0f,
                       8.0f, 8.0f, 0.36f, 0.76f, 0.46f, 0.95f, 4.0f);

  for (i = 0; i < (int)(sizeof(k_lines) / sizeof(k_lines[0])); ++i) {
    float rn = 0.28f;
    float gn = 0.34f;
    float bn = 0.45f;
    float alpha = 0.95f;
    if (k_lines[i][0] == '$') {
      rn = 0.58f;
      gn = 0.83f;
      bn = 0.55f;
    } else if (strncmp(k_lines[i], "[ok]", 4) == 0) {
      rn = 0.54f;
      gn = 0.84f;
      bn = 0.72f;
    } else if (strncmp(k_lines[i], "[todo]", 6) == 0) {
      rn = 0.98f;
      gn = 0.79f;
      bn = 0.45f;
    } else if (strncmp(k_lines[i], " M ", 3) == 0) {
      rn = 0.92f;
      gn = 0.72f;
      bn = 0.40f;
    } else if (strncmp(k_lines[i], "fade@stygian", 12) == 0) {
      rn = 0.72f;
      gn = 0.78f;
      bn = 0.92f;
    }
    stygian_text(ctx, font, k_lines[i], layout->viewport_x + 24.0f, line_y,
                 13.0f, rn, gn, bn, alpha);
    line_y += line_h;
  }

  stygian_rect_rounded(ctx, layout->input_x, layout->input_y, layout->input_w,
                       layout->input_h, 0.09f, 0.115f, 0.15f, 1.0f, 10.0f);
  stygian_text(ctx, font, "$", layout->input_x + 18.0f, layout->input_y + 12.0f,
               14.0f, 0.58f, 0.83f, 0.55f, 1.0f);
  stygian_text(ctx, font, "./run_session --profile nlnet-demo", layout->input_x + 38.0f,
               layout->input_y + 12.0f, 13.0f, 0.94f, 0.96f, 0.99f, 0.98f);
  stygian_rect_rounded(ctx, layout->input_x + 296.0f, layout->input_y + 10.0f,
                       2.0f, 18.0f, 0.61f, 0.78f, 1.0f, 0.98f, 1.0f);
}
int main(void) {
  const StygianScopeId k_scope_shell = 0x5201u;
  StygianWindowConfig win_cfg = {
      .title = STYGIAN_TEMPLATE_APP_TITLE,
      .width = 1180,
      .height = 760,
      .flags = STYGIAN_TEMPLATE_WINDOW_FLAGS,
  };
  StygianWindow *window = stygian_window_create(&win_cfg);
  StygianContext *ctx;
  StygianFont font = 0;
  FadeTermState state;
  bool first_frame = true;
#if defined(_WIN32) && defined(STYGIAN_DEMO_VULKAN)
  HWND native_hwnd = NULL;
  bool native_window_shown = false;
#endif

  if (!window)
    return 1;

  fadeterm_seed_tabs(&state);

#if defined(_WIN32) && defined(STYGIAN_DEMO_VULKAN)
  native_hwnd = (HWND)stygian_window_native_handle(window);
  if (native_hwnd)
    ShowWindow(native_hwnd, SW_HIDE);
#endif

  {
    StygianTitlebarBehavior titlebar_behavior = {
        .double_click_mode = STYGIAN_TITLEBAR_DBLCLICK_MAXIMIZE_RESTORE,
        .hover_menu_enabled = false,
    };
    stygian_window_set_titlebar_behavior(window, &titlebar_behavior);
  }

  ctx = stygian_create(&(StygianConfig){
      .backend = STYGIAN_TEMPLATE_BACKEND,
      .window = window,
  });
  if (!ctx)
    return 1;
  stygian_set_vsync(ctx, false);
  font = stygian_get_default_font(ctx);

  while (!stygian_window_should_close(window)) {
    StygianEvent event;
    StygianTitlebarHints hints;
    TemplateLayout layout;
    bool event_mutated = false;
    bool event_requested = false;
    bool event_eval_requested = false;
    uint32_t wait_ms = stygian_next_repaint_wait_ms(ctx, 250u);
    int width, height;

    stygian_window_get_titlebar_hints(window, &hints);
    stygian_window_get_size(window, &width, &height);
    template_compute_layout(&layout, width, height, &hints);
    fadeterm_clamp_scroll(ctx, font, &layout, &state);

    stygian_widgets_begin_frame(ctx);

    while (stygian_window_poll_event(window, &event)) {
      StygianWidgetEventImpact impact =
          stygian_widgets_process_event_ex(ctx, &event);
      if (impact & STYGIAN_IMPACT_MUTATED_STATE)
        event_mutated = true;
      if (impact & STYGIAN_IMPACT_REQUEST_REPAINT)
        event_requested = true;
      if (impact & STYGIAN_IMPACT_REQUEST_EVAL)
        event_eval_requested = true;

      fadeterm_process_event(window, ctx, font, &layout, &state, &event,
                             &event_mutated, &event_requested);
    }

    if (!first_frame && !event_mutated && !event_requested &&
        !stygian_has_pending_repaint(ctx) && !event_eval_requested) {
      if (stygian_window_wait_event_timeout(window, &event, wait_ms)) {
        StygianWidgetEventImpact impact =
            stygian_widgets_process_event_ex(ctx, &event);
        if (impact & STYGIAN_IMPACT_MUTATED_STATE)
          event_mutated = true;
        if (impact & STYGIAN_IMPACT_REQUEST_REPAINT)
          event_requested = true;
        if (impact & STYGIAN_IMPACT_REQUEST_EVAL)
          event_eval_requested = true;

        fadeterm_process_event(window, ctx, font, &layout, &state, &event,
                               &event_mutated, &event_requested);
      }
    }

    if (state.shutting_down && stygian_window_should_close(window))
      break;

    {
      bool hovered_menu_now =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.menu_x);
      bool hovered_minimize_now =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.minimize_x);
      bool hovered_maximize_now =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.maximize_x);
      bool hovered_close_now =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.close_x);
      bool hovered_add_now =
          fadeterm_point_in_add_button(&layout, state.mouse_x, state.mouse_y);
      int hovered_tab_now =
          fadeterm_hovered_tab(ctx, font, &layout, &state, state.mouse_x,
                               state.mouse_y);

      if (!state.hover_state_valid || hovered_menu_now != state.hovered_menu_prev ||
          hovered_minimize_now != state.hovered_minimize_prev ||
          hovered_maximize_now != state.hovered_maximize_prev ||
          hovered_close_now != state.hovered_close_prev ||
          hovered_add_now != state.hovered_add_prev ||
          hovered_tab_now != state.hovered_tab_prev) {
        state.hovered_menu_prev = hovered_menu_now;
        state.hovered_minimize_prev = hovered_minimize_now;
        state.hovered_maximize_prev = hovered_maximize_now;
        state.hovered_close_prev = hovered_close_now;
        state.hovered_add_prev = hovered_add_now;
        state.hovered_tab_prev = hovered_tab_now;
        state.hover_state_valid = true;
        event_requested = true;
      }
    }

    if (!first_frame && !event_mutated && !event_requested &&
        !event_eval_requested && !stygian_has_pending_repaint(ctx)) {
      stygian_widgets_commit_regions();
      stygian_end_frame(ctx);
      continue;
    }
    first_frame = false;

    stygian_window_get_size(window, &width, &height);
    template_compute_layout(&layout, width, height, &hints);
    fadeterm_clamp_scroll(ctx, font, &layout, &state);

    stygian_begin_frame_intent(
        ctx, width, height,
        (event_eval_requested && !event_requested && !event_mutated &&
         !stygian_has_pending_repaint(ctx))
            ? STYGIAN_FRAME_EVAL_ONLY
            : STYGIAN_FRAME_RENDER);

    stygian_scope_begin(ctx, k_scope_shell);
    stygian_rect(ctx, 0.0f, 0.0f, (float)width, (float)height, 0.15f, 0.15f,
                 0.16f, 1.0f);
    // Draw slightly past the window bounds so the rounded-edge AA gets clipped
    // off instead of leaving a pale seam around the shell.
    stygian_rect_rounded(ctx, layout.shell_x - 2.0f, layout.shell_y - 2.0f,
                         layout.shell_w + 4.0f, layout.shell_h + 4.0f, 0.15f,
                         0.15f, 0.16f, 1.0f, 8.0f);
    {
      bool hovered_menu =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.menu_x);
      bool hovered_minimize =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.minimize_x);
      bool hovered_maximize =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.maximize_x);
      bool hovered_close =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.close_x);
      bool maximized = stygian_window_is_maximized(window);
      float base_icon = 0.94f;
      float menu_icon = hovered_menu ? 0.995f : 0.95f;
      float minimize_icon = hovered_minimize ? 0.995f : base_icon;
      float maximize_icon = hovered_maximize ? 0.995f : base_icon;
      float close_icon = hovered_close ? 1.0f : 0.965f;
      float max_r, max_g, max_b;
      template_button_fill(hovered_maximize,
                           hovered_maximize && state.mouse_down, false, &max_r,
                           &max_g, &max_b);
      template_draw_caption_button_bg(ctx, &layout, layout.menu_x, hovered_menu,
                                      hovered_menu && state.mouse_down, false);
      template_draw_menu_glyph(ctx, layout.menu_x, layout.button_y,
                               layout.button_w, layout.button_h, menu_icon);
      fadeterm_draw_tab_strip(ctx, font, &layout, &state);
      fadeterm_draw_terminal_body(ctx, font, &layout, &state);
      template_draw_caption_button_bg(ctx, &layout, layout.minimize_x,
                                      hovered_minimize,
                                      hovered_minimize && state.mouse_down,
                                      false);
      template_draw_minimize_glyph(ctx, layout.minimize_x, layout.button_y,
                                   layout.button_w, layout.button_h,
                                   minimize_icon);
      template_draw_caption_button_bg(ctx, &layout, layout.maximize_x,
                                      hovered_maximize,
                                      hovered_maximize && state.mouse_down,
                                      false);
      if (maximized) {
        template_draw_restore_glyph(ctx, layout.maximize_x, layout.button_y,
                                    layout.button_w, layout.button_h,
                                    maximize_icon, max_r, max_g, max_b);
      } else {
        template_draw_maximize_glyph(ctx, layout.maximize_x, layout.button_y,
                                     layout.button_w, layout.button_h,
                                     maximize_icon, max_r, max_g, max_b);
      }
      template_draw_caption_button_bg(ctx, &layout, layout.close_x,
                                      hovered_close,
                                      hovered_close && state.mouse_down, true);
      template_draw_close_glyph(ctx, layout.close_x, layout.button_y,
                                layout.button_w, layout.button_h, close_icon);
    }
    stygian_scope_end(ctx);

    stygian_scope_invalidate_next(ctx, k_scope_shell);
    if (event_requested || event_mutated) {
      stygian_set_repaint_source(ctx, "fadeterm");
      stygian_request_repaint_after_ms(ctx, 0u);
    }

    stygian_widgets_commit_regions();
    stygian_end_frame(ctx);

#if defined(_WIN32) && defined(STYGIAN_DEMO_VULKAN)
    if (!native_window_shown && native_hwnd) {
      ShowWindow(native_hwnd, SW_SHOW);
      UpdateWindow(native_hwnd);
      native_window_shown = true;
    }
#endif
  }

#ifdef STYGIAN_DEMO_VULKAN
  // Vulkan close already did the present-off ramp. Drop the window first so
  // the compositor stops caring before we tear the backend down.
  if (font)
    stygian_font_destroy(ctx, font);
  stygian_window_destroy(window);
  stygian_destroy(ctx);
#else
  if (font)
    stygian_font_destroy(ctx, font);
  stygian_destroy(ctx);
  stygian_window_destroy(window);
#endif
  return 0;
}
