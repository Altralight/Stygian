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
  const char *nav_label;
  const char *url;
  const char *title;
  const char *subtitle;
  const char *chips[3];
  float accent_r;
  float accent_g;
  float accent_b;
} BrowserPage;

static const BrowserPage k_pages[] = {
    {
        .tab_label = "Atlas",
        .nav_label = "Field Engine",
        .url = "https://fieldengine.space/atlas",
        .title = "Atlas Workspace",
        .subtitle =
            "A native command center for capture streams, field notes, and "
            "build telemetry.",
        .chips = {"Pinned", "Live", "Native"},
        .accent_r = 0.21f,
        .accent_g = 0.60f,
        .accent_b = 0.98f,
    },
    {
        .tab_label = "Bench",
        .nav_label = "Bench Lab",
        .url = "https://fieldengine.space/bench",
        .title = "Bench Lab",
        .subtitle =
            "Fast numbers, ugly truths, and enough telemetry to catch the lies "
            "before they fossilize.",
        .chips = {"GPU-native", "SDF", "Replay"},
        .accent_r = 0.27f,
        .accent_g = 0.86f,
        .accent_b = 0.62f,
    },
    {
        .tab_label = "Studio",
        .nav_label = "Northern Light",
        .url = "https://fieldengine.space/studio",
        .title = "Northern Light",
        .subtitle =
            "A visual gallery of material studies, editorial layouts, and "
            "shipping ideas that survived contact with real code.",
        .chips = {"Gallery", "Color", "Layout"},
        .accent_r = 0.93f,
        .accent_g = 0.54f,
        .accent_b = 0.24f,
    },
    {
        .tab_label = "Docs",
        .nav_label = "Protocol Notes",
        .url = "https://fieldengine.space/docs/runtime-model",
        .title = "Protocol Notes",
        .subtitle =
            "Runtime notes, integration guides, and enough structure to make "
            "the architecture readable without sanding off its edge.",
        .chips = {"Docs", "Architecture", "Guide"},
        .accent_r = 0.77f,
        .accent_g = 0.52f,
        .accent_b = 0.98f,
    },
};

static void browser_copy_text(char *dst, int cap, const char *src) {
  if (!dst || cap <= 0)
    return;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  snprintf(dst, (size_t)cap, "%s", src);
}

static int browser_page_count(void) {
  return (int)(sizeof(k_pages) / sizeof(k_pages[0]));
}

static void browser_text(StygianContext *ctx, StygianFont font, const char *text,
                         float x, float y, float size, float r, float g,
                         float b, float a) {
  if (!font || !text || !text[0])
    return;
  stygian_text(ctx, font, text, x, y, size, r, g, b, a);
}

static void browser_text_lines(StygianContext *ctx, StygianFont font,
                               const char *text, float x, float y, float size,
                               float line_gap, float r, float g, float b,
                               float a) {
  const char *line = text;
  if (!font || !text || !text[0])
    return;
  while (*line) {
    const char *end = strchr(line, '\n');
    char buffer[256];
    size_t len;
    if (!end)
      end = line + strlen(line);
    len = (size_t)(end - line);
    if (len >= sizeof(buffer))
      len = sizeof(buffer) - 1u;
    memcpy(buffer, line, len);
    buffer[len] = '\0';
    stygian_text(ctx, font, buffer, x, y, size, r, g, b, a);
    y += size + line_gap;
    if (!*end)
      break;
    line = end + 1;
  }
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

static void browser_chip(StygianContext *ctx, StygianFont font, const char *text,
                         float x, float y, float r, float g, float b) {
  float width;
  if (!text || !text[0])
    return;
  width = stygian_text_width(ctx, font, text, 12.0f) + 24.0f;
  stygian_rect_rounded(ctx, x, y, width, 26.0f, r, g, b, 0.18f, 13.0f);
  stygian_rect_rounded(ctx, x + 1.0f, y + 1.0f, width - 2.0f, 24.0f, 0.05f,
                       0.07f, 0.10f, 0.86f, 12.0f);
  browser_text(ctx, font, text, x + 12.0f, y + 6.0f, 12.0f, 0.90f, 0.93f, 0.98f,
               1.0f);
}

static void browser_metric_card(StygianContext *ctx, StygianFont font, float x,
                                float y, float w, float h, const char *label,
                                const char *value, float r, float g, float b) {
  stygian_rect_rounded(ctx, x, y, w, h, 0.10f, 0.12f, 0.16f, 0.96f, 18.0f);
  stygian_rect_rounded(ctx, x + 1.0f, y + 1.0f, w - 2.0f, 10.0f, r, g, b, 0.75f,
                       17.0f);
  browser_text(ctx, font, label, x + 18.0f, y + 24.0f, 12.0f, 0.56f, 0.66f,
               0.80f, 1.0f);
  browser_text(ctx, font, value, x + 18.0f, y + 46.0f, 22.0f, 0.96f, 0.97f,
               0.99f, 1.0f);
}

static void browser_story_card(StygianContext *ctx, StygianFont font, float x,
                               float y, float w, float h, const char *eyebrow,
                               const char *title, const char *body, float r,
                               float g, float b) {
  stygian_rect_rounded(ctx, x, y, w, h, 0.09f, 0.11f, 0.14f, 0.98f, 20.0f);
  stygian_rect_rounded(ctx, x + 14.0f, y + 16.0f, 54.0f, 8.0f, r, g, b, 0.85f,
                       4.0f);
  browser_text(ctx, font, eyebrow, x + 14.0f, y + 34.0f, 12.0f, 0.60f, 0.72f,
               0.86f, 1.0f);
  browser_text(ctx, font, title, x + 14.0f, y + 58.0f, 20.0f, 0.95f, 0.96f,
               0.99f, 1.0f);
  browser_text_lines(ctx, font, body, x + 14.0f, y + 92.0f, 13.0f, 6.0f, 0.70f,
                     0.78f, 0.88f, 1.0f);
}

static void browser_gallery_tile(StygianContext *ctx, StygianFont font, float x,
                                 float y, float w, float h, const char *title,
                                 const char *caption, float r, float g,
                                 float b) {
  stygian_rect_rounded(ctx, x, y, w, h, r, g, b, 0.20f, 22.0f);
  stygian_rect_rounded(ctx, x + 8.0f, y + 8.0f, w - 16.0f, h - 16.0f, 0.08f,
                       0.09f, 0.12f, 0.92f, 18.0f);
  stygian_rect_rounded(ctx, x + 18.0f, y + 18.0f, w - 36.0f, h * 0.52f, r, g,
                       b, 0.28f, 16.0f);
  browser_text(ctx, font, title, x + 18.0f, y + h - 56.0f, 18.0f, 0.95f, 0.96f,
               0.99f, 1.0f);
  browser_text(ctx, font, caption, x + 18.0f, y + h - 28.0f, 12.0f, 0.68f, 0.75f,
               0.84f, 1.0f);
}

static void browser_sync_address(char *buffer, int cap, int page_index) {
  if (page_index < 0 || page_index >= browser_page_count())
    return;
  browser_copy_text(buffer, cap, k_pages[page_index].url);
}

static void browser_draw_home(StygianContext *ctx, StygianFont font,
                              const BrowserPage *page, float x, float y,
                              float w, float h) {
  float chip_x = x + 34.0f;
  stygian_rect_rounded(ctx, x, y, w, h, 0.07f, 0.09f, 0.12f, 0.98f, 28.0f);
  stygian_rect_rounded(ctx, x + 22.0f, y + 22.0f, w - 44.0f, 166.0f, 0.06f,
                       0.07f, 0.10f, 1.0f, 24.0f);
  stygian_rect_rounded(ctx, x + 22.0f, y + 22.0f, w - 44.0f, 10.0f,
                       page->accent_r, page->accent_g, page->accent_b, 0.82f,
                       24.0f);
  browser_text(ctx, font, "Workspace", x + 34.0f, y + 42.0f, 13.0f, 0.58f, 0.70f,
               0.84f, 1.0f);
  browser_text(ctx, font, page->title, x + 34.0f, y + 68.0f, 32.0f, 0.97f,
               0.98f, 0.99f, 1.0f);
  browser_text(ctx, font, page->subtitle, x + 34.0f, y + 112.0f, 15.0f, 0.74f,
               0.80f, 0.88f, 1.0f);
  for (int i = 0; i < 3; ++i) {
    browser_chip(ctx, font, page->chips[i], chip_x, y + 150.0f, page->accent_r,
                 page->accent_g, page->accent_b);
    chip_x += stygian_text_width(ctx, font, page->chips[i], 12.0f) + 36.0f;
  }

  browser_story_card(ctx, font, x + 22.0f, y + 212.0f, w * 0.54f - 28.0f,
                     164.0f, "Pinned session", "Field notes and snapshot bundles",
                     "A tight browser shell makes native tooling feel like it has "
                     "a pulse instead of just a panel grid.",
                     page->accent_r, page->accent_g, page->accent_b);
  browser_story_card(ctx, font, x + w * 0.54f + 6.0f, y + 212.0f,
                     w * 0.46f - 28.0f, 164.0f, "Downloads", "Three fresh captures",
                     "1. node_graph_demo.png\n2. browser_mockup\n3. perf notes",
                     0.28f, 0.76f, 0.60f);

  browser_metric_card(ctx, font, x + 22.0f, y + 398.0f, (w - 60.0f) / 3.0f,
                      114.0f, "Frames", "742 FPS", page->accent_r,
                      page->accent_g, page->accent_b);
  browser_metric_card(ctx, font, x + 30.0f + (w - 60.0f) / 3.0f, y + 398.0f,
                      (w - 60.0f) / 3.0f, 114.0f, "Dirty upload", "0 KB",
                      0.25f, 0.74f, 0.58f);
  browser_metric_card(ctx, font, x + 38.0f + 2.0f * ((w - 60.0f) / 3.0f),
                      y + 398.0f, (w - 60.0f) / 3.0f, 114.0f, "Theme", "Aster",
                      0.92f, 0.58f, 0.28f);
}

static void browser_draw_bench(StygianContext *ctx, StygianFont font,
                               const BrowserPage *page, float x, float y,
                               float w, float h) {
  float chart_x = x + 22.0f;
  float chart_y = y + 22.0f;
  float chart_w = w * 0.58f;
  float chart_h = h - 44.0f;
  float right_x = x + chart_w + 34.0f;
  stygian_rect_rounded(ctx, x, y, w, h, 0.07f, 0.09f, 0.12f, 0.98f, 28.0f);
  stygian_rect_rounded(ctx, chart_x, chart_y, chart_w, chart_h, 0.08f, 0.10f,
                       0.13f, 1.0f, 24.0f);
  browser_text(ctx, font, "Native lane", chart_x + 20.0f, chart_y + 20.0f,
               13.0f, 0.58f, 0.70f, 0.84f, 1.0f);
  browser_text(ctx, font, "OpenGL and Vulkan stay in their own lane here.",
               chart_x + 20.0f, chart_y + 46.0f, 18.0f, 0.95f, 0.96f, 0.99f,
               1.0f);

  for (int i = 0; i < 5; ++i) {
    float bar_x = chart_x + 30.0f + (float)i * 88.0f;
    float bar_h = 72.0f + (float)(i * 26);
    stygian_rect_rounded(ctx, bar_x, chart_y + chart_h - bar_h - 34.0f, 42.0f,
                         bar_h, page->accent_r, page->accent_g, page->accent_b,
                         0.30f + 0.10f * (float)i, 12.0f);
    stygian_rect_rounded(ctx, bar_x + 50.0f,
                         chart_y + chart_h - (bar_h * 0.72f) - 34.0f, 20.0f,
                         bar_h * 0.72f, 0.25f, 0.72f, 0.58f,
                         0.24f + 0.08f * (float)i, 10.0f);
  }
  browser_story_card(ctx, font, right_x, chart_y, w - chart_w - 56.0f, 156.0f,
                     "Observed", "HD 4600 still refuses to die",
                     "That is the whole point of this thing. Native GPU work on "
                     "old hardware should still feel unfair.",
                     page->accent_r, page->accent_g, page->accent_b);
  browser_metric_card(ctx, font, right_x, chart_y + 176.0f, w - chart_w - 56.0f,
                      96.0f, "Vulkan / GTX 860M", "1886 FPS", 0.25f, 0.76f,
                      0.58f);
  browser_metric_card(ctx, font, right_x, chart_y + 286.0f, w - chart_w - 56.0f,
                      96.0f, "OpenGL / HD 4600", "662 FPS", page->accent_r,
                      page->accent_g, page->accent_b);
  browser_metric_card(ctx, font, right_x, chart_y + 396.0f, w - chart_w - 56.0f,
                      96.0f, "Draw calls", "1", 0.92f, 0.58f, 0.28f);
}

static void browser_draw_gallery(StygianContext *ctx, StygianFont font,
                                 const BrowserPage *page, float x, float y,
                                 float w, float h) {
  stygian_rect_rounded(ctx, x, y, w, h, 0.07f, 0.09f, 0.12f, 0.98f, 28.0f);
  browser_gallery_tile(ctx, font, x + 22.0f, y + 22.0f, w * 0.44f, 248.0f,
                       "Material study", "Warm chrome over midnight panels",
                       page->accent_r, page->accent_g, page->accent_b);
  browser_gallery_tile(ctx, font, x + w * 0.44f + 34.0f, y + 22.0f,
                       w * 0.56f - 56.0f, 162.0f, "Landing draft",
                       "A sharper front page for native tools", 0.20f, 0.60f,
                       0.98f);
  browser_gallery_tile(ctx, font, x + w * 0.44f + 34.0f, y + 198.0f,
                       w * 0.56f - 56.0f, 248.0f, "Motion sketch",
                       "Calm surfaces with louder accents where it counts", 0.27f,
                       0.76f, 0.58f);
  browser_gallery_tile(ctx, font, x + 22.0f, y + 286.0f, w * 0.28f, 180.0f,
                       "Palette", "Amber, cyan, steel, ink", 0.94f, 0.56f,
                       0.24f);
  browser_gallery_tile(ctx, font, x + w * 0.28f + 34.0f, y + 468.0f,
                       w * 0.72f - 56.0f, h - 490.0f, "Content frame",
                       "The nice part of a browser mockup is the lie can still "
                       "feel useful.", 0.76f, 0.50f, 0.96f);
}

static void browser_draw_docs(StygianContext *ctx, StygianFont font,
                              const BrowserPage *page, float x, float y,
                              float w, float h) {
  float left_w = w * 0.66f;
  stygian_rect_rounded(ctx, x, y, w, h, 0.07f, 0.09f, 0.12f, 0.98f, 28.0f);
  browser_story_card(ctx, font, x + 22.0f, y + 22.0f, left_w - 34.0f, 136.0f,
                     "Runtime model", "Collect -> Commit -> Evaluate -> Render",
                     "This is the page people should land on when they want the "
                     "architecture without the sales fog.", page->accent_r,
                     page->accent_g, page->accent_b);
  browser_story_card(ctx, font, x + 22.0f, y + 176.0f, left_w - 34.0f, 136.0f,
                     "Benchmarks", "Keep the lanes honest",
                     "Native GPU numbers lead. CPU-builder comparisons stay "
                     "secondary and clearly labeled.", 0.20f, 0.60f, 0.98f);
  browser_story_card(ctx, font, x + 22.0f, y + 330.0f, left_w - 34.0f, 136.0f,
                     "Integration", "Drop-in C23, no browser engine baggage",
                     "That line matters because reviewers have seen plenty of "
                     "tooling stories that secretly end in Chromium again.",
                     0.27f, 0.76f, 0.58f);
  browser_metric_card(ctx, font, x + left_w + 8.0f, y + 22.0f, w - left_w - 30.0f,
                      104.0f, "Open issues", "12", page->accent_r,
                      page->accent_g, page->accent_b);
  browser_metric_card(ctx, font, x + left_w + 8.0f, y + 142.0f,
                      w - left_w - 30.0f, 104.0f, "CI", "3 / 3 green", 0.25f,
                      0.76f, 0.58f);
  browser_metric_card(ctx, font, x + left_w + 8.0f, y + 262.0f,
                      w - left_w - 30.0f, 104.0f, "Tag", "v0.1.0", 0.92f, 0.58f,
                      0.28f);
  browser_story_card(ctx, font, x + left_w + 8.0f, y + 382.0f,
                     w - left_w - 30.0f, 154.0f, "Pinned",
                     "Write the form like a builder, not a prophet",
                     "The code already does enough talking. The proposal just has "
                     "to stop getting in the way.", 0.76f, 0.50f, 0.96f);
}

int main(void) {
  const StygianScopeId k_scope_shell = 0x4601u;
  const StygianScopeId k_scope_sidebar = 0x4602u;
  const StygianScopeId k_scope_page = 0x4603u;
  const StygianScopeId k_scope_perf =
      STYGIAN_OVERLAY_SCOPE_BASE | (StygianScopeId)0x4604u;

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
  bool compact_sidebar = false;
  bool zen_mode = false;
  int active_page = 0;
  char address_buffer[256];
  char status_line[128];
  bool page_bookmarked[8] = {true, true, false, true};

  const StygianWidgetStyle k_tab_idle = {
      .bg_color = {0.09f, 0.11f, 0.15f, 0.95f},
      .hover_color = {0.12f, 0.15f, 0.20f, 1.0f},
      .active_color = {0.17f, 0.22f, 0.30f, 1.0f},
      .text_color = {0.82f, 0.88f, 0.96f, 1.0f},
      .border_radius = 16.0f,
      .padding = 10.0f,
  };
  const StygianWidgetStyle k_tab_active = {
      .bg_color = {0.18f, 0.25f, 0.36f, 0.98f},
      .hover_color = {0.21f, 0.30f, 0.42f, 1.0f},
      .active_color = {0.25f, 0.35f, 0.49f, 1.0f},
      .text_color = {0.97f, 0.98f, 0.99f, 1.0f},
      .border_radius = 16.0f,
      .padding = 10.0f,
  };
  const StygianWidgetStyle k_sidebar_idle = {
      .bg_color = {0.08f, 0.10f, 0.13f, 0.96f},
      .hover_color = {0.12f, 0.16f, 0.20f, 1.0f},
      .active_color = {0.16f, 0.22f, 0.29f, 1.0f},
      .text_color = {0.84f, 0.89f, 0.95f, 1.0f},
      .border_radius = 18.0f,
      .padding = 10.0f,
  };
  const StygianWidgetStyle k_sidebar_active = {
      .bg_color = {0.19f, 0.27f, 0.39f, 0.98f},
      .hover_color = {0.21f, 0.31f, 0.44f, 1.0f},
      .active_color = {0.24f, 0.35f, 0.48f, 1.0f},
      .text_color = {0.97f, 0.98f, 0.99f, 1.0f},
      .border_radius = 18.0f,
      .padding = 10.0f,
  };
  const StygianWidgetStyle k_small_pill = {
      .bg_color = {0.10f, 0.12f, 0.16f, 0.96f},
      .hover_color = {0.16f, 0.19f, 0.24f, 1.0f},
      .active_color = {0.21f, 0.26f, 0.32f, 1.0f},
      .text_color = {0.87f, 0.91f, 0.97f, 1.0f},
      .border_radius = 14.0f,
      .padding = 8.0f,
  };

  stygian_mini_perf_init(&perf, "browser_mockup");
  perf.widget.renderer_name = STYGIAN_BROWSER_RENDERER_NAME;
  browser_sync_address(address_buffer, (int)sizeof(address_buffer), active_page);
  browser_copy_text(status_line, (int)sizeof(status_line),
                    "Pinned workspace ready. Native shell stays out of the way.");

  while (!stygian_window_should_close(window)) {
    StygianEvent event;
    bool event_mutated = false;
    bool event_requested = false;
    bool event_eval_requested = false;
    bool shell_changed = false;
    bool sidebar_changed = false;
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
      float sidebar_x;
      float sidebar_y;
      float sidebar_w;
      float sidebar_h;
      float content_x;
      float content_y;
      float content_w;
      float content_h;

      if (!render_frame && !eval_only_frame)
        continue;
      first_frame = false;

      stygian_window_get_size(window, &width, &height);
      stygian_begin_frame_intent(
          ctx, width, height,
          eval_only_frame ? STYGIAN_FRAME_EVAL_ONLY : STYGIAN_FRAME_RENDER);

      sidebar_x = 18.0f;
      sidebar_y = 18.0f;
      sidebar_w = compact_sidebar ? 94.0f : 236.0f;
      sidebar_h = (float)height - 36.0f;
      content_x = sidebar_x + sidebar_w + 18.0f;
      content_y = 18.0f;
      content_w = (float)width - content_x - 18.0f;
      content_h = (float)height - 36.0f;

      stygian_scope_begin(ctx, k_scope_shell);
      stygian_rect(ctx, 0.0f, 0.0f, (float)width, (float)height, 0.03f, 0.04f,
                   zen_mode ? 0.05f : 0.08f, 1.0f);
      stygian_rect_rounded(ctx, content_x, content_y, content_w, content_h,
                           0.05f, 0.06f, 0.09f, 0.98f, 30.0f);
      stygian_rect_rounded(ctx, content_x + 16.0f, content_y + 16.0f,
                           content_w - 32.0f, 46.0f, 0.06f, 0.08f, 0.11f, 1.0f,
                           18.0f);
      stygian_rect_rounded(ctx, content_x + 16.0f, content_y + 74.0f,
                           content_w - 32.0f, 54.0f, 0.06f, 0.08f, 0.11f, 1.0f,
                           18.0f);

      browser_text(ctx, font, "Aster", content_x + 30.0f, content_y + 31.0f,
                   18.0f, 0.97f, 0.98f, 0.99f, 1.0f);
      browser_text(ctx, font, "native browser shell mockup", content_x + 92.0f,
                   content_y + 33.0f, 12.0f, 0.56f, 0.66f, 0.80f, 1.0f);

      for (int i = 0; i < browser_page_count(); ++i) {
        float tab_x = content_x + 216.0f + (float)i * 112.0f;
        const StygianWidgetStyle *tab_style =
            (i == active_page) ? &k_tab_active : &k_tab_idle;
        if (browser_button(ctx, font, k_pages[i].tab_label, tab_x,
                           content_y + 24.0f, 96.0f, 28.0f, tab_style)) {
          active_page = i;
          browser_sync_address(address_buffer, (int)sizeof(address_buffer),
                               active_page);
          browser_copy_text(status_line, (int)sizeof(status_line),
                            "Swapped tabs without the usual browser sludge.");
          shell_changed = true;
          sidebar_changed = true;
          page_changed = true;
        }
      }

      if (browser_button(ctx, font, compact_sidebar ? "Wide" : "Tight",
                         content_x + content_w - 272.0f, content_y + 24.0f,
                         74.0f, 28.0f, &k_small_pill)) {
        compact_sidebar = !compact_sidebar;
        browser_copy_text(status_line, (int)sizeof(status_line),
                          compact_sidebar ? "Sidebar pulled in."
                                          : "Sidebar opened up.");
        shell_changed = true;
        sidebar_changed = true;
        page_changed = true;
      }
      if (browser_button(ctx, font, zen_mode ? "Calm" : "Loud",
                         content_x + content_w - 190.0f, content_y + 24.0f,
                         74.0f, 28.0f, &k_small_pill)) {
        zen_mode = !zen_mode;
        browser_copy_text(status_line, (int)sizeof(status_line),
                          zen_mode ? "Zen mode on." : "Zen mode off.");
        shell_changed = true;
        page_changed = true;
      }
      if (browser_button(ctx, font,
                         page_bookmarked[active_page] ? "Pinned" : "Pin",
                         content_x + content_w - 108.0f, content_y + 24.0f,
                         78.0f, 28.0f, &k_small_pill)) {
        page_bookmarked[active_page] = !page_bookmarked[active_page];
        browser_copy_text(status_line, (int)sizeof(status_line),
                          page_bookmarked[active_page]
                              ? "Page pinned to the sidebar."
                              : "Page removed from pins.");
        shell_changed = true;
        sidebar_changed = true;
      }

      browser_text(ctx, font, "Search or enter address",
                   content_x + 42.0f, content_y + 90.0f, 11.0f, 0.53f, 0.63f,
                   0.76f, 1.0f);
      if (stygian_text_input(ctx, font, content_x + 34.0f, content_y + 96.0f,
                             content_w - 236.0f, 30.0f, address_buffer,
                             (int)sizeof(address_buffer))) {
        shell_changed = true;
      }
      if (browser_button(ctx, font, "Go", content_x + content_w - 186.0f,
                         content_y + 95.0f, 64.0f, 32.0f, &k_small_pill)) {
        browser_copy_text(status_line, (int)sizeof(status_line),
                          "Mock navigation complete. The page stayed native.");
        shell_changed = true;
      }
      if (browser_button(ctx, font, "Reader", content_x + content_w - 114.0f,
                         content_y + 95.0f, 80.0f, 32.0f, &k_small_pill)) {
        browser_copy_text(status_line, (int)sizeof(status_line),
                          "Reader mode is just a good-looking lie for now.");
        shell_changed = true;
      }
      stygian_scope_end(ctx);

      stygian_scope_begin(ctx, k_scope_sidebar);
      stygian_rect_rounded(ctx, sidebar_x, sidebar_y, sidebar_w, sidebar_h,
                           zen_mode ? 0.06f : 0.05f, zen_mode ? 0.07f : 0.08f,
                           0.11f, 0.98f, 30.0f);
      stygian_rect_rounded(ctx, sidebar_x + 18.0f, sidebar_y + 18.0f,
                           sidebar_w - 36.0f, 54.0f, 0.08f, 0.10f, 0.14f, 1.0f,
                           18.0f);
      browser_text(ctx, font, compact_sidebar ? "A" : "Aster",
                   sidebar_x + 30.0f, sidebar_y + 34.0f,
                   compact_sidebar ? 18.0f : 22.0f, 0.98f, 0.99f, 1.0f, 1.0f);
      if (!compact_sidebar) {
        browser_text(ctx, font, "Wave 1 browser study", sidebar_x + 30.0f,
                     sidebar_y + 56.0f, 11.0f, 0.58f, 0.70f, 0.84f, 1.0f);
      }

      for (int i = 0; i < browser_page_count(); ++i) {
        float item_y = sidebar_y + 94.0f + (float)i * 58.0f;
        const StygianWidgetStyle *nav_style =
            (i == active_page) ? &k_sidebar_active : &k_sidebar_idle;
        if (browser_button(ctx, font,
                           compact_sidebar ? k_pages[i].tab_label
                                           : k_pages[i].nav_label,
                           sidebar_x + 18.0f, item_y, sidebar_w - 36.0f, 42.0f,
                           nav_style)) {
          active_page = i;
          browser_sync_address(address_buffer, (int)sizeof(address_buffer),
                               active_page);
          browser_copy_text(status_line, (int)sizeof(status_line),
                            "Sidebar route changed. No browser engine was harmed.");
          shell_changed = true;
          sidebar_changed = true;
          page_changed = true;
        }
      }

      if (!compact_sidebar) {
        browser_text(ctx, font, "Pinned", sidebar_x + 26.0f,
                     sidebar_y + sidebar_h - 206.0f, 12.0f, 0.55f, 0.65f, 0.78f,
                     1.0f);
        browser_chip(ctx, font, "stygian.dev", sidebar_x + 24.0f,
                     sidebar_y + sidebar_h - 186.0f, 0.20f, 0.60f, 0.98f);
        browser_chip(ctx, font, "fieldengine.space", sidebar_x + 24.0f,
                     sidebar_y + sidebar_h - 150.0f, 0.27f, 0.76f, 0.58f);
        browser_chip(ctx, font, "browser shell", sidebar_x + 24.0f,
                     sidebar_y + sidebar_h - 114.0f, 0.94f, 0.56f, 0.24f);
      }
      browser_text(ctx, font, compact_sidebar ? "F1" : status_line,
                   sidebar_x + 24.0f, sidebar_y + sidebar_h - 40.0f, 11.0f,
                   0.56f, 0.66f, 0.80f, 1.0f);
      stygian_scope_end(ctx);

      stygian_scope_begin(ctx, k_scope_page);
      switch (active_page) {
      case 0:
        browser_draw_home(ctx, font, &k_pages[active_page], content_x + 24.0f,
                          content_y + 146.0f, content_w - 48.0f,
                          content_h - 170.0f);
        break;
      case 1:
        browser_draw_bench(ctx, font, &k_pages[active_page], content_x + 24.0f,
                           content_y + 146.0f, content_w - 48.0f,
                           content_h - 170.0f);
        break;
      case 2:
        browser_draw_gallery(ctx, font, &k_pages[active_page], content_x + 24.0f,
                             content_y + 146.0f, content_w - 48.0f,
                             content_h - 170.0f);
        break;
      default:
        browser_draw_docs(ctx, font, &k_pages[active_page], content_x + 24.0f,
                          content_y + 146.0f, content_w - 48.0f,
                          content_h - 170.0f);
        break;
      }
      stygian_scope_end(ctx);

      if (show_perf) {
        stygian_scope_begin(ctx, k_scope_perf);
        stygian_mini_perf_draw(ctx, font, &perf, width, height);
        stygian_scope_end(ctx);
      }

      if (shell_changed || showcase_redraw)
        stygian_scope_invalidate_next(ctx, k_scope_shell);
      if (sidebar_changed || showcase_redraw)
        stygian_scope_invalidate_next(ctx, k_scope_sidebar);
      if (page_changed || showcase_redraw || event_mutated)
        stygian_scope_invalidate_next(ctx, k_scope_page);
      if (!show_perf)
        stygian_scope_invalidate_next(ctx, k_scope_perf);
      if (show_perf)
        stygian_scope_invalidate_next(ctx, k_scope_perf);

      if (shell_changed || sidebar_changed || page_changed || event_mutated ||
          showcase_redraw) {
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
