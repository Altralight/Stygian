#ifndef STYGIAN_BENCH_COMMON_H
#define STYGIAN_BENCH_COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

#define BENCH_LABEL_CAP 32

typedef enum BenchScenario {
    BENCH_SCENARIO_STATIC = 0,
    BENCH_SCENARIO_SPARSE = 1,
    BENCH_SCENARIO_FULLHOT = 2,
} BenchScenario;

typedef enum BenchSceneKind {
    BENCH_SCENE_CARDS = 0,
    BENCH_SCENE_TEXT_WALL = 1,
    BENCH_SCENE_GRAPH = 2,
    BENCH_SCENE_INSPECTOR = 3,
    BENCH_SCENE_HIERARCHY = 4,
} BenchSceneKind;

typedef struct BenchConfig {
    int width;
    int height;
    int count;
    int mutate_count;
    int columns;
    double seconds;
    double warmup_seconds;
    float cell_width;
    float cell_height;
    float gap;
    const char *csv_path;
    const char *mode_name;
    BenchSceneKind scene_kind;
    const char *scene_name;
    BenchScenario scenario;
    const char *scenario_name;
} BenchConfig;

typedef struct BenchFrameMetrics {
    double frame_ms;
    uint64_t command_count;
    uint64_t vertex_count;
    uint64_t index_count;
    uint64_t text_bytes;
} BenchFrameMetrics;

typedef struct BenchAccumulator {
    uint64_t frames;
    double total_frame_ms;
    double min_frame_ms;
    double max_frame_ms;
    double total_wall_seconds;
    uint64_t total_command_count;
    uint64_t total_vertex_count;
    uint64_t total_index_count;
    uint64_t total_text_bytes;
} BenchAccumulator;

typedef struct BenchScene {
    char *labels;
    uint8_t *label_lens;
    uint64_t total_text_bytes;
} BenchScene;

static inline double bench_now_seconds(void) {
#ifdef _WIN32
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER now;
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart / (double)freq.QuadPart;
#else
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
#endif
}

static inline const char *bench_scenario_name(BenchScenario scenario) {
    switch (scenario) {
        case BENCH_SCENARIO_STATIC: return "static";
        case BENCH_SCENARIO_SPARSE: return "sparse";
        case BENCH_SCENARIO_FULLHOT: return "fullhot";
        default: return "static";
    }
}

static inline const char *bench_scene_kind_name(BenchSceneKind scene_kind) {
    switch (scene_kind) {
        case BENCH_SCENE_CARDS: return "cards";
        case BENCH_SCENE_TEXT_WALL: return "textwall";
        case BENCH_SCENE_GRAPH: return "graph";
        case BENCH_SCENE_INSPECTOR: return "inspector";
        case BENCH_SCENE_HIERARCHY: return "hierarchy";
        default: return "cards";
    }
}

static inline BenchScenario bench_parse_scenario(const char *value) {
    if (strcmp(value, "static") == 0) {
        return BENCH_SCENARIO_STATIC;
    }
    if (strcmp(value, "sparse") == 0) {
        return BENCH_SCENARIO_SPARSE;
    }
    if (strcmp(value, "fullhot") == 0) {
        return BENCH_SCENARIO_FULLHOT;
    }
    fprintf(stderr, "unknown scenario: %s\n", value);
    exit(2);
}

static inline BenchSceneKind bench_parse_scene_kind(const char *value) {
    if (strcmp(value, "cards") == 0) {
        return BENCH_SCENE_CARDS;
    }
    if (strcmp(value, "textwall") == 0) {
        return BENCH_SCENE_TEXT_WALL;
    }
    if (strcmp(value, "graph") == 0) {
        return BENCH_SCENE_GRAPH;
    }
    if (strcmp(value, "inspector") == 0) {
        return BENCH_SCENE_INSPECTOR;
    }
    if (strcmp(value, "hierarchy") == 0) {
        return BENCH_SCENE_HIERARCHY;
    }
    fprintf(stderr, "unknown scene: %s\n", value);
    exit(2);
}

static inline BenchConfig bench_default_config(void) {
    BenchConfig cfg;
    cfg.width = 1920;
    cfg.height = 1080;
    cfg.count = 10000;
    cfg.mutate_count = 100;
    cfg.columns = 16;
    cfg.seconds = 2.0;
    cfg.warmup_seconds = 1.0;
    cfg.cell_width = 112.0f;
    cfg.cell_height = 34.0f;
    cfg.gap = 6.0f;
    cfg.csv_path = NULL;
    cfg.mode_name = "headless-primitive";
    cfg.scene_kind = BENCH_SCENE_CARDS;
    cfg.scene_name = bench_scene_kind_name(cfg.scene_kind);
    cfg.scenario = BENCH_SCENARIO_STATIC;
    cfg.scenario_name = bench_scenario_name(cfg.scenario);
        return cfg;
}

static inline void bench_usage(const char *program_name) {
    fprintf(stderr,
        "usage: %s [--scene cards|textwall|graph|inspector|hierarchy] [--scenario static|sparse|fullhot] [--count N] [--mutate-count N]\n"
        "          [--seconds S] [--warmup-seconds S] [--columns N] [--width W] [--height H]\n"
        "          [--csv path]\n",
        program_name);
}

static inline BenchConfig bench_parse_args(int argc, char **argv) {
    BenchConfig cfg = bench_default_config();
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--scene") == 0 && i + 1 < argc) {
            cfg.scene_kind = bench_parse_scene_kind(argv[++i]);
        } else if (strcmp(argv[i], "--scenario") == 0 && i + 1 < argc) {
            cfg.scenario = bench_parse_scenario(argv[++i]);
        } else if (strcmp(argv[i], "--count") == 0 && i + 1 < argc) {
            cfg.count = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--mutate-count") == 0 && i + 1 < argc) {
            cfg.mutate_count = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--seconds") == 0 && i + 1 < argc) {
            cfg.seconds = atof(argv[++i]);
        } else if (strcmp(argv[i], "--warmup-seconds") == 0 && i + 1 < argc) {
            cfg.warmup_seconds = atof(argv[++i]);
        } else if (strcmp(argv[i], "--columns") == 0 && i + 1 < argc) {
            cfg.columns = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            cfg.width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            cfg.height = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--csv") == 0 && i + 1 < argc) {
            cfg.csv_path = argv[++i];
        } else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            cfg.mode_name = argv[++i];
        } else {
            bench_usage(argv[0]);
            exit(2);
        }
    }

    if (cfg.count <= 0 || cfg.columns <= 0 || cfg.seconds <= 0.0 || cfg.warmup_seconds < 0.0) {
        fprintf(stderr, "bad benchmark config\n");
        exit(2);
    }
    if (cfg.mutate_count < 0) {
        cfg.mutate_count = 0;
    }
    if (cfg.mutate_count > cfg.count) {
        cfg.mutate_count = cfg.count;
    }
    cfg.scene_name = bench_scene_kind_name(cfg.scene_kind);
    cfg.scenario_name = bench_scenario_name(cfg.scenario);
    return cfg;
}

static inline void bench_accumulator_reset(BenchAccumulator *acc) {
    memset(acc, 0, sizeof(*acc));
    acc->min_frame_ms = 1e30;
}

static inline void bench_accumulator_add(BenchAccumulator *acc, const BenchFrameMetrics *frame) {
    acc->frames += 1;
    acc->total_frame_ms += frame->frame_ms;
    if (frame->frame_ms < acc->min_frame_ms) {
        acc->min_frame_ms = frame->frame_ms;
    }
    if (frame->frame_ms > acc->max_frame_ms) {
        acc->max_frame_ms = frame->frame_ms;
    }
    acc->total_command_count += frame->command_count;
    acc->total_vertex_count += frame->vertex_count;
    acc->total_index_count += frame->index_count;
    acc->total_text_bytes += frame->text_bytes;
}

static inline bool bench_item_is_hot(const BenchConfig *cfg, uint64_t frame_index, int item_index) {
    if (cfg->scenario == BENCH_SCENARIO_FULLHOT) {
        return true;
    }
    if (cfg->scenario != BENCH_SCENARIO_SPARSE || cfg->mutate_count == 0) {
        return false;
    }

    // rotating hot window so we don't benchmark a cute special case
    int start = (int)((frame_index * (uint64_t)cfg->mutate_count) % (uint64_t)cfg->count);
    int end = start + cfg->mutate_count;
    if (end <= cfg->count) {
        return item_index >= start && item_index < end;
    }
    return item_index >= start || item_index < (end - cfg->count);
}

static inline uint32_t bench_rgba_u32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | (uint32_t)a;
}

static inline uint32_t bench_fill_color(const BenchConfig *cfg, uint64_t frame_index, int item_index) {
    bool hot = bench_item_is_hot(cfg, frame_index, item_index);
    uint8_t r = (uint8_t)(26 + (item_index % 5) * 8);
    uint8_t g = (uint8_t)(31 + (item_index % 7) * 6);
    uint8_t b = (uint8_t)(39 + (item_index % 11) * 5);
    if (hot) {
        r = (uint8_t)(60 + ((frame_index + (uint64_t)item_index * 3u) % 120u));
        g = (uint8_t)(110 + ((frame_index + (uint64_t)item_index * 5u) % 100u));
        b = (uint8_t)(180 + ((frame_index + (uint64_t)item_index * 7u) % 60u));
    }
    return bench_rgba_u32(r, g, b, 255);
}

static inline uint32_t bench_text_color(void) {
    return bench_rgba_u32(230, 233, 240, 255);
}

static inline uint32_t bench_hot_text_color(void) {
    return bench_rgba_u32(163, 214, 255, 255);
}

static inline uint32_t bench_wire_color(void) {
    return bench_rgba_u32(74, 158, 255, 220);
}

static inline uint32_t bench_hot_wire_color(void) {
    return bench_rgba_u32(110, 210, 255, 255);
}

static inline int bench_inspector_columns(const BenchConfig *cfg) {
    if (cfg->columns <= 0) {
        return 1;
    }
    return cfg->columns > 4 ? 4 : cfg->columns;
}

static inline void bench_cell_rect(const BenchConfig *cfg, int item_index, float *x, float *y, float *w, float *h) {
    int row = item_index / cfg->columns;
    int column = item_index % cfg->columns;
    *w = cfg->cell_width;
    *h = cfg->cell_height;
    *x = cfg->gap + (float)column * (cfg->cell_width + cfg->gap);
    *y = cfg->gap + (float)row * (cfg->cell_height + cfg->gap);
}

static inline void bench_text_line_rect(const BenchConfig *cfg, int item_index, float *x, float *y, float *w, float *h) {
    const float line_height = 18.0f;
    const float column_width = 248.0f;
    int row = item_index / cfg->columns;
    int column = item_index % cfg->columns;
    *w = column_width;
    *h = line_height;
    *x = 14.0f + (float)column * (column_width + 24.0f);
    *y = 14.0f + (float)row * line_height;
}

static inline void bench_graph_node_rect(const BenchConfig *cfg, int item_index, float *x, float *y, float *w, float *h) {
    const float node_width = 136.0f;
    const float node_height = 78.0f;
    const float pitch_x = 172.0f;
    const float pitch_y = 112.0f;
    int row = item_index / cfg->columns;
    int column = item_index % cfg->columns;
    float stagger = (row & 1) ? 18.0f : -18.0f;
    *w = node_width;
    *h = node_height;
    *x = 28.0f + (float)column * pitch_x + stagger;
    *y = 24.0f + (float)row * pitch_y + (float)((column % 3) * 7);
}

static inline void bench_inspector_row_rect(const BenchConfig *cfg, int item_index, float *x, float *y, float *w, float *h) {
    const float row_height = 28.0f;
    const float pane_width = 360.0f;
    const float pane_gap = 32.0f;
    const int columns = bench_inspector_columns(cfg);
    int row = item_index / columns;
    int column = item_index % columns;
    *w = pane_width;
    *h = 24.0f;
    *x = 20.0f + (float)column * (pane_width + pane_gap);
    *y = 18.0f + (float)row * row_height;
}

static inline int bench_hierarchy_columns(const BenchConfig *cfg) {
    if (cfg->columns <= 0) {
        return 1;
    }
    return cfg->columns > 3 ? 3 : cfg->columns;
}

static inline void bench_hierarchy_row_rect(const BenchConfig *cfg, int item_index,
                                            float *x, float *y, float *w, float *h) {
    const float row_height = 24.0f;
    const float pane_width = 320.0f;
    const float pane_gap = 26.0f;
    const int columns = bench_hierarchy_columns(cfg);
    int row = item_index / columns;
    int column = item_index % columns;
    *w = pane_width;
    *h = 20.0f;
    *x = 18.0f + (float)column * (pane_width + pane_gap);
    *y = 18.0f + (float)row * row_height;
}

static inline int bench_hierarchy_depth(int item_index) {
    int depth = 0;
    int cursor = item_index;
    while ((cursor % 7) != 0 && depth < 4) {
        depth += 1;
        cursor /= 2;
    }
    return depth;
}

static inline int bench_hierarchy_has_children(int item_index) {
    return (item_index % 5) != 4;
}

static inline uint32_t bench_hierarchy_row_fill_color(const BenchConfig *cfg,
                                                      uint64_t frame_index,
                                                      int item_index) {
    if (bench_item_is_hot(cfg, frame_index, item_index)) {
        return bench_rgba_u32(38, 66, 103, 255);
    }
    if ((item_index % 6) == 0) {
        return bench_rgba_u32(28, 33, 42, 255);
    }
    return bench_rgba_u32(22, 26, 33, 220);
}

static inline uint32_t bench_hierarchy_marker_fill_color(const BenchConfig *cfg,
                                                         uint64_t frame_index,
                                                         int item_index) {
    if (bench_item_is_hot(cfg, frame_index, item_index)) {
        return bench_hot_wire_color();
    }
    return bench_rgba_u32(74, 86, 104, 255);
}

static inline uint32_t bench_hierarchy_kind_fill_color(const BenchConfig *cfg,
                                                       uint64_t frame_index,
                                                       int item_index) {
    (void)cfg;
    (void)frame_index;
    switch (item_index % 4) {
        case 0: return bench_rgba_u32(49, 55, 70, 255);
        case 1: return bench_rgba_u32(58, 70, 92, 255);
        case 2: return bench_rgba_u32(61, 79, 63, 255);
        default: return bench_rgba_u32(82, 64, 45, 255);
    }
}

static inline const char *bench_hierarchy_kind_name(int item_index) {
    switch (item_index % 4) {
        case 0: return "Mesh";
        case 1: return "Light";
        case 2: return "Group";
        default: return "Cam";
    }
}

static inline size_t bench_hierarchy_kind_name_len(int item_index) {
    return strlen(bench_hierarchy_kind_name(item_index));
}

static inline void bench_scene_rect(const BenchConfig *cfg, int item_index, float *x, float *y, float *w, float *h) {
    if (cfg->scene_kind == BENCH_SCENE_TEXT_WALL) {
        bench_text_line_rect(cfg, item_index, x, y, w, h);
        return;
    }
    if (cfg->scene_kind == BENCH_SCENE_GRAPH) {
        bench_graph_node_rect(cfg, item_index, x, y, w, h);
        return;
    }
    if (cfg->scene_kind == BENCH_SCENE_INSPECTOR) {
        bench_inspector_row_rect(cfg, item_index, x, y, w, h);
        return;
    }
    if (cfg->scene_kind == BENCH_SCENE_HIERARCHY) {
        bench_hierarchy_row_rect(cfg, item_index, x, y, w, h);
        return;
    }
    bench_cell_rect(cfg, item_index, x, y, w, h);
}

static inline void bench_scene_bounds(const BenchConfig *cfg, float *out_w, float *out_h) {
    if (cfg->count <= 0) {
        *out_w = (float)cfg->width;
        *out_h = (float)cfg->height;
        return;
    }

    float max_x = 0.0f;
    float max_y = 0.0f;
    for (int i = 0; i < cfg->count; ++i) {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        bench_scene_rect(cfg, i, &x, &y, &w, &h);
        if (i == 0 || (x + w) > max_x) {
            max_x = x + w;
        }
        if (i == 0 || (y + h) > max_y) {
            max_y = y + h;
        }
    }
    *out_w = max_x + 24.0f;
    *out_h = max_y + 24.0f;
}

static inline int bench_graph_has_right_link(const BenchConfig *cfg, int item_index) {
    int column = item_index % cfg->columns;
    return (column + 1 < cfg->columns) && (item_index + 1 < cfg->count);
}

static inline int bench_graph_has_down_link(const BenchConfig *cfg, int item_index) {
    return (item_index + cfg->columns) < cfg->count;
}

static inline int bench_graph_has_diag_link(const BenchConfig *cfg, int item_index) {
    int column = item_index % cfg->columns;
    return ((item_index % 3) == 0) &&
           (column + 1 < cfg->columns) &&
           (item_index + cfg->columns + 1 < cfg->count);
}

static inline uint32_t bench_inspector_value_fill_color(const BenchConfig *cfg, uint64_t frame_index, int item_index) {
    (void)frame_index;
    if (bench_item_is_hot(cfg, frame_index, item_index)) {
        return bench_hot_wire_color();
    }
    return bench_rgba_u32(49, 55, 70, 255);
}

static inline int bench_inspector_value_text(char *buffer, size_t buffer_cap, const BenchConfig *cfg,
                                             uint64_t frame_index, int item_index) {
    const bool hot = bench_item_is_hot(cfg, frame_index, item_index);
    switch (item_index & 3) {
        case 0:
            return snprintf(buffer, buffer_cap, "%s",
                            hot ? "Enabled" : ((item_index & 8) ? "Disabled" : "Enabled"));
        case 1: {
            const double base = 0.15 + (double)((item_index * 7) % 700) / 1000.0;
            const double value = base + (hot ? 0.125 : 0.0);
            return snprintf(buffer, buffer_cap, "%.3f", value);
        }
        case 2: {
            const int px = 8 + (int)((item_index * 13 + (hot ? (int)frame_index : 0)) % 512);
            return snprintf(buffer, buffer_cap, "%d px", px);
        }
        default: {
            const uint32_t color = bench_fill_color(cfg, frame_index, item_index);
            return snprintf(buffer, buffer_cap, "#%02X%02X%02X",
                            (unsigned)((color >> 24) & 0xff),
                            (unsigned)((color >> 16) & 0xff),
                            (unsigned)((color >> 8) & 0xff));
        }
    }
}

static inline void bench_scene_init(BenchScene *scene, const BenchConfig *cfg) {
    scene->labels = (char *)malloc((size_t)cfg->count * BENCH_LABEL_CAP);
    scene->label_lens = (uint8_t *)malloc((size_t)cfg->count);
    scene->total_text_bytes = 0;
    if (!scene->labels || !scene->label_lens) {
        fprintf(stderr, "out of memory for labels\n");
        exit(3);
    }
    for (int i = 0; i < cfg->count; ++i) {
        char *label = scene->labels + ((size_t)i * BENCH_LABEL_CAP);
        char value_text[BENCH_LABEL_CAP];
        const int len = snprintf(
            label, BENCH_LABEL_CAP,
            cfg->scene_kind == BENCH_SCENE_GRAPH
                ? "Node %05d"
                : (cfg->scene_kind == BENCH_SCENE_INSPECTOR
                       ? "Prop %05d"
                       : (cfg->scene_kind == BENCH_SCENE_HIERARCHY
                              ? "Entry %05d"
                              : "Item %05d")),
            i);
        scene->label_lens[i] = (uint8_t)((len > 0) ? len : 0);
        scene->total_text_bytes += (uint64_t)scene->label_lens[i];
        if (cfg->scene_kind == BENCH_SCENE_INSPECTOR) {
            const int value_len = bench_inspector_value_text(value_text, sizeof(value_text), cfg, 0u, i);
            scene->total_text_bytes += (uint64_t)((value_len > 0) ? value_len : 0);
        } else if (cfg->scene_kind == BENCH_SCENE_HIERARCHY) {
            scene->total_text_bytes += (uint64_t)bench_hierarchy_kind_name_len(i);
        }
    }
}

static inline void bench_scene_free(BenchScene *scene) {
    free(scene->labels);
    free(scene->label_lens);
    scene->labels = NULL;
    scene->label_lens = NULL;
  scene->total_text_bytes = 0;
}

static inline uint32_t bench_scene_static_primitive_count(const BenchConfig *cfg) {
  uint64_t primitive_count = 0u;
  if (!cfg)
    return 0u;

  switch (cfg->scene_kind) {
    case BENCH_SCENE_CARDS:
      primitive_count = (uint64_t)cfg->count;
      break;
    case BENCH_SCENE_TEXT_WALL:
      primitive_count = 0u;
      break;
    case BENCH_SCENE_GRAPH: {
      uint64_t link_count = 0u;
      for (int i = 0; i < cfg->count; ++i) {
        if (bench_graph_has_right_link(cfg, i))
          link_count++;
        if (bench_graph_has_down_link(cfg, i))
          link_count++;
        if (bench_graph_has_diag_link(cfg, i))
          link_count++;
      }
      primitive_count = (uint64_t)cfg->count * 2u + link_count * 3u;
      break;
    }
    case BENCH_SCENE_INSPECTOR:
      primitive_count = (uint64_t)cfg->count * 2u;
      break;
    case BENCH_SCENE_HIERARCHY:
      primitive_count = (uint64_t)cfg->count * 3u;
      break;
    default:
      primitive_count = (uint64_t)cfg->count;
      break;
  }

  return primitive_count > UINT32_MAX ? UINT32_MAX : (uint32_t)primitive_count;
}

static inline uint32_t bench_stygian_element_budget(const BenchConfig *cfg,
                                                    const BenchScene *scene) {
  uint64_t static_prims;
  uint64_t static_text;
  uint64_t total;
  if (!cfg || !scene)
    return 0u;

  static_prims = (uint64_t)bench_scene_static_primitive_count(cfg);
  static_text = scene->total_text_bytes;

  switch (cfg->scene_kind) {
    case BENCH_SCENE_CARDS:
      total = static_text + static_prims + (uint64_t)cfg->count + 16384u;
      break;
    case BENCH_SCENE_TEXT_WALL:
      total = static_text * 2u + 8192u;
      break;
    case BENCH_SCENE_GRAPH:
    case BENCH_SCENE_INSPECTOR:
    case BENCH_SCENE_HIERARCHY:
      total = (static_text + static_prims) * 2u + 16384u;
      break;
    default:
      total = static_text + static_prims + 16384u;
      break;
  }

  if (total < 16384u)
    total = 16384u;
  if (total > UINT32_MAX)
    total = UINT32_MAX;
  return (uint32_t)total;
}

static inline const char *bench_label_at(const BenchScene *scene, int item_index) {
  return scene->labels + ((size_t)item_index * BENCH_LABEL_CAP);
}

static inline uint8_t bench_label_len_at(const BenchScene *scene, int item_index) {
    return scene->label_lens[item_index];
}

static inline double bench_average_or_zero(double total, uint64_t count) {
    return count ? (total / (double)count) : 0.0;
}

static inline uint64_t bench_average_u64_or_zero(uint64_t total, uint64_t count) {
    return count ? (total / count) : 0;
}

static inline void bench_summary_print(const char *library_name, const BenchConfig *cfg, const BenchAccumulator *acc) {
    double avg_frame_ms = bench_average_or_zero(acc->total_frame_ms, acc->frames);
    double wall_fps = (acc->total_wall_seconds > 0.0) ? ((double)acc->frames / acc->total_wall_seconds) : 0.0;

    printf(
        "COMPSUMMARY library=%s mode=%s scene=%s scenario=%s count=%d mutate=%d "
        "frames=%" PRIu64 " wall_fps=%.2f frame_ms=%.4f min_ms=%.4f max_ms=%.4f "
        "commands=%" PRIu64 " vertices=%" PRIu64 " indices=%" PRIu64 " text_bytes=%" PRIu64 "\n",
        library_name,
        cfg->mode_name ? cfg->mode_name : "headless-primitive",
        cfg->scene_name,
        cfg->scenario_name,
        cfg->count,
        cfg->mutate_count,
        acc->frames,
        wall_fps,
        avg_frame_ms,
        acc->min_frame_ms,
        acc->max_frame_ms,
        bench_average_u64_or_zero(acc->total_command_count, acc->frames),
        bench_average_u64_or_zero(acc->total_vertex_count, acc->frames),
        bench_average_u64_or_zero(acc->total_index_count, acc->frames),
        bench_average_u64_or_zero(acc->total_text_bytes, acc->frames));
}

static inline void bench_summary_append_csv(const char *library_name, const BenchConfig *cfg, const BenchAccumulator *acc) {
    if (!cfg->csv_path) {
        return;
    }

    bool write_header = true;
    FILE *probe = fopen(cfg->csv_path, "rb");
    if (probe) {
        fseek(probe, 0, SEEK_END);
        write_header = (ftell(probe) == 0);
        fclose(probe);
    }

    FILE *out = fopen(cfg->csv_path, "ab");
    if (!out) {
        fprintf(stderr, "failed to open csv output: %s\n", cfg->csv_path);
        exit(4);
    }

    if (write_header) {
        fputs("library,mode,scene,scenario,count,mutate_count,frames,wall_fps,avg_frame_ms,min_frame_ms,max_frame_ms,avg_commands,avg_vertices,avg_indices,avg_text_bytes\n", out);
    }

    double avg_frame_ms = bench_average_or_zero(acc->total_frame_ms, acc->frames);
    double wall_fps = (acc->total_wall_seconds > 0.0) ? ((double)acc->frames / acc->total_wall_seconds) : 0.0;
    fprintf(
        out,
        "%s,%s,%s,%s,%d,%d,%" PRIu64 ",%.4f,%.4f,%.4f,%.4f,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n",
        library_name,
        cfg->mode_name ? cfg->mode_name : "headless-primitive",
        cfg->scene_name,
        cfg->scenario_name,
        cfg->count,
        cfg->mutate_count,
        acc->frames,
        wall_fps,
        avg_frame_ms,
        acc->min_frame_ms,
        acc->max_frame_ms,
        bench_average_u64_or_zero(acc->total_command_count, acc->frames),
        bench_average_u64_or_zero(acc->total_vertex_count, acc->frames),
        bench_average_u64_or_zero(acc->total_index_count, acc->frames),
        bench_average_u64_or_zero(acc->total_text_bytes, acc->frames));
    fclose(out);
}

#endif
