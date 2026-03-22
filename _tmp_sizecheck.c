#include <stdio.h>
#include "editor/apps/stygian_editor_shell_canvas.c"
int main(void){
  printf("FrameNode=%zu\n", sizeof(ShellCanvasFrameNode));
  printf("Scene=%zu\n", sizeof(ShellCanvasSceneState));
  printf("History=%zu\n", sizeof(ShellCanvasHistoryState));
  printf("Dialog=%zu\n", sizeof(ShellDialogState));
  printf("Main-ish total=%zu\n", sizeof(ShellCanvasImportState)+sizeof(ShellCanvasSceneState)+sizeof(ShellCanvasHistoryState)+sizeof(ShellCanvasClipboardState)+sizeof(ShellCanvasInteractionState)+sizeof(ShellPathDraftState)+sizeof(ShellCanvasViewState)+sizeof(ShellCanvasSnapSettings)+sizeof(ShellCanvasGuideState)+sizeof(ShellInspectorState)+sizeof(ShellSidebarState)+sizeof(ShellToolDockState)+sizeof(ShellDialogState)+sizeof(ShellCommitState));
  return 0;
}
