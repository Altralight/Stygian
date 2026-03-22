#ifndef STYGIAN_FS_H
#define STYGIAN_FS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STYGIAN_FS_PATH_CAP
#define STYGIAN_FS_PATH_CAP 1024
#endif

#ifndef STYGIAN_FS_NAME_CAP
#define STYGIAN_FS_NAME_CAP 256
#endif

typedef enum StygianFsEntryType {
  STYGIAN_FS_ENTRY_UNKNOWN = 0,
  STYGIAN_FS_ENTRY_FILE = 1,
  STYGIAN_FS_ENTRY_DIRECTORY = 2,
  STYGIAN_FS_ENTRY_OTHER = 3,
} StygianFsEntryType;

typedef struct StygianFsStat {
  StygianFsEntryType type;
  uint64_t size_bytes;
  uint64_t modified_unix_ms;
  bool exists;
  bool hidden;
  bool read_only;
} StygianFsStat;

typedef struct StygianFsEntry {
  char name[STYGIAN_FS_NAME_CAP];
  char path[STYGIAN_FS_PATH_CAP];
  StygianFsStat stat;
} StygianFsEntry;

typedef uint32_t StygianFsListFlags;
#define STYGIAN_FS_LIST_FILES (1u << 0)
#define STYGIAN_FS_LIST_DIRECTORIES (1u << 1)
#define STYGIAN_FS_LIST_INCLUDE_HIDDEN (1u << 2)
#define STYGIAN_FS_LIST_CASE_SENSITIVE (1u << 3)
#define STYGIAN_FS_LIST_SORT_DIRECTORIES_FIRST (1u << 4)
#define STYGIAN_FS_LIST_SORT_NAME_DESC (1u << 5)

typedef struct StygianFsListOptions {
  StygianFsListFlags flags;
  const char *name_contains;
  const char *extension;
  uint32_t max_entries;
} StygianFsListOptions;

typedef struct StygianFsList {
  StygianFsEntry *entries;
  uint32_t count;
  uint32_t capacity;
} StygianFsList;

bool stygian_fs_stat(const char *path, StygianFsStat *out_stat);
bool stygian_fs_list(const char *directory, const StygianFsListOptions *options,
                     StygianFsList *out_list);
void stygian_fs_list_free(StygianFsList *list);

bool stygian_fs_path_join(const char *lhs, const char *rhs, char *out_path,
                          size_t out_path_cap);
bool stygian_fs_path_normalize(const char *path, char *out_path,
                               size_t out_path_cap);
bool stygian_fs_path_parent(const char *path, char *out_path,
                            size_t out_path_cap);
bool stygian_fs_path_filename(const char *path, char *out_name,
                              size_t out_name_cap);
bool stygian_fs_path_has_extension(const char *path, const char *extension,
                                   bool case_sensitive);

#ifdef __cplusplus
}
#endif

#endif
