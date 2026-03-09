#include "bench_common.h"

#include "../../build/_cmp_src/imgui/imgui.h"
#include <cmath>

static ImU32 imgui_color_from_rgba(uint32_t rgba) {
    const uint8_t r = (uint8_t)((rgba >> 24) & 0xff);
    const uint8_t g = (uint8_t)((rgba >> 16) & 0xff);
    const uint8_t b = (uint8_t)((rgba >> 8) & 0xff);
    const uint8_t a = (uint8_t)(rgba & 0xff);
    return IM_COL32(r, g, b, a);
}

static void imgui_add_wire_segment(ImDrawList *draw_list, float x, float y,
                                   float w, float h, ImU32 color) {
    draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), color, 1.5f);
}

static void imgui_add_graph_wire(ImDrawList *draw_list, float ax, float ay,
                                 float bx, float by, ImU32 color) {
    const float thick = 2.0f;
    const float mid_x = ax + (bx - ax) * 0.5f;
    const float h1_x = ax < mid_x ? ax : mid_x;
    const float h1_w = fabsf(mid_x - ax);
    const float h2_x = bx < mid_x ? bx : mid_x;
    const float h2_w = fabsf(bx - mid_x);
    const float v_y = ay < by ? ay : by;
    const float v_h = fabsf(by - ay);

    imgui_add_wire_segment(draw_list, h1_x, ay - thick * 0.5f, h1_w, thick, color);
    imgui_add_wire_segment(draw_list, mid_x - thick * 0.5f, v_y, thick, v_h, color);
    imgui_add_wire_segment(draw_list, h2_x, by - thick * 0.5f, h2_w, thick, color);
}

static void imgui_render_cards(const BenchConfig *cfg, const BenchScene *scene,
                               ImDrawList *draw_list, ImFont *font,
                               float font_size, uint64_t frame_index) {
    for (int i = 0; i < cfg->count; ++i) {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        bench_scene_rect(cfg, i, &x, &y, &w, &h);

        const uint32_t fill_rgba = bench_fill_color(cfg, frame_index, i);
        draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h),
                                 imgui_color_from_rgba(fill_rgba), 6.0f);
        draw_list->AddText(font, font_size, ImVec2(x + 10.0f, y + 9.0f),
                           imgui_color_from_rgba(bench_text_color()),
                           bench_label_at(scene, i));
    }
}

static void imgui_render_graph(const BenchConfig *cfg, const BenchScene *scene,
                               ImDrawList *draw_list, ImFont *font,
                               float font_size, uint64_t frame_index) {
    for (int i = 0; i < cfg->count; ++i) {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        bench_scene_rect(cfg, i, &x, &y, &w, &h);
        const float ax = x + w;
        const float ay = y + 40.0f;

        if (bench_graph_has_right_link(cfg, i)) {
            float tx = 0.0f;
            float ty = 0.0f;
            float tw = 0.0f;
            float th = 0.0f;
            bench_scene_rect(cfg, i + 1, &tx, &ty, &tw, &th);
            const bool hot = bench_item_is_hot(cfg, frame_index, i) ||
                             bench_item_is_hot(cfg, frame_index, i + 1);
            imgui_add_graph_wire(draw_list, ax, ay, tx, ty + 40.0f,
                                 imgui_color_from_rgba(hot ? bench_hot_wire_color()
                                                           : bench_wire_color()));
        }
        if (bench_graph_has_down_link(cfg, i)) {
            float tx = 0.0f;
            float ty = 0.0f;
            float tw = 0.0f;
            float th = 0.0f;
            bench_scene_rect(cfg, i + cfg->columns, &tx, &ty, &tw, &th);
            const bool hot = bench_item_is_hot(cfg, frame_index, i) ||
                             bench_item_is_hot(cfg, frame_index, i + cfg->columns);
            imgui_add_graph_wire(draw_list, ax, ay, tx, ty + 40.0f,
                                 imgui_color_from_rgba(hot ? bench_hot_wire_color()
                                                           : bench_wire_color()));
        }
        if (bench_graph_has_diag_link(cfg, i)) {
            float tx = 0.0f;
            float ty = 0.0f;
            float tw = 0.0f;
            float th = 0.0f;
            bench_scene_rect(cfg, i + cfg->columns + 1, &tx, &ty, &tw, &th);
            const bool hot = bench_item_is_hot(cfg, frame_index, i) ||
                             bench_item_is_hot(cfg, frame_index,
                                               i + cfg->columns + 1);
            imgui_add_graph_wire(draw_list, ax, ay, tx, ty + 40.0f,
                                 imgui_color_from_rgba(hot ? bench_hot_wire_color()
                                                           : bench_wire_color()));
        }
    }

    for (int i = 0; i < cfg->count; ++i) {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        const bool hot = bench_item_is_hot(cfg, frame_index, i);
        const uint32_t fill_rgba = bench_fill_color(cfg, frame_index, i);
        bench_scene_rect(cfg, i, &x, &y, &w, &h);
        draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h),
                                 imgui_color_from_rgba(fill_rgba), 8.0f);
        draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + 24.0f),
                                 imgui_color_from_rgba(
                                     hot ? bench_hot_wire_color()
                                         : bench_rgba_u32(49, 55, 70, 255)),
                                 8.0f);
        draw_list->AddText(font, font_size, ImVec2(x + 10.0f, y + 4.0f),
                           imgui_color_from_rgba(hot ? bench_hot_text_color()
                                                     : bench_text_color()),
                           bench_label_at(scene, i));
    }
}

static void imgui_render_text_wall(const BenchConfig *cfg, const BenchScene *scene,
                                   ImDrawList *draw_list, ImFont *font,
                                   float font_size, uint64_t frame_index) {
    for (int i = 0; i < cfg->count; ++i) {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        bench_scene_rect(cfg, i, &x, &y, &w, &h);

        const uint32_t text_rgba = bench_item_is_hot(cfg, frame_index, i)
                                       ? bench_hot_text_color()
                                       : bench_text_color();
        draw_list->AddText(font, font_size, ImVec2(x, y),
                           imgui_color_from_rgba(text_rgba),
                           bench_label_at(scene, i));
    }
}

static void imgui_render_inspector(const BenchConfig *cfg, const BenchScene *scene,
                                   ImDrawList *draw_list, ImFont *font,
                                   float font_size, uint64_t frame_index) {
    for (int i = 0; i < cfg->count; ++i) {
        char value_text[BENCH_LABEL_CAP];
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        bench_scene_rect(cfg, i, &x, &y, &w, &h);
        (void)bench_inspector_value_text(value_text, sizeof(value_text), cfg, frame_index, i);

        draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h),
                                 imgui_color_from_rgba(bench_fill_color(cfg, frame_index, i)), 5.0f);
        draw_list->AddText(font, font_size, ImVec2(x + 10.0f, y + 5.0f),
                           imgui_color_from_rgba(bench_item_is_hot(cfg, frame_index, i)
                                                     ? bench_hot_text_color()
                                                     : bench_text_color()),
                           bench_label_at(scene, i));

        const float pill_x = x + w - 126.0f;
        const float pill_y = y + 3.0f;
        const float pill_w = 112.0f;
        const float pill_h = 18.0f;
        draw_list->AddRectFilled(ImVec2(pill_x, pill_y), ImVec2(pill_x + pill_w, pill_y + pill_h),
                                 imgui_color_from_rgba(bench_inspector_value_fill_color(cfg, frame_index, i)), 4.0f);
        draw_list->AddText(font, font_size, ImVec2(pill_x + 8.0f, pill_y + 2.0f),
                           imgui_color_from_rgba(bench_item_is_hot(cfg, frame_index, i)
                                                     ? bench_hot_text_color()
                                                     : bench_text_color()),
                           value_text);
    }
}

static void imgui_render_hierarchy(const BenchConfig *cfg, const BenchScene *scene,
                                   ImDrawList *draw_list, ImFont *font,
                                   float font_size, uint64_t frame_index) {
    for (int i = 0; i < cfg->count; ++i) {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        const int depth = bench_hierarchy_depth(i);
        const bool has_children = bench_hierarchy_has_children(i) != 0;
        const float indent = 12.0f + (float)depth * 14.0f;
        const char *kind = bench_hierarchy_kind_name(i);
        bench_scene_rect(cfg, i, &x, &y, &w, &h);

        draw_list->AddRectFilled(
            ImVec2(x, y), ImVec2(x + w, y + h),
            imgui_color_from_rgba(bench_hierarchy_row_fill_color(cfg, frame_index, i)),
            4.0f);
        draw_list->AddRectFilled(
            ImVec2(x + indent, y + 5.0f), ImVec2(x + indent + 10.0f, y + 15.0f),
            imgui_color_from_rgba(
                bench_hierarchy_marker_fill_color(cfg, frame_index, i)),
            has_children ? 2.0f : 5.0f);
        draw_list->AddRectFilled(
            ImVec2(x + w - 58.0f, y + 2.0f), ImVec2(x + w - 12.0f, y + 18.0f),
            imgui_color_from_rgba(
                bench_hierarchy_kind_fill_color(cfg, frame_index, i)),
            4.0f);
        draw_list->AddText(
            font, font_size, ImVec2(x + indent + 18.0f, y + 3.0f),
            imgui_color_from_rgba(
                bench_item_is_hot(cfg, frame_index, i) ? bench_hot_text_color()
                                                       : bench_text_color()),
            bench_label_at(scene, i));
        draw_list->AddText(
            font, 12.0f, ImVec2(x + w - 50.0f, y + 3.0f),
            imgui_color_from_rgba(
                bench_item_is_hot(cfg, frame_index, i) ? bench_hot_text_color()
                                                       : bench_text_color()),
            kind);
    }
}

static void imgui_render_frame(const BenchConfig *cfg, const BenchScene *scene, uint64_t frame_index, BenchFrameMetrics *out_metrics) {
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)cfg->width, (float)cfg->height);
    ImDrawList draw_list(ImGui::GetDrawListSharedData());
    float scene_w = 0.0f;
    float scene_h = 0.0f;
    draw_list._ResetForNewFrame();
    draw_list.PushTexture(io.Fonts->TexRef);
    bench_scene_bounds(cfg, &scene_w, &scene_h);
    draw_list.PushClipRect(ImVec2(-1024.0f, -1024.0f),
                           ImVec2(scene_w + 1024.0f, scene_h + 1024.0f), false);

    ImFont *font = io.FontDefault ? io.FontDefault : io.Fonts->Fonts[0];
    const float font_size = 14.0f;

    if (cfg->scene_kind == BENCH_SCENE_TEXT_WALL) {
        imgui_render_text_wall(cfg, scene, &draw_list, font, font_size, frame_index);
    } else if (cfg->scene_kind == BENCH_SCENE_INSPECTOR) {
        imgui_render_inspector(cfg, scene, &draw_list, font, font_size, frame_index);
    } else if (cfg->scene_kind == BENCH_SCENE_HIERARCHY) {
        imgui_render_hierarchy(cfg, scene, &draw_list, font, font_size, frame_index);
    } else if (cfg->scene_kind == BENCH_SCENE_GRAPH) {
        imgui_render_graph(cfg, scene, &draw_list, font, font_size, frame_index);
    } else {
        imgui_render_cards(cfg, scene, &draw_list, font, font_size, frame_index);
    }

    draw_list.PopClipRect();
    draw_list.PopTexture();

    out_metrics->command_count = (uint64_t)draw_list.CmdBuffer.Size;
    out_metrics->vertex_count = (uint64_t)draw_list.VtxBuffer.Size;
    out_metrics->index_count = (uint64_t)draw_list.IdxBuffer.Size;
    out_metrics->text_bytes = scene->total_text_bytes;
}

int main(int argc, char **argv) {
    const BenchConfig cfg = bench_parse_args(argc, argv);
    BenchScene scene = {0};
    bench_scene_init(&scene, &cfg);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.LogFilename = NULL;
    io.DisplaySize = ImVec2((float)cfg.width, (float)cfg.height);
    unsigned char *pixels = NULL;
    int atlas_w = 0;
    int atlas_h = 0;
    int atlas_bpp = 0;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &atlas_w, &atlas_h, &atlas_bpp);
    io.Fonts->SetTexID((ImTextureID)(intptr_t)1);
    (void)pixels;
    (void)atlas_w;
    (void)atlas_h;
    (void)atlas_bpp;

    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(0.0f, 0.0f);
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.AntiAliasedFill = false;
    style.AntiAliasedLines = false;
    style.AntiAliasedLinesUseTex = false;
    io.DeltaTime = 1.0f / 60.0f;
    ImGui::NewFrame();
    ImGui::EndFrame();

    const double start = bench_now_seconds();
    const double warmup_end = start + cfg.warmup_seconds;
    const double end = warmup_end + cfg.seconds;

    BenchAccumulator acc;
    bench_accumulator_reset(&acc);

    uint64_t frame_index = 0;
    while (true) {
        const double loop_now = bench_now_seconds();
        if (loop_now >= end) {
            break;
        }

        BenchFrameMetrics frame = {0};
        const double frame_start = bench_now_seconds();
        imgui_render_frame(&cfg, &scene, frame_index, &frame);
        const double frame_end = bench_now_seconds();
        frame.frame_ms = (frame_end - frame_start) * 1000.0;

        if (frame_end >= warmup_end) {
            bench_accumulator_add(&acc, &frame);
            acc.total_wall_seconds += (frame_end - frame_start);
        }
        frame_index += 1;
    }

    bench_summary_print("imgui", &cfg, &acc);
    bench_summary_append_csv("imgui", &cfg, &acc);

    ImGui::DestroyContext();
    bench_scene_free(&scene);
    return 0;
}
