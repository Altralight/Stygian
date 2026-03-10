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

static const BrowserTab k_tabs[] = {
    {.label = "H", .url = "https://fieldengine.space/", .title = "Home"},
    {.label = "B", .url = "https://fieldengine.space/bench", .title = "Bench"},
    {.label = "D", .url = "https://fieldengine.space/docs", .title = "Docs"},
    {.label = "C", .url = "https://fieldengine.space/capture", .title = "Capture"},
};

static int browser_tab_count(void) {
  return (int)(sizeof(k_tabs) / sizeof(k_tabs[0]));
}

static void browser_copy_text(char *dst, int cap, const char *src) {
  if (!dst || cap <= 0)
    return;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  snprintf(dst, (size_t)cap, "%s", src);
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

int main(void) {
  const StygianScopeId k_scope_shell = 0x4601u;
  const StygianScopeId k_scope_perf =
      STYGIAN_OVERLAY_SCOPE_BASE | (StygianScopeId)0x4602u;

  StygianWindowConfig win_cfg = {
      .title = "Aster Browser (Stygian Mockup)",
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
      stygian_font_load(ctx, "assets/atlas.png", "assets/atlas.json");
  StygianMiniPerfHarness perf;
  bool first_frame = true;
  bool show_perf = false;
  int active_tab = 0;
  char address_buffer[256];

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
  const StygianWidgetStyle k_small_button = {
      .bg_color = {0.12f, 0.12f, 0.13f, 1.0f},
      .hover_color = {0.16f, 0.16f, 0.17f, 1.0f},
      .active_color = {0.19f, 0.19f, 0.20f, 1.0f},
      .text_color = {0.90f, 0.90f, 0.92f, 1.0f},
      .border_radius = 12.0f,
      .padding = 8.0f,
  };

  stygian_mini_perf_init(&perf, "browser_mockup");
  perf.widget.renderer_name = STYGIAN_BROWSER_RENDERER_NAME;
  browser_copy_text(address_buffer, (int)sizeof(address_buffer),
                    k_tabs[active_tab].url);

  while (!stygian_window_should_close(window)) {
    StygianEvent event;
    StygianTitlebarHints titlebar_hints;
    bool event_mutated = false;
    bool event_requested = false;
    bool event_eval_requested = false;
    bool shell_changed = false;
    uint32_t wait_ms = stygian_next_repaint_wait_ms(ctx, 250u);
    float titlebar_height;
    float titlebar_button_w;
    float titlebar_button_h;
    float titlebar_button_gap;
    float titlebar_button_y;
    float close_x;
    float max_x;
    float min_x;
    float controls_start;
    float controls_end;
    float address_x;
    float address_w;

    stygian_window_get_titlebar_hints(window, &titlebar_hints);
    titlebar_height = titlebar_hints.recommended_titlebar_height > 0.0f
                          ? titlebar_hints.recommended_titlebar_height
                          : 36.0f;
    titlebar_button_w = titlebar_hints.recommended_button_width > 0.0f
                            ? titlebar_hints.recommended_button_width
                            : 28.0f;
    titlebar_button_h = titlebar_hints.recommended_button_height > 0.0f
                            ? titlebar_hints.recommended_button_height
                            : 24.0f;
    titlebar_button_gap = titlebar_hints.recommended_button_gap > 0.0f
                              ? titlebar_hints.recommended_button_gap
                              : 6.0f;
    titlebar_button_y = 12.0f + (titlebar_height - titlebar_button_h) * 0.5f;
    if (titlebar_hints.button_order == STYGIAN_TITLEBAR_BUTTONS_LEFT) {
      close_x = 16.0f;
      min_x = close_x + titlebar_button_w + titlebar_button_gap;
      max_x = min_x + titlebar_button_w + titlebar_button_gap;
      controls_start = close_x;
      controls_end = max_x + titlebar_button_w;
      address_x = controls_end + 16.0f;
    } else {
      close_x = 1440.0f; // overwritten once we know frame width
      min_x = 0.0f;
      max_x = 0.0f;
      controls_start = 0.0f;
      controls_end = 0.0f;
      address_x = 94.0f;
    }
    address_w = 0.0f;

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
        float mouse_x = (float)event.mouse_button.x;
        float mouse_y = (float)event.mouse_button.y;
        bool in_titlebar = browser_point_in_rect(mouse_x, mouse_y, 12.0f, 12.0f,
                                                 2000.0f, titlebar_height);
        bool in_controls = browser_point_in_rect(mouse_x, mouse_y, controls_start,
                                                 titlebar_button_y,
                                                 controls_end - controls_start,
                                                 titlebar_button_h);
        bool in_address = browser_point_in_rect(mouse_x, mouse_y, address_x,
                                                titlebar_button_y - 1.0f,
                                                address_w, 32.0f);
        if (in_titlebar && !in_controls && !in_address) {
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
          float mouse_x = (float)event.mouse_button.x;
          float mouse_y = (float)event.mouse_button.y;
          bool in_titlebar = browser_point_in_rect(mouse_x, mouse_y, 12.0f, 12.0f,
                                                   2000.0f, titlebar_height);
          bool in_controls = browser_point_in_rect(mouse_x, mouse_y, controls_start,
                                                   titlebar_button_y,
                                                   controls_end - controls_start,
                                                   titlebar_button_h);
          bool in_address = browser_point_in_rect(mouse_x, mouse_y, address_x,
                                                  titlebar_button_y - 1.0f,
                                                  address_w, 32.0f);
          if (in_titlebar && !in_controls && !in_address) {
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
      int width, height;
      float rail_x = 12.0f;
      float rail_y;
      float rail_w = 58.0f;
      float rail_h;
      float bar_x = 12.0f;
      float bar_y = 12.0f;
      float bar_w;
      float page_x = 82.0f;
      float page_y;
      float page_w;
      float page_h;

      if (!render_frame && !eval_only_frame)
        continue;
      first_frame = false;

      stygian_window_get_size(window, &width, &height);
      if (titlebar_hints.button_order == STYGIAN_TITLEBAR_BUTTONS_LEFT) {
        close_x = 16.0f;
        min_x = close_x + titlebar_button_w + titlebar_button_gap;
        max_x = min_x + titlebar_button_w + titlebar_button_gap;
        controls_start = close_x;
        controls_end = max_x + titlebar_button_w;
        address_x = controls_end + 16.0f;
      } else {
        close_x = (float)width - 16.0f - titlebar_button_w;
        max_x = close_x - titlebar_button_gap - titlebar_button_w;
        min_x = max_x - titlebar_button_gap - titlebar_button_w;
        controls_start = min_x;
        controls_end = close_x + titlebar_button_w;
        address_x = 90.0f;
      }
      address_w = (float)width - address_x - (controls_end - controls_start) - 34.0f;
      if (address_w < 220.0f)
        address_w = 220.0f;
      rail_y = 12.0f + titlebar_height + 10.0f;
      rail_h = (float)height - rail_y - 12.0f;
      bar_w = (float)width - 24.0f;
      page_w = (float)width - 94.0f;
      page_y = rail_y;
      page_h = (float)height - page_y - 12.0f;

      stygian_begin_frame_intent(
          ctx, width, height,
          eval_only_frame ? STYGIAN_FRAME_EVAL_ONLY : STYGIAN_FRAME_RENDER);

      stygian_scope_begin(ctx, k_scope_shell);
      stygian_rect(ctx, 0.0f, 0.0f, (float)width, (float)height, 0.07f, 0.07f,
                   0.08f, 1.0f);

      stygian_rect_rounded(ctx, bar_x, bar_y, bar_w, titlebar_height, 0.10f,
                           0.10f, 0.11f, 1.0f, 22.0f);
      stygian_rect_rounded(ctx, rail_x, rail_y, rail_w, rail_h, 0.10f, 0.10f,
                           0.11f, 1.0f, 22.0f);
      stygian_rect_rounded(ctx, page_x, page_y, page_w, page_h, 0.11f, 0.11f,
                           0.12f, 1.0f, 22.0f);
      stygian_rect_rounded(ctx, page_x + 1.0f, page_y + 1.0f, page_w - 2.0f,
                           38.0f, 0.13f, 0.13f, 0.14f, 1.0f, 21.0f);

      for (int i = 0; i < browser_tab_count(); ++i) {
        float tab_y = rail_y + 78.0f + (float)i * 56.0f;
        const StygianWidgetStyle *style =
            (i == active_tab) ? &k_rail_active : &k_rail_idle;
        if (browser_button(ctx, font, k_tabs[i].label, rail_x + 9.0f, tab_y,
                           40.0f, 40.0f, style)) {
          active_tab = i;
          browser_copy_text(address_buffer, (int)sizeof(address_buffer),
                            k_tabs[active_tab].url);
          shell_changed = true;
        }
      }

      browser_text(ctx, font, "A", rail_x + 21.0f, rail_y + 22.0f, 18.0f, 0.96f,
                   0.96f, 0.97f, 1.0f);
      browser_text(ctx, font, "Aster", bar_x + 18.0f, bar_y + 10.0f, 15.0f,
                   0.90f, 0.90f, 0.92f, 1.0f);
      browser_text(ctx, font, k_tabs[active_tab].title, page_x + 22.0f,
                   page_y + 12.0f, 14.0f, 0.92f, 0.92f, 0.93f, 1.0f);
      browser_text(ctx, font, STYGIAN_BROWSER_RENDERER_NAME, rail_x + 12.0f,
                   rail_y + rail_h - 28.0f, 10.0f, 0.56f, 0.56f, 0.59f, 1.0f);

      browser_text(ctx, font, "Address", address_x - 68.0f, bar_y + 11.0f, 11.0f,
                   0.56f, 0.56f, 0.59f, 1.0f);
      if (stygian_text_input(ctx, font, address_x, titlebar_button_y - 1.0f,
                             address_w, 30.0f, address_buffer,
                             (int)sizeof(address_buffer))) {
        shell_changed = true;
      }
      if (browser_button(ctx, font, "-", min_x, titlebar_button_y,
                         titlebar_button_w, titlebar_button_h, &k_small_button)) {
        shell_changed = true;
        stygian_window_minimize(window);
      }
      if (browser_button(ctx, font,
                         stygian_window_is_maximized(window) ? "R" : "[]", max_x,
                         titlebar_button_y, titlebar_button_w, titlebar_button_h,
                         &k_small_button)) {
        shell_changed = true;
        if (stygian_window_is_maximized(window)) {
          stygian_window_restore(window);
        } else {
          stygian_window_maximize(window);
        }
      }
      if (browser_button(ctx, font, "X", close_x, titlebar_button_y,
                         titlebar_button_w, titlebar_button_h, &k_small_button)) {
        shell_changed = true;
        stygian_window_request_close(window);
      }
      if (browser_button(ctx, font, "F1", rail_x + 9.0f, rail_y + rail_h - 58.0f,
                         40.0f, 30.0f, &k_small_button)) {
        show_perf = !show_perf;
        shell_changed = true;
      }

      browser_text(ctx, font, k_tabs[active_tab].url, page_x + 22.0f,
                   page_y + 54.0f, 12.0f, 0.58f, 0.58f, 0.61f, 1.0f);
      stygian_rect(ctx, page_x + 22.0f, page_y + 74.0f, page_w - 44.0f, 1.0f,
                   0.18f, 0.18f, 0.20f, 1.0f);
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
