#include "../include/stygian.h"
#include "../window/stygian_window.h"

int main(void) {
  char path[8] = "x";
  StygianNativeFileDialogOptions file_opts = {
      .title = "Open",
  };
  StygianNativeMessageDialogOptions msg_opts = {
      .title = "Message",
      .message = "Hello",
      .kind = STYGIAN_NATIVE_MESSAGE_INFO,
      .buttons = STYGIAN_NATIVE_MESSAGE_OK,
  };
  int rc = 0;

  if (stygian_window_native_file_dialog(NULL, STYGIAN_NATIVE_FILE_DIALOG_OPEN,
                                        &file_opts, path,
                                        sizeof(path)) !=
      STYGIAN_NATIVE_DIALOG_RESULT_ERROR) {
    rc |= 1;
  }
  if (path[0] != '\0')
    rc |= 2;
  if (stygian_window_native_message_dialog(NULL, &msg_opts) !=
      STYGIAN_NATIVE_DIALOG_RESULT_ERROR) {
    rc |= 4;
  }
  if (stygian_get_default_font(NULL) != 0u)
    rc |= 8;

  return rc;
}
