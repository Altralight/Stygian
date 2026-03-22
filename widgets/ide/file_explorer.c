// file_explorer.c - Real file browser widgets built on the core fs API.

#include "../../include/stygian_fs.h"
#include "../stygian_widgets.h"
#include <stdio.h>
#include <string.h>

static bool file_explorer_mouse_over(StygianContext *ctx, float x, float y,
                                     float w, float h) {
  StygianWindow *win;
  int mx = 0;
  int my = 0;
  if (!ctx)
    return false;
  win = stygian_get_window(ctx);
  if (!win)
    return false;
  stygian_mouse_pos(win, &mx, &my);
  return mx >= x && mx < x + w && my >= y && my < y + h;
}

static void file_explorer_seed_path(StygianFileExplorer *state) {
  const char *seed;
  if (!state)
    return;
  if (state->current_path[0])
    return;
  seed = state->root_path && state->root_path[0] ? state->root_path : ".";
  if (!stygian_fs_path_normalize(seed, state->current_path,
                                 sizeof(state->current_path))) {
    strcpy(state->current_path, ".");
  }
}

static void file_explorer_copy_selected_path(StygianFileExplorer *state,
                                             const char *path) {
  size_t len;
  if (!state)
    return;
  if (!path)
    path = "";
  len = strlen(path);
  if (len >= sizeof(state->selected_path))
    len = sizeof(state->selected_path) - 1u;
  memcpy(state->selected_path, path, len);
  state->selected_path[len] = '\0';
}

static void file_explorer_set_current_path(StygianFileExplorer *state,
                                           const char *path) {
  if (!state || !path || !path[0])
    return;
  if (!stygian_fs_path_normalize(path, state->current_path,
                                 sizeof(state->current_path))) {
    return;
  }
}

static void file_explorer_sync_history(StygianFileExplorer *state) {
  if (!state || !state->current_path[0])
    return;
  if (state->history_count == 0u) {
    snprintf(state->history_paths[0], sizeof(state->history_paths[0]), "%s",
             state->current_path);
    state->history_count = 1u;
    state->history_index = 0;
    return;
  }
  if (state->history_index < 0 ||
      state->history_index >= (int)state->history_count) {
    state->history_index = (int)state->history_count - 1;
  }
  if (strcmp(state->history_paths[state->history_index], state->current_path) != 0) {
    snprintf(state->history_paths[state->history_index],
             sizeof(state->history_paths[state->history_index]), "%s",
             state->current_path);
  }
}

static void file_explorer_push_history(StygianFileExplorer *state,
                                       const char *path) {
  uint32_t i;
  if (!state || !path || !path[0])
    return;
  file_explorer_set_current_path(state, path);
  if (!state->current_path[0])
    return;
  if (state->history_count == 0u) {
    file_explorer_sync_history(state);
    return;
  }
  if (state->history_index >= 0 &&
      strcmp(state->history_paths[state->history_index], state->current_path) == 0) {
    return;
  }
  if (state->history_index < (int)state->history_count - 1) {
    state->history_count = (uint32_t)(state->history_index + 1);
  }
  if (state->history_count >= 16u) {
    for (i = 1u; i < state->history_count; ++i) {
      snprintf(state->history_paths[i - 1u], sizeof(state->history_paths[i - 1u]),
               "%s", state->history_paths[i]);
    }
    state->history_count = 15u;
    state->history_index = 14;
  }
  snprintf(state->history_paths[state->history_count],
           sizeof(state->history_paths[state->history_count]), "%s",
           state->current_path);
  state->history_index = (int)state->history_count;
  state->history_count++;
}

static bool file_explorer_jump_history(StygianFileExplorer *state, int delta) {
  int target;
  if (!state || state->history_count == 0u)
    return false;
  target = state->history_index + delta;
  if (target < 0 || target >= (int)state->history_count)
    return false;
  state->history_index = target;
  file_explorer_set_current_path(state, state->history_paths[target]);
  state->selected_path[0] = '\0';
  state->scroll_y = 0.0f;
  return true;
}

static bool file_explorer_header_button(StygianContext *ctx, float x, float y,
                                        float w, float h, bool hovered,
                                        bool enabled, bool back) {
  float bg = hovered && enabled ? 0.18f : 0.13f;
  float alpha = enabled ? 0.98f : 0.50f;
  float cx = x + w * 0.5f;
  float cy = y + h * 0.5f;
  stygian_rect_rounded(ctx, x, y, w, h, bg, bg, bg + 0.01f, alpha, 6.0f);
  if (back) {
    stygian_line(ctx, cx + 3.0f, cy - 5.0f, cx - 3.0f, cy, 1.5f, 0.92f, 0.92f,
                 0.96f, enabled ? 1.0f : 0.45f);
    stygian_line(ctx, cx - 3.0f, cy, cx + 3.0f, cy + 5.0f, 1.5f, 0.92f, 0.92f,
                 0.96f, enabled ? 1.0f : 0.45f);
  } else {
    stygian_line(ctx, cx - 3.0f, cy - 5.0f, cx + 3.0f, cy, 1.5f, 0.92f, 0.92f,
                 0.96f, enabled ? 1.0f : 0.45f);
    stygian_line(ctx, cx + 3.0f, cy, cx - 3.0f, cy + 5.0f, 1.5f, 0.92f, 0.92f,
                 0.96f, enabled ? 1.0f : 0.45f);
  }
  return enabled && hovered;
}

static void file_explorer_scroll_to_row(StygianFileExplorer *state, float content_h,
                                        float row_h, bool has_parent,
                                        int list_index) {
  float top = 0.0f;
  float bottom = 0.0f;
  if (!state || list_index < 0)
    return;
  top = (has_parent ? row_h : 0.0f) + (float)list_index * row_h;
  bottom = top + row_h;
  if (top < state->scroll_y) {
    state->scroll_y = top;
  } else if (bottom > state->scroll_y + content_h) {
    state->scroll_y = bottom - content_h;
  }
  if (state->scroll_y < 0.0f)
    state->scroll_y = 0.0f;
}

static void file_explorer_row(StygianContext *ctx, StygianFont font, float x,
                              float y, float w, float h, const char *label,
                              bool selected, bool hovered, bool pressed,
                              bool directory, bool parent_row) {
  float icon_x = x + 8.0f;
  float icon_y = y + 6.0f;
  float text_x = x + 32.0f;
  float text_y = y + 5.0f;
  float bg_r = 0.11f;
  float bg_g = 0.12f;
  float bg_b = 0.14f;
  float bg_a = 1.0f;

  if (selected) {
    bg_r = 0.16f;
    bg_g = 0.24f;
    bg_b = 0.38f;
  } else if (hovered) {
    bg_r = 0.16f;
    bg_g = 0.18f;
    bg_b = 0.21f;
  }
  if (pressed) {
    bg_r = selected ? 0.14f : 0.18f;
    bg_g = selected ? 0.22f : 0.22f;
    bg_b = selected ? 0.34f : 0.28f;
  }
  stygian_rect_rounded(ctx, x, y, w, h, bg_r, bg_g, bg_b, bg_a, 6.0f);

  if (parent_row) {
    stygian_line(ctx, icon_x + 11.0f, icon_y + 6.0f, icon_x + 3.0f,
                 icon_y + 6.0f, 1.5f, 0.88f, 0.88f, 0.92f, 1.0f);
    stygian_line(ctx, icon_x + 3.0f, icon_y + 6.0f, icon_x + 7.0f,
                 icon_y + 2.5f, 1.5f, 0.88f, 0.88f, 0.92f, 1.0f);
    stygian_line(ctx, icon_x + 3.0f, icon_y + 6.0f, icon_x + 7.0f,
                 icon_y + 9.5f, 1.5f, 0.88f, 0.88f, 0.92f, 1.0f);
  } else if (directory) {
    stygian_rect_rounded(ctx, icon_x, icon_y + 2.0f, 16.0f, 11.0f, 0.70f,
                         0.56f, 0.20f, 1.0f, 3.0f);
    stygian_rect_rounded(ctx, icon_x + 1.0f, icon_y, 7.0f, 5.0f, 0.82f, 0.69f,
                         0.26f, 1.0f, 2.0f);
  } else {
    stygian_rect_rounded(ctx, icon_x + 2.0f, icon_y, 12.0f, 15.0f, 0.77f,
                         0.80f, 0.86f, 1.0f, 2.0f);
    stygian_rect(ctx, icon_x + 5.0f, icon_y + 4.0f, 6.0f, 1.0f, 0.56f, 0.60f,
                 0.68f, 1.0f);
    stygian_rect(ctx, icon_x + 5.0f, icon_y + 7.0f, 5.0f, 1.0f, 0.56f, 0.60f,
                 0.68f, 1.0f);
  }

  if (font && label) {
    stygian_text(ctx, font, label, text_x, text_y, 14.0f, 0.94f, 0.95f, 0.98f,
                 1.0f);
  }
}

bool stygian_file_explorer(StygianContext *ctx, StygianFont font,
                           StygianFileExplorer *state) {
  StygianFsList list = {0};
  StygianFsListOptions opts = {
      .flags = STYGIAN_FS_LIST_FILES | STYGIAN_FS_LIST_DIRECTORIES |
               STYGIAN_FS_LIST_SORT_DIRECTORIES_FIRST,
      .name_contains = state->name_filter,
      .extension = state->extension_filter,
  };
  char nav_path[STYGIAN_FS_PATH_CAP];
  float header_h = 30.0f;
  float breadcrumb_h = 28.0f;
  float row_h = 28.0f;
  float content_y = state->y + header_h + breadcrumb_h + 14.0f;
  float content_h = state->h - header_h - breadcrumb_h - 18.0f;
  float row_x = state->x + 6.0f;
  float row_w = state->w - 18.0f;
  float total_h = 0.0f;
  float row_y;
  bool has_parent = false;
  bool changed = false;
  bool selected_present = false;
  bool can_go_back = false;
  bool can_go_forward = false;
  bool hover_back = false;
  bool hover_forward = false;
  uint32_t i;
  StygianBreadcrumb crumb = {
      .x = state->x + 8.0f,
      .y = state->y + header_h + 2.0f,
      .w = state->w - 16.0f,
      .h = 22.0f,
      .path = state->current_path,
      .separator = '/',
  };

  if (!ctx || !state)
    return false;

  file_explorer_seed_path(state);
  file_explorer_sync_history(state);
  state->activated_path[0] = '\0';
  stygian_panel_begin(ctx, state->x, state->y, state->w, state->h);

  stygian_rect(ctx, state->x, state->y, state->w, state->h, 0.10f, 0.11f, 0.13f,
               1.0f);
  stygian_rect(ctx, state->x, state->y, state->w, header_h, 0.13f, 0.14f, 0.17f,
               1.0f);
  if (font) {
    stygian_text(ctx, font, "Files", state->x + 74.0f, state->y + 6.0f, 14.0f,
                 0.95f, 0.96f, 0.99f, 1.0f);
  }
  can_go_back = state->history_index > 0;
  can_go_forward = state->history_index >= 0 &&
                   state->history_index < (int)state->history_count - 1;
  hover_back = state->input_mouse_x >= state->x + 8.0f &&
               state->input_mouse_x < state->x + 32.0f &&
               state->input_mouse_y >= state->y + 4.0f &&
               state->input_mouse_y < state->y + 26.0f;
  hover_forward = state->input_mouse_x >= state->x + 36.0f &&
                  state->input_mouse_x < state->x + 60.0f &&
                  state->input_mouse_y >= state->y + 4.0f &&
                  state->input_mouse_y < state->y + 26.0f;
  file_explorer_header_button(ctx, state->x + 8.0f, state->y + 4.0f, 24.0f,
                              22.0f, hover_back, can_go_back, true);
  file_explorer_header_button(ctx, state->x + 36.0f, state->y + 4.0f, 24.0f,
                              22.0f, hover_forward, can_go_forward, false);
  if (state->input_left_pressed && hover_back && can_go_back) {
    if (file_explorer_jump_history(state, -1))
      changed = true;
  } else if (state->input_left_pressed && hover_forward && can_go_forward) {
    if (file_explorer_jump_history(state, 1))
      changed = true;
  }

  nav_path[0] = '\0';
  if (stygian_fs_path_parent(state->current_path, nav_path, sizeof(nav_path)) &&
      strcmp(nav_path, state->current_path) != 0) {
    has_parent = true;
  }
  nav_path[0] = '\0';
  if (stygian_breadcrumb(ctx, font, &crumb, nav_path, (int)sizeof(nav_path))) {
    if (nav_path[0]) {
      file_explorer_push_history(state, nav_path);
      state->selected_path[0] = '\0';
      state->scroll_y = 0.0f;
      changed = true;
    }
  }
  stygian_rect(ctx, state->x + 6.0f, state->y + header_h + breadcrumb_h + 6.0f,
               state->w - 12.0f, 1.0f, 0.18f, 0.19f, 0.21f, 1.0f);

  if (stygian_fs_list(state->current_path, &opts, &list)) {
    total_h = (float)list.count * row_h;
    if (has_parent) {
      total_h += row_h;
    }
  }

  if (state->scroll_y < 0.0f)
    state->scroll_y = 0.0f;
  if (content_h > 0.0f && total_h > content_h &&
      state->scroll_y > total_h - content_h) {
    state->scroll_y = total_h - content_h;
  }
  if (file_explorer_mouse_over(ctx, state->x, content_y, state->w, content_h) &&
      state->input_scroll_dy != 0.0f && total_h > content_h) {
    state->scroll_y -= state->input_scroll_dy * 32.0f;
    if (state->scroll_y < 0.0f)
      state->scroll_y = 0.0f;
    if (state->scroll_y > total_h - content_h)
      state->scroll_y = total_h - content_h;
    changed = true;
  }

  stygian_clip_push(ctx, state->x + 2.0f, content_y, state->w - 4.0f, content_h);

  row_y = content_y - state->scroll_y;

  if (has_parent) {
    bool row_hovered = state->input_mouse_x >= row_x &&
                       state->input_mouse_x < row_x + row_w &&
                       state->input_mouse_y >= row_y &&
                       state->input_mouse_y < row_y + (row_h - 2.0f);
    bool row_pressed = row_hovered && state->input_left_pressed;
    file_explorer_row(ctx, font, row_x, row_y, row_w, row_h - 2.0f, "..",
                      false, row_hovered, row_pressed, true, true);
    if (row_pressed &&
        stygian_fs_path_parent(state->current_path, nav_path, sizeof(nav_path))) {
      file_explorer_push_history(state, nav_path);
      state->selected_path[0] = '\0';
      state->scroll_y = 0.0f;
      changed = true;
    }
    row_y += row_h;
  }

  for (i = 0u; i < list.count; ++i) {
    bool selected = strcmp(state->selected_path, list.entries[i].path) == 0;
    bool directory = list.entries[i].stat.type == STYGIAN_FS_ENTRY_DIRECTORY;
    bool row_hovered = false;
    bool activate_selected = false;
    if (selected)
      selected_present = true;
    if (row_y + row_h >= content_y && row_y <= content_y + content_h) {
      row_hovered = state->input_mouse_x >= row_x &&
                    state->input_mouse_x < row_x + row_w &&
                    state->input_mouse_y >= row_y &&
                    state->input_mouse_y < row_y + (row_h - 2.0f);
      file_explorer_row(ctx, font, row_x, row_y, row_w, row_h - 2.0f,
                        list.entries[i].name, selected, row_hovered,
                        row_hovered && state->input_left_pressed, directory,
                        false);
      if (row_hovered && state->input_left_pressed) {
        file_explorer_copy_selected_path(state, list.entries[i].path);
        selected_present = true;
        changed = true;
        if (state->on_file_select)
          state->on_file_select(state->selected_path);
      }
      activate_selected = row_hovered && state->input_left_pressed &&
                          state->input_left_clicks >= 2;
      if (!activate_selected && selected && state->input_confirm_pressed) {
        activate_selected =
            strcmp(state->selected_path, list.entries[i].path) == 0;
      }
      if (activate_selected && !directory) {
        file_explorer_copy_selected_path(state, list.entries[i].path);
        strcpy(state->activated_path, list.entries[i].path);
        changed = true;
        if (state->on_file_open)
          state->on_file_open(state->activated_path);
      } else if (activate_selected && directory) {
        file_explorer_push_history(state, list.entries[i].path);
        state->selected_path[0] = '\0';
        state->scroll_y = 0.0f;
        changed = true;
      }
    }
    row_y += row_h;
  }

  if (state->selected_path[0] && !selected_present) {
    state->selected_path[0] = '\0';
    changed = true;
  }

  if (list.count > 0u && (state->input_up_pressed || state->input_down_pressed)) {
    int selected_index = -1;
    int target_index = -1;
    for (i = 0u; i < list.count; ++i) {
      if (strcmp(state->selected_path, list.entries[i].path) == 0) {
        selected_index = (int)i;
        break;
      }
    }
    if (state->input_down_pressed) {
      target_index = selected_index < 0 ? 0 : selected_index + 1;
      if (target_index >= (int)list.count)
        target_index = (int)list.count - 1;
    } else if (state->input_up_pressed) {
      target_index = selected_index < 0 ? (int)list.count - 1 : selected_index - 1;
      if (target_index < 0)
        target_index = 0;
    }
    if (target_index >= 0 && target_index < (int)list.count) {
      // File dialogs feel much better when arrows move the selection instead of
      // teleporting into a folder on first keypress.
      file_explorer_copy_selected_path(state, list.entries[target_index].path);
      file_explorer_scroll_to_row(state, content_h, row_h, has_parent, target_index);
      changed = true;
      if (state->on_file_select)
        state->on_file_select(state->selected_path);
    }
  }

  if (list.count == 0u && font) {
    stygian_text(ctx, font, "No files match this view.", row_x + 4.0f,
                 content_y + 14.0f, 13.0f, 0.66f, 0.70f, 0.76f, 1.0f);
  }

  stygian_clip_pop(ctx);
  stygian_scrollbar_v(ctx, state->x + state->w - 10.0f, content_y, 6.0f,
                      content_h, total_h, &state->scroll_y);
  stygian_fs_list_free(&list);
  stygian_panel_end(ctx);
  return changed;
}

// ============================================================================
// Breadcrumb Widget
// ============================================================================

bool stygian_breadcrumb(StygianContext *ctx, StygianFont font,
                        StygianBreadcrumb *state, char *out_path, int max_len) {
  if (!state->path || !state->path[0])
    return false;

  // Make a copy to tokenize safely (or parse manually)
  // For immediate mode, we'll parse on the fly

  float cur_x = state->x;
  float cur_y = state->y;
  float h = state->h > 0 ? state->h : 24.0f;

  const char *ptr = state->path;
  const char *start = ptr;

  bool clicked_any = false;

  // Background container
  // stygian_rect(ctx, state->x, state->y, state->w, h, 0.15f, 0.15f,
  // 0.15f, 1.0f);

  while (*ptr) {
    if (*ptr == state->separator || *ptr == '/' || *ptr == '\\' ||
        *(ptr + 1) == 0) {
      int len = (int)(ptr - start);
      if (*(ptr + 1) == 0 && *ptr != state->separator && *ptr != '/' &&
          *ptr != '\\')
        len++;

      if (len > 0) {
        // Render segment
        char segment[64];
        if (len >= 64)
          len = 63;
        memcpy(segment, start, len);
        segment[len] = 0;

        float text_w = stygian_text_width(ctx, font, segment, 14.0f);
        float item_w = text_w + 16.0f; // Padding

        // Interaction
        bool hovered = file_explorer_mouse_over(ctx, cur_x, cur_y, item_w, h);
        if (hovered) {
          stygian_rect_rounded(ctx, cur_x, cur_y + 2, item_w, h - 4, 0.3f, 0.3f,
                               0.3f, 1.0f, 4.0f);

          // Check click (simple)
          StygianWindow *win = stygian_get_window(ctx);
          if (stygian_mouse_down(win, STYGIAN_MOUSE_LEFT)) {
            // Build path up to this segment
            int path_len = (int)(ptr - state->path);
            if (*(ptr + 1) == 0)
              path_len = (int)strlen(state->path);

            if (path_len < max_len) {
              memcpy(out_path, state->path, path_len);
              out_path[path_len] = 0;
              clicked_any = true;
            }
          }
        }

        stygian_text(ctx, font, segment, cur_x + 8, cur_y + (h - 14) / 2 + 2,
                     14.0f, 0.9f, 0.9f, 0.9f, 1.0f);

        cur_x += item_w;

        // Render separator
        if (*(ptr + 1) != 0 || *ptr == state->separator) {
          char sep_str[2] = {state->separator ? state->separator : '>', 0};
          stygian_text(ctx, font, sep_str, cur_x, cur_y + (h - 14) / 2 + 2,
                       14.0f, 0.5f, 0.5f, 0.5f, 1.0f);
          cur_x += 16.0f;
        }
      }

      start = ptr + 1;
    }
    ptr++;
  }

  return clicked_any;
}
