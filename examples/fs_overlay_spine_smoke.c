#include "../include/stygian_fs.h"
#include "../widgets/stygian_widgets.h"

#include <string.h>

static int fail_if(bool cond, int bit, int *rc) {
  if (cond)
    *rc |= bit;
  return *rc;
}

int main(void) {
  char path[STYGIAN_FS_PATH_CAP];
  char parent[STYGIAN_FS_PATH_CAP];
  char name[STYGIAN_FS_NAME_CAP];
  StygianFsStat st = {0};
  StygianFsList list = {0};
  StygianFsListOptions opts = {
      .flags = STYGIAN_FS_LIST_FILES | STYGIAN_FS_LIST_DIRECTORIES |
               STYGIAN_FS_LIST_SORT_DIRECTORIES_FIRST,
      .extension = ".h",
  };
  StygianPopover popover = {.open = true};
  StygianDrawer drawer = {.open = true};
  StygianSheet sheet = {.open = true};
  StygianModal modal = {.open = true, .close_on_backdrop = true};
  int rc = 0;
  uint32_t i;
  bool saw_header = false;

  fail_if(!stygian_fs_path_join("include", "stygian.h", path, sizeof(path)), 1,
          &rc);
  fail_if(strcmp(path, "include/stygian.h") != 0, 2, &rc);
  fail_if(!stygian_fs_path_parent("editor/apps/stygian_editor_shell_canvas.c",
                                  parent, sizeof(parent)),
          4, &rc);
  fail_if(strcmp(parent, "editor/apps") != 0, 8, &rc);
  fail_if(!stygian_fs_path_filename(path, name, sizeof(name)), 16, &rc);
  fail_if(strcmp(name, "stygian.h") != 0, 32, &rc);
  fail_if(!stygian_fs_path_has_extension(path, ".h", false), 64, &rc);

  fail_if(!stygian_fs_stat("include/stygian.h", &st), 128, &rc);
  fail_if(!st.exists || st.type != STYGIAN_FS_ENTRY_FILE, 256, &rc);

  fail_if(!stygian_fs_list("include", &opts, &list), 512, &rc);
  for (i = 0u; i < list.count; ++i) {
    if (strcmp(list.entries[i].name, "stygian.h") == 0) {
      saw_header = true;
      break;
    }
  }
  fail_if(!saw_header, 1024, &rc);
  stygian_fs_list_free(&list);

  fail_if(stygian_overlay_pointer_blocked(), 2048, &rc);
  fail_if(stygian_overlay_keyboard_blocked(), 4096, &rc);
  fail_if(stygian_popover_begin(NULL, &popover, 800.0f, 600.0f), 8192, &rc);
  fail_if(stygian_drawer_begin(NULL, &drawer, 800.0f, 600.0f), 16384, &rc);
  fail_if(stygian_sheet_begin(NULL, &sheet, 800.0f, 600.0f), 32768, &rc);
  fail_if(stygian_modal_begin(NULL, 0u, &modal, 800.0f, 600.0f), 65536, &rc);

  return rc;
}
