#include "../../../../include/stygian.h"
#include "../../../../widgets/stygian_widgets.h"
#include "../../../../window/stygian_input.h"
#include "../../../../window/stygian_window.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef STYGIAN_TEMPLATE_APP_TITLE
#define STYGIAN_TEMPLATE_APP_TITLE "FileNight"
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

#define FILENIGHT_SHELL_R 0.030f
#define FILENIGHT_SHELL_G 0.030f
#define FILENIGHT_SHELL_B 0.032f
#define FILENIGHT_CHROME_R 0.090f
#define FILENIGHT_CHROME_G 0.090f
#define FILENIGHT_CHROME_B 0.095f
#define FILENIGHT_TEXT_R 0.90f
#define FILENIGHT_TEXT_G 0.90f
#define FILENIGHT_TEXT_B 0.92f
#define FILENIGHT_MAX_TABS 8
#define FILENIGHT_MAX_ROWS 256
#define FILENIGHT_MAX_PATH 512
#define FILENIGHT_MAX_LABEL 64
#define FILENIGHT_MAX_TYPE 48
#define FILENIGHT_MAX_TIME 40
#define FILENIGHT_MAX_SIZE 24
#define FILENIGHT_MAX_SIDEBAR_ITEMS 16

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
  float tabs_add_w;
  float tabs_add_h;
  float tabs_add_y;
  float body_x;
  float body_y;
  float body_w;
  float body_h;
  float sidebar_x;
  float sidebar_y;
  float sidebar_w;
  float sidebar_h;
  float divider_x;
  float divider_y;
  float divider_w;
  float divider_h;
  float content_x;
  float content_y;
  float content_w;
  float content_h;
} TemplateLayout;

typedef enum FileNightSidebarIcon {
  FILENIGHT_ICON_DESKTOP,
  FILENIGHT_ICON_DOCUMENTS,
  FILENIGHT_ICON_DOWNLOADS,
  FILENIGHT_ICON_FOLDER,
  FILENIGHT_ICON_BOOKMARK,
  FILENIGHT_ICON_DRIVE,
  FILENIGHT_ICON_NETWORK,
} FileNightSidebarIcon;

typedef struct FileNightRow {
  char name[MAX_PATH];
  char type[FILENIGHT_MAX_TYPE];
  char modified[FILENIGHT_MAX_TIME];
  char size[FILENIGHT_MAX_SIZE];
  char path[FILENIGHT_MAX_PATH];
  bool is_directory;
  bool is_drive;
  FileNightSidebarIcon icon;
} FileNightRow;

typedef struct FileNightPanelState {
  char path[FILENIGHT_MAX_PATH];
  char title[32];
  FileNightRow rows[FILENIGHT_MAX_ROWS];
  int row_count;
  int selected_row;
  bool is_this_pc;
} FileNightPanelState;

typedef struct FileNightSidebarItem {
  char label[FILENIGHT_MAX_LABEL];
  char path[FILENIGHT_MAX_PATH];
  FileNightSidebarIcon icon;
  bool is_this_pc;
} FileNightSidebarItem;

typedef struct FileNightSidebarState {
  FileNightSidebarItem study[FILENIGHT_MAX_SIDEBAR_ITEMS];
  int study_count;
  FileNightSidebarItem recent[FILENIGHT_MAX_SIDEBAR_ITEMS];
  int recent_count;
  FileNightSidebarItem storage[FILENIGHT_MAX_SIDEBAR_ITEMS];
  int storage_count;
  FileNightSidebarItem places[FILENIGHT_MAX_SIDEBAR_ITEMS];
  int places_count;
  char current_dir[FILENIGHT_MAX_PATH];
  char user_profile[FILENIGHT_MAX_PATH];
} FileNightSidebarState;

static bool template_point_in_rect(float px, float py, float x, float y, float w,
                                   float h) {
  return px >= x && px < (x + w) && py >= y && py < (y + h);
}

static bool template_point_in_button(const TemplateLayout *layout, float px,
                                     float py, float x) {
  return template_point_in_rect(px, py, x, layout->button_y, layout->button_w,
                                layout->button_h);
}

static float template_button_tint(bool hovered, bool pressed) {
  if (pressed)
    return 0.30f;
  if (hovered)
    return 0.22f;
  return 0.14f;
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
    *out_r = FILENIGHT_CHROME_R + bg * 0.18f;
    *out_g = FILENIGHT_CHROME_G + bg * 0.20f;
    *out_b = FILENIGHT_CHROME_B + bg * 0.22f;
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

static void filenight_copy_str(char *dst, size_t dst_cap, const char *src) {
  if (!dst || dst_cap == 0)
    return;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  snprintf(dst, dst_cap, "%s", src);
}

static bool filenight_path_exists(const char *path) {
#ifdef _WIN32
  DWORD attrs;
  if (!path || !path[0])
    return false;
  attrs = GetFileAttributesA(path);
  return attrs != INVALID_FILE_ATTRIBUTES;
#else
  (void)path;
  return false;
#endif
}

static bool filenight_path_is_directory(const char *path) {
#ifdef _WIN32
  DWORD attrs;
  if (!path || !path[0])
    return false;
  attrs = GetFileAttributesA(path);
  return attrs != INVALID_FILE_ATTRIBUTES &&
         (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
  (void)path;
  return false;
#endif
}

static bool filenight_path_is_root(const char *path) {
  if (!path)
    return false;
  return strlen(path) == 3 && path[1] == ':' &&
         (path[2] == '\\' || path[2] == '/');
}

static void filenight_path_join(char *dst, size_t dst_cap, const char *base,
                                const char *name) {
  size_t len;
  if (!dst || dst_cap == 0)
    return;
  if (!base || !base[0]) {
    filenight_copy_str(dst, dst_cap, name);
    return;
  }
  len = strlen(base);
  if (len > 0 && (base[len - 1] == '\\' || base[len - 1] == '/'))
    snprintf(dst, dst_cap, "%s%s", base, name);
  else
    snprintf(dst, dst_cap, "%s\\%s", base, name);
}

static const char *filenight_path_leaf(const char *path) {
  const char *slash;
  if (!path || !path[0])
    return "";
  if (filenight_path_is_root(path))
    return path;
  slash = strrchr(path, '\\');
  if (!slash)
    slash = strrchr(path, '/');
  return slash ? slash + 1 : path;
}

static void filenight_drive_tag(const char *root, char *dst, size_t dst_cap) {
  if (!dst || dst_cap == 0)
    return;
  if (!root || strlen(root) < 2) {
    dst[0] = '\0';
    return;
  }
  snprintf(dst, dst_cap, "%c:", root[0]);
}

static void filenight_drive_display_name(const char *root, char *dst,
                                         size_t dst_cap) {
#ifdef _WIN32
  char volume[MAX_PATH];
  char drive[8];
  if (!dst || dst_cap == 0)
    return;
  filenight_drive_tag(root, drive, sizeof(drive));
  volume[0] = '\0';
  if (root && GetVolumeInformationA(root, volume, (DWORD)sizeof(volume), NULL,
                                    NULL, NULL, NULL, 0) &&
      volume[0]) {
    snprintf(dst, dst_cap, "%s (%s)", volume, drive);
    return;
  }
  snprintf(dst, dst_cap, "Local Disk (%s)", drive);
#else
  filenight_copy_str(dst, dst_cap, root);
#endif
}

static void filenight_current_directory(char *dst, size_t dst_cap) {
#ifdef _WIN32
  DWORD len;
  if (!dst || dst_cap == 0)
    return;
  len = GetCurrentDirectoryA((DWORD)dst_cap, dst);
  if (len == 0 || len >= dst_cap)
    filenight_copy_str(dst, dst_cap, "C:\\");
#else
  filenight_copy_str(dst, dst_cap, ".");
#endif
}

static void filenight_user_profile(char *dst, size_t dst_cap) {
#ifdef _WIN32
  DWORD len;
  if (!dst || dst_cap == 0)
    return;
  len = GetEnvironmentVariableA("USERPROFILE", dst, (DWORD)dst_cap);
  if (len == 0 || len >= dst_cap)
    filenight_current_directory(dst, dst_cap);
#else
  filenight_current_directory(dst, dst_cap);
#endif
}

static void filenight_format_file_size(unsigned long long size, bool is_directory,
                                       char *dst, size_t dst_cap) {
  const char *units[] = {"B", "KB", "MB", "GB", "TB"};
  double value = (double)size;
  int unit = 0;
  if (!dst || dst_cap == 0)
    return;
  if (is_directory) {
    filenight_copy_str(dst, dst_cap, "--");
    return;
  }
  while (value >= 1024.0 && unit < 4) {
    value /= 1024.0;
    ++unit;
  }
  if (unit == 0)
    snprintf(dst, dst_cap, "%llu %s", size, units[unit]);
  else if (value >= 100.0)
    snprintf(dst, dst_cap, "%.0f %s", value, units[unit]);
  else if (value >= 10.0)
    snprintf(dst, dst_cap, "%.1f %s", value, units[unit]);
  else
    snprintf(dst, dst_cap, "%.2f %s", value, units[unit]);
}

static void filenight_format_file_time(const FILETIME *ft, char *dst,
                                       size_t dst_cap) {
#ifdef _WIN32
  FILETIME local_ft;
  SYSTEMTIME st;
  if (!dst || dst_cap == 0)
    return;
  if (!ft || !FileTimeToLocalFileTime(ft, &local_ft) ||
      !FileTimeToSystemTime(&local_ft, &st)) {
    filenight_copy_str(dst, dst_cap, "--");
    return;
  }
  snprintf(dst, dst_cap, "%u/%u/%04u %02u:%02u", st.wMonth, st.wDay,
           st.wYear, st.wHour, st.wMinute);
#else
  (void)ft;
  filenight_copy_str(dst, dst_cap, "--");
#endif
}

static void filenight_file_type_for_name(const char *name, bool is_directory,
                                         bool is_drive, char *dst,
                                         size_t dst_cap) {
  const char *ext;
  if (!dst || dst_cap == 0)
    return;
  if (is_drive) {
    filenight_copy_str(dst, dst_cap, "Drive");
    return;
  }
  if (is_directory) {
    filenight_copy_str(dst, dst_cap, "Folder");
    return;
  }
  ext = name ? strrchr(name, '.') : NULL;
  if (!ext || !ext[1]) {
    filenight_copy_str(dst, dst_cap, "File");
    return;
  }
  if (_stricmp(ext, ".txt") == 0)
    filenight_copy_str(dst, dst_cap, "Text Document");
  else if (_stricmp(ext, ".md") == 0)
    filenight_copy_str(dst, dst_cap, "Markdown File");
  else if (_stricmp(ext, ".exe") == 0)
    filenight_copy_str(dst, dst_cap, "Application");
  else if (_stricmp(ext, ".png") == 0 || _stricmp(ext, ".jpg") == 0 ||
           _stricmp(ext, ".jpeg") == 0 || _stricmp(ext, ".webp") == 0)
    filenight_copy_str(dst, dst_cap, "Image File");
  else if (_stricmp(ext, ".mp4") == 0 || _stricmp(ext, ".mkv") == 0 ||
           _stricmp(ext, ".mov") == 0)
    filenight_copy_str(dst, dst_cap, "Video File");
  else
    snprintf(dst, dst_cap, "%s File", ext + 1);
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

static void template_draw_title(StygianContext *ctx, StygianFont font,
                                const TemplateLayout *layout,
                                const char *title) {
  float text_size = 14.0f;
  float left_bound;
  float right_bound;
  float avail_w;
  float title_w;
  float title_x;
  float title_y;

  if (!ctx || !font || !layout || !title || !title[0])
    return;

  left_bound = layout->menu_x + layout->button_w + 18.0f;
  right_bound = layout->minimize_x - 18.0f;
  avail_w = right_bound - left_bound;
  if (avail_w <= 24.0f)
    return;

  title_w = stygian_text_width(ctx, font, title, text_size);
  title_x = template_clamp(layout->shell_x + (layout->shell_w - title_w) * 0.5f,
                           left_bound, right_bound - title_w);
  title_y = template_snap(layout->button_y + 5.0f);
  stygian_text(ctx, font, title, title_x, title_y, text_size, FILENIGHT_TEXT_R,
               FILENIGHT_TEXT_G, FILENIGHT_TEXT_B, 0.98f);
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

static void filenight_draw_sidebar_icon(StygianContext *ctx, float x, float y,
                                        FileNightSidebarIcon icon,
                                        float scale) {
  float s = scale > 0.0f ? scale : 1.0f;
  switch (icon) {
  case FILENIGHT_ICON_DESKTOP:
    stygian_rect_rounded(ctx, x + 1.0f * s, y + 1.0f * s, 14.0f * s, 10.0f * s,
                         0.66f, 0.66f, 0.70f, 0.98f, 2.5f * s);
    stygian_rect(ctx, x + 6.0f * s, y + 12.0f * s, 4.0f * s, 2.0f * s, 0.84f,
                 0.84f, 0.87f,
                 0.98f);
    stygian_rect(ctx, x + 4.0f * s, y + 14.0f * s, 8.0f * s, 1.5f * s, 0.46f,
                 0.46f, 0.49f,
                 0.96f);
    break;
  case FILENIGHT_ICON_DOCUMENTS:
    stygian_rect_rounded(ctx, x + 2.0f * s, y + 1.0f * s, 11.0f * s, 14.0f * s,
                         0.87f, 0.87f, 0.90f, 0.98f, 2.2f * s);
    stygian_rect(ctx, x + 5.0f * s, y + 5.0f * s, 6.0f * s, 1.0f * s, 0.50f,
                 0.50f, 0.54f,
                 0.96f);
    stygian_rect(ctx, x + 5.0f * s, y + 8.0f * s, 6.0f * s, 1.0f * s, 0.50f,
                 0.50f, 0.54f,
                 0.96f);
    stygian_rect(ctx, x + 5.0f * s, y + 11.0f * s, 4.0f * s, 1.0f * s, 0.50f,
                 0.50f, 0.54f,
                 0.96f);
    break;
  case FILENIGHT_ICON_DOWNLOADS:
    stygian_rect_rounded(ctx, x + 1.0f * s, y + 1.0f * s, 14.0f * s, 10.0f * s,
                         0.66f, 0.66f, 0.69f, 0.96f, 2.5f * s);
    stygian_rect(ctx, x + 7.0f * s, y + 4.0f * s, 2.0f * s, 6.0f * s, 0.90f,
                 0.90f, 0.93f,
                 0.98f);
    stygian_rect(ctx, x + 5.0f * s, y + 9.0f * s, 6.0f * s, 2.0f * s, 0.90f,
                 0.90f, 0.93f,
                 0.98f);
    stygian_rect(ctx, x + 4.0f * s, y + 12.0f * s, 8.0f * s, 1.5f * s, 0.46f,
                 0.46f, 0.50f,
                 0.96f);
    break;
  case FILENIGHT_ICON_FOLDER:
    stygian_rect_rounded(ctx, x + 3.0f * s, y + 4.0f * s, 13.0f * s, 9.0f * s,
                         0.68f, 0.68f, 0.72f, 0.96f, 2.4f * s);
    stygian_rect_rounded(ctx, x, y + 1.0f * s, 7.0f * s, 4.0f * s, 0.80f, 0.80f,
                         0.84f, 0.98f, 2.2f * s);
    break;
  case FILENIGHT_ICON_BOOKMARK:
    stygian_rect_rounded(ctx, x + 3.0f * s, y + 1.0f * s, 10.0f * s, 14.0f * s,
                         0.82f, 0.82f, 0.86f, 0.98f, 1.8f * s);
    stygian_rect(ctx, x + 5.0f * s, y + 10.0f * s, 6.0f * s, 4.0f * s, 0.82f,
                 0.82f, 0.86f,
                 0.98f);
    break;
  case FILENIGHT_ICON_DRIVE:
    stygian_rect_rounded(ctx, x + 1.0f * s, y + 4.0f * s, 14.0f * s, 8.0f * s,
                         0.64f, 0.64f, 0.68f, 0.98f, 3.0f * s);
    stygian_rect(ctx, x + 3.0f * s, y + 7.0f * s, 10.0f * s, 1.0f * s, 0.42f,
                 0.42f, 0.46f,
                 0.96f);
    stygian_rect_rounded(ctx, x + 12.0f * s, y + 9.0f * s, 2.0f * s, 2.0f * s,
                         0.88f, 0.88f, 0.91f, 0.98f, 1.0f * s);
    break;
  case FILENIGHT_ICON_NETWORK:
    stygian_rect_rounded(ctx, x + 1.0f * s, y + 2.0f * s, 4.0f * s, 4.0f * s,
                         0.78f, 0.78f, 0.82f, 0.98f, 2.0f * s);
    stygian_rect_rounded(ctx, x + 11.0f * s, y + 2.0f * s, 4.0f * s, 4.0f * s,
                         0.78f, 0.78f, 0.82f, 0.98f, 2.0f * s);
    stygian_rect_rounded(ctx, x + 6.0f * s, y + 10.0f * s, 4.0f * s, 4.0f * s,
                         0.78f, 0.78f, 0.82f, 0.98f, 2.0f * s);
    stygian_rect(ctx, x + 4.0f * s, y + 4.0f * s, 8.0f * s, 1.2f * s, 0.48f, 0.48f, 0.52f,
                 0.96f);
    stygian_rect(ctx, x + 5.0f * s, y + 6.0f * s, 1.2f * s, 5.0f * s, 0.48f, 0.48f, 0.52f,
                 0.96f);
    stygian_rect(ctx, x + 10.0f * s, y + 6.0f * s, 1.2f * s, 5.0f * s, 0.48f, 0.48f, 0.52f,
                 0.96f);
    break;
  }
}

static void filenight_draw_section_chevron(StygianContext *ctx, float x, float y,
                                           bool expanded, float scale) {
  float s = scale > 0.0f ? scale : 1.0f;
  if (expanded) {
    stygian_rect_rounded(ctx, x + 0.8f * s, y + 3.0f * s, 2.4f * s, 2.4f * s,
                         0.82f, 0.82f, 0.85f, 1.0f, 1.2f * s);
    stygian_rect_rounded(ctx, x + 3.4f * s, y + 5.5f * s, 2.4f * s, 2.4f * s,
                         0.82f, 0.82f, 0.85f, 1.0f, 1.2f * s);
    stygian_rect_rounded(ctx, x + 6.0f * s, y + 3.0f * s, 2.4f * s, 2.4f * s,
                         0.82f, 0.82f, 0.85f, 1.0f, 1.2f * s);
  } else {
    stygian_rect_rounded(ctx, x + 2.0f * s, y + 1.4f * s, 2.4f * s, 2.4f * s,
                         0.82f, 0.82f, 0.85f, 1.0f, 1.2f * s);
    stygian_rect_rounded(ctx, x + 4.6f * s, y + 4.0f * s, 2.4f * s, 2.4f * s,
                         0.82f, 0.82f, 0.85f, 1.0f, 1.2f * s);
    stygian_rect_rounded(ctx, x + 2.0f * s, y + 6.6f * s, 2.4f * s, 2.4f * s,
                         0.82f, 0.82f, 0.85f, 1.0f, 1.2f * s);
  }
}

static float filenight_section_header_height(float scale) {
  return template_snap(20.0f * scale);
}

static float filenight_clamp_sidebar_width(float shell_w, float sidebar_w) {
  float min_w = 180.0f;
  float max_w = shell_w * 0.42f;
  if (max_w < min_w)
    max_w = min_w;
  return template_clamp(sidebar_w, min_w, max_w);
}

static float filenight_panel_tab_width(StygianContext *ctx, StygianFont font,
                                       const char *label) {
  float w = 86.0f;
  if (ctx && font && label && label[0])
    w = stygian_text_width(ctx, font, label, 13.0f) + 54.0f;
  return template_clamp(w, 92.0f, 180.0f);
}

static bool filenight_point_in_tab_strip(const TemplateLayout *layout, float px,
                                         float py) {
  return template_point_in_rect(px, py, layout->tabs_x, layout->tabs_y,
                                layout->tabs_w, layout->tabs_h);
}

static float filenight_tab_total_width(StygianContext *ctx, StygianFont font,
                                       const char tabs[][32], int tab_count) {
  float total = 4.0f;
  int i;
  for (i = 0; i < tab_count; ++i)
    total += filenight_panel_tab_width(ctx, font, tabs[i]) + 6.0f;
  return total;
}

static float filenight_tab_scroll_max(StygianContext *ctx, StygianFont font,
                                      const TemplateLayout *layout,
                                      const char tabs[][32], int tab_count) {
  float overflow =
      filenight_tab_total_width(ctx, font, tabs, tab_count) -
      (layout->tabs_w - layout->tabs_add_w - 10.0f);
  return overflow > 0.0f ? overflow : 0.0f;
}

static float filenight_clamp_tab_scroll(StygianContext *ctx, StygianFont font,
                                        const TemplateLayout *layout,
                                        const char tabs[][32], int tab_count,
                                        float tab_scroll) {
  return template_clamp(tab_scroll, 0.0f,
                        filenight_tab_scroll_max(ctx, font, layout, tabs,
                                                 tab_count));
}

static void filenight_tab_target_rect(StygianContext *ctx, StygianFont font,
                                      const TemplateLayout *layout,
                                      const char tabs[][32], int tab_count,
                                      float tab_scroll, int index,
                                      float *out_x, float *out_w);

static void filenight_reveal_tab(StygianContext *ctx, StygianFont font,
                                 const TemplateLayout *layout,
                                 const char tabs[][32], int tab_count,
                                 float *tab_scroll, int index) {
  float tab_x, tab_w;
  float visible_left = layout->tabs_x + 6.0f;
  float visible_right =
      layout->tabs_x + layout->tabs_w - layout->tabs_add_w - 8.0f;
  if (!tab_scroll || index < 0 || index >= tab_count)
    return;
  filenight_tab_target_rect(ctx, font, layout, tabs, tab_count, *tab_scroll,
                            index, &tab_x, &tab_w);
  if (tab_x < visible_left)
    *tab_scroll -= visible_left - tab_x;
  else if (tab_x + tab_w > visible_right)
    *tab_scroll += (tab_x + tab_w) - visible_right;
  *tab_scroll =
      filenight_clamp_tab_scroll(ctx, font, layout, tabs, tab_count, *tab_scroll);
}

static int filenight_row_at(const TemplateLayout *layout, float panel_zoom,
                            float px, float py, int row_count) {
  float panel_x = layout->content_x + 14.0f;
  float panel_y = layout->body_y + 2.0f;
  float panel_h = layout->content_h - 14.0f;
  float panel_body_y = layout->body_y + 1.0f;
  float zoom_scale = panel_zoom / 12.5f;
  float path_y = panel_body_y + 10.0f;
  float path_h = template_snap(30.0f * zoom_scale);
  float header_y = path_y + path_h + template_snap(8.0f * zoom_scale);
  float header_h = template_snap(28.0f * zoom_scale);
  float rows_y = header_y + header_h + template_snap(4.0f * zoom_scale);
  float row_h = template_snap(30.0f * zoom_scale);
  float footer_h = template_snap(28.0f * zoom_scale);
  float footer_y = panel_y + panel_h - footer_h;
  int row;
  if (!template_point_in_rect(px, py, panel_x + 10.0f, rows_y,
                              layout->content_w - 48.0f, footer_y - rows_y))
    return -1;
  row = (int)((py - rows_y) / row_h);
  if (row < 0 || row >= row_count)
    return -1;
  return row;
}

static bool filenight_visual_tab_close_rect(float tab_x, float tab_w, float tab_y,
                                            float *out_x, float *out_y,
                                            float *out_w, float *out_h) {
  if (tab_w < 98.0f)
    return false;
  if (out_x)
    *out_x = template_snap(tab_x + tab_w - 24.0f);
  if (out_y)
    *out_y = template_snap(tab_y + 5.0f);
  if (out_w)
    *out_w = 14.0f;
  if (out_h)
    *out_h = 14.0f;
  return true;
}

static bool filenight_panel_add_rect(StygianContext *ctx, StygianFont font,
                                     const TemplateLayout *layout,
                                     const char tabs[][32],
                                     const float tab_visual_x[], int tab_count,
                                     float *out_x, float *out_y, float *out_w,
                                     float *out_h) {
  float x;
  float y;
  float w;
  float h;

  if (!layout || !tabs)
    return false;

  x = layout->tabs_x + 4.0f;
  y = layout->tabs_add_y;
  w = 18.0f;
  h = 18.0f;
  if (tab_count > 0) {
    int last = tab_count - 1;
    float last_w = filenight_panel_tab_width(ctx, font, tabs[last]);
    float last_x = tab_visual_x ? tab_visual_x[last] : x;
    float visible_right =
        layout->tabs_x + layout->tabs_w - layout->tabs_add_w - 4.0f;
    x = template_clamp(last_x + last_w + 6.0f, layout->tabs_x + 4.0f,
                       visible_right);
  }

  if (out_x)
    *out_x = x;
  if (out_y)
    *out_y = y;
  if (out_w)
    *out_w = w;
  if (out_h)
    *out_h = h;
  return true;
}

static FileNightSidebarItem *filenight_sidebar_items(FileNightSidebarState *sidebar,
                                                     int section, int *count) {
  if (!sidebar)
    return NULL;
  switch (section) {
  case 0:
    if (count)
      *count = sidebar->study_count;
    return sidebar->study;
  case 1:
    if (count)
      *count = sidebar->recent_count;
    return sidebar->recent;
  case 2:
    if (count)
      *count = sidebar->storage_count;
    return sidebar->storage;
  case 3:
    if (count)
      *count = sidebar->places_count;
    return sidebar->places;
  default:
    if (count)
      *count = 0;
    return NULL;
  }
}

static const FileNightSidebarItem *
filenight_sidebar_items_const(const FileNightSidebarState *sidebar, int section,
                              int *count) {
  return (const FileNightSidebarItem *)filenight_sidebar_items(
      (FileNightSidebarState *)sidebar, section, count);
}

static bool filenight_sidebar_push(FileNightSidebarItem *items, int *count,
                                   const char *label, const char *path,
                                   FileNightSidebarIcon icon, bool is_this_pc) {
  int slot = count ? *count : 0;
  if (!items || !count || slot < 0 || slot >= FILENIGHT_MAX_SIDEBAR_ITEMS)
    return false;
  filenight_copy_str(items[slot].label, sizeof(items[slot].label), label);
  filenight_copy_str(items[slot].path, sizeof(items[slot].path), path);
  items[slot].icon = icon;
  items[slot].is_this_pc = is_this_pc;
  ++(*count);
  return true;
}

static int filenight_section_item_count(const FileNightSidebarState *sidebar,
                                        int index) {
  int count = 0;
  filenight_sidebar_items_const(sidebar, index, &count);
  return count;
}

static float filenight_section_body_height(const FileNightSidebarState *sidebar,
                                           int index, float scale) {
  int item_count = filenight_section_item_count(sidebar, index);
  if (item_count <= 0)
    return 0.0f;
  return template_snap(item_count * 20.0f * scale);
}

static float filenight_section_top(const TemplateLayout *layout,
                                   const float section_open[4],
                                   const FileNightSidebarState *sidebar,
                                   int index, float scale) {
  float y = layout->sidebar_y + template_snap(18.0f * scale);
  int i;
  for (i = 0; i < index; ++i) {
    float open = section_open ? section_open[i] : 1.0f;
    y += filenight_section_header_height(scale) + template_snap(8.0f * scale);
    y += filenight_section_body_height(sidebar, i, scale) * open;
    y += template_snap(10.0f * scale) + template_snap(12.0f * scale) * open;
  }
  return y;
}

static bool filenight_section_header_rect(const TemplateLayout *layout,
                                          const float section_open[4],
                                          const FileNightSidebarState *sidebar,
                                          int index, float scale, float *out_x,
                                          float *out_y, float *out_w,
                                          float *out_h) {
  float y;
  if (!layout)
    return false;
  y = filenight_section_top(layout, section_open, sidebar, index, scale);
  if (out_x)
    *out_x = layout->sidebar_x + template_snap(12.0f * scale);
  if (out_y)
    *out_y = y;
  if (out_w)
    *out_w = layout->sidebar_w - template_snap(24.0f * scale);
  if (out_h)
    *out_h = filenight_section_header_height(scale);
  return true;
}

static int filenight_row_compare(const void *a, const void *b) {
  const FileNightRow *ra = (const FileNightRow *)a;
  const FileNightRow *rb = (const FileNightRow *)b;
  if (ra->is_directory != rb->is_directory)
    return ra->is_directory ? -1 : 1;
  return _stricmp(ra->name, rb->name);
}

static void filenight_panel_update_title(FileNightPanelState *panel,
                                         const FileNightSidebarState *sidebar) {
  if (!panel)
    return;
  if (panel->is_this_pc) {
    filenight_copy_str(panel->title, sizeof(panel->title), "This PC");
    return;
  }
  if (sidebar && sidebar->user_profile[0] &&
      _stricmp(panel->path, sidebar->user_profile) == 0) {
    filenight_copy_str(panel->title, sizeof(panel->title), "Night");
    return;
  }
  if (filenight_path_is_root(panel->path)) {
    char drive_name[32];
    filenight_drive_tag(panel->path, drive_name, sizeof(drive_name));
    filenight_copy_str(panel->title, sizeof(panel->title), drive_name);
    return;
  }
  filenight_copy_str(panel->title, sizeof(panel->title),
                     filenight_path_leaf(panel->path));
}

static void filenight_panel_load_this_pc(FileNightPanelState *panel,
                                         const FileNightSidebarState *sidebar) {
#ifdef _WIN32
  char drives[512];
  DWORD len;
  char *it;
  if (!panel)
    return;
  panel->row_count = 0;
  panel->selected_row = -1;
  panel->is_this_pc = true;
  panel->path[0] = '\0';
  len = GetLogicalDriveStringsA((DWORD)sizeof(drives), drives);
  if (len == 0 || len >= sizeof(drives)) {
    filenight_panel_update_title(panel, sidebar);
    return;
  }
  for (it = drives; *it && panel->row_count < FILENIGHT_MAX_ROWS;
       it += strlen(it) + 1) {
    FileNightRow *row = &panel->rows[panel->row_count++];
    filenight_drive_display_name(it, row->name, sizeof(row->name));
    filenight_copy_str(row->path, sizeof(row->path), it);
    filenight_file_type_for_name(row->name, true, true, row->type,
                                 sizeof(row->type));
    filenight_copy_str(row->modified, sizeof(row->modified), "--");
    filenight_copy_str(row->size, sizeof(row->size), "--");
    row->is_directory = true;
    row->is_drive = true;
    row->icon = FILENIGHT_ICON_DRIVE;
  }
  qsort(panel->rows, (size_t)panel->row_count, sizeof(panel->rows[0]),
        filenight_row_compare);
  filenight_panel_update_title(panel, sidebar);
#else
  (void)sidebar;
  if (panel) {
    panel->row_count = 0;
    panel->selected_row = -1;
  }
#endif
}

static void filenight_panel_load_directory(FileNightPanelState *panel,
                                           const FileNightSidebarState *sidebar) {
#ifdef _WIN32
  WIN32_FIND_DATAA find_data;
  HANDLE find_handle;
  char search[FILENIGHT_MAX_PATH];
  if (!panel)
    return;
  panel->row_count = 0;
  panel->selected_row = -1;
  panel->is_this_pc = false;
  if (!filenight_path_is_directory(panel->path)) {
    filenight_panel_load_this_pc(panel, sidebar);
    return;
  }
  filenight_path_join(search, sizeof(search), panel->path, "*");
  find_handle = FindFirstFileA(search, &find_data);
  if (find_handle == INVALID_HANDLE_VALUE) {
    filenight_panel_update_title(panel, sidebar);
    return;
  }
  do {
    FileNightRow *row;
    unsigned long long size_value;
    if (strcmp(find_data.cFileName, ".") == 0 ||
        strcmp(find_data.cFileName, "..") == 0)
      continue;
    if (panel->row_count >= FILENIGHT_MAX_ROWS)
      break;
    row = &panel->rows[panel->row_count++];
    memset(row, 0, sizeof(*row));
    filenight_copy_str(row->name, sizeof(row->name), find_data.cFileName);
    filenight_path_join(row->path, sizeof(row->path), panel->path,
                        find_data.cFileName);
    row->is_directory =
        (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    row->is_drive = false;
    row->icon = row->is_directory ? FILENIGHT_ICON_FOLDER
                                  : FILENIGHT_ICON_DOCUMENTS;
    filenight_file_type_for_name(row->name, row->is_directory, false, row->type,
                                 sizeof(row->type));
    filenight_format_file_time(&find_data.ftLastWriteTime, row->modified,
                               sizeof(row->modified));
    size_value =
        ((unsigned long long)find_data.nFileSizeHigh << 32) |
        (unsigned long long)find_data.nFileSizeLow;
    filenight_format_file_size(size_value, row->is_directory, row->size,
                               sizeof(row->size));
  } while (FindNextFileA(find_handle, &find_data));
  FindClose(find_handle);
  qsort(panel->rows, (size_t)panel->row_count, sizeof(panel->rows[0]),
        filenight_row_compare);
  filenight_panel_update_title(panel, sidebar);
#else
  (void)sidebar;
  if (panel) {
    panel->row_count = 0;
    panel->selected_row = -1;
  }
#endif
}

static void filenight_panel_open(FileNightPanelState *panel, const char *path,
                                 bool is_this_pc,
                                 const FileNightSidebarState *sidebar) {
  if (!panel)
    return;
  if (is_this_pc) {
    filenight_panel_load_this_pc(panel, sidebar);
    return;
  }
  if (!path || !path[0])
    return;
  filenight_copy_str(panel->path, sizeof(panel->path), path);
  filenight_panel_load_directory(panel, sidebar);
}

static void filenight_sidebar_init(FileNightSidebarState *sidebar) {
#ifdef _WIN32
  char desktop[FILENIGHT_MAX_PATH];
  char documents[FILENIGHT_MAX_PATH];
  char downloads[FILENIGHT_MAX_PATH];
  char pictures[FILENIGHT_MAX_PATH];
  char music[FILENIGHT_MAX_PATH];
  char drives[512];
  DWORD len;
  char *it;
#endif
  if (!sidebar)
    return;
  memset(sidebar, 0, sizeof(*sidebar));
  filenight_current_directory(sidebar->current_dir, sizeof(sidebar->current_dir));
  filenight_user_profile(sidebar->user_profile, sizeof(sidebar->user_profile));

  filenight_sidebar_push(sidebar->study, &sidebar->study_count, "Study",
                         sidebar->current_dir, FILENIGHT_ICON_BOOKMARK, false);

#ifdef _WIN32
  filenight_path_join(desktop, sizeof(desktop), sidebar->user_profile, "Desktop");
  filenight_path_join(documents, sizeof(documents), sidebar->user_profile,
                      "Documents");
  filenight_path_join(downloads, sizeof(downloads), sidebar->user_profile,
                      "Downloads");
  filenight_path_join(pictures, sizeof(pictures), sidebar->user_profile,
                      "Pictures");
  filenight_path_join(music, sizeof(music), sidebar->user_profile, "Music");

  if (filenight_path_exists(desktop))
    filenight_sidebar_push(sidebar->recent, &sidebar->recent_count, "Desktop",
                           desktop, FILENIGHT_ICON_DESKTOP, false);
  if (filenight_path_exists(documents))
    filenight_sidebar_push(sidebar->recent, &sidebar->recent_count, "Documents",
                           documents, FILENIGHT_ICON_DOCUMENTS, false);
  if (filenight_path_exists(downloads))
    filenight_sidebar_push(sidebar->recent, &sidebar->recent_count, "Downloads",
                           downloads, FILENIGHT_ICON_DOWNLOADS, false);
  filenight_sidebar_push(sidebar->recent, &sidebar->recent_count, "Projects",
                         sidebar->current_dir, FILENIGHT_ICON_FOLDER, false);

  len = GetLogicalDriveStringsA((DWORD)sizeof(drives), drives);
  if (len > 0 && len < sizeof(drives)) {
    for (it = drives; *it; it += strlen(it) + 1) {
      char label[FILENIGHT_MAX_LABEL];
      filenight_drive_display_name(it, label, sizeof(label));
      filenight_sidebar_push(sidebar->storage, &sidebar->storage_count, label, it,
                             FILENIGHT_ICON_DRIVE, false);
    }
  }

  filenight_sidebar_push(sidebar->places, &sidebar->places_count, "This PC", "",
                         FILENIGHT_ICON_DESKTOP, true);
  filenight_sidebar_push(sidebar->places, &sidebar->places_count, "Night",
                         sidebar->user_profile, FILENIGHT_ICON_BOOKMARK, false);
  if (filenight_path_exists(desktop))
    filenight_sidebar_push(sidebar->places, &sidebar->places_count, "Desktop",
                           desktop, FILENIGHT_ICON_DESKTOP, false);
  if (filenight_path_exists(documents))
    filenight_sidebar_push(sidebar->places, &sidebar->places_count, "Documents",
                           documents, FILENIGHT_ICON_DOCUMENTS, false);
  if (filenight_path_exists(downloads))
    filenight_sidebar_push(sidebar->places, &sidebar->places_count, "Downloads",
                           downloads, FILENIGHT_ICON_DOWNLOADS, false);
  if (filenight_path_exists(pictures))
    filenight_sidebar_push(sidebar->places, &sidebar->places_count, "Pictures",
                           pictures, FILENIGHT_ICON_FOLDER, false);
  if (filenight_path_exists(music))
    filenight_sidebar_push(sidebar->places, &sidebar->places_count, "Music", music,
                           FILENIGHT_ICON_FOLDER, false);
#endif
}

static bool filenight_path_matches_item(const FileNightPanelState *panel,
                                        const FileNightSidebarItem *item,
                                        const FileNightSidebarState *sidebar) {
  if (!panel || !item)
    return false;
  if (item->is_this_pc)
    return panel->is_this_pc;
  if (!panel->path[0] || !item->path[0])
    return false;
  if (_stricmp(panel->path, item->path) == 0)
    return true;
  if (sidebar && sidebar->user_profile[0] && _stricmp(item->label, "Night") == 0 &&
      _stricmp(panel->path, sidebar->user_profile) == 0)
    return true;
  return false;
}

static void filenight_draw_breadcrumbs(StygianContext *ctx, StygianFont font,
                                       const FileNightPanelState *panel,
                                       const FileNightSidebarState *sidebar,
                                       float x, float y, float text_px) {
  float crumb_x = x;
  float seg_gap = template_snap(16.0f * (text_px / 12.5f));
  float sep_gap = template_snap(14.0f * (text_px / 12.5f));
  float sep_w = stygian_text_width(ctx, font, ">", text_px);
  const char *cursor;
  bool emitted = false;

  stygian_text(ctx, font, "This PC", crumb_x, y, text_px, 0.65f, 0.65f, 0.68f,
               0.94f);
  crumb_x += stygian_text_width(ctx, font, "This PC", text_px) + seg_gap;

  if (panel->is_this_pc)
    return;

  stygian_text(ctx, font, ">", crumb_x, y, text_px, 0.42f, 0.42f, 0.45f, 0.90f);
  crumb_x += sep_w + sep_gap;

  cursor = panel->path;
  while (cursor && *cursor) {
    char segment[FILENIGHT_MAX_LABEL];
    const char *label = segment;
    size_t seg_len = 0;
    bool is_last = false;
    while (*cursor == '\\' || *cursor == '/')
      ++cursor;
    while (cursor[seg_len] && cursor[seg_len] != '\\' && cursor[seg_len] != '/')
      ++seg_len;
    if (seg_len == 0)
      break;
    if (seg_len >= sizeof(segment))
      seg_len = sizeof(segment) - 1;
    memcpy(segment, cursor, seg_len);
    segment[seg_len] = '\0';
    cursor += seg_len;
    while (*cursor == '\\' || *cursor == '/')
      ++cursor;
    if (!*cursor)
      is_last = true;

    if (!emitted && sidebar && sidebar->user_profile[0] &&
        _strnicmp(panel->path, sidebar->user_profile,
                  strlen(sidebar->user_profile)) == 0 &&
        _stricmp(segment, filenight_path_leaf(sidebar->user_profile)) == 0) {
      label = "Night";
    }
    if (strlen(segment) == 2 && segment[1] == ':')
      label = segment;

    stygian_text(ctx, font, label, crumb_x, y, text_px,
                 is_last ? 0.90f : 0.65f, is_last ? 0.90f : 0.65f,
                 is_last ? 0.92f : 0.68f, is_last ? 0.99f : 0.94f);
    crumb_x += stygian_text_width(ctx, font, label, text_px);
    emitted = true;

    if (*cursor) {
      crumb_x += seg_gap;
      stygian_text(ctx, font, ">", crumb_x, y, text_px, 0.42f, 0.42f, 0.45f,
                   0.90f);
      crumb_x += sep_w + sep_gap;
    }
  }
}

static const FileNightSidebarItem *
filenight_sidebar_item_at(const TemplateLayout *layout, const float section_open[4],
                          const FileNightSidebarState *sidebar,
                          float sidebar_zoom, float px, float py) {
  float scale = template_clamp(sidebar_zoom / 12.5f, 0.90f, 1.65f);
  float item_step = template_snap(20.0f * scale);
  int section;
  for (section = 0; section < 4; ++section) {
    const FileNightSidebarItem *items;
    int item_count = 0;
    float hx, hy, hw, hh;
    float y;
    int i;
    items = filenight_sidebar_items_const(sidebar, section, &item_count);
    filenight_section_header_rect(layout, section_open, sidebar, section, scale,
                                  &hx, &hy, &hw, &hh);
    y = hy + template_snap(24.0f * scale);
    if (!items || item_count <= 0 || section_open[section] <= 0.01f)
      continue;
    for (i = 0; i < item_count; ++i) {
      if (template_point_in_rect(px, py, layout->sidebar_x + template_snap(10.0f * scale),
                                 y - template_snap(2.0f * scale),
                                 layout->sidebar_w - template_snap(20.0f * scale),
                                 item_step))
        return &items[i];
      y += item_step;
    }
  }
  return NULL;
}

static void filenight_tab_target_rect(StygianContext *ctx, StygianFont font,
                                      const TemplateLayout *layout,
                                      const char tabs[][32], int tab_count,
                                      float tab_scroll, int index, float *out_x,
                                      float *out_w) {
  float x = layout->tabs_x + 4.0f - tab_scroll;
  int i;
  for (i = 0; i < index && i < tab_count; ++i)
    x += filenight_panel_tab_width(ctx, font, tabs[i]) + 6.0f;
  if (out_x)
    *out_x = x;
  if (out_w)
    *out_w = filenight_panel_tab_width(ctx, font, tabs[index]);
}

static void template_compute_layout(TemplateLayout *layout, int width, int height,
                                    const StygianTitlebarHints *hints,
                                    float sidebar_w) {
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
  layout->tabs_h = 26.0f;
  layout->tabs_y = (layout->titlebar_h + 8.0f) - layout->tabs_h + 1.0f;
  layout->tabs_add_w = 18.0f;
  layout->tabs_add_h = 18.0f;
  layout->tabs_add_y = layout->tabs_y + 4.0f;
  layout->body_x = layout->shell_x + 8.0f;
  layout->body_y = layout->titlebar_h + 8.0f;
  layout->body_w = layout->shell_w - 16.0f;
  layout->body_h = layout->shell_h - layout->body_y - 8.0f;
  layout->sidebar_x = layout->body_x;
  layout->sidebar_y = layout->body_y;
  layout->sidebar_w = filenight_clamp_sidebar_width(layout->shell_w, sidebar_w);
  layout->sidebar_h = layout->body_h;
  layout->divider_w = 8.0f;
  layout->divider_x = layout->sidebar_x + layout->sidebar_w;
  layout->divider_y = layout->body_y + 6.0f;
  layout->divider_h = layout->body_h - 12.0f;
  layout->content_x = layout->divider_x + layout->divider_w;
  layout->content_y = layout->body_y;
  layout->content_w =
      (layout->body_x + layout->body_w) - layout->content_x;
  layout->content_h = layout->body_h;
  layout->tabs_x = layout->content_x + 12.0f;
  layout->tabs_w = layout->minimize_x - layout->tabs_x - 12.0f;
}

static void filenight_draw_body(StygianContext *ctx, StygianFont font,
                                const TemplateLayout *layout,
                                float mouse_x, float mouse_y,
                                const bool section_expanded[4],
                                const float section_open[4],
                                const FileNightSidebarState *sidebar,
                                const FileNightPanelState *panel_states,
                                const float panel_tab_visual_x[FILENIGHT_MAX_TABS],
                                const char panel_tabs[][32], int panel_tab_count,
                                int active_panel_tab, float sidebar_zoom,
                                float panel_zoom, float panel_tab_scroll,
                                bool divider_hovered,
                                bool divider_dragging) {
  float icon_x;
  float item_text_x;
  float sidebar_scale = template_clamp(sidebar_zoom / 12.5f, 0.90f, 1.65f);
  float sidebar_header_px = template_clamp(sidebar_zoom + 0.5f, 11.5f, 20.5f);
  float sidebar_item_px = template_clamp(sidebar_zoom, 11.0f, 19.0f);
  float sidebar_r = 0.050f, sidebar_g = 0.050f, sidebar_b = 0.054f;
  float content_r = 0.040f, content_g = 0.040f, content_b = 0.043f;
  float divider_r = divider_dragging ? 0.64f : (divider_hovered ? 0.46f : 0.20f);
  float divider_g = divider_dragging ? 0.64f : (divider_hovered ? 0.46f : 0.20f);
  float divider_b = divider_dragging ? 0.68f : (divider_hovered ? 0.50f : 0.24f);
  float line_w = divider_dragging ? 3.0f : (divider_hovered ? 2.0f : 1.0f);
  float line_x = template_snap(layout->divider_x + (layout->divider_w - line_w) * 0.5f);
  static const char *sections[] = {"Study", "Recent", "Storage", "Places"};
  const FileNightPanelState *active_panel =
      (panel_states && active_panel_tab >= 0 && active_panel_tab < panel_tab_count)
          ? &panel_states[active_panel_tab]
          : NULL;
  int i;
  float y;

  stygian_rect_rounded(ctx, layout->sidebar_x, layout->sidebar_y, layout->sidebar_w,
                       layout->sidebar_h, sidebar_r, sidebar_g, sidebar_b, 1.0f, 11.0f);
  stygian_rect_rounded(ctx, layout->content_x, layout->content_y, layout->content_w,
                       layout->content_h, content_r, content_g, content_b, 1.0f, 11.0f);
  stygian_rect_rounded(ctx, line_x, layout->divider_y, line_w, layout->divider_h,
                       divider_r, divider_g, divider_b,
                       divider_dragging ? 0.98f : 0.92f, line_w * 0.5f);

  if (!font)
    return;

  icon_x = layout->sidebar_x + template_snap(18.0f * sidebar_scale);
  item_text_x = layout->sidebar_x + template_snap(42.0f * sidebar_scale);

  for (i = 0; i < 4; ++i) {
    const FileNightSidebarItem *items;
    int item_count = 0;
    float open = section_open[i];
    float hx, hy, hw, hh;
    float item_step = template_snap(20.0f * sidebar_scale);
    items = filenight_sidebar_items_const(sidebar, i, &item_count);
    filenight_section_header_rect(layout, section_open, sidebar, i, sidebar_scale,
                                  &hx, &hy, &hw, &hh);
    y = hy;
    filenight_draw_section_chevron(ctx, hx, hy + 3.0f * sidebar_scale,
                                   section_expanded[i], sidebar_scale);
    stygian_text(ctx, font, sections[i], hx + template_snap(16.0f * sidebar_scale),
                 y, sidebar_header_px,
                 0.82f, 0.82f, 0.85f, 0.98f);
    y += template_snap(24.0f * sidebar_scale);
    if (open <= 0.01f || !items || item_count <= 0) {
      continue;
    }
    {
      int j;
      for (j = 0; j < item_count; ++j) {
        bool selected = active_panel &&
                        filenight_path_matches_item(active_panel, &items[j], sidebar);
        if (selected) {
          stygian_rect_rounded(ctx, layout->sidebar_x + template_snap(10.0f * sidebar_scale),
                               y - template_snap(2.0f * sidebar_scale),
                               layout->sidebar_w - template_snap(20.0f * sidebar_scale),
                               item_step - template_snap(2.0f * sidebar_scale),
                               0.16f, 0.16f, 0.18f, 0.56f, 6.0f);
        }
        if (open > 0.22f)
          filenight_draw_sidebar_icon(ctx, icon_x, y, items[j].icon, sidebar_scale);
        stygian_text(ctx, font, items[j].label, item_text_x, y, sidebar_item_px,
                     selected ? 0.96f : 0.88f, selected ? 0.96f : 0.88f,
                     selected ? 0.98f : 0.90f, open * open);
        y += item_step;
      }
    }
  }

  {
    float panel_x = layout->content_x + 14.0f;
    float panel_y = layout->body_y + 2.0f;
    float panel_w = layout->content_w - 28.0f;
    float panel_h = layout->content_h - 14.0f;
    float tab_y = layout->tabs_y;
    float tab_h = layout->tabs_h;
    float panel_body_y = layout->body_y + 1.0f;
    float zoom_scale = panel_zoom / 12.5f;
    float path_y = panel_body_y + 10.0f;
    float path_h = template_snap(30.0f * zoom_scale);
    float header_y = path_y + path_h + template_snap(8.0f * zoom_scale);
    float header_h = template_snap(28.0f * zoom_scale);
    float rows_y = header_y + header_h + template_snap(4.0f * zoom_scale);
    float row_h = template_snap(30.0f * zoom_scale);
    float footer_h = template_snap(28.0f * zoom_scale);
    float footer_y = panel_y + panel_h - footer_h;
    float rows_h = footer_y - rows_y - 8.0f;
    float name_x = panel_x + 18.0f;
    float type_x = panel_x + panel_w * 0.52f;
    float modified_x = panel_x + panel_w * 0.70f;
    float size_x = panel_x + panel_w - 86.0f;
    int row_count = active_panel ? active_panel->row_count : 0;
    int r;
    int t;
    int hovered_tab = -1;
    int hovered_tab_close = -1;
    float active_tab_x = 0.0f;
    float active_tab_w = 0.0f;

    stygian_rect_rounded(ctx, panel_x, panel_body_y, panel_w,
                         (panel_y + panel_h) - panel_body_y,
                         0.055f, 0.055f, 0.058f, 0.96f, 10.0f);

    stygian_clip_push(ctx, layout->tabs_x, layout->tabs_y, layout->tabs_w,
                      layout->tabs_h);
    for (t = 0; t < panel_tab_count; ++t) {
      float tab_x, tab_w;
      float close_x, close_y, close_w, close_h;
      filenight_tab_target_rect(ctx, font, layout, panel_tabs, panel_tab_count,
                                panel_tab_scroll, t, &tab_x, &tab_w);
      tab_x = panel_tab_visual_x[t];
      if (tab_x + tab_w < layout->tabs_x - 2.0f ||
          tab_x > layout->tabs_x + layout->tabs_w + 2.0f)
        continue;
      if (template_point_in_rect(mouse_x, mouse_y, tab_x, tab_y + 2.0f, tab_w,
                                 tab_h - 4.0f))
        hovered_tab = t;
      if (panel_tab_count > 1 &&
          filenight_visual_tab_close_rect(tab_x, tab_w,
                                          (t == active_panel_tab) ? tab_y + 1.0f
                                                                  : tab_y + 3.0f,
                                          &close_x, &close_y, &close_w, &close_h) &&
          template_point_in_rect(mouse_x, mouse_y, close_x, close_y, close_w,
                                 close_h * 0.7f))
        hovered_tab_close = t;
    }

    for (t = 0; t < panel_tab_count; ++t) {
      float tab_x, tab_w;
      bool active = (t == active_panel_tab);
      filenight_tab_target_rect(ctx, font, layout, panel_tabs, panel_tab_count,
                                panel_tab_scroll, t, &tab_x, &tab_w);
      tab_x = panel_tab_visual_x[t];
      if (tab_x + tab_w < layout->tabs_x - 2.0f ||
          tab_x > layout->tabs_x + layout->tabs_w + 2.0f)
        continue;
      if (active) {
        active_tab_x = tab_x;
        active_tab_w = tab_w;
        continue;
      }
      stygian_rect_rounded(ctx, tab_x, tab_y + 3.0f, tab_w, tab_h - 4.0f,
                           (t == hovered_tab) ? 0.15f : 0.11f,
                           (t == hovered_tab) ? 0.15f : 0.11f,
                           (t == hovered_tab) ? 0.16f : 0.12f, 1.0f, 6.0f);
      stygian_text(ctx, font, panel_tabs[t], tab_x + 16.0f,
                   template_snap(tab_y + 6.0f), 12.5f,
                   (t == hovered_tab) ? 0.90f : 0.72f,
                   (t == hovered_tab) ? 0.90f : 0.72f,
                   (t == hovered_tab) ? 0.92f : 0.74f,
                   (t == hovered_tab) ? 0.98f : 0.94f);
      if ((t == hovered_tab || t == hovered_tab_close) && panel_tab_count > 1) {
        float close_x, close_y, close_w, close_h;
        if (filenight_visual_tab_close_rect(tab_x, tab_w, tab_y + 3.0f, &close_x,
                                            &close_y, &close_w, &close_h)) {
          template_draw_builtin_icon(ctx, close_x, close_y, close_w, close_h,
                                     STYGIAN_ICON_CLOSE,
                                     (t == hovered_tab_close) ? 0.98f : 0.78f);
        }
      }
    }
    stygian_clip_pop(ctx);

    if (active_panel_tab >= 0 && active_panel_tab < panel_tab_count) {
      if (active_tab_w <= 0.0f)
        filenight_tab_target_rect(ctx, font, layout, panel_tabs, panel_tab_count,
                                  panel_tab_scroll, active_panel_tab, &active_tab_x,
                                  &active_tab_w);
      active_tab_x = panel_tab_visual_x[active_panel_tab];
      stygian_rect_rounded(ctx, active_tab_x, tab_y + 1.0f, active_tab_w,
                           panel_body_y - tab_y + 5.0f, 0.055f, 0.055f, 0.058f,
                           1.0f, 6.0f);
      stygian_rect_rounded(ctx, active_tab_x - 4.0f, panel_body_y - 4.0f,
                           active_tab_w + 8.0f, 12.0f, 0.055f, 0.055f, 0.058f,
                           1.0f, 5.0f);
      stygian_rect(ctx, active_tab_x + 12.0f, tab_y + 3.0f, active_tab_w - 24.0f,
                   2.0f, 0.68f, 0.68f, 0.70f, 0.96f);
      stygian_rect_rounded(ctx, active_tab_x + 12.0f, tab_y + 10.0f, 5.0f, 5.0f,
                           0.74f, 0.74f, 0.76f, 0.98f, 2.5f);
      stygian_text(ctx, font, panel_tabs[active_panel_tab], active_tab_x + 24.0f,
                   template_snap(tab_y + 6.0f), 12.5f, 0.94f, 0.94f, 0.95f, 1.0f);
      if ((active_panel_tab == hovered_tab || active_panel_tab == hovered_tab_close) &&
          panel_tab_count > 1) {
        float close_x, close_y, close_w, close_h;
        if (filenight_visual_tab_close_rect(active_tab_x, active_tab_w, tab_y + 1.0f,
                                            &close_x, &close_y, &close_w, &close_h)) {
          template_draw_builtin_icon(ctx, close_x, close_y, close_w, close_h,
                                     STYGIAN_ICON_CLOSE,
                                     (active_panel_tab == hovered_tab_close) ? 0.98f
                                                                             : 0.78f);
        }
      }
    }
    {
      float add_x, add_y, add_w, add_h;
      filenight_panel_add_rect(ctx, font, layout, panel_tabs, panel_tab_visual_x,
                               panel_tab_count,
                               &add_x, &add_y, &add_w, &add_h);
      template_draw_builtin_icon(
          ctx, add_x, add_y, add_w, add_h, STYGIAN_ICON_PLUS,
          template_point_in_rect(mouse_x, mouse_y, add_x, add_y, add_w, add_h)
              ? 0.98f
              : 0.88f);
    }

    stygian_rect_rounded(ctx, panel_x + 10.0f, path_y, panel_w - 20.0f, path_h,
                         0.016f, 0.045f, 0.073f, 0.98f, 8.0f);
    if (active_panel)
      filenight_draw_breadcrumbs(ctx, font, active_panel, sidebar,
                                 panel_x + 24.0f,
                                 path_y + template_snap(7.0f * zoom_scale),
                                 panel_zoom);

    stygian_rect_rounded(ctx, panel_x + 10.0f, header_y, panel_w - 20.0f, header_h,
                         0.075f, 0.075f, 0.079f, 0.96f, 7.0f);
    stygian_text(ctx, font, "Name", name_x, header_y + template_snap(6.0f * zoom_scale), panel_zoom, 0.60f, 0.60f,
                 0.63f, 0.92f);
    stygian_text(ctx, font, "Type", type_x, header_y + template_snap(6.0f * zoom_scale), panel_zoom, 0.60f, 0.60f,
                 0.63f, 0.92f);
    stygian_text(ctx, font, "Date Modified", modified_x, header_y + template_snap(6.0f * zoom_scale), panel_zoom,
                 0.60f, 0.60f, 0.63f, 0.92f);
    stygian_text(ctx, font, "Size", size_x, header_y + template_snap(6.0f * zoom_scale), panel_zoom, 0.60f, 0.60f,
                 0.63f, 0.92f);

    for (r = 0; r < row_count; ++r) {
      float row_y = rows_y + (float)r * row_h;
      bool row_hovered = template_point_in_rect(mouse_x, mouse_y, panel_x + 10.0f,
                                                row_y, panel_w - 20.0f, row_h);
      if (row_hovered || (active_panel && r == active_panel->selected_row)) {
        bool selected = active_panel && (r == active_panel->selected_row);
        float rr = selected ? 0.23f : 0.13f;
        float rg = selected ? 0.23f : 0.13f;
        float rb = selected ? 0.26f : 0.15f;
        float ra = selected ? 0.86f : 0.50f;
        stygian_rect_rounded(ctx, panel_x + 10.0f, row_y, panel_w - 20.0f,
                             row_h - 2.0f, rr, rg, rb, ra, 6.0f);
      }

      filenight_draw_sidebar_icon(ctx, name_x, row_y + template_snap(6.0f * zoom_scale),
                                  active_panel ? active_panel->rows[r].icon
                                               : FILENIGHT_ICON_DOCUMENTS,
                                  zoom_scale);

      stygian_text(ctx, font, active_panel ? active_panel->rows[r].name : "",
                   name_x + template_snap(24.0f * zoom_scale),
                   row_y + template_snap(6.0f * zoom_scale), panel_zoom,
                   0.90f, 0.90f, 0.92f, 0.98f);
      stygian_text(ctx, font, active_panel ? active_panel->rows[r].type : "",
                   type_x, row_y + template_snap(6.0f * zoom_scale),
                   template_clamp(panel_zoom - 0.5f, 11.0f, 22.0f), 0.58f,
                   0.58f, 0.60f, 0.90f);
      stygian_text(ctx, font, active_panel ? active_panel->rows[r].modified : "",
                   modified_x, row_y + template_snap(6.0f * zoom_scale),
                   template_clamp(panel_zoom - 0.5f, 11.0f, 22.0f),
                   0.58f, 0.58f, 0.60f, 0.90f);
      stygian_text(ctx, font, active_panel ? active_panel->rows[r].size : "",
                   size_x, row_y + template_snap(6.0f * zoom_scale),
                   template_clamp(panel_zoom - 0.5f, 11.0f, 22.0f), 0.58f,
                   0.58f, 0.60f, 0.90f);
    }

    stygian_rect_rounded(ctx, panel_x + 10.0f, footer_y, panel_w - 20.0f, footer_h,
                         0.075f, 0.075f, 0.079f, 0.96f, 7.0f);
    {
      char footer_label[32];
      if (active_panel && active_panel->selected_row >= 0)
        snprintf(footer_label, sizeof(footer_label), "1 selected");
      else if (active_panel)
        snprintf(footer_label, sizeof(footer_label), "%d items",
                 active_panel->row_count);
      else
        snprintf(footer_label, sizeof(footer_label), "Ready");
      stygian_text(ctx, font, footer_label, panel_x + 22.0f,
                   footer_y + template_snap(6.0f * zoom_scale), panel_zoom,
                 0.65f, 0.65f, 0.68f,
                 0.94f);
    }
    stygian_text(ctx, font, "Details", panel_x + panel_w - 92.0f,
                 footer_y + template_snap(6.0f * zoom_scale), panel_zoom,
                 0.74f, 0.74f, 0.77f, 0.96f);

    if (rows_h < row_count * row_h) {
      float track_x = panel_x + panel_w - 12.0f;
      stygian_rect_rounded(ctx, track_x, rows_y + 4.0f, 3.0f, rows_h - 8.0f,
                           0.12f, 0.12f, 0.13f, 0.88f, 1.5f);
      stygian_rect_rounded(ctx, track_x, rows_y + 14.0f, 3.0f, 54.0f, 0.42f,
                           0.42f, 0.45f, 0.92f, 1.5f);
    }
  }
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
  bool first_frame = true;
  float mouse_x = 0.0f;
  float mouse_y = 0.0f;
  float sidebar_w = 246.0f;
  bool mouse_down = false;
  bool shutting_down = false;
  bool divider_dragging = false;
  bool hover_state_valid = false;
  bool hovered_menu_prev = false;
  bool hovered_minimize_prev = false;
  bool hovered_maximize_prev = false;
  bool hovered_close_prev = false;
  bool hovered_divider_prev = false;
  bool section_expanded[4] = {true, true, true, true};
  float section_open[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float panel_tab_visual_x[FILENIGHT_MAX_TABS] = {0};
  float panel_drag_offset_x = 0.0f;
  float panel_drag_start_x = 0.0f;
  float panel_drag_start_y = 0.0f;
  float panel_tab_scroll = 0.0f;
  bool panel_tab_visual_valid = false;
  bool panel_tab_drag_active = false;
  float sidebar_zoom = 12.5f;
  float panel_zoom = 12.5f;
  int panel_drag_tab = -1;
  int panel_tab_count = 2;
  int active_panel_tab = 0;
  int next_panel_tab_id = 1;
  static FileNightSidebarState sidebar;
  static FileNightPanelState panel_states[FILENIGHT_MAX_TABS];
  char panel_tabs[FILENIGHT_MAX_TABS][32] = {"Study", "Shared (Z:)"};
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
      .window = window,
  });
  if (!ctx)
    return 1;
  stygian_set_vsync(ctx, false);
  font = stygian_get_default_font(ctx);
  filenight_sidebar_init(&sidebar);
  filenight_panel_open(&panel_states[0], sidebar.current_dir, false, &sidebar);
  if (sidebar.storage_count > 0)
    filenight_panel_open(&panel_states[1], sidebar.storage[0].path, false, &sidebar);
  else
    filenight_panel_open(&panel_states[1], "", true, &sidebar);
  filenight_copy_str(panel_tabs[0], sizeof(panel_tabs[0]), panel_states[0].title);
  filenight_copy_str(panel_tabs[1], sizeof(panel_tabs[1]), panel_states[1].title);

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
    template_compute_layout(&layout, width, height, &hints, sidebar_w);

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
        if (divider_dragging) {
          sidebar_w = filenight_clamp_sidebar_width(
              (float)width, mouse_x - layout.body_x - layout.divider_w * 0.5f);
          template_compute_layout(&layout, width, height, &hints, sidebar_w);
          event_mutated = true;
          event_requested = true;
        }
          if (panel_drag_tab >= 0) {
            float dx = fabsf(mouse_x - panel_drag_start_x);
            float dy = fabsf(mouse_y - panel_drag_start_y);
            if (!panel_tab_drag_active && (dx > 5.0f || dy > 4.0f))
              panel_tab_drag_active = true;
            event_requested = true;
          }
      } else if (event.type == STYGIAN_EVENT_SCROLL) {
        mouse_x = (float)event.scroll.x;
        mouse_y = (float)event.scroll.y;
        if ((stygian_get_mods(window) & STYGIAN_MOD_CTRL) &&
            template_point_in_rect(mouse_x, mouse_y, layout.sidebar_x,
                                   layout.sidebar_y, layout.sidebar_w,
                                   layout.sidebar_h)) {
          sidebar_zoom =
              template_clamp(sidebar_zoom + event.scroll.dy * 0.75f, 11.0f, 20.0f);
          event_mutated = true;
          event_requested = true;
        } else if ((stygian_get_mods(window) & STYGIAN_MOD_CTRL) &&
                   template_point_in_rect(mouse_x, mouse_y, layout.content_x,
                                          layout.content_y, layout.content_w,
                                          layout.content_h)) {
          panel_zoom = template_clamp(panel_zoom + event.scroll.dy * 0.75f,
                                      11.0f, 22.0f);
          event_mutated = true;
          event_requested = true;
        } else if (filenight_point_in_tab_strip(&layout, mouse_x, mouse_y)) {
          panel_tab_scroll = filenight_clamp_tab_scroll(
              ctx, font, &layout, panel_tabs, panel_tab_count,
              panel_tab_scroll + (-event.scroll.dy * 48.0f) -
                  (event.scroll.dx * 36.0f));
          event_requested = true;
        }
      } else if (event.type == STYGIAN_EVENT_MOUSE_DOWN ||
                 event.type == STYGIAN_EVENT_MOUSE_UP) {
        mouse_x = (float)event.mouse_button.x;
        mouse_y = (float)event.mouse_button.y;
        mouse_down =
            (event.type == STYGIAN_EVENT_MOUSE_DOWN) &&
            (event.mouse_button.button == STYGIAN_MOUSE_LEFT);
        if (event.type == STYGIAN_EVENT_MOUSE_UP &&
            event.mouse_button.button == STYGIAN_MOUSE_LEFT && divider_dragging) {
          divider_dragging = false;
          event_requested = true;
        }
        if (event.type == STYGIAN_EVENT_MOUSE_UP &&
            event.mouse_button.button == STYGIAN_MOUSE_LEFT && panel_drag_tab >= 0) {
          panel_drag_tab = -1;
          panel_tab_drag_active = false;
          event_requested = true;
        }
      }

      if (event.type == STYGIAN_EVENT_MOUSE_DOWN &&
          event.mouse_button.button == STYGIAN_MOUSE_LEFT) {
        FileNightPanelState *active_panel =
            (active_panel_tab >= 0 && active_panel_tab < panel_tab_count)
                ? &panel_states[active_panel_tab]
                : NULL;
        const FileNightSidebarItem *clicked_sidebar_item = NULL;
        int clicked_section = -1;
        int clicked_tab = -1;
        int clicked_tab_close = -1;
        int clicked_row =
            filenight_row_at(&layout, panel_zoom, mouse_x, mouse_y,
                             active_panel ? active_panel->row_count : 0);
        bool in_menu =
            template_point_in_button(&layout, mouse_x, mouse_y, layout.menu_x);
        bool in_minimize =
            template_point_in_button(&layout, mouse_x, mouse_y,
                                     layout.minimize_x);
        bool in_maximize =
            template_point_in_button(&layout, mouse_x, mouse_y,
                                     layout.maximize_x);
        bool in_close = template_point_in_button(&layout, mouse_x, mouse_y,
                                                 layout.close_x);
        bool in_divider = template_point_in_rect(mouse_x, mouse_y, layout.divider_x,
                                                 layout.divider_y, layout.divider_w,
                                                 layout.divider_h);
        bool in_titlebar =
            template_point_in_rect(mouse_x, mouse_y, layout.shell_x,
                                   layout.shell_y, layout.shell_w,
                                   layout.titlebar_h + 10.0f);
        bool in_panel_add = false;
        {
          int s;
          for (s = 0; s < 4; ++s) {
            float hx, hy, hw, hh;
              filenight_section_header_rect(
                  &layout, section_open, &sidebar, s,
                  template_clamp(sidebar_zoom / 12.5f, 0.90f, 1.65f), &hx, &hy,
                  &hw, &hh);
            if (template_point_in_rect(mouse_x, mouse_y, hx, hy, hw, hh)) {
              clicked_section = s;
              break;
            }
          }
        }
        {
          int t;
          float add_x, add_y, add_w, add_h;
          filenight_panel_add_rect(ctx, font, &layout, panel_tabs,
                                   panel_tab_visual_x, panel_tab_count, &add_x,
                                   &add_y, &add_w, &add_h);
          in_panel_add = template_point_in_rect(mouse_x, mouse_y, add_x, add_y,
                                                add_w, add_h);
          for (t = 0; t < panel_tab_count; ++t) {
            float tx, ty, tw, th;
            float cx, cy, cw, ch;
            filenight_tab_target_rect(ctx, font, &layout, panel_tabs,
                                      panel_tab_count, panel_tab_scroll, t, &tx,
                                      &tw);
            tx = panel_tab_visual_x[t];
            ty = layout.tabs_y;
            th = layout.tabs_h;
            filenight_visual_tab_close_rect(tx, tw, (t == active_panel_tab)
                                                     ? layout.tabs_y + 1.0f
                                                     : layout.tabs_y + 3.0f,
                                            &cx, &cy, &cw, &ch);
            if (panel_tab_count > 1 &&
                template_point_in_rect(mouse_x, mouse_y, cx, cy, cw, ch)) {
              clicked_tab_close = t;
              break;
            }
            if (template_point_in_rect(mouse_x, mouse_y, tx, ty, tw, th))
              clicked_tab = t;
          }
        }
        clicked_sidebar_item = filenight_sidebar_item_at(
            &layout, section_open, &sidebar, sidebar_zoom, mouse_x, mouse_y);
        if (in_divider) {
          divider_dragging = true;
          event_mutated = true;
          event_requested = true;
        } else if (clicked_tab_close >= 0) {
          int t;
          for (t = clicked_tab_close + 1; t < panel_tab_count; ++t) {
            memcpy(panel_tabs[t - 1], panel_tabs[t], sizeof(panel_tabs[t - 1]));
            memcpy(&panel_states[t - 1], &panel_states[t],
                   sizeof(panel_states[t - 1]));
          }
          panel_tab_count--;
          if (active_panel_tab >= panel_tab_count)
            active_panel_tab = panel_tab_count - 1;
          panel_tab_scroll = filenight_clamp_tab_scroll(
              ctx, font, &layout, panel_tabs, panel_tab_count, panel_tab_scroll);
          event_mutated = true;
          event_requested = true;
        } else if (clicked_tab >= 0) {
          active_panel_tab = clicked_tab;
          filenight_reveal_tab(ctx, font, &layout, panel_tabs, panel_tab_count,
                               &panel_tab_scroll, active_panel_tab);
          panel_drag_tab = clicked_tab;
          panel_tab_drag_active = false;
          panel_drag_start_x = mouse_x;
          panel_drag_start_y = mouse_y;
          panel_drag_offset_x = mouse_x - panel_tab_visual_x[clicked_tab];
          event_mutated = true;
          event_requested = true;
        } else if (in_panel_add && panel_tab_count < FILENIGHT_MAX_TABS) {
          (void)next_panel_tab_id;
          filenight_panel_open(&panel_states[panel_tab_count], "", true, &sidebar);
          active_panel_tab = panel_tab_count;
          panel_tab_count++;
          filenight_copy_str(panel_tabs[active_panel_tab], sizeof(panel_tabs[0]),
                             panel_states[active_panel_tab].title);
          filenight_reveal_tab(ctx, font, &layout, panel_tabs, panel_tab_count,
                               &panel_tab_scroll, active_panel_tab);
          event_mutated = true;
          event_requested = true;
        } else if (clicked_row >= 0) {
          if (active_panel)
            active_panel->selected_row = clicked_row;
          if (active_panel && event.mouse_button.clicks >= 2 &&
              clicked_row < active_panel->row_count &&
              (active_panel->rows[clicked_row].is_directory ||
               active_panel->rows[clicked_row].is_drive)) {
            filenight_panel_open(active_panel, active_panel->rows[clicked_row].path,
                                 false, &sidebar);
            filenight_copy_str(panel_tabs[active_panel_tab], sizeof(panel_tabs[0]),
                               active_panel->title);
          }
          event_mutated = true;
          event_requested = true;
        } else if (clicked_sidebar_item) {
          filenight_panel_open(active_panel, clicked_sidebar_item->path,
                               clicked_sidebar_item->is_this_pc, &sidebar);
          if (active_panel)
            filenight_copy_str(panel_tabs[active_panel_tab], sizeof(panel_tabs[0]),
                               active_panel->title);
          event_mutated = true;
          event_requested = true;
        } else if (clicked_section >= 0) {
          section_expanded[clicked_section] = !section_expanded[clicked_section];
          event_mutated = true;
          event_requested = true;
        } else if (in_menu) {
          // Leave this as a dead-simple anchor for the first real app menu.
          event_requested = true;
        } else if (in_minimize) {
          stygian_window_minimize(window);
          event_mutated = true;
          event_requested = true;
        } else if (in_maximize) {
          if (stygian_window_is_maximized(window))
            stygian_window_restore(window);
          else
            stygian_window_maximize(window);
          event_mutated = true;
          event_requested = true;
        } else if (in_close) {
          stygian_set_present_enabled(ctx, false);
          shutting_down = true;
          stygian_window_request_close(window);
          event_mutated = true;
          event_requested = true;
        } else if (in_titlebar) {
          if (event.mouse_button.clicks >= 2) {
            stygian_window_titlebar_double_click(window);
          } else {
            stygian_window_begin_system_move(window);
          }
          event_mutated = true;
          event_requested = true;
        }
      }

      if (event.type == STYGIAN_EVENT_KEY_DOWN && !event.key.repeat &&
          (event.key.mods & STYGIAN_MOD_CTRL) &&
          event.key.key == STYGIAN_KEY_0) {
        bool reset_any = false;
        if (template_point_in_rect(mouse_x, mouse_y, layout.sidebar_x,
                                   layout.sidebar_y, layout.sidebar_w,
                                   layout.sidebar_h)) {
          sidebar_zoom = 12.5f;
          reset_any = true;
        }
        if (template_point_in_rect(mouse_x, mouse_y, layout.content_x,
                                   layout.content_y, layout.content_w,
                                   layout.content_h)) {
          panel_zoom = 12.5f;
          reset_any = true;
        }
        if (!reset_any) {
          sidebar_zoom = 12.5f;
          panel_zoom = 12.5f;
        }
        event_mutated = true;
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
          if (divider_dragging) {
            sidebar_w = filenight_clamp_sidebar_width(
                (float)width, mouse_x - layout.body_x - layout.divider_w * 0.5f);
            template_compute_layout(&layout, width, height, &hints, sidebar_w);
            event_mutated = true;
            event_requested = true;
          }
          if (panel_drag_tab >= 0) {
            float dx = fabsf(mouse_x - panel_drag_start_x);
            float dy = fabsf(mouse_y - panel_drag_start_y);
            if (!panel_tab_drag_active && (dx > 5.0f || dy > 4.0f))
              panel_tab_drag_active = true;
            event_requested = true;
          }
        } else if (event.type == STYGIAN_EVENT_SCROLL) {
          mouse_x = (float)event.scroll.x;
          mouse_y = (float)event.scroll.y;
          if ((stygian_get_mods(window) & STYGIAN_MOD_CTRL) &&
              template_point_in_rect(mouse_x, mouse_y, layout.sidebar_x,
                                     layout.sidebar_y, layout.sidebar_w,
                                     layout.sidebar_h)) {
            sidebar_zoom =
                template_clamp(sidebar_zoom + event.scroll.dy * 0.75f, 11.0f, 20.0f);
            event_mutated = true;
            event_requested = true;
          } else if ((stygian_get_mods(window) & STYGIAN_MOD_CTRL) &&
                     template_point_in_rect(mouse_x, mouse_y, layout.content_x,
                                            layout.content_y, layout.content_w,
                                            layout.content_h)) {
            panel_zoom = template_clamp(panel_zoom + event.scroll.dy * 0.75f,
                                        11.0f, 22.0f);
            event_mutated = true;
            event_requested = true;
          } else if (filenight_point_in_tab_strip(&layout, mouse_x, mouse_y)) {
            panel_tab_scroll = filenight_clamp_tab_scroll(
                ctx, font, &layout, panel_tabs, panel_tab_count,
                panel_tab_scroll + (-event.scroll.dy * 48.0f) -
                    (event.scroll.dx * 36.0f));
            event_requested = true;
          }
        } else if (event.type == STYGIAN_EVENT_MOUSE_DOWN ||
                   event.type == STYGIAN_EVENT_MOUSE_UP) {
          mouse_x = (float)event.mouse_button.x;
          mouse_y = (float)event.mouse_button.y;
          mouse_down =
              (event.type == STYGIAN_EVENT_MOUSE_DOWN) &&
              (event.mouse_button.button == STYGIAN_MOUSE_LEFT);
          if (event.type == STYGIAN_EVENT_MOUSE_UP &&
              event.mouse_button.button == STYGIAN_MOUSE_LEFT && divider_dragging) {
            divider_dragging = false;
            event_requested = true;
          }
          if (event.type == STYGIAN_EVENT_MOUSE_UP &&
              event.mouse_button.button == STYGIAN_MOUSE_LEFT && panel_drag_tab >= 0) {
            panel_drag_tab = -1;
            panel_tab_drag_active = false;
            event_requested = true;
          }
        }

        if (event.type == STYGIAN_EVENT_MOUSE_DOWN &&
            event.mouse_button.button == STYGIAN_MOUSE_LEFT) {
          FileNightPanelState *active_panel =
              (active_panel_tab >= 0 && active_panel_tab < panel_tab_count)
                  ? &panel_states[active_panel_tab]
                  : NULL;
          const FileNightSidebarItem *clicked_sidebar_item = NULL;
          int clicked_section = -1;
          int clicked_tab = -1;
          int clicked_tab_close = -1;
          int clicked_row =
              filenight_row_at(&layout, panel_zoom, mouse_x, mouse_y,
                               active_panel ? active_panel->row_count : 0);
          bool in_menu =
              template_point_in_button(&layout, mouse_x, mouse_y, layout.menu_x);
          bool in_minimize =
              template_point_in_button(&layout, mouse_x, mouse_y,
                                       layout.minimize_x);
          bool in_maximize =
              template_point_in_button(&layout, mouse_x, mouse_y,
                                       layout.maximize_x);
          bool in_close = template_point_in_button(&layout, mouse_x, mouse_y,
                                                   layout.close_x);
          bool in_divider =
              template_point_in_rect(mouse_x, mouse_y, layout.divider_x,
                                     layout.divider_y, layout.divider_w,
                                     layout.divider_h);
          bool in_titlebar =
              template_point_in_rect(mouse_x, mouse_y, layout.shell_x,
                                     layout.shell_y, layout.shell_w,
                                     layout.titlebar_h + 10.0f);
          bool in_panel_add = false;
          {
            int s;
            for (s = 0; s < 4; ++s) {
              float hx, hy, hw, hh;
              filenight_section_header_rect(
                  &layout, section_open, &sidebar, s,
                  template_clamp(sidebar_zoom / 12.5f, 0.90f, 1.65f), &hx, &hy,
                  &hw, &hh);
              if (template_point_in_rect(mouse_x, mouse_y, hx, hy, hw, hh)) {
                clicked_section = s;
                break;
              }
            }
          }
          {
            int t;
            float add_x, add_y, add_w, add_h;
            filenight_panel_add_rect(ctx, font, &layout, panel_tabs,
                                     panel_tab_visual_x, panel_tab_count,
                                     &add_x, &add_y, &add_w, &add_h);
            in_panel_add = template_point_in_rect(mouse_x, mouse_y, add_x, add_y,
                                                  add_w, add_h);
            for (t = 0; t < panel_tab_count; ++t) {
              float tx, ty, tw, th;
              float cx, cy, cw, ch;
              filenight_tab_target_rect(ctx, font, &layout, panel_tabs,
                                        panel_tab_count, panel_tab_scroll, t, &tx,
                                        &tw);
              tx = panel_tab_visual_x[t];
              ty = layout.tabs_y;
              th = layout.tabs_h;
              filenight_visual_tab_close_rect(tx, tw, (t == active_panel_tab)
                                                       ? layout.tabs_y + 1.0f
                                                       : layout.tabs_y + 3.0f,
                                              &cx, &cy, &cw, &ch);
              if (panel_tab_count > 1 &&
                  template_point_in_rect(mouse_x, mouse_y, cx, cy, cw, ch)) {
                clicked_tab_close = t;
                break;
              }
              if (template_point_in_rect(mouse_x, mouse_y, tx, ty, tw, th))
                clicked_tab = t;
            }
          }
          clicked_sidebar_item = filenight_sidebar_item_at(
              &layout, section_open, &sidebar, sidebar_zoom, mouse_x, mouse_y);
          if (in_divider) {
            divider_dragging = true;
            event_mutated = true;
            event_requested = true;
          } else if (clicked_tab_close >= 0) {
            int t;
            for (t = clicked_tab_close + 1; t < panel_tab_count; ++t) {
              memcpy(panel_tabs[t - 1], panel_tabs[t], sizeof(panel_tabs[t - 1]));
              memcpy(&panel_states[t - 1], &panel_states[t],
                     sizeof(panel_states[t - 1]));
            }
            panel_tab_count--;
            if (active_panel_tab >= panel_tab_count)
              active_panel_tab = panel_tab_count - 1;
            panel_tab_scroll = filenight_clamp_tab_scroll(
                ctx, font, &layout, panel_tabs, panel_tab_count,
                panel_tab_scroll);
            event_mutated = true;
            event_requested = true;
          } else if (clicked_tab >= 0) {
            active_panel_tab = clicked_tab;
            filenight_reveal_tab(ctx, font, &layout, panel_tabs,
                                 panel_tab_count, &panel_tab_scroll,
                                 active_panel_tab);
            panel_drag_tab = clicked_tab;
            panel_tab_drag_active = false;
            panel_drag_start_x = mouse_x;
            panel_drag_start_y = mouse_y;
            panel_drag_offset_x = mouse_x - panel_tab_visual_x[clicked_tab];
            event_mutated = true;
            event_requested = true;
          } else if (in_panel_add && panel_tab_count < FILENIGHT_MAX_TABS) {
            (void)next_panel_tab_id;
            filenight_panel_open(&panel_states[panel_tab_count], "", true,
                                 &sidebar);
            active_panel_tab = panel_tab_count;
            panel_tab_count++;
            filenight_copy_str(panel_tabs[active_panel_tab],
                               sizeof(panel_tabs[0]),
                               panel_states[active_panel_tab].title);
            filenight_reveal_tab(ctx, font, &layout, panel_tabs,
                                 panel_tab_count, &panel_tab_scroll,
                                 active_panel_tab);
            event_mutated = true;
            event_requested = true;
          } else if (clicked_row >= 0) {
            if (active_panel)
              active_panel->selected_row = clicked_row;
            if (active_panel && event.mouse_button.clicks >= 2 &&
                clicked_row < active_panel->row_count &&
                (active_panel->rows[clicked_row].is_directory ||
                 active_panel->rows[clicked_row].is_drive)) {
              filenight_panel_open(active_panel,
                                   active_panel->rows[clicked_row].path, false,
                                   &sidebar);
              filenight_copy_str(panel_tabs[active_panel_tab],
                                 sizeof(panel_tabs[0]), active_panel->title);
            }
            event_mutated = true;
            event_requested = true;
          } else if (clicked_sidebar_item) {
            filenight_panel_open(active_panel, clicked_sidebar_item->path,
                                 clicked_sidebar_item->is_this_pc, &sidebar);
            if (active_panel)
              filenight_copy_str(panel_tabs[active_panel_tab],
                                 sizeof(panel_tabs[0]), active_panel->title);
            event_mutated = true;
            event_requested = true;
          } else if (clicked_section >= 0) {
            section_expanded[clicked_section] = !section_expanded[clicked_section];
            event_mutated = true;
            event_requested = true;
          } else if (in_menu) {
            event_requested = true;
          } else if (in_minimize) {
            stygian_window_minimize(window);
            event_mutated = true;
            event_requested = true;
          } else if (in_maximize) {
            if (stygian_window_is_maximized(window))
              stygian_window_restore(window);
            else
              stygian_window_maximize(window);
            event_mutated = true;
            event_requested = true;
          } else if (in_close) {
            stygian_set_present_enabled(ctx, false);
            shutting_down = true;
            stygian_window_request_close(window);
            event_mutated = true;
            event_requested = true;
          } else if (in_titlebar) {
            if (event.mouse_button.clicks >= 2) {
              stygian_window_titlebar_double_click(window);
            } else {
              stygian_window_begin_system_move(window);
            }
            event_mutated = true;
            event_requested = true;
          }
        }

        if (event.type == STYGIAN_EVENT_KEY_DOWN && !event.key.repeat &&
            (event.key.mods & STYGIAN_MOD_CTRL) &&
            event.key.key == STYGIAN_KEY_0) {
          bool reset_any = false;
          if (template_point_in_rect(mouse_x, mouse_y, layout.sidebar_x,
                                     layout.sidebar_y, layout.sidebar_w,
                                     layout.sidebar_h)) {
            sidebar_zoom = 12.5f;
            reset_any = true;
          }
          if (template_point_in_rect(mouse_x, mouse_y, layout.content_x,
                                     layout.content_y, layout.content_w,
                                     layout.content_h)) {
            panel_zoom = 12.5f;
            reset_any = true;
          }
          if (!reset_any) {
            sidebar_zoom = 12.5f;
            panel_zoom = 12.5f;
          }
          event_mutated = true;
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

    panel_tab_scroll = filenight_clamp_tab_scroll(ctx, font, &layout, panel_tabs,
                                                  panel_tab_count,
                                                  panel_tab_scroll);

    {
      int i;
      for (i = 0; i < panel_tab_count; ++i) {
        float target_x, target_w;
        filenight_tab_target_rect(ctx, font, &layout, panel_tabs,
                                  panel_tab_count, panel_tab_scroll, i, &target_x,
                                  &target_w);
        if (!panel_tab_visual_valid)
          panel_tab_visual_x[i] = target_x;
        else if (panel_drag_tab == i && panel_tab_drag_active)
          panel_tab_visual_x[i] = mouse_x - panel_drag_offset_x;
        else
          panel_tab_visual_x[i] += (target_x - panel_tab_visual_x[i]) * 0.24f;
        if (fabsf(panel_tab_visual_x[i] - target_x) > 0.35f)
          event_requested = true;
      }
      panel_tab_visual_valid = true;
    }

    if (panel_drag_tab >= 0 && panel_tab_drag_active) {
      int i;
      float drag_w;
      float drag_center;
      float edge_pad = 24.0f;
      filenight_tab_target_rect(ctx, font, &layout, panel_tabs, panel_tab_count,
                                panel_tab_scroll, panel_drag_tab, NULL, &drag_w);
      if (mouse_x < layout.tabs_x + edge_pad)
        panel_tab_scroll = filenight_clamp_tab_scroll(
            ctx, font, &layout, panel_tabs, panel_tab_count,
            panel_tab_scroll - 10.0f);
      else if (mouse_x > layout.tabs_x + layout.tabs_w - edge_pad)
        panel_tab_scroll = filenight_clamp_tab_scroll(
            ctx, font, &layout, panel_tabs, panel_tab_count,
            panel_tab_scroll + 10.0f);
      drag_center = panel_tab_visual_x[panel_drag_tab] + drag_w * 0.5f;
      for (i = 0; i < panel_tab_count; ++i) {
        float target_x, target_w;
        float target_center;
        char tmp[32];
        if (i == panel_drag_tab)
          continue;
        filenight_tab_target_rect(ctx, font, &layout, panel_tabs,
                                  panel_tab_count, panel_tab_scroll, i, &target_x,
                                  &target_w);
        target_center = target_x + target_w * 0.5f;
        if ((panel_drag_tab < i && drag_center > target_center) ||
            (panel_drag_tab > i && drag_center < target_center)) {
          FileNightPanelState panel_tmp;
          memcpy(tmp, panel_tabs[panel_drag_tab], sizeof(tmp));
          memcpy(panel_tabs[panel_drag_tab], panel_tabs[i], sizeof(panel_tabs[0]));
          memcpy(panel_tabs[i], tmp, sizeof(tmp));
          memcpy(&panel_tmp, &panel_states[panel_drag_tab], sizeof(panel_tmp));
          memcpy(&panel_states[panel_drag_tab], &panel_states[i],
                 sizeof(panel_states[0]));
          memcpy(&panel_states[i], &panel_tmp, sizeof(panel_tmp));
          {
            float tmp_x = panel_tab_visual_x[panel_drag_tab];
            panel_tab_visual_x[panel_drag_tab] = panel_tab_visual_x[i];
            panel_tab_visual_x[i] = tmp_x;
          }
          if (active_panel_tab == panel_drag_tab)
            active_panel_tab = i;
          else if (active_panel_tab == i)
            active_panel_tab = panel_drag_tab;
          panel_drag_tab = i;
          event_mutated = true;
          event_requested = true;
          break;
        }
      }
    }

    {
      int i;
      for (i = 0; i < 4; ++i) {
        float target = section_expanded[i] ? 1.0f : 0.0f;
        float current = section_open[i];
        float next = current + (target - current) * 0.34f;
        if (fabsf(next - target) < 0.01f)
          next = target;
        if (fabsf(next - current) > 0.0005f) {
          section_open[i] = next;
          event_requested = true;
        }
      }
    }

    {
      bool hovered_menu_now =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.menu_x);
      bool hovered_minimize_now =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.minimize_x);
      bool hovered_maximize_now =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.maximize_x);
      bool hovered_close_now =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.close_x);
      bool hovered_divider_now =
          template_point_in_rect(mouse_x, mouse_y, layout.divider_x,
                                 layout.divider_y, layout.divider_w,
                                 layout.divider_h);

      if (!hover_state_valid || hovered_menu_now != hovered_menu_prev ||
          hovered_minimize_now != hovered_minimize_prev ||
          hovered_maximize_now != hovered_maximize_prev ||
          hovered_close_now != hovered_close_prev ||
          hovered_divider_now != hovered_divider_prev) {
        hovered_menu_prev = hovered_menu_now;
        hovered_minimize_prev = hovered_minimize_now;
        hovered_maximize_prev = hovered_maximize_now;
        hovered_close_prev = hovered_close_now;
        hovered_divider_prev = hovered_divider_now;
        hover_state_valid = true;
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
    template_compute_layout(&layout, width, height, &hints, sidebar_w);

    stygian_begin_frame_intent(
        ctx, width, height,
        (event_eval_requested && !event_requested && !event_mutated &&
         !stygian_has_pending_repaint(ctx))
            ? STYGIAN_FRAME_EVAL_ONLY
            : STYGIAN_FRAME_RENDER);

    stygian_scope_begin(ctx, k_scope_shell);
    stygian_rect(ctx, 0.0f, 0.0f, (float)width, (float)height, FILENIGHT_SHELL_R,
                 FILENIGHT_SHELL_G, FILENIGHT_SHELL_B, 1.0f);
    // Draw slightly past the window bounds so the rounded-edge AA gets clipped
    // off instead of leaving a pale seam around the shell.
    stygian_rect_rounded(ctx, layout.shell_x - 2.0f, layout.shell_y - 2.0f,
                         layout.shell_w + 4.0f, layout.shell_h + 4.0f,
                         FILENIGHT_SHELL_R, FILENIGHT_SHELL_G, FILENIGHT_SHELL_B,
                         1.0f, 8.0f);
    {
      bool hovered_menu =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.menu_x);
      bool hovered_minimize =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.minimize_x);
      bool hovered_maximize =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.maximize_x);
      bool hovered_close =
          template_point_in_button(&layout, mouse_x, mouse_y, layout.close_x);
      bool hovered_divider =
          template_point_in_rect(mouse_x, mouse_y, layout.divider_x,
                                 layout.divider_y, layout.divider_w,
                                 layout.divider_h);
      bool maximized = stygian_window_is_maximized(window);
      float base_icon = 0.92f;
      float menu_icon = hovered_menu ? 0.99f : 0.94f;
      float minimize_icon = hovered_minimize ? 0.995f : base_icon;
      float maximize_icon = hovered_maximize ? 0.995f : base_icon;
      float close_icon = hovered_close ? 1.0f : 0.965f;
      float max_r, max_g, max_b;
      template_button_fill(hovered_maximize, hovered_maximize && mouse_down,
                           false, &max_r, &max_g, &max_b);
      template_draw_caption_button_bg(ctx, &layout, layout.menu_x, hovered_menu,
                                      hovered_menu && mouse_down, false);
      template_draw_menu_glyph(ctx, layout.menu_x, layout.button_y,
                               layout.button_w, layout.button_h, menu_icon);
      filenight_draw_body(ctx, font, &layout, mouse_x, mouse_y,
                          section_expanded, section_open, &sidebar, panel_states,
                          panel_tab_visual_x, panel_tabs, panel_tab_count,
                          active_panel_tab, sidebar_zoom, panel_zoom,
                          panel_tab_scroll, hovered_divider, divider_dragging);
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

    stygian_scope_invalidate_next(ctx, k_scope_shell);
    if (event_requested || event_mutated) {
      stygian_set_repaint_source(ctx, "filenight");
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
