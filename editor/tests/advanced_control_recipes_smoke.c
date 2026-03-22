#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *editor = NULL;
  StygianEditorNodeId root = STYGIAN_EDITOR_INVALID_ID;
  uint32_t before = 0u;
  uint32_t after = 0u;
  static char c23[524288];
  size_t need = 0u;

  cfg.max_nodes = 512u;
  cfg.max_path_points = 4096u;
  cfg.max_behaviors = 128u;
  cfg.max_color_tokens = 32u;
  editor = stygian_editor_create(&cfg, NULL);
  if (!editor)
    return fail("create editor failed");

  before = stygian_editor_node_count(editor);
  if (!stygian_editor_add_control_recipe(editor, STYGIAN_EDITOR_RECIPE_BUTTON,
                                         20.0f, 20.0f, 160.0f, 40.0f, "Button",
                                         &root) ||
      !root) {
    return fail("button recipe failed");
  }
  if (!stygian_editor_add_control_recipe(editor, STYGIAN_EDITOR_RECIPE_SLIDER,
                                         20.0f, 74.0f, 200.0f, 24.0f, "Slider",
                                         &root) ||
      !root) {
    return fail("slider recipe failed");
  }
  if (!stygian_editor_add_control_recipe(editor, STYGIAN_EDITOR_RECIPE_TOGGLE,
                                         20.0f, 110.0f, 64.0f, 28.0f, "Toggle",
                                         &root) ||
      !root) {
    return fail("toggle recipe failed");
  }
  if (!stygian_editor_add_control_recipe(
          editor, STYGIAN_EDITOR_RECIPE_CHECKBOX, 20.0f, 150.0f, 180.0f, 24.0f,
          "Checkbox", &root) ||
      !root) {
    return fail("checkbox recipe failed");
  }
  if (!stygian_editor_add_control_recipe(
          editor, STYGIAN_EDITOR_RECIPE_RADIO_GROUP, 20.0f, 184.0f, 220.0f,
          80.0f, "Radio", &root) ||
      !root) {
    return fail("radio recipe failed");
  }
  if (!stygian_editor_add_control_recipe(
          editor, STYGIAN_EDITOR_RECIPE_INPUT_FIELD, 20.0f, 274.0f, 220.0f,
          34.0f, "Input", &root) ||
      !root) {
    return fail("input recipe failed");
  }
  if (!stygian_editor_add_control_recipe(editor, STYGIAN_EDITOR_RECIPE_DROPDOWN,
                                         20.0f, 318.0f, 220.0f, 34.0f, "Dropdown",
                                         &root) ||
      !root) {
    return fail("dropdown recipe failed");
  }
  if (!stygian_editor_add_control_recipe(
          editor, STYGIAN_EDITOR_RECIPE_SCROLL_REGION, 260.0f, 20.0f, 220.0f,
          120.0f, "Scroll", &root) ||
      !root) {
    return fail("scroll recipe failed");
  }
  if (!stygian_editor_add_control_recipe(editor, STYGIAN_EDITOR_RECIPE_TABS,
                                         260.0f, 150.0f, 220.0f, 30.0f, "Tabs",
                                         &root) ||
      !root) {
    return fail("tabs recipe failed");
  }
  if (!stygian_editor_add_control_recipe(
          editor, STYGIAN_EDITOR_RECIPE_ACCORDION, 260.0f, 190.0f, 220.0f, 80.0f,
          "Accordion", &root) ||
      !root) {
    return fail("accordion recipe failed");
  }
  if (!stygian_editor_add_control_recipe(editor, STYGIAN_EDITOR_RECIPE_DATA_TABLE,
                                         260.0f, 280.0f, 280.0f, 130.0f, "Table",
                                         &root) ||
      !root) {
    return fail("table recipe failed");
  }
  if (!stygian_editor_add_control_recipe(editor, STYGIAN_EDITOR_RECIPE_DATA_CARD,
                                         560.0f, 20.0f, 180.0f, 220.0f, "Card",
                                         &root) ||
      !root) {
    return fail("card recipe failed");
  }

  after = stygian_editor_node_count(editor);
  if (after <= before + 12u)
    return fail("advanced recipes did not create expected scaffolds");

  need = stygian_editor_build_c23(editor, NULL, 0u);
  if (need < 2u || need > sizeof(c23))
    return fail("build c23 size failed");
  if (stygian_editor_build_c23(editor, c23, sizeof(c23)) != need)
    return fail("build c23 failed");

  stygian_editor_destroy(editor);
  printf("[PASS] editor advanced control recipes smoke\n");
  return 0;
}
