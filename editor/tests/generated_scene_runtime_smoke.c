#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../fixtures/runtime_generated_scene_fixture.c"

static int fail(const char *msg) {
  fprintf(stderr, "[FAIL] %s\n", msg);
  return 1;
}

static bool nearf(float a, float b) { return fabsf(a - b) <= 0.05f; }

int main(void) {
  const StygianEditorGeneratedConstraintRecord *recs = NULL;
  StygianEditorGeneratedResolvedConstraint out[8];
  uint32_t rec_count = 0u;
  uint32_t out_count = 0u;
  bool found_11 = false;
  bool found_12 = false;
  uint32_t i;

  if (stygian_editor_generated_node_index(10u) != 0 ||
      stygian_editor_generated_node_index(11u) != 1 ||
      stygian_editor_generated_node_index(12u) != 2 ||
      stygian_editor_generated_node_index(999u) != -1) {
    return fail("node index mapping failed");
  }

  recs = stygian_editor_generated_constraint_records(&rec_count);
  if (!recs || rec_count != 2u)
    return fail("constraint record exposure failed");

  out_count = stygian_editor_generated_apply_constraints_for_parent(
      10u, 400.0f, 200.0f, out, 8u);
  if (out_count != 2u)
    return fail("constraint solve count failed");

  for (i = 0u; i < out_count; ++i) {
    if (out[i].child_id == 11u) {
      if (!nearf(out[i].x, 12.0f) || !nearf(out[i].y, 8.0f) ||
          !nearf(out[i].w, 40.0f) || !nearf(out[i].h, 20.0f)) {
        return fail("child 11 constraint mismatch");
      }
      found_11 = true;
    } else if (out[i].child_id == 12u) {
      if (!nearf(out[i].x, 200.0f) || !nearf(out[i].y, 6.0f) ||
          !nearf(out[i].w, 120.0f) || !nearf(out[i].h, 188.0f)) {
        return fail("child 12 constraint mismatch");
      }
      found_12 = true;
    }
  }

  if (!found_11 || !found_12)
    return fail("missing solved children");

  printf("[PASS] editor generated scene runtime smoke\n");
  return 0;
}
