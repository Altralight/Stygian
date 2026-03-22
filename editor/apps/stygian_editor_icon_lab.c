#include "../../include/stygian.h"
#include "../../include/stygian_clipboard.h"
#include "../include/stygian_editor_icons.h"
#include "../../window/stygian_input.h"
#include "../../window/stygian_window.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef STYGIAN_DEMO_VULKAN
#define STYGIAN_EDITOR_ICON_LAB_BACKEND STYGIAN_BACKEND_VULKAN
#define STYGIAN_EDITOR_ICON_LAB_RENDER_FLAG STYGIAN_WINDOW_VULKAN
#else
#define STYGIAN_EDITOR_ICON_LAB_BACKEND STYGIAN_BACKEND_OPENGL
#define STYGIAN_EDITOR_ICON_LAB_RENDER_FLAG STYGIAN_WINDOW_OPENGL
#endif

typedef struct IconLabFrameState {
  float mouse_x;
  float mouse_y;
  bool left_down;
  bool left_pressed;
  bool left_released;
  bool right_pressed;
  bool event_requested;
} IconLabFrameState;

typedef struct IconLabLayout {
  float x;
  float y;
  float w;
  float h;
  float sidebar_x;
  float sidebar_y;
  float sidebar_w;
  float sidebar_h;
  float stage_x;
  float stage_y;
  float stage_w;
  float stage_h;
  float info_x;
  float info_y;
  float info_w;
  float info_h;
  float copy_x;
  float copy_y;
  float copy_w;
  float copy_h;
  float preview_x;
  float preview_y;
  float preview_w;
  float preview_h;
} IconLabLayout;

static bool icon_lab_path_up_one(char *path) {
  size_t len;
  if (!path || !path[0])
    return false;
  len = strlen(path);
  while (len > 0u &&
         (path[len - 1u] == '\\' || path[len - 1u] == '/')) {
    path[--len] = '\0';
  }
  while (len > 0u && path[len - 1u] != '\\' && path[len - 1u] != '/') {
    path[--len] = '\0';
  }
  while (len > 0u &&
         (path[len - 1u] == '\\' || path[len - 1u] == '/')) {
    path[--len] = '\0';
  }
  return len > 0u;
}

static bool icon_lab_resolve_repo_paths(char *shader_dir, size_t shader_dir_sz,
                                        char *atlas_png, size_t atlas_png_sz,
                                        char *atlas_json,
                                        size_t atlas_json_sz,
                                        char *repo_root,
                                        size_t repo_root_sz) {
  char exe_path[1024];
  size_t len;
  if (!shader_dir || !atlas_png || !atlas_json || !repo_root)
    return false;
  len = GetModuleFileNameA(NULL, exe_path, (DWORD)sizeof(exe_path));
  if (len == 0u || len >= sizeof(exe_path))
    return false;
  exe_path[len] = '\0';
  if (repo_root_sz == 0u)
    return false;
  strncpy(repo_root, exe_path, repo_root_sz - 1u);
  repo_root[repo_root_sz - 1u] = '\0';
  if (!icon_lab_path_up_one(repo_root) || !icon_lab_path_up_one(repo_root) ||
      !icon_lab_path_up_one(repo_root) || !icon_lab_path_up_one(repo_root))
    return false;
  snprintf(shader_dir, shader_dir_sz, "%s\\shaders", repo_root);
  snprintf(atlas_png, atlas_png_sz, "%s\\assets\\atlas.png", repo_root);
  snprintf(atlas_json, atlas_json_sz, "%s\\assets\\atlas.json", repo_root);
  return true;
}

static uint64_t icon_lab_now_ms(void) {
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}

static bool icon_lab_point_in_rect(float px, float py, float x, float y, float w,
                                   float h) {
  return px >= x && py >= y && px < x + w && py < y + h;
}

static void icon_lab_query_mouse(StygianWindow *window, float *out_x,
                                 float *out_y) {
  if (!out_x || !out_y)
    return;
  *out_x = 0.0f;
  *out_y = 0.0f;
  if (!window)
    return;
  {
    int x = 0;
    int y = 0;
    stygian_mouse_pos(window, &x, &y);
    *out_x = (float)x;
    *out_y = (float)y;
  }
}

static void icon_lab_compute_layout(IconLabLayout *layout, int width,
                                    int height) {
  if (!layout)
    return;
  layout->x = 20.0f;
  layout->y = 20.0f;
  layout->w = (float)width - 40.0f;
  layout->h = (float)height - 40.0f;
  layout->sidebar_x = layout->x;
  layout->sidebar_y = layout->y;
  layout->sidebar_w = 230.0f;
  layout->sidebar_h = layout->h;
  layout->info_w = 320.0f;
  layout->info_x = layout->x + layout->w - layout->info_w;
  layout->info_y = layout->y;
  layout->info_h = layout->h;
  layout->stage_x = layout->sidebar_x + layout->sidebar_w + 16.0f;
  layout->stage_y = layout->y;
  layout->stage_w = layout->info_x - 16.0f - layout->stage_x;
  layout->stage_h = layout->h;
  layout->preview_x = layout->stage_x + 18.0f;
  layout->preview_y = layout->stage_y + 74.0f;
  layout->preview_w = layout->stage_w - 36.0f;
  layout->preview_h = layout->stage_h - 148.0f;
  layout->copy_x = layout->info_x + 18.0f;
  layout->copy_y = layout->info_y + 258.0f;
  layout->copy_w = layout->info_w - 36.0f;
  layout->copy_h = 34.0f;
}

static void icon_lab_draw_grid(StygianContext *ctx, float x, float y, float w,
                               float h, float step) {
  int ix;
  int iy;
  int cols = (int)(w / step);
  int rows = (int)(h / step);
  for (ix = 0; ix <= cols; ++ix) {
    float lx = x + ix * step;
    float alpha = (ix % 4 == 0) ? 0.10f : 0.05f;
    stygian_line(ctx, lx, y, lx, y + h, 1.0f, 0.42f, 0.47f, 0.54f, alpha);
  }
  for (iy = 0; iy <= rows; ++iy) {
    float ly = y + iy * step;
    float alpha = (iy % 4 == 0) ? 0.10f : 0.05f;
    stygian_line(ctx, x, ly, x + w, ly, 1.0f, 0.42f, 0.47f, 0.54f, alpha);
  }
}

static void icon_lab_copy_snippet(StygianContext *ctx,
                                  StygianEditorIconKind kind) {
  char text[256];
  snprintf(text, sizeof(text),
           "stygian_editor_draw_icon(ctx, %s, x, y, w, h, 1.0f, 1.0f, 1.0f, "
           "1.0f);",
           stygian_editor_icon_token(kind));
  stygian_clipboard_push(ctx, text, "stygian-editor-icon-call");
}

int main(void) {
  StygianWindowConfig win_cfg = {
      .width = 1380,
      .height = 880,
      .title = "Stygian Editor Icon Lab",
      .flags = STYGIAN_WINDOW_RESIZABLE | STYGIAN_EDITOR_ICON_LAB_RENDER_FLAG,
  };
  StygianWindow *window = NULL;
  StygianContext *ctx = NULL;
  StygianConfig cfg = {0};
  StygianFont font = 0u;
  StygianEditorIconKind selected = STYGIAN_EDITOR_ICON_SELECT;
  uint64_t copied_until_ms = 0u;
  char shader_dir[1024];
  char atlas_png[1024];
  char atlas_json[1024];
  char repo_root[1024];
  bool has_repo_paths = false;

  window = stygian_window_create(&win_cfg);
  if (!window)
    return 1;

  has_repo_paths = icon_lab_resolve_repo_paths(shader_dir, sizeof(shader_dir),
                                               atlas_png, sizeof(atlas_png),
                                               atlas_json, sizeof(atlas_json),
                                               repo_root, sizeof(repo_root));
#ifdef _WIN32
  if (has_repo_paths)
    SetCurrentDirectoryA(repo_root);
#endif
  cfg.backend = STYGIAN_EDITOR_ICON_LAB_BACKEND;
  cfg.window = window;
  if (has_repo_paths) {
    cfg.shader_dir = shader_dir;
    cfg.default_font_atlas_png = atlas_png;
    cfg.default_font_atlas_json = atlas_json;
  }
  ctx = stygian_create(&cfg);
  if (!ctx) {
    stygian_window_destroy(window);
    return 1;
  }

  font = stygian_get_default_font(ctx);

  while (!stygian_window_should_close(window)) {
    StygianEvent event;
    IconLabFrameState frame = {0};
    IconLabLayout layout;
    int width = 0;
    int height = 0;
    uint64_t now_ms = icon_lab_now_ms();
    int i;

    stygian_window_get_size(window, &width, &height);
    icon_lab_compute_layout(&layout, width, height);

    while (stygian_window_poll_event(window, &event)) {
      if (event.type == STYGIAN_EVENT_CLOSE) {
        stygian_window_request_close(window);
      } else if (event.type == STYGIAN_EVENT_MOUSE_MOVE) {
        frame.mouse_x = (float)event.mouse_move.x;
        frame.mouse_y = (float)event.mouse_move.y;
        frame.event_requested = true;
      } else if (event.type == STYGIAN_EVENT_MOUSE_DOWN) {
        frame.mouse_x = (float)event.mouse_button.x;
        frame.mouse_y = (float)event.mouse_button.y;
        if (event.mouse_button.button == STYGIAN_MOUSE_LEFT) {
          frame.left_down = true;
          frame.left_pressed = true;
        } else if (event.mouse_button.button == STYGIAN_MOUSE_RIGHT) {
          frame.right_pressed = true;
        }
        frame.event_requested = true;
      } else if (event.type == STYGIAN_EVENT_MOUSE_UP) {
        frame.mouse_x = (float)event.mouse_button.x;
        frame.mouse_y = (float)event.mouse_button.y;
        if (event.mouse_button.button == STYGIAN_MOUSE_LEFT) {
          frame.left_down = false;
          frame.left_released = true;
        }
        frame.event_requested = true;
      } else if (event.type == STYGIAN_EVENT_RESIZE) {
        width = event.resize.width;
        height = event.resize.height;
        icon_lab_compute_layout(&layout, width, height);
        frame.event_requested = true;
      }
    }

    if (!frame.event_requested && !stygian_has_pending_repaint(ctx)) {
      if (!stygian_window_wait_event_timeout(window, &event, 16u))
        continue;
      if (event.type == STYGIAN_EVENT_MOUSE_MOVE) {
        frame.mouse_x = (float)event.mouse_move.x;
        frame.mouse_y = (float)event.mouse_move.y;
      } else if (event.type == STYGIAN_EVENT_MOUSE_DOWN) {
        frame.mouse_x = (float)event.mouse_button.x;
        frame.mouse_y = (float)event.mouse_button.y;
        if (event.mouse_button.button == STYGIAN_MOUSE_LEFT) {
          frame.left_down = true;
          frame.left_pressed = true;
        } else if (event.mouse_button.button == STYGIAN_MOUSE_RIGHT) {
          frame.right_pressed = true;
        }
      } else if (event.type == STYGIAN_EVENT_MOUSE_UP) {
        frame.mouse_x = (float)event.mouse_button.x;
        frame.mouse_y = (float)event.mouse_button.y;
        if (event.mouse_button.button == STYGIAN_MOUSE_LEFT) {
          frame.left_down = false;
          frame.left_released = true;
        }
      } else if (event.type == STYGIAN_EVENT_CLOSE) {
        stygian_window_request_close(window);
      }
    }

    icon_lab_query_mouse(window, &frame.mouse_x, &frame.mouse_y);
    frame.left_down = stygian_mouse_down(window, STYGIAN_MOUSE_LEFT);

    stygian_begin_frame_intent(ctx, width, height, STYGIAN_FRAME_RENDER);

    stygian_rect(ctx, 0.0f, 0.0f, (float)width, (float)height, 0.07f, 0.07f,
                 0.08f, 1.0f);
    stygian_rect_rounded(ctx, layout.sidebar_x, layout.sidebar_y,
                         layout.sidebar_w, layout.sidebar_h, 0.10f, 0.10f,
                         0.11f, 1.0f, 18.0f);
    stygian_rect_rounded(ctx, layout.stage_x, layout.stage_y, layout.stage_w,
                         layout.stage_h, 0.10f, 0.10f, 0.11f, 1.0f, 18.0f);
    stygian_rect_rounded(ctx, layout.info_x, layout.info_y, layout.info_w,
                         layout.info_h, 0.10f, 0.10f, 0.11f, 1.0f, 18.0f);

    stygian_text(ctx, font, "Icon Lab", layout.sidebar_x + 18.0f,
                 layout.sidebar_y + 18.0f, 20.0f, 0.95f, 0.95f, 0.96f, 1.0f);
    stygian_text(ctx, font, "Shared SDF icon source", layout.sidebar_x + 18.0f,
                 layout.sidebar_y + 44.0f, 13.0f, 0.62f, 0.64f, 0.68f, 1.0f);

    for (i = 0; i < STYGIAN_EDITOR_ICON_COUNT; ++i) {
      float row_x = layout.sidebar_x + 12.0f;
      float row_y = layout.sidebar_y + 82.0f + i * 38.0f;
      float row_w = layout.sidebar_w - 24.0f;
      float row_h = 30.0f;
      bool hovered = icon_lab_point_in_rect(frame.mouse_x, frame.mouse_y, row_x,
                                            row_y, row_w, row_h);
      bool active = selected == (StygianEditorIconKind)i;
      float bg = active ? 0.11f : 0.08f;
      if (hovered)
        bg += 0.05f;
      stygian_rect_rounded(ctx, row_x, row_y, row_w, row_h, bg, bg, bg + 0.01f,
                           0.96f, 10.0f);
      if (hovered || active) {
        stygian_rect_rounded(ctx, row_x + 1.0f, row_y + 1.0f, row_w - 2.0f,
                             row_h - 2.0f, 0.17f, 0.22f, 0.29f,
                             active ? 0.34f : 0.18f, 9.0f);
      }
      stygian_editor_draw_icon(ctx, (StygianEditorIconKind)i, row_x + 8.0f,
                               row_y + 4.0f, 22.0f, 22.0f, 0.93f, 0.93f, 0.95f,
                               1.0f);
      stygian_text(ctx, font,
                   stygian_editor_icon_name((StygianEditorIconKind)i),
                   row_x + 38.0f, row_y + 6.0f, 13.0f, 0.88f, 0.89f, 0.91f,
                   1.0f);
      if (hovered && frame.left_pressed) {
        selected = (StygianEditorIconKind)i;
      }
    }

    stygian_text(ctx, font, "Stage", layout.stage_x + 18.0f,
                 layout.stage_y + 18.0f, 20.0f, 0.95f, 0.95f, 0.96f, 1.0f);
    stygian_text(ctx, font, "Preview sizes, states, and spacing.", layout.stage_x + 18.0f,
                 layout.stage_y + 44.0f, 13.0f, 0.62f, 0.64f, 0.68f, 1.0f);

    stygian_rect_rounded(ctx, layout.preview_x, layout.preview_y,
                         layout.preview_w, layout.preview_h, 0.08f, 0.08f,
                         0.09f, 1.0f, 16.0f);
    icon_lab_draw_grid(ctx, layout.preview_x + 14.0f, layout.preview_y + 14.0f,
                       layout.preview_w - 28.0f, layout.preview_h - 28.0f,
                       20.0f);
    stygian_line(ctx, layout.preview_x + layout.preview_w * 0.5f,
                 layout.preview_y + 20.0f,
                 layout.preview_x + layout.preview_w * 0.5f,
                 layout.preview_y + layout.preview_h - 20.0f, 1.0f, 0.30f,
                 0.34f, 0.39f, 0.25f);
    stygian_line(ctx, layout.preview_x + 20.0f,
                 layout.preview_y + layout.preview_h * 0.5f,
                 layout.preview_x + layout.preview_w - 20.0f,
                 layout.preview_y + layout.preview_h * 0.5f, 1.0f, 0.30f,
                 0.34f, 0.39f, 0.25f);

    stygian_editor_draw_icon(ctx, selected, layout.preview_x + 120.0f,
                             layout.preview_y + 110.0f, 84.0f, 84.0f, 0.97f,
                             0.97f, 0.99f, 1.0f);
    stygian_text(ctx, font, "Large", layout.preview_x + 126.0f,
                 layout.preview_y + 208.0f, 12.0f, 0.62f, 0.64f, 0.68f, 1.0f);

    stygian_editor_draw_icon(ctx, selected, layout.preview_x + 280.0f,
                             layout.preview_y + 118.0f, 48.0f, 48.0f, 0.94f,
                             0.94f, 0.96f, 1.0f);
    stygian_editor_draw_icon(ctx, selected, layout.preview_x + 352.0f,
                             layout.preview_y + 124.0f, 36.0f, 36.0f, 0.94f,
                             0.94f, 0.96f, 1.0f);
    stygian_editor_draw_icon(ctx, selected, layout.preview_x + 412.0f,
                             layout.preview_y + 129.0f, 26.0f, 26.0f, 0.94f,
                             0.94f, 0.96f, 1.0f);
    stygian_text(ctx, font, "48", layout.preview_x + 294.0f,
                 layout.preview_y + 181.0f, 12.0f, 0.62f, 0.64f, 0.68f, 1.0f);
    stygian_text(ctx, font, "36", layout.preview_x + 362.0f,
                 layout.preview_y + 181.0f, 12.0f, 0.62f, 0.64f, 0.68f, 1.0f);
    stygian_text(ctx, font, "26", layout.preview_x + 419.0f,
                 layout.preview_y + 181.0f, 12.0f, 0.62f, 0.64f, 0.68f, 1.0f);

    {
      float state_x = layout.preview_x + 120.0f;
      float state_y = layout.preview_y + 280.0f;
      float button_w = 56.0f;
      float button_h = 56.0f;
      const char *labels[3] = {"Idle", "Hover", "Active"};
      for (i = 0; i < 3; ++i) {
        float bx = state_x + i * 88.0f;
        float bg_r = i == 2 ? 0.09f : 0.14f;
        float bg_g = i == 2 ? 0.46f : 0.14f;
        float bg_b = i == 2 ? 0.86f : 0.15f;
        stygian_rect_rounded(ctx, bx, state_y, button_w, button_h, bg_r, bg_g,
                             bg_b, 1.0f, 12.0f);
        stygian_rect_rounded(ctx, bx + 1.0f, state_y + 1.0f, button_w - 2.0f,
                             button_h - 2.0f, bg_r + 0.03f, bg_g + 0.03f,
                             bg_b + 0.03f, i == 1 ? 0.34f : 0.20f, 11.0f);
        if (i > 0) {
          stygian_rect_rounded(ctx, bx - 1.0f, state_y - 1.0f, button_w + 2.0f,
                               button_h + 2.0f, 0.27f, 0.63f, 0.98f,
                               i == 1 ? 0.12f : 0.08f, 13.0f);
        }
        stygian_editor_draw_icon(ctx, selected, bx + 15.0f, state_y + 15.0f,
                                 26.0f, 26.0f, 0.97f, 0.97f, 0.99f, 1.0f);
        stygian_text(ctx, font, labels[i], bx + 11.0f, state_y + 68.0f, 12.0f,
                     0.64f, 0.66f, 0.69f, 1.0f);
      }
    }

    stygian_text(ctx, font, "Usage", layout.info_x + 18.0f,
                 layout.info_y + 18.0f, 20.0f, 0.95f, 0.95f, 0.96f, 1.0f);
    stygian_text(ctx, font, stygian_editor_icon_name(selected),
                 layout.info_x + 18.0f, layout.info_y + 56.0f, 16.0f, 0.85f,
                 0.89f, 0.98f, 1.0f);
    stygian_text(ctx, font,
                 "This lab is for tuning icon geometry and state read.",
                 layout.info_x + 18.0f, layout.info_y + 88.0f, 13.0f, 0.62f,
                 0.64f, 0.68f, 1.0f);
    stygian_text(ctx, font,
                 "It does not export project assets. It feeds shared C icon "
                 "draw code.",
                 layout.info_x + 18.0f, layout.info_y + 108.0f, 13.0f, 0.62f,
                 0.64f, 0.68f, 1.0f);
    stygian_text(ctx, font,
                 "Right click the preview or use Copy Call to push a C snippet.",
                 layout.info_x + 18.0f, layout.info_y + 128.0f, 13.0f, 0.62f,
                 0.64f, 0.68f, 1.0f);

    stygian_rect_rounded(ctx, layout.info_x + 18.0f, layout.info_y + 160.0f,
                         layout.info_w - 36.0f, 78.0f, 0.08f, 0.08f, 0.09f,
                         1.0f, 12.0f);
    stygian_text(ctx, font, "Call:", layout.info_x + 32.0f,
                 layout.info_y + 174.0f, 13.0f, 0.72f, 0.74f, 0.78f, 1.0f);
    stygian_text(ctx, font, "stygian_editor_draw_icon(...)", layout.info_x + 32.0f,
                 layout.info_y + 196.0f, 13.0f, 0.92f, 0.92f, 0.94f, 1.0f);
    stygian_text(ctx, font, "enum: use the selected icon kind", layout.info_x + 32.0f,
                 layout.info_y + 216.0f, 13.0f, 0.62f, 0.64f, 0.68f, 1.0f);

    {
      bool copy_hovered =
          icon_lab_point_in_rect(frame.mouse_x, frame.mouse_y, layout.copy_x,
                                 layout.copy_y, layout.copy_w, layout.copy_h);
      float bg = copy_hovered ? 0.15f : 0.11f;
      stygian_rect_rounded(ctx, layout.copy_x, layout.copy_y, layout.copy_w,
                           layout.copy_h, bg, bg + 0.02f, bg + 0.04f, 1.0f,
                           11.0f);
      stygian_text(ctx, font, "Copy Call Snippet", layout.copy_x + 18.0f,
                   layout.copy_y + 8.0f, 14.0f, 0.96f, 0.96f, 0.97f, 1.0f);
      if ((copy_hovered && frame.left_pressed) ||
          (frame.right_pressed &&
           icon_lab_point_in_rect(frame.mouse_x, frame.mouse_y, layout.preview_x,
                                  layout.preview_y, layout.preview_w,
                                  layout.preview_h))) {
        icon_lab_copy_snippet(ctx, selected);
        copied_until_ms = now_ms + 1800u;
      }
    }

    if (copied_until_ms > now_ms) {
      stygian_rect_rounded(ctx, layout.info_x + 18.0f, layout.info_y + 310.0f,
                           layout.info_w - 36.0f, 30.0f, 0.11f, 0.18f, 0.13f,
                           1.0f, 10.0f);
      stygian_text(ctx, font, "Copied icon call snippet to clipboard.",
                   layout.info_x + 30.0f, layout.info_y + 318.0f, 13.0f, 0.82f,
                   0.93f, 0.84f, 1.0f);
      stygian_request_repaint_after_ms(ctx, 16u);
    }

    stygian_end_frame(ctx);
  }

  stygian_destroy(ctx);
  stygian_window_destroy(window);
  return 0;
}
