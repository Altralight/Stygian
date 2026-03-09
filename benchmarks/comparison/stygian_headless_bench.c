#include "bench_common.h"

#include "../../include/stygian.h"
#include "../../window/stygian_window.h"

enum {
  STYGIAN_SCOPE_STATIC = 1,
  STYGIAN_SCOPE_DYNAMIC = 2,
};

static void stygian_color_from_rgba(uint32_t rgba, float *r, float *g, float *b,
                                    float *a) {
  *r = (float)((rgba >> 24) & 0xff) / 255.0f;
  *g = (float)((rgba >> 16) & 0xff) / 255.0f;
  *b = (float)((rgba >> 8) & 0xff) / 255.0f;
  *a = (float)(rgba & 0xff) / 255.0f;
}

static void stygian_draw_graph_wire(StygianContext *ctx, float ax, float ay,
                                    float bx, float by, uint32_t rgba) {
  const float mid_x = ax + (bx - ax) * 0.5f;
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 0.0f;
  stygian_color_from_rgba(rgba, &r, &g, &b, &a);
  stygian_line(ctx, ax, ay, mid_x, ay, 2.0f, r, g, b, a);
  stygian_line(ctx, mid_x, ay, mid_x, by, 2.0f, r, g, b, a);
  stygian_line(ctx, mid_x, by, bx, by, 2.0f, r, g, b, a);
}

static void stygian_render_graph_wires(StygianContext *ctx,
                                       const BenchConfig *cfg,
                                       uint64_t frame_index, bool hot_only) {
  for (int i = 0; i < cfg->count; ++i) {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    bench_scene_rect(cfg, i, &x, &y, &w, &h);
    const float ax = x + w;
    const float ay = y + 40.0f;

    if (bench_graph_has_right_link(cfg, i)) {
      const bool hot = bench_item_is_hot(cfg, frame_index, i) ||
                       bench_item_is_hot(cfg, frame_index, i + 1);
      if (!hot_only || hot) {
        float tx = 0.0f;
        float ty = 0.0f;
        float tw = 0.0f;
        float th = 0.0f;
        bench_scene_rect(cfg, i + 1, &tx, &ty, &tw, &th);
        stygian_draw_graph_wire(ctx, ax, ay, tx, ty + 40.0f,
                                hot ? bench_hot_wire_color()
                                    : bench_wire_color());
      }
    }
    if (bench_graph_has_down_link(cfg, i)) {
      const bool hot = bench_item_is_hot(cfg, frame_index, i) ||
                       bench_item_is_hot(cfg, frame_index, i + cfg->columns);
      if (!hot_only || hot) {
        float tx = 0.0f;
        float ty = 0.0f;
        float tw = 0.0f;
        float th = 0.0f;
        bench_scene_rect(cfg, i + cfg->columns, &tx, &ty, &tw, &th);
        stygian_draw_graph_wire(ctx, ax, ay, tx, ty + 40.0f,
                                hot ? bench_hot_wire_color()
                                    : bench_wire_color());
      }
    }
    if (bench_graph_has_diag_link(cfg, i)) {
      const bool hot = bench_item_is_hot(cfg, frame_index, i) ||
                       bench_item_is_hot(cfg, frame_index,
                                         i + cfg->columns + 1);
      if (!hot_only || hot) {
        float tx = 0.0f;
        float ty = 0.0f;
        float tw = 0.0f;
        float th = 0.0f;
        bench_scene_rect(cfg, i + cfg->columns + 1, &tx, &ty, &tw, &th);
        stygian_draw_graph_wire(ctx, ax, ay, tx, ty + 40.0f,
                                hot ? bench_hot_wire_color()
                                    : bench_wire_color());
      }
    }
  }
}

static void stygian_render_inspector_row(StygianContext *ctx,
                                         const BenchConfig *cfg,
                                         const BenchScene *scene,
                                         StygianFont font,
                                         uint64_t frame_index,
                                         int i) {
  char value_text[BENCH_LABEL_CAP];
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 0.0f;
  const uint32_t fill = bench_fill_color(cfg, frame_index, i);
  const uint32_t value_fill = bench_inspector_value_fill_color(cfg, frame_index, i);
  const uint32_t text = bench_item_is_hot(cfg, frame_index, i)
                            ? bench_hot_text_color()
                            : bench_text_color();
  bench_scene_rect(cfg, i, &x, &y, &w, &h);
  (void)bench_inspector_value_text(value_text, sizeof(value_text), cfg, frame_index, i);

  stygian_color_from_rgba(fill, &r, &g, &b, &a);
  stygian_rect_rounded(ctx, x, y, w, h, r, g, b, a, 5.0f);

  stygian_color_from_rgba(value_fill, &r, &g, &b, &a);
  stygian_rect_rounded(ctx, x + w - 126.0f, y + 3.0f, 112.0f, 18.0f,
                       r, g, b, a, 4.0f);

  if (font) {
    stygian_text_span(
        ctx, font, bench_label_at(scene, i), (size_t)bench_label_len_at(scene, i),
        x + 10.0f, y + 5.0f, 14.0f,
        (float)((text >> 24) & 0xff) / 255.0f,
        (float)((text >> 16) & 0xff) / 255.0f,
        (float)((text >> 8) & 0xff) / 255.0f, 1.0f);
    stygian_text_span(
        ctx, font, value_text, strlen(value_text),
        x + w - 118.0f, y + 5.0f, 14.0f,
        (float)((text >> 24) & 0xff) / 255.0f,
        (float)((text >> 16) & 0xff) / 255.0f,
        (float)((text >> 8) & 0xff) / 255.0f, 1.0f);
  }
}

static void stygian_render_hierarchy_row(StygianContext *ctx,
                                         const BenchConfig *cfg,
                                         const BenchScene *scene,
                                         StygianFont font,
                                         uint64_t frame_index,
                                         int i) {
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 0.0f;
  const int depth = bench_hierarchy_depth(i);
  const bool has_children = bench_hierarchy_has_children(i) != 0;
  const float indent = 12.0f + (float)depth * 14.0f;
  const uint32_t row_fill =
      bench_hierarchy_row_fill_color(cfg, frame_index, i);
  const uint32_t marker_fill =
      bench_hierarchy_marker_fill_color(cfg, frame_index, i);
  const uint32_t kind_fill =
      bench_hierarchy_kind_fill_color(cfg, frame_index, i);
  const uint32_t text = bench_item_is_hot(cfg, frame_index, i)
                            ? bench_hot_text_color()
                            : bench_text_color();
  const char *kind = bench_hierarchy_kind_name(i);
  const size_t kind_len = bench_hierarchy_kind_name_len(i);
  bench_scene_rect(cfg, i, &x, &y, &w, &h);

  stygian_color_from_rgba(row_fill, &r, &g, &b, &a);
  stygian_rect_rounded(ctx, x, y, w, h, r, g, b, a, 4.0f);

  stygian_color_from_rgba(marker_fill, &r, &g, &b, &a);
  stygian_rect_rounded(ctx, x + indent, y + 5.0f, 10.0f, 10.0f, r, g, b, a,
                       has_children ? 2.0f : 5.0f);

  stygian_color_from_rgba(kind_fill, &r, &g, &b, &a);
  stygian_rect_rounded(ctx, x + w - 58.0f, y + 2.0f, 46.0f, 16.0f, r, g, b, a,
                       4.0f);

  if (font) {
    stygian_text_span(
        ctx, font, bench_label_at(scene, i), (size_t)bench_label_len_at(scene, i),
        x + indent + 18.0f, y + 3.0f, 14.0f,
        (float)((text >> 24) & 0xff) / 255.0f,
        (float)((text >> 16) & 0xff) / 255.0f,
        (float)((text >> 8) & 0xff) / 255.0f, 1.0f);
    stygian_text_span(
        ctx, font, kind, kind_len, x + w - 50.0f, y + 3.0f, 12.0f,
        (float)((text >> 24) & 0xff) / 255.0f,
        (float)((text >> 16) & 0xff) / 255.0f,
        (float)((text >> 8) & 0xff) / 255.0f, 1.0f);
  }
}

static void stygian_render_static_scope(StygianContext *ctx,
                                        const BenchConfig *cfg,
                                        const BenchScene *scene,
                                        StygianFont font) {
  // Let scope replay do the real work here. That's the whole flex.
  stygian_scope_begin(ctx, STYGIAN_SCOPE_STATIC);
  if (cfg->scene_kind == BENCH_SCENE_GRAPH) {
    stygian_render_graph_wires(ctx, cfg, 0u, false);
  }
  for (int i = 0; i < cfg->count; ++i) {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    bench_scene_rect(cfg, i, &x, &y, &w, &h);
    if (cfg->scene_kind == BENCH_SCENE_TEXT_WALL) {
      if (font) {
        const uint32_t text = bench_text_color();
        stygian_text_span(
            ctx, font, bench_label_at(scene, i),
            (size_t)bench_label_len_at(scene, i), x, y, 14.0f,
            (float)((text >> 24) & 0xff) / 255.0f,
            (float)((text >> 16) & 0xff) / 255.0f,
            (float)((text >> 8) & 0xff) / 255.0f, 1.0f);
      }
      continue;
    }

    if (cfg->scene_kind == BENCH_SCENE_INSPECTOR) {
      stygian_render_inspector_row(ctx, cfg, scene, font, 0u, i);
      continue;
    }

    if (cfg->scene_kind == BENCH_SCENE_HIERARCHY) {
      stygian_render_hierarchy_row(ctx, cfg, scene, font, 0u, i);
      continue;
    }

    if (cfg->scene_kind == BENCH_SCENE_GRAPH) {
      const uint32_t fill = bench_fill_color(cfg, 0u, i);
      const uint32_t header = bench_rgba_u32(49, 55, 70, 255);
      float r = 0.0f;
      float g = 0.0f;
      float b = 0.0f;
      float a = 0.0f;
      stygian_color_from_rgba(fill, &r, &g, &b, &a);
      stygian_rect_rounded(ctx, x, y, w, h, r, g, b, a, 8.0f);
      stygian_color_from_rgba(header, &r, &g, &b, &a);
      stygian_rect_rounded(ctx, x, y, w, 24.0f, r, g, b, a, 8.0f);
      if (font) {
        const uint32_t text = bench_text_color();
        stygian_text_span(
            ctx, font, bench_label_at(scene, i),
            (size_t)bench_label_len_at(scene, i), x + 10.0f, y + 4.0f, 14.0f,
            (float)((text >> 24) & 0xff) / 255.0f,
            (float)((text >> 16) & 0xff) / 255.0f,
            (float)((text >> 8) & 0xff) / 255.0f, 1.0f);
      }
      continue;
    }

    const uint32_t fill = bench_fill_color(cfg, 0u, i);
    const float r = (float)((fill >> 24) & 0xff) / 255.0f;
    const float g = (float)((fill >> 16) & 0xff) / 255.0f;
    const float b = (float)((fill >> 8) & 0xff) / 255.0f;

    stygian_rect_rounded(ctx, x, y, w, h, r, g, b, 1.0f, 6.0f);
    if (font) {
      const uint32_t text = bench_text_color();
      stygian_text_span(
          ctx, font, bench_label_at(scene, i), (size_t)bench_label_len_at(scene, i),
          x + 10.0f, y + 9.0f, 14.0f, (float)((text >> 24) & 0xff) / 255.0f,
          (float)((text >> 16) & 0xff) / 255.0f,
          (float)((text >> 8) & 0xff) / 255.0f, 1.0f);
    }
  }
  stygian_scope_end(ctx);
}

static void stygian_render_dynamic_scope(StygianContext *ctx,
                                         const BenchConfig *cfg,
                                         const BenchScene *scene,
                                         StygianFont font,
                                         uint64_t frame_index) {
  stygian_scope_begin(ctx, STYGIAN_SCOPE_DYNAMIC);
  if (cfg->scenario == BENCH_SCENARIO_SPARSE ||
      cfg->scenario == BENCH_SCENARIO_FULLHOT) {
    if (cfg->scene_kind == BENCH_SCENE_GRAPH) {
      stygian_render_graph_wires(ctx, cfg, frame_index, true);
    }
    for (int i = 0; i < cfg->count; ++i) {
      if (!bench_item_is_hot(cfg, frame_index, i))
        continue;
      float x = 0.0f;
      float y = 0.0f;
      float w = 0.0f;
      float h = 0.0f;
      bench_scene_rect(cfg, i, &x, &y, &w, &h);
      const uint32_t fill = bench_fill_color(cfg, frame_index, i);
      const float r = (float)((fill >> 24) & 0xff) / 255.0f;
      const float g = (float)((fill >> 16) & 0xff) / 255.0f;
      const float b = (float)((fill >> 8) & 0xff) / 255.0f;

      if (cfg->scene_kind == BENCH_SCENE_TEXT_WALL) {
        if (font) {
          const uint32_t text = bench_hot_text_color();
          stygian_text_span(
              ctx, font, bench_label_at(scene, i),
              (size_t)bench_label_len_at(scene, i), x, y, 14.0f,
              (float)((text >> 24) & 0xff) / 255.0f,
              (float)((text >> 16) & 0xff) / 255.0f,
              (float)((text >> 8) & 0xff) / 255.0f, 1.0f);
        }
      } else if (cfg->scene_kind == BENCH_SCENE_INSPECTOR) {
        stygian_render_inspector_row(ctx, cfg, scene, font, frame_index, i);
      } else if (cfg->scene_kind == BENCH_SCENE_HIERARCHY) {
        stygian_render_hierarchy_row(ctx, cfg, scene, font, frame_index, i);
      } else if (cfg->scene_kind == BENCH_SCENE_GRAPH) {
        const uint32_t header = bench_hot_wire_color();
        float hr = 0.0f;
        float hg = 0.0f;
        float hb = 0.0f;
        float ha = 0.0f;
        if (cfg->scenario == BENCH_SCENARIO_SPARSE) {
          stygian_color_from_rgba(header, &hr, &hg, &hb, &ha);
          stygian_rect_rounded(ctx, x + 2.0f, y + 2.0f, w - 4.0f, 8.0f, hr, hg,
                               hb, 0.95f, 4.0f);
        } else {
          stygian_rect_rounded(ctx, x, y, w, h, r, g, b, 1.0f, 8.0f);
          stygian_color_from_rgba(header, &hr, &hg, &hb, &ha);
          stygian_rect_rounded(ctx, x, y, w, 24.0f, hr, hg, hb, ha, 8.0f);
          if (font) {
            const uint32_t text = bench_hot_text_color();
            stygian_text_span(
                ctx, font, bench_label_at(scene, i),
                (size_t)bench_label_len_at(scene, i), x + 10.0f, y + 4.0f,
                14.0f, (float)((text >> 24) & 0xff) / 255.0f,
                (float)((text >> 16) & 0xff) / 255.0f,
                (float)((text >> 8) & 0xff) / 255.0f, 1.0f);
          }
        }
      } else if (cfg->scenario == BENCH_SCENARIO_SPARSE) {
        // sparse is just a hot inset. fullhot redraws the whole card.
        stygian_rect_rounded(ctx, x + 3.0f, y + 3.0f, w - 6.0f, h - 6.0f, r, g,
                             b, 0.95f, 5.0f);
      } else {
        stygian_rect_rounded(ctx, x, y, w, h, r, g, b, 1.0f, 6.0f);
      }
    }
  }
  stygian_scope_end(ctx);
}

int main(int argc, char **argv) {
  BenchConfig cfg = bench_parse_args(argc, argv);
  BenchScene scene = {0};
  BenchAccumulator acc;
  StygianWindowConfig win_cfg;
  StygianConfig styg_cfg;
  StygianWindow *window = NULL;
  StygianContext *ctx = NULL;
  StygianFont font = 0;
  uint64_t total_elements = 0;
  uint64_t total_upload_bytes = 0;
  uint64_t total_upload_ranges = 0;
  uint64_t total_replay_hits = 0;
  uint64_t total_replay_misses = 0;
  uint64_t trace_frames = 0;
  const char *trace_env = getenv("STYGIAN_BENCH_TRACE_FRAMES");

  if (!cfg.mode_name || strcmp(cfg.mode_name, "headless-primitive") == 0) {
    cfg.mode_name = "raw-gpu-resident";
  }
  if (trace_env && trace_env[0] != '\0') {
    trace_frames = (uint64_t)_strtoui64(trace_env, NULL, 10);
  }
  const bool eval_only_mode =
      (strcmp(cfg.mode_name, "eval-only-replay") == 0 ||
       strcmp(cfg.mode_name, "eval-only-retained") == 0);
  const bool cpu_authoring_mode =
      (strcmp(cfg.mode_name, "cpu-authoring-full-build") == 0);
  if (eval_only_mode) {
    cfg.mode_name = "eval-only-replay";
  }
  if (cpu_authoring_mode) {
    cfg.mode_name = "cpu-authoring-full-build";
  }
  const bool raw_render_mode = (!eval_only_mode && !cpu_authoring_mode);
  const bool eval_frame_mode = (eval_only_mode || cpu_authoring_mode);
  bench_scene_init(&scene, &cfg);
  bench_accumulator_reset(&acc);

  memset(&win_cfg, 0, sizeof(win_cfg));
  win_cfg.width = cfg.width;
  win_cfg.height = cfg.height;
  win_cfg.title = "Stygian Comparison Harness";
  win_cfg.flags = STYGIAN_WINDOW_RESIZABLE | STYGIAN_WINDOW_OPENGL;
  win_cfg.role = STYGIAN_ROLE_MAIN;
  win_cfg.gl_major = 4;
  win_cfg.gl_minor = 3;

  window = stygian_window_create(&win_cfg);
  if (!window) {
    fprintf(stderr, "stygian window create failed\n");
    bench_scene_free(&scene);
    return 10;
  }

  memset(&styg_cfg, 0, sizeof(styg_cfg));
  styg_cfg.backend = STYGIAN_BACKEND_OPENGL;
  // Richer editor scenes can quietly starve the hot overlay path if this cap is
  // too small, and then the benchmark lies with a straight face.
  styg_cfg.max_elements = bench_stygian_element_budget(&cfg, &scene);
  styg_cfg.max_textures = 0u;
  styg_cfg.glyph_feature_flags = 0u;
  styg_cfg.window = window;

  ctx = stygian_create(&styg_cfg);
  if (!ctx) {
    fprintf(stderr, "stygian context create failed\n");
    stygian_window_destroy(window);
    bench_scene_free(&scene);
    return 11;
  }

  stygian_set_vsync(ctx, false);
  stygian_set_present_enabled(ctx, false);
  stygian_set_gpu_timing_enabled(ctx, false);
  stygian_set_output_icc_auto(ctx, false);
  font = stygian_font_load(ctx, "assets/atlas.png", "assets/atlas.json");

  const double start = bench_now_seconds();
  const double warmup_end = start + cfg.warmup_seconds;
  const double end = warmup_end + cfg.seconds;
  uint64_t frame_index = 0;

  while (!stygian_window_should_close(window)) {
    const double loop_now = bench_now_seconds();
    if (loop_now >= end) {
      break;
    }

    StygianEvent event;
    while (stygian_window_poll_event(window, &event)) {
      if (event.type == STYGIAN_EVENT_CLOSE) {
        stygian_window_request_close(window);
      }
    }

    int width = 0;
    int height = 0;
    stygian_window_get_size(window, &width, &height);

    BenchFrameMetrics frame = {0};
    const double frame_start = bench_now_seconds();
    stygian_begin_frame_intent(
        ctx, width, height,
        eval_frame_mode ? STYGIAN_FRAME_EVAL_ONLY : STYGIAN_FRAME_RENDER);
    stygian_render_static_scope(ctx, &cfg, &scene, font);
    stygian_render_dynamic_scope(ctx, &cfg, &scene, font, frame_index);
    if (cpu_authoring_mode) {
      // This is the fair CPU-churn lane. Rebuild both scopes every frame and
      // keep the backend asleep so we aren't pretending replay happened.
      stygian_scope_invalidate_next(ctx, STYGIAN_SCOPE_STATIC);
      stygian_scope_invalidate_next(ctx, STYGIAN_SCOPE_DYNAMIC);
    }
    if (cfg.scenario == BENCH_SCENARIO_SPARSE ||
        cfg.scenario == BENCH_SCENARIO_FULLHOT) {
      stygian_scope_invalidate_next(ctx, STYGIAN_SCOPE_DYNAMIC);
      stygian_request_repaint_after_ms(ctx, 0u);
    }
    stygian_end_frame(ctx);
    const double frame_end = bench_now_seconds();

    frame.frame_ms = (frame_end - frame_start) * 1000.0;
    frame.command_count = (uint64_t)stygian_get_last_frame_draw_calls(ctx);
    frame.vertex_count = 0;
    frame.index_count = 0;
    frame.text_bytes = scene.total_text_bytes;

    if (frame_end >= warmup_end) {
      bench_accumulator_add(&acc, &frame);
      acc.total_wall_seconds += (frame_end - frame_start);
      total_elements += (uint64_t)stygian_get_last_frame_element_count(ctx);
      total_replay_hits +=
          (uint64_t)stygian_get_last_frame_scope_replay_hits(ctx);
      total_replay_misses +=
          (uint64_t)stygian_get_last_frame_scope_replay_misses(ctx);
      if (raw_render_mode) {
        total_upload_bytes += (uint64_t)stygian_get_last_frame_upload_bytes(ctx);
        total_upload_ranges +=
            (uint64_t)stygian_get_last_frame_upload_ranges(ctx);
      }
      if (trace_frames > 0u && acc.frames <= trace_frames) {
        printf("STYGIANTRACE frame=%" PRIu64
               " mode=%s scene=%s scenario=%s elements=%u free=%u capacity=%u"
               " draws=%u uploads=%u ranges=%u replay_hit=%u replay_miss=%u"
               " build_ms=%.4f submit_ms=%.4f present_ms=%.4f\n",
               acc.frames, cfg.mode_name, cfg.scene_name, cfg.scenario_name,
               stygian_get_last_frame_element_count(ctx),
               stygian_get_free_element_count(ctx),
               stygian_get_element_capacity(ctx),
               stygian_get_last_frame_draw_calls(ctx),
               stygian_get_last_frame_upload_bytes(ctx),
               stygian_get_last_frame_upload_ranges(ctx),
               stygian_get_last_frame_scope_replay_hits(ctx),
               stygian_get_last_frame_scope_replay_misses(ctx),
               stygian_get_last_frame_build_ms(ctx),
               stygian_get_last_frame_submit_ms(ctx),
               stygian_get_last_frame_present_ms(ctx));
      }
    }
    frame_index += 1;
  }

  bench_summary_print("stygian", &cfg, &acc);
  bench_summary_append_csv("stygian", &cfg, &acc);
  if (acc.frames > 0) {
    printf("STYGIANDETAIL mode=%s scene=%s scenario=%s avg_elements=%" PRIu64
           " avg_upload_bytes=%" PRIu64
           " avg_upload_ranges=%" PRIu64
           " avg_replay_hits=%.2f"
           " avg_replay_misses=%.2f"
           " total_replay_hits=%" PRIu64
           " total_replay_misses=%" PRIu64 "\n",
           cfg.mode_name, cfg.scene_name, cfg.scenario_name, total_elements / acc.frames,
           total_upload_bytes / acc.frames, total_upload_ranges / acc.frames,
           (double)total_replay_hits / (double)acc.frames,
           (double)total_replay_misses / (double)acc.frames,
           total_replay_hits, total_replay_misses);
  }

  if (font) {
    stygian_font_destroy(ctx, font);
  }
  stygian_destroy(ctx);
  stygian_window_destroy(window);
  bench_scene_free(&scene);
  return 0;
}
