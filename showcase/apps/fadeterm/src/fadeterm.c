#include "../../../../include/stygian.h"
#include "../../../../widgets/stygian_widgets.h"
#include "../../../../window/stygian_input.h"
#include "../../../../window/stygian_window.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef STYGIAN_TEMPLATE_APP_TITLE
#define STYGIAN_TEMPLATE_APP_TITLE "FadeTerm"
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

#define FADETERM_MAX_TABS 24
#define FADETERM_MAX_INPUT 256
#define FADETERM_MAX_HISTORY 24
#define FADETERM_MAX_TRANSCRIPT 96
#define FADETERM_MAX_LINE 192

typedef struct TemplateLayout {
  float shell_x, shell_y, shell_w, shell_h;
  float titlebar_h;
  float button_w, button_h, button_gap;
  float button_y;
  float menu_x;
  float minimize_x;
  float maximize_x;
  float close_x;
  float tabs_x;
  float tabs_y;
  float tabs_w;
  float tabs_h;
  float tabs_scroll_x;
  float tabs_scroll_w;
  float tabs_add_x;
  float tabs_add_y;
  float tabs_add_w;
  float tabs_add_h;
  float body_x;
  float body_y;
  float body_w;
  float body_h;
  float viewport_x;
  float viewport_y;
  float viewport_w;
  float viewport_h;
  float input_x;
  float input_y;
  float input_w;
  float input_h;
} TemplateLayout;

typedef struct FadeTermSession {
  float zoom;
  int input_len;
  int cursor_idx;
  int history_count;
  int history_browse;
  int transcript_count;
  char cwd[FADETERM_MAX_LINE];
  char input[FADETERM_MAX_INPUT];
  char history[FADETERM_MAX_HISTORY][FADETERM_MAX_INPUT];
  char transcript[FADETERM_MAX_TRANSCRIPT][FADETERM_MAX_LINE];
} FadeTermSession;

typedef struct FadeTermState {
  float mouse_x;
  float mouse_y;
  float tab_scroll;
  float tab_visual_x[FADETERM_MAX_TABS];
  float drag_offset_x;
  float drag_start_x;
  float drag_start_y;
  bool mouse_down;
  bool shutting_down;
  bool terminal_focused;
  bool hover_state_valid;
  bool tab_visual_valid;
  bool tab_drag_active;
  bool hovered_menu_prev;
  bool hovered_minimize_prev;
  bool hovered_maximize_prev;
  bool hovered_close_prev;
  bool hovered_add_prev;
  int hovered_tab_prev;
  int hovered_tab_close_prev;
  int tab_visual_count;
  int drag_tab;
  int active_tab;
  int tab_count;
  int next_session_id;
  char tabs[FADETERM_MAX_TABS][32];
  FadeTermSession sessions[FADETERM_MAX_TABS];
} FadeTermState;

static void fadeterm_init_session(FadeTermSession *session);

static bool fadeterm_repo_path(const char *repo_relative, char *out,
                               size_t out_size) {
#ifdef _WIN32
  char module_path[MAX_PATH];
  DWORD len;
  char *sep;

  if (!repo_relative || !out || out_size == 0u)
    return false;

  len = GetModuleFileNameA(NULL, module_path, (DWORD)sizeof(module_path));
  if (len == 0u || len >= (DWORD)sizeof(module_path))
    return false;

  sep = strrchr(module_path, '\\');
  if (!sep)
    sep = strrchr(module_path, '/');
  if (!sep)
    return false;
  *sep = '\0'; // Drop the exe name.

  sep = strrchr(module_path, '\\');
  if (!sep)
    sep = strrchr(module_path, '/');
  if (!sep)
    return false;
  *sep = '\0'; // Step from showcase\ back to the repo root.

  return snprintf(out, out_size, "%s\\%s", module_path, repo_relative) > 0 &&
         strlen(out) < out_size;
#else
  (void)repo_relative;
  (void)out;
  (void)out_size;
  return false;
#endif
}

static bool template_point_in_rect(float px, float py, float x, float y, float w,
                                   float h) {
  return px >= x && px < (x + w) && py >= y && py < (y + h);
}

static bool template_point_in_button(const TemplateLayout *layout, float px,
                                     float py, float x) {
  return template_point_in_rect(px, py, x, layout->button_y, layout->button_w,
                                layout->button_h);
}

static bool fadeterm_point_in_tab_strip(const TemplateLayout *layout, float px,
                                        float py) {
  return template_point_in_rect(px, py, layout->tabs_scroll_x, layout->tabs_y,
                                layout->tabs_scroll_w, layout->tabs_h);
}

static float template_button_tint(bool hovered, bool pressed) {
  if (pressed)
    return 0.28f;
  if (hovered)
    return 0.24f;
  return 0.205f;
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
  float glyph_w = template_snap(w * 0.42f);
  float glyph_h = 2.0f;
  float gap = 4.0f;
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

static void fadeterm_draw_plus_glyph(StygianContext *ctx, float x, float y,
                                     float w, float h, float c) {
  float pad_x = 1.0f;
  float pad_y = 1.0f;
  template_draw_builtin_icon(ctx, x + pad_x, y + pad_y, w - pad_x * 2.0f,
                             h - pad_y * 2.0f, STYGIAN_ICON_PLUS, c);
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

static void fadeterm_seed_tabs(FadeTermState *state) {
  int i;
  memset(state, 0, sizeof(*state));
  state->hovered_tab_prev = -1;
  state->hovered_tab_close_prev = -1;
  state->drag_tab = -1;
  state->active_tab = 0;
  state->tab_count = 3;
  state->next_session_id = 4;
  state->terminal_focused = true;
  for (i = 0; i < FADETERM_MAX_TABS; ++i)
    fadeterm_init_session(&state->sessions[i]);
  strcpy(state->tabs[0], "PowerShell");
  strcpy(state->tabs[1], "Server");
  strcpy(state->tabs[2], "Build");
}

static void fadeterm_current_directory(char *out, size_t out_size) {
  if (!out || out_size == 0u)
    return;
#ifdef _WIN32
  if (GetCurrentDirectoryA((DWORD)out_size, out) > 0u && out[0])
    return;
#endif
  snprintf(out, out_size, "D:\\Projects\\Code\\Stygian\\showcase");
}

static FadeTermSession *fadeterm_active_session(FadeTermState *state) {
  if (!state || state->active_tab < 0 || state->active_tab >= state->tab_count)
    return NULL;
  return &state->sessions[state->active_tab];
}

static const FadeTermSession *fadeterm_active_session_const(
    const FadeTermState *state) {
  if (!state || state->active_tab < 0 || state->active_tab >= state->tab_count)
    return NULL;
  return &state->sessions[state->active_tab];
}

static void fadeterm_init_session(FadeTermSession *session) {
  if (!session)
    return;
  memset(session, 0, sizeof(*session));
  session->zoom = 12.5f;
  session->history_browse = -1;
  fadeterm_current_directory(session->cwd, sizeof(session->cwd));
}

static void fadeterm_push_transcript(FadeTermSession *session,
                                     const char *line) {
  int i;
  if (!session || !line)
    return;
  if (session->transcript_count >= FADETERM_MAX_TRANSCRIPT) {
    for (i = 1; i < FADETERM_MAX_TRANSCRIPT; ++i)
      memcpy(session->transcript[i - 1], session->transcript[i],
             sizeof(session->transcript[i - 1]));
    session->transcript_count = FADETERM_MAX_TRANSCRIPT - 1;
  }
  snprintf(session->transcript[session->transcript_count++], FADETERM_MAX_LINE,
           "%s", line);
}

static void fadeterm_set_input(FadeTermSession *session, const char *text) {
  if (!session || !text)
    return;
  snprintf(session->input, sizeof(session->input), "%s", text);
  session->input_len = (int)strlen(session->input);
  session->cursor_idx = session->input_len;
}

static void fadeterm_store_history(FadeTermSession *session, const char *text) {
  int i;
  if (!session || !text || !text[0])
    return;
  if (session->history_count > 0 &&
      strcmp(session->history[session->history_count - 1], text) == 0) {
    session->history_browse = -1;
    return;
  }
  if (session->history_count >= FADETERM_MAX_HISTORY) {
    for (i = 1; i < FADETERM_MAX_HISTORY; ++i)
      memcpy(session->history[i - 1], session->history[i],
             sizeof(session->history[0]));
    session->history_count = FADETERM_MAX_HISTORY - 1;
  }
  snprintf(session->history[session->history_count++], FADETERM_MAX_INPUT, "%s",
           text);
  session->history_browse = -1;
}

static void fadeterm_push_transcript_block(FadeTermSession *session,
                                           const char *block) {
  const char *line = block;
  const char *p;
  char scratch[FADETERM_MAX_LINE];

  if (!session || !block || !block[0])
    return;

  while (*line) {
    size_t len;
    p = line;
    while (*p && *p != '\n' && *p != '\r')
      p++;
    len = (size_t)(p - line);
    if (len >= sizeof(scratch))
      len = sizeof(scratch) - 1u;
    memcpy(scratch, line, len);
    scratch[len] = '\0';
    fadeterm_push_transcript(session, scratch);
    while (*p == '\r' || *p == '\n')
      p++;
    line = p;
  }
}

static void fadeterm_command_arg(const char *command, char *out, size_t out_size) {
  const char *start;
  const char *end;
  size_t len;

  if (!out || out_size == 0u) {
    return;
  }
  out[0] = '\0';
  if (!command)
    return;

  start = command + 2;
  while (*start == ' ' || *start == '\t')
    start++;
  end = start + strlen(start);
  while (end > start &&
         (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n'))
    end--;
  if (end > start + 1 && start[0] == '"' && end[-1] == '"') {
    start++;
    end--;
  }
  len = (size_t)(end - start);
  if (len >= out_size)
    len = out_size - 1u;
  memcpy(out, start, len);
  out[len] = '\0';
}

static bool fadeterm_session_change_directory(FadeTermSession *session,
                                              const char *path_arg) {
#ifdef _WIN32
  char candidate[MAX_PATH];
  char resolved[MAX_PATH];
  DWORD attrs;
  DWORD len;

  if (!session || !path_arg || !path_arg[0])
    return false;

  if ((strlen(path_arg) >= 2u && path_arg[1] == ':') ||
      (path_arg[0] == '\\' && path_arg[1] == '\\')) {
    snprintf(candidate, sizeof(candidate), "%s", path_arg);
  } else {
    snprintf(candidate, sizeof(candidate), "%s\\%s", session->cwd, path_arg);
  }

  len = GetFullPathNameA(candidate, (DWORD)sizeof(resolved), resolved, NULL);
  if (len == 0u || len >= (DWORD)sizeof(resolved))
    return false;

  attrs = GetFileAttributesA(resolved);
  if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY))
    return false;

  snprintf(session->cwd, sizeof(session->cwd), "%s", resolved);
  return true;
#else
  (void)session;
  (void)path_arg;
  return false;
#endif
}

static bool fadeterm_run_shell_capture(FadeTermSession *session,
                                       const char *command) {
#ifdef _WIN32
  SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  HANDLE out_read = NULL;
  HANDLE out_write = NULL;
  HANDLE in_read = NULL;
  HANDLE in_write = NULL;
  DWORD written = 0u;
  char buffer[2048];
  char command_line[] =
      "powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -Command -";
  bool ok = false;

  if (!session || !command || !command[0])
    return false;

  memset(&si, 0, sizeof(si));
  memset(&pi, 0, sizeof(pi));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
  si.wShowWindow = SW_HIDE;

  if (!CreatePipe(&out_read, &out_write, &sa, 0))
    goto cleanup;
  if (!SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0))
    goto cleanup;
  if (!CreatePipe(&in_read, &in_write, &sa, 0))
    goto cleanup;
  if (!SetHandleInformation(in_write, HANDLE_FLAG_INHERIT, 0))
    goto cleanup;

  si.hStdOutput = out_write;
  si.hStdError = out_write;
  si.hStdInput = in_read;

  if (!CreateProcessA(NULL, command_line, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                      NULL, session->cwd, &si, &pi)) {
    snprintf(buffer, sizeof(buffer), "shell launch failed (%lu)", GetLastError());
    fadeterm_push_transcript(session, buffer);
    goto cleanup;
  }

  CloseHandle(out_write);
  out_write = NULL;
  CloseHandle(in_read);
  in_read = NULL;

  WriteFile(in_write, command, (DWORD)strlen(command), &written, NULL);
  WriteFile(in_write, "\r\n", 2u, &written, NULL);
  CloseHandle(in_write);
  in_write = NULL;

  while (ReadFile(out_read, buffer, sizeof(buffer) - 1u, &written, NULL) &&
         written > 0u) {
    buffer[written] = '\0';
    fadeterm_push_transcript_block(session, buffer);
  }

  WaitForSingleObject(pi.hProcess, INFINITE);
  ok = true;

cleanup:
  if (out_read)
    CloseHandle(out_read);
  if (out_write)
    CloseHandle(out_write);
  if (in_read)
    CloseHandle(in_read);
  if (in_write)
    CloseHandle(in_write);
  if (pi.hThread)
    CloseHandle(pi.hThread);
  if (pi.hProcess)
    CloseHandle(pi.hProcess);
  return ok;
#else
  (void)session;
  (void)command;
  return false;
#endif
}

static void fadeterm_run_command(FadeTermSession *session,
                                 const char *host_label) {
  char line[FADETERM_MAX_LINE];
  char arg[FADETERM_MAX_LINE];

  if (!session || !host_label)
    return;

  snprintf(line, sizeof(line), "PS %s> %s", host_label,
           session->input[0] ? session->input : "");
  fadeterm_push_transcript(session, line);

  if (!session->input[0]) {
    fadeterm_push_transcript(session, "");
    return;
  }

  fadeterm_store_history(session, session->input);

  if (strcmp(session->input, "cls") == 0 || strcmp(session->input, "clear") == 0) {
    session->transcript_count = 0;
  } else if (strncmp(session->input, "cd", 2) == 0 &&
             (session->input[2] == '\0' || session->input[2] == ' ' ||
              session->input[2] == '\t')) {
    fadeterm_command_arg(session->input, arg, sizeof(arg));
    if (!arg[0]) {
      fadeterm_push_transcript(session, session->cwd);
    } else if (!fadeterm_session_change_directory(session, arg)) {
      snprintf(line, sizeof(line), "The system cannot find the path '%s'.", arg);
      fadeterm_push_transcript(session, line);
    }
  } else {
    fadeterm_run_shell_capture(session, session->input);
  }

  fadeterm_push_transcript(session, "");
  session->input[0] = '\0';
  session->input_len = 0;
  session->cursor_idx = 0;
  session->history_browse = -1;
}

static float fadeterm_tab_width(StygianContext *ctx, StygianFont font,
                                const char *label) {
  float text_w = 62.0f;
  if (ctx && font && label)
    text_w = stygian_text_width(ctx, font, label, 12.5f);
  return template_clamp(text_w + 60.0f, 108.0f, 196.0f);
}

static float fadeterm_tab_total_width(StygianContext *ctx, StygianFont font,
                                      const FadeTermState *state) {
  int i;
  float total = 6.0f;
  for (i = 0; i < state->tab_count; ++i)
    total += fadeterm_tab_width(ctx, font, state->tabs[i]) + 6.0f;
  total += 22.0f;
  return total;
}

static float fadeterm_tab_max_scroll(StygianContext *ctx, StygianFont font,
                                     const TemplateLayout *layout,
                                     const FadeTermState *state) {
  float overflow =
      fadeterm_tab_total_width(ctx, font, state) - layout->tabs_scroll_w;
  return overflow > 0.0f ? overflow : 0.0f;
}

static void fadeterm_clamp_scroll(StygianContext *ctx, StygianFont font,
                                  const TemplateLayout *layout,
                                  FadeTermState *state) {
  state->tab_scroll = template_clamp(
      state->tab_scroll, 0.0f, fadeterm_tab_max_scroll(ctx, font, layout, state));
}

static void fadeterm_tab_target_rect(StygianContext *ctx, StygianFont font,
                                     const TemplateLayout *layout,
                                     const FadeTermState *state, int index,
                                     float *out_x, float *out_w) {
  int i;
  float x = layout->tabs_scroll_x + 4.0f - state->tab_scroll;
  for (i = 0; i < index; ++i)
    x += fadeterm_tab_width(ctx, font, state->tabs[i]) + 6.0f;
  *out_x = x;
  *out_w = fadeterm_tab_width(ctx, font, state->tabs[index]);
}

static void fadeterm_tab_visual_rect(StygianContext *ctx, StygianFont font,
                                     const TemplateLayout *layout,
                                     const FadeTermState *state, int index,
                                     float *out_x, float *out_w) {
  float x, w;
  fadeterm_tab_target_rect(ctx, font, layout, state, index, &x, &w);
  if (state->tab_visual_valid && index >= 0 && index < state->tab_visual_count)
    x = state->tab_visual_x[index];
  *out_x = x;
  *out_w = w;
}

static void fadeterm_swap_tabs(FadeTermState *state, int a, int b) {
  char tmp[32];
  float tmp_x;
  FadeTermSession tmp_session;

  if (!state || a < 0 || b < 0 || a >= state->tab_count || b >= state->tab_count ||
      a == b)
    return;

  memcpy(tmp, state->tabs[a], sizeof(tmp));
  memcpy(state->tabs[a], state->tabs[b], sizeof(state->tabs[a]));
  memcpy(state->tabs[b], tmp, sizeof(tmp));
  memcpy(&tmp_session, &state->sessions[a], sizeof(tmp_session));
  memcpy(&state->sessions[a], &state->sessions[b], sizeof(tmp_session));
  memcpy(&state->sessions[b], &tmp_session, sizeof(tmp_session));

  tmp_x = state->tab_visual_x[a];
  state->tab_visual_x[a] = state->tab_visual_x[b];
  state->tab_visual_x[b] = tmp_x;

  if (state->active_tab == a)
    state->active_tab = b;
  else if (state->active_tab == b)
    state->active_tab = a;

  if (state->hovered_tab_prev == a)
    state->hovered_tab_prev = b;
  else if (state->hovered_tab_prev == b)
    state->hovered_tab_prev = a;
}

static float fadeterm_add_button_x(StygianContext *ctx, StygianFont font,
                                   const TemplateLayout *layout,
                                   const FadeTermState *state) {
  float x = layout->tabs_scroll_x + 4.0f;
  float visible_right =
      layout->tabs_scroll_x + layout->tabs_scroll_w - layout->tabs_add_w - 2.0f;
  bool found_visible = false;
  int i;

  for (i = 0; i < state->tab_count; ++i) {
    float tab_x, tab_w;
    fadeterm_tab_visual_rect(ctx, font, layout, state, i, &tab_x, &tab_w);
    if (tab_x + tab_w < layout->tabs_scroll_x - 2.0f ||
        tab_x > layout->tabs_scroll_x + layout->tabs_scroll_w + 2.0f)
      continue;
    x = tab_x + tab_w + 6.0f;
    found_visible = true;
  }

  if (!found_visible)
    x = layout->tabs_scroll_x + 4.0f;

  return template_clamp(x, layout->tabs_scroll_x + 4.0f, visible_right);
}

static bool fadeterm_point_in_add_button(StygianContext *ctx, StygianFont font,
                                         const TemplateLayout *layout,
                                         const FadeTermState *state, float px,
                                         float py) {
  float add_x = fadeterm_add_button_x(ctx, font, layout, state);
  return template_point_in_rect(px, py, add_x, layout->tabs_add_y,
                                layout->tabs_add_w, layout->tabs_add_h);
}

static int fadeterm_hovered_tab(StygianContext *ctx, StygianFont font,
                                const TemplateLayout *layout,
                                const FadeTermState *state, float px, float py) {
  int i;
  if (!fadeterm_point_in_tab_strip(layout, px, py))
    return -1;
  for (i = 0; i < state->tab_count; ++i) {
    float tab_x, tab_w;
    fadeterm_tab_visual_rect(ctx, font, layout, state, i, &tab_x, &tab_w);
    if (template_point_in_rect(px, py, tab_x, layout->tabs_y + 2.0f, tab_w,
                               layout->tabs_h - 4.0f))
      return i;
  }
  return -1;
}

static bool fadeterm_tab_close_rect(float tab_x, float tab_w, float tab_top_y,
                                    float *out_x, float *out_y, float *out_w,
                                    float *out_h) {
  float close_w = 14.0f;
  float close_h = 14.0f;
  float close_x = template_snap(tab_x + tab_w - close_w - 10.0f);
  float close_y = template_snap(tab_top_y + 5.0f);

  if (tab_w < 98.0f)
    return false;

  if (out_x)
    *out_x = close_x;
  if (out_y)
    *out_y = close_y;
  if (out_w)
    *out_w = close_w;
  if (out_h)
    *out_h = close_h;
  return true;
}

static void fadeterm_draw_tab_close_cap(StygianContext *ctx, float tab_x,
                                        float tab_w, float tab_top_y,
                                        float bury_r, float bury_g,
                                        float bury_b,
                                        bool close_hovered) {
  float close_x, close_y, close_w, close_h;
  float glyph = close_hovered ? 0.98f : 0.78f;

  (void)bury_r;
  (void)bury_g;
  (void)bury_b;

  if (!fadeterm_tab_close_rect(tab_x, tab_w, tab_top_y, &close_x, &close_y,
                               &close_w, &close_h))
    return;

  template_draw_builtin_icon(ctx, close_x, close_y, close_w, close_h,
                             STYGIAN_ICON_CLOSE, glyph);
}

static void fadeterm_end_tab_drag(FadeTermState *state);

static int fadeterm_hovered_tab_close(StygianContext *ctx, StygianFont font,
                                      const TemplateLayout *layout,
                                      const FadeTermState *state, float px,
                                      float py) {
  int i;
  if (!fadeterm_point_in_tab_strip(layout, px, py) || state->tab_count <= 1)
    return -1;
  for (i = 0; i < state->tab_count; ++i) {
    float tab_x, tab_w;
    float cap_x, cap_y, cap_w, cap_h;
    float tab_top_y = (i == state->active_tab) ? layout->tabs_y + 1.0f
                                               : layout->tabs_y + 3.0f;
    fadeterm_tab_visual_rect(ctx, font, layout, state, i, &tab_x, &tab_w);
    if (!fadeterm_tab_close_rect(tab_x, tab_w, tab_top_y, &cap_x, &cap_y,
                                 &cap_w, &cap_h))
      continue;
    if (template_point_in_rect(px, py, cap_x, cap_y, cap_w, cap_h * 0.56f))
      return i;
  }
  return -1;
}

static void fadeterm_reveal_tab(StygianContext *ctx, StygianFont font,
                                const TemplateLayout *layout,
                                FadeTermState *state, int index) {
  float tab_x, tab_w;
  float visible_left = layout->tabs_scroll_x + 6.0f;
  float visible_right = layout->tabs_scroll_x + layout->tabs_scroll_w - 6.0f;
  if (index < 0 || index >= state->tab_count)
    return;
  fadeterm_tab_target_rect(ctx, font, layout, state, index, &tab_x, &tab_w);
  if (tab_x < visible_left)
    state->tab_scroll -= visible_left - tab_x;
  else if (tab_x + tab_w > visible_right)
    state->tab_scroll += (tab_x + tab_w) - visible_right;
  fadeterm_clamp_scroll(ctx, font, layout, state);
}

static bool fadeterm_close_tab(FadeTermState *state, int index) {
  int i;
  if (!state || state->tab_count <= 1 || index < 0 || index >= state->tab_count)
    return false;

  for (i = index; i + 1 < state->tab_count; ++i) {
    memcpy(state->tabs[i], state->tabs[i + 1], sizeof(state->tabs[i]));
    memcpy(&state->sessions[i], &state->sessions[i + 1],
           sizeof(state->sessions[i]));
    state->tab_visual_x[i] = state->tab_visual_x[i + 1];
  }
  state->tab_count--;
  if (state->tab_visual_count > state->tab_count)
    state->tab_visual_count = state->tab_count;

  if (state->active_tab > index)
    state->active_tab--;
  else if (state->active_tab == index)
    state->active_tab = (index >= state->tab_count) ? state->tab_count - 1 : index;

  if (state->drag_tab == index)
    fadeterm_end_tab_drag(state);
  else if (state->drag_tab > index)
    state->drag_tab--;

  if (state->hovered_tab_prev == index)
    state->hovered_tab_prev = -1;
  else if (state->hovered_tab_prev > index)
    state->hovered_tab_prev--;

  if (state->hovered_tab_close_prev == index)
    state->hovered_tab_close_prev = -1;
  else if (state->hovered_tab_close_prev > index)
    state->hovered_tab_close_prev--;

  return true;
}

static void fadeterm_scroll_tabs(StygianContext *ctx, StygianFont font,
                                 const TemplateLayout *layout,
                                 FadeTermState *state, float delta);

static void fadeterm_begin_tab_drag(StygianContext *ctx, StygianFont font,
                                    const TemplateLayout *layout,
                                    FadeTermState *state, int index) {
  float tab_x, tab_w;

  if (!state || index < 0 || index >= state->tab_count)
    return;

  fadeterm_tab_visual_rect(ctx, font, layout, state, index, &tab_x, &tab_w);
  state->drag_tab = index;
  state->tab_drag_active = false;
  state->drag_start_x = state->mouse_x;
  state->drag_start_y = state->mouse_y;
  state->drag_offset_x = state->mouse_x - tab_x;
  if (state->drag_offset_x < 0.0f)
    state->drag_offset_x = tab_w * 0.5f;
}

static void fadeterm_end_tab_drag(FadeTermState *state) {
  if (!state)
    return;
  state->drag_tab = -1;
  state->tab_drag_active = false;
}

static bool fadeterm_update_tab_animation(StygianContext *ctx, StygianFont font,
                                          const TemplateLayout *layout,
                                          FadeTermState *state) {
  bool animating = false;
  int i;

  if (!state)
    return false;

  if (!state->tab_visual_valid) {
    for (i = 0; i < state->tab_count; ++i) {
      float target_x, target_w;
      fadeterm_tab_target_rect(ctx, font, layout, state, i, &target_x, &target_w);
      state->tab_visual_x[i] = target_x;
    }
    state->tab_visual_count = state->tab_count;
    state->tab_visual_valid = true;
  } else if (state->tab_visual_count < state->tab_count) {
    for (i = state->tab_visual_count; i < state->tab_count; ++i) {
      float target_x, target_w;
      fadeterm_tab_target_rect(ctx, font, layout, state, i, &target_x, &target_w);
      state->tab_visual_x[i] = target_x;
    }
    state->tab_visual_count = state->tab_count;
  } else if (state->tab_visual_count > state->tab_count) {
    state->tab_visual_count = state->tab_count;
  }

  if (state->tab_drag_active && state->drag_tab >= 0 &&
      state->drag_tab < state->tab_count) {
    float edge_pad = 24.0f;
    float drag_w = fadeterm_tab_width(ctx, font, state->tabs[state->drag_tab]);
    float drag_left_min = layout->tabs_scroll_x + 4.0f;
    float drag_left_max =
        layout->tabs_scroll_x + layout->tabs_scroll_w - drag_w - 4.0f;
    float drag_left = template_clamp(state->mouse_x - state->drag_offset_x,
                                     drag_left_min, drag_left_max);
    float drag_center = drag_left + drag_w * 0.5f;

    if (state->mouse_x < layout->tabs_scroll_x + edge_pad)
      fadeterm_scroll_tabs(ctx, font, layout, state, -10.0f);
    else if (state->mouse_x >
             layout->tabs_scroll_x + layout->tabs_scroll_w - edge_pad)
      fadeterm_scroll_tabs(ctx, font, layout, state, 10.0f);

    // The first slot was being weird because a wider dragged tab can hit the
    // left clamp without its center ever crossing the first tab's midpoint.
    if (state->drag_tab > 0 && drag_left <= drag_left_min + 1.0f) {
      while (state->drag_tab > 0) {
        fadeterm_swap_tabs(state, state->drag_tab, state->drag_tab - 1);
        state->drag_tab--;
        animating = true;
      }
    }

    while (state->drag_tab > 0) {
      float target_x, target_w;
      fadeterm_tab_target_rect(ctx, font, layout, state, state->drag_tab - 1,
                               &target_x, &target_w);
      if (drag_center <= target_x + target_w * 0.5f) {
        fadeterm_swap_tabs(state, state->drag_tab, state->drag_tab - 1);
        state->drag_tab--;
        animating = true;
        continue;
      }
      break;
    }

    while (state->drag_tab + 1 < state->tab_count) {
      float target_x, target_w;
      fadeterm_tab_target_rect(ctx, font, layout, state, state->drag_tab + 1,
                               &target_x, &target_w);
      if (drag_center > target_x + target_w * 0.5f) {
        fadeterm_swap_tabs(state, state->drag_tab, state->drag_tab + 1);
        state->drag_tab++;
        animating = true;
        continue;
      }
      break;
    }
  }

  for (i = 0; i < state->tab_count; ++i) {
    float target_x, target_w;
    fadeterm_tab_target_rect(ctx, font, layout, state, i, &target_x, &target_w);

    if (state->tab_drag_active && i == state->drag_tab) {
      float drag_left_min = layout->tabs_scroll_x + 4.0f;
      float drag_left_max =
          layout->tabs_scroll_x + layout->tabs_scroll_w - target_w - 4.0f;
      state->tab_visual_x[i] =
          template_clamp(state->mouse_x - state->drag_offset_x, drag_left_min,
                         drag_left_max);
      animating = true;
      continue;
    }

    if (fabsf(state->tab_visual_x[i] - target_x) <= 0.25f) {
      state->tab_visual_x[i] = target_x;
    } else {
      state->tab_visual_x[i] += (target_x - state->tab_visual_x[i]) * 0.28f;
      animating = true;
    }
  }

  return animating;
}

static bool fadeterm_add_tab(FadeTermState *state) {
  if (state->tab_count >= FADETERM_MAX_TABS)
    return false;
  snprintf(state->tabs[state->tab_count], sizeof(state->tabs[state->tab_count]),
           "session-%d", state->next_session_id++);
  fadeterm_init_session(&state->sessions[state->tab_count]);
  state->active_tab = state->tab_count;
  state->tab_count++;
  return true;
}

static void fadeterm_scroll_tabs(StygianContext *ctx, StygianFont font,
                                 const TemplateLayout *layout,
                                 FadeTermState *state, float delta) {
  state->tab_scroll += delta;
  fadeterm_clamp_scroll(ctx, font, layout, state);
}

static void template_compute_layout(TemplateLayout *layout, int width, int height,
                                    const StygianTitlebarHints *hints) {
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
  layout->tabs_x = layout->menu_x + layout->button_w + 18.0f;
  layout->tabs_h = 26.0f;
  layout->body_x = layout->shell_x + 8.0f;
  layout->body_y = layout->titlebar_h + 6.0f;
  layout->body_w = layout->shell_w - 16.0f;
  layout->body_h = layout->shell_h - layout->body_y - 8.0f;
  layout->tabs_y = layout->body_y - layout->tabs_h + 1.0f;
  layout->tabs_w = layout->minimize_x - layout->tabs_x - 10.0f;
  layout->tabs_add_w = 18.0f;
  layout->tabs_add_h = 18.0f;
  layout->tabs_add_y = layout->tabs_y + 4.0f;
  layout->tabs_scroll_x = layout->tabs_x;
  layout->tabs_scroll_w = layout->tabs_w;
  layout->viewport_x = layout->body_x + 1.0f;
  layout->viewport_y = layout->body_y + 1.0f;
  layout->viewport_w = layout->body_w - 2.0f;
  layout->viewport_h = layout->body_h - 2.0f;
  layout->input_x = layout->viewport_x + 18.0f;
  layout->input_y = layout->viewport_y + layout->viewport_h - 38.0f;
  layout->input_w = layout->viewport_w - 36.0f;
  layout->input_h = 22.0f;
}

static void fadeterm_update_pointer(FadeTermState *state,
                                    const StygianEvent *event) {
  if (event->type == STYGIAN_EVENT_MOUSE_MOVE) {
    state->mouse_x = (float)event->mouse_move.x;
    state->mouse_y = (float)event->mouse_move.y;
  } else if (event->type == STYGIAN_EVENT_MOUSE_DOWN ||
             event->type == STYGIAN_EVENT_MOUSE_UP) {
    state->mouse_x = (float)event->mouse_button.x;
    state->mouse_y = (float)event->mouse_button.y;
    state->mouse_down =
        (event->type == STYGIAN_EVENT_MOUSE_DOWN) &&
        (event->mouse_button.button == STYGIAN_MOUSE_LEFT);
  } else if (event->type == STYGIAN_EVENT_SCROLL) {
    state->mouse_x = (float)event->scroll.x;
    state->mouse_y = (float)event->scroll.y;
  }
}

static bool fadeterm_terminal_append_codepoint(FadeTermState *state,
                                               uint32_t codepoint) {
  FadeTermSession *session = fadeterm_active_session(state);
  int i;
  if (!session || codepoint < 32u || codepoint > 126u ||
      session->input_len >= FADETERM_MAX_INPUT - 1)
    return false;

  for (i = session->input_len; i > session->cursor_idx; --i)
    session->input[i] = session->input[i - 1];
  session->input[session->cursor_idx] = (char)codepoint;
  session->input_len++;
  session->cursor_idx++;
  session->input[session->input_len] = '\0';
  session->history_browse = -1;
  return true;
}

static bool fadeterm_terminal_keydown(FadeTermState *state, StygianKey key,
                                      const char *host_label) {
  FadeTermSession *session = fadeterm_active_session(state);
  int i;
  if (!state || !state->terminal_focused || !session)
    return false;

  switch (key) {
  case STYGIAN_KEY_BACKSPACE:
    if (session->cursor_idx <= 0)
      return false;
    for (i = session->cursor_idx; i <= session->input_len; ++i)
      session->input[i - 1] = session->input[i];
    session->cursor_idx--;
    session->input_len--;
    session->history_browse = -1;
    return true;
  case STYGIAN_KEY_DELETE:
    if (session->cursor_idx >= session->input_len)
      return false;
    for (i = session->cursor_idx + 1; i <= session->input_len; ++i)
      session->input[i - 1] = session->input[i];
    session->input_len--;
    session->history_browse = -1;
    return true;
  case STYGIAN_KEY_LEFT:
    if (session->cursor_idx > 0)
      session->cursor_idx--;
    return true;
  case STYGIAN_KEY_RIGHT:
    if (session->cursor_idx < session->input_len)
      session->cursor_idx++;
    return true;
  case STYGIAN_KEY_HOME:
    session->cursor_idx = 0;
    return true;
  case STYGIAN_KEY_END:
    session->cursor_idx = session->input_len;
    return true;
  case STYGIAN_KEY_UP:
    if (session->history_count <= 0)
      return false;
    if (session->history_browse < 0)
      session->history_browse = session->history_count - 1;
    else if (session->history_browse > 0)
      session->history_browse--;
    fadeterm_set_input(session, session->history[session->history_browse]);
    return true;
  case STYGIAN_KEY_DOWN:
    if (session->history_count <= 0)
      return false;
    if (session->history_browse < 0)
      return false;
    if (session->history_browse + 1 < session->history_count) {
      session->history_browse++;
      fadeterm_set_input(session, session->history[session->history_browse]);
    } else {
      session->history_browse = -1;
      fadeterm_set_input(session, "");
    }
    return true;
  case STYGIAN_KEY_ENTER:
    fadeterm_run_command(session, host_label);
    return true;
  case STYGIAN_KEY_ESCAPE:
    state->terminal_focused = false;
    return true;
  default:
    break;
  }

  return false;
}

static void fadeterm_handle_left_click(StygianWindow *window, StygianContext *ctx,
                                       StygianFont font,
                                       const TemplateLayout *layout,
                                       FadeTermState *state,
                                       const StygianEvent *event,
                                       bool *event_mutated,
                                       bool *event_requested) {
  bool in_menu =
      template_point_in_button(layout, state->mouse_x, state->mouse_y,
                               layout->menu_x);
  bool in_minimize =
      template_point_in_button(layout, state->mouse_x, state->mouse_y,
                               layout->minimize_x);
  bool in_maximize =
      template_point_in_button(layout, state->mouse_x, state->mouse_y,
                               layout->maximize_x);
  bool in_close =
      template_point_in_button(layout, state->mouse_x, state->mouse_y,
                               layout->close_x);
  bool in_add =
      fadeterm_point_in_add_button(ctx, font, layout, state, state->mouse_x,
                                   state->mouse_y);
  bool in_terminal =
      template_point_in_rect(state->mouse_x, state->mouse_y, layout->viewport_x,
                             layout->viewport_y, layout->viewport_w,
                             layout->viewport_h);
  int hovered_tab_close = fadeterm_hovered_tab_close(
      ctx, font, layout, state, state->mouse_x, state->mouse_y);
  int hovered_tab =
      fadeterm_hovered_tab(ctx, font, layout, state, state->mouse_x, state->mouse_y);
  bool in_titlebar = template_point_in_rect(state->mouse_x, state->mouse_y,
                                            layout->shell_x, layout->shell_y,
                                            layout->shell_w,
                                            layout->titlebar_h + 10.0f);

  if (in_menu) {
    // This stays a stub for now. Real menu logic can show up when the app earns it.
    state->terminal_focused = false;
    *event_requested = true;
  } else if (hovered_tab_close >= 0) {
    state->terminal_focused = false;
    if (fadeterm_close_tab(state, hovered_tab_close)) {
      fadeterm_clamp_scroll(ctx, font, layout, state);
      *event_mutated = true;
    }
    *event_requested = true;
  } else if (hovered_tab >= 0) {
    state->terminal_focused = false;
    state->active_tab = hovered_tab;
    fadeterm_reveal_tab(ctx, font, layout, state, hovered_tab);
    fadeterm_begin_tab_drag(ctx, font, layout, state, hovered_tab);
    *event_mutated = true;
    *event_requested = true;
  } else if (in_add) {
    state->terminal_focused = false;
    if (fadeterm_add_tab(state)) {
      fadeterm_reveal_tab(ctx, font, layout, state, state->active_tab);
      *event_mutated = true;
    }
    *event_requested = true;
  } else if (in_minimize) {
    state->terminal_focused = false;
    stygian_window_minimize(window);
    *event_mutated = true;
    *event_requested = true;
  } else if (in_maximize) {
    state->terminal_focused = false;
    if (stygian_window_is_maximized(window))
      stygian_window_restore(window);
    else
      stygian_window_maximize(window);
    *event_mutated = true;
    *event_requested = true;
  } else if (in_close) {
    state->terminal_focused = false;
    stygian_set_present_enabled(ctx, false);
    state->shutting_down = true;
    stygian_window_request_close(window);
    *event_mutated = true;
    *event_requested = true;
  } else if (in_terminal) {
    state->terminal_focused = true;
    *event_requested = true;
  } else if (in_titlebar) {
    state->terminal_focused = false;
    if (event->mouse_button.clicks >= 2)
      stygian_window_titlebar_double_click(window);
    else
      stygian_window_begin_system_move(window);
    *event_mutated = true;
    *event_requested = true;
  }
}

static void fadeterm_process_event(StygianWindow *window, StygianContext *ctx,
                                   StygianFont font,
                                   const TemplateLayout *layout,
                                   FadeTermState *state,
                                   const StygianEvent *event,
                                   bool *event_mutated,
                                   bool *event_requested) {
  FadeTermSession *session = fadeterm_active_session(state);
  fadeterm_update_pointer(state, event);

  if (event->type == STYGIAN_EVENT_SCROLL &&
      fadeterm_point_in_tab_strip(layout, state->mouse_x, state->mouse_y)) {
    if ((event->scroll.dx != 0.0f || event->scroll.dy != 0.0f) &&
        (stygian_get_mods(window) & STYGIAN_MOD_CTRL) && session) {
      session->zoom =
          template_clamp(session->zoom + event->scroll.dy * 0.75f, 11.0f, 22.0f);
      *event_mutated = true;
      *event_requested = true;
    } else {
      float delta = (-event->scroll.dy * 48.0f) - (event->scroll.dx * 36.0f);
      fadeterm_scroll_tabs(ctx, font, layout, state, delta);
      *event_requested = true;
    }
  } else if (event->type == STYGIAN_EVENT_SCROLL &&
             template_point_in_rect(state->mouse_x, state->mouse_y,
                                    layout->viewport_x, layout->viewport_y,
                                    layout->viewport_w, layout->viewport_h) &&
             (stygian_get_mods(window) & STYGIAN_MOD_CTRL) && session) {
    session->zoom =
        template_clamp(session->zoom + event->scroll.dy * 0.75f, 11.0f, 22.0f);
    *event_mutated = true;
    *event_requested = true;
  }

  if (event->type == STYGIAN_EVENT_MOUSE_MOVE && state->drag_tab >= 0) {
    float dx = fabsf(state->mouse_x - state->drag_start_x);
    float dy = fabsf(state->mouse_y - state->drag_start_y);
    if (!state->tab_drag_active && (dx > 5.0f || dy > 4.0f))
      state->tab_drag_active = true;
    *event_requested = true;
  }

  if (event->type == STYGIAN_EVENT_MOUSE_DOWN &&
      event->mouse_button.button == STYGIAN_MOUSE_LEFT) {
    fadeterm_handle_left_click(window, ctx, font, layout, state, event,
                               event_mutated, event_requested);
  }

  if (event->type == STYGIAN_EVENT_KEY_DOWN && !event->key.repeat) {
    if (fadeterm_terminal_keydown(state, event->key.key,
                                  session ? session->cwd : "")) {
      *event_mutated = true;
      *event_requested = true;
    }
  }

  if (event->type == STYGIAN_EVENT_CHAR) {
    if (fadeterm_terminal_append_codepoint(state, event->chr.codepoint)) {
      *event_mutated = true;
      *event_requested = true;
    }
  }

  if (event->type == STYGIAN_EVENT_MOUSE_UP &&
      event->mouse_button.button == STYGIAN_MOUSE_LEFT && state->drag_tab >= 0) {
    if (state->tab_drag_active)
      *event_mutated = true;
    fadeterm_end_tab_drag(state);
    *event_requested = true;
  }

  if (event->type == STYGIAN_EVENT_CLOSE) {
    stygian_set_present_enabled(ctx, false);
    state->shutting_down = true;
    stygian_window_request_close(window);
  }
}

static void fadeterm_draw_tab_strip(StygianContext *ctx, StygianFont font,
                                    const TemplateLayout *layout,
                                    const FadeTermState *state) {
  int i;
  int active_index = state->active_tab;
  float active_tab_x = 0.0f;
  float active_tab_w = 0.0f;
  float add_x = fadeterm_add_button_x(ctx, font, layout, state);
  float strip_r = 0.112f;
  float strip_g = 0.116f;
  float strip_b = 0.128f;
  float active_r = 0.047f;
  float active_g = 0.050f;
  float active_b = 0.058f;
  int hovered_tab = fadeterm_hovered_tab(ctx, font, layout, state, state->mouse_x,
                                         state->mouse_y);
  int hovered_tab_close = fadeterm_hovered_tab_close(
      ctx, font, layout, state, state->mouse_x, state->mouse_y);
  bool plus_hovered = fadeterm_point_in_add_button(ctx, font, layout, state,
                                                   state->mouse_x,
                                                   state->mouse_y);
  float plus_color = plus_hovered ? 0.98f : 0.88f;

  stygian_rect(ctx, layout->tabs_scroll_x, layout->tabs_y + layout->tabs_h - 1.0f,
               layout->tabs_scroll_w, 1.0f, strip_r, strip_g, strip_b, 0.95f);

  // The strip is clipped because overflowing tabs love to scribble over the
  // window controls when nobody is watching.
  stygian_clip_push(ctx, layout->tabs_scroll_x, layout->tabs_y,
                    layout->tabs_scroll_w, layout->tabs_h);
  for (i = 0; i < state->tab_count; ++i) {
    float tab_x, tab_w;
    float tab_r, tab_g, tab_b;
    float text_r, text_g, text_b, text_a;
    bool hovered = (i == hovered_tab);
    bool active = (i == state->active_tab);
    bool show_close = (i == hovered_tab || i == hovered_tab_close);
    bool close_hovered = (i == hovered_tab_close);

    fadeterm_tab_visual_rect(ctx, font, layout, state, i, &tab_x, &tab_w);
    if (tab_x + tab_w < layout->tabs_scroll_x - 2.0f ||
        tab_x > layout->tabs_scroll_x + layout->tabs_scroll_w + 2.0f)
      continue;
    if (active) {
      active_tab_x = tab_x;
      active_tab_w = tab_w;
      continue;
    }

    if (active) {
      tab_r = 0.060f;
      tab_g = 0.064f;
      tab_b = 0.074f;
      text_r = 0.96f;
      text_g = 0.97f;
      text_b = 0.99f;
      text_a = 1.0f;
    } else if (hovered) {
      tab_r = 0.104f;
      tab_g = 0.108f;
      tab_b = 0.122f;
      text_r = 0.90f;
      text_g = 0.92f;
      text_b = 0.96f;
      text_a = 0.98f;
    } else {
      tab_r = 0.095f;
      tab_g = 0.098f;
      tab_b = 0.108f;
      text_r = 0.70f;
      text_g = 0.74f;
      text_b = 0.82f;
      text_a = 0.92f;
    }

    stygian_rect_rounded(ctx, tab_x, layout->tabs_y + 3.0f, tab_w,
                         layout->tabs_h - 4.0f, tab_r, tab_g, tab_b, 1.0f,
                         5.0f);
    stygian_text(ctx, font, state->tabs[i], tab_x + 16.0f,
                 template_snap(layout->tabs_y + 6.0f), 12.5f, text_r, text_g,
                 text_b, text_a);
    if (show_close && state->tab_count > 1)
      fadeterm_draw_tab_close_cap(ctx, tab_x, tab_w, layout->tabs_y + 3.0f,
                                  tab_r, tab_g, tab_b, close_hovered);
  }

  stygian_clip_pop(ctx);

  if (active_index >= 0 && active_index < state->tab_count) {
    float text_r = 0.96f;
    float text_g = 0.97f;
    float text_b = 0.99f;
    float text_a = 1.0f;
    bool show_close = (active_index == hovered_tab || active_index == hovered_tab_close);
    bool close_hovered = (active_index == hovered_tab_close);
    if (active_tab_w <= 0.0f)
      fadeterm_tab_visual_rect(ctx, font, layout, state, active_index,
                               &active_tab_x, &active_tab_w);

    // Draw the selected tab after the client area so it can actually fuse into
    // the pane instead of getting clipped back into a capsule.
    stygian_rect_rounded(ctx, active_tab_x, layout->tabs_y + 1.0f, active_tab_w,
                         layout->body_y - layout->tabs_y + 4.0f, active_r,
                         active_g, active_b,
                         1.0f, 6.0f);
    stygian_rect_rounded(ctx, active_tab_x - 4.0f,
                         layout->body_y - 4.0f, active_tab_w + 8.0f, 12.0f,
                         active_r, active_g,
                         active_b, 1.0f, 5.0f);
    stygian_rect(ctx, active_tab_x + 12.0f, layout->tabs_y + 3.0f,
                 active_tab_w - 24.0f, 2.0f, 0.50f, 0.67f, 0.98f, 0.96f);
    stygian_rect_rounded(ctx, active_tab_x + 12.0f, layout->tabs_y + 10.0f,
                         5.0f, 5.0f, 0.54f, 0.69f, 0.98f, 0.98f, 2.5f);
    stygian_text(ctx, font, state->tabs[active_index], active_tab_x + 24.0f,
                 template_snap(layout->tabs_y + 6.0f), 12.5f, text_r, text_g,
                 text_b, text_a);
    if (show_close && state->tab_count > 1)
      fadeterm_draw_tab_close_cap(ctx, active_tab_x, active_tab_w,
                                  layout->tabs_y + 1.0f, active_r, active_g,
                                  active_b,
                                  close_hovered);
  }

  fadeterm_draw_plus_glyph(ctx, add_x, layout->tabs_add_y, layout->tabs_add_w,
                           layout->tabs_add_h, plus_color);
}

static void fadeterm_draw_terminal_body(StygianContext *ctx, StygianFont font,
                                        const TemplateLayout *layout,
                                        const FadeTermState *state) {
  const FadeTermSession *session = fadeterm_active_session_const(state);
  const char *prompt_label = "PS";
  float font_px;
  float body_r = 0.070f;
  float body_g = 0.074f;
  float body_b = 0.086f;
  float panel_r = 0.047f;
  float panel_g = 0.050f;
  float panel_b = 0.058f;
  float line_y;
  float line_h;
  float text_x = layout->viewport_x + 20.0f;
  float scroll_x = layout->viewport_x + layout->viewport_w - 8.0f;
  float scroll_y = layout->viewport_y + 14.0f;
  float scroll_h = layout->viewport_h - 28.0f;
  float prompt_y;
  float prompt_x = layout->input_x;
  float prompt_run_x;
  float input_x;
  float input_text_w = 0.0f;
  float caret_x;
  int visible_lines;
  int start_line;
  int i;
  bool caret_on = false;
  char cursor_prefix[FADETERM_MAX_INPUT];

  stygian_rect_rounded(ctx, layout->body_x, layout->body_y, layout->body_w,
                       layout->body_h, body_r, body_g, body_b, 1.0f, 12.0f);

  stygian_rect_rounded(ctx, layout->viewport_x, layout->viewport_y,
                       layout->viewport_w, layout->viewport_h, panel_r, panel_g,
                       panel_b, 1.0f, 11.0f);
  stygian_rect_rounded(ctx, scroll_x, scroll_y, 3.0f, scroll_h, 0.10f, 0.11f,
                       0.15f, 0.95f, 1.0f);
  stygian_rect_rounded(ctx, scroll_x, scroll_y + scroll_h * 0.18f, 3.0f,
                       scroll_h * 0.22f, 0.39f, 0.59f, 0.98f, 0.92f, 1.0f);

  if (!session)
    return;

  font_px = session->zoom;
  line_h = template_snap(font_px + 6.0f);
  line_y = layout->viewport_y + 18.0f;
  prompt_y = layout->viewport_y + layout->viewport_h - (font_px + 14.0f);

  visible_lines = (int)((prompt_y - layout->viewport_y - 28.0f) / line_h);
  if (visible_lines < 1)
    visible_lines = 1;
  start_line = session->transcript_count > visible_lines
                   ? session->transcript_count - visible_lines
                   : 0;

  for (i = start_line; i < session->transcript_count; ++i) {
    const char *line = session->transcript[i];
    float r = 0.80f, g = 0.84f, b = 0.92f, a = 0.95f;

    if (!line[0]) {
      line_y += line_h * 0.8f;
      continue;
    }
    if (strncmp(line, "Windows PowerShell", 18) == 0) {
      r = 0.76f;
      g = 0.82f;
      b = 0.94f;
      a = 0.94f;
    } else if (strncmp(line, "Copyright", 9) == 0) {
      r = 0.58f;
      g = 0.63f;
      b = 0.74f;
      a = 0.90f;
    } else if (strncmp(line, "PS ", 3) == 0) {
      r = 0.92f;
      g = 0.94f;
      b = 0.97f;
      a = 0.98f;
    } else if (strncmp(line, "test result:", 12) == 0) {
      r = 0.88f;
      g = 0.80f;
      b = 0.60f;
      a = 0.98f;
    }

    stygian_text(ctx, font, line, text_x, line_y, font_px, r, g, b, a);
    line_y += line_h;
  }

  stygian_rect(ctx, layout->input_x - 12.0f, prompt_y - 10.0f, layout->input_w + 24.0f,
               1.0f, 0.105f, 0.112f, 0.130f, 0.95f);
  stygian_text(ctx, font, prompt_label, prompt_x, prompt_y,
               font_px, 0.48f, 0.74f, 0.98f, 0.98f);
  prompt_run_x = prompt_x + 20.0f;
  stygian_text(ctx, font, session->cwd, prompt_run_x, prompt_y,
               font_px, 0.92f, 0.94f, 0.97f, 0.98f);
  input_x =
      prompt_run_x + stygian_text_width(ctx, font, session->cwd, font_px) + 18.0f;
  stygian_text(ctx, font, "> ", input_x, prompt_y, font_px, 0.92f, 0.94f, 0.97f,
               0.98f);
  input_x += 18.0f;
  stygian_text(ctx, font, session->input, input_x, prompt_y, font_px, 0.92f, 0.94f,
               0.97f, 0.98f);

  memset(cursor_prefix, 0, sizeof(cursor_prefix));
  if (session->cursor_idx > 0) {
    memcpy(cursor_prefix, session->input, (size_t)session->cursor_idx);
    cursor_prefix[session->cursor_idx] = '\0';
  }
  input_text_w = stygian_text_width(ctx, font, cursor_prefix, font_px);
  caret_x = input_x + input_text_w;
#ifdef _WIN32
  caret_on = state->terminal_focused &&
             (((GetTickCount64() / 530ull) & 1ull) == 0ull);
#else
  caret_on = state->terminal_focused;
#endif
  if (caret_on) {
    stygian_rect_rounded(ctx, caret_x, prompt_y - 2.0f, 2.0f, font_px + 5.0f, 0.61f,
                         0.78f, 1.0f, 0.98f, 1.0f);
  }
}
int main(void) {
  const StygianScopeId k_scope_shell = 0x5201u;
  StygianWindowConfig win_cfg = {
      .title = STYGIAN_TEMPLATE_APP_TITLE,
      .width = 1000,
      .height = 500,
      .flags = STYGIAN_TEMPLATE_WINDOW_FLAGS,
  };
  StygianConfig ctx_cfg = {
      .backend = STYGIAN_TEMPLATE_BACKEND,
      .window = NULL,
  };
  StygianWindow *window = stygian_window_create(&win_cfg);
  StygianContext *ctx;
  StygianFont font = 0;
  FadeTermState state;
  bool first_frame = true;
  char shader_dir[MAX_PATH];
  char atlas_png[MAX_PATH];
  char atlas_json[MAX_PATH];
#if defined(_WIN32) && defined(STYGIAN_DEMO_VULKAN)
  HWND native_hwnd = NULL;
  bool native_window_shown = false;
#endif

  if (!window)
    return 1;

  fadeterm_seed_tabs(&state);

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

  ctx_cfg.window = window;
  if (fadeterm_repo_path("shaders", shader_dir, sizeof(shader_dir)))
    ctx_cfg.shader_dir = shader_dir;
  if (fadeterm_repo_path("assets\\atlas.png", atlas_png, sizeof(atlas_png)))
    ctx_cfg.default_font_atlas_png = atlas_png;
  if (fadeterm_repo_path("assets\\atlas.json", atlas_json, sizeof(atlas_json)))
    ctx_cfg.default_font_atlas_json = atlas_json;

  ctx = stygian_create(&ctx_cfg);
  if (!ctx)
    return 1;
  stygian_set_vsync(ctx, true);
  font = stygian_get_default_font(ctx);

  while (!stygian_window_should_close(window)) {
    StygianEvent event;
    StygianTitlebarHints hints;
    TemplateLayout layout;
    bool event_mutated = false;
    bool event_requested = false;
    bool event_eval_requested = false;
    uint32_t wait_ms = stygian_next_repaint_wait_ms(ctx, 250u);
    int width, height;

    stygian_window_get_titlebar_hints(window, &hints);
    stygian_window_get_size(window, &width, &height);
    template_compute_layout(&layout, width, height, &hints);
    fadeterm_clamp_scroll(ctx, font, &layout, &state);

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

      fadeterm_process_event(window, ctx, font, &layout, &state, &event,
                             &event_mutated, &event_requested);
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

        fadeterm_process_event(window, ctx, font, &layout, &state, &event,
                               &event_mutated, &event_requested);
      }
    }

    if (fadeterm_update_tab_animation(ctx, font, &layout, &state))
      event_requested = true;

    if (state.shutting_down && stygian_window_should_close(window))
      break;

    {
      bool hovered_menu_now =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.menu_x);
      bool hovered_minimize_now =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.minimize_x);
      bool hovered_maximize_now =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.maximize_x);
      bool hovered_close_now =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.close_x);
      bool hovered_add_now =
          fadeterm_point_in_add_button(ctx, font, &layout, &state, state.mouse_x,
                                       state.mouse_y);
      int hovered_tab_now =
          fadeterm_hovered_tab(ctx, font, &layout, &state, state.mouse_x,
                               state.mouse_y);
      int hovered_tab_close_now =
          fadeterm_hovered_tab_close(ctx, font, &layout, &state, state.mouse_x,
                                     state.mouse_y);

      if (!state.hover_state_valid || hovered_menu_now != state.hovered_menu_prev ||
          hovered_minimize_now != state.hovered_minimize_prev ||
          hovered_maximize_now != state.hovered_maximize_prev ||
          hovered_close_now != state.hovered_close_prev ||
          hovered_add_now != state.hovered_add_prev ||
          hovered_tab_now != state.hovered_tab_prev ||
          hovered_tab_close_now != state.hovered_tab_close_prev) {
        state.hovered_menu_prev = hovered_menu_now;
        state.hovered_minimize_prev = hovered_minimize_now;
        state.hovered_maximize_prev = hovered_maximize_now;
        state.hovered_close_prev = hovered_close_now;
        state.hovered_add_prev = hovered_add_now;
        state.hovered_tab_prev = hovered_tab_now;
        state.hovered_tab_close_prev = hovered_tab_close_now;
        state.hover_state_valid = true;
        event_requested = true;
      }
    }

    if (!first_frame && !event_mutated && !event_requested &&
        !event_eval_requested && !stygian_has_pending_repaint(ctx)) {
      stygian_widgets_commit_regions();
      stygian_end_frame(ctx);
      continue;
    }
    first_frame = false;

    stygian_window_get_size(window, &width, &height);
    template_compute_layout(&layout, width, height, &hints);
    fadeterm_clamp_scroll(ctx, font, &layout, &state);

    stygian_begin_frame_intent(
        ctx, width, height,
        (event_eval_requested && !event_requested && !event_mutated &&
         !stygian_has_pending_repaint(ctx))
            ? STYGIAN_FRAME_EVAL_ONLY
            : STYGIAN_FRAME_RENDER);

    stygian_scope_begin(ctx, k_scope_shell);
    stygian_rect(ctx, 0.0f, 0.0f, (float)width, (float)height, 0.15f, 0.15f,
                 0.16f, 1.0f);
    // Draw slightly past the window bounds so the rounded-edge AA gets clipped
    // off instead of leaving a pale seam around the shell.
    stygian_rect_rounded(ctx, layout.shell_x - 2.0f, layout.shell_y - 2.0f,
                         layout.shell_w + 4.0f, layout.shell_h + 4.0f, 0.15f,
                         0.15f, 0.16f, 1.0f, 8.0f);
    {
      bool hovered_menu =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.menu_x);
      bool hovered_minimize =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.minimize_x);
      bool hovered_maximize =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.maximize_x);
      bool hovered_close =
          template_point_in_button(&layout, state.mouse_x, state.mouse_y,
                                   layout.close_x);
      bool maximized = stygian_window_is_maximized(window);
      float base_icon = 0.94f;
      float menu_icon = hovered_menu ? 0.995f : 0.95f;
      float minimize_icon = hovered_minimize ? 0.995f : base_icon;
      float maximize_icon = hovered_maximize ? 0.995f : base_icon;
      float close_icon = hovered_close ? 1.0f : 0.965f;
      float max_r, max_g, max_b;
      template_button_fill(hovered_maximize,
                           hovered_maximize && state.mouse_down, false, &max_r,
                           &max_g, &max_b);
      template_draw_caption_button_bg(ctx, &layout, layout.menu_x, hovered_menu,
                                      hovered_menu && state.mouse_down, false);
      template_draw_menu_glyph(ctx, layout.menu_x, layout.button_y,
                               layout.button_w, layout.button_h, menu_icon);
      fadeterm_draw_terminal_body(ctx, font, &layout, &state);
      fadeterm_draw_tab_strip(ctx, font, &layout, &state);
      template_draw_caption_button_bg(ctx, &layout, layout.minimize_x,
                                      hovered_minimize,
                                      hovered_minimize && state.mouse_down,
                                      false);
      template_draw_minimize_glyph(ctx, layout.minimize_x, layout.button_y,
                                   layout.button_w, layout.button_h,
                                   minimize_icon);
      template_draw_caption_button_bg(ctx, &layout, layout.maximize_x,
                                      hovered_maximize,
                                      hovered_maximize && state.mouse_down,
                                      false);
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
                                      hovered_close && state.mouse_down, true);
      template_draw_close_glyph(ctx, layout.close_x, layout.button_y,
                                layout.button_w, layout.button_h, close_icon);
    }
    stygian_scope_end(ctx);

    // This shell still changes on hover, press, and tab focus. Caching it
    // forever was the thing that made the caption controls look dead.
    stygian_scope_invalidate_next(ctx, k_scope_shell);
    if (event_requested || event_mutated) {
      stygian_set_repaint_source(ctx, "fadeterm");
      stygian_request_repaint_after_ms(ctx, 0u);
    } else if (state.terminal_focused) {
      stygian_request_repaint_after_ms(ctx, 530u);
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
