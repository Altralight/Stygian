#include "../../../../include/stygian.h"
#include "../../../../widgets/stygian_widgets.h"
#include "../../../../window/stygian_input.h"
#include "../../../../window/stygian_window.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef STYGIAN_TEMPLATE_APP_TITLE
#define STYGIAN_TEMPLATE_APP_TITLE "StygianWeb"
#endif

#ifdef STYGIAN_DEMO_VULKAN
#define STYGIAN_TEMPLATE_BACKEND STYGIAN_BACKEND_VULKAN
#define STYGIAN_TEMPLATE_WINDOW_FLAGS                                           \
  (STYGIAN_WINDOW_RESIZABLE | STYGIAN_WINDOW_BORDERLESS |                      \
   STYGIAN_WINDOW_VULKAN)
#else
#define STYGIAN_TEMPLATE_BACKEND STYGIAN_BACKEND_OPENGL
#define STYGIAN_TEMPLATE_WINDOW_FLAGS                                           \
  (STYGIAN_WINDOW_RESIZABLE | STYGIAN_WINDOW_BORDERLESS |                      \
   STYGIAN_WINDOW_OPENGL)
#endif

typedef struct TemplateLayout {
  float shell_x, shell_y, shell_w, shell_h;
  float titlebar_h;
  float button_w, button_h, button_gap;
  float button_y;
  float menu_x;
  float minimize_x;
  float maximize_x;
  float close_x;
  float drawer_x;
  float drawer_y;
  float drawer_w;
  float drawer_h;
  float drawer_menu_x;
  float drawer_menu_y;
  float drawer_menu_w;
  float drawer_menu_h;
  float drawer_collapsed_w;
  float drawer_expanded_w;
  float title_reserve_w;
  float url_x;
  float url_y;
  float url_w;
  float url_h;
  float actions_x;
  float actions_y;
  float actions_w;
  float actions_h;
  float content_x;
  float content_y;
  float content_w;
  float content_h;
} TemplateLayout;

static bool template_point_in_url_bar(const TemplateLayout *layout, float px,
                                      float py);
static int template_hovered_action_index(const TemplateLayout *layout, float px,
                                         float py, int expanded_index);

static bool template_point_in_rect(float px, float py, float x, float y, float w,
                                   float h) {
  return px >= x && px < (x + w) && py >= y && py < (y + h);
}

static bool template_point_in_button(const TemplateLayout *layout, float px,
                                     float py, float x) {
  return template_point_in_rect(px, py, x, layout->button_y, layout->button_w,
                                layout->button_h);
}

static bool template_point_in_drawer_menu(const TemplateLayout *layout, float px,
                                          float py) {
  if (!layout)
    return false;
  return template_point_in_rect(px, py, layout->drawer_menu_x,
                                layout->drawer_menu_y, layout->drawer_menu_w,
                                layout->drawer_menu_h);
}

static bool template_point_in_drawer_hotzone(const TemplateLayout *layout,
                                             float px, float py) {
  float hot_x;
  float hot_w;
  if (!layout)
    return false;
  hot_x = layout->shell_x;
  hot_w = (layout->drawer_x + layout->drawer_w + 8.0f) - hot_x;
  return template_point_in_rect(px, py, hot_x, layout->drawer_y, hot_w,
                                layout->drawer_h);
}

static void template_nav_pod_rect(const TemplateLayout *layout, float *out_x,
                                  float *out_y, float *out_w, float *out_h) {
  if (!layout || !out_x || !out_y || !out_w || !out_h)
    return;
  *out_x = layout->content_x + 8.0f;
  *out_y = layout->content_y + 8.0f;
  *out_w = 102.0f;
  *out_h = 34.0f;
}

static bool template_point_in_nav_pod(const TemplateLayout *layout, float px,
                                      float py) {
  float x, y, w, h;
  template_nav_pod_rect(layout, &x, &y, &w, &h);
  return template_point_in_rect(px, py, x, y, w, h);
}

static int template_hovered_nav_index(const TemplateLayout *layout, float px,
                                      float py) {
  float pod_x, pod_y, pod_w, pod_h;
  float button_x;
  int i;
  template_nav_pod_rect(layout, &pod_x, &pod_y, &pod_w, &pod_h);
  button_x = pod_x + 5.0f;
  for (i = 0; i < 3; ++i) {
    if (template_point_in_rect(px, py, button_x, pod_y + 5.0f, 24.0f, 24.0f))
      return i;
    button_x += 30.0f;
  }
  return -1;
}

static float template_button_tint(bool hovered, bool pressed) {
  if (pressed)
    return 0.24f;
  if (hovered)
    return 0.205f;
  return 0.165f;
}

static void template_button_fill(bool hovered, bool pressed, bool is_close,
                                 float *out_r, float *out_g, float *out_b) {
  if (!out_r || !out_g || !out_b)
    return;
  if (is_close && hovered) {
    if (pressed) {
      *out_r = 0.84f;
      *out_g = 0.23f;
      *out_b = 0.23f;
    } else {
      *out_r = 0.66f;
      *out_g = 0.18f;
      *out_b = 0.18f;
    }
    return;
  }

  {
    float bg = template_button_tint(hovered, pressed);
    *out_r = bg;
    *out_g = bg;
    *out_b = bg + 0.005f;
  }
}

static float template_snap(float v) { return floorf(v + 0.5f); }

static float template_clamp(float v, float lo, float hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static uint64_t template_now_ms(void) {
#ifdef _WIN32
  return (uint64_t)GetTickCount64();
#else
  return (uint64_t)((clock() * 1000.0) / CLOCKS_PER_SEC);
#endif
}

static float template_lerp(float a, float b, float t) {
  return a + (b - a) * template_clamp(t, 0.0f, 1.0f);
}

static float template_approach(float current, float target, float step) {
  if (current < target)
    return fminf(current + step, target);
  if (current > target)
    return fmaxf(current - step, target);
  return current;
}

static int template_utf8_encode(char *bytes, uint32_t codepoint) {
  if (!bytes)
    return 0;
  if (codepoint < 0x80u) {
    bytes[0] = (char)codepoint;
    return 1;
  }
  if (codepoint < 0x800u) {
    bytes[0] = (char)(0xC0u | (codepoint >> 6));
    bytes[1] = (char)(0x80u | (codepoint & 0x3Fu));
    return 2;
  }
  if (codepoint < 0x10000u) {
    bytes[0] = (char)(0xE0u | (codepoint >> 12));
    bytes[1] = (char)(0x80u | ((codepoint >> 6) & 0x3Fu));
    bytes[2] = (char)(0x80u | (codepoint & 0x3Fu));
    return 3;
  }
  bytes[0] = (char)(0xF0u | (codepoint >> 18));
  bytes[1] = (char)(0x80u | ((codepoint >> 12) & 0x3Fu));
  bytes[2] = (char)(0x80u | ((codepoint >> 6) & 0x3Fu));
  bytes[3] = (char)(0x80u | (codepoint & 0x3Fu));
  return 4;
}

static int template_utf8_prev_char_start(const char *text, int cursor) {
  if (!text || cursor <= 0)
    return 0;
  cursor--;
  while (cursor > 0 && (((unsigned char)text[cursor] & 0xC0u) == 0x80u))
    cursor--;
  return cursor;
}

static int template_utf8_next_char_start(const char *text, int len, int cursor) {
  if (!text || cursor >= len)
    return len;
  cursor++;
  while (cursor < len && (((unsigned char)text[cursor] & 0xC0u) == 0x80u))
    cursor++;
  return cursor;
}

static int template_utf8_insert_char(char *buffer, size_t capacity, int len,
                                     int cursor, uint32_t codepoint) {
  char bytes[4];
  int byte_count;
  if (!buffer || capacity == 0u || len < 0)
    return len;
  if (cursor < 0)
    cursor = 0;
  if (cursor > len)
    cursor = len;
  byte_count = template_utf8_encode(bytes, codepoint);
  if (byte_count <= 0)
    return len;
  if ((size_t)len + (size_t)byte_count + 1u > capacity)
    return len;
  memmove(buffer + cursor + byte_count, buffer + cursor,
          (size_t)(len - cursor) + 1u);
  memcpy(buffer + cursor, bytes, (size_t)byte_count);
  return len + byte_count;
}

static int template_utf8_delete_span(char *buffer, int len, int start, int end) {
  if (!buffer || len < 0)
    return len;
  if (start < 0)
    start = 0;
  if (end > len)
    end = len;
  if (start >= end)
    return len;
  memmove(buffer + start, buffer + end, (size_t)(len - end) + 1u);
  return len - (end - start);
}

static bool template_url_has_selection(int selection_start, int selection_end) {
  return selection_end > selection_start;
}

static void template_url_set_selection(int anchor, int cursor,
                                       int *selection_start,
                                       int *selection_end) {
  if (!selection_start || !selection_end)
    return;
  if (cursor < anchor) {
    *selection_start = cursor;
    *selection_end = anchor;
  } else {
    *selection_start = anchor;
    *selection_end = cursor;
  }
}

static void template_url_clear_selection(int cursor, int *selection_anchor,
                                         int *selection_start,
                                         int *selection_end) {
  if (selection_anchor)
    *selection_anchor = cursor;
  if (selection_start)
    *selection_start = cursor;
  if (selection_end)
    *selection_end = cursor;
}

static bool template_url_delete_selection(char *buffer, int *cursor,
                                          int *selection_anchor,
                                          int *selection_start,
                                          int *selection_end) {
  int len;
  if (!buffer || !cursor || !selection_anchor || !selection_start ||
      !selection_end)
    return false;
  if (!template_url_has_selection(*selection_start, *selection_end))
    return false;
  len = (int)strlen(buffer);
  len = template_utf8_delete_span(buffer, len, *selection_start, *selection_end);
  (void)len;
  *cursor = *selection_start;
  template_url_clear_selection(*cursor, selection_anchor, selection_start,
                               selection_end);
  return true;
}

static int template_utf8_insert_text(char *buffer, size_t capacity, int len,
                                     int cursor, const char *text) {
  size_t insert_len;
  if (!buffer || capacity == 0u || len < 0 || !text)
    return len;
  if (cursor < 0)
    cursor = 0;
  if (cursor > len)
    cursor = len;
  insert_len = strlen(text);
  if (insert_len == 0u)
    return len;
  if ((size_t)len + insert_len + 1u > capacity)
    insert_len = capacity - (size_t)len - 1u;
  if (insert_len == 0u)
    return len;
  memmove(buffer + cursor + insert_len, buffer + cursor,
          (size_t)(len - cursor) + 1u);
  memcpy(buffer + cursor, text, insert_len);
  return len + (int)insert_len;
}

static int template_url_insert_clipboard_text(char *buffer, size_t capacity,
                                              int len, int cursor,
                                              const char *text) {
  char filtered[512];
  int out_len = 0;
  size_t i;
  if (!buffer || capacity == 0u || !text)
    return len;
  for (i = 0u; text[i] != '\0' && out_len < (int)sizeof(filtered) - 1; ++i) {
    unsigned char ch = (unsigned char)text[i];
    if (ch < 0x20u) {
      if (ch == '\r' || ch == '\n' || ch == '\t')
        continue;
      continue;
    }
    filtered[out_len++] = (char)ch;
  }
  filtered[out_len] = '\0';
  return template_utf8_insert_text(buffer, capacity, len, cursor, filtered);
}

static float template_url_text_x(const TemplateLayout *layout) {
  return layout ? (layout->url_x + 44.0f) : 0.0f;
}

static float template_url_text_w(const TemplateLayout *layout) {
  return layout ? (layout->url_w - 70.0f) : 0.0f;
}

static float template_url_text_draw_x(StygianContext *ctx, StygianFont font,
                                      const TemplateLayout *layout,
                                      const char *text, int cursor) {
  char prefix[512];
  float base_x;
  float text_w;
  float visible_w;
  float shift = 0.0f;

  if (!ctx || !font || !layout || !text)
    return template_url_text_x(layout);

  base_x = template_url_text_x(layout);
  visible_w = template_url_text_w(layout);
  text_w = stygian_text_width(ctx, font, text, 13.0f);
  if (text_w > visible_w)
    shift = visible_w - text_w;

  if (cursor >= 0) {
    float caret_x;
    int prefix_len = cursor;
    if (prefix_len < 0)
      prefix_len = 0;
    if (prefix_len > (int)sizeof(prefix) - 1)
      prefix_len = (int)sizeof(prefix) - 1;
    memcpy(prefix, text, (size_t)prefix_len);
    prefix[prefix_len] = '\0';
    caret_x = base_x + shift + stygian_text_width(ctx, font, prefix, 13.0f);
    if (caret_x > base_x + visible_w - 2.0f)
      shift -= caret_x - (base_x + visible_w - 2.0f);
    if (caret_x < base_x)
      shift += base_x - caret_x;
    if (shift > 0.0f)
      shift = 0.0f;
    if (text_w > visible_w && shift < visible_w - text_w)
      shift = visible_w - text_w;
  }

  return base_x + shift;
}

static int template_url_cursor_from_x(StygianContext *ctx, StygianFont font,
                                      const char *text, float text_x,
                                      float mouse_x) {
  char prefix[512];
  int len;
  int cursor = 0;
  float prev_x = text_x;

  if (!ctx || !font || !text)
    return 0;
  len = (int)strlen(text);
  if (mouse_x <= text_x)
    return 0;

  while (cursor < len) {
    int next = template_utf8_next_char_start(text, len, cursor);
    float next_x;
    memcpy(prefix, text, (size_t)next);
    prefix[next] = '\0';
    next_x = text_x + stygian_text_width(ctx, font, prefix, 13.0f);
    if (mouse_x < (prev_x + next_x) * 0.5f)
      return cursor;
    cursor = next;
    prev_x = next_x;
  }
  return len;
}

static void template_url_copy_selection(StygianWindow *window, const char *text,
                                        int selection_start,
                                        int selection_end) {
  char snippet[512];
  int length;
  if (!window || !text || !template_url_has_selection(selection_start,
                                                      selection_end))
    return;
  length = selection_end - selection_start;
  if (length >= (int)sizeof(snippet))
    length = (int)sizeof(snippet) - 1;
  memcpy(snippet, text + selection_start, (size_t)length);
  snippet[length] = '\0';
  stygian_clipboard_write(window, snippet);
}

static void template_url_focus(char *url_edit, size_t capacity,
                               const char *url_committed, bool *url_focused,
                               int *url_cursor, int *selection_anchor,
                               int *selection_start, int *selection_end) {
  size_t len;
  if (!url_edit || capacity == 0u || !url_committed || !url_focused ||
      !url_cursor)
    return;
  strncpy(url_edit, url_committed, capacity - 1u);
  url_edit[capacity - 1u] = '\0';
  len = strlen(url_edit);
  *url_cursor = (int)len;
  *url_focused = true;
  template_url_clear_selection(*url_cursor, selection_anchor, selection_start,
                               selection_end);
}

static void template_url_commit(char *url_committed, size_t committed_capacity,
                                char *url_edit, size_t edit_capacity,
                                bool *url_focused, int *url_cursor,
                                int *selection_anchor, int *selection_start,
                                int *selection_end) {
  size_t len;
  if (!url_committed || committed_capacity == 0u || !url_edit ||
      edit_capacity == 0u || !url_focused || !url_cursor)
    return;
  strncpy(url_committed, url_edit, committed_capacity - 1u);
  url_committed[committed_capacity - 1u] = '\0';
  strncpy(url_edit, url_committed, edit_capacity - 1u);
  url_edit[edit_capacity - 1u] = '\0';
  len = strlen(url_edit);
  *url_cursor = (int)len;
  *url_focused = false;
  template_url_clear_selection(*url_cursor, selection_anchor, selection_start,
                               selection_end);
}

static void template_url_revert(char *url_edit, size_t edit_capacity,
                                const char *url_committed, bool *url_focused,
                                int *url_cursor, int *selection_anchor,
                                int *selection_start, int *selection_end) {
  size_t len;
  if (!url_edit || edit_capacity == 0u || !url_committed || !url_focused ||
      !url_cursor)
    return;
  strncpy(url_edit, url_committed, edit_capacity - 1u);
  url_edit[edit_capacity - 1u] = '\0';
  len = strlen(url_edit);
  *url_cursor = (int)len;
  *url_focused = false;
  template_url_clear_selection(*url_cursor, selection_anchor, selection_start,
                               selection_end);
}

static bool template_handle_url_input(StygianWindow *window,
                                      const StygianEvent *event,
                                      char *url_committed,
                                      size_t committed_capacity,
                                      char *url_edit, size_t edit_capacity,
                                      bool *url_focused, int *url_cursor,
                                      int *selection_anchor,
                                      int *selection_start,
                                      int *selection_end) {
  int len;
  bool ctrl;
  bool shift;

  if (!event || !url_committed || !url_edit || !url_focused || !url_cursor ||
      !*url_focused)
    return false;

  len = (int)strlen(url_edit);
  if (*url_cursor < 0)
    *url_cursor = 0;
  if (*url_cursor > len)
    *url_cursor = len;

  if (event->type == STYGIAN_EVENT_CHAR) {
    int next_len;
    if (event->chr.codepoint < 0x20u || event->chr.codepoint == 0x7Fu)
      return false;
    template_url_delete_selection(url_edit, url_cursor, selection_anchor,
                                  selection_start, selection_end);
    len = (int)strlen(url_edit);
    next_len = template_utf8_insert_char(url_edit, edit_capacity, len,
                                         *url_cursor, event->chr.codepoint);
    if (next_len == len)
      return false;
    *url_cursor += next_len - len;
    template_url_clear_selection(*url_cursor, selection_anchor, selection_start,
                                 selection_end);
    return true;
  }

  if (event->type != STYGIAN_EVENT_KEY_DOWN)
    return false;

  ctrl = (event->key.mods & STYGIAN_MOD_CTRL) != 0u;
  shift = (event->key.mods & STYGIAN_MOD_SHIFT) != 0u;

  if (ctrl) {
    switch (event->key.key) {
    case STYGIAN_KEY_A:
      *selection_anchor = 0;
      *selection_start = 0;
      *selection_end = len;
      *url_cursor = len;
      return true;
    case STYGIAN_KEY_C:
      template_url_copy_selection(window, url_edit, *selection_start,
                                  *selection_end);
      return template_url_has_selection(*selection_start, *selection_end);
    case STYGIAN_KEY_X:
      if (!template_url_has_selection(*selection_start, *selection_end))
        return false;
      template_url_copy_selection(window, url_edit, *selection_start,
                                  *selection_end);
      template_url_delete_selection(url_edit, url_cursor, selection_anchor,
                                    selection_start, selection_end);
      return true;
    case STYGIAN_KEY_V: {
      char *clipboard = stygian_clipboard_read(window);
      int next_len;
      if (!clipboard)
        return false;
      template_url_delete_selection(url_edit, url_cursor, selection_anchor,
                                    selection_start, selection_end);
      len = (int)strlen(url_edit);
      next_len = template_url_insert_clipboard_text(
          url_edit, edit_capacity, len, *url_cursor, clipboard);
      free(clipboard);
      if (next_len == len)
        return false;
      *url_cursor += next_len - len;
      template_url_clear_selection(*url_cursor, selection_anchor,
                                   selection_start, selection_end);
      return true;
    }
    default:
      break;
    }
  }

  switch (event->key.key) {
  case STYGIAN_KEY_LEFT: {
    int next_cursor = template_utf8_prev_char_start(url_edit, *url_cursor);
    if (shift) {
      if (!template_url_has_selection(*selection_start, *selection_end))
        *selection_anchor = *url_cursor;
      *url_cursor = next_cursor;
      template_url_set_selection(*selection_anchor, *url_cursor,
                                 selection_start, selection_end);
    } else {
      *url_cursor = next_cursor;
      template_url_clear_selection(*url_cursor, selection_anchor,
                                   selection_start, selection_end);
    }
    return true;
  }
  case STYGIAN_KEY_RIGHT: {
    int next_cursor = template_utf8_next_char_start(url_edit, len, *url_cursor);
    if (shift) {
      if (!template_url_has_selection(*selection_start, *selection_end))
        *selection_anchor = *url_cursor;
      *url_cursor = next_cursor;
      template_url_set_selection(*selection_anchor, *url_cursor,
                                 selection_start, selection_end);
    } else {
      *url_cursor = next_cursor;
      template_url_clear_selection(*url_cursor, selection_anchor,
                                   selection_start, selection_end);
    }
    return true;
  }
  case STYGIAN_KEY_HOME:
    if (shift && !template_url_has_selection(*selection_start, *selection_end))
      *selection_anchor = *url_cursor;
    *url_cursor = 0;
    if (shift) {
      template_url_set_selection(*selection_anchor, *url_cursor,
                                 selection_start, selection_end);
    } else {
      template_url_clear_selection(*url_cursor, selection_anchor,
                                   selection_start, selection_end);
    }
    return true;
  case STYGIAN_KEY_END:
    if (shift && !template_url_has_selection(*selection_start, *selection_end))
      *selection_anchor = *url_cursor;
    *url_cursor = len;
    if (shift) {
      template_url_set_selection(*selection_anchor, *url_cursor,
                                 selection_start, selection_end);
    } else {
      template_url_clear_selection(*url_cursor, selection_anchor,
                                   selection_start, selection_end);
    }
    return true;
  case STYGIAN_KEY_BACKSPACE: {
    if (template_url_delete_selection(url_edit, url_cursor, selection_anchor,
                                      selection_start, selection_end))
      return true;
    int start = template_utf8_prev_char_start(url_edit, *url_cursor);
    if (start == *url_cursor)
      return false;
    template_utf8_delete_span(url_edit, len, start, *url_cursor);
    *url_cursor = start;
    template_url_clear_selection(*url_cursor, selection_anchor, selection_start,
                                 selection_end);
    return true;
  }
  case STYGIAN_KEY_DELETE: {
    if (template_url_delete_selection(url_edit, url_cursor, selection_anchor,
                                      selection_start, selection_end))
      return true;
    int end = template_utf8_next_char_start(url_edit, len, *url_cursor);
    if (end == *url_cursor)
      return false;
    template_utf8_delete_span(url_edit, len, *url_cursor, end);
    template_url_clear_selection(*url_cursor, selection_anchor, selection_start,
                                 selection_end);
    return true;
  }
  case STYGIAN_KEY_ENTER:
    template_url_commit(url_committed, committed_capacity, url_edit,
                        edit_capacity, url_focused, url_cursor,
                        selection_anchor, selection_start, selection_end);
    return true;
  case STYGIAN_KEY_ESCAPE:
    template_url_revert(url_edit, edit_capacity, url_committed, url_focused,
                        url_cursor, selection_anchor, selection_start,
                        selection_end);
    return true;
  default:
    return false;
  }
}

static void template_handle_titlebar_click(
    StygianWindow *window, StygianContext *ctx, StygianFont font,
    const TemplateLayout *layout, const StygianEvent *event, float mouse_x,
    float mouse_y, char *url_committed, size_t committed_capacity,
    char *url_edit, size_t edit_capacity, bool *url_focused, int *url_cursor,
    int *selection_anchor, int *selection_start, int *selection_end,
    bool *url_drag_selecting, int *expanded_action, bool *more_panel_open,
    bool *shutting_down, bool *event_mutated, bool *event_requested) {
  int clicked_action;
  bool in_menu;
  bool in_url;
  bool in_minimize;
  bool in_maximize;
  bool in_close;
  bool in_titlebar;

  if (!window || !ctx || !layout || !event || !url_committed || !url_edit ||
      !url_focused || !url_cursor || !selection_anchor || !selection_start ||
      !selection_end || !url_drag_selecting || !expanded_action ||
      !more_panel_open || !shutting_down || !event_mutated ||
      !event_requested)
    return;

  clicked_action =
      template_hovered_action_index(layout, mouse_x, mouse_y, *expanded_action);
  in_menu = template_point_in_drawer_menu(layout, mouse_x, mouse_y);
  in_url = template_point_in_url_bar(layout, mouse_x, mouse_y);
  in_minimize =
      template_point_in_button(layout, mouse_x, mouse_y, layout->minimize_x);
  in_maximize =
      template_point_in_button(layout, mouse_x, mouse_y, layout->maximize_x);
  in_close = template_point_in_button(layout, mouse_x, mouse_y, layout->close_x);
  in_titlebar =
      template_point_in_rect(mouse_x, mouse_y, layout->shell_x, layout->shell_y,
                             layout->shell_w, layout->titlebar_h + 10.0f);

  if (in_url) {
    if (*expanded_action != -1 || *more_panel_open) {
      *expanded_action = -1;
      *more_panel_open = false;
      *event_requested = true;
    }
    if (!*url_focused) {
      template_url_focus(url_edit, edit_capacity, url_committed, url_focused,
                         url_cursor, selection_anchor, selection_start,
                         selection_end);
    }
    if (event->mouse_button.clicks >= 2) {
      int len = (int)strlen(url_edit);
      *selection_anchor = 0;
      *selection_start = 0;
      *selection_end = len;
      *url_cursor = len;
      *url_drag_selecting = false;
    } else {
      *url_cursor =
          template_url_cursor_from_x(
              ctx, font, url_edit,
              template_url_text_draw_x(ctx, font, layout, url_edit, *url_cursor),
              mouse_x);
      template_url_clear_selection(*url_cursor, selection_anchor,
                                   selection_start, selection_end);
      *url_drag_selecting = true;
    }
    *event_requested = true;
    return;
  }

  if (*url_focused) {
    template_url_commit(url_committed, committed_capacity, url_edit,
                        edit_capacity, url_focused, url_cursor,
                        selection_anchor, selection_start, selection_end);
    *url_drag_selecting = false;
    *event_requested = true;
  }

  if (clicked_action >= 0) {
    if (clicked_action == 3) {
      *more_panel_open = !*more_panel_open;
      *expanded_action = -1;
    } else {
      *expanded_action =
          (*expanded_action == clicked_action) ? -1 : clicked_action;
      *more_panel_open = false;
    }
    *event_requested = true;
  } else if (in_menu) {
    *expanded_action = -1;
    *more_panel_open = false;
    // This is just a future menu anchor for now. No point pretending otherwise.
    *event_requested = true;
  } else if (in_minimize) {
    *expanded_action = -1;
    *more_panel_open = false;
    stygian_window_minimize(window);
    *event_mutated = true;
    *event_requested = true;
  } else if (in_maximize) {
    *expanded_action = -1;
    *more_panel_open = false;
    if (stygian_window_is_maximized(window))
      stygian_window_restore(window);
    else
      stygian_window_maximize(window);
    *event_mutated = true;
    *event_requested = true;
  } else if (in_close) {
    *expanded_action = -1;
    *more_panel_open = false;
    stygian_set_present_enabled(ctx, false);
    *shutting_down = true;
    stygian_window_request_close(window);
    *event_mutated = true;
    *event_requested = true;
  } else if (in_titlebar) {
    *expanded_action = -1;
    *more_panel_open = false;
    if (event->mouse_button.clicks >= 2) {
      stygian_window_titlebar_double_click(window);
    } else {
      stygian_window_begin_system_move(window);
    }
    *event_mutated = true;
    *event_requested = true;
  } else if (*expanded_action != -1 || *more_panel_open) {
    *expanded_action = -1;
    *more_panel_open = false;
    *event_requested = true;
  }
}

static void template_draw_frame_glyph(StygianContext *ctx, float x, float y,
                                      float w, float h, float radius, float c,
                                      float bg_r, float bg_g, float bg_b,
                                      float stroke) {
  stygian_rect_rounded(ctx, x, y, w, h, c, c, c, 1.0f, radius);
  stygian_rect_rounded(ctx, x + stroke, y + stroke, w - stroke * 2.0f,
                       h - stroke * 2.0f, bg_r, bg_g, bg_b, 1.0f,
                       radius - stroke * 0.35f);
}

static void template_draw_builtin_icon(StygianContext *ctx, float x, float y,
                                       float w, float h, StygianType type,
                                       float c) {
  StygianElement icon = stygian_element_transient(ctx);
  if (!icon)
    return;
  stygian_set_bounds(ctx, icon, template_snap(x), template_snap(y), w, h);
  stygian_set_color(ctx, icon, c, c, c, 1.0f);
  stygian_set_type(ctx, icon, type);
}

static void template_draw_minimize_glyph(StygianContext *ctx, float x, float y,
                                         float w, float h, float c) {
  float gx = x + w * 0.24f;
  float gy = y + h * 0.24f;
  float gw = w * 0.52f;
  float gh = h * 0.52f;
  template_draw_builtin_icon(ctx, gx, gy, gw, gh, STYGIAN_ICON_CHEVRON, c);
}

static void template_draw_maximize_glyph(StygianContext *ctx, float x, float y,
                                         float w, float h, float c, float bg_r,
                                         float bg_g, float bg_b) {
  float size = template_snap(w * 0.42f);
  float left = template_snap(x + (w - size) * 0.5f);
  float top = template_snap(y + (h - size) * 0.5f - 0.5f);

  template_draw_frame_glyph(ctx, left, top, size, size, 1.6f, c, bg_r, bg_g,
                            bg_b, 2.0f);
}

static void template_draw_restore_glyph(StygianContext *ctx, float x, float y,
                                        float w, float h, float c, float bg_r,
                                        float bg_g, float bg_b) {
  float size = template_snap(w * 0.24f);
  float stroke = 1.8f;
  float back_left = template_snap(x + w * 0.42f);
  float back_top = template_snap(y + h * 0.30f);
  float front_left = template_snap(x + w * 0.30f);
  float front_top = template_snap(y + h * 0.42f);

  template_draw_frame_glyph(ctx, back_left, back_top, size, size, 1.4f, c,
                            bg_r, bg_g, bg_b, stroke);
  template_draw_frame_glyph(ctx, front_left, front_top, size, size, 1.4f, c,
                            bg_r, bg_g, bg_b, stroke);
}

static void template_draw_close_glyph(StygianContext *ctx, float x, float y,
                                      float w, float h, float c) {
  float gx = x + w * 0.24f;
  float gy = y + h * 0.24f;
  float gw = w * 0.52f;
  float gh = h * 0.52f;
  template_draw_builtin_icon(ctx, gx, gy, gw, gh, STYGIAN_ICON_CLOSE, c);
}

static void template_draw_menu_glyph(StygianContext *ctx, float x, float y,
                                     float w, float h, float c) {
  float glyph_w = template_snap(w * 0.48f);
  float glyph_h = 2.5f;
  float gap = 3.5f;
  float left = template_snap(x + (w - glyph_w) * 0.5f);
  float top = template_snap(y + (h - (glyph_h * 3.0f + gap * 2.0f)) * 0.5f);
  float radius = 1.0f;

  stygian_rect_rounded(ctx, left, top, glyph_w, glyph_h, c, c, c, 1.0f,
                       radius);
  stygian_rect_rounded(ctx, left, top + glyph_h + gap, glyph_w, glyph_h, c, c,
                       c, 1.0f, radius);
  stygian_rect_rounded(ctx, left, top + (glyph_h + gap) * 2.0f, glyph_w,
                       glyph_h, c, c, c, 1.0f, radius);
}

static void template_draw_title(StygianContext *ctx, StygianFont font,
                                const TemplateLayout *layout,
                                const char *title) {
  float text_size = 14.0f;
  float title_w;
  float title_x;
  float title_y;

  if (!ctx || !font || !layout || !title || !title[0])
    return;

  title_w = stygian_text_width(ctx, font, title, text_size);
  title_x = template_snap(layout->shell_x + (layout->shell_w - title_w) * 0.5f);
  title_y = template_snap(layout->button_y + 5.0f);
  stygian_text(ctx, font, title, title_x, title_y, text_size, 0.91f, 0.93f,
               0.96f, 0.98f);
}

static void template_draw_caption_button_bg(StygianContext *ctx,
                                            const TemplateLayout *layout,
                                            float x, bool hovered, bool pressed,
                                            bool is_close) {
  float r, g, b;
  template_button_fill(hovered, pressed, is_close, &r, &g, &b);
  if (is_close && hovered) {
    float mid_pad = pressed ? 5.0f : 4.0f;
    float outer_pad = pressed ? 8.0f : 7.0f;
    float mid_a = pressed ? 0.55f : 0.35f;
    float outer_a = pressed ? 0.34f : 0.20f;
    stygian_rect_rounded(ctx, x - outer_pad, layout->button_y - outer_pad,
                         layout->button_w + outer_pad * 2.0f,
                         layout->button_h + outer_pad * 2.0f, r, g, b, outer_a,
                         11.0f);
    stygian_rect_rounded(ctx, x - mid_pad, layout->button_y - mid_pad,
                         layout->button_w + mid_pad * 2.0f,
                         layout->button_h + mid_pad * 2.0f, r, g, b, mid_a,
                         9.0f);
  }
  stygian_rect_rounded(ctx, x, layout->button_y, layout->button_w,
                       layout->button_h, r, g, b, 1.0f, 7.0f);
}

static bool template_point_in_url_bar(const TemplateLayout *layout, float px,
                                      float py) {
  if (!layout)
    return false;
  return template_point_in_rect(px, py, layout->url_x, layout->url_y,
                                layout->url_w, layout->url_h);
}

static float template_action_button_width(int index, int expanded_index) {
  static const float expanded_w[4] = {96.0f, 82.0f, 88.0f, 78.0f};
  if (index == 3)
    return 30.0f;
  return (index == expanded_index) ? expanded_w[index] : 30.0f;
}

static void template_action_button_rect(const TemplateLayout *layout,
                                        int expanded_index, int index,
                                        float *out_x, float *out_w) {
  float cursor = layout->actions_x + layout->actions_w;
  float gap = 8.0f;
  int i;

  if (!layout || !out_x || !out_w || index < 0 || index > 3) {
    return;
  }

  for (i = 3; i >= 0; --i) {
    float w = template_action_button_width(i, expanded_index);
    cursor -= w;
    if (i == index) {
      *out_x = cursor;
      *out_w = w;
      return;
    }
    cursor -= gap;
  }

  *out_x = layout->actions_x;
  *out_w = 30.0f;
}

static int template_hovered_action_index(const TemplateLayout *layout, float px,
                                         float py, int expanded_index) {
  int i;
  if (!layout)
    return -1;
  for (i = 0; i < 4; ++i) {
    float x, w;
    template_action_button_rect(layout, expanded_index, i, &x, &w);
    if (template_point_in_rect(px, py, x, layout->actions_y, w,
                               layout->actions_h)) {
      return i;
    }
  }
  return -1;
}

static void template_draw_action_hint(StygianContext *ctx, StygianFont font,
                                      float x, float y, float w,
                                      const char *label) {
  float bubble_w;
  if (!ctx || !font || !label)
    return;
  bubble_w = stygian_text_width(ctx, font, label, 11.0f) + 20.0f;
  if (bubble_w < 54.0f)
    bubble_w = 54.0f;
  stygian_rect_rounded(ctx, x + (w - bubble_w) * 0.5f, y + 40.0f, bubble_w,
                       24.0f, 0.13f, 0.14f, 0.17f, 0.97f, 12.0f);
  stygian_text(ctx, font, label, x + (w - bubble_w) * 0.5f + 10.0f, y + 46.0f,
               11.0f, 0.67f, 0.71f, 0.78f, 0.98f);
}

static void template_draw_more_panel(StygianContext *ctx, StygianFont font,
                                     const TemplateLayout *layout,
                                     int expanded_action) {
  const char *items[4] = {"Bookmarks", "Downloads", "Extensions", "Settings"};
  float more_x, more_w;
  int i;

  if (!ctx || !layout)
    return;

  template_action_button_rect(layout, expanded_action, 3, &more_x, &more_w);
  stygian_rect_rounded(ctx, more_x - 78.0f, layout->actions_y + 42.0f, 148.0f,
                       148.0f, 0.13f, 0.14f, 0.17f, 0.985f, 14.0f);
  for (i = 0; i < 4; ++i) {
    float row_y = layout->actions_y + 52.0f + 32.0f * (float)i;
    stygian_rect_rounded(ctx, more_x - 66.0f, row_y, 124.0f, 24.0f, 0.16f,
                         0.18f, 0.23f, 0.95f, 12.0f);
    if (font) {
      stygian_text(ctx, font, items[i], more_x - 50.0f, row_y + 5.0f, 12.0f,
                   0.91f, 0.93f, 0.96f, 0.95f);
    }
  }
}

static void template_draw_url_selection(StygianContext *ctx, StygianFont font,
                                        const char *url_text, float text_x,
                                        float text_y, int selection_start,
                                        int selection_end) {
  char prefix[512];
  char selected[512];
  float selection_x;
  float selection_w;
  if (!ctx || !font || !url_text ||
      !template_url_has_selection(selection_start, selection_end))
    return;
  if (selection_end >= (int)sizeof(prefix) ||
      selection_end - selection_start >= (int)sizeof(selected))
    return;
  memcpy(prefix, url_text, (size_t)selection_start);
  prefix[selection_start] = '\0';
  memcpy(selected, url_text + selection_start,
         (size_t)(selection_end - selection_start));
  selected[selection_end - selection_start] = '\0';
  selection_x = text_x + stygian_text_width(ctx, font, prefix, 13.0f);
  selection_w = stygian_text_width(ctx, font, selected, 13.0f);
  stygian_rect_rounded(ctx, selection_x - 1.0f, text_y - 2.0f,
                       selection_w + 2.0f, 19.0f, 0.48f, 0.64f, 0.97f, 0.42f,
                       6.0f);
}

static void template_draw_nav_back_glyph(StygianContext *ctx, float x, float y,
                                         float c, float alpha) {
  float cx = x + 12.0f;
  float cy = y + 12.0f;
  stygian_transform_push(ctx);
  stygian_transform_translate(ctx, cx, cy);
  stygian_transform_rotate(ctx, 1.57079632679f);
  stygian_transform_translate(ctx, -cx, -cy);
  template_draw_builtin_icon(ctx, x + 6.0f, y + 6.0f, 12.0f, 12.0f,
                             STYGIAN_ICON_CHEVRON, c);
  stygian_transform_pop(ctx);
}

static void template_draw_nav_forward_glyph(StygianContext *ctx, float x, float y,
                                            float c, float alpha) {
  float cx = x + 12.0f;
  float cy = y + 12.0f;
  stygian_transform_push(ctx);
  stygian_transform_translate(ctx, cx, cy);
  stygian_transform_rotate(ctx, -1.57079632679f);
  stygian_transform_translate(ctx, -cx, -cy);
  template_draw_builtin_icon(ctx, x + 6.0f, y + 6.0f, 12.0f, 12.0f,
                             STYGIAN_ICON_CHEVRON, c);
  stygian_transform_pop(ctx);
}

static void template_draw_nav_reload_glyph(StygianContext *ctx, float x, float y,
                                           float fg, float bg, float alpha,
                                           float angle_radians) {
  float cx = x + 12.0f;
  float cy = y + 12.0f;
  if (!ctx)
    return;
  stygian_transform_push(ctx);
  stygian_transform_translate(ctx, cx, cy);
  stygian_transform_rotate(ctx, angle_radians);
  stygian_transform_translate(ctx, -cx, -cy);
  stygian_rect_rounded(ctx, x + 6.0f, y + 6.0f, 12.0f, 12.0f, fg, fg, fg,
                       alpha, 6.0f);
  stygian_rect_rounded(ctx, x + 8.25f, y + 8.25f, 7.5f, 7.5f, bg, bg,
                       bg + 0.01f, alpha, 3.75f);
  stygian_rect_rounded(ctx, x + 5.0f, y + 11.0f, 14.0f, 2.0f, bg, bg,
                       bg + 0.01f, alpha, 1.0f);
  stygian_rect_rounded(ctx, x + 8.0f, y + 11.0f, 8.0f, 2.0f, fg, fg, fg, alpha,
                       1.0f);
  stygian_transform_pop(ctx);
}

static void template_draw_nav_pod(StygianContext *ctx, StygianFont font,
                                  const TemplateLayout *layout, float alpha,
                                  int hovered_button, bool mouse_down,
                                  float reload_spin) {
  float pod_x, pod_y, pod_w, pod_h;
  float button_x;
  int i;

  if (!ctx || !layout || alpha <= 0.01f)
    return;

  template_nav_pod_rect(layout, &pod_x, &pod_y, &pod_w, &pod_h);
  stygian_rect_rounded(ctx, pod_x, pod_y, pod_w, pod_h, 0.03f, 0.035f, 0.045f,
                       0.84f * alpha, 11.0f);

  button_x = pod_x + 5.0f;
  for (i = 0; i < 3; ++i) {
    bool hovered = (i == hovered_button);
    float bg = hovered ? (mouse_down ? 0.16f : 0.12f) : 0.07f;
    stygian_rect_rounded(ctx, button_x, pod_y + 5.0f, 24.0f, 24.0f, bg, bg,
                         bg + 0.01f, (hovered ? 0.92f : 0.78f) * alpha, 8.0f);
    if (i == 0) {
      template_draw_nav_back_glyph(ctx, button_x, pod_y + 5.0f, 0.94f, alpha);
    } else if (i == 1) {
      template_draw_nav_forward_glyph(ctx, button_x, pod_y + 5.0f, 0.94f,
                                      alpha);
    } else {
      template_draw_nav_reload_glyph(ctx, button_x, pod_y + 5.0f, 0.94f, bg,
                                     alpha, reload_spin * 6.28318530718f);
    }
    button_x += 30.0f;
  }
}

static void template_draw_browser_toolbar(StygianContext *ctx, StygianFont font,
                                          const TemplateLayout *layout,
                                          int hovered_action,
                                          int expanded_action,
                                          bool more_panel_open,
                                          const char *url_text,
                                          bool url_focused,
                                          int url_cursor,
                                          int selection_start,
                                          int selection_end) {
  const char *action_labels[4] = {"Add-ons", "Saved", "History", "More"};
  float field_x = layout->url_x + 8.0f;
  float field_y = layout->url_y + 5.0f;
  float field_w = layout->url_w - 16.0f;
  float field_h = layout->url_h - 10.0f;
  float text_x = 0.0f;
  float text_y = layout->url_y + 8.0f;
  float text_clip_x = layout->url_x + 39.0f;
  float text_clip_y = layout->url_y + 7.0f;
  float text_clip_w = template_url_text_w(layout) + 2.0f;
  float text_clip_h = 20.0f;
  uint8_t text_clip = 0u;
  int i;

  stygian_rect_rounded(ctx, layout->url_x, layout->url_y, layout->url_w,
                       layout->url_h, 0.125f, 0.133f, 0.165f, 0.97f,
                       layout->url_h * 0.5f);
  stygian_rect_rounded(ctx, field_x, field_y, field_w, field_h,
                       url_focused ? 0.185f : 0.165f,
                       url_focused ? 0.205f : 0.184f,
                       url_focused ? 0.25f : 0.227f, 1.0f, field_h * 0.5f);
  stygian_rect_rounded(ctx, layout->url_x + 14.0f, layout->url_y + 10.0f, 16.0f,
                       16.0f, 0.48f, 0.64f, 0.97f, 0.96f, 8.0f);
  stygian_rect_rounded(ctx, layout->url_x + 40.0f, layout->url_y + 10.0f,
                       layout->url_w - 84.0f, 16.0f, 0.27f, 0.31f, 0.38f, 0.36f,
                       8.0f);
  stygian_rect_rounded(ctx, layout->url_x + layout->url_w - 22.0f,
                       template_snap(field_y + (field_h - 7.0f) * 0.5f), 7.0f,
                       7.0f, 0.48f, 0.64f, 0.97f, 0.90f, 3.5f);

  if (font && url_text) {
    text_x = template_url_text_draw_x(ctx, font, layout, url_text,
                                      url_focused ? url_cursor : -1);
    text_clip = stygian_clip_push(ctx, text_clip_x, text_clip_y, text_clip_w,
                                  text_clip_h);
    if (url_focused) {
      template_draw_url_selection(ctx, font, url_text, text_x, text_y,
                                  selection_start, selection_end);
    }
    stygian_text(ctx, font, url_text, text_x, text_y, 13.0f, 0.94f, 0.95f,
                 0.98f, 0.98f);
    if (url_focused && ((template_now_ms() / 500u) & 1u) == 0u) {
      char prefix[512];
      int prefix_len = url_cursor;
      float caret_x;
      if (prefix_len < 0)
        prefix_len = 0;
      if (prefix_len > (int)sizeof(prefix) - 1)
        prefix_len = (int)sizeof(prefix) - 1;
      memcpy(prefix, url_text, (size_t)prefix_len);
      prefix[prefix_len] = '\0';
      caret_x = text_x + stygian_text_width(ctx, font, prefix, 13.0f);
      stygian_rect_rounded(ctx, caret_x, layout->url_y + 9.0f, 2.0f, 16.0f,
                           0.92f, 0.94f, 0.98f, 0.96f, 1.0f);
    }
    (void)text_clip;
    stygian_clip_pop(ctx);
  }

  for (i = 0; i < 4; ++i) {
    float action_x, action_w;
    float chip_w = (i == 3) ? 14.0f : 10.0f;
    bool expanded = (i == expanded_action && i < 3);
      bool hovered = (i == hovered_action);
      float button_y = layout->actions_y;
      float button_h = layout->actions_h;
      float icon_scale = (i == 3 && hovered) ? 1.12f : 1.0f;
      float bg_r = 0.125f;
      float bg_g = 0.133f;
      float bg_b = 0.165f;
      float bg_a = 0.94f;
      template_action_button_rect(layout, expanded_action, i, &action_x, &action_w);
      if (hovered && i < 3 && !expanded) {
        bg_r = 0.10f;
        bg_g = 0.115f;
        bg_b = 0.145f;
      }
      if (i == 3 && hovered) {
        button_y -= 1.0f;
      button_h += 2.0f;
    }
    stygian_rect_rounded(ctx, action_x, button_y, action_w, button_h, bg_r, bg_g,
                         bg_b, bg_a, button_h * 0.5f);
      if (i == 3) {
        float dot_center = action_w * 0.5f - 10.0f;
        float dot_y = layout->actions_y + 12.0f - (icon_scale - 1.0f) * 2.0f;
        float dot_size = 4.0f * icon_scale;
        float gap = 8.0f * icon_scale;
        stygian_rect_rounded(ctx, action_x + dot_center, dot_y, dot_size, dot_size,
                             0.55f, 0.71f, 1.0f, 0.96f, dot_size * 0.5f);
        stygian_rect_rounded(ctx, action_x + dot_center + gap, dot_y, dot_size,
                             dot_size, 0.55f, 0.71f, 1.0f, 0.96f,
                             dot_size * 0.5f);
        stygian_rect_rounded(ctx, action_x + dot_center + gap * 2.0f, dot_y,
                             dot_size, dot_size, 0.55f, 0.71f, 1.0f, 0.96f,
                             dot_size * 0.5f);
      } else {
        float icon_x = expanded ? 12.0f : (action_w * 0.5f - chip_w * 0.5f);
        stygian_rect_rounded(ctx, action_x + icon_x, layout->actions_y + 9.0f,
                             chip_w, 10.0f, 0.48f, 0.64f, 0.97f, 0.86f, 5.0f);
      }
      if (font && expanded) {
        stygian_text(ctx, font, action_labels[i], action_x + 30.0f,
                     layout->actions_y + 8.0f, 12.0f, 0.91f, 0.93f, 0.96f,
                     0.96f);
      }
    if (font && hovered) {
      template_draw_action_hint(ctx, font, action_x, layout->actions_y, action_w,
                                action_labels[i]);
    }
  }

  if (more_panel_open) {
    template_draw_more_panel(ctx, font, layout, expanded_action);
  }
}

static void template_compute_layout(TemplateLayout *layout, int width, int height,
                                    const StygianTitlebarHints *hints,
                                    float drawer_width,
                                    float title_reserve_w) {
  float drawer_progress;
  memset(layout, 0, sizeof(*layout));
  layout->shell_x = 0.0f;
  layout->shell_y = 0.0f;
  layout->shell_w = (float)width;
  layout->shell_h = (float)height;
  layout->titlebar_h = hints->recommended_titlebar_height > 0.0f
                           ? hints->recommended_titlebar_height + 4.0f
                           : 42.0f;
  layout->button_w = hints->recommended_button_width > 0.0f
                         ? hints->recommended_button_width + 6.0f
                         : 34.0f;
  layout->button_h = hints->recommended_button_height > 0.0f
                         ? hints->recommended_button_height + 2.0f
                         : 26.0f;
  layout->button_gap = hints->recommended_button_gap > 0.0f
                           ? hints->recommended_button_gap + 2.0f
                           : 7.0f;
  layout->button_y = layout->shell_y + 7.0f;
  layout->menu_x = layout->shell_x + 12.0f;
  layout->close_x =
      layout->shell_x + layout->shell_w - layout->button_w - 8.0f;
  layout->maximize_x =
      layout->close_x - layout->button_gap - layout->button_w;
  layout->minimize_x =
      layout->maximize_x - layout->button_gap - layout->button_w;
  layout->drawer_collapsed_w = 60.0f;
  layout->drawer_expanded_w = 254.0f;
  layout->drawer_x = layout->shell_x + 8.0f;
  layout->drawer_y = layout->titlebar_h + 6.0f;
  layout->drawer_h = layout->shell_h - layout->drawer_y - 6.0f;
  layout->drawer_w =
      template_clamp(drawer_width, layout->drawer_collapsed_w,
                     layout->drawer_expanded_w);
  drawer_progress =
      (layout->drawer_w - layout->drawer_collapsed_w) /
      (layout->drawer_expanded_w - layout->drawer_collapsed_w);
  layout->drawer_menu_w = layout->button_w;
  layout->drawer_menu_h = layout->button_h;
  layout->drawer_menu_y = layout->drawer_y + 10.0f;
  layout->drawer_menu_x =
      layout->drawer_x +
      template_lerp((layout->drawer_w - layout->drawer_menu_w) * 0.5f, 12.0f,
                    drawer_progress);
  layout->title_reserve_w = template_clamp(title_reserve_w, 140.0f, 260.0f);
  layout->url_x = layout->shell_x + 68.0f;
  layout->url_y = layout->button_y + 1.0f;
  layout->url_h = 34.0f;
  layout->actions_w = template_clamp(layout->shell_w * 0.22f, 160.0f, 260.0f);
  layout->actions_h = 30.0f;
  layout->actions_x = layout->shell_x + layout->shell_w * 0.5f +
                      layout->title_reserve_w * 0.5f + 22.0f;
  layout->actions_y = layout->url_y;
  layout->actions_w = layout->minimize_x - layout->actions_x - 52.0f;
  layout->url_w = layout->shell_x + layout->shell_w * 0.5f -
                  layout->title_reserve_w * 0.5f - 22.0f - layout->url_x;
  if (layout->url_w < 260.0f)
    layout->url_w = 260.0f;
  if (layout->actions_w < 152.0f)
    layout->actions_w = 152.0f;
  layout->content_x = layout->drawer_x + layout->drawer_w + 8.0f;
  layout->content_y = layout->drawer_y;
  layout->content_w = layout->shell_w - layout->content_x - 6.0f;
  layout->content_h = layout->drawer_h;
}

static void template_draw_drawer(StygianContext *ctx, StygianFont font,
                                 const TemplateLayout *layout, bool hovered) {
  float rail_r = hovered ? 0.48f : 0.40f;
  float rail_g = hovered ? 0.64f : 0.56f;
  float rail_b = hovered ? 0.97f : 0.86f;
  float panel_r = hovered ? 0.12f : 0.11f;
  float panel_g = hovered ? 0.13f : 0.12f;
  float panel_b = hovered ? 0.16f : 0.15f;
  float progress;
  float accent_x;
  float menu_x;
  int i;
  const char *labels[5] = {"Home", "Search", "Feeds", "Saved", "Tools"};

  stygian_rect_rounded(ctx, layout->drawer_x, layout->drawer_y, layout->drawer_w,
                       layout->drawer_h, panel_r, panel_g, panel_b, 0.98f, 10.0f);

  accent_x = layout->drawer_x + 6.0f;
  stygian_rect_rounded(ctx, accent_x, layout->drawer_y + 10.0f, 2.0f,
                       layout->drawer_h - 20.0f, rail_r, rail_g, rail_b, 0.85f,
                       1.0f);

  progress =
      (layout->drawer_w - layout->drawer_collapsed_w) /
      (layout->drawer_expanded_w - layout->drawer_collapsed_w);
  progress = template_clamp(progress, 0.0f, 1.0f);

  menu_x = layout->drawer_menu_x;
  stygian_rect_rounded(ctx, template_snap(menu_x), layout->drawer_menu_y,
                       layout->drawer_menu_w, layout->drawer_menu_h, 0.125f,
                       0.133f, 0.165f, 0.92f, layout->drawer_menu_h * 0.5f);
  template_draw_menu_glyph(ctx, template_snap(menu_x), layout->drawer_menu_y,
                           layout->drawer_menu_w, layout->drawer_menu_h, 0.98f);
  if (font && progress > 0.05f) {
    float reveal = progress * progress * (3.0f - 2.0f * progress);
    stygian_text(ctx, font, "Menu", menu_x + 38.0f + (1.0f - reveal) * 6.0f,
                 layout->drawer_menu_y + 7.0f, 13.0f, 0.67f, 0.71f, 0.78f,
                 reveal);
  }

  for (i = 0; i < 5; ++i) {
    float row_y = template_snap(layout->drawer_y + 58.0f + (float)i * 42.0f);
    float tab_h = 34.0f;
    float circle_w = tab_h;
    float capsule_w = template_clamp(layout->drawer_w - 24.0f, tab_h,
                                     layout->drawer_expanded_w - 24.0f);
    float reveal = progress * progress * (3.0f - 2.0f * progress);
    float icon_center_x =
        template_snap(layout->drawer_x + layout->drawer_collapsed_w * 0.5f);
    float tab_left_inset = template_lerp(circle_w * 0.5f, 18.0f, reveal);
    float tab_x = icon_center_x - tab_left_inset;
    float tab_w = template_lerp(circle_w, capsule_w, reveal);
    float radius = template_lerp(tab_h * 0.5f, 10.0f, progress);
    float tab_r = (i == 0) ? 0.125f : 0.11f;
    float tab_g = (i == 0) ? 0.133f : 0.12f;
    float tab_b = (i == 0) ? 0.20f : 0.17f;
    float dot_x = template_snap(icon_center_x - 4.0f);
    float dot_y = template_snap(row_y + (tab_h - 8.0f) * 0.5f);
    float label_x = icon_center_x + 18.0f + (1.0f - reveal) * 6.0f;

    // Keep the collapsed tab as a proper circle so the stretch back to capsule
    // reads without any decorative junk getting in the way.
    stygian_rect_rounded(ctx, tab_x, row_y, tab_w, tab_h, tab_r,
                         tab_g, tab_b, (i == 0) ? 0.96f : 0.80f, radius);
    stygian_rect_rounded(ctx, dot_x, dot_y, 8.0f, 8.0f, 0.48f, 0.64f, 0.97f,
                         0.94f, 4.0f);
    if (font && reveal > 0.05f) {
      stygian_text(ctx, font, labels[i], label_x, row_y + 9.0f, 13.0f, 0.91f,
                   0.93f, 0.96f, reveal);
    }
  }
}

static void template_draw_workspace(StygianContext *ctx, StygianFont font,
                                    const TemplateLayout *layout) {
  (void)font;
  stygian_rect_rounded(ctx, layout->content_x, layout->content_y,
                       layout->content_w, layout->content_h, 0.063f, 0.165f,
                       0.322f, 1.0f, 10.0f);
}

int main(void) {
  const StygianScopeId k_scope_shell = 0x5201u;
  StygianWindowConfig win_cfg = {
      .title = STYGIAN_TEMPLATE_APP_TITLE,
      .width = 1440,
      .height = 920,
      .flags = STYGIAN_TEMPLATE_WINDOW_FLAGS,
  };
  StygianWindow *window = stygian_window_create(&win_cfg);
  StygianContext *ctx;
  StygianFont font = 0;
  char url_committed[512] = "https://fieldengine.space/blog/stygian";
  char url_edit[512] = "https://fieldengine.space/blog/stygian";
  bool first_frame = true;
  float mouse_x = 0.0f;
  float mouse_y = 0.0f;
  float drawer_width = 60.0f;
  float nav_alpha = 0.0f;
  float prev_nav_pod_x = 0.0f;
  float reload_spin = 0.0f;
  float title_reserve_w = 170.0f;
  uint64_t drawer_last_inside_ms = 0u;
  uint64_t nav_last_active_ms = 0u;
  int url_cursor = (int)strlen("https://fieldengine.space/blog/stygian");
  int url_selection_anchor = (int)strlen("https://fieldengine.space/blog/stygian");
  int url_selection_start = (int)strlen("https://fieldengine.space/blog/stygian");
  int url_selection_end = (int)strlen("https://fieldengine.space/blog/stygian");
  bool mouse_down = false;
  bool shutting_down = false;
  bool url_focused = false;
  bool url_drag_selecting = false;
  bool hover_state_valid = false;
  int expanded_action = -1;
  int hovered_action_prev = -1;
  bool more_panel_open = false;
  bool hovered_drawer_prev = false;
  int hovered_nav_prev = -1;
  bool hovered_menu_prev = false;
  bool hovered_minimize_prev = false;
  bool hovered_maximize_prev = false;
  bool hovered_close_prev = false;
#if defined(_WIN32) && defined(STYGIAN_DEMO_VULKAN)
  HWND native_hwnd = NULL;
  bool native_window_shown = false;
#endif

  if (!window)
    return 1;

#if defined(_WIN32) && defined(STYGIAN_DEMO_VULKAN)
  native_hwnd = (HWND)stygian_window_native_handle(window);
  if (native_hwnd)
    ShowWindow(native_hwnd, SW_HIDE);
#endif

  {
    StygianTitlebarBehavior titlebar_behavior = {
        .double_click_mode = STYGIAN_TITLEBAR_DBLCLICK_MAXIMIZE_RESTORE,
        .hover_menu_enabled = false,
    };
    stygian_window_set_titlebar_behavior(window, &titlebar_behavior);
  }

  ctx = stygian_create(&(StygianConfig){
      .backend = STYGIAN_TEMPLATE_BACKEND,
      .max_elements = 4096,
      .max_textures = 64,
      .window = window,
  });
  if (!ctx)
    return 1;
  // This shell is mostly idle chrome, not a twitch demo. Let the swapchain
  // pace the app instead of chewing CPU when nothing interesting is happening.
  stygian_set_vsync(ctx, true);
  font = stygian_get_default_font(ctx);

  while (!stygian_window_should_close(window)) {
    StygianEvent event;
    StygianTitlebarHints hints;
    TemplateLayout layout;
    uint64_t now_ms = 0u;
    bool hovered_drawer_now = false;
    bool drawer_linger_active = false;
    bool drawer_should_expand = false;
    bool nav_trigger_hovered = false;
    bool nav_linger_active = false;
    bool nav_should_show = false;
    bool event_mutated = false;
    bool event_requested = false;
    bool event_eval_requested = false;
    int hovered_action_now = -1;
    int hovered_nav_now = -1;
    uint32_t wait_ms = stygian_next_repaint_wait_ms(ctx, 500u);
    int width, height;

    if (font) {
      title_reserve_w =
          stygian_text_width(ctx, font, STYGIAN_TEMPLATE_APP_TITLE, 14.0f) +
          56.0f;
    }
    stygian_window_get_titlebar_hints(window, &hints);
    stygian_window_get_size(window, &width, &height);
    template_compute_layout(&layout, width, height, &hints, drawer_width,
                            title_reserve_w);

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

      if (event.type == STYGIAN_EVENT_MOUSE_MOVE) {
        mouse_x = (float)event.mouse_move.x;
        mouse_y = (float)event.mouse_move.y;
      } else if (event.type == STYGIAN_EVENT_MOUSE_DOWN ||
                 event.type == STYGIAN_EVENT_MOUSE_UP) {
        mouse_x = (float)event.mouse_button.x;
        mouse_y = (float)event.mouse_button.y;
        mouse_down =
            (event.type == STYGIAN_EVENT_MOUSE_DOWN) &&
            (event.mouse_button.button == STYGIAN_MOUSE_LEFT);
      }

      if (event.type == STYGIAN_EVENT_MOUSE_MOVE && url_drag_selecting &&
          url_focused && mouse_down) {
        url_cursor = template_url_cursor_from_x(
            ctx, font, url_edit,
            template_url_text_draw_x(ctx, font, &layout, url_edit, url_cursor),
            mouse_x);
        template_url_set_selection(url_selection_anchor, url_cursor,
                                   &url_selection_start, &url_selection_end);
        event_requested = true;
      } else if (event.type == STYGIAN_EVENT_MOUSE_UP &&
                 event.mouse_button.button == STYGIAN_MOUSE_LEFT) {
        url_drag_selecting = false;
      }

      if (template_handle_url_input(window, &event, url_committed,
                                    sizeof(url_committed), url_edit,
                                    sizeof(url_edit), &url_focused, &url_cursor,
                                    &url_selection_anchor, &url_selection_start,
                                    &url_selection_end)) {
        event_requested = true;
      }

      if (event.type == STYGIAN_EVENT_MOUSE_DOWN &&
          event.mouse_button.button == STYGIAN_MOUSE_LEFT) {
        if (nav_alpha > 0.05f) {
          int clicked_nav = template_hovered_nav_index(&layout, mouse_x, mouse_y);
          if (clicked_nav >= 0) {
            if (clicked_nav == 2)
              reload_spin = 1.0f;
            event_requested = true;
            continue;
          }
        }
        template_handle_titlebar_click(
            window, ctx, font, &layout, &event, mouse_x, mouse_y,
            url_committed, sizeof(url_committed), url_edit, sizeof(url_edit),
            &url_focused, &url_cursor, &url_selection_anchor,
            &url_selection_start, &url_selection_end, &url_drag_selecting,
            &expanded_action, &more_panel_open, &shutting_down, &event_mutated,
            &event_requested);
      }

      if (event.type == STYGIAN_EVENT_BLUR && url_focused) {
        template_url_commit(url_committed, sizeof(url_committed), url_edit,
                            sizeof(url_edit), &url_focused, &url_cursor,
                            &url_selection_anchor, &url_selection_start,
                            &url_selection_end);
        url_drag_selecting = false;
        event_requested = true;
      }

      if (event.type == STYGIAN_EVENT_CLOSE) {
        stygian_set_present_enabled(ctx, false);
        shutting_down = true;
        stygian_window_request_close(window);
      }
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

        if (event.type == STYGIAN_EVENT_MOUSE_MOVE) {
          mouse_x = (float)event.mouse_move.x;
          mouse_y = (float)event.mouse_move.y;
        } else if (event.type == STYGIAN_EVENT_MOUSE_DOWN ||
                   event.type == STYGIAN_EVENT_MOUSE_UP) {
          mouse_x = (float)event.mouse_button.x;
          mouse_y = (float)event.mouse_button.y;
          mouse_down =
              (event.type == STYGIAN_EVENT_MOUSE_DOWN) &&
              (event.mouse_button.button == STYGIAN_MOUSE_LEFT);
        }

        if (event.type == STYGIAN_EVENT_MOUSE_MOVE && url_drag_selecting &&
            url_focused && mouse_down) {
          url_cursor = template_url_cursor_from_x(
              ctx, font, url_edit,
              template_url_text_draw_x(ctx, font, &layout, url_edit,
                                       url_cursor),
              mouse_x);
          template_url_set_selection(url_selection_anchor, url_cursor,
                                     &url_selection_start,
                                     &url_selection_end);
          event_requested = true;
        } else if (event.type == STYGIAN_EVENT_MOUSE_UP &&
                   event.mouse_button.button == STYGIAN_MOUSE_LEFT) {
          url_drag_selecting = false;
        }

        if (template_handle_url_input(window, &event, url_committed,
                                      sizeof(url_committed), url_edit,
                                      sizeof(url_edit), &url_focused,
                                      &url_cursor, &url_selection_anchor,
                                      &url_selection_start,
                                      &url_selection_end)) {
          event_requested = true;
        }

        if (event.type == STYGIAN_EVENT_MOUSE_DOWN &&
            event.mouse_button.button == STYGIAN_MOUSE_LEFT) {
          if (nav_alpha > 0.05f) {
            int clicked_nav =
                template_hovered_nav_index(&layout, mouse_x, mouse_y);
            if (clicked_nav >= 0) {
              if (clicked_nav == 2)
                reload_spin = 1.0f;
              event_requested = true;
              continue;
            }
          }
          template_handle_titlebar_click(
              window, ctx, font, &layout, &event, mouse_x, mouse_y,
              url_committed, sizeof(url_committed), url_edit,
              sizeof(url_edit), &url_focused, &url_cursor,
              &url_selection_anchor, &url_selection_start, &url_selection_end,
              &url_drag_selecting, &expanded_action, &more_panel_open,
              &shutting_down, &event_mutated, &event_requested);
        }

        if (event.type == STYGIAN_EVENT_BLUR && url_focused) {
          template_url_commit(url_committed, sizeof(url_committed), url_edit,
                              sizeof(url_edit), &url_focused, &url_cursor,
                              &url_selection_anchor, &url_selection_start,
                              &url_selection_end);
          url_drag_selecting = false;
          event_requested = true;
        }

        if (event.type == STYGIAN_EVENT_CLOSE) {
          stygian_set_present_enabled(ctx, false);
          shutting_down = true;
          stygian_window_request_close(window);
        }
      }
    }

    if (shutting_down && stygian_window_should_close(window))
      break;

    now_ms = template_now_ms();
    hovered_drawer_now = template_point_in_drawer_hotzone(&layout, mouse_x, mouse_y);
    hovered_action_now =
        template_hovered_action_index(&layout, mouse_x, mouse_y,
                                      expanded_action);
    hovered_nav_now =
        (nav_alpha > 0.05f) ? template_hovered_nav_index(&layout, mouse_x, mouse_y)
                            : -1;
    if (hovered_drawer_now) {
      drawer_last_inside_ms = now_ms;
    }
    drawer_linger_active =
        !hovered_drawer_now && drawer_last_inside_ms != 0u &&
        (now_ms - drawer_last_inside_ms) < 2000u;
    drawer_should_expand = hovered_drawer_now || drawer_linger_active;
    {
      float nav_pod_x, nav_pod_y, nav_pod_w, nav_pod_h;
      template_nav_pod_rect(&layout, &nav_pod_x, &nav_pod_y, &nav_pod_w,
                            &nav_pod_h);
      if (!first_frame && prev_nav_pod_x != 0.0f &&
          fabsf(nav_pod_x - prev_nav_pod_x) > 0.01f &&
          template_point_in_rect(mouse_x, mouse_y, prev_nav_pod_x, nav_pod_y,
                                 nav_pod_w, nav_pod_h)) {
#ifdef _WIN32
        HWND drag_hwnd = (HWND)stygian_window_native_handle(window);
        if (drag_hwnd) {
          POINT pt = {(LONG)lroundf(mouse_x + (nav_pod_x - prev_nav_pod_x)),
                      (LONG)lroundf(mouse_y)};
          ClientToScreen(drag_hwnd, &pt);
          SetCursorPos(pt.x, pt.y);
        }
#endif
        mouse_x += nav_pod_x - prev_nav_pod_x;
      }
      prev_nav_pod_x = nav_pod_x;
    }
    nav_trigger_hovered =
        hovered_drawer_now || template_point_in_url_bar(&layout, mouse_x, mouse_y) ||
        (nav_alpha > 0.05f && template_point_in_nav_pod(&layout, mouse_x, mouse_y));
    if (nav_trigger_hovered) {
      nav_last_active_ms = now_ms;
    }
    nav_linger_active =
        !nav_trigger_hovered && nav_last_active_ms != 0u &&
        (now_ms - nav_last_active_ms) < 1000u;

    {
      bool hovered_menu_now =
          template_point_in_drawer_menu(&layout, mouse_x, mouse_y);
      bool hovered_minimize_now =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.minimize_x);
      bool hovered_maximize_now =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.maximize_x);
      bool hovered_close_now =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.close_x);

      if (!hover_state_valid || hovered_drawer_now != hovered_drawer_prev ||
          hovered_nav_now != hovered_nav_prev ||
          hovered_action_now != hovered_action_prev ||
          hovered_menu_now != hovered_menu_prev ||
          hovered_minimize_now != hovered_minimize_prev ||
          hovered_maximize_now != hovered_maximize_prev ||
          hovered_close_now != hovered_close_prev) {
        hovered_drawer_prev = hovered_drawer_now;
        hovered_nav_prev = hovered_nav_now;
        hovered_action_prev = hovered_action_now;
        hovered_menu_prev = hovered_menu_now;
        hovered_minimize_prev = hovered_minimize_now;
        hovered_maximize_prev = hovered_maximize_now;
        hovered_close_prev = hovered_close_now;
        hover_state_valid = true;
        event_requested = true;
      }
    }

    {
      float target_drawer_width = drawer_should_expand
                                      ? layout.drawer_expanded_w
                                      : layout.drawer_collapsed_w;
      float next_drawer_width =
          template_approach(drawer_width, target_drawer_width, 26.0f);
      if (fabsf(next_drawer_width - drawer_width) > 0.01f) {
        drawer_width = next_drawer_width;
        event_requested = true;
      }
    }

    nav_should_show = nav_trigger_hovered || nav_linger_active;
    {
      float target_nav_alpha = nav_should_show ? 1.0f : 0.0f;
      float step = nav_should_show ? 0.24f : 0.10f;
      float next_nav_alpha = template_approach(nav_alpha, target_nav_alpha, step);
      if (fabsf(next_nav_alpha - nav_alpha) > 0.001f) {
        nav_alpha = next_nav_alpha;
        event_requested = true;
      }
    }

    if (drawer_linger_active) {
      uint32_t remain_ms = (uint32_t)(2000u - (now_ms - drawer_last_inside_ms));
      stygian_set_repaint_source(ctx, "drawer-linger");
      stygian_request_repaint_after_ms(ctx, remain_ms);
    }

    if (reload_spin > 0.0f) {
      float next_reload_spin = template_approach(reload_spin, 0.0f, 0.12f);
      if (fabsf(next_reload_spin - reload_spin) > 0.001f) {
        reload_spin = next_reload_spin;
        event_requested = true;
        stygian_set_repaint_source(ctx, "reload-spin");
        stygian_request_repaint_after_ms(ctx, 16u);
      }
    }

    if (nav_linger_active) {
      uint32_t remain_ms = (uint32_t)(1000u - (now_ms - nav_last_active_ms));
      stygian_set_repaint_source(ctx, "nav-linger");
      stygian_request_repaint_after_ms(ctx, remain_ms);
    } else if (nav_alpha > 0.0f && !nav_should_show) {
      stygian_set_repaint_source(ctx, "nav-fade");
      stygian_request_repaint_after_ms(ctx, 16u);
    } else if (nav_should_show && nav_alpha < 1.0f) {
      stygian_set_repaint_source(ctx, "nav-reveal");
      stygian_request_repaint_after_ms(ctx, 16u);
    }

    if (url_focused) {
      uint32_t caret_wait_ms = 500u - (uint32_t)(now_ms % 500u);
      if (caret_wait_ms == 0u)
        caret_wait_ms = 500u;
      stygian_set_repaint_source(ctx, "url-caret");
      stygian_request_repaint_after_ms(ctx, caret_wait_ms);
    }

    if (!first_frame && !event_mutated && !event_requested &&
        !event_eval_requested && !stygian_has_pending_repaint(ctx)) {
      stygian_widgets_commit_regions();
      stygian_end_frame(ctx);
      continue;
    }
    first_frame = false;

    stygian_window_get_size(window, &width, &height);
    template_compute_layout(&layout, width, height, &hints, drawer_width,
                            title_reserve_w);

    stygian_begin_frame_intent(
        ctx, width, height,
        (event_eval_requested && !event_requested && !event_mutated &&
         !stygian_has_pending_repaint(ctx))
            ? STYGIAN_FRAME_EVAL_ONLY
            : STYGIAN_FRAME_RENDER);

    stygian_scope_begin(ctx, k_scope_shell);
    stygian_rect(ctx, 0.0f, 0.0f, (float)width, (float)height, 0.09f, 0.094f,
                 0.114f, 1.0f);
    // Draw slightly past the window bounds so the rounded-edge AA gets clipped
    // off instead of leaving a pale seam around the shell.
    stygian_rect_rounded(ctx, layout.shell_x - 2.0f, layout.shell_y - 2.0f,
                         layout.shell_w + 4.0f, layout.shell_h + 4.0f, 0.09f,
                         0.094f, 0.114f, 1.0f, 8.0f);
    template_draw_drawer(ctx, font, &layout,
                         template_point_in_drawer_hotzone(&layout, mouse_x,
                                                          mouse_y));
    template_draw_workspace(ctx, font, &layout);
    template_draw_nav_pod(ctx, font, &layout, nav_alpha, hovered_nav_prev,
                          mouse_down, reload_spin);
    template_draw_browser_toolbar(ctx, font, &layout, hovered_action_prev,
                                  expanded_action, more_panel_open,
                                  url_focused ? url_edit : url_committed,
                                  url_focused, url_cursor,
                                  url_selection_start, url_selection_end);
    {
      bool hovered_minimize =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.minimize_x);
      bool hovered_maximize =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.maximize_x);
      bool hovered_close =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.close_x);
      bool maximized = stygian_window_is_maximized(window);
      float base_icon = 0.94f;
      float minimize_icon = hovered_minimize ? 0.995f : base_icon;
      float maximize_icon = hovered_maximize ? 0.995f : base_icon;
      float close_icon = hovered_close ? 1.0f : 0.965f;
      float max_r, max_g, max_b;
      template_button_fill(hovered_maximize, hovered_maximize && mouse_down,
                           false, &max_r, &max_g, &max_b);
      template_draw_caption_button_bg(ctx, &layout, layout.minimize_x,
                                      hovered_minimize,
                                      hovered_minimize && mouse_down, false);
      template_draw_minimize_glyph(ctx, layout.minimize_x, layout.button_y,
                                   layout.button_w, layout.button_h,
                                   minimize_icon);
      template_draw_caption_button_bg(ctx, &layout, layout.maximize_x,
                                      hovered_maximize,
                                      hovered_maximize && mouse_down, false);
      if (maximized) {
        template_draw_restore_glyph(ctx, layout.maximize_x, layout.button_y,
                                    layout.button_w, layout.button_h,
                                    maximize_icon, max_r, max_g, max_b);
      } else {
        template_draw_maximize_glyph(ctx, layout.maximize_x, layout.button_y,
                                     layout.button_w, layout.button_h,
                                     maximize_icon, max_r, max_g, max_b);
      }
      template_draw_caption_button_bg(ctx, &layout, layout.close_x,
                                      hovered_close,
                                      hovered_close && mouse_down, true);
      template_draw_close_glyph(ctx, layout.close_x, layout.button_y,
                                layout.button_w, layout.button_h, close_icon);
      template_draw_title(ctx, font, &layout, STYGIAN_TEMPLATE_APP_TITLE);
    }
    stygian_scope_end(ctx);

    if (event_requested || event_mutated || first_frame) {
      stygian_scope_invalidate_next(ctx, k_scope_shell);
    }
    if (event_requested || event_mutated) {
      stygian_set_repaint_source(ctx, "stygianweb");
      stygian_request_repaint_after_ms(ctx, 0u);
    }

    stygian_widgets_commit_regions();
    stygian_end_frame(ctx);

#if defined(_WIN32) && defined(STYGIAN_DEMO_VULKAN)
    if (!native_window_shown && native_hwnd) {
      ShowWindow(native_hwnd, SW_SHOW);
      UpdateWindow(native_hwnd);
      native_window_shown = true;
    }
#endif
  }

#ifdef STYGIAN_DEMO_VULKAN
  // Vulkan close already did the present-off ramp. Drop the window first so
  // the compositor stops caring before we tear the backend down.
  if (font)
    stygian_font_destroy(ctx, font);
  stygian_window_destroy(window);
  stygian_destroy(ctx);
#else
  if (font)
    stygian_font_destroy(ctx, font);
  stygian_destroy(ctx);
  stygian_window_destroy(window);
#endif
  return 0;
}
