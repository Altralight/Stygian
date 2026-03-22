#include "../include/stygian_editor_module.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

int main(void) {
  StygianEditorModuleInfo info = {0};
  const StygianEditorModuleApi *api = NULL;
  uint32_t module_version = stygian_editor_module_abi_version();
  uint32_t major = STYGIAN_EDITOR_MODULE_ABI_VERSION_MAJOR(module_version);
  uint32_t minor = STYGIAN_EDITOR_MODULE_ABI_VERSION_MINOR(module_version);
  uint32_t older_minor_version = 0u;
  uint32_t newer_minor_version = 0u;
  uint32_t wrong_major_version = 0u;

  if (module_version != STYGIAN_EDITOR_MODULE_ABI_VERSION)
    return fail("module abi version mismatch with header constant");

  if (stygian_editor_module_get_info(NULL))
    return fail("get_info accepted null output pointer");

  info.struct_size = sizeof(StygianEditorModuleInfo);
  if (!stygian_editor_module_get_info(&info))
    return fail("get_info failed");

  if (info.struct_size != sizeof(StygianEditorModuleInfo))
    return fail("get_info returned unexpected struct size");
  if (info.abi_version != module_version)
    return fail("get_info returned unexpected abi version");
  if (!info.module_name || !info.module_name[0])
    return fail("get_info returned empty module name");
  if (!info.module_version || !info.module_version[0])
    return fail("get_info returned empty module version");

  info.struct_size = sizeof(StygianEditorModuleInfo) - 1u;
  if (stygian_editor_module_get_info(&info))
    return fail("get_info accepted undersized struct");

  api = stygian_editor_module_get_api(module_version);
  if (!api)
    return fail("get_api rejected exact version");
  if (api->abi_version != module_version)
    return fail("api abi_version field mismatch");
  if (api->struct_size != sizeof(StygianEditorModuleApi))
    return fail("api struct_size field mismatch");
  if (!api->create || !api->destroy || !api->build_c23)
    return fail("api table missing required function pointers");

  if (minor > 0u) {
    older_minor_version = (major << 16) | (minor - 1u);
    if (!stygian_editor_module_get_api(older_minor_version))
      return fail("get_api rejected supported older minor");
  }

  newer_minor_version = (major << 16) | (minor + 1u);
  if (stygian_editor_module_get_api(newer_minor_version))
    return fail("get_api accepted newer minor than module supports");

  wrong_major_version = ((major + 1u) << 16) | minor;
  if (stygian_editor_module_get_api(wrong_major_version))
    return fail("get_api accepted mismatched major");

  printf("[PASS] editor module ABI compatibility smoke\n");
  return 0;
}
