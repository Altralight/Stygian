#include "../../../../include/stygian.h"
#include "../../../../widgets/stygian_widgets.h"
#include "../../../../window/stygian_input.h"
#include "../../../../window/stygian_window.h"
#include <math.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef STYGIAN_TEMPLATE_APP_TITLE
#define STYGIAN_TEMPLATE_APP_TITLE "Stygian Template"
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

typedef struct TemplateLayout {
  float shell_x, shell_y, shell_w, shell_h;
  float titlebar_h;
  float button_w, button_h, button_gap;
  float button_y;
  float menu_x;
  float minimize_x;
  float maximize_x;
  float close_x;
} TemplateLayout;

static bool template_point_in_rect(float px, float py, float x, float y, float w,
                                   float h) {
  return px >= x && px < (x + w) && py >= y && py < (y + h);
}

static bool template_point_in_button(const TemplateLayout *layout, float px,
                                     float py, float x) {
  return template_point_in_rect(px, py, x, layout->button_y, layout->button_w,
                                layout->button_h);
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
}

int main(void) {
  const StygianScopeId k_scope_shell = 0x5201u;
  StygianWindowConfig win_cfg = {
      .title = STYGIAN_TEMPLATE_APP_TITLE,
      .width = 1440,
      .height = 920,
      .flags = STYGIAN_TEMPLATE_WINDOW_FLAGS,
  };
  StygianWindow *window = stygian_window_create(&win_cfg);
  StygianContext *ctx;
  StygianFont font = 0;
  bool first_frame = true;
  float mouse_x = 0.0f;
  float mouse_y = 0.0f;
  bool mouse_down = false;
  bool shutting_down = false;
  bool hover_state_valid = false;
  bool hovered_menu_prev = false;
  bool hovered_minimize_prev = false;
  bool hovered_maximize_prev = false;
  bool hovered_close_prev = false;
#if defined(_WIN32) && defined(STYGIAN_DEMO_VULKAN)
  HWND native_hwnd = NULL;
  bool native_window_shown = false;
#endif

  if (!window)
    return 1;

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

      if (event.type == STYGIAN_EVENT_MOUSE_MOVE) {
        mouse_x = (float)event.mouse_move.x;
        mouse_y = (float)event.mouse_move.y;
      } else if (event.type == STYGIAN_EVENT_MOUSE_DOWN ||
                 event.type == STYGIAN_EVENT_MOUSE_UP) {
        mouse_x = (float)event.mouse_button.x;
        mouse_y = (float)event.mouse_button.y;
        mouse_down =
            (event.type == STYGIAN_EVENT_MOUSE_DOWN) &&
            (event.mouse_button.button == STYGIAN_MOUSE_LEFT);
      }

      if (event.type == STYGIAN_EVENT_MOUSE_DOWN &&
          event.mouse_button.button == STYGIAN_MOUSE_LEFT) {
        bool in_menu =
            template_point_in_button(&layout, mouse_x, mouse_y, layout.menu_x);
        bool in_minimize =
            template_point_in_button(&layout, mouse_x, mouse_y,
                                     layout.minimize_x);
        bool in_maximize =
            template_point_in_button(&layout, mouse_x, mouse_y,
                                     layout.maximize_x);
        bool in_close = template_point_in_button(&layout, mouse_x, mouse_y,
                                                 layout.close_x);
        bool in_titlebar =
            template_point_in_rect(mouse_x, mouse_y, layout.shell_x,
                                   layout.shell_y, layout.shell_w,
                                   layout.titlebar_h + 10.0f);
        if (in_menu) {
          // Leave this as a dead-simple anchor for the first real app menu.
          event_requested = true;
        } else if (in_minimize) {
          stygian_window_minimize(window);
          event_mutated = true;
          event_requested = true;
        } else if (in_maximize) {
          if (stygian_window_is_maximized(window))
            stygian_window_restore(window);
          else
            stygian_window_maximize(window);
          event_mutated = true;
          event_requested = true;
        } else if (in_close) {
          stygian_set_present_enabled(ctx, false);
          shutting_down = true;
          stygian_window_request_close(window);
          event_mutated = true;
          event_requested = true;
        } else if (in_titlebar) {
          if (event.mouse_button.clicks >= 2) {
            stygian_window_titlebar_double_click(window);
          } else {
            stygian_window_begin_system_move(window);
          }
          event_mutated = true;
          event_requested = true;
        }
      }

      if (event.type == STYGIAN_EVENT_CLOSE) {
        stygian_set_present_enabled(ctx, false);
        shutting_down = true;
        stygian_window_request_close(window);
      }
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

        if (event.type == STYGIAN_EVENT_MOUSE_MOVE) {
          mouse_x = (float)event.mouse_move.x;
          mouse_y = (float)event.mouse_move.y;
        } else if (event.type == STYGIAN_EVENT_MOUSE_DOWN ||
                   event.type == STYGIAN_EVENT_MOUSE_UP) {
          mouse_x = (float)event.mouse_button.x;
          mouse_y = (float)event.mouse_button.y;
          mouse_down =
              (event.type == STYGIAN_EVENT_MOUSE_DOWN) &&
              (event.mouse_button.button == STYGIAN_MOUSE_LEFT);
        }

        if (event.type == STYGIAN_EVENT_MOUSE_DOWN &&
            event.mouse_button.button == STYGIAN_MOUSE_LEFT) {
          bool in_menu =
              template_point_in_button(&layout, mouse_x, mouse_y, layout.menu_x);
          bool in_minimize =
              template_point_in_button(&layout, mouse_x, mouse_y,
                                       layout.minimize_x);
          bool in_maximize =
              template_point_in_button(&layout, mouse_x, mouse_y,
                                       layout.maximize_x);
          bool in_close = template_point_in_button(&layout, mouse_x, mouse_y,
                                                   layout.close_x);
          bool in_titlebar =
              template_point_in_rect(mouse_x, mouse_y, layout.shell_x,
                                     layout.shell_y, layout.shell_w,
                                     layout.titlebar_h + 10.0f);
          if (in_menu) {
            event_requested = true;
          } else if (in_minimize) {
            stygian_window_minimize(window);
            event_mutated = true;
            event_requested = true;
          } else if (in_maximize) {
            if (stygian_window_is_maximized(window))
              stygian_window_restore(window);
            else
              stygian_window_maximize(window);
            event_mutated = true;
            event_requested = true;
          } else if (in_close) {
            stygian_set_present_enabled(ctx, false);
            shutting_down = true;
            stygian_window_request_close(window);
            event_mutated = true;
            event_requested = true;
          } else if (in_titlebar) {
            if (event.mouse_button.clicks >= 2) {
              stygian_window_titlebar_double_click(window);
            } else {
              stygian_window_begin_system_move(window);
            }
            event_mutated = true;
            event_requested = true;
          }
        }

        if (event.type == STYGIAN_EVENT_CLOSE) {
          stygian_set_present_enabled(ctx, false);
          shutting_down = true;
          stygian_window_request_close(window);
        }
      }
    }

    if (shutting_down && stygian_window_should_close(window))
      break;

    {
      bool hovered_menu_now =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.menu_x);
      bool hovered_minimize_now =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.minimize_x);
      bool hovered_maximize_now =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.maximize_x);
      bool hovered_close_now =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.close_x);

      if (!hover_state_valid || hovered_menu_now != hovered_menu_prev ||
          hovered_minimize_now != hovered_minimize_prev ||
          hovered_maximize_now != hovered_maximize_prev ||
          hovered_close_now != hovered_close_prev) {
        hovered_menu_prev = hovered_menu_now;
        hovered_minimize_prev = hovered_minimize_now;
        hovered_maximize_prev = hovered_maximize_now;
        hovered_close_prev = hovered_close_now;
        hover_state_valid = true;
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
          template_point_in_button(&layout, mouse_x, mouse_y, layout.menu_x);
      bool hovered_minimize =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.minimize_x);
      bool hovered_maximize =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.maximize_x);
      bool hovered_close =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.close_x);
      bool maximized = stygian_window_is_maximized(window);
      float base_icon = 0.94f;
      float menu_icon = hovered_menu ? 0.995f : 0.95f;
      float minimize_icon = hovered_minimize ? 0.995f : base_icon;
      float maximize_icon = hovered_maximize ? 0.995f : base_icon;
      float close_icon = hovered_close ? 1.0f : 0.965f;
      float max_r, max_g, max_b;
      template_button_fill(hovered_maximize, hovered_maximize && mouse_down,
                           false, &max_r, &max_g, &max_b);
      template_draw_caption_button_bg(ctx, &layout, layout.menu_x, hovered_menu,
                                      hovered_menu && mouse_down, false);
      template_draw_menu_glyph(ctx, layout.menu_x, layout.button_y,
                               layout.button_w, layout.button_h, menu_icon);
      template_draw_caption_button_bg(ctx, &layout, layout.minimize_x,
                                      hovered_minimize,
                                      hovered_minimize && mouse_down, false);
      template_draw_minimize_glyph(ctx, layout.minimize_x, layout.button_y,
                                   layout.button_w, layout.button_h,
                                   minimize_icon);
      template_draw_caption_button_bg(ctx, &layout, layout.maximize_x,
                                      hovered_maximize,
                                      hovered_maximize && mouse_down, false);
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
                                      hovered_close && mouse_down, true);
      template_draw_close_glyph(ctx, layout.close_x, layout.button_y,
                                layout.button_w, layout.button_h, close_icon);
      template_draw_title(ctx, font, &layout, STYGIAN_TEMPLATE_APP_TITLE);
    }
    stygian_scope_end(ctx);

    stygian_scope_invalidate_next(ctx, k_scope_shell);
    if (event_requested || event_mutated) {
      stygian_set_repaint_source(ctx, "template");
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
