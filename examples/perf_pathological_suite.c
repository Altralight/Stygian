#include "../include/stygian.h"
#include "../widgets/stygian_widgets.h"
#include "../window/stygian_input.h"
#include "../window/stygian_window.h"
#include "mini_perf_harness.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef STYGIAN_DEMO_VULKAN
#define STYGIAN_SUITE_BACKEND STYGIAN_BACKEND_VULKAN
#define STYGIAN_SUITE_WINDOW_FLAGS                                              \
  (STYGIAN_WINDOW_RESIZABLE | STYGIAN_WINDOW_VULKAN)
#define STYGIAN_SUITE_RENDERER_NAME "vk"
#else
#define STYGIAN_SUITE_BACKEND STYGIAN_BACKEND_OPENGL
#define STYGIAN_SUITE_WINDOW_FLAGS                                              \
  (STYGIAN_WINDOW_RESIZABLE | STYGIAN_WINDOW_OPENGL)
#define STYGIAN_SUITE_RENDERER_NAME "gl"
#endif

typedef enum PerfScenario {
  PERF_SCENARIO_IDLE = 0,
  PERF_SCENARIO_OVERLAY = 1,
  PERF_SCENARIO_SPARSE = 2,
  PERF_SCENARIO_CLIP = 3,
  PERF_SCENARIO_SCROLL = 4,
  PERF_SCENARIO_TEXT = 5,
  PERF_SCENARIO_STATIC = 6,
  PERF_SCENARIO_FULLHOT = 7,
  PERF_SCENARIO_TEXT_STATIC = 8,
} PerfScenario;

typedef struct DenseGridLayout {
  float base_x;
  float base_y;
  float cell_w;
  float cell_h;
  float rect_w;
  float rect_h;
  uint32_t cols;
} DenseGridLayout;

typedef struct PerfIntervalStats {
  uint32_t render_frames;
  uint32_t eval_frames;
  uint64_t samples;
  double sum_draw_calls;
  double sum_elements;
  double sum_clip_count;
  double sum_replay_hits;
  double sum_replay_misses;
  double sum_replay_forced;
  double sum_gpu_ms;
  double sum_build_ms;
  double sum_submit_ms;
  double sum_present_ms;
  double sum_upload_bytes;
  double sum_upload_ranges;
} PerfIntervalStats;

typedef struct PerfIntervalRow {
  uint32_t second_index;
  uint32_t render_frames;
  uint32_t eval_frames;
  double draw_calls;
  double elements;
  double clip_count;
  double replay_hits;
  double replay_misses;
  double replay_forced;
  double render_ms;
  double gpu_ms;
  double build_ms;
  double submit_ms;
  double present_ms;
  double upload_bytes;
  double upload_ranges;
  uint32_t cmd_applied;
  uint32_t cmd_drops;
} PerfIntervalRow;

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

static const char *scenario_name(PerfScenario scenario) {
  switch (scenario) {
  case PERF_SCENARIO_IDLE:
    return "idle";
  case PERF_SCENARIO_OVERLAY:
    return "overlay";
  case PERF_SCENARIO_SPARSE:
    return "sparse";
  case PERF_SCENARIO_CLIP:
    return "clip";
  case PERF_SCENARIO_SCROLL:
    return "scroll";
  case PERF_SCENARIO_TEXT:
    return "text";
  case PERF_SCENARIO_STATIC:
    return "static";
  case PERF_SCENARIO_FULLHOT:
    return "fullhot";
  case PERF_SCENARIO_TEXT_STATIC:
    return "text_static";
  default:
    return "idle";
  }
}

static PerfScenario parse_scenario(const char *name) {
  if (!name)
    return PERF_SCENARIO_IDLE;
  if (strcmp(name, "overlay") == 0)
    return PERF_SCENARIO_OVERLAY;
  if (strcmp(name, "sparse") == 0)
    return PERF_SCENARIO_SPARSE;
  if (strcmp(name, "clip") == 0)
    return PERF_SCENARIO_CLIP;
  if (strcmp(name, "scroll") == 0)
    return PERF_SCENARIO_SCROLL;
  if (strcmp(name, "text") == 0)
    return PERF_SCENARIO_TEXT;
  if (strcmp(name, "static") == 0)
    return PERF_SCENARIO_STATIC;
  if (strcmp(name, "fullhot") == 0)
    return PERF_SCENARIO_FULLHOT;
  if (strcmp(name, "text_static") == 0)
    return PERF_SCENARIO_TEXT_STATIC;
  return PERF_SCENARIO_IDLE;
}

static void interval_add_sample(StygianContext *ctx, PerfIntervalStats *stats,
                                bool eval_only) {
  if (!ctx || !stats)
    return;
  if (eval_only)
    stats->eval_frames++;
  else
    stats->render_frames++;
  stats->samples++;
  stats->sum_draw_calls += (double)stygian_get_last_frame_draw_calls(ctx);
  stats->sum_elements += (double)stygian_get_last_frame_element_count(ctx);
  stats->sum_clip_count += (double)stygian_get_last_frame_clip_count(ctx);
  stats->sum_replay_hits +=
      (double)stygian_get_last_frame_scope_replay_hits(ctx);
  stats->sum_replay_misses +=
      (double)stygian_get_last_frame_scope_replay_misses(ctx);
  stats->sum_replay_forced +=
      (double)stygian_get_last_frame_scope_forced_rebuilds(ctx);
  stats->sum_gpu_ms += (double)stygian_get_last_frame_gpu_ms(ctx);
  stats->sum_build_ms += (double)stygian_get_last_frame_build_ms(ctx);
  stats->sum_submit_ms += (double)stygian_get_last_frame_submit_ms(ctx);
  stats->sum_present_ms += (double)stygian_get_last_frame_present_ms(ctx);
  stats->sum_upload_bytes +=
      (double)stygian_get_last_frame_upload_bytes(ctx);
  stats->sum_upload_ranges +=
      (double)stygian_get_last_frame_upload_ranges(ctx);
}

static void interval_add_idle_sample(PerfIntervalStats *stats) {
  if (!stats)
    return;
  stats->samples++;
}

static PerfIntervalRow interval_row_make(const PerfIntervalStats *stats,
                                         uint32_t second_index,
                                         StygianContext *ctx) {
  PerfIntervalRow row;
  double n = (stats && stats->samples > 0u) ? (double)stats->samples : 1.0;
  memset(&row, 0, sizeof(row));
  row.second_index = second_index;
  row.render_frames = stats ? stats->render_frames : 0u;
  row.eval_frames = stats ? stats->eval_frames : 0u;
  row.draw_calls = stats ? stats->sum_draw_calls / n : 0.0;
  row.elements = stats ? stats->sum_elements / n : 0.0;
  row.clip_count = stats ? stats->sum_clip_count / n : 0.0;
  row.replay_hits = stats ? stats->sum_replay_hits / n : 0.0;
  row.replay_misses = stats ? stats->sum_replay_misses / n : 0.0;
  row.replay_forced = stats ? stats->sum_replay_forced / n : 0.0;
  row.build_ms = stats ? stats->sum_build_ms / n : 0.0;
  row.submit_ms = stats ? stats->sum_submit_ms / n : 0.0;
  row.render_ms = row.build_ms + row.submit_ms;
  row.gpu_ms = stats ? stats->sum_gpu_ms / n : 0.0;
  row.present_ms = stats ? stats->sum_present_ms / n : 0.0;
  row.upload_bytes = stats ? stats->sum_upload_bytes / n : 0.0;
  row.upload_ranges = stats ? stats->sum_upload_ranges / n : 0.0;
  row.cmd_applied = ctx ? stygian_get_last_commit_applied(ctx) : 0u;
  row.cmd_drops = ctx ? stygian_get_total_command_drops(ctx) : 0u;
  return row;
}

static void interval_write_csv(FILE *csv_file, const PerfIntervalRow *row,
                               const char *scenario_label, uint32_t scene_count,
                               uint32_t mutate_count, bool raw_mode,
                               double tick_hz) {
  if (!csv_file || !row)
    return;
  fprintf(csv_file,
          "%u,%s,%s,%s,%u,%u,%.2f,%u,%u,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.4f,%.4f,%.4f,%.4f,%.4f,%.0f,%.2f,%u,%u\n",
          row->second_index, scenario_label, STYGIAN_SUITE_RENDERER_NAME,
          raw_mode ? "raw" : "present", scene_count, mutate_count, tick_hz,
          row->render_frames, row->eval_frames, row->draw_calls, row->elements,
          row->clip_count, row->replay_hits, row->replay_misses,
          row->replay_forced, row->render_ms, row->gpu_ms, row->build_ms,
          row->submit_ms, row->present_ms, row->upload_bytes,
          row->upload_ranges, row->cmd_applied, row->cmd_drops);
  fflush(csv_file);
}

static void interval_log(const PerfIntervalRow *row, const char *scenario_label,
                         uint32_t scene_count, uint32_t mutate_count,
                         bool raw_mode, double tick_hz) {
  if (!row)
    return;
  printf("PERFCASE scenario=%s backend=%s mode=%s second=%u render=%u eval=%u "
         "count=%u mutate=%u tick_hz=%.2f draws=%.2f elems=%.2f clips=%.2f "
         "replay_h=%.2f replay_m=%.2f replay_f=%.2f render_ms=%.4f gpu_ms=%.4f "
         "build_ms=%.4f submit_ms=%.4f present_ms=%.4f "
         "upload_bytes=%.0f upload_ranges=%.2f cmd_applied=%u cmd_drops=%u\n",
         scenario_label, STYGIAN_SUITE_RENDERER_NAME,
         raw_mode ? "raw" : "present", row->second_index, row->render_frames,
         row->eval_frames, scene_count, mutate_count, tick_hz, row->draw_calls,
         row->elements, row->clip_count, row->replay_hits, row->replay_misses,
         row->replay_forced, row->render_ms, row->gpu_ms, row->build_ms,
         row->submit_ms, row->present_ms, row->upload_bytes,
         row->upload_ranges, row->cmd_applied, row->cmd_drops);
}

static void summary_log(const PerfIntervalStats *stats,
                        const char *scenario_label, uint32_t scene_count,
                        uint32_t mutate_count, bool raw_mode, double tick_hz,
                        double capture_seconds) {
  PerfIntervalRow row = interval_row_make(stats, 0u, NULL);
  double wall_fps = capture_seconds > 0.0
                        ? (double)row.render_frames / capture_seconds
                        : 0.0;
  double wall_frame_ms = wall_fps > 0.0 ? 1000.0 / wall_fps : 0.0;
  double loop_overhead_ms = wall_frame_ms > row.render_ms
                                ? wall_frame_ms - row.render_ms
                                : 0.0;
  printf("PERFSUMMARY scenario=%s backend=%s mode=%s capture_seconds=%.2f "
         "render=%u eval=%u count=%u mutate=%u tick_hz=%.2f "
         "draws=%.2f elems=%.2f clips=%.2f replay_h=%.2f replay_m=%.2f replay_f=%.2f "
         "render_ms=%.4f wall_fps=%.2f loop_overhead_ms=%.4f gpu_ms=%.4f build_ms=%.4f submit_ms=%.4f "
         "present_ms=%.4f upload_bytes=%.0f upload_ranges=%.2f\n",
         scenario_label, STYGIAN_SUITE_RENDERER_NAME,
         raw_mode ? "raw" : "present", capture_seconds, row.render_frames,
         row.eval_frames, scene_count, mutate_count, tick_hz, row.draw_calls,
         row.elements, row.clip_count, row.replay_hits, row.replay_misses,
         row.replay_forced, row.render_ms, wall_fps, loop_overhead_ms,
         row.gpu_ms, row.build_ms,
         row.submit_ms, row.present_ms, row.upload_bytes, row.upload_ranges);
}

static void dense_grid_layout_init(DenseGridLayout *layout, uint32_t count,
                                   int width, int height) {
  float view_w;
  float view_h;
  uint32_t rows;
  if (!layout)
    return;
  memset(layout, 0, sizeof(*layout));
  if (count == 0u)
    count = 1u;
  layout->base_x = 12.0f;
  layout->base_y = 72.0f;
  view_w = (float)width - 24.0f;
  view_h = (float)height - 84.0f;
  if (view_w < 32.0f)
    view_w = 32.0f;
  if (view_h < 32.0f)
    view_h = 32.0f;
  layout->cols =
      (uint32_t)ceil(sqrt(((double)count * (double)view_w) / (double)view_h));
  if (layout->cols == 0u)
    layout->cols = 1u;
  rows = (count + layout->cols - 1u) / layout->cols;
  if (rows == 0u)
    rows = 1u;
  layout->cell_w = view_w / (float)layout->cols;
  layout->cell_h = view_h / (float)rows;
  layout->rect_w = floorf(layout->cell_w) - 1.0f;
  layout->rect_h = floorf(layout->cell_h) - 1.0f;
  if (layout->rect_w < 1.0f)
    layout->rect_w = 1.0f;
  if (layout->rect_h < 1.0f)
    layout->rect_h = 1.0f;
}

static void dense_grid_rect(const DenseGridLayout *layout, uint32_t index,
                            float *x, float *y) {
  uint32_t col;
  uint32_t row;
  if (!layout || !x || !y || layout->cols == 0u)
    return;
  col = index % layout->cols;
  row = index / layout->cols;
  *x = layout->base_x + (float)col * layout->cell_w;
  *y = layout->base_y + (float)row * layout->cell_h;
}

static void render_sparse_static_scene(StygianContext *ctx, uint32_t count,
                                       int width, int height) {
  DenseGridLayout layout;
  dense_grid_layout_init(&layout, count, width, height);
  for (uint32_t index = 0u; index < count; index++) {
    float x = 0.0f;
    float y = 0.0f;
    float phase = ((float)(index % 251u)) / 250.0f;
    dense_grid_rect(&layout, index, &x, &y);
    stygian_rect(ctx, x, y, layout.rect_w, layout.rect_h, 0.11f + 0.04f * phase,
                 0.13f + 0.03f * phase, 0.17f + 0.04f * phase, 1.0f);
  }
}

static void render_sparse_dynamic_scene(StygianContext *ctx, uint32_t count,
                                        uint32_t mutate_count, int width,
                                        int height, uint32_t tick_count) {
  DenseGridLayout layout;
  dense_grid_layout_init(&layout, count, width, height);
  if (mutate_count > count)
    mutate_count = count;
  for (uint32_t i = 0u; i < mutate_count; i++) {
    uint32_t idx = (i * 97u + tick_count * 131u) % count;
    float x = 0.0f;
    float y = 0.0f;
    float phase = ((float)((idx + tick_count * 13u) % 211u)) / 210.0f;
    dense_grid_rect(&layout, idx, &x, &y);
    stygian_rect(ctx, x, y, layout.rect_w, layout.rect_h,
                 0.35f + 0.6f * phase, 0.86f - 0.4f * phase,
                 0.2f + 0.4f * phase, 1.0f);
  }
}

static void render_fullhot_scene(StygianContext *ctx, uint32_t count, int width,
                                 int height, uint32_t tick_count) {
  DenseGridLayout layout;
  dense_grid_layout_init(&layout, count, width, height);
  for (uint32_t index = 0u; index < count; index++) {
    float x = 0.0f;
    float y = 0.0f;
    float phase = ((float)((index * 17u + tick_count * 29u) % 1021u)) / 1020.0f;
    dense_grid_rect(&layout, index, &x, &y);
    stygian_rect(ctx, x, y, layout.rect_w, layout.rect_h,
                 0.18f + 0.72f * phase, 0.22f + 0.50f * (1.0f - phase),
                 0.30f + 0.45f * phase, 1.0f);
  }
}

static void render_clip_scene(StygianContext *ctx, uint32_t tick_count, int width,
                              int height) {
  float x = 80.0f;
  float y = 100.0f;
  float w = (float)width - 220.0f;
  float h = (float)height - 180.0f;
  float drift = (float)((tick_count % 120u)) * 0.5f;
  for (uint32_t depth = 0u; depth < 20u; depth++) {
    float inset = (float)depth * 10.0f;
    if (w - inset * 2.0f <= 4.0f || h - inset * 2.0f <= 4.0f)
      break;
    stygian_clip_push(ctx, x + inset, y + inset, w - inset * 2.0f,
                      h - inset * 2.0f);
    stygian_rect(ctx, x + inset + drift, y + inset + 2.0f + (float)depth * 0.5f,
                 w - inset * 2.0f - drift, 3.0f, 0.14f + 0.01f * (float)depth,
                 0.25f, 0.35f, 1.0f);
  }
  for (uint32_t depth = 0u; depth < 20u; depth++) {
    stygian_clip_pop(ctx);
  }
}

static void render_scroll_shell(StygianContext *ctx, int width, int height) {
  float vx = 40.0f;
  float vy = 82.0f;
  float vw = (float)width - 80.0f;
  float vh = (float)height - 130.0f;
  stygian_rect_rounded(ctx, vx, vy, vw, vh, 0.12f, 0.13f, 0.16f, 1.0f, 8.0f);
  stygian_widgets_register_region(vx, vy, vw, vh, STYGIAN_WIDGET_REGION_SCROLL);
}

static bool render_scroll_rows(StygianContext *ctx, StygianFont font,
                               float *scroll_y, int width, int height) {
  float vx = 40.0f;
  float vy = 82.0f;
  float vw = (float)width - 80.0f;
  float vh = (float)height - 130.0f;
  float row_h = 32.0f;
  int total_rows = 180;
  float content_h = (float)total_rows * row_h;
  int first_row;
  int visible_rows;
  int end_row;
  bool changed = false;
  if (!scroll_y)
    return false;
  first_row = (int)(*scroll_y / row_h) - 2;
  if (first_row < 0)
    first_row = 0;
  visible_rows = (int)(vh / row_h) + 6;
  end_row = first_row + visible_rows;
  if (end_row > total_rows)
    end_row = total_rows;

  stygian_clip_push(ctx, vx + 8.0f, vy + 8.0f, vw - 16.0f, vh - 16.0f);
  for (int i = first_row; i < end_row; i++) {
    char line[80];
    float ly = vy + 10.0f + (float)i * row_h - *scroll_y;
    snprintf(line, sizeof(line), "Scrollable row %03d  mutation target", i + 1);
    stygian_text(ctx, font, line, vx + 14.0f, ly, 15.0f, 0.78f, 0.83f, 0.9f,
                 1.0f);
  }
  stygian_clip_pop(ctx);
  if (stygian_scrollbar_v(ctx, vx + vw - 10.0f, vy + 6.0f, 6.0f, vh - 12.0f,
                          content_h, scroll_y)) {
    changed = true;
  }
  return changed;
}

static void render_text_scene(StygianContext *ctx, StygianFont font,
                              StygianTextArea *editor) {
  stygian_rect_rounded(ctx, editor->x - 6.0f, editor->y - 6.0f,
                       editor->w + 12.0f, editor->h + 12.0f, 0.12f, 0.13f, 0.16f,
                       1.0f, 8.0f);
  stygian_text_area(ctx, font, editor);
}

int main(int argc, char **argv) {
  PerfScenario scenario = PERF_SCENARIO_IDLE;
  int duration_seconds = 12;
  int width = 1280;
  int height = 820;
  double warmup_seconds = 1.0;
  uint32_t scene_count = 10000u;
  uint32_t mutate_count = 256u;
  bool first_frame = true;
  bool show_perf = true;
  bool perf_scope_visible_prev = false;
  bool capture_started = false;
  bool raw_mode = false;
  bool unattended_bench = false;
  bool tick_hz_overridden = false;
  double start_time;
  double capture_start_time = 0.0;
  double last_tick_time;
  double next_interval_time;
  double tick_hz = 30.0;
  uint32_t second_index = 0u;
  uint32_t tick_count = 0u;
  float auto_scroll_y = 0.0f;
  float auto_scroll_dir = 1.0f;
  char editor_buffer[32768];
  StygianTextArea editor_state;
  StygianMiniPerfHarness perf;
  PerfIntervalStats interval_stats;
  PerfIntervalStats capture_stats;
  const StygianScopeId k_scope_chrome = 0x4301u;
  const StygianScopeId k_scope_scene_static = 0x4302u;
  const StygianScopeId k_scope_scene_dynamic = 0x4305u;
  const StygianScopeId k_scope_overlay =
      STYGIAN_OVERLAY_SCOPE_BASE | (StygianScopeId)0x4303u;
  const StygianScopeId k_scope_perf =
      STYGIAN_OVERLAY_SCOPE_BASE | (StygianScopeId)0x4304u;
  const char *scenario_label;
  const char *csv_path = NULL;
  FILE *csv_file = NULL;
  StygianWindowConfig win_cfg;
  StygianWindow *window;
  StygianConfig cfg;
  StygianContext *ctx;
  StygianFont font;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--scenario") == 0 && (i + 1) < argc) {
      scenario = parse_scenario(argv[++i]);
    } else if (strcmp(argv[i], "--seconds") == 0 && (i + 1) < argc) {
      duration_seconds = atoi(argv[++i]);
      if (duration_seconds < 2)
        duration_seconds = 2;
    } else if (strcmp(argv[i], "--warmup-seconds") == 0 && (i + 1) < argc) {
      warmup_seconds = atof(argv[++i]);
      if (warmup_seconds < 0.0)
        warmup_seconds = 0.0;
    } else if (strcmp(argv[i], "--count") == 0 && (i + 1) < argc) {
      unsigned long parsed = strtoul(argv[++i], NULL, 10);
      scene_count = (parsed == 0ul) ? 1u : (uint32_t)parsed;
    } else if (strcmp(argv[i], "--mutate-count") == 0 && (i + 1) < argc) {
      unsigned long parsed = strtoul(argv[++i], NULL, 10);
      mutate_count = (uint32_t)parsed;
    } else if (strcmp(argv[i], "--tick-hz") == 0 && (i + 1) < argc) {
      tick_hz = atof(argv[++i]);
      if (tick_hz < 0.0)
        tick_hz = 0.0;
      tick_hz_overridden = true;
    } else if (strcmp(argv[i], "--raw") == 0) {
      raw_mode = true;
    } else if (strcmp(argv[i], "--no-perf") == 0) {
      show_perf = false;
    } else if (strcmp(argv[i], "--csv") == 0 && (i + 1) < argc) {
      csv_path = argv[++i];
    }
  }

  if (mutate_count > scene_count)
    mutate_count = scene_count;
  if (raw_mode) {
    if (!tick_hz_overridden)
      tick_hz = 0.0;
    show_perf = false;
    unattended_bench = true;
  }
  if (duration_seconds <= (int)ceil(warmup_seconds))
    duration_seconds = (int)ceil(warmup_seconds) + 1;

  scenario_label = scenario_name(scenario);
  memset(&win_cfg, 0, sizeof(win_cfg));
  memset(&cfg, 0, sizeof(cfg));

  win_cfg.title = "Stygian Pathological Perf Suite";
  win_cfg.width = width;
  win_cfg.height = height;
  win_cfg.flags = STYGIAN_SUITE_WINDOW_FLAGS;
  win_cfg.role = STYGIAN_ROLE_MAIN;
  win_cfg.gl_major = 4;
  win_cfg.gl_minor = 3;

  window = stygian_window_create(&win_cfg);
  if (!window) {
    printf("BENCHERR stage=window_create\n");
    return 1;
  }

  cfg.backend = STYGIAN_SUITE_BACKEND;
  if (scenario == PERF_SCENARIO_STATIC || scenario == PERF_SCENARIO_SPARSE ||
      scenario == PERF_SCENARIO_FULLHOT) {
    cfg.max_elements = scene_count + 16384u;
  } else {
    cfg.max_elements = 0u;
  }
  cfg.max_textures = 0u;
  cfg.glyph_feature_flags = 0u;
  cfg.window = window;
  cfg.shader_dir = NULL;
  cfg.persistent_allocator = NULL;

  ctx = stygian_create(&cfg);
  if (!ctx) {
    printf("BENCHERR stage=context_create\n");
    return 1;
  }
  stygian_set_vsync(ctx, false);
  if (raw_mode) {
    stygian_set_present_enabled(ctx, false);
    stygian_set_gpu_timing_enabled(ctx, false);
    stygian_set_output_icc_auto(ctx, false);
  }
  font = stygian_get_default_font(ctx);
  printf("BENCHCFG scenario=%s backend=%s mode=%s count=%u mutate=%u tick_hz=%.2f warmup=%.2f\n",
         scenario_label, STYGIAN_SUITE_RENDERER_NAME,
         raw_mode ? "raw" : "present", scene_count, mutate_count, tick_hz,
         warmup_seconds);
  if (csv_path) {
    csv_file = fopen(csv_path, "w");
    if (!csv_file) {
      printf("BENCHERR stage=csv_open path=%s\n", csv_path);
      if (font)
        stygian_font_destroy(ctx, font);
      stygian_destroy(ctx);
      stygian_window_destroy(window);
      return 1;
    }
    fprintf(csv_file,
            "second,scenario,backend,mode,count,mutate,tick_hz,render_frames,eval_frames,draw_calls,elements,clip_count,replay_hits,replay_misses,replay_forced,render_ms,gpu_ms,build_ms,submit_ms,present_ms,upload_bytes,upload_ranges,cmd_applied,cmd_drops\n");
    fflush(csv_file);
  }

  memset(&interval_stats, 0, sizeof(interval_stats));
  memset(&capture_stats, 0, sizeof(capture_stats));
  memset(editor_buffer, 0, sizeof(editor_buffer));
  snprintf(editor_buffer, sizeof(editor_buffer),
           "// pathological text churn\n"
           "let perf = stygian::perf();\n"
           "fn mutate() { /* append */ }\n");
  memset(&editor_state, 0, sizeof(editor_state));
  editor_state.buffer = editor_buffer;
  editor_state.buffer_size = (int)sizeof(editor_buffer);
  editor_state.buffer_len = (int)strlen(editor_buffer);
  editor_state.content_revision = 1u;
  editor_state.cursor_idx = (int)strlen(editor_buffer);
  editor_state.selection_start = editor_state.cursor_idx;
  editor_state.selection_end = editor_state.cursor_idx;

  stygian_mini_perf_init(&perf, "perf_pathological_suite");
  perf.widget.renderer_name = STYGIAN_SUITE_RENDERER_NAME;
  perf.widget.enabled = show_perf;
  perf.widget.show_graph = true;
  perf.widget.idle_hz = 30u;
  perf.widget.active_hz = 30u;
  perf.widget.max_stress_hz = 120u;
  perf.widget.graph_max_segments = 64u;

  start_time = now_seconds();
  last_tick_time = start_time;
  next_interval_time = start_time + warmup_seconds + 1.0;

  while (!stygian_window_should_close(window)) {
    StygianEvent event;
    bool event_mutated = false;
    bool event_requested = false;
    bool event_eval_requested = false;
    bool scene_static_changed = false;
    bool scene_dynamic_changed = false;
    bool overlay_changed = false;
    bool chrome_changed = false;
    bool frame_is_first;
    bool repaint_pending;
    bool render_frame;
    bool eval_only_frame;
    double current_time = now_seconds();
    double dt;
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
        if (event.type == STYGIAN_EVENT_CLOSE)
          stygian_window_request_close(window);
      }
    }

    if (!unattended_bench && !first_frame && !event_mutated && !event_requested &&
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
        if (event.type == STYGIAN_EVENT_CLOSE)
          stygian_window_request_close(window);
      }
    }

    current_time = now_seconds();
    dt = current_time - last_tick_time;
    if (tick_hz <= 0.0 || dt > (1.0 / tick_hz)) {
      tick_count++;
      last_tick_time = current_time;
      if (scenario == PERF_SCENARIO_OVERLAY) {
        overlay_changed = true;
      } else if (scenario == PERF_SCENARIO_SPARSE) {
        scene_dynamic_changed = true;
      } else if (scenario == PERF_SCENARIO_FULLHOT) {
        scene_dynamic_changed = true;
      } else if (scenario == PERF_SCENARIO_CLIP) {
        scene_dynamic_changed = true;
      } else if (scenario == PERF_SCENARIO_SCROLL) {
        auto_scroll_y += auto_scroll_dir * 22.0f;
        if (auto_scroll_y > 5400.0f) {
          auto_scroll_y = 5400.0f;
          auto_scroll_dir = -1.0f;
        } else if (auto_scroll_y < 0.0f) {
          auto_scroll_y = 0.0f;
          auto_scroll_dir = 1.0f;
        }
        scene_dynamic_changed = true;
      } else if (scenario == PERF_SCENARIO_TEXT) {
        size_t len = (editor_state.buffer_len >= 0)
                         ? (size_t)editor_state.buffer_len
                         : strlen(editor_buffer);
        if (len + 3 < sizeof(editor_buffer)) {
          editor_buffer[len] = (char)('a' + (tick_count % 26u));
          editor_buffer[len + 1] = '\n';
          editor_buffer[len + 2] = '\0';
          stygian_text_area_mark_text_dirty(&editor_state);
          editor_state.cursor_idx = (int)(len + 2u);
          editor_state.selection_start = editor_state.cursor_idx;
          editor_state.selection_end = editor_state.cursor_idx;
          scene_dynamic_changed = true;
        }
      }
    }

    if (scene_static_changed || scene_dynamic_changed || overlay_changed ||
        chrome_changed || event_mutated) {
      stygian_set_repaint_source(ctx, "mutation");
      stygian_request_repaint_after_ms(ctx, 0u);
    }

    repaint_pending = stygian_has_pending_repaint(ctx);
    render_frame = first_frame || event_mutated || scene_static_changed ||
                   scene_dynamic_changed ||
                   overlay_changed || repaint_pending;
    eval_only_frame =
        (!render_frame && (event_eval_requested || event_requested));
    if (!render_frame && !eval_only_frame) {
      current_time = now_seconds();
      if (!capture_started && (current_time - start_time) >= warmup_seconds) {
        capture_started = true;
        capture_start_time = current_time;
        next_interval_time = capture_start_time + 1.0;
        second_index = 0u;
        memset(&interval_stats, 0, sizeof(interval_stats));
        memset(&capture_stats, 0, sizeof(capture_stats));
        printf("BENCHNOTE capture_start=%.3f\n", capture_start_time - start_time);
      }
      if (capture_started) {
        // Idle is supposed to go quiet. Still emit timing rows so the perf
        // gates don't pretend the harness exploded.
        interval_add_idle_sample(&interval_stats);
        interval_add_idle_sample(&capture_stats);
      }
      if (capture_started && current_time >= next_interval_time) {
        PerfIntervalRow row =
            interval_row_make(&interval_stats, second_index + 1u, ctx);
        second_index++;
        interval_log(&row, scenario_label, scene_count, mutate_count, raw_mode,
                     tick_hz);
        interval_write_csv(csv_file, &row, scenario_label, scene_count,
                           mutate_count, raw_mode, tick_hz);
        memset(&interval_stats, 0, sizeof(interval_stats));
        next_interval_time += 1.0;
      }
      if ((current_time - start_time) >= (double)duration_seconds) {
        break;
      }
      continue;
    }
    frame_is_first = first_frame;
    first_frame = false;

    stygian_window_get_size(window, &width, &height);
    stygian_begin_frame_intent(
        ctx, width, height,
        eval_only_frame ? STYGIAN_FRAME_EVAL_ONLY : STYGIAN_FRAME_RENDER);

    stygian_scope_begin(ctx, k_scope_chrome);
    stygian_rect(ctx, 0.0f, 0.0f, (float)width, 44.0f, 0.09f, 0.11f, 0.14f,
                 1.0f);
    if (font) {
      char title[128];
      snprintf(title, sizeof(title), "Pathological suite: %s (%s)",
               scenario_label, STYGIAN_SUITE_RENDERER_NAME);
      stygian_text(ctx, font, title, 14.0f, 13.0f, 15.0f, 0.95f, 0.96f, 0.99f,
                   1.0f);
    }
    stygian_scope_end(ctx);

    stygian_scope_begin(ctx, k_scope_scene_static);
    if (scenario == PERF_SCENARIO_IDLE) {
      stygian_rect_rounded(ctx, 24.0f, 80.0f, (float)width - 48.0f,
                           (float)height - 120.0f, 0.12f, 0.13f, 0.16f, 1.0f,
                           8.0f);
      if (font) {
        stygian_text(ctx, font, "Idle scenario: no active mutation path.",
                     40.0f, 112.0f, 18.0f, 0.8f, 0.85f, 0.9f, 1.0f);
      }
    } else if (scenario == PERF_SCENARIO_STATIC ||
               scenario == PERF_SCENARIO_SPARSE) {
      render_sparse_static_scene(ctx, scene_count, width, height);
    } else if (scenario == PERF_SCENARIO_SCROLL) {
      render_scroll_shell(ctx, width, height);
    } else if (scenario == PERF_SCENARIO_OVERLAY) {
      stygian_rect_rounded(ctx, 24.0f, 80.0f, (float)width - 48.0f,
                           (float)height - 120.0f, 0.11f, 0.12f, 0.15f, 1.0f,
                           8.0f);
      if (font) {
        stygian_text(ctx, font, "Overlay scenario: base scope should stay clean.",
                     38.0f, 116.0f, 17.0f, 0.8f, 0.85f, 0.9f, 1.0f);
      }
    } else {
      stygian_rect_rounded(ctx, 24.0f, 80.0f, (float)width - 48.0f,
                           (float)height - 120.0f, 0.12f, 0.13f, 0.16f, 1.0f,
                           8.0f);
    }
    stygian_scope_end(ctx);

    stygian_scope_begin(ctx, k_scope_scene_dynamic);
    if (scenario == PERF_SCENARIO_SPARSE) {
      render_sparse_dynamic_scene(ctx, scene_count, mutate_count, width, height,
                                  tick_count);
    } else if (scenario == PERF_SCENARIO_FULLHOT) {
      render_fullhot_scene(ctx, scene_count, width, height, tick_count);
    } else if (scenario == PERF_SCENARIO_CLIP) {
      render_clip_scene(ctx, tick_count, width, height);
    } else if (scenario == PERF_SCENARIO_SCROLL) {
      if (render_scroll_rows(ctx, font, &auto_scroll_y, width, height)) {
        scene_dynamic_changed = true;
      }
    } else if (scenario == PERF_SCENARIO_TEXT ||
               scenario == PERF_SCENARIO_TEXT_STATIC) {
      editor_state.x = 30.0f;
      editor_state.y = 74.0f;
      editor_state.w = (float)width - 60.0f;
      editor_state.h = (float)height - 120.0f;
      render_text_scene(ctx, font, &editor_state);
    }
    stygian_scope_end(ctx);

    stygian_scope_begin(ctx, k_scope_overlay);
    if (scenario == PERF_SCENARIO_OVERLAY) {
      stygian_request_overlay_hz(ctx, 30u);
      stygian_line(ctx, 36.0f, (float)height - 70.0f,
                   36.0f + (float)((tick_count * 7u) % 600u),
                   (float)height - 70.0f, 1.8f, 0.2f, 0.85f, 0.42f, 1.0f);
    }
    stygian_scope_end(ctx);

    if (show_perf) {
      stygian_scope_begin(ctx, k_scope_perf);
      stygian_mini_perf_draw(ctx, font, &perf, width, height);
      stygian_scope_end(ctx);
      perf_scope_visible_prev = true;
    }

    if (frame_is_first || event_mutated) {
      stygian_scope_invalidate_next(ctx, k_scope_scene_static);
      stygian_scope_invalidate_next(ctx, k_scope_scene_dynamic);
    } else {
      if (scene_static_changed) {
        stygian_scope_invalidate_next(ctx, k_scope_scene_static);
      }
      if (scene_dynamic_changed) {
        stygian_scope_invalidate_next(ctx, k_scope_scene_dynamic);
      }
    }
    if (overlay_changed) {
      stygian_scope_invalidate_next(ctx, k_scope_overlay);
    }
    if (!show_perf && perf_scope_visible_prev) {
      // Clear cached perf scope once when hiding; repeated invalidation keeps
      // idle scenarios rendering even with --no-perf.
      stygian_scope_invalidate_next(ctx, k_scope_perf);
      perf_scope_visible_prev = false;
    }
    if (raw_mode) {
      stygian_set_repaint_source(ctx, "benchmark_raw");
      stygian_request_repaint_after_ms(ctx, 0u);
    }

    stygian_widgets_commit_regions();
    stygian_end_frame(ctx);
    stygian_mini_perf_accumulate(&perf, eval_only_frame);
    current_time = now_seconds();
    if (!capture_started && (current_time - start_time) >= warmup_seconds) {
      capture_started = true;
      capture_start_time = current_time;
      next_interval_time = capture_start_time + 1.0;
      second_index = 0u;
      memset(&interval_stats, 0, sizeof(interval_stats));
      memset(&capture_stats, 0, sizeof(capture_stats));
      printf("BENCHNOTE capture_start=%.3f\n", capture_start_time - start_time);
    }
    if (capture_started) {
      interval_add_sample(ctx, &interval_stats, eval_only_frame);
      interval_add_sample(ctx, &capture_stats, eval_only_frame);
    }

    if (capture_started && current_time >= next_interval_time) {
      PerfIntervalRow row = interval_row_make(&interval_stats, second_index + 1u, ctx);
      second_index++;
      interval_log(&row, scenario_label, scene_count, mutate_count, raw_mode,
                   tick_hz);
      interval_write_csv(csv_file, &row, scenario_label, scene_count,
                         mutate_count, raw_mode, tick_hz);
      memset(&interval_stats, 0, sizeof(interval_stats));
      next_interval_time += 1.0;
    }

    if ((current_time - start_time) >= (double)duration_seconds) {
      break;
    }
  }

  if (capture_started && interval_stats.samples > 0u) {
    PerfIntervalRow row = interval_row_make(&interval_stats, second_index + 1u, ctx);
    second_index++;
    interval_log(&row, scenario_label, scene_count, mutate_count, raw_mode,
                 tick_hz);
    interval_write_csv(csv_file, &row, scenario_label, scene_count,
                       mutate_count, raw_mode, tick_hz);
  }
  if (capture_started && capture_stats.samples > 0u) {
    summary_log(&capture_stats, scenario_label, scene_count, mutate_count,
                raw_mode, tick_hz, now_seconds() - capture_start_time);
  }

  if (font)
    stygian_font_destroy(ctx, font);
  if (csv_file)
    fclose(csv_file);
  stygian_destroy(ctx);
  stygian_window_destroy(window);
  return 0;
}
