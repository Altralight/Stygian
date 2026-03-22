#define main stygian_editor_shell_canvas_app_main
#include "../apps/stygian_editor_shell_canvas.c"
#undef main

#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static ShellCanvasFrameNode *append_node(ShellCanvasSceneState *scene, int page_id,
                                         ShellCanvasNodeKind kind,
                                         const char *name, float x, float y,
                                         float w, float h) {
  ShellCanvasFrameNode *node = NULL;
  if (!scene || scene->frame_count >= SHELL_CANVAS_FRAME_CAP)
    return NULL;
  node = &scene->frames[scene->frame_count++];
  memset(node, 0, sizeof(*node));
  node->alive = true;
  node->visible = true;
  node->id = scene->next_frame_id++;
  node->page_id = page_id;
  node->kind = kind;
  node->x = x;
  node->y = y;
  node->w = w;
  node->h = h;
  node->radius = kind == SHELL_NODE_FRAME ? 18.0f : 10.0f;
  shell_canvas_apply_default_style(node);
  snprintf(node->name, sizeof(node->name), "%s", name);
  return node;
}

int main(void) {
  ShellCanvasSceneState scene;
  ShellAssetLibrary assets = {0};
  ShellCanvasFrameNode *shell_frame = NULL;
  ShellCanvasFrameNode *nav = NULL;
  ShellCanvasFrameNode *content = NULL;
  int page_one = 0;
  int page_two = 0;
  int count = 0;

  shell_canvas_scene_init(&scene);
  if (scene.page_count != 1u || scene.active_page_id != 1)
    return fail("scene init did not seed the first page");

  if (!shell_canvas_scene_add_page(&scene, "Files"))
    return fail("add page files failed");
  if (!shell_canvas_scene_add_page(&scene, "IDE"))
    return fail("add page ide failed");
  if (scene.page_count != 3u)
    return fail("page count mismatch");

  page_one = scene.pages[0].id;
  page_two = scene.pages[2].id;

  shell_canvas_scene_activate_page(&scene, page_one);
  shell_frame = append_node(&scene, page_one, SHELL_NODE_FRAME, "Browser Shell",
                            24.0f, 24.0f, 1280.0f, 800.0f);
  if (!shell_frame)
    return fail("append browser shell failed");
  nav = append_node(&scene, page_one, SHELL_NODE_FRAME, "Sidebar", 42.0f, 96.0f,
                    260.0f, 690.0f);
  content = append_node(&scene, page_one, SHELL_NODE_FRAME, "Content", 324.0f,
                        96.0f, 962.0f, 690.0f);
  if (!nav || !content)
    return fail("append primary frames failed");
  shell_canvas_scene_assign_parent(&scene, nav, shell_frame->id);
  shell_canvas_scene_assign_parent(&scene, content, shell_frame->id);
  for (int i = 0; i < 7; ++i) {
    char name[32];
    ShellCanvasFrameNode *node = NULL;
    snprintf(name, sizeof(name), "Nav Item %d", i + 1);
    node = append_node(&scene, page_one, SHELL_NODE_RECTANGLE, name, 60.0f,
                       126.0f + i * 72.0f, 220.0f, 44.0f);
    if (!node)
      return fail("append page one node failed");
    shell_canvas_scene_assign_parent(&scene, node, nav->id);
  }

  shell_canvas_scene_activate_page(&scene, page_two);
  shell_frame = append_node(&scene, page_two, SHELL_NODE_FRAME, "IDE Shell",
                            32.0f, 32.0f, 1400.0f, 860.0f);
  nav = append_node(&scene, page_two, SHELL_NODE_FRAME, "Activity Bar", 52.0f,
                    92.0f, 72.0f, 760.0f);
  content = append_node(&scene, page_two, SHELL_NODE_FRAME, "Editor Stack",
                        148.0f, 92.0f, 1248.0f, 760.0f);
  if (!shell_frame || !nav || !content)
    return fail("append page two frames failed");
  shell_canvas_scene_assign_parent(&scene, nav, shell_frame->id);
  shell_canvas_scene_assign_parent(&scene, content, shell_frame->id);
  for (int i = 0; i < 7; ++i) {
    char name[32];
    ShellCanvasFrameNode *node = NULL;
    snprintf(name, sizeof(name), "Editor Tab %d", i + 1);
    node = append_node(&scene, page_two, i == 0 ? SHELL_NODE_TEXT
                                                 : SHELL_NODE_RECTANGLE,
                       name, 176.0f + i * 148.0f, 110.0f, 132.0f, 36.0f);
    if (!node)
      return fail("append page two node failed");
    shell_canvas_scene_assign_parent(&scene, node, content->id);
  }
  if (!append_node(&scene, page_two, SHELL_NODE_RECTANGLE, "Floating Preview",
                   1180.0f, 180.0f, 180.0f, 120.0f)) {
    return fail("append orphan node failed");
  }

  if (scene.frame_count < 21u)
    return fail("expected at least 20 authored nodes");

  shell_canvas_scene_select_all(&scene);
  count = shell_canvas_scene_selection_count(&scene);
  if (count != 11)
    return fail("select all did not stay inside the active page");

  scene.frames[scene.frame_count - 1].visible = false;
  shell_canvas_scene_clear_selection_for_hidden(&scene);
  count = shell_canvas_scene_selection_count(&scene);
  if (count != 10)
    return fail("hidden nodes stayed selected");

  shell_canvas_scene_select_only(&scene, (int)scene.frame_count - 1);
  if (!shell_canvas_scene_reorder_selected(&scene, -1))
    return fail("layer reorder down failed");
  if (!shell_canvas_scene_reorder_selected(&scene, 1))
    return fail("layer reorder up failed");
  shell_canvas_scene_cycle_parent(&scene, &scene.frames[scene.frame_count - 1]);
  if (shell_canvas_scene_node_depth(&scene, &scene.frames[scene.frame_count - 1]) < 1)
    return fail("parent cycle did not attach the node");

  shell_asset_library_add(&assets, SHELL_DIALOG_IMPORT_IMAGE,
                          "assets\\ui\\folder.png");
  shell_asset_library_add(&assets, SHELL_DIALOG_IMPORT_FONT,
                          "assets\\fonts\\editor.ttf");
  shell_asset_library_add(&assets, SHELL_DIALOG_IMPORT_TEXTURE,
                          "assets\\textures\\noise.png");
  if (assets.entry_count != 3u)
    return fail("asset library did not track imported assets");

  printf("[PASS] workspace structure smoke\n");
  return 0;
}
