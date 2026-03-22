#include "../include/stygian_fs.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#endif

static int fs_tolower_ascii(int ch) {
  if (ch >= 'A' && ch <= 'Z')
    return ch - 'A' + 'a';
  return ch;
}

static int fs_stricmp_ascii(const char *a, const char *b) {
  int ca;
  int cb;
  if (!a)
    a = "";
  if (!b)
    b = "";
  while (*a && *b) {
    ca = fs_tolower_ascii((unsigned char)*a++);
    cb = fs_tolower_ascii((unsigned char)*b++);
    if (ca != cb)
      return ca - cb;
  }
  return fs_tolower_ascii((unsigned char)*a) -
         fs_tolower_ascii((unsigned char)*b);
}

static bool fs_contains_token(const char *haystack, const char *needle,
                              bool case_sensitive) {
  size_t i;
  size_t j;
  size_t hay_len;
  size_t needle_len;
  if (!needle || !needle[0])
    return true;
  if (!haystack)
    return false;
  hay_len = strlen(haystack);
  needle_len = strlen(needle);
  if (needle_len == 0u)
    return true;
  if (needle_len > hay_len)
    return false;
  for (i = 0; i + needle_len <= hay_len; ++i) {
    bool match = true;
    for (j = 0; j < needle_len; ++j) {
      int a = (unsigned char)haystack[i + j];
      int b = (unsigned char)needle[j];
      if (!case_sensitive) {
        a = fs_tolower_ascii(a);
        b = fs_tolower_ascii(b);
      }
      if (a != b) {
        match = false;
        break;
      }
    }
    if (match)
      return true;
  }
  return false;
}

static bool fs_path_is_absolute(const char *path) {
  if (!path || !path[0])
    return false;
  if (path[0] == '/' || path[0] == '\\')
    return true;
  if (((path[0] >= 'A' && path[0] <= 'Z') ||
       (path[0] >= 'a' && path[0] <= 'z')) &&
      path[1] == ':') {
    return true;
  }
  return false;
}

static const char *fs_path_basename_ptr(const char *path) {
  const char *last_fwd;
  const char *last_back;
  const char *last;
  if (!path)
    return "";
  last_fwd = strrchr(path, '/');
  last_back = strrchr(path, '\\');
  last = last_fwd;
  if (!last || (last_back && last_back > last))
    last = last_back;
  return last ? last + 1 : path;
}

static bool fs_copy_string(char *dst, size_t cap, const char *src) {
  size_t len;
  if (!dst || cap == 0u)
    return false;
  if (!src)
    src = "";
  len = strlen(src);
  if (len >= cap)
    return false;
  memcpy(dst, src, len + 1u);
  return true;
}

static void fs_normalize_separators(char *path) {
  size_t i;
  if (!path)
    return;
  for (i = 0; path[i]; ++i) {
    if (path[i] == '\\')
      path[i] = '/';
  }
}

static bool fs_make_native_path(const char *path, char *out_path,
                                size_t out_path_cap) {
  size_t i;
  if (!fs_copy_string(out_path, out_path_cap, path))
    return false;
#ifdef _WIN32
  for (i = 0; out_path[i]; ++i) {
    if (out_path[i] == '/')
      out_path[i] = '\\';
  }
#else
  (void)i;
#endif
  return true;
}

bool stygian_fs_path_normalize(const char *path, char *out_path,
                               size_t out_path_cap) {
  char scratch[STYGIAN_FS_PATH_CAP];
  char prefix[16];
  char *tokens[128];
  uint32_t token_count = 0u;
  char *cursor;
  size_t prefix_len = 0u;
  bool absolute = false;
  uint32_t i;
  if (!path || !out_path || out_path_cap == 0u)
    return false;
  if (!fs_copy_string(scratch, sizeof(scratch), path))
    return false;
  fs_normalize_separators(scratch);

  prefix[0] = '\0';
  cursor = scratch;
  if (((cursor[0] >= 'A' && cursor[0] <= 'Z') ||
       (cursor[0] >= 'a' && cursor[0] <= 'z')) &&
      cursor[1] == ':') {
    prefix[0] = cursor[0];
    prefix[1] = ':';
    prefix[2] = '\0';
    prefix_len = 2u;
    cursor += 2;
    if (*cursor == '/') {
      absolute = true;
      ++cursor;
    }
  } else if (cursor[0] == '/' && cursor[1] == '/') {
    prefix[0] = '/';
    prefix[1] = '/';
    prefix[2] = '\0';
    prefix_len = 2u;
    absolute = true;
    cursor += 2;
  } else if (cursor[0] == '/') {
    prefix[0] = '/';
    prefix[1] = '\0';
    prefix_len = 1u;
    absolute = true;
    ++cursor;
  }

  while (*cursor) {
    char *segment = cursor;
    while (*cursor && *cursor != '/')
      ++cursor;
    if (*cursor == '/') {
      *cursor = '\0';
      ++cursor;
    }
    if (segment[0] == '\0' || strcmp(segment, ".") == 0)
      continue;
    if (strcmp(segment, "..") == 0) {
      if (token_count > 0u && strcmp(tokens[token_count - 1u], "..") != 0) {
        --token_count;
      } else if (!absolute && token_count < 128u) {
        tokens[token_count++] = segment;
      }
      continue;
    }
    if (token_count < 128u)
      tokens[token_count++] = segment;
  }

  out_path[0] = '\0';
  if (prefix_len > 0u) {
    if (!fs_copy_string(out_path, out_path_cap, prefix))
      return false;
    if (absolute && prefix_len == 2u && prefix[1] == ':') {
      if (strlen(out_path) + 1u >= out_path_cap)
        return false;
      strcat(out_path, "/");
    }
  }
  for (i = 0u; i < token_count; ++i) {
    size_t need_slash = 0u;
    size_t cur_len = strlen(out_path);
    const char *segment = tokens[i];
    if (cur_len > 0u && out_path[cur_len - 1u] != '/')
      need_slash = 1u;
    if (cur_len + need_slash + strlen(segment) + 1u >= out_path_cap)
      return false;
    if (need_slash)
      strcat(out_path, "/");
    strcat(out_path, segment);
  }

  if (out_path[0] == '\0') {
    if (absolute)
      return fs_copy_string(out_path, out_path_cap, prefix_len ? prefix : "/");
    return fs_copy_string(out_path, out_path_cap, ".");
  }
  return true;
}

bool stygian_fs_path_join(const char *lhs, const char *rhs, char *out_path,
                          size_t out_path_cap) {
  char combined[STYGIAN_FS_PATH_CAP];
  size_t lhs_len;
  if (!rhs || !rhs[0])
    return stygian_fs_path_normalize(lhs ? lhs : ".", out_path, out_path_cap);
  if (fs_path_is_absolute(rhs))
    return stygian_fs_path_normalize(rhs, out_path, out_path_cap);
  if (!lhs || !lhs[0])
    return stygian_fs_path_normalize(rhs, out_path, out_path_cap);
  if (!fs_copy_string(combined, sizeof(combined), lhs))
    return false;
  lhs_len = strlen(combined);
  if (lhs_len > 0u && combined[lhs_len - 1u] != '/' &&
      combined[lhs_len - 1u] != '\\') {
    if (lhs_len + 1u >= sizeof(combined))
      return false;
    combined[lhs_len++] = '/';
    combined[lhs_len] = '\0';
  }
  if (lhs_len + strlen(rhs) + 1u >= sizeof(combined))
    return false;
  strcat(combined, rhs);
  return stygian_fs_path_normalize(combined, out_path, out_path_cap);
}

bool stygian_fs_path_parent(const char *path, char *out_path,
                            size_t out_path_cap) {
  char normalized[STYGIAN_FS_PATH_CAP];
  char *last;
  if (!stygian_fs_path_normalize(path, normalized, sizeof(normalized)))
    return false;
  last = strrchr(normalized, '/');
  if (!last) {
    return fs_copy_string(out_path, out_path_cap, ".");
  }
  if (last == normalized) {
    last[1] = '\0';
    return fs_copy_string(out_path, out_path_cap, normalized);
  }
  if (last == normalized + 2 && normalized[1] == ':') {
    last[1] = '\0';
    return fs_copy_string(out_path, out_path_cap, normalized);
  }
  *last = '\0';
  return fs_copy_string(out_path, out_path_cap, normalized);
}

bool stygian_fs_path_filename(const char *path, char *out_name,
                              size_t out_name_cap) {
  const char *name = fs_path_basename_ptr(path);
  return fs_copy_string(out_name, out_name_cap, name);
}

bool stygian_fs_path_has_extension(const char *path, const char *extension,
                                   bool case_sensitive) {
  const char *dot;
  if (!path || !extension || !extension[0])
    return false;
  dot = strrchr(fs_path_basename_ptr(path), '.');
  if (!dot)
    return false;
  if (extension[0] == '.') {
    if (case_sensitive)
      return strcmp(dot, extension) == 0;
    return fs_stricmp_ascii(dot, extension) == 0;
  }
  if (case_sensitive)
    return strcmp(dot + 1, extension) == 0;
  return fs_stricmp_ascii(dot + 1, extension) == 0;
}

static bool fs_entry_matches_filter(const StygianFsEntry *entry,
                                    const StygianFsListOptions *options) {
  uint32_t flags;
  bool case_sensitive;
  bool want_files;
  bool want_dirs;
  if (!entry)
    return false;
  flags = options ? options->flags : 0u;
  case_sensitive = (flags & STYGIAN_FS_LIST_CASE_SENSITIVE) != 0u;
  want_files = (flags & STYGIAN_FS_LIST_FILES) != 0u;
  want_dirs = (flags & STYGIAN_FS_LIST_DIRECTORIES) != 0u;
  if (!want_files && !want_dirs) {
    want_files = true;
    want_dirs = true;
  }
  if (!((entry->stat.type == STYGIAN_FS_ENTRY_FILE && want_files) ||
        (entry->stat.type == STYGIAN_FS_ENTRY_DIRECTORY && want_dirs) ||
        (entry->stat.type != STYGIAN_FS_ENTRY_FILE &&
         entry->stat.type != STYGIAN_FS_ENTRY_DIRECTORY && want_files))) {
    return false;
  }
  if (!(flags & STYGIAN_FS_LIST_INCLUDE_HIDDEN) && entry->stat.hidden)
    return false;
  if (options && options->name_contains &&
      !fs_contains_token(entry->name, options->name_contains, case_sensitive)) {
    return false;
  }
  if (options && options->extension && options->extension[0] &&
      entry->stat.type == STYGIAN_FS_ENTRY_FILE &&
      !stygian_fs_path_has_extension(entry->name, options->extension,
                                     case_sensitive)) {
    return false;
  }
  return true;
}

static int fs_compare_entry(const StygianFsEntry *a, const StygianFsEntry *b,
                            const StygianFsListOptions *options) {
  uint32_t flags = options ? options->flags : 0u;
  int cmp;
  if ((flags & STYGIAN_FS_LIST_SORT_DIRECTORIES_FIRST) &&
      a->stat.type != b->stat.type) {
    if (a->stat.type == STYGIAN_FS_ENTRY_DIRECTORY)
      return -1;
    if (b->stat.type == STYGIAN_FS_ENTRY_DIRECTORY)
      return 1;
  }
  if (flags & STYGIAN_FS_LIST_CASE_SENSITIVE)
    cmp = strcmp(a->name, b->name);
  else
    cmp = fs_stricmp_ascii(a->name, b->name);
  if (flags & STYGIAN_FS_LIST_SORT_NAME_DESC)
    cmp = -cmp;
  return cmp;
}

static void fs_sort_entries(StygianFsEntry *entries, uint32_t count,
                            const StygianFsListOptions *options) {
  uint32_t i;
  if (!entries || count < 2u)
    return;
  for (i = 1u; i < count; ++i) {
    StygianFsEntry key = entries[i];
    uint32_t j = i;
    while (j > 0u &&
           fs_compare_entry(&entries[j - 1u], &key, options) > 0) {
      entries[j] = entries[j - 1u];
      --j;
    }
    entries[j] = key;
  }
}

static bool fs_list_push(StygianFsList *list, const StygianFsEntry *entry) {
  StygianFsEntry *grown;
  uint32_t new_cap;
  if (!list || !entry)
    return false;
  if (list->count == list->capacity) {
    new_cap = list->capacity ? list->capacity * 2u : 32u;
    grown = (StygianFsEntry *)realloc(list->entries,
                                      sizeof(StygianFsEntry) * new_cap);
    if (!grown)
      return false;
    list->entries = grown;
    list->capacity = new_cap;
  }
  list->entries[list->count++] = *entry;
  return true;
}

#ifdef _WIN32
static uint64_t fs_filetime_to_unix_ms(FILETIME ft) {
  ULARGE_INTEGER ticks;
  ticks.LowPart = ft.dwLowDateTime;
  ticks.HighPart = ft.dwHighDateTime;
  if (ticks.QuadPart < 116444736000000000ULL)
    return 0u;
  return (ticks.QuadPart - 116444736000000000ULL) / 10000ULL;
}

static bool fs_fill_stat_from_win32(const WIN32_FILE_ATTRIBUTE_DATA *data,
                                    const char *name, StygianFsStat *out_stat) {
  ULARGE_INTEGER size;
  if (!data || !out_stat)
    return false;
  memset(out_stat, 0, sizeof(*out_stat));
  out_stat->exists = true;
  if (data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    out_stat->type = STYGIAN_FS_ENTRY_DIRECTORY;
  else
    out_stat->type = STYGIAN_FS_ENTRY_FILE;
  size.LowPart = data->nFileSizeLow;
  size.HighPart = data->nFileSizeHigh;
  out_stat->size_bytes = size.QuadPart;
  out_stat->modified_unix_ms = fs_filetime_to_unix_ms(data->ftLastWriteTime);
  out_stat->hidden = ((data->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0u);
  if (!out_stat->hidden && name && name[0] == '.' && strcmp(name, ".") != 0 &&
      strcmp(name, "..") != 0) {
    out_stat->hidden = true;
  }
  out_stat->read_only = (data->dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0u;
  return true;
}
#else
static uint64_t fs_timespec_to_unix_ms(struct timespec ts) {
  return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000L);
}

static bool fs_fill_stat_from_posix(const struct stat *st, const char *name,
                                    const char *full_path,
                                    StygianFsStat *out_stat) {
  if (!st || !out_stat)
    return false;
  memset(out_stat, 0, sizeof(*out_stat));
  out_stat->exists = true;
  if (S_ISDIR(st->st_mode))
    out_stat->type = STYGIAN_FS_ENTRY_DIRECTORY;
  else if (S_ISREG(st->st_mode))
    out_stat->type = STYGIAN_FS_ENTRY_FILE;
  else
    out_stat->type = STYGIAN_FS_ENTRY_OTHER;
  out_stat->size_bytes = (uint64_t)st->st_size;
#if defined(__APPLE__)
  out_stat->modified_unix_ms =
      (uint64_t)st->st_mtimespec.tv_sec * 1000ULL +
      (uint64_t)(st->st_mtimespec.tv_nsec / 1000000L);
#else
  out_stat->modified_unix_ms = (uint64_t)st->st_mtime * 1000ULL;
#endif
  out_stat->hidden = name && name[0] == '.' && strcmp(name, ".") != 0 &&
                     strcmp(name, "..") != 0;
  out_stat->read_only = full_path ? access(full_path, W_OK) != 0 : false;
  return true;
}
#endif

bool stygian_fs_stat(const char *path, StygianFsStat *out_stat) {
  char normalized[STYGIAN_FS_PATH_CAP];
  char native[STYGIAN_FS_PATH_CAP];
  if (!out_stat)
    return false;
  memset(out_stat, 0, sizeof(*out_stat));
  if (!path || !path[0])
    return false;
  if (!stygian_fs_path_normalize(path, normalized, sizeof(normalized)))
    return false;
  if (!fs_make_native_path(normalized, native, sizeof(native)))
    return false;
#ifdef _WIN32
  WIN32_FILE_ATTRIBUTE_DATA data;
  const char *name = fs_path_basename_ptr(normalized);
  if (!GetFileAttributesExA(native, GetFileExInfoStandard, &data))
    return false;
  return fs_fill_stat_from_win32(&data, name, out_stat);
#else
  struct stat st;
  const char *name = fs_path_basename_ptr(normalized);
  if (stat(native, &st) != 0)
    return false;
  return fs_fill_stat_from_posix(&st, name, native, out_stat);
#endif
}

bool stygian_fs_list(const char *directory, const StygianFsListOptions *options,
                     StygianFsList *out_list) {
  char normalized[STYGIAN_FS_PATH_CAP];
  char native[STYGIAN_FS_PATH_CAP];
  StygianFsList result = {0};
  if (!out_list)
    return false;
  if (!directory || !directory[0])
    directory = ".";
  if (!stygian_fs_path_normalize(directory, normalized, sizeof(normalized)))
    return false;
  if (!fs_make_native_path(normalized, native, sizeof(native)))
    return false;

#ifdef _WIN32
  {
    char pattern[STYGIAN_FS_PATH_CAP];
    WIN32_FIND_DATAA data;
    HANDLE find;
    if (!fs_copy_string(pattern, sizeof(pattern), native))
      return false;
    if (!pattern[0])
      return false;
    if (pattern[strlen(pattern) - 1u] != '\\' &&
        pattern[strlen(pattern) - 1u] != '/') {
      if (strlen(pattern) + 2u >= sizeof(pattern))
        return false;
      strcat(pattern, "\\");
    }
    strcat(pattern, "*");
    find = FindFirstFileA(pattern, &data);
    if (find == INVALID_HANDLE_VALUE)
      return false;
    do {
      StygianFsEntry entry;
      if (strcmp(data.cFileName, ".") == 0 || strcmp(data.cFileName, "..") == 0)
        continue;
      memset(&entry, 0, sizeof(entry));
      if (!fs_copy_string(entry.name, sizeof(entry.name), data.cFileName))
        continue;
      if (!stygian_fs_path_join(normalized, data.cFileName, entry.path,
                                sizeof(entry.path))) {
        continue;
      }
      if (!fs_fill_stat_from_win32(&(WIN32_FILE_ATTRIBUTE_DATA){
                                       .dwFileAttributes = data.dwFileAttributes,
                                       .ftCreationTime = data.ftCreationTime,
                                       .ftLastAccessTime = data.ftLastAccessTime,
                                       .ftLastWriteTime = data.ftLastWriteTime,
                                       .nFileSizeHigh = data.nFileSizeHigh,
                                       .nFileSizeLow = data.nFileSizeLow,
                                   },
                                   entry.name, &entry.stat)) {
        continue;
      }
      if (!fs_entry_matches_filter(&entry, options))
        continue;
      if (!fs_list_push(&result, &entry)) {
        FindClose(find);
        stygian_fs_list_free(&result);
        return false;
      }
      if (options && options->max_entries > 0u &&
          result.count >= options->max_entries) {
        break;
      }
    } while (FindNextFileA(find, &data));
    FindClose(find);
  }
#else
  {
    DIR *dir = opendir(native);
    struct dirent *entry_dir;
    if (!dir)
      return false;
    while ((entry_dir = readdir(dir)) != NULL) {
      StygianFsEntry entry;
      char joined_native[STYGIAN_FS_PATH_CAP];
      struct stat st;
      if (strcmp(entry_dir->d_name, ".") == 0 ||
          strcmp(entry_dir->d_name, "..") == 0) {
        continue;
      }
      memset(&entry, 0, sizeof(entry));
      if (!fs_copy_string(entry.name, sizeof(entry.name), entry_dir->d_name))
        continue;
      if (!stygian_fs_path_join(normalized, entry_dir->d_name, entry.path,
                                sizeof(entry.path)))
        continue;
      if (!fs_make_native_path(entry.path, joined_native, sizeof(joined_native)))
        continue;
      if (stat(joined_native, &st) != 0)
        continue;
      if (!fs_fill_stat_from_posix(&st, entry.name, joined_native, &entry.stat))
        continue;
      if (!fs_entry_matches_filter(&entry, options))
        continue;
      if (!fs_list_push(&result, &entry)) {
        closedir(dir);
        stygian_fs_list_free(&result);
        return false;
      }
      if (options && options->max_entries > 0u &&
          result.count >= options->max_entries) {
        break;
      }
    }
    closedir(dir);
  }
#endif

  fs_sort_entries(result.entries, result.count, options);
  *out_list = result;
  return true;
}

void stygian_fs_list_free(StygianFsList *list) {
  if (!list)
    return;
  free(list->entries);
  list->entries = NULL;
  list->count = 0u;
  list->capacity = 0u;
}
