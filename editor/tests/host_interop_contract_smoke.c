#include "../include/stygian_editor_module.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct HostProbe {
  uint64_t now_ms_value;
  uint32_t log_calls;
  uint32_t repaint_calls;
  uint32_t navigate_calls;
} HostProbe;

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static uint64_t host_now_ms(void *user_data) {
  HostProbe *probe = (HostProbe *)user_data;
  if (!probe)
    return 0u;
  return probe->now_ms_value;
}

static void host_log(void *user_data, StygianEditorLogLevel level,
                     const char *message) {
  HostProbe *probe = (HostProbe *)user_data;
  (void)level;
  (void)message;
  if (probe)
    probe->log_calls += 1u;
}

static void host_request_repaint_hz(void *user_data, uint32_t hz) {
  HostProbe *probe = (HostProbe *)user_data;
  (void)hz;
  if (probe)
    probe->repaint_calls += 1u;
}

static void host_navigate(void *user_data, const char *target) {
  HostProbe *probe = (HostProbe *)user_data;
  (void)target;
  if (probe)
    probe->navigate_calls += 1u;
}

int main(void) {
  const StygianEditorModuleApi *api = NULL;
  uint32_t abi_version = stygian_editor_module_abi_version();
  uint32_t major = STYGIAN_EDITOR_MODULE_ABI_VERSION_MAJOR(abi_version);
  uint32_t minor = STYGIAN_EDITOR_MODULE_ABI_VERSION_MINOR(abi_version);
  HostProbe probe = {0};
  StygianEditorHost host = {0};
  StygianEditorConfig cfg;
  StygianEditor *editor = NULL;
  StygianEditorRectDesc rect = {0};
  StygianEditorNodeId node_id = STYGIAN_EDITOR_INVALID_ID;
  static char json_buffer[65536];
  size_t json_need = 0u;
  const char *project_path = "editor/build/windows/host_interop_contract_tmp.stygian";

  api = stygian_editor_module_get_api(abi_version);
  if (!api)
    return fail("module get_api failed for exact version");

  if (stygian_editor_module_get_api(((major + 1u) << 16) | minor))
    return fail("module get_api accepted mismatched major");
  if (stygian_editor_module_get_api((major << 16) | (minor + 1u)))
    return fail("module get_api accepted unsupported newer minor");

  if (!api->create || !api->destroy || !api->set_host || !api->reset ||
      !api->add_rect || !api->node_count || !api->build_project_json ||
      !api->load_project_json || !api->save_project_file ||
      !api->load_project_file) {
    return fail("module api missing host-contract function pointers");
  }

  cfg = api->config_default();
  cfg.max_nodes = 128u;
  cfg.max_path_points = 512u;
  cfg.max_behaviors = 64u;
  cfg.max_color_tokens = 16u;

  probe.now_ms_value = 4242u;
  host.user_data = &probe;
  host.now_ms = host_now_ms;
  host.log = host_log;
  host.request_repaint_hz = host_request_repaint_hz;
  host.navigate = host_navigate;

  editor = api->create(&cfg, &host);
  if (!editor)
    return fail("module api create failed");

  api->tick(editor, 0u);
  api->set_host(editor, &host);

  rect.x = 24.0f;
  rect.y = 20.0f;
  rect.w = 100.0f;
  rect.h = 52.0f;
  rect.radius[0] = rect.radius[1] = rect.radius[2] = rect.radius[3] = 6.0f;
  rect.fill = api->color_rgba(0.20f, 0.38f, 0.70f, 1.0f);
  rect.visible = true;
  node_id = api->add_rect(editor, &rect);
  if (!node_id)
    return fail("module api add_rect failed");
  if (api->node_count(editor) < 1u)
    return fail("node count did not increase after host-side add");

  json_need = api->build_project_json(editor, NULL, 0u);
  if (json_need < 2u || json_need > sizeof(json_buffer))
    return fail("build_project_json size probe failed");
  if (api->build_project_json(editor, json_buffer, sizeof(json_buffer)) !=
      json_need) {
    return fail("build_project_json full emit failed");
  }

  if (!api->save_project_file(editor, project_path))
    return fail("save_project_file failed");

  api->reset(editor);
  if (api->node_count(editor) != 0u)
    return fail("reset did not clear node graph");

  if (!api->load_project_file(editor, project_path))
    return fail("load_project_file failed");
  if (api->node_count(editor) < 1u)
    return fail("load_project_file did not restore graph");

  if (api->load_project_json(editor, "{bad-json"))
    return fail("load_project_json accepted malformed input");

  api->destroy(editor);
  editor = NULL;
  (void)remove(project_path);

  printf("[PASS] editor host interop contract smoke\n");
  return 0;
}
