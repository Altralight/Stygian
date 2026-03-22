/* AUTO-GENERATED FIXTURE SNAPSHOT (trimmed for runtime smoke). */
#include "stygian.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct StygianEditorGeneratedResolvedConstraint {
  uint32_t child_id;
  float x;
  float y;
  float w;
  float h;
} StygianEditorGeneratedResolvedConstraint;

typedef struct StygianEditorGeneratedConstraintRecord {
  uint32_t parent_id;
  uint32_t child_id;
  uint32_t h_mode;
  uint32_t v_mode;
  float left;
  float right;
  float top;
  float bottom;
  float center_dx;
  float center_dy;
  float x_ratio;
  float y_ratio;
  float w_ratio;
  float h_ratio;
  float base_w;
  float base_h;
} StygianEditorGeneratedConstraintRecord;

static const StygianEditorGeneratedConstraintRecord
    kStygianEditorGeneratedConstraints[2] = {
        {10u, 11u, 0u, 0u, 12.0f, 0.0f, 8.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 40.0f, 20.0f},
        {10u, 12u, 4u, 2u, 0.0f, 0.0f, 6.0f, 6.0f, 0.0f, 0.0f, 0.50f, 0.0f,
         0.30f, 0.0f, 100.0f, 32.0f},
};

int stygian_editor_generated_node_index(uint32_t node_id) {
  switch (node_id) {
  case 10u:
    return 0;
  case 11u:
    return 1;
  case 12u:
    return 2;
  default:
    return -1;
  }
}

const StygianEditorGeneratedConstraintRecord *
stygian_editor_generated_constraint_records(uint32_t *out_count) {
  if (out_count)
    *out_count = 2u;
  return kStygianEditorGeneratedConstraints;
}

uint32_t stygian_editor_generated_apply_constraints_for_parent(
    uint32_t parent_id, float parent_w, float parent_h,
    StygianEditorGeneratedResolvedConstraint *out_resolved,
    uint32_t max_resolved) {
  uint32_t i;
  uint32_t out = 0u;
  if (!out_resolved || max_resolved == 0u)
    return 0u;
  for (i = 0u; i < 2u; ++i) {
    const StygianEditorGeneratedConstraintRecord *c =
        &kStygianEditorGeneratedConstraints[i];
    float x = c->left;
    float y = c->top;
    float w = c->base_w;
    float h = c->base_h;
    if (c->parent_id != parent_id)
      continue;
    if (c->h_mode == 4u) {
      x = parent_w * c->x_ratio;
      w = parent_w * c->w_ratio;
    }
    if (c->v_mode == 2u) {
      y = c->top;
      h = parent_h - c->top - c->bottom;
      if (h < 0.0f)
        h = 0.0f;
    }
    if (out >= max_resolved)
      break;
    out_resolved[out].child_id = c->child_id;
    out_resolved[out].x = x;
    out_resolved[out].y = y;
    out_resolved[out].w = w;
    out_resolved[out].h = h;
    out += 1u;
  }
  return out;
}
