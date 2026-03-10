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

typedef struct BrowserPage {
  const char *tab_label;
  const char *url;
  const char *title;
  const char *subtitle;
  const char *body_lines[3];
  const char *side_label;
  const char *side_value;
} BrowserPage;

static const BrowserPage k_pages[] = {
    {
        .tab_label = "Home",
        .url = "https://fieldengine.space/",
        .title = "Field Engine",
        .subtitle = "A quiet browser shell for native tooling.",
        .body_lines = {
            "Pinned capture sessions, build notes, and quick links live here.",
            "No fake browser engine, no fake tabs, just a useful shell.",
            "The whole point is native feel without web-stack baggage.",
        },
        .side_label = "Pinned",
        .side_value = "3 tabs",
    },
    {
        .tab_label = "Bench",
        .url = "https://fieldengine.space/bench",
        .title = "Bench Notes",
        .subtitle = "Native lane first. Everything else stays labeled.",
        .body_lines = {
            "OpenGL on the HD 4600 is finally in respectable territory.",
            "Vulkan on the GTX 860M has the headroom to actually look unfair.",
            "The benchmark story stops lying when the lanes stay separate.",
        },
        .side_label = "Latest",
        .side_value = "1 draw call",
    },
    {
        .tab_label = "Docs",
        .url = "https://fieldengine.space/docs",
        .title = "Protocol Notes",
        .subtitle = "Collect, commit, evaluate, render or skip.",
        .body_lines = {
            "This page is where architecture should be readable, not theatrical.",
            "The docs have enough signal now that they can carry the proposal.",
            "That took longer than it should have, but here we are.",
        },
        .side_label = "Status",
        .side_value = "CI green",
    },
    {
        .tab_label = "Capture",
        .url = "https://fieldengine.space/capture",
        .title = "Capture Queue",
        .subtitle = "Screenshots, traces, and the bits worth keeping.",
        .body_lines = {
            "Node graph, browser shell, chat shell, terminal shell.",
            "The small apps matter because they make the engine feel real.",
            "Pretty numbers alone are never enough.",
        },
        .side_label = "Queue",
        .side_value = "4 items",
    },
};

static int browser_page_count(void) {
  return (int)(sizeof(k_pages) / sizeof(k_pages[0]));
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

static void browser_divider(StygianContext *ctx, float x, float y, float w) {
  stygian_rect(ctx, x, y, w, 1.0f, 0.20f, 0.22f, 0.25f, 1.0f);
}

static void browser_stat(StygianContext *ctx, StygianFont font, float x, float y,
                         float w, const char *label, const char *value) {
  stygian_rect_rounded(ctx, x, y, w, 86.0f, 0.13f, 0.14f, 0.16f, 1.0f, 16.0f);
  browser_text(ctx, font, label, x + 18.0f, y + 18.0f, 12.0f, 0.62f, 0.64f, 0.68f,
               1.0f);
  browser_text(ctx, font, value, x + 18.0f, y + 42.0f, 24.0f, 0.96f, 0.96f, 0.97f,
               1.0f);
}

static void browser_line_block(StygianContext *ctx, StygianFont font, float x,
                               float y, const char *text) {
  browser_text(ctx, font, text, x, y, 14.0f, 0.78f, 0.79f, 0.82f, 1.0f);
}

static void browser_surface(StygianContext *ctx, StygianFont font,
                            const BrowserPage *page, float x, float y, float w,
                            float h) {
  float body_x = x + 30.0f;
  float side_x = x + w - 260.0f;
  stygian_rect_rounded(ctx, x, y, w, h, 0.09f, 0.10f, 0.12f, 1.0f, 24.0f);
  stygian_rect_rounded(ctx, x + 1.0f, y + 1.0f, w - 2.0f, 44.0f, 0.12f, 0.13f,
                       0.15f, 1.0f, 23.0f);
  browser_text(ctx, font, page->title, body_x, y + 22.0f, 24.0f, 0.97f, 0.97f,
               0.98f, 1.0f);
  browser_text(ctx, font, page->subtitle, body_x, y + 60.0f, 14.0f, 0.70f, 0.71f,
               0.75f, 1.0f);

  browser_stat(ctx, font, body_x, y + 104.0f, 188.0f, "Renderer",
               STYGIAN_BROWSER_RENDERER_NAME);
  browser_stat(ctx, font, body_x + 202.0f, y + 104.0f, 188.0f, page->side_label,
               page->side_value);
  browser_stat(ctx, font, body_x + 404.0f, y + 104.0f, 188.0f, "Theme",
               "Ash / Graphite");

  browser_divider(ctx, body_x, y + 220.0f, w - 60.0f);

  stygian_rect_rounded(ctx, body_x, y + 244.0f, w - 320.0f, 212.0f, 0.12f, 0.13f,
                       0.15f, 1.0f, 20.0f);
  browser_text(ctx, font, "Session", body_x + 22.0f, y + 266.0f, 13.0f, 0.60f,
               0.62f, 0.66f, 1.0f);
  browser_line_block(ctx, font, body_x + 22.0f, y + 302.0f, page->body_lines[0]);
  browser_line_block(ctx, font, body_x + 22.0f, y + 334.0f, page->body_lines[1]);
  browser_line_block(ctx, font, body_x + 22.0f, y + 366.0f, page->body_lines[2]);

  stygian_rect_rounded(ctx, side_x, y + 244.0f, 230.0f, 212.0f, 0.12f, 0.13f,
                       0.15f, 1.0f, 20.0f);
  browser_text(ctx, font, "Saved", side_x + 18.0f, y + 266.0f, 13.0f, 0.60f,
               0.62f, 0.66f, 1.0f);
  browser_text(ctx, font, "node_graph_demo.png", side_x + 18.0f, y + 304.0f,
               14.0f, 0.94f, 0.94f, 0.95f, 1.0f);
  browser_text(ctx, font, "browser_mockup", side_x + 18.0f, y + 336.0f, 14.0f,
               0.94f, 0.94f, 0.95f, 1.0f);
  browser_text(ctx, font, "comparison/latest", side_x + 18.0f, y + 368.0f,
               14.0f, 0.94f, 0.94f, 0.95f, 1.0f);
  browser_text(ctx, font, "release checklist", side_x + 18.0f, y + 400.0f,
               14.0f, 0.94f, 0.94f, 0.95f, 1.0f);

  stygian_rect_rounded(ctx, body_x, y + 476.0f, w - 60.0f, h - 508.0f, 0.12f,
                       0.13f, 0.15f, 1.0f, 20.0f);
  browser_text(ctx, font, "Notes", body_x + 22.0f, y + 498.0f, 13.0f, 0.60f,
               0.62f, 0.66f, 1.0f);
  browser_text(ctx, font,
               "Keep this shell simple. It is here to sell the runtime, not to "
               "cosplay as a full browser.",
               body_x + 22.0f, y + 536.0f, 15.0f, 0.80f, 0.80f, 0.83f, 1.0f);
}

int main(void) {
  const StygianScopeId k_scope_chrome = 0x4601u;
  const StygianScopeId k_scope_page = 0x4602u;
  const StygianScopeId k_scope_perf =
      STYGIAN_OVERLAY_SCOPE_BASE | (StygianScopeId)0x4603u;

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
  int active_page = 0;
  char address_buffer[256];
  const StygianWidgetStyle k_tab_idle = {
      .bg_color = {0.13f, 0.14f, 0.16f, 1.0f},
      .hover_color = {0.16f, 0.17f, 0.19f, 1.0f},
      .active_color = {0.18f, 0.19f, 0.21f, 1.0f},
      .text_color = {0.82f, 0.83f, 0.86f, 1.0f},
      .border_radius = 14.0f,
      .padding = 8.0f,
  };
  const StygianWidgetStyle k_tab_active = {
      .bg_color = {0.22f, 0.23f, 0.25f, 1.0f},
      .hover_color = {0.24f, 0.25f, 0.27f, 1.0f},
      .active_color = {0.26f, 0.27f, 0.29f, 1.0f},
      .text_color = {0.97f, 0.97f, 0.98f, 1.0f},
      .border_radius = 14.0f,
      .padding = 8.0f,
  };
  const StygianWidgetStyle k_small_button = {
      .bg_color = {0.13f, 0.14f, 0.16f, 1.0f},
      .hover_color = {0.16f, 0.17f, 0.19f, 1.0f},
      .active_color = {0.20f, 0.21f, 0.23f, 1.0f},
      .text_color = {0.90f, 0.90f, 0.92f, 1.0f},
      .border_radius = 12.0f,
      .padding = 8.0f,
  };

  stygian_mini_perf_init(&perf, "browser_mockup");
  perf.widget.renderer_name = STYGIAN_BROWSER_RENDERER_NAME;
  browser_copy_text(address_buffer, (int)sizeof(address_buffer),
                    k_pages[active_page].url);

  while (!stygian_window_should_close(window)) {
    StygianEvent event;
    bool event_mutated = false;
    bool event_requested = false;
    bool event_eval_requested = false;
    bool chrome_changed = false;
    bool page_changed = false;
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
          chrome_changed = true;
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
            chrome_changed = true;
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
      float chrome_x = 20.0f;
      float chrome_y = 20.0f;
      float chrome_w;
      float page_x;
      float page_y;
      float page_w;
      float page_h;

      if (!render_frame && !eval_only_frame)
        continue;
      first_frame = false;

      stygian_window_get_size(window, &width, &height);
      chrome_w = (float)width - 40.0f;
      page_x = 20.0f;
      page_y = 150.0f;
      page_w = (float)width - 40.0f;
      page_h = (float)height - 170.0f;

      stygian_begin_frame_intent(
          ctx, width, height,
          eval_only_frame ? STYGIAN_FRAME_EVAL_ONLY : STYGIAN_FRAME_RENDER);

      stygian_scope_begin(ctx, k_scope_chrome);
      stygian_rect(ctx, 0.0f, 0.0f, (float)width, (float)height, 0.06f, 0.06f,
                   0.07f, 1.0f);
      stygian_rect_rounded(ctx, chrome_x, chrome_y, chrome_w, 116.0f, 0.10f,
                           0.11f, 0.13f, 1.0f, 24.0f);
      browser_text(ctx, font, "Aster", chrome_x + 24.0f, chrome_y + 22.0f, 22.0f,
                   0.97f, 0.97f, 0.98f, 1.0f);
      browser_text(ctx, font, "browser shell mockup", chrome_x + 94.0f,
                   chrome_y + 26.0f, 12.0f, 0.62f, 0.63f, 0.67f, 1.0f);

      for (int i = 0; i < browser_page_count(); ++i) {
        float tab_x = chrome_x + 22.0f + (float)i * 104.0f;
        const StygianWidgetStyle *style =
            (i == active_page) ? &k_tab_active : &k_tab_idle;
        if (browser_button(ctx, font, k_pages[i].tab_label, tab_x,
                           chrome_y + 58.0f, 90.0f, 30.0f, style)) {
          active_page = i;
          browser_copy_text(address_buffer, (int)sizeof(address_buffer),
                            k_pages[i].url);
          chrome_changed = true;
          page_changed = true;
        }
      }

      if (stygian_text_input(ctx, font, chrome_x + 454.0f, chrome_y + 58.0f,
                             chrome_w - 620.0f, 30.0f, address_buffer,
                             (int)sizeof(address_buffer))) {
        chrome_changed = true;
      }
      if (browser_button(ctx, font, "Go", chrome_x + chrome_w - 150.0f,
                         chrome_y + 58.0f, 54.0f, 30.0f, &k_small_button)) {
        chrome_changed = true;
      }
      if (browser_button(ctx, font, show_perf ? "Perf" : "Perf",
                         chrome_x + chrome_w - 86.0f, chrome_y + 58.0f, 54.0f,
                         30.0f, &k_small_button)) {
        show_perf = !show_perf;
        chrome_changed = true;
      }
      stygian_scope_end(ctx);

      stygian_scope_begin(ctx, k_scope_page);
      browser_surface(ctx, font, &k_pages[active_page], page_x, page_y, page_w,
                      page_h);
      stygian_scope_end(ctx);

      if (show_perf) {
        stygian_scope_begin(ctx, k_scope_perf);
        stygian_mini_perf_draw(ctx, font, &perf, width, height);
        stygian_scope_end(ctx);
      }

      if (chrome_changed || showcase_redraw)
        stygian_scope_invalidate_next(ctx, k_scope_chrome);
      if (page_changed || showcase_redraw || event_mutated)
        stygian_scope_invalidate_next(ctx, k_scope_page);
      if (!show_perf)
        stygian_scope_invalidate_next(ctx, k_scope_perf);
      if (show_perf)
        stygian_scope_invalidate_next(ctx, k_scope_perf);

      if (chrome_changed || page_changed || event_mutated || showcase_redraw) {
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
