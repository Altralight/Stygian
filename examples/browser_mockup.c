#include "../include/stygian.h"
#include "../widgets/stygian_widgets.h"
#include "../window/stygian_input.h"
#include "../window/stygian_window.h"
#include "mini_perf_harness.h"
#include <stdio.h>
#include <string.h>

#ifdef STYGIAN_DEMO_VULKAN
#define STYGIAN_BROWSER_BACKEND STYGIAN_BACKEND_VULKAN
#define STYGIAN_BROWSER_WINDOW_FLAGS                                            \
  (STYGIAN_WINDOW_RESIZABLE | STYGIAN_WINDOW_BORDERLESS |                   \
   STYGIAN_WINDOW_VULKAN)
#define STYGIAN_BROWSER_RENDERER_NAME "Vulkan"
#else
#define STYGIAN_BROWSER_BACKEND STYGIAN_BACKEND_OPENGL
#define STYGIAN_BROWSER_WINDOW_FLAGS                                            \
  (STYGIAN_WINDOW_RESIZABLE | STYGIAN_WINDOW_BORDERLESS |                  \
   STYGIAN_WINDOW_OPENGL)
#define STYGIAN_BROWSER_RENDERER_NAME "OpenGL"
#endif

typedef struct BrowserTab {
  const char *label;
  const char *url;
  const char *title;
} BrowserTab;

typedef struct BrowserLayout {
  float shell_x, shell_y, shell_w, shell_h;
  float shell_radius;
  float titlebar_h;
  float rail_x, rail_y, rail_w, rail_h;
  float page_x, page_y, page_w, page_h;
  float address_x, address_y, address_w, address_h;
  float min_x, max_x, close_x;
  float control_y, control_w, control_h, control_gap;
  float controls_start, controls_end;
} BrowserLayout;

typedef enum BrowserCaptionAction {
  BROWSER_CAPTION_NONE = 0,
  BROWSER_CAPTION_MINIMIZE,
  BROWSER_CAPTION_MAXIMIZE,
  BROWSER_CAPTION_CLOSE,
} BrowserCaptionAction;

static const BrowserTab k_tabs[] = {
    {.label = "H", .url = "https://fieldengine.space/", .title = "Home"},
    {.label = "B", .url = "https://fieldengine.space/bench", .title = "Bench"},
    {.label = "D", .url = "https://fieldengine.space/docs", .title = "Docs"},
    {.label = "C", .url = "https://fieldengine.space/capture", .title = "Capture"},
};

static int browser_tab_count(void) {
  return (int)(sizeof(k_tabs) / sizeof(k_tabs[0]));
}

static void browser_text(StygianContext *ctx, StygianFont font, const char *text,
                         float x, float y, float size, float r, float g,
                         float b, float a) {
  if (!font || !text || !text[0])
    return;
  stygian_text(ctx, font, text, x, y, size, r, g, b, a);
}

static bool browser_button(StygianContext *ctx, StygianFont font,
                           const char *label, float x, float y, float w,
                           float h, const StygianWidgetStyle *style) {
  StygianButton button = {
      .x = x,
      .y = y,
      .w = w,
      .h = h,
      .label = label,
  };
  return stygian_button_ex(ctx, font, &button, style);
}

static bool browser_point_in_rect(float px, float py, float x, float y, float w,
                                  float h) {
  return px >= x && px < (x + w) && py >= y && py < (y + h);
}

static void browser_compute_layout(BrowserLayout *layout, int width, int height,
                                   const StygianTitlebarHints *hints) {
  float shell_margin = 8.0f;
  float inner_pad = 10.0f;

  memset(layout, 0, sizeof(*layout));
  layout->shell_x = shell_margin;
  layout->shell_y = shell_margin;
  layout->shell_w = (float)width - shell_margin * 2.0f;
  layout->shell_h = (float)height - shell_margin * 2.0f;
  layout->shell_radius = 26.0f;
  layout->titlebar_h = hints->recommended_titlebar_height > 0.0f
                           ? hints->recommended_titlebar_height + 4.0f
                           : 44.0f;
  layout->control_w = hints->recommended_button_width > 0.0f
                          ? hints->recommended_button_width + 4.0f
                          : 34.0f;
  layout->control_h = hints->recommended_button_height > 0.0f
                          ? hints->recommended_button_height + 2.0f
                          : 28.0f;
  layout->control_gap = hints->recommended_button_gap > 0.0f
                            ? hints->recommended_button_gap + 2.0f
                            : 8.0f;
  layout->control_y =
      layout->shell_y + 10.0f + (layout->titlebar_h - layout->control_h) * 0.5f;

  if (hints->button_order == STYGIAN_TITLEBAR_BUTTONS_LEFT) {
    layout->close_x = layout->shell_x + 14.0f;
    layout->min_x =
        layout->close_x + layout->control_w + layout->control_gap;
    layout->max_x = layout->min_x + layout->control_w + layout->control_gap;
    layout->controls_start = layout->close_x;
    layout->controls_end = layout->max_x + layout->control_w;
    layout->address_x = layout->controls_end + 14.0f;
  } else {
    layout->close_x =
        layout->shell_x + layout->shell_w - 14.0f - layout->control_w;
    layout->max_x = layout->close_x - layout->control_gap - layout->control_w;
    layout->min_x = layout->max_x - layout->control_gap - layout->control_w;
    layout->controls_start = layout->min_x;
    layout->controls_end = layout->close_x + layout->control_w;
    layout->address_x = layout->shell_x + 78.0f;
  }

  layout->address_y = layout->shell_y + 10.0f;
  layout->address_h = 30.0f;
  layout->address_w = layout->controls_start - layout->address_x - 12.0f;
  if (layout->address_w < 240.0f)
    layout->address_w = 240.0f;

  layout->rail_x = layout->shell_x + inner_pad;
  layout->rail_y = layout->shell_y + layout->titlebar_h + inner_pad;
  layout->rail_w = 52.0f;
  layout->rail_h =
      layout->shell_h - layout->titlebar_h - inner_pad * 2.0f;

  layout->page_x = layout->rail_x + layout->rail_w + 10.0f;
  layout->page_y = layout->rail_y;
  layout->page_w = layout->shell_x + layout->shell_w - inner_pad - layout->page_x;
  layout->page_h = layout->shell_y + layout->shell_h - inner_pad - layout->page_y;
}

static BrowserCaptionAction browser_hit_caption_button(
    const BrowserLayout *layout, float px, float py) {
  if (browser_point_in_rect(px, py, layout->min_x, layout->control_y,
                            layout->control_w, layout->control_h)) {
    return BROWSER_CAPTION_MINIMIZE;
  }
  if (browser_point_in_rect(px, py, layout->max_x, layout->control_y,
                            layout->control_w, layout->control_h)) {
    return BROWSER_CAPTION_MAXIMIZE;
  }
  if (browser_point_in_rect(px, py, layout->close_x, layout->control_y,
                            layout->control_w, layout->control_h)) {
    return BROWSER_CAPTION_CLOSE;
  }
  return BROWSER_CAPTION_NONE;
}

static void browser_draw_caption_button(StygianContext *ctx, float x, float y,
                                        float w, float h, bool hovered,
                                        BrowserCaptionAction action,
                                        bool maximized) {
  float bg = hovered ? 0.18f : 0.12f;
  float icon = hovered ? 0.95f : 0.83f;
  float left = x + 10.0f;
  float right = x + w - 10.0f;
  float top = y + 8.0f;
  float bottom = y + h - 8.0f;
  float mid_y = y + h * 0.5f;

  if (action == BROWSER_CAPTION_CLOSE && hovered) {
    stygian_rect_rounded(ctx, x, y, w, h, 0.36f, 0.17f, 0.18f, 1.0f, 10.0f);
    icon = 0.98f;
  } else {
    stygian_rect_rounded(ctx, x, y, w, h, bg, bg, bg + 0.01f, 1.0f, 10.0f);
  }

  if (action == BROWSER_CAPTION_MINIMIZE) {
    stygian_line(ctx, left + 1.0f, mid_y + 3.0f, right - 1.0f, mid_y + 3.0f,
                 1.5f, icon, icon, icon, 1.0f);
  } else if (action == BROWSER_CAPTION_MAXIMIZE) {
    if (maximized) {
      stygian_line(ctx, left + 3.0f, top + 3.0f, right - 2.0f, top + 3.0f, 1.4f,
                   icon, icon, icon, 1.0f);
      stygian_line(ctx, right - 2.0f, top + 3.0f, right - 2.0f, bottom - 3.0f,
                   1.4f, icon, icon, icon, 1.0f);
      stygian_line(ctx, left + 3.0f, bottom - 3.0f, right - 2.0f,
                   bottom - 3.0f, 1.4f, icon, icon, icon, 1.0f);
      stygian_line(ctx, left + 3.0f, top + 3.0f, left + 3.0f, bottom - 3.0f,
                   1.4f, icon, icon, icon, 1.0f);
      stygian_line(ctx, left, top + 7.0f, right - 5.0f, top + 7.0f, 1.4f, icon,
                   icon, icon, 1.0f);
      stygian_line(ctx, left, top + 7.0f, left, bottom, 1.4f, icon, icon, icon,
                   1.0f);
    } else {
      stygian_line(ctx, left + 1.0f, top + 1.0f, right - 1.0f, top + 1.0f, 1.4f,
                   icon, icon, icon, 1.0f);
      stygian_line(ctx, right - 1.0f, top + 1.0f, right - 1.0f, bottom - 1.0f,
                   1.4f, icon, icon, icon, 1.0f);
      stygian_line(ctx, left + 1.0f, bottom - 1.0f, right - 1.0f,
                   bottom - 1.0f, 1.4f, icon, icon, icon, 1.0f);
      stygian_line(ctx, left + 1.0f, top + 1.0f, left + 1.0f, bottom - 1.0f,
                   1.4f, icon, icon, icon, 1.0f);
    }
  } else if (action == BROWSER_CAPTION_CLOSE) {
    stygian_line(ctx, left + 1.0f, top + 1.0f, right - 1.0f, bottom - 1.0f, 1.5f,
                 icon, icon, icon, 1.0f);
    stygian_line(ctx, right - 1.0f, top + 1.0f, left + 1.0f, bottom - 1.0f, 1.5f,
                 icon, icon, icon, 1.0f);
  }
}

int main(void) {
  const StygianScopeId k_scope_shell = 0x4601u;
  const StygianScopeId k_scope_perf =
      STYGIAN_OVERLAY_SCOPE_BASE | (StygianScopeId)0x4602u;

  StygianWindowConfig win_cfg = {
      .title = "Aster",
      .width = 1440,
      .height = 920,
      .flags = STYGIAN_BROWSER_WINDOW_FLAGS,
  };
  StygianWindow *window = stygian_window_create(&win_cfg);
  if (!window)
    return 1;
  {
    StygianTitlebarBehavior titlebar_behavior = {
        .double_click_mode = STYGIAN_TITLEBAR_DBLCLICK_MAXIMIZE_RESTORE,
        .hover_menu_enabled = false,
    };
    stygian_window_set_titlebar_behavior(window, &titlebar_behavior);
  }

  StygianConfig cfg = {.backend = STYGIAN_BROWSER_BACKEND, .window = window};
  StygianContext *ctx = stygian_create(&cfg);
  if (!ctx)
    return 1;
  stygian_set_vsync(ctx, false);

  StygianFont font =
      stygian_get_default_font(ctx);
  StygianMiniPerfHarness perf;
  bool first_frame = true;
  bool show_perf = false;
  int active_tab = 0;
  float mouse_x = 0.0f;
  float mouse_y = 0.0f;

  const StygianWidgetStyle k_rail_idle = {
      .bg_color = {0.12f, 0.12f, 0.13f, 1.0f},
      .hover_color = {0.16f, 0.16f, 0.17f, 1.0f},
      .active_color = {0.18f, 0.18f, 0.19f, 1.0f},
      .text_color = {0.82f, 0.82f, 0.84f, 1.0f},
      .border_radius = 14.0f,
      .padding = 8.0f,
  };
  const StygianWidgetStyle k_rail_active = {
      .bg_color = {0.26f, 0.26f, 0.28f, 1.0f},
      .hover_color = {0.28f, 0.28f, 0.30f, 1.0f},
      .active_color = {0.30f, 0.30f, 0.32f, 1.0f},
      .text_color = {0.97f, 0.97f, 0.98f, 1.0f},
      .border_radius = 14.0f,
      .padding = 8.0f,
  };

  stygian_mini_perf_init(&perf, "browser_mockup");
  perf.widget.renderer_name = STYGIAN_BROWSER_RENDERER_NAME;

  while (!stygian_window_should_close(window)) {
    StygianEvent event;
    StygianTitlebarHints titlebar_hints;
    BrowserLayout layout;
    bool event_mutated = false;
    bool event_requested = false;
    bool event_eval_requested = false;
    bool shell_changed = false;
    uint32_t wait_ms = stygian_next_repaint_wait_ms(ctx, 250u);
    int frame_width;
    int frame_height;

    stygian_window_get_titlebar_hints(window, &titlebar_hints);
    stygian_window_get_size(window, &frame_width, &frame_height);
    browser_compute_layout(&layout, frame_width, frame_height, &titlebar_hints);

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
      }
      if (event.type == STYGIAN_EVENT_KEY_DOWN && !event.key.repeat) {
        if (event.key.key == STYGIAN_KEY_F1) {
          show_perf = !show_perf;
          shell_changed = true;
          event_requested = true;
        } else if (event.key.key == STYGIAN_KEY_F11) {
          stygian_window_set_fullscreen(
              window, !stygian_window_is_fullscreen(window));
          event_requested = true;
        }
      } else if (event.type == STYGIAN_EVENT_MOUSE_DOWN &&
                 event.mouse_button.button == STYGIAN_MOUSE_LEFT) {
        BrowserCaptionAction action =
            browser_hit_caption_button(&layout, mouse_x, mouse_y);
        bool in_titlebar = browser_point_in_rect(mouse_x, mouse_y, layout.shell_x,
                                                 layout.shell_y, layout.shell_w,
                                                 layout.titlebar_h + 10.0f);
        if (action == BROWSER_CAPTION_MINIMIZE) {
          stygian_window_minimize(window);
          shell_changed = true;
          event_requested = true;
        } else if (action == BROWSER_CAPTION_MAXIMIZE) {
          if (stygian_window_is_maximized(window)) {
            stygian_window_restore(window);
          } else {
            stygian_window_maximize(window);
          }
          shell_changed = true;
          event_requested = true;
        } else if (action == BROWSER_CAPTION_CLOSE) {
          stygian_window_request_close(window);
          shell_changed = true;
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
      if (event.type == STYGIAN_EVENT_CLOSE)
        stygian_window_request_close(window);
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
        }
        if (event.type == STYGIAN_EVENT_KEY_DOWN && !event.key.repeat) {
          if (event.key.key == STYGIAN_KEY_F1) {
            show_perf = !show_perf;
            shell_changed = true;
            event_requested = true;
          } else if (event.key.key == STYGIAN_KEY_F11) {
            stygian_window_set_fullscreen(
                window, !stygian_window_is_fullscreen(window));
            event_requested = true;
          }
        } else if (event.type == STYGIAN_EVENT_MOUSE_DOWN &&
                   event.mouse_button.button == STYGIAN_MOUSE_LEFT) {
          BrowserCaptionAction action =
              browser_hit_caption_button(&layout, mouse_x, mouse_y);
          bool in_titlebar =
              browser_point_in_rect(mouse_x, mouse_y, layout.shell_x,
                                    layout.shell_y, layout.shell_w,
                                    layout.titlebar_h + 10.0f);
          if (action == BROWSER_CAPTION_MINIMIZE) {
            stygian_window_minimize(window);
            shell_changed = true;
            event_requested = true;
          } else if (action == BROWSER_CAPTION_MAXIMIZE) {
            if (stygian_window_is_maximized(window)) {
              stygian_window_restore(window);
            } else {
              stygian_window_maximize(window);
            }
            shell_changed = true;
            event_requested = true;
          } else if (action == BROWSER_CAPTION_CLOSE) {
            stygian_window_request_close(window);
            shell_changed = true;
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
        if (event.type == STYGIAN_EVENT_CLOSE)
          stygian_window_request_close(window);
      }
    }

    {
      bool showcase_redraw = show_perf;
      bool repaint_pending = stygian_has_pending_repaint(ctx);
      bool render_frame = first_frame || event_mutated || repaint_pending;
      bool eval_only_frame =
          (!render_frame && (event_eval_requested || event_requested));
      int width = frame_width;
      int height = frame_height;

      if (!render_frame && !eval_only_frame)
        continue;
      first_frame = false;

      stygian_window_get_size(window, &width, &height);
      browser_compute_layout(&layout, width, height, &titlebar_hints);

      stygian_begin_frame_intent(
          ctx, width, height,
          eval_only_frame ? STYGIAN_FRAME_EVAL_ONLY : STYGIAN_FRAME_RENDER);

      stygian_scope_begin(ctx, k_scope_shell);
      stygian_rect(ctx, 0.0f, 0.0f, (float)width, (float)height, 0.03f, 0.03f,
                   0.04f, 1.0f);
      stygian_rect_rounded(ctx, layout.shell_x, layout.shell_y, layout.shell_w,
                           layout.shell_h, 0.08f, 0.08f, 0.09f, 1.0f,
                           layout.shell_radius);
      stygian_rect_rounded(ctx, layout.rail_x, layout.rail_y, layout.rail_w,
                           layout.rail_h, 0.06f, 0.06f, 0.07f, 1.0f, 20.0f);
      stygian_rect_rounded(ctx, layout.page_x, layout.page_y, layout.page_w,
                           layout.page_h, 0.09f, 0.09f, 0.10f, 1.0f, 20.0f);
      stygian_rect_rounded(ctx, layout.address_x, layout.address_y,
                           layout.address_w, layout.address_h, 0.16f, 0.16f,
                           0.17f, 1.0f, 14.0f);

      for (int i = 0; i < browser_tab_count(); ++i) {
        float tab_y = layout.rail_y + 18.0f + (float)i * 54.0f;
        const StygianWidgetStyle *style =
            (i == active_tab) ? &k_rail_active : &k_rail_idle;
        if (browser_button(ctx, font, k_tabs[i].label, layout.rail_x + 6.0f,
                           tab_y,
                           40.0f, 40.0f, style)) {
          active_tab = i;
          shell_changed = true;
        }
      }

      browser_text(ctx, font, k_tabs[active_tab].url, layout.address_x + 12.0f,
                   layout.address_y + 7.0f, 13.0f, 0.84f, 0.85f, 0.86f, 1.0f);

      browser_draw_caption_button(
          ctx, layout.min_x, layout.control_y, layout.control_w, layout.control_h,
          browser_point_in_rect(mouse_x, mouse_y, layout.min_x, layout.control_y,
                                layout.control_w, layout.control_h),
          BROWSER_CAPTION_MINIMIZE, false);
      browser_draw_caption_button(
          ctx, layout.max_x, layout.control_y, layout.control_w, layout.control_h,
          browser_point_in_rect(mouse_x, mouse_y, layout.max_x, layout.control_y,
                                layout.control_w, layout.control_h),
          BROWSER_CAPTION_MAXIMIZE, stygian_window_is_maximized(window));
      browser_draw_caption_button(
          ctx, layout.close_x, layout.control_y, layout.control_w,
          layout.control_h,
          browser_point_in_rect(mouse_x, mouse_y, layout.close_x, layout.control_y,
                                layout.control_w, layout.control_h),
          BROWSER_CAPTION_CLOSE, false);
      stygian_scope_end(ctx);

      if (show_perf) {
        stygian_scope_begin(ctx, k_scope_perf);
        stygian_mini_perf_draw(ctx, font, &perf, width, height);
        stygian_scope_end(ctx);
      }

      if (shell_changed || showcase_redraw || event_mutated)
        stygian_scope_invalidate_next(ctx, k_scope_shell);
      if (!show_perf)
        stygian_scope_invalidate_next(ctx, k_scope_perf);
      if (show_perf)
        stygian_scope_invalidate_next(ctx, k_scope_perf);

      if (shell_changed || event_mutated || showcase_redraw) {
        stygian_set_repaint_source(ctx, showcase_redraw ? "showcase" : "mutation");
        stygian_request_repaint_after_ms(ctx, 0u);
      }

      stygian_widgets_commit_regions();
      stygian_end_frame(ctx);
      stygian_mini_perf_accumulate(&perf, eval_only_frame);
      stygian_mini_perf_log(ctx, &perf);
    }
  }

  if (font)
    stygian_font_destroy(ctx, font);
  stygian_destroy(ctx);
  stygian_window_destroy(window);
  return 0;
}
