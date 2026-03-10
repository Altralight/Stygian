#include "../include/stygian.h"
#include "../widgets/stygian_widgets.h"
#include "../window/stygian_window.h"
#include "mini_perf_harness.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define NODE_COUNT 64
#define GRID_COLS 8
#define GRID_ROWS 8

typedef struct GraphLink {
  int from;
  int to;
} GraphLink;

static const GraphLink k_graph_links[] = {
    {0, 8},   {0, 9},   {1, 9},   {1, 10},  {2, 10},  {2, 11},  {3, 11},
    {3, 12},  {4, 12},  {4, 13},  {5, 13},  {5, 14},  {6, 14},  {6, 15},
    {7, 15},  {8, 16},  {8, 17},  {9, 17},  {9, 18},  {10, 18}, {10, 19},
    {11, 19}, {11, 20}, {12, 20}, {12, 21}, {13, 21}, {13, 22}, {14, 22},
    {14, 23}, {15, 23}, {16, 24}, {16, 25}, {17, 25}, {17, 26}, {18, 26},
    {18, 27}, {19, 27}, {19, 28}, {20, 28}, {20, 29}, {21, 29}, {21, 30},
    {22, 30}, {22, 31}, {23, 31}, {24, 32}, {24, 33}, {25, 33}, {25, 34},
    {26, 34}, {26, 35}, {27, 35}, {27, 36}, {28, 36}, {28, 37}, {29, 37},
    {29, 38}, {30, 38}, {30, 39}, {31, 39}, {32, 40}, {33, 41}, {34, 42},
    {35, 43}, {36, 44}, {37, 45}, {38, 46}, {39, 47}, {40, 48}, {41, 49},
    {42, 50}, {43, 51}, {44, 52}, {45, 53}, {46, 54}, {47, 55}, {48, 56},
    {49, 57}, {50, 58}, {51, 59}, {52, 60}, {53, 61}, {54, 62}, {55, 63},
};

typedef struct NodeGraphDemo {
  StygianGraphState graph;
  StygianNodeBuffers buffers;
  StygianMiniPerfHarness perf;
  StygianFont font;
  float node_x[NODE_COUNT];
  float node_y[NODE_COUNT];
  float node_w[NODE_COUNT];
  float node_h[NODE_COUNT];
  float prev_x[NODE_COUNT];
  float prev_y[NODE_COUNT];
  char node_title[NODE_COUNT][32];
  uint8_t node_title_len[NODE_COUNT];
  int node_type[NODE_COUNT];
  bool node_selected[NODE_COUNT];
  bool show_perf;
  bool benchmark_mode;
  bool hero_shot_mode;
  bool camera_fitted;
  bool first_frame;
} NodeGraphDemo;

typedef struct NodeGraphBenchStats {
  unsigned render_frames;
  double sum_build_ms;
  double sum_submit_ms;
  double sum_gpu_ms;
} NodeGraphBenchStats;

static double now_seconds(void) {
#ifdef _WIN32
  static LARGE_INTEGER freq = {0};
  LARGE_INTEGER counter;
  if (freq.QuadPart == 0) {
    QueryPerformanceFrequency(&freq);
  }
  QueryPerformanceCounter(&counter);
  return (double)counter.QuadPart / (double)freq.QuadPart;
#else
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
#endif
}

static void node_graph_fit_view(NodeGraphDemo *demo, float view_w,
                                float view_h) {
  float min_x;
  float min_y;
  float max_x;
  float max_y;
  float content_w;
  float content_h;
  float fit_w;
  float fit_h;
  float zoom_x;
  float zoom_y;
  float center_x;
  float center_y;
  int i;

  if (!demo || view_w <= 32.0f || view_h <= 32.0f)
    return;

  min_x = demo->node_x[0];
  min_y = demo->node_y[0];
  max_x = demo->node_x[0] + demo->node_w[0];
  max_y = demo->node_y[0] + demo->node_h[0];
  for (i = 1; i < NODE_COUNT; ++i) {
    if (demo->node_x[i] < min_x)
      min_x = demo->node_x[i];
    if (demo->node_y[i] < min_y)
      min_y = demo->node_y[i];
    if (demo->node_x[i] + demo->node_w[i] > max_x)
      max_x = demo->node_x[i] + demo->node_w[i];
    if (demo->node_y[i] + demo->node_h[i] > max_y)
      max_y = demo->node_y[i] + demo->node_h[i];
  }

  // The old framing left too much dead air, then immediately dropped into
  // low-detail LOD. Bad combo for a showcase shot.
  fit_w = view_w - 300.0f;
  fit_h = view_h - 130.0f;
  if (fit_w < view_w * 0.55f)
    fit_w = view_w * 0.55f;
  if (fit_h < view_h * 0.7f)
    fit_h = view_h * 0.7f;

  content_w = (max_x - min_x) + 140.0f;
  content_h = (max_y - min_y) + 150.0f;
  zoom_x = fit_w / content_w;
  zoom_y = fit_h / content_h;
  demo->graph.zoom = zoom_x < zoom_y ? zoom_x : zoom_y;
  if (demo->graph.zoom > 1.55f)
    demo->graph.zoom = 1.55f;
  if (demo->graph.zoom < 0.1f)
    demo->graph.zoom = 0.1f;

  center_x = (min_x + max_x) * 0.5f;
  center_y = (min_y + max_y) * 0.5f;
  demo->graph.pan_x = -center_x - 90.0f / demo->graph.zoom;
  demo->graph.pan_y = -center_y - 6.0f / demo->graph.zoom;
  demo->camera_fitted = true;
}

static void node_graph_set_showcase_view(NodeGraphDemo *demo) {
  float min_x;
  float min_y;
  float max_x;
  float max_y;
  float center_x;
  float center_y;
  int i;

  if (!demo)
    return;

  min_x = demo->node_x[0];
  min_y = demo->node_y[0];
  max_x = demo->node_x[0] + demo->node_w[0];
  max_y = demo->node_y[0] + demo->node_h[0];
  for (i = 1; i < NODE_COUNT; ++i) {
    if (demo->node_x[i] < min_x)
      min_x = demo->node_x[i];
    if (demo->node_y[i] < min_y)
      min_y = demo->node_y[i];
    if (demo->node_x[i] + demo->node_w[i] > max_x)
      max_x = demo->node_x[i] + demo->node_w[i];
    if (demo->node_y[i] + demo->node_h[i] > max_y)
      max_y = demo->node_y[i] + demo->node_h[i];
  }

  // The showcase view is intentionally tighter than "fit everything".
  // For a showcase screenshot, a good crop beats a perfectly visible last row.
  center_x = (min_x + max_x) * 0.5f;
  center_y = (min_y + max_y) * 0.5f;
  demo->graph.zoom = 0.92f;
  demo->graph.pan_x = -center_x - 90.0f / demo->graph.zoom;
  demo->graph.pan_y = -center_y - 6.0f / demo->graph.zoom;
  demo->camera_fitted = true;
}

static void node_graph_set_hero_view(NodeGraphDemo *demo) {
  float min_x;
  float min_y;
  float max_x;
  float max_y;
  float center_x;
  float center_y;
  int i;

  if (!demo)
    return;

  min_x = demo->node_x[0];
  min_y = demo->node_y[0];
  max_x = demo->node_x[0] + demo->node_w[0];
  max_y = demo->node_y[0] + demo->node_h[0];
  for (i = 1; i < NODE_COUNT; ++i) {
    if (demo->node_x[i] < min_x)
      min_x = demo->node_x[i];
    if (demo->node_y[i] < min_y)
      min_y = demo->node_y[i];
    if (demo->node_x[i] + demo->node_w[i] > max_x)
      max_x = demo->node_x[i] + demo->node_w[i];
    if (demo->node_y[i] + demo->node_h[i] > max_y)
      max_y = demo->node_y[i] + demo->node_h[i];
  }

  // Hero shots should show actual labels, not a wall of anonymous pills.
  center_x = (min_x + max_x) * 0.5f;
  center_y = (min_y + max_y) * 0.5f;
  demo->graph.zoom = 1.12f;
  demo->graph.pan_x = -center_x - 28.0f / demo->graph.zoom;
  demo->graph.pan_y = -center_y + 26.0f / demo->graph.zoom;
  demo->camera_fitted = true;
}

static void node_graph_demo_init(NodeGraphDemo *demo) {
  int row;
  int col;
  if (!demo)
    return;
  memset(demo, 0, sizeof(*demo));
  demo->graph.zoom = 1.0f;
  demo->graph.pan_x = 0.0f;
  demo->graph.pan_y = 0.0f;
  demo->graph.pin_y_offset = 42.0f;
  demo->graph.pin_size = 12.0f;
  demo->graph.snap_enabled = false;
  demo->graph.disable_grid = false;
  stygian_graph_set_wire_style(&demo->graph, STYGIAN_WIRE_SMOOTH);
  for (row = 0; row < GRID_ROWS; ++row) {
    for (col = 0; col < GRID_COLS; ++col) {
      int idx = row * GRID_COLS + col;
      float stagger = ((row & 1) == 0) ? -18.0f : 18.0f;
      demo->node_x[idx] = -462.0f + (float)(col * 132) + stagger;
      demo->node_y[idx] = -286.0f + (float)(row * 101) +
                          (float)((col % 3) * 7) -
                          (float)((row % 2) * 4);
      demo->node_w[idx] = 126.0f + (float)((idx % 3) * 12);
      demo->node_h[idx] = 78.0f + (float)(((idx + row) % 2) * 10);
      demo->node_type[idx] = (row + col) % 4;
      demo->node_selected[idx] = (idx == 9 || idx == 18 || idx == 27 ||
                                  idx == 36 || idx == 45);
    }
  }
  memcpy(demo->prev_x, demo->node_x, sizeof(demo->node_x));
  memcpy(demo->prev_y, demo->node_y, sizeof(demo->node_y));
  for (int i = 0; i < NODE_COUNT; ++i) {
    snprintf(demo->node_title[i], sizeof(demo->node_title[i]), "Node %02d",
             i + 1);
    demo->node_title_len[i] = (uint8_t)strlen(demo->node_title[i]);
  }
  demo->buffers.x = demo->node_x;
  demo->buffers.y = demo->node_y;
  demo->buffers.w = demo->node_w;
  demo->buffers.h = demo->node_h;
  demo->buffers.type_id = demo->node_type;
  demo->buffers.selected = demo->node_selected;
  stygian_mini_perf_init(&demo->perf, "node_graph_demo");
  demo->perf.widget.renderer_name = "OpenGL";
  demo->perf.widget.stress_mode = true;
  demo->perf.widget.idle_hz = 1000u;
  demo->perf.widget.active_hz = 1000u;
  demo->perf.widget.max_stress_hz = 1000u;
  demo->show_perf = true;
  demo->perf.widget.enabled = demo->show_perf;
  demo->benchmark_mode = false;
  demo->first_frame = true;
  node_graph_set_showcase_view(demo);
}

static void node_graph_apply_mode(NodeGraphDemo *demo) {
  if (!demo)
    return;
  demo->graph.disable_grid = demo->benchmark_mode;
  stygian_graph_set_wire_style(&demo->graph,
                               demo->benchmark_mode ? STYGIAN_WIRE_LINE
                                                    : STYGIAN_WIRE_SMOOTH);
}

static int node_graph_wire_style_for_view(const NodeGraphDemo *demo) {
  if (!demo)
    return STYGIAN_WIRE_LINE;
  if (demo->benchmark_mode)
    return STYGIAN_WIRE_LINE;
  if (demo->graph.zoom < 0.55f)
    return STYGIAN_WIRE_LINE;
  if (demo->graph.zoom < 0.9f)
    return STYGIAN_WIRE_SHARP;
  return demo->graph.wire_style;
}

static void draw_pretty_node_lod(StygianContext *ctx, const NodeGraphDemo *demo,
                                 int idx, float x, float y, float w, float h) {
  bool selected;
  float body_r;
  float body_g;
  float body_b;
  float body_a;
  float header_r;
  float header_g;
  float header_b;
  float header_a;
  float radius;
  bool far_lod;
  bool medium_lod;

  if (!ctx || !demo)
    return;

  selected = demo->node_selected[idx];
  far_lod = (demo->graph.zoom < 0.42f || w < 64.0f || h < 38.0f);
  medium_lod = (!far_lod && (demo->graph.zoom < 0.68f || w < 92.0f));
  body_r = selected ? 0.18f : 0.14f;
  body_g = selected ? 0.19f : 0.15f;
  body_b = selected ? 0.25f : 0.19f;
  body_a = 0.98f;
  header_r = selected ? 0.25f : 0.21f;
  header_g = selected ? 0.31f : 0.22f;
  header_b = selected ? 0.42f : 0.28f;
  header_a = 1.0f;
  radius = far_lod ? 6.0f : 8.0f;

  stygian_rect_rounded(ctx, x, y, w, h, body_r, body_g, body_b, body_a,
                       radius);
  if (far_lod) {
    if (selected) {
      stygian_rect_rounded(ctx, x + 2.0f, y + 2.0f, w - 4.0f, 5.0f, 0.36f,
                           0.62f, 1.0f, 0.95f, 2.5f);
    }
    return;
  }

  stygian_rect_rounded(ctx, x, y, w, 24.0f, header_r, header_g, header_b,
                       header_a, radius);
  if (medium_lod)
    return;

  stygian_text_span(ctx, demo->font, demo->node_title[idx],
                    demo->node_title_len[idx], x + 10.0f, y + 4.0f, 16.0f,
                    0.9f, 0.92f, 0.96f, 1.0f);
}

static bool node_graph_positions_changed(NodeGraphDemo *demo) {
  int i;
  if (!demo)
    return false;
  for (i = 0; i < NODE_COUNT; ++i) {
    if (demo->prev_x[i] != demo->node_x[i] || demo->prev_y[i] != demo->node_y[i])
      return true;
  }
  return false;
}

static void node_graph_positions_snapshot(NodeGraphDemo *demo) {
  if (!demo)
    return;
  memcpy(demo->prev_x, demo->node_x, sizeof(demo->node_x));
  memcpy(demo->prev_y, demo->node_y, sizeof(demo->node_y));
}

static void draw_graph_wires(StygianContext *ctx, const NodeGraphDemo *demo) {
  static float wire_color[4] = {74.0f / 255.0f, 158.0f / 255.0f, 1.0f, 0.95f};
  int style;
  size_t i;
  if (!ctx || !demo)
    return;
  style = node_graph_wire_style_for_view(demo);
  for (i = 0; i < sizeof(k_graph_links) / sizeof(k_graph_links[0]); ++i) {
    int from = k_graph_links[i].from;
    int to = k_graph_links[i].to;
    float ax_world = 0.0f;
    float ay_world = 0.0f;
    float bx_world = 0.0f;
    float by_world = 0.0f;
    float ax = 0.0f;
    float ay = 0.0f;
    float bx = 0.0f;
    float by = 0.0f;
    stygian_graph_pin_center_world(&demo->graph, demo->node_x[from],
                                   demo->node_y[from], demo->node_w[from], true,
                                   &ax_world, &ay_world);
    stygian_graph_pin_center_world(&demo->graph, demo->node_x[to],
                                   demo->node_y[to], demo->node_w[to], false,
                                   &bx_world, &by_world);
    bool visible =
        (style == STYGIAN_WIRE_SMOOTH)
            ? stygian_graph_link_visible_bezier(&demo->graph, ax_world,
                                                ay_world, bx_world, by_world,
                                                24.0f)
            : stygian_graph_link_visible(&demo->graph, ax_world, ay_world,
                                         bx_world, by_world, 24.0f);
    if (visible) {
      StygianGraphState style_state = demo->graph;
      stygian_graph_world_to_screen(&demo->graph, ax_world, ay_world, &ax, &ay);
      stygian_graph_world_to_screen(&demo->graph, bx_world, by_world, &bx, &by);
      style_state.wire_style = style;
      stygian_graph_link(ctx, &style_state, ax, ay, bx, by, 2.0f, wire_color);
    }
  }
}

static void draw_graph_nodes(StygianContext *ctx, NodeGraphDemo *demo) {
  int idx;
  if (!ctx || !demo)
    return;
  while (stygian_node_graph_next(ctx, &demo->graph, &idx)) {
    float sx = 0.0f;
    float sy = 0.0f;
    float sw = 0.0f;
    float sh = 0.0f;
    stygian_graph_node_screen_rect(&demo->graph, demo->node_x[idx],
                                   demo->node_y[idx], demo->node_w[idx],
                                   demo->node_h[idx], &sx, &sy, &sw, &sh);
    if (demo->graph.dragging_id == idx + 1)
      demo->node_selected[idx] = true;
    if (demo->benchmark_mode) {
      float c = demo->node_selected[idx] ? 0.22f : 0.16f;
      float a = demo->node_selected[idx] ? 1.0f : 0.96f;
      stygian_rect_rounded(ctx, sx, sy, sw, sh, c, c, 0.24f, a, 7.0f);
    } else {
      draw_pretty_node_lod(ctx, demo, idx, sx, sy, sw, sh);
    }
  }
}

static void node_graph_force_all_visible(NodeGraphDemo *demo) {
  int i;
  if (!demo)
    return;
  demo->graph.iter_idx = 0;
  demo->graph.visible_count = NODE_COUNT;
  for (i = 0; i < NODE_COUNT; ++i) {
    demo->graph.visible_ids[i] = i;
  }
}

int main(int argc, char **argv) {
  const StygianScopeId k_scope_graph = 0x7101u;
  const StygianScopeId k_scope_perf =
      STYGIAN_OVERLAY_SCOPE_BASE | (StygianScopeId)0x7102u;
  StygianWindowConfig win_cfg = {
      .width = 1280,
      .height = 720,
      .title = "Stygian Node Graph Demo",
      .flags = STYGIAN_WINDOW_RESIZABLE | STYGIAN_WINDOW_OPENGL,
  };
  StygianWindow *window = stygian_window_create(&win_cfg);
  StygianConfig cfg;
  StygianContext *ctx;
  NodeGraphDemo demo;
  bool cli_benchmark_mode = false;
  bool cli_hero_shot_mode = false;
  bool cli_show_perf = true;
  double bench_seconds = 0.0;
  double bench_start_time = 0.0;
  double bench_capture_start = 0.0;
  const double bench_warmup_seconds = 1.0;
  bool bench_capture_started = false;
  bool unattended_bench = false;
  NodeGraphBenchStats bench_stats = {0};

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--benchmark-mode") == 0) {
      cli_benchmark_mode = true;
    } else if (strcmp(argv[i], "--hero-shot") == 0) {
      cli_hero_shot_mode = true;
    } else if (strcmp(argv[i], "--no-perf") == 0) {
      cli_show_perf = false;
    } else if (strcmp(argv[i], "--bench-seconds") == 0 && (i + 1) < argc) {
      bench_seconds = atof(argv[++i]);
      if (bench_seconds < 0.0)
        bench_seconds = 0.0;
    }
  }

  if (!window)
    return 1;
  memset(&cfg, 0, sizeof(cfg));
  cfg.backend = STYGIAN_BACKEND_OPENGL;
  cfg.window = window;
  ctx = stygian_create(&cfg);
  if (!ctx) {
    stygian_window_destroy(window);
    return 1;
  }
  stygian_set_vsync(ctx, false);

  node_graph_demo_init(&demo);
  demo.benchmark_mode = cli_benchmark_mode;
  demo.hero_shot_mode = cli_hero_shot_mode;
  demo.show_perf = cli_show_perf;
  if (demo.hero_shot_mode) {
    demo.show_perf = false;
    node_graph_set_hero_view(&demo);
  }
  demo.perf.widget.enabled = demo.show_perf;
  node_graph_apply_mode(&demo);
  demo.font = stygian_font_load(ctx, "assets/atlas.png", "assets/atlas.json");
  if (bench_seconds > 0.0) {
    unattended_bench = true;
    demo.show_perf = false;
    demo.perf.widget.enabled = false;
    demo.camera_fitted = false;
    stygian_set_present_enabled(ctx, false);
    stygian_set_gpu_timing_enabled(ctx, false);
    stygian_set_output_icc_auto(ctx, false);
    bench_start_time = now_seconds();
  }

  while (!stygian_window_should_close(window)) {
    StygianEvent event;
    bool event_mutated = false;
    bool event_requested = false;
    bool event_eval_requested = false;
    uint32_t wait_ms = stygian_next_repaint_wait_ms(ctx, 250u);

    stygian_widgets_begin_frame(ctx);

    if (!unattended_bench) {
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
            demo.show_perf = !demo.show_perf;
            demo.perf.widget.enabled = demo.show_perf;
            event_requested = true;
          } else if (event.key.key == STYGIAN_KEY_F2) {
            demo.benchmark_mode = !demo.benchmark_mode;
            node_graph_apply_mode(&demo);
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

    if (!unattended_bench && !demo.first_frame && !event_mutated && !event_requested &&
        !stygian_has_pending_repaint(ctx) &&
        !event_eval_requested) {
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
            demo.show_perf = !demo.show_perf;
            demo.perf.widget.enabled = demo.show_perf;
            event_requested = true;
          } else if (event.key.key == STYGIAN_KEY_F2) {
            demo.benchmark_mode = !demo.benchmark_mode;
            node_graph_apply_mode(&demo);
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
      bool repaint_pending = stygian_has_pending_repaint(ctx);
      bool render_frame = demo.first_frame || event_mutated || repaint_pending;
      bool eval_only_frame =
          (!render_frame && (event_requested || event_eval_requested));
      int width;
      int height;
      float prev_pan_x;
      float prev_pan_y;
      float prev_zoom;
      bool graph_changed;

      if (!render_frame && !eval_only_frame)
        continue;

      demo.first_frame = false;
      stygian_window_get_size(window, &width, &height);
      demo.graph.x = 0.0f;
      demo.graph.y = 0.0f;
      demo.graph.w = (float)width;
      demo.graph.h = (float)height;
      if (!demo.camera_fitted) {
        node_graph_fit_view(&demo, (float)width, (float)height);
      }

      prev_pan_x = demo.graph.pan_x;
      prev_pan_y = demo.graph.pan_y;
      prev_zoom = demo.graph.zoom;
      if (!unattended_bench) {
        node_graph_positions_snapshot(&demo);
      }

      stygian_begin_frame_intent(
          ctx, width, height,
          eval_only_frame ? STYGIAN_FRAME_EVAL_ONLY : STYGIAN_FRAME_RENDER);

      stygian_scope_begin(ctx, k_scope_graph);
      demo.perf.widget.enabled = demo.show_perf;
      stygian_node_graph_begin(ctx, &demo.graph, &demo.buffers, NODE_COUNT);
      node_graph_force_all_visible(&demo);
      draw_graph_wires(ctx, &demo);
      draw_graph_nodes(ctx, &demo);
      stygian_node_graph_end(ctx, &demo.graph);
      stygian_scope_end(ctx);

      if (demo.show_perf) {
        stygian_scope_begin(ctx, k_scope_perf);
        stygian_mini_perf_draw(ctx, demo.font, &demo.perf, width, height);
        stygian_scope_end(ctx);
      }

      graph_changed = !unattended_bench &&
                      ((prev_pan_x != demo.graph.pan_x) ||
                       (prev_pan_y != demo.graph.pan_y) ||
                       (prev_zoom != demo.graph.zoom) ||
                       node_graph_positions_changed(&demo));

      if (graph_changed) {
        stygian_scope_invalidate_next(ctx, k_scope_graph);
        stygian_set_repaint_source(ctx, "graph_mutation");
        stygian_request_repaint_after_ms(ctx, 0u);
      }

      if (demo.show_perf) {
        stygian_scope_invalidate_next(ctx, k_scope_perf);
      }
      stygian_set_repaint_source(ctx, "showcase");
      stygian_request_repaint_after_ms(ctx, 0u);

      stygian_widgets_commit_regions();
      stygian_end_frame(ctx);
      if (!unattended_bench) {
        stygian_mini_perf_accumulate(&demo.perf, eval_only_frame);
        stygian_mini_perf_log(ctx, &demo.perf);
      }
      if (bench_seconds > 0.0 && !eval_only_frame) {
        double now = now_seconds();
        if (!bench_capture_started &&
            (now - bench_start_time) >= bench_warmup_seconds) {
          bench_capture_started = true;
          bench_capture_start = now;
          memset(&bench_stats, 0, sizeof(bench_stats));
          printf("NODEBENCH start mode=%s\n",
                 demo.benchmark_mode ? "benchmark" : "pretty");
        }
        if (bench_capture_started) {
          bench_stats.render_frames++;
          bench_stats.sum_build_ms += stygian_get_last_frame_build_ms(ctx);
          bench_stats.sum_submit_ms += stygian_get_last_frame_submit_ms(ctx);
          bench_stats.sum_gpu_ms += stygian_get_last_frame_gpu_ms(ctx);
          if ((now - bench_capture_start) >= bench_seconds) {
            double n =
                bench_stats.render_frames > 0u ? (double)bench_stats.render_frames
                                               : 1.0;
            double wall_fps =
                bench_seconds > 0.0 ? (double)bench_stats.render_frames / bench_seconds
                                    : 0.0;
            double wall_frame_ms = wall_fps > 0.0 ? 1000.0 / wall_fps : 0.0;
            double render_ms =
                (bench_stats.sum_build_ms + bench_stats.sum_submit_ms) / n;
            double loop_overhead_ms =
                wall_frame_ms > render_ms ? wall_frame_ms - render_ms : 0.0;
            printf("NODEBENCH summary mode=%s seconds=%.2f render=%u "
                   "render_ms=%.4f wall_fps=%.2f loop_overhead_ms=%.4f "
                   "build_ms=%.4f submit_ms=%.4f gpu_ms=%.4f\n",
                   demo.benchmark_mode ? "benchmark" : "pretty", bench_seconds,
                   bench_stats.render_frames, render_ms, wall_fps,
                   loop_overhead_ms,
                   bench_stats.sum_build_ms / n, bench_stats.sum_submit_ms / n,
                   bench_stats.sum_gpu_ms / n);
            break;
          }
        }
      }
    }
  }

  if (demo.font)
    stygian_font_destroy(ctx, demo.font);
  stygian_destroy(ctx);
  stygian_window_destroy(window);
  return 0;
}
