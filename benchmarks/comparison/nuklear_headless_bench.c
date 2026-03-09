#include "bench_common.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_IO
#define NK_IMPLEMENTATION
#include "../../build/_cmp_src/nuklear/nuklear.h"
#include <math.h>

typedef struct BenchCanvas {
    struct nk_command_buffer *painter;
    struct nk_vec2 saved_padding;
    struct nk_vec2 saved_spacing;
    struct nk_style_item saved_background;
} BenchCanvas;

static float nuklear_text_width(nk_handle handle, float height, const char *text, int len) {
    (void)handle;
    return (float)len * (height * 0.56f);
}

static struct nk_color nuklear_color_from_rgba(uint32_t rgba) {
    return nk_rgba(
        (nk_byte)((rgba >> 24) & 0xff),
        (nk_byte)((rgba >> 16) & 0xff),
        (nk_byte)((rgba >> 8) & 0xff),
        (nk_byte)(rgba & 0xff));
}

static void nuklear_fill_wire_segment(struct nk_command_buffer *painter, float x,
                                      float y, float w, float h,
                                      struct nk_color color) {
    nk_fill_rect(painter, nk_rect(x, y, w, h), 1.5f, color);
}

static void nuklear_draw_graph_wire(struct nk_command_buffer *painter, float ax,
                                    float ay, float bx, float by,
                                    struct nk_color color) {
    const float thick = 2.0f;
    const float mid_x = ax + (bx - ax) * 0.5f;
    const float h1_x = ax < mid_x ? ax : mid_x;
    const float h1_w = fabsf(mid_x - ax);
    const float h2_x = bx < mid_x ? bx : mid_x;
    const float h2_w = fabsf(bx - mid_x);
    const float v_y = ay < by ? ay : by;
    const float v_h = fabsf(by - ay);

    nuklear_fill_wire_segment(painter, h1_x, ay - thick * 0.5f, h1_w, thick, color);
    nuklear_fill_wire_segment(painter, mid_x - thick * 0.5f, v_y, thick, v_h, color);
    nuklear_fill_wire_segment(painter, h2_x, by - thick * 0.5f, h2_w, thick, color);
}

static nk_bool bench_canvas_begin(struct nk_context *ctx, BenchCanvas *canvas, const BenchConfig *cfg) {
    canvas->saved_padding = ctx->style.window.padding;
    canvas->saved_spacing = ctx->style.window.spacing;
    canvas->saved_background = ctx->style.window.fixed_background;

    ctx->style.window.padding = nk_vec2(0.0f, 0.0f);
    ctx->style.window.spacing = nk_vec2(0.0f, 0.0f);
    ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));

    if (!nk_begin(ctx, "BenchCanvas", nk_rect(0, 0, (float)cfg->width, (float)cfg->height), NK_WINDOW_NO_SCROLLBAR)) {
        return nk_false;
    }

    {
        struct nk_rect total_space = nk_window_get_content_region(ctx);
        if (cfg->scene_kind == BENCH_SCENE_GRAPH) {
            float scene_w = 0.0f;
            float scene_h = 0.0f;
            bench_scene_bounds(cfg, &scene_w, &scene_h);
            total_space.w = scene_w;
            total_space.h = scene_h;
        }
        nk_layout_row_dynamic(ctx, total_space.h, 1);
        nk_widget(&total_space, ctx);
        canvas->painter = nk_window_get_canvas(ctx);
    }
    return nk_true;
}

static void bench_canvas_end(struct nk_context *ctx, BenchCanvas *canvas) {
    nk_end(ctx);
    ctx->style.window.padding = canvas->saved_padding;
    ctx->style.window.spacing = canvas->saved_spacing;
    ctx->style.window.fixed_background = canvas->saved_background;
}

static uint64_t nuklear_command_count(struct nk_context *ctx) {
    uint64_t count = 0;
    const struct nk_command *cmd = NULL;
    nk_foreach(cmd, ctx) {
        count += 1;
    }
    return count;
}

static void nuklear_render_frame(struct nk_context *ctx, const struct nk_user_font *font, const BenchConfig *cfg, const BenchScene *scene, uint64_t frame_index, BenchFrameMetrics *out_metrics) {
    BenchCanvas canvas;

    nk_input_begin(ctx);
    nk_input_end(ctx);

    if (bench_canvas_begin(ctx, &canvas, cfg)) {
        const float ox = canvas.painter->clip.x;
        const float oy = canvas.painter->clip.y;
        // nuklear will happily scissor most of the scene away if we let it
        nk_push_scissor(canvas.painter, nk_rect(-100000.0f, -100000.0f, 200000.0f, 200000.0f));
        for (int i = 0; i < cfg->count; ++i) {
            float x = 0.0f;
            float y = 0.0f;
            float w = 0.0f;
            float h = 0.0f;
            bench_scene_rect(cfg, i, &x, &y, &w, &h);

            const char *label = bench_label_at(scene, i);
            const int label_len = (int)bench_label_len_at(scene, i);
            if (cfg->scene_kind == BENCH_SCENE_GRAPH) {
                const float ax = ox + x + w;
                const float ay = oy + y + 40.0f;
                if (bench_graph_has_right_link(cfg, i)) {
                    float tx = 0.0f;
                    float ty = 0.0f;
                    float tw = 0.0f;
                    float th = 0.0f;
                    bench_scene_rect(cfg, i + 1, &tx, &ty, &tw, &th);
                    nuklear_draw_graph_wire(
                        canvas.painter, ax, ay, ox + tx, oy + ty + 40.0f,
                        nuklear_color_from_rgba(
                            (bench_item_is_hot(cfg, frame_index, i) ||
                             bench_item_is_hot(cfg, frame_index, i + 1))
                                ? bench_hot_wire_color()
                                : bench_wire_color()));
                }
                if (bench_graph_has_down_link(cfg, i)) {
                    float tx = 0.0f;
                    float ty = 0.0f;
                    float tw = 0.0f;
                    float th = 0.0f;
                    bench_scene_rect(cfg, i + cfg->columns, &tx, &ty, &tw, &th);
                    nuklear_draw_graph_wire(
                        canvas.painter, ax, ay, ox + tx, oy + ty + 40.0f,
                        nuklear_color_from_rgba(
                            (bench_item_is_hot(cfg, frame_index, i) ||
                             bench_item_is_hot(cfg, frame_index, i + cfg->columns))
                                ? bench_hot_wire_color()
                                : bench_wire_color()));
                }
                if (bench_graph_has_diag_link(cfg, i)) {
                    float tx = 0.0f;
                    float ty = 0.0f;
                    float tw = 0.0f;
                    float th = 0.0f;
                    bench_scene_rect(cfg, i + cfg->columns + 1, &tx, &ty, &tw, &th);
                    nuklear_draw_graph_wire(
                        canvas.painter, ax, ay, ox + tx, oy + ty + 40.0f,
                        nuklear_color_from_rgba(
                            (bench_item_is_hot(cfg, frame_index, i) ||
                             bench_item_is_hot(cfg, frame_index,
                                               i + cfg->columns + 1))
                                ? bench_hot_wire_color()
                                : bench_wire_color()));
                }

                nk_fill_rect(canvas.painter, nk_rect(ox + x, oy + y, w, h), 8.0f,
                             nuklear_color_from_rgba(
                                 bench_fill_color(cfg, frame_index, i)));
                nk_fill_rect(canvas.painter, nk_rect(ox + x, oy + y, w, 24.0f), 8.0f,
                             nuklear_color_from_rgba(
                                 bench_item_is_hot(cfg, frame_index, i)
                                     ? bench_hot_wire_color()
                                     : bench_rgba_u32(49, 55, 70, 255)));
            } else if (cfg->scene_kind == BENCH_SCENE_INSPECTOR) {
                char value_text[BENCH_LABEL_CAP];
                const int value_len = bench_inspector_value_text(
                    value_text, sizeof(value_text), cfg, frame_index, i);
                nk_fill_rect(canvas.painter, nk_rect(ox + x, oy + y, w, h), 5.0f,
                             nuklear_color_from_rgba(
                                 bench_fill_color(cfg, frame_index, i)));
                nk_draw_text(
                    canvas.painter,
                    nk_rect(ox + x + 10.0f, oy + y + 5.0f, 180.0f, h - 8.0f),
                    label,
                    label_len,
                    font,
                    nk_rgba(0, 0, 0, 0),
                    nuklear_color_from_rgba(
                        bench_item_is_hot(cfg, frame_index, i) ? bench_hot_text_color()
                                                               : bench_text_color()));
                nk_fill_rect(canvas.painter,
                             nk_rect(ox + x + w - 126.0f, oy + y + 3.0f, 112.0f, 18.0f),
                             4.0f,
                             nuklear_color_from_rgba(
                                 bench_inspector_value_fill_color(cfg, frame_index, i)));
                nk_draw_text(
                    canvas.painter,
                    nk_rect(ox + x + w - 118.0f, oy + y + 5.0f, 104.0f, 14.0f),
                    value_text,
                    value_len,
                    font,
                    nk_rgba(0, 0, 0, 0),
                    nuklear_color_from_rgba(
                        bench_item_is_hot(cfg, frame_index, i) ? bench_hot_text_color()
                                                               : bench_text_color()));
                continue;
            } else if (cfg->scene_kind == BENCH_SCENE_HIERARCHY) {
                const int depth = bench_hierarchy_depth(i);
                const nk_bool has_children = bench_hierarchy_has_children(i);
                const float indent = 12.0f + (float)depth * 14.0f;
                const char *kind = bench_hierarchy_kind_name(i);
                const int kind_len = (int)bench_hierarchy_kind_name_len(i);

                nk_fill_rect(canvas.painter, nk_rect(ox + x, oy + y, w, h), 4.0f,
                             nuklear_color_from_rgba(
                                 bench_hierarchy_row_fill_color(cfg, frame_index, i)));
                nk_fill_rect(canvas.painter,
                             nk_rect(ox + x + indent, oy + y + 5.0f, 10.0f, 10.0f),
                             has_children ? 2.0f : 5.0f,
                             nuklear_color_from_rgba(
                                 bench_hierarchy_marker_fill_color(cfg, frame_index, i)));
                nk_fill_rect(canvas.painter,
                             nk_rect(ox + x + w - 58.0f, oy + y + 2.0f, 46.0f, 16.0f),
                             4.0f,
                             nuklear_color_from_rgba(
                                 bench_hierarchy_kind_fill_color(cfg, frame_index, i)));
                nk_draw_text(
                    canvas.painter,
                    nk_rect(ox + x + indent + 18.0f, oy + y + 3.0f, w - indent - 84.0f, 14.0f),
                    label,
                    label_len,
                    font,
                    nk_rgba(0, 0, 0, 0),
                    nuklear_color_from_rgba(
                        bench_item_is_hot(cfg, frame_index, i) ? bench_hot_text_color()
                                                               : bench_text_color()));
                nk_draw_text(
                    canvas.painter,
                    nk_rect(ox + x + w - 50.0f, oy + y + 3.0f, 38.0f, 14.0f),
                    kind,
                    kind_len,
                    font,
                    nk_rgba(0, 0, 0, 0),
                    nuklear_color_from_rgba(
                        bench_item_is_hot(cfg, frame_index, i) ? bench_hot_text_color()
                                                               : bench_text_color()));
                continue;
            } else if (cfg->scene_kind == BENCH_SCENE_CARDS) {
                nk_fill_rect(canvas.painter, nk_rect(ox + x, oy + y, w, h), 6.0f,
                             nuklear_color_from_rgba(bench_fill_color(cfg, frame_index, i)));
            }
            nk_draw_text(
                canvas.painter,
                nk_rect(ox + x + (cfg->scene_kind == BENCH_SCENE_TEXT_WALL ? 0.0f : 10.0f),
                        oy + y + (cfg->scene_kind == BENCH_SCENE_TEXT_WALL ? 0.0f : 4.0f),
                        w - (cfg->scene_kind == BENCH_SCENE_TEXT_WALL ? 0.0f : 20.0f),
                        h - (cfg->scene_kind == BENCH_SCENE_TEXT_WALL ? 0.0f : 8.0f)),
                label,
                label_len,
                font,
                nk_rgba(0, 0, 0, 0),
                nuklear_color_from_rgba(
                    bench_item_is_hot(cfg, frame_index, i) ? bench_hot_text_color()
                                                           : bench_text_color()));
        }
        bench_canvas_end(ctx, &canvas);
    }

    out_metrics->command_count = nuklear_command_count(ctx);
    out_metrics->vertex_count = 0;
    out_metrics->index_count = 0;
    out_metrics->text_bytes = scene->total_text_bytes;
    nk_clear(ctx);
}

int main(int argc, char **argv) {
    const BenchConfig cfg = bench_parse_args(argc, argv);
    BenchScene scene = {0};
    BenchAccumulator acc;
    struct nk_context ctx;
    struct nk_user_font font;

    bench_scene_init(&scene, &cfg);
    bench_accumulator_reset(&acc);

    memset(&font, 0, sizeof(font));
    font.height = 14.0f;
    font.width = nuklear_text_width;

    if (!nk_init_default(&ctx, &font)) {
        fprintf(stderr, "nuklear init failed\n");
        bench_scene_free(&scene);
        return 7;
    }

    ctx.style.window.padding = nk_vec2(0.0f, 0.0f);
    ctx.style.window.spacing = nk_vec2(0.0f, 0.0f);

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
        nuklear_render_frame(&ctx, &font, &cfg, &scene, frame_index, &frame);
        const double frame_end = bench_now_seconds();
        frame.frame_ms = (frame_end - frame_start) * 1000.0;

        if (frame_end >= warmup_end) {
            bench_accumulator_add(&acc, &frame);
            acc.total_wall_seconds += (frame_end - frame_start);
        }
        frame_index += 1;
    }

    bench_summary_print("nuklear", &cfg, &acc);
    bench_summary_append_csv("nuklear", &cfg, &acc);

    nk_free(&ctx);
    bench_scene_free(&scene);
    return 0;
}
