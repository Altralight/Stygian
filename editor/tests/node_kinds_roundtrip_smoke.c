#include "../include/stygian_editor.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

int main(void) {
  StygianEditorConfig cfg = stygian_editor_config_default();
  StygianEditor *a = NULL;
  StygianEditor *b = NULL;
  size_t json_need = 0u;
  char json_out[131072];
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
  uint32_t i;
  const char *required_kinds[] = {"k=frame",          "k=text",
                                  "k=image",          "k=line",
                                  "k=arrow",          "k=polygon",
                                  "k=star",           "k=arc",
                                  "k=group",          "k=component_def",
                                  "k=component_instance"};
  const StygianEditorShapeKind expected_kinds[] = {
      STYGIAN_EDITOR_SHAPE_FRAME,      STYGIAN_EDITOR_SHAPE_TEXT,
      STYGIAN_EDITOR_SHAPE_IMAGE,      STYGIAN_EDITOR_SHAPE_LINE,
      STYGIAN_EDITOR_SHAPE_ARROW,      STYGIAN_EDITOR_SHAPE_POLYGON,
      STYGIAN_EDITOR_SHAPE_STAR,       STYGIAN_EDITOR_SHAPE_ARC,
      STYGIAN_EDITOR_SHAPE_GROUP,      STYGIAN_EDITOR_SHAPE_COMPONENT_DEF,
      STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE};
  const char *json =
      "{\n"
      "  \"schema_name\": \"stygian-editor-project\",\n"
      "  \"schema_major\": 0,\n"
      "  \"schema_minor\": 0,\n"
      "  \"viewport\": \"w=1280.000000;h=720.000000;px=0.000000;py=0.000000;z=1.000000\",\n"
      "  \"grid\": \"en=1;sub=1;maj=96.000000;div=4;min=8.000000;tol=-1.000000\",\n"
      "  \"next_ids\": \"node=100;path=1;behavior=1\",\n"
      "  \"selection\": \"primary=0;ids=\",\n"
      "  \"color_tokens\": [],\n"
      "  \"nodes\": [\n"
      "    \"id=1;k=frame;v=1;z=1.000000;val=0.000000;tok=;x=10.000000;y=10.000000;w=200.000000;h=120.000000;clip=1;fill=0.100000,0.120000,0.140000,1.000000\",\n"
      "    \"id=2;k=text;v=1;z=2.000000;val=0.000000;tok=;x=20.000000;y=20.000000;w=180.000000;h=48.000000;size=16.000000;fill=0.900000,0.900000,0.900000,1.000000;txt=hello\",\n"
      "    \"id=3;k=image;v=1;z=3.000000;val=0.000000;tok=;x=40.000000;y=70.000000;w=96.000000;h=96.000000;fit=1;src=assets/icon.png\",\n"
      "    \"id=4;k=line;v=1;z=4.000000;val=0.000000;tok=;x1=10.000000;y1=200.000000;x2=200.000000;y2=220.000000;th=2.000000;stroke=0.300000,0.700000,1.000000,1.000000\",\n"
      "    \"id=5;k=arrow;v=1;z=5.000000;val=0.000000;tok=;x1=20.000000;y1=240.000000;x2=220.000000;y2=260.000000;th=3.000000;hs=10.000000;stroke=1.000000,0.800000,0.200000,1.000000\",\n"
      "    \"id=6;k=polygon;v=1;z=6.000000;val=0.000000;tok=;x=260.000000;y=40.000000;w=120.000000;h=120.000000;sides=6;corner=4.000000;fill=0.400000,0.500000,0.900000,0.900000\",\n"
      "    \"id=7;k=star;v=1;z=7.000000;val=0.000000;tok=;x=400.000000;y=40.000000;w=120.000000;h=120.000000;pts=5;inner=0.450000;fill=0.900000,0.600000,0.100000,0.900000\",\n"
      "    \"id=8;k=arc;v=1;z=8.000000;val=0.000000;tok=;x=540.000000;y=40.000000;w=120.000000;h=120.000000;start=0.000000;sweep=180.000000;th=4.000000;stroke=0.200000,1.000000,0.700000,1.000000\",\n"
      "    \"id=9;k=group;v=1;z=9.000000;val=0.000000;tok=;x=260.000000;y=200.000000;w=240.000000;h=140.000000;clip=0\",\n"
      "    \"id=10;k=component_def;v=1;z=10.000000;val=0.000000;tok=;x=520.000000;y=200.000000;w=220.000000;h=120.000000;sym=ButtonPrimary\",\n"
      "    \"id=11;k=component_instance;v=1;z=11.000000;val=0.000000;tok=;x=760.000000;y=200.000000;w=220.000000;h=120.000000;ref=ButtonPrimary\"\n"
      "  ],\n"
      "  \"behaviors\": []\n"
      "}\n";

  cfg.max_nodes = 256u;
  cfg.max_path_points = 4096u;
  cfg.max_behaviors = 256u;
  cfg.max_color_tokens = 64u;

  a = stygian_editor_create(&cfg, NULL);
  b = stygian_editor_create(&cfg, NULL);
  if (!a || !b)
    return fail("editor create failed");
  if (!stygian_editor_load_project_json(a, json))
    return fail("load project json failed");
  if (stygian_editor_node_count(a) != 11u)
    return fail("node count mismatch");

  for (i = 0u; i < 11u; ++i) {
    StygianEditorShapeKind kind = STYGIAN_EDITOR_SHAPE_RECT;
    if (!stygian_editor_node_get_shape_kind(a, i + 1u, &kind))
      return fail("shape kind read failed");
    if (kind != expected_kinds[i])
      return fail("shape kind corruption");
    if (!stygian_editor_node_get_bounds(a, i + 1u, &x, &y, &w, &h))
      return fail("bounds lookup failed for loaded kind");
  }

  json_need = stygian_editor_build_project_json(a, NULL, 0u);
  if (json_need == 0u || json_need > sizeof(json_out))
    return fail("build_project_json size failed");
  if (stygian_editor_build_project_json(a, json_out, sizeof(json_out)) != json_need)
    return fail("build_project_json output failed");

  for (i = 0u; i < sizeof(required_kinds) / sizeof(required_kinds[0]); ++i) {
    if (!strstr(json_out, required_kinds[i]))
      return fail("serialized kind missing from roundtrip output");
  }

  if (!stygian_editor_load_project_json(b, json_out))
    return fail("reload roundtrip json failed");
  if (stygian_editor_node_count(b) != 11u)
    return fail("reloaded node count mismatch");

  stygian_editor_destroy(a);
  stygian_editor_destroy(b);
  printf("[PASS] editor node kinds roundtrip smoke\n");
  return 0;
}
