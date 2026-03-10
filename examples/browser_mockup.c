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
  (STYGIAN_WINDOW_RESIZABLE | STYGIAN_WINDOW_VULKAN)
#define STYGIAN_BROWSER_RENDERER_NAME "Vulkan"
#else
#define STYGIAN_BROWSER_BACKEND STYGIAN_BACKEND_OPENGL
#define STYGIAN_BROWSER_WINDOW_FLAGS                                            \
  (STYGIAN_WINDOW_RESIZABLE | STYGIAN_WINDOW_OPENGL)
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
    bool event_mutated = false;
    bool event_requested = false;
    bool event_eval_requested = false;
    bool shell_changed = false;
    uint32_t wait_ms = stygian_next_repaint_wait_ms(ctx, 250u);

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
      float rail_y = 12.0f;
      float rail_w = 58.0f;
      float rail_h;
      float bar_x = 82.0f;
      float bar_y = 12.0f;
      float bar_w;
      float page_x = 82.0f;
      float page_y = 80.0f;
      float page_w;
      float page_h;

      if (!render_frame && !eval_only_frame)
        continue;
      first_frame = false;

      stygian_window_get_size(window, &width, &height);
      rail_h = (float)height - 24.0f;
      bar_w = (float)width - 94.0f;
      page_w = (float)width - 94.0f;
      page_h = (float)height - 92.0f;

      stygian_begin_frame_intent(
          ctx, width, height,
          eval_only_frame ? STYGIAN_FRAME_EVAL_ONLY : STYGIAN_FRAME_RENDER);

      stygian_scope_begin(ctx, k_scope_shell);
      stygian_rect(ctx, 0.0f, 0.0f, (float)width, (float)height, 0.07f, 0.07f,
                   0.08f, 1.0f);

      stygian_rect_rounded(ctx, rail_x, rail_y, rail_w, rail_h, 0.10f, 0.10f,
                           0.11f, 1.0f, 22.0f);
      stygian_rect_rounded(ctx, bar_x, bar_y, bar_w, 56.0f, 0.10f, 0.10f, 0.11f,
                           1.0f, 22.0f);
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
      browser_text(ctx, font, k_tabs[active_tab].title, page_x + 22.0f,
                   page_y + 12.0f, 14.0f, 0.92f, 0.92f, 0.93f, 1.0f);
      browser_text(ctx, font, STYGIAN_BROWSER_RENDERER_NAME, rail_x + 12.0f,
                   rail_y + rail_h - 28.0f, 10.0f, 0.56f, 0.56f, 0.59f, 1.0f);

      browser_text(ctx, font, "Address", bar_x + 24.0f, bar_y + 18.0f, 11.0f,
                   0.56f, 0.56f, 0.59f, 1.0f);
      if (stygian_text_input(ctx, font, bar_x + 92.0f, bar_y + 13.0f,
                             bar_w - 230.0f, 30.0f, address_buffer,
                             (int)sizeof(address_buffer))) {
        shell_changed = true;
      }
      if (browser_button(ctx, font, "Go", bar_x + bar_w - 122.0f, bar_y + 13.0f,
                         48.0f, 30.0f, &k_small_button)) {
        shell_changed = true;
      }
      if (browser_button(ctx, font, "F1", bar_x + bar_w - 64.0f, bar_y + 13.0f,
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
