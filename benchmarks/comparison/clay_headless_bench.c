#include "bench_common.h"

#define CLAY_IMPLEMENTATION
#include "../../build/_cmp_src/clay/clay.h"
#include <math.h>

typedef struct ClayBenchState {
    void *arena_memory;
} ClayBenchState;

static Clay_Color clay_color_from_rgba(uint32_t rgba) {
    Clay_Color color;
    color.r = (float)((rgba >> 24) & 0xff);
    color.g = (float)((rgba >> 16) & 0xff);
    color.b = (float)((rgba >> 8) & 0xff);
    color.a = (float)(rgba & 0xff);
    return color;
}

static Clay_Dimensions clay_measure_text(Clay_StringSlice text, Clay_TextElementConfig *config, void *user_data) {
    (void)user_data;
    const float font_size = config->fontSize ? (float)config->fontSize : 14.0f;
    const float line_height = config->lineHeight ? (float)config->lineHeight : font_size;
    Clay_Dimensions dims;
    dims.width = (float)text.length * (font_size * 0.56f);
    dims.height = line_height;
    return dims;
}

static void clay_error_handler(Clay_ErrorData error_data) {
    fprintf(stderr, "clay blew up: %.*s\n", (int)error_data.errorText.length, error_data.errorText.chars);
    exit(5);
}

static void clay_render_frame(const BenchConfig *cfg, const BenchScene *scene, uint64_t frame_index, BenchFrameMetrics *out_metrics) {
    float scene_w = 0.0f;
    float scene_h = 0.0f;
    Clay_TextElementConfig text_config = {
        .textColor = clay_color_from_rgba(bench_text_color()),
        .fontId = 0,
        .fontSize = 14,
        .lineHeight = 14,
        .wrapMode = CLAY_TEXT_WRAP_NONE,
        .textAlignment = CLAY_TEXT_ALIGN_LEFT,
    };

    bench_scene_bounds(cfg, &scene_w, &scene_h);
    Clay_SetLayoutDimensions((Clay_Dimensions){
        scene_w > (float)cfg->width ? scene_w : (float)cfg->width,
        scene_h > (float)cfg->height ? scene_h : (float)cfg->height
    });
    Clay_BeginLayout();

    CLAY(CLAY_ID("BenchRoot"), {
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) } }
    }) {
        if (cfg->scene_kind == BENCH_SCENE_GRAPH && 0) {
            // Clay doesn't have a clean headless wire primitive here.
            // Node churn is still a useful graph-ish workload, the wire spam isn't.
            for (int i = 0; i < cfg->count; ++i) {
                float x = 0.0f;
                float y = 0.0f;
                float w = 0.0f;
                float h = 0.0f;
                const float thick = 2.0f;
                bench_scene_rect(cfg, i, &x, &y, &w, &h);
                const float ax = x + w;
                const float ay = y + 40.0f;

                if (bench_graph_has_right_link(cfg, i)) {
                    float tx = 0.0f;
                    float ty = 0.0f;
                    float tw = 0.0f;
                    float th = 0.0f;
                    const bool hot = bench_item_is_hot(cfg, frame_index, i) ||
                                     bench_item_is_hot(cfg, frame_index, i + 1);
                    const Clay_Color wire = clay_color_from_rgba(
                        hot ? bench_hot_wire_color() : bench_wire_color());
                    bench_scene_rect(cfg, i + 1, &tx, &ty, &tw, &th);
                    {
                        const float elbow_x = ax + ((tx - ax) * 0.5f);
                        const float h1_x = ax < elbow_x ? ax : elbow_x;
                        const float h1_w = fabsf(elbow_x - ax);
                        const float h2_x = tx < elbow_x ? tx : elbow_x;
                        const float h2_w = fabsf(tx - elbow_x);
                        const float v_y = ay < (ty + 40.0f) ? ay : (ty + 40.0f);
                        const float v_h = fabsf((ty + 40.0f) - ay);
                        CLAY(CLAY_IDI("GraphR1", i), {
                            .layout = { .sizing = { CLAY_SIZING_FIXED(h1_w), CLAY_SIZING_FIXED(thick) } },
                            .backgroundColor = wire,
                            .cornerRadius = CLAY_CORNER_RADIUS(1),
                            .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .offset = { h1_x, ay - thick * 0.5f } }
                        }) {}
                        CLAY(CLAY_IDI("GraphR2", i), {
                            .layout = { .sizing = { CLAY_SIZING_FIXED(thick), CLAY_SIZING_FIXED(v_h) } },
                            .backgroundColor = wire,
                            .cornerRadius = CLAY_CORNER_RADIUS(1),
                            .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .offset = { elbow_x - thick * 0.5f, v_y } }
                        }) {}
                        CLAY(CLAY_IDI("GraphR3", i), {
                            .layout = { .sizing = { CLAY_SIZING_FIXED(h2_w), CLAY_SIZING_FIXED(thick) } },
                            .backgroundColor = wire,
                            .cornerRadius = CLAY_CORNER_RADIUS(1),
                            .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .offset = { h2_x, ty + 40.0f - thick * 0.5f } }
                        }) {}
                    }
                }
                if (bench_graph_has_down_link(cfg, i)) {
                    float tx = 0.0f;
                    float ty = 0.0f;
                    float tw = 0.0f;
                    float th = 0.0f;
                    const bool hot = bench_item_is_hot(cfg, frame_index, i) ||
                                     bench_item_is_hot(cfg, frame_index, i + cfg->columns);
                    const Clay_Color wire = clay_color_from_rgba(
                        hot ? bench_hot_wire_color() : bench_wire_color());
                    bench_scene_rect(cfg, i + cfg->columns, &tx, &ty, &tw, &th);
                    {
                        const float elbow_x = ax + ((tx - ax) * 0.5f);
                        const float h1_x = ax < elbow_x ? ax : elbow_x;
                        const float h1_w = fabsf(elbow_x - ax);
                        const float h2_x = tx < elbow_x ? tx : elbow_x;
                        const float h2_w = fabsf(tx - elbow_x);
                        const float v_y = ay < (ty + 40.0f) ? ay : (ty + 40.0f);
                        const float v_h = fabsf((ty + 40.0f) - ay);
                        CLAY(CLAY_IDI("GraphD1", i), {
                            .layout = { .sizing = { CLAY_SIZING_FIXED(h1_w), CLAY_SIZING_FIXED(thick) } },
                            .backgroundColor = wire,
                            .cornerRadius = CLAY_CORNER_RADIUS(1),
                            .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .offset = { h1_x, ay - thick * 0.5f } }
                        }) {}
                        CLAY(CLAY_IDI("GraphD2", i), {
                            .layout = { .sizing = { CLAY_SIZING_FIXED(thick), CLAY_SIZING_FIXED(v_h) } },
                            .backgroundColor = wire,
                            .cornerRadius = CLAY_CORNER_RADIUS(1),
                            .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .offset = { elbow_x - thick * 0.5f, v_y } }
                        }) {}
                        CLAY(CLAY_IDI("GraphD3", i), {
                            .layout = { .sizing = { CLAY_SIZING_FIXED(h2_w), CLAY_SIZING_FIXED(thick) } },
                            .backgroundColor = wire,
                            .cornerRadius = CLAY_CORNER_RADIUS(1),
                            .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .offset = { h2_x, ty + 40.0f - thick * 0.5f } }
                        }) {}
                    }
                }
                if (bench_graph_has_diag_link(cfg, i)) {
                    float tx = 0.0f;
                    float ty = 0.0f;
                    float tw = 0.0f;
                    float th = 0.0f;
                    const bool hot = bench_item_is_hot(cfg, frame_index, i) ||
                                     bench_item_is_hot(cfg, frame_index, i + cfg->columns + 1);
                    const Clay_Color wire = clay_color_from_rgba(
                        hot ? bench_hot_wire_color() : bench_wire_color());
                    bench_scene_rect(cfg, i + cfg->columns + 1, &tx, &ty, &tw, &th);
                    {
                        const float elbow_x = ax + ((tx - ax) * 0.5f);
                        const float h1_x = ax < elbow_x ? ax : elbow_x;
                        const float h1_w = fabsf(elbow_x - ax);
                        const float h2_x = tx < elbow_x ? tx : elbow_x;
                        const float h2_w = fabsf(tx - elbow_x);
                        const float v_y = ay < (ty + 40.0f) ? ay : (ty + 40.0f);
                        const float v_h = fabsf((ty + 40.0f) - ay);
                        CLAY(CLAY_IDI("GraphG1", i), {
                            .layout = { .sizing = { CLAY_SIZING_FIXED(h1_w), CLAY_SIZING_FIXED(thick) } },
                            .backgroundColor = wire,
                            .cornerRadius = CLAY_CORNER_RADIUS(1),
                            .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .offset = { h1_x, ay - thick * 0.5f } }
                        }) {}
                        CLAY(CLAY_IDI("GraphG2", i), {
                            .layout = { .sizing = { CLAY_SIZING_FIXED(thick), CLAY_SIZING_FIXED(v_h) } },
                            .backgroundColor = wire,
                            .cornerRadius = CLAY_CORNER_RADIUS(1),
                            .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .offset = { elbow_x - thick * 0.5f, v_y } }
                        }) {}
                        CLAY(CLAY_IDI("GraphG3", i), {
                            .layout = { .sizing = { CLAY_SIZING_FIXED(h2_w), CLAY_SIZING_FIXED(thick) } },
                            .backgroundColor = wire,
                            .cornerRadius = CLAY_CORNER_RADIUS(1),
                            .floating = { .attachTo = CLAY_ATTACH_TO_ROOT, .offset = { h2_x, ty + 40.0f - thick * 0.5f } }
                        }) {}
                    }
                }
            }
        }

        for (int i = 0; i < cfg->count; ++i) {
            float x = 0.0f;
            float y = 0.0f;
            float w = 0.0f;
            float h = 0.0f;
            char value_text[BENCH_LABEL_CAP];
            bench_scene_rect(cfg, i, &x, &y, &w, &h);

            const char *label_chars = bench_label_at(scene, i);
            const Clay_String label = { .length = (int32_t)bench_label_len_at(scene, i), .chars = label_chars };
            const Clay_Color text_color = clay_color_from_rgba(
                bench_item_is_hot(cfg, frame_index, i) ? bench_hot_text_color() : bench_text_color());

            text_config.textColor = text_color;

            if (cfg->scene_kind == BENCH_SCENE_TEXT_WALL) {
                // keep the text case honest: no background cards doing free extra work
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(w), CLAY_SIZING_FIXED(h) }
                    },
                    .floating = {
                        .attachTo = CLAY_ATTACH_TO_ROOT,
                        .offset = { x, y }
                    }
                }) {
                    CLAY_TEXT(label, &text_config);
                }
            } else if (cfg->scene_kind == BENCH_SCENE_INSPECTOR) {
                const int value_len = bench_inspector_value_text(
                    value_text, sizeof(value_text), cfg, frame_index, i);
                const Clay_String value = {
                    .length = (int32_t)((value_len > 0) ? value_len : 0),
                    .chars = value_text
                };
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(w), CLAY_SIZING_FIXED(h) },
                        .padding = { 10, 10, 5, 5 },
                        .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
                    },
                    .backgroundColor = clay_color_from_rgba(bench_fill_color(cfg, frame_index, i)),
                    .cornerRadius = CLAY_CORNER_RADIUS(5),
                    .floating = {
                        .attachTo = CLAY_ATTACH_TO_ROOT,
                        .offset = { x, y }
                    }
                }) {
                    CLAY_TEXT(label, &text_config);
                }
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(112), CLAY_SIZING_FIXED(18) },
                        .padding = { 8, 8, 2, 2 },
                        .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
                    },
                    .backgroundColor = clay_color_from_rgba(
                        bench_inspector_value_fill_color(cfg, frame_index, i)),
                    .cornerRadius = CLAY_CORNER_RADIUS(4),
                    .floating = {
                        .attachTo = CLAY_ATTACH_TO_ROOT,
                        .offset = { x + w - 126.0f, y + 3.0f }
                    }
                }) {
                    CLAY_TEXT(value, &text_config);
                }
            } else if (cfg->scene_kind == BENCH_SCENE_HIERARCHY) {
                const int depth = bench_hierarchy_depth(i);
                const bool has_children = bench_hierarchy_has_children(i) != 0;
                const float indent = 12.0f + (float)depth * 14.0f;
                const Clay_String kind = {
                    .length = (int32_t)bench_hierarchy_kind_name_len(i),
                    .chars = bench_hierarchy_kind_name(i)
                };
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(w), CLAY_SIZING_FIXED(h) }
                    },
                    .backgroundColor = clay_color_from_rgba(
                        bench_hierarchy_row_fill_color(cfg, frame_index, i)),
                    .cornerRadius = CLAY_CORNER_RADIUS(4),
                    .floating = {
                        .attachTo = CLAY_ATTACH_TO_ROOT,
                        .offset = { x, y }
                    }
                }) {}
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(10), CLAY_SIZING_FIXED(10) }
                    },
                    .backgroundColor = clay_color_from_rgba(
                        bench_hierarchy_marker_fill_color(cfg, frame_index, i)),
                    .cornerRadius = CLAY_CORNER_RADIUS(has_children ? 2 : 5),
                    .floating = {
                        .attachTo = CLAY_ATTACH_TO_ROOT,
                        .offset = { x + indent, y + 5.0f }
                    }
                }) {}
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(46), CLAY_SIZING_FIXED(16) },
                        .padding = { 6, 6, 2, 2 },
                        .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
                    },
                    .backgroundColor = clay_color_from_rgba(
                        bench_hierarchy_kind_fill_color(cfg, frame_index, i)),
                    .cornerRadius = CLAY_CORNER_RADIUS(4),
                    .floating = {
                        .attachTo = CLAY_ATTACH_TO_ROOT,
                        .offset = { x + w - 58.0f, y + 2.0f }
                    }
                }) {
                    CLAY_TEXT(kind, &text_config);
                }
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(w - indent - 84.0f), CLAY_SIZING_FIXED(14) }
                    },
                    .floating = {
                        .attachTo = CLAY_ATTACH_TO_ROOT,
                        .offset = { x + indent + 18.0f, y + 3.0f }
                    }
                }) {
                    CLAY_TEXT(label, &text_config);
                }
            } else if (cfg->scene_kind == BENCH_SCENE_GRAPH) {
                CLAY(CLAY_IDI("GraphNode", i), {
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(w), CLAY_SIZING_FIXED(h) },
                        .padding = { 10, 10, 7, 7 },
                        .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
                    },
                    .backgroundColor = clay_color_from_rgba(bench_fill_color(cfg, frame_index, i)),
                    .cornerRadius = CLAY_CORNER_RADIUS(8),
                    .floating = {
                        .attachTo = CLAY_ATTACH_TO_ROOT,
                        .offset = { x, y }
                    }
                }) {
                    CLAY_TEXT(label, &text_config);
                }
            } else {
                // absolute positioning keeps this about scene churn, not flex drama
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(w), CLAY_SIZING_FIXED(h) },
                        .padding = { 10, 10, 9, 9 },
                        .childAlignment = { .y = CLAY_ALIGN_Y_CENTER }
                    },
                    .backgroundColor = clay_color_from_rgba(bench_fill_color(cfg, frame_index, i)),
                    .cornerRadius = CLAY_CORNER_RADIUS(6),
                    .floating = {
                        .attachTo = CLAY_ATTACH_TO_ROOT,
                        .offset = { x, y }
                    }
                }) {
                    CLAY_TEXT(label, &text_config);
                }
            }
        }
    }

    Clay_RenderCommandArray commands = Clay_EndLayout();
    out_metrics->command_count = (uint64_t)commands.length;
    out_metrics->vertex_count = 0;
    out_metrics->index_count = 0;
    out_metrics->text_bytes = scene->total_text_bytes;
}

int main(int argc, char **argv) {
    const BenchConfig cfg = bench_parse_args(argc, argv);
    BenchScene scene = {0};
    BenchAccumulator acc;
    ClayBenchState state = {0};

    bench_scene_init(&scene, &cfg);
    bench_accumulator_reset(&acc);

    Clay_SetMaxElementCount(cfg.count * 16 + 4096);
    Clay_SetMaxMeasureTextCacheWordCount(cfg.count * 8 + 1024);
    const uint32_t arena_bytes = Clay_MinMemorySize();
    state.arena_memory = malloc(arena_bytes);
    if (!state.arena_memory) {
        fprintf(stderr, "out of memory for clay arena\n");
        return 6;
    }

    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(arena_bytes, state.arena_memory);
    Clay_Initialize(arena, (Clay_Dimensions){ (float)cfg.width, (float)cfg.height }, (Clay_ErrorHandler){ clay_error_handler });
    Clay_SetMeasureTextFunction(clay_measure_text, NULL);

    const double start = bench_now_seconds();
    const double warmup_end = start + cfg.warmup_seconds;
    const double end = warmup_end + cfg.seconds;

    uint64_t frame_index = 0;
    while (true) {
        const double loop_now = bench_now_seconds();
        if (loop_now >= end) {
            break;
        }

        BenchFrameMetrics frame = {0};
        const double frame_start = bench_now_seconds();
        clay_render_frame(&cfg, &scene, frame_index, &frame);
        const double frame_end = bench_now_seconds();
        frame.frame_ms = (frame_end - frame_start) * 1000.0;

        if (frame_end >= warmup_end) {
            bench_accumulator_add(&acc, &frame);
            acc.total_wall_seconds += (frame_end - frame_start);
        }
        frame_index += 1;
    }

    bench_summary_print("clay", &cfg, &acc);
    bench_summary_append_csv("clay", &cfg, &acc);

    bench_scene_free(&scene);
    free(state.arena_memory);
    return 0;
}
