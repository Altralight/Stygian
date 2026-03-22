typedef struct StygianEditorProjectLoadState {
  StygianEditorNodeId *selected_ids;
  uint32_t selected_count;
  uint32_t selected_cap;
  StygianEditorNodeId primary_node_id;
  bool primary_explicit;
  bool next_node_explicit;
  bool next_path_explicit;
  bool next_behavior_explicit;
  bool next_timeline_track_explicit;
  bool next_timeline_clip_explicit;
  bool next_driver_explicit;
  bool next_guide_explicit;
} StygianEditorProjectLoadState;

static void sb_append_json_string(StygianEditorStringBuilder *sb,
                                  const char *text) {
  static const char *hex = "0123456789ABCDEF";
  const unsigned char *p = (const unsigned char *)(text ? text : "");
  sb_append_raw(sb, "\"");
  while (*p) {
    unsigned char ch = *p++;
    switch (ch) {
    case '\\':
      sb_append_raw(sb, "\\\\");
      break;
    case '"':
      sb_append_raw(sb, "\\\"");
      break;
    case '\n':
      sb_append_raw(sb, "\\n");
      break;
    case '\r':
      sb_append_raw(sb, "\\r");
      break;
    case '\t':
      sb_append_raw(sb, "\\t");
      break;
    default:
      if (ch < 0x20u) {
        char escaped[7];
        escaped[0] = '\\';
        escaped[1] = 'u';
        escaped[2] = '0';
        escaped[3] = '0';
        escaped[4] = hex[(ch >> 4) & 0x0Fu];
        escaped[5] = hex[ch & 0x0Fu];
        escaped[6] = '\0';
        sb_append_raw(sb, escaped);
      } else {
        char one[2];
        one[0] = (char)ch;
        one[1] = '\0';
        sb_append_raw(sb, one);
      }
      break;
    }
  }
  sb_append_raw(sb, "\"");
}

static void sb_append_record_escaped(StygianEditorStringBuilder *sb,
                                     const char *text) {
  static const char *hex = "0123456789ABCDEF";
  const unsigned char *p = (const unsigned char *)(text ? text : "");
  while (*p) {
    unsigned char ch = *p++;
    if (ch == '%' || ch == ';' || ch == '=' || ch == ',' || ch == '|' ||
        ch == '"' || ch == '\\' || ch < 0x20u) {
      char encoded[4];
      encoded[0] = '%';
      encoded[1] = hex[(ch >> 4) & 0x0Fu];
      encoded[2] = hex[ch & 0x0Fu];
      encoded[3] = '\0';
      sb_append_raw(sb, encoded);
    } else {
      char one[2];
      one[0] = (char)ch;
      one[1] = '\0';
      sb_append_raw(sb, one);
    }
  }
}

static bool editor_project_decode_record_string_n(const char *src, size_t src_len,
                                                  char *dst, size_t dst_cap) {
  size_t len = 0u;
  size_t i = 0u;
  if (!src || !dst || dst_cap == 0u)
    return false;
  while (i < src_len) {
    unsigned char ch = (unsigned char)src[i++];
    if (ch == '%') {
      unsigned hi;
      unsigned lo;
      if ((i + 1u) >= src_len)
        return false;
      if (!isxdigit((unsigned char)src[i]) ||
          !isxdigit((unsigned char)src[i + 1u]))
        return false;
      hi = (unsigned)(isdigit((unsigned char)src[i]) ? src[i] - '0'
                                                     : (toupper(src[i]) - 'A' + 10));
      lo = (unsigned)(isdigit((unsigned char)src[i + 1u])
                          ? src[i + 1u] - '0'
                          : (toupper(src[i + 1u]) - 'A' + 10));
      ch = (unsigned char)((hi << 4) | lo);
      i += 2u;
    }
    if (len + 1u >= dst_cap)
      return false;
    dst[len++] = (char)ch;
  }
  dst[len] = '\0';
  return true;
}

static const char *editor_project_kind_name(StygianEditorShapeKind kind) {
  switch (kind) {
  case STYGIAN_EDITOR_SHAPE_RECT:
    return "rect";
  case STYGIAN_EDITOR_SHAPE_ELLIPSE:
    return "ellipse";
  case STYGIAN_EDITOR_SHAPE_PATH:
    return "path";
  case STYGIAN_EDITOR_SHAPE_FRAME:
    return "frame";
  case STYGIAN_EDITOR_SHAPE_TEXT:
    return "text";
  case STYGIAN_EDITOR_SHAPE_IMAGE:
    return "image";
  case STYGIAN_EDITOR_SHAPE_LINE:
    return "line";
  case STYGIAN_EDITOR_SHAPE_ARROW:
    return "arrow";
  case STYGIAN_EDITOR_SHAPE_POLYGON:
    return "polygon";
  case STYGIAN_EDITOR_SHAPE_STAR:
    return "star";
  case STYGIAN_EDITOR_SHAPE_ARC:
    return "arc";
  case STYGIAN_EDITOR_SHAPE_GROUP:
    return "group";
  case STYGIAN_EDITOR_SHAPE_COMPONENT_DEF:
    return "component_def";
  case STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE:
    return "component_instance";
  default:
    return "unknown";
  }
}

static const char *editor_project_event_name(StygianEditorEventKind kind) {
  switch (kind) {
  case STYGIAN_EDITOR_EVENT_PRESS:
    return "press";
  case STYGIAN_EDITOR_EVENT_RELEASE:
    return "release";
  case STYGIAN_EDITOR_EVENT_HOVER_ENTER:
    return "hover_enter";
  case STYGIAN_EDITOR_EVENT_HOVER_LEAVE:
    return "hover_leave";
  case STYGIAN_EDITOR_EVENT_DRAG_START:
    return "drag_start";
  case STYGIAN_EDITOR_EVENT_DRAG_MOVE:
    return "drag_move";
  case STYGIAN_EDITOR_EVENT_DRAG_END:
    return "drag_end";
  case STYGIAN_EDITOR_EVENT_SCROLL:
    return "scroll";
  case STYGIAN_EDITOR_EVENT_VALUE_CHANGED:
    return "value_changed";
  case STYGIAN_EDITOR_EVENT_FOCUS_ENTER:
    return "focus_enter";
  case STYGIAN_EDITOR_EVENT_FOCUS_LEAVE:
    return "focus_leave";
  default:
    return "unknown";
  }
}

static const char *editor_project_property_name(StygianEditorPropertyKind kind) {
  switch (kind) {
  case STYGIAN_EDITOR_PROP_X:
    return "x";
  case STYGIAN_EDITOR_PROP_Y:
    return "y";
  case STYGIAN_EDITOR_PROP_WIDTH:
    return "width";
  case STYGIAN_EDITOR_PROP_HEIGHT:
    return "height";
  case STYGIAN_EDITOR_PROP_OPACITY:
    return "opacity";
  case STYGIAN_EDITOR_PROP_RADIUS_TL:
    return "radius_tl";
  case STYGIAN_EDITOR_PROP_RADIUS_TR:
    return "radius_tr";
  case STYGIAN_EDITOR_PROP_RADIUS_BR:
    return "radius_br";
  case STYGIAN_EDITOR_PROP_RADIUS_BL:
    return "radius_bl";
  case STYGIAN_EDITOR_PROP_VALUE:
    return "value";
  case STYGIAN_EDITOR_PROP_ROTATION_DEG:
    return "rotation_deg";
  case STYGIAN_EDITOR_PROP_VISIBLE:
    return "visible";
  case STYGIAN_EDITOR_PROP_FILL_COLOR:
    return "fill_color";
  case STYGIAN_EDITOR_PROP_COLOR_TOKEN:
    return "color_token";
  case STYGIAN_EDITOR_PROP_SHAPE_KIND:
    return "shape_kind";
  default:
    return "unknown";
  }
}

static const char *editor_project_easing_name(StygianEditorEasing easing) {
  switch (easing) {
  case STYGIAN_EDITOR_EASING_LINEAR:
    return "linear";
  case STYGIAN_EDITOR_EASING_OUT_CUBIC:
    return "out_cubic";
  case STYGIAN_EDITOR_EASING_IN_OUT_CUBIC:
    return "in_out_cubic";
  default:
    return "unknown";
  }
}

static bool editor_project_parse_kind_name(const char *text,
                                           StygianEditorShapeKind *out_kind) {
  if (!text || !out_kind)
    return false;
  if (strcmp(text, "rect") == 0) {
    *out_kind = STYGIAN_EDITOR_SHAPE_RECT;
    return true;
  }
  if (strcmp(text, "ellipse") == 0) {
    *out_kind = STYGIAN_EDITOR_SHAPE_ELLIPSE;
    return true;
  }
  if (strcmp(text, "path") == 0) {
    *out_kind = STYGIAN_EDITOR_SHAPE_PATH;
    return true;
  }
  if (strcmp(text, "frame") == 0) {
    *out_kind = STYGIAN_EDITOR_SHAPE_FRAME;
    return true;
  }
  if (strcmp(text, "text") == 0) {
    *out_kind = STYGIAN_EDITOR_SHAPE_TEXT;
    return true;
  }
  if (strcmp(text, "image") == 0) {
    *out_kind = STYGIAN_EDITOR_SHAPE_IMAGE;
    return true;
  }
  if (strcmp(text, "line") == 0) {
    *out_kind = STYGIAN_EDITOR_SHAPE_LINE;
    return true;
  }
  if (strcmp(text, "arrow") == 0) {
    *out_kind = STYGIAN_EDITOR_SHAPE_ARROW;
    return true;
  }
  if (strcmp(text, "polygon") == 0) {
    *out_kind = STYGIAN_EDITOR_SHAPE_POLYGON;
    return true;
  }
  if (strcmp(text, "star") == 0) {
    *out_kind = STYGIAN_EDITOR_SHAPE_STAR;
    return true;
  }
  if (strcmp(text, "arc") == 0) {
    *out_kind = STYGIAN_EDITOR_SHAPE_ARC;
    return true;
  }
  if (strcmp(text, "group") == 0) {
    *out_kind = STYGIAN_EDITOR_SHAPE_GROUP;
    return true;
  }
  if (strcmp(text, "component_def") == 0) {
    *out_kind = STYGIAN_EDITOR_SHAPE_COMPONENT_DEF;
    return true;
  }
  if (strcmp(text, "component_instance") == 0) {
    *out_kind = STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE;
    return true;
  }
  return false;
}

static bool editor_project_parse_event_name(const char *text,
                                            StygianEditorEventKind *out_kind) {
  if (!text || !out_kind)
    return false;
  if (strcmp(text, "press") == 0) {
    *out_kind = STYGIAN_EDITOR_EVENT_PRESS;
    return true;
  }
  if (strcmp(text, "release") == 0) {
    *out_kind = STYGIAN_EDITOR_EVENT_RELEASE;
    return true;
  }
  if (strcmp(text, "hover_enter") == 0) {
    *out_kind = STYGIAN_EDITOR_EVENT_HOVER_ENTER;
    return true;
  }
  if (strcmp(text, "hover_leave") == 0) {
    *out_kind = STYGIAN_EDITOR_EVENT_HOVER_LEAVE;
    return true;
  }
  if (strcmp(text, "drag_start") == 0) {
    *out_kind = STYGIAN_EDITOR_EVENT_DRAG_START;
    return true;
  }
  if (strcmp(text, "drag_move") == 0) {
    *out_kind = STYGIAN_EDITOR_EVENT_DRAG_MOVE;
    return true;
  }
  if (strcmp(text, "drag_end") == 0) {
    *out_kind = STYGIAN_EDITOR_EVENT_DRAG_END;
    return true;
  }
  if (strcmp(text, "scroll") == 0) {
    *out_kind = STYGIAN_EDITOR_EVENT_SCROLL;
    return true;
  }
  if (strcmp(text, "value_changed") == 0) {
    *out_kind = STYGIAN_EDITOR_EVENT_VALUE_CHANGED;
    return true;
  }
  if (strcmp(text, "focus_enter") == 0) {
    *out_kind = STYGIAN_EDITOR_EVENT_FOCUS_ENTER;
    return true;
  }
  if (strcmp(text, "focus_leave") == 0) {
    *out_kind = STYGIAN_EDITOR_EVENT_FOCUS_LEAVE;
    return true;
  }
  return false;
}

static bool editor_project_parse_property_name(const char *text,
                                               StygianEditorPropertyKind *out) {
  if (!text || !out)
    return false;
  if (strcmp(text, "x") == 0) {
    *out = STYGIAN_EDITOR_PROP_X;
    return true;
  }
  if (strcmp(text, "y") == 0) {
    *out = STYGIAN_EDITOR_PROP_Y;
    return true;
  }
  if (strcmp(text, "width") == 0) {
    *out = STYGIAN_EDITOR_PROP_WIDTH;
    return true;
  }
  if (strcmp(text, "height") == 0) {
    *out = STYGIAN_EDITOR_PROP_HEIGHT;
    return true;
  }
  if (strcmp(text, "opacity") == 0) {
    *out = STYGIAN_EDITOR_PROP_OPACITY;
    return true;
  }
  if (strcmp(text, "radius_tl") == 0) {
    *out = STYGIAN_EDITOR_PROP_RADIUS_TL;
    return true;
  }
  if (strcmp(text, "radius_tr") == 0) {
    *out = STYGIAN_EDITOR_PROP_RADIUS_TR;
    return true;
  }
  if (strcmp(text, "radius_br") == 0) {
    *out = STYGIAN_EDITOR_PROP_RADIUS_BR;
    return true;
  }
  if (strcmp(text, "radius_bl") == 0) {
    *out = STYGIAN_EDITOR_PROP_RADIUS_BL;
    return true;
  }
  if (strcmp(text, "value") == 0) {
    *out = STYGIAN_EDITOR_PROP_VALUE;
    return true;
  }
  if (strcmp(text, "rotation_deg") == 0) {
    *out = STYGIAN_EDITOR_PROP_ROTATION_DEG;
    return true;
  }
  if (strcmp(text, "visible") == 0) {
    *out = STYGIAN_EDITOR_PROP_VISIBLE;
    return true;
  }
  if (strcmp(text, "fill_color") == 0) {
    *out = STYGIAN_EDITOR_PROP_FILL_COLOR;
    return true;
  }
  if (strcmp(text, "color_token") == 0) {
    *out = STYGIAN_EDITOR_PROP_COLOR_TOKEN;
    return true;
  }
  if (strcmp(text, "shape_kind") == 0) {
    *out = STYGIAN_EDITOR_PROP_SHAPE_KIND;
    return true;
  }
  return false;
}

static bool editor_project_parse_easing_name(const char *text,
                                             StygianEditorEasing *out) {
  if (!text || !out)
    return false;
  if (strcmp(text, "linear") == 0) {
    *out = STYGIAN_EDITOR_EASING_LINEAR;
    return true;
  }
  if (strcmp(text, "out_cubic") == 0) {
    *out = STYGIAN_EDITOR_EASING_OUT_CUBIC;
    return true;
  }
  if (strcmp(text, "in_out_cubic") == 0) {
    *out = STYGIAN_EDITOR_EASING_IN_OUT_CUBIC;
    return true;
  }
  return false;
}

static bool editor_project_fail(StygianEditor *editor, const char *fmt, ...) {
  char message[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);
  if (editor)
    editor_logf(editor, STYGIAN_EDITOR_LOG_ERROR, "%s", message);
  return false;
}

static bool editor_project_record_field_span(const char *record, const char *key,
                                             const char **out_value,
                                             size_t *out_value_len) {
  size_t key_len;
  const char *at;
  if (!record || !key || !out_value || !out_value_len)
    return false;
  key_len = strlen(key);
  at = record;
  while (*at) {
    const char *field_end = strchr(at, ';');
    const char *eq;
    if (!field_end)
      field_end = at + strlen(at);
    eq = memchr(at, '=', (size_t)(field_end - at));
    if (eq && (size_t)(eq - at) == key_len && strncmp(at, key, key_len) == 0) {
      *out_value = eq + 1;
      *out_value_len = (size_t)(field_end - eq - 1);
      return true;
    }
    if (*field_end == '\0')
      break;
    at = field_end + 1;
  }
  return false;
}

static bool editor_project_record_field(const char *record, const char *key,
                                        char *out, size_t out_cap) {
  const char *value = NULL;
  size_t value_len = 0u;
  if (!record || !key || !out || out_cap == 0u)
    return false;
  if (!editor_project_record_field_span(record, key, &value, &value_len))
    return false;
  if (value_len >= out_cap)
    return false;
  memcpy(out, value, value_len);
  out[value_len] = '\0';
  return true;
}

static bool editor_project_record_string(const char *record, const char *key,
                                         char *out, size_t out_cap) {
  const char *value = NULL;
  size_t value_len = 0u;
  if (!editor_project_record_field_span(record, key, &value, &value_len))
    return false;
  return editor_project_decode_record_string_n(value, value_len, out, out_cap);
}

static bool editor_project_record_u32(const char *record, const char *key,
                                      uint32_t *out) {
  char field[64];
  char *end = NULL;
  unsigned long value;
  if (!out || !editor_project_record_field(record, key, field, sizeof(field)))
    return false;
  value = strtoul(field, &end, 10);
  if (end == field)
    return false;
  *out = (uint32_t)value;
  return true;
}

static bool editor_project_record_float(const char *record, const char *key,
                                        float *out) {
  char field[64];
  char *end = NULL;
  double value;
  if (!out || !editor_project_record_field(record, key, field, sizeof(field)))
    return false;
  if (strcmp(field, "null") == 0) {
    *out = NAN;
    return true;
  }
  value = strtod(field, &end);
  if (end == field)
    return false;
  *out = (float)value;
  return true;
}

static bool editor_project_record_bool(const char *record, const char *key,
                                       bool *out) {
  char field[16];
  if (!out || !editor_project_record_field(record, key, field, sizeof(field)))
    return false;
  if (strcmp(field, "1") == 0 || strcmp(field, "true") == 0) {
    *out = true;
    return true;
  }
  if (strcmp(field, "0") == 0 || strcmp(field, "false") == 0) {
    *out = false;
    return true;
  }
  return false;
}

static bool editor_project_parse_color_text(const char *text,
                                            StygianEditorColor *out) {
  float r, g, b, a;
  if (!text || !out)
    return false;
  if (sscanf(text, "%f,%f,%f,%f", &r, &g, &b, &a) != 4)
    return false;
  *out = stygian_editor_color_rgba(r, g, b, a);
  return true;
}

static bool editor_project_parse_fills_text(const char *text,
                                            StygianEditorNode *node) {
  const char *at = text;
  uint32_t count = 0u;
  if (!text || !node)
    return false;
  if (!text[0]) {
    uint32_t i;
    node->fill_count = 0u;
    for (i = 0u; i < STYGIAN_EDITOR_NODE_FILL_CAP; ++i)
      node->fill_gradient_xform[i] = editor_gradient_xform_default();
    return true;
  }
  while (*at) {
    StygianEditorNodeFill fill;
    StygianEditorGradientTransform xform = editor_gradient_xform_default();
    char entry[512];
    const char *sep = strchr(at, '|');
    size_t len = sep ? (size_t)(sep - at) : strlen(at);
    char image[64] = {0};
    unsigned kind = 0u, visible = 0u;
    int parsed = 0;
    if (count >= STYGIAN_EDITOR_NODE_FILL_CAP || len >= sizeof(entry))
      return false;
    memcpy(entry, at, len);
    entry[len] = '\0';
    memset(&fill, 0, sizeof(fill));
    parsed = sscanf(
        entry,
        "%u,%u,%f,%f:%f:%f:%f,%f:%f:%f:%f,%f:%f:%f:%f,%f,%f,%f,%f,%63[^,],%f,%f,%f,%f,%f",
        &kind, &visible, &fill.opacity, &fill.solid.r, &fill.solid.g,
        &fill.solid.b, &fill.solid.a, &fill.stops[0].color.r,
        &fill.stops[0].color.g, &fill.stops[0].color.b,
        &fill.stops[0].color.a, &fill.stops[1].color.r,
        &fill.stops[1].color.g, &fill.stops[1].color.b,
        &fill.stops[1].color.a, &fill.gradient_angle_deg, &fill.radial_cx,
        &fill.radial_cy, &fill.radial_r, image, &xform.origin_x, &xform.origin_y,
        &xform.scale_x, &xform.scale_y, &xform.rotation_deg);
    if (parsed < 20) {
      return false;
    }
    if (kind > (unsigned)STYGIAN_EDITOR_FILL_IMAGE)
      kind = (unsigned)STYGIAN_EDITOR_FILL_SOLID;
    fill.kind = (StygianEditorFillKind)kind;
    fill.visible = visible ? true : false;
    fill.stops[0].position = 0.0f;
    fill.stops[1].position = 1.0f;
    editor_gradient_xform_sanitize(&xform);
    if (strcmp(image, "-") != 0)
      snprintf(fill.image_asset, sizeof(fill.image_asset), "%s", image);
    node->fills[count++] = fill;
    node->fill_gradient_xform[count - 1u] = xform;
    at = sep ? (sep + 1) : (at + len);
  }
  {
    uint32_t i;
    for (i = count; i < STYGIAN_EDITOR_NODE_FILL_CAP; ++i)
      node->fill_gradient_xform[i] = editor_gradient_xform_default();
  }
  node->fill_count = count;
  return true;
}

static void editor_project_append_fills(StygianEditorStringBuilder *sb,
                                        const StygianEditorNode *node) {
  uint32_t i;
  if (!sb || !node)
    return;
  if (node->fill_count == 0u)
    return;
  sb_append_raw(sb, ";fills=");
  for (i = 0u; i < node->fill_count && i < STYGIAN_EDITOR_NODE_FILL_CAP; ++i) {
    const StygianEditorNodeFill *fill = &node->fills[i];
    const StygianEditorGradientTransform *xform = &node->fill_gradient_xform[i];
    if (i > 0u)
      sb_append_raw(sb, "|");
    sb_appendf(sb,
               "%u,%u,%.6f,%.6f:%.6f:%.6f:%.6f,%.6f:%.6f:%.6f:%.6f,%.6f:%.6f:%.6f:%.6f,%.6f,%.6f,%.6f,%.6f,%s,%.6f,%.6f,%.6f,%.6f,%.6f",
               (uint32_t)fill->kind, fill->visible ? 1u : 0u, fill->opacity,
               fill->solid.r, fill->solid.g, fill->solid.b, fill->solid.a,
               fill->stops[0].color.r, fill->stops[0].color.g,
               fill->stops[0].color.b, fill->stops[0].color.a,
               fill->stops[1].color.r, fill->stops[1].color.g,
               fill->stops[1].color.b, fill->stops[1].color.a,
               fill->gradient_angle_deg, fill->radial_cx, fill->radial_cy,
               fill->radial_r, fill->image_asset[0] ? fill->image_asset : "-",
               xform->origin_x, xform->origin_y, xform->scale_x, xform->scale_y,
               xform->rotation_deg);
  }
}

static bool editor_project_parse_strokes_text(const char *text,
                                              StygianEditorNode *node) {
  const char *at = text;
  uint32_t count = 0u;
  if (!text || !node)
    return false;
  if (!text[0]) {
    node->stroke_count = 0u;
    return true;
  }
  while (*at) {
    StygianEditorNodeStroke stroke;
    char entry[512];
    const char *sep = strchr(at, '|');
    size_t len = sep ? (size_t)(sep - at) : strlen(at);
    unsigned vis = 0u, cap = 0u, join = 0u, align = 0u, dash_count = 0u;
    uint32_t i;
    if (count >= STYGIAN_EDITOR_NODE_STROKE_CAP || len >= sizeof(entry))
      return false;
    memcpy(entry, at, len);
    entry[len] = '\0';
    memset(&stroke, 0, sizeof(stroke));
    if (sscanf(entry,
               "%u,%f,%f,%f:%f:%f:%f,%u,%u,%u,%f,%u,%f:%f:%f:%f:%f:%f:%f:%f",
               &vis, &stroke.opacity, &stroke.thickness, &stroke.color.r,
               &stroke.color.g, &stroke.color.b, &stroke.color.a, &cap, &join,
               &align, &stroke.miter_limit, &dash_count, &stroke.dash_pattern[0],
               &stroke.dash_pattern[1], &stroke.dash_pattern[2],
               &stroke.dash_pattern[3], &stroke.dash_pattern[4],
               &stroke.dash_pattern[5], &stroke.dash_pattern[6],
               &stroke.dash_pattern[7]) < 12) {
      return false;
    }
    stroke.visible = vis ? true : false;
    stroke.cap = cap <= (unsigned)STYGIAN_EDITOR_STROKE_CAP_SQUARE
                     ? (StygianEditorStrokeCap)cap
                     : STYGIAN_EDITOR_STROKE_CAP_BUTT;
    stroke.join = join <= (unsigned)STYGIAN_EDITOR_STROKE_JOIN_BEVEL
                      ? (StygianEditorStrokeJoin)join
                      : STYGIAN_EDITOR_STROKE_JOIN_MITER;
    stroke.align = align <= (unsigned)STYGIAN_EDITOR_STROKE_ALIGN_OUTSIDE
                       ? (StygianEditorStrokeAlign)align
                       : STYGIAN_EDITOR_STROKE_ALIGN_CENTER;
    if (dash_count > STYGIAN_EDITOR_STROKE_DASH_CAP)
      dash_count = STYGIAN_EDITOR_STROKE_DASH_CAP;
    stroke.dash_count = dash_count;
    for (i = dash_count; i < STYGIAN_EDITOR_STROKE_DASH_CAP; ++i)
      stroke.dash_pattern[i] = 0.0f;
    node->strokes[count++] = stroke;
    at = sep ? (sep + 1) : (at + len);
  }
  node->stroke_count = count;
  return true;
}

static void editor_project_append_strokes(StygianEditorStringBuilder *sb,
                                          const StygianEditorNode *node) {
  uint32_t i, j;
  if (!sb || !node || node->stroke_count == 0u)
    return;
  sb_append_raw(sb, ";strokes=");
  for (i = 0u; i < node->stroke_count && i < STYGIAN_EDITOR_NODE_STROKE_CAP; ++i) {
    const StygianEditorNodeStroke *stroke = &node->strokes[i];
    if (i > 0u)
      sb_append_raw(sb, "|");
    sb_appendf(sb, "%u,%.6f,%.6f,%.6f:%.6f:%.6f:%.6f,%u,%u,%u,%.6f,%u,",
               stroke->visible ? 1u : 0u, stroke->opacity, stroke->thickness,
               stroke->color.r, stroke->color.g, stroke->color.b, stroke->color.a,
               (uint32_t)stroke->cap, (uint32_t)stroke->join,
               (uint32_t)stroke->align, stroke->miter_limit, stroke->dash_count);
    for (j = 0u; j < STYGIAN_EDITOR_STROKE_DASH_CAP; ++j) {
      if (j > 0u)
        sb_append_raw(sb, ":");
      sb_appendf(sb, "%.6f", stroke->dash_pattern[j]);
    }
  }
}

static bool editor_project_parse_effects_text(const char *text,
                                              StygianEditorNode *node) {
  const char *at = text;
  uint32_t count = 0u;
  if (!text || !node)
    return false;
  if (!text[0]) {
    uint32_t i;
    node->effect_count = 0u;
    for (i = 0u; i < STYGIAN_EDITOR_NODE_EFFECT_CAP; ++i)
      node->effect_xform[i] = editor_effect_xform_default();
    return true;
  }
  while (*at) {
    StygianEditorNodeEffect effect;
    StygianEditorEffectTransform xform = editor_effect_xform_default();
    char entry[384];
    const char *sep = strchr(at, '|');
    size_t len = sep ? (size_t)(sep - at) : strlen(at);
    unsigned kind = 0u, vis = 0u;
    int parsed = 0;
    if (count >= STYGIAN_EDITOR_NODE_EFFECT_CAP || len >= sizeof(entry))
      return false;
    memcpy(entry, at, len);
    entry[len] = '\0';
    memset(&effect, 0, sizeof(effect));
    parsed = sscanf(entry,
                    "%u,%u,%f,%f,%f,%f,%f,%f,%f:%f:%f:%f,%f,%f,%f", &kind,
                    &vis, &effect.opacity, &effect.radius, &effect.spread,
                    &effect.offset_x, &effect.offset_y, &effect.intensity,
                    &effect.color.r, &effect.color.g, &effect.color.b,
                    &effect.color.a, &xform.scale_x, &xform.scale_y,
                    &xform.rotation_deg);
    if (parsed < 12) {
      return false;
    }
    if (kind > (unsigned)STYGIAN_EDITOR_EFFECT_NOISE)
      kind = (unsigned)STYGIAN_EDITOR_EFFECT_DROP_SHADOW;
    effect.kind = (StygianEditorEffectKind)kind;
    effect.visible = vis ? true : false;
    editor_effect_xform_sanitize(&xform);
    node->effects[count++] = effect;
    node->effect_xform[count - 1u] = xform;
    at = sep ? (sep + 1) : (at + len);
  }
  {
    uint32_t i;
    for (i = count; i < STYGIAN_EDITOR_NODE_EFFECT_CAP; ++i)
      node->effect_xform[i] = editor_effect_xform_default();
  }
  node->effect_count = count;
  return true;
}

static void editor_project_append_effects(StygianEditorStringBuilder *sb,
                                          const StygianEditorNode *node) {
  uint32_t i;
  if (!sb || !node || node->effect_count == 0u)
    return;
  sb_append_raw(sb, ";effects=");
  for (i = 0u; i < node->effect_count && i < STYGIAN_EDITOR_NODE_EFFECT_CAP; ++i) {
    const StygianEditorNodeEffect *effect = &node->effects[i];
    const StygianEditorEffectTransform *xform = &node->effect_xform[i];
    if (i > 0u)
      sb_append_raw(sb, "|");
    sb_appendf(sb,
               "%u,%u,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f:%.6f:%.6f:%.6f,%.6f,%.6f,%.6f",
               (uint32_t)effect->kind, effect->visible ? 1u : 0u, effect->opacity,
               effect->radius, effect->spread, effect->offset_x, effect->offset_y,
               effect->intensity, effect->color.r, effect->color.g, effect->color.b,
               effect->color.a, xform->scale_x, xform->scale_y,
               xform->rotation_deg);
  }
}

static void editor_project_append_node_extras(StygianEditorStringBuilder *sb,
                                              const StygianEditorNode *node) {
  if (!sb || !node)
    return;
  editor_project_append_strokes(sb, node);
  editor_project_append_effects(sb, node);
  if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID) {
    sb_appendf(sb, ";mask=%u;maskm=%u;maski=%u", node->mask_node_id,
               (uint32_t)node->mask_mode, node->mask_invert ? 1u : 0u);
  }
  if (node->shader_attachment.enabled || node->shader_attachment.asset_path[0] ||
      node->shader_attachment.slot_name[0] || node->shader_attachment.entry_point[0]) {
    sb_appendf(sb, ";shen=%u;shslot=", node->shader_attachment.enabled ? 1u : 0u);
    sb_append_record_escaped(sb, node->shader_attachment.slot_name);
    sb_append_raw(sb, ";shasset=");
    sb_append_record_escaped(sb, node->shader_attachment.asset_path);
    sb_append_raw(sb, ";shentry=");
    sb_append_record_escaped(sb, node->shader_attachment.entry_point);
  }
  if (node->boolean_valid) {
    uint32_t i;
    sb_appendf(sb, ";boolop=%u;boolids=", (uint32_t)node->boolean_op);
    for (i = 0u; i < node->boolean_operand_count &&
                i < STYGIAN_EDITOR_NODE_BOOLEAN_OPERAND_CAP;
         ++i) {
      if (i > 0u)
        sb_append_raw(sb, "|");
      sb_appendf(sb, "%u", node->boolean_operands[i]);
    }
  }
  if (node->kind == STYGIAN_EDITOR_SHAPE_TEXT && node->as.text.span_count > 0u) {
    uint32_t i;
    sb_append_raw(sb, ";tspans=");
    for (i = 0u; i < node->as.text.span_count && i < STYGIAN_EDITOR_TEXT_SPAN_CAP;
         ++i) {
      const StygianEditorTextStyleSpan *span = &node->as.text.spans[i];
      if (i > 0u)
        sb_append_raw(sb, "|");
      sb_appendf(sb, "%u,%u,%.6f,%.6f,%.6f,%u,%.6f:%.6f:%.6f:%.6f", span->start,
                 span->length, span->font_size, span->line_height,
                 span->letter_spacing, span->weight, span->color.r,
                 span->color.g, span->color.b, span->color.a);
    }
  }
}

static bool editor_project_parse_text_spans(const char *text,
                                            StygianEditorNode *node) {
  const char *at = text;
  uint32_t count = 0u;
  if (!text || !node)
    return false;
  if (!text[0]) {
    node->as.text.span_count = 0u;
    return true;
  }
  while (*at) {
    StygianEditorTextStyleSpan span;
    char entry[256];
    const char *sep = strchr(at, '|');
    size_t len = sep ? (size_t)(sep - at) : strlen(at);
    if (count >= STYGIAN_EDITOR_TEXT_SPAN_CAP || len >= sizeof(entry))
      return false;
    memcpy(entry, at, len);
    entry[len] = '\0';
    memset(&span, 0, sizeof(span));
    if (sscanf(entry, "%u,%u,%f,%f,%f,%u,%f:%f:%f:%f", &span.start,
               &span.length, &span.font_size, &span.line_height,
               &span.letter_spacing, &span.weight, &span.color.r, &span.color.g,
               &span.color.b, &span.color.a) != 10) {
      return false;
    }
    node->as.text.spans[count++] = span;
    at = sep ? (sep + 1) : (at + len);
  }
  node->as.text.span_count = count;
  return true;
}

static void editor_project_append_component_properties(
    StygianEditorStringBuilder *sb, const StygianEditorNodeComponentDef *def) {
  uint32_t i;
  if (!sb || !def || def->property_count == 0u)
    return;
  sb_append_raw(sb, ";cprops=");
  for (i = 0u; i < def->property_count && i < STYGIAN_EDITOR_COMPONENT_PROPERTY_CAP;
       ++i) {
    const StygianEditorComponentPropertyDef *p = &def->properties[i];
    uint32_t o;
    if (i > 0u)
      sb_append_raw(sb, "|");
    sb_append_raw(sb, "n=");
    sb_append_record_escaped(sb, p->name);
    sb_appendf(sb, ",t=%u,b=%u,e=", (uint32_t)p->type, p->default_bool ? 1u : 0u);
    sb_append_record_escaped(sb, p->default_enum);
    sb_appendf(sb, ",oc=%u,o=", p->enum_option_count);
    for (o = 0u; o < p->enum_option_count &&
                 o < STYGIAN_EDITOR_COMPONENT_ENUM_OPTION_CAP;
         ++o) {
      if (o > 0u)
        sb_append_raw(sb, "~");
      sb_append_record_escaped(sb, p->enum_options[o]);
    }
  }
}

static bool editor_project_parse_component_properties(
    const char *text, StygianEditorNodeComponentDef *def) {
  const char *at = text;
  uint32_t count = 0u;
  if (!text || !def)
    return false;
  if (!text[0]) {
    def->property_count = 0u;
    return true;
  }
  while (*at) {
    char entry[512];
    const char *sep = strchr(at, '|');
    size_t len = sep ? (size_t)(sep - at) : strlen(at);
    char name[32] = {0};
    char enum_default[32] = {0};
    unsigned t = 0u, b = 0u, oc = 0u;
    char options[256] = {0};
    uint32_t oi = 0u;
    char *opt_at;
    if (count >= STYGIAN_EDITOR_COMPONENT_PROPERTY_CAP || len >= sizeof(entry))
      return false;
    memcpy(entry, at, len);
    entry[len] = '\0';
    if (sscanf(entry, "n=%31[^,],t=%u,b=%u,e=%31[^,],oc=%u,o=%255[^\n]", name, &t,
               &b, enum_default, &oc, options) < 5) {
      return false;
    }
    memset(&def->properties[count], 0, sizeof(def->properties[count]));
    if (!editor_project_decode_record_string_n(name, strlen(name),
                                               def->properties[count].name,
                                               sizeof(def->properties[count].name))) {
      return false;
    }
    def->properties[count].type = t <= (uint32_t)STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM
                                      ? (StygianEditorComponentPropertyType)t
                                      : STYGIAN_EDITOR_COMPONENT_PROPERTY_BOOL;
    def->properties[count].default_bool = b ? true : false;
    if (!editor_project_decode_record_string_n(
            enum_default, strlen(enum_default),
            def->properties[count].default_enum,
            sizeof(def->properties[count].default_enum))) {
      return false;
    }
    if (oc > STYGIAN_EDITOR_COMPONENT_ENUM_OPTION_CAP)
      oc = STYGIAN_EDITOR_COMPONENT_ENUM_OPTION_CAP;
    def->properties[count].enum_option_count = oc;
    opt_at = options;
    for (oi = 0u; oi < oc && opt_at && *opt_at; ++oi) {
      char *opt_sep = strchr(opt_at, '~');
      size_t opt_len =
          opt_sep ? (size_t)(opt_sep - opt_at) : strlen(opt_at);
      if (!editor_project_decode_record_string_n(
              opt_at, opt_len, def->properties[count].enum_options[oi],
              sizeof(def->properties[count].enum_options[oi]))) {
        return false;
      }
      opt_at = opt_sep ? (opt_sep + 1) : NULL;
    }
    count += 1u;
    at = sep ? (sep + 1) : (at + len);
  }
  def->property_count = count;
  return true;
}

static void editor_project_append_component_overrides(
    StygianEditorStringBuilder *sb, const StygianEditorComponentOverrideState *ovr) {
  uint32_t i;
  if (!sb || !ovr || ovr->mask == 0u)
    return;
  sb_appendf(sb,
             ";om=%u;ot=", ovr->mask);
  sb_append_record_escaped(sb, ovr->text);
  sb_appendf(sb, ";ov=%u;os=%u;ox=%.6f;oy=%.6f;ow=%.6f;oh=%.6f;ob=",
             ovr->visible ? 1u : 0u, ovr->swap_component_def_id, ovr->x, ovr->y,
             ovr->w, ovr->h);
  sb_append_record_escaped(sb, ovr->style_binding);
  sb_appendf(sb, ";opc=%u;op=", ovr->property_override_count);
  for (i = 0u; i < ovr->property_override_count &&
              i < STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP;
       ++i) {
    const StygianEditorComponentPropertyValue *p = &ovr->property_overrides[i];
    if (i > 0u)
      sb_append_raw(sb, "|");
    sb_append_raw(sb, "n=");
    sb_append_record_escaped(sb, p->name);
    sb_appendf(sb, ",t=%u,b=%u,e=", (uint32_t)p->type, p->bool_value ? 1u : 0u);
    sb_append_record_escaped(sb, p->enum_value);
  }
}

static bool editor_project_parse_component_overrides(
    const char *record, StygianEditorComponentOverrideState *out) {
  uint32_t mask = 0u;
  char field[512];
  if (!record || !out)
    return false;
  memset(out, 0, sizeof(*out));
  if (!editor_project_record_u32(record, "om", &mask)) {
    out->mask = 0u;
    return true;
  }
  out->mask = mask;
  (void)editor_project_record_string(record, "ot", out->text, sizeof(out->text));
  (void)editor_project_record_bool(record, "ov", &out->visible);
  (void)editor_project_record_u32(record, "os", &out->swap_component_def_id);
  (void)editor_project_record_float(record, "ox", &out->x);
  (void)editor_project_record_float(record, "oy", &out->y);
  (void)editor_project_record_float(record, "ow", &out->w);
  (void)editor_project_record_float(record, "oh", &out->h);
  (void)editor_project_record_string(record, "ob", out->style_binding,
                                     sizeof(out->style_binding));
  if (editor_project_record_string(record, "op", field, sizeof(field))) {
    const char *at = field;
    uint32_t count = 0u;
    while (*at && count < STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP) {
      char entry[256];
      const char *sep = strchr(at, '|');
      size_t len = sep ? (size_t)(sep - at) : strlen(at);
      char name[32] = {0};
      char enum_value[32] = {0};
      unsigned t = 0u, b = 0u;
      if (len >= sizeof(entry))
        return false;
      memcpy(entry, at, len);
      entry[len] = '\0';
      if (sscanf(entry, "n=%31[^,],t=%u,b=%u,e=%31s", name, &t, &b, enum_value) !=
          4) {
        return false;
      }
      memset(&out->property_overrides[count], 0,
             sizeof(out->property_overrides[count]));
      if (!editor_project_decode_record_string_n(
              name, strlen(name), out->property_overrides[count].name,
              sizeof(out->property_overrides[count].name)) ||
          !editor_project_decode_record_string_n(
              enum_value, strlen(enum_value),
              out->property_overrides[count].enum_value,
              sizeof(out->property_overrides[count].enum_value))) {
        return false;
      }
      out->property_overrides[count].type =
          t <= (uint32_t)STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM
              ? (StygianEditorComponentPropertyType)t
              : STYGIAN_EDITOR_COMPONENT_PROPERTY_BOOL;
      out->property_overrides[count].bool_value = b ? true : false;
      count += 1u;
      at = sep ? (sep + 1) : (at + len);
    }
    out->property_override_count = count;
  }
  return true;
}

static bool editor_project_parse_radii_text(const char *text, float out[4]) {
  return text && out &&
         sscanf(text, "%f,%f,%f,%f", &out[0], &out[1], &out[2], &out[3]) == 4;
}

static bool editor_project_next_json_string(const char **io_at, const char *end,
                                            char *out, size_t out_cap,
                                            bool *out_done) {
  const char *at;
  size_t len = 0u;
  if (!io_at || !*io_at || !out || out_cap == 0u || !out_done)
    return false;
  at = *io_at;
  while (at < end && (*at == ' ' || *at == '\n' || *at == '\r' || *at == '\t' ||
                      *at == ',')) {
    at++;
  }
  if (at >= end || *at == ']') {
    *out_done = true;
    *io_at = at;
    return true;
  }
  if (*at != '"')
    return false;
  at++;
  while (at < end && *at != '"') {
    unsigned char ch = (unsigned char)*at++;
    if (ch == '\\') {
      if (at >= end)
        return false;
      ch = (unsigned char)*at++;
      switch (ch) {
      case '"':
      case '\\':
      case '/':
        break;
      case 'n':
        ch = '\n';
        break;
      case 'r':
        ch = '\r';
        break;
      case 't':
        ch = '\t';
        break;
      default:
        return false;
      }
    }
    if (len + 1u >= out_cap)
      return false;
    out[len++] = (char)ch;
  }
  if (at >= end || *at != '"')
    return false;
  out[len] = '\0';
  at++;
  *out_done = false;
  *io_at = at;
  return true;
}

static bool editor_project_find_container(const char *json, const char *key,
                                          char open_char, char close_char,
                                          const char **out_begin,
                                          const char **out_end) {
  char pattern[64];
  const char *key_at;
  const char *at;
  int depth = 0;
  bool in_string = false;
  if (!json || !key || !out_begin || !out_end)
    return false;
  snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  key_at = strstr(json, pattern);
  if (!key_at)
    return false;
  at = strchr(key_at, ':');
  if (!at)
    return false;
  at++;
  while (*at && isspace((unsigned char)*at))
    at++;
  if (*at != open_char)
    return false;
  *out_begin = at;
  while (*at) {
    char ch = *at++;
    if (ch == '"' && (at - 2 < json || at[-2] != '\\'))
      in_string = !in_string;
    if (in_string)
      continue;
    if (ch == open_char)
      depth++;
    else if (ch == close_char) {
      depth--;
      if (depth == 0) {
        *out_end = at - 1;
        return true;
      }
    }
  }
  return false;
}

static bool editor_project_find_number(const char *json, const char *key,
                                       uint32_t *out_value) {
  char pattern[64];
  const char *at;
  char *end = NULL;
  unsigned long value;
  if (!json || !key || !out_value)
    return false;
  snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  at = strstr(json, pattern);
  if (!at)
    return false;
  at = strchr(at, ':');
  if (!at)
    return false;
  at++;
  while (*at && isspace((unsigned char)*at))
    at++;
  value = strtoul(at, &end, 10);
  if (end == at)
    return false;
  *out_value = (uint32_t)value;
  return true;
}

static bool editor_project_find_string(const char *json, const char *key,
                                       char *out, size_t out_cap) {
  char pattern[64];
  const char *at;
  bool done = false;
  if (!json || !key || !out || out_cap == 0u)
    return false;
  snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  at = strstr(json, pattern);
  if (!at)
    return false;
  at = strchr(at, ':');
  if (!at)
    return false;
  at++;
  return editor_project_next_json_string(&at, at + strlen(at), out, out_cap,
                                         &done) && !done;
}

static void editor_project_copy_state(StygianEditor *dst,
                                      const StygianEditor *src) {
  StygianEditorHost saved_host = dst->host;
  memset(dst->nodes, 0, sizeof(StygianEditorNode) * (size_t)dst->max_nodes);
  memset(dst->path_points, 0,
         sizeof(StygianEditorPoint) * (size_t)dst->max_path_points);
  memset(dst->behaviors, 0,
         sizeof(StygianEditorBehaviorSlot) * (size_t)dst->max_behaviors);
  memset(dst->active_anims, 0,
         sizeof(StygianEditorActiveAnimation) * (size_t)dst->max_behaviors);
  memset(dst->color_tokens, 0,
         sizeof(StygianEditorColorToken) * (size_t)dst->max_color_tokens);
  memset(dst->timeline_tracks, 0,
         sizeof(StygianEditorTimelineTrackSlot) *
             (size_t)dst->max_timeline_tracks);
  memset(dst->timeline_clips, 0,
         sizeof(StygianEditorTimelineClipSlot) *
             (size_t)dst->max_timeline_clips);
  memset(dst->drivers, 0,
         sizeof(StygianEditorDriverSlot) * (size_t)dst->max_drivers);
  memset(dst->guides, 0, sizeof(StygianEditorGuideSlot) * (size_t)dst->max_guides);
  memcpy(dst->nodes, src->nodes,
         sizeof(StygianEditorNode) * (size_t)src->node_count);
  memcpy(dst->path_points, src->path_points,
         sizeof(StygianEditorPoint) * (size_t)src->point_count);
  memcpy(dst->behaviors, src->behaviors,
         sizeof(StygianEditorBehaviorSlot) * (size_t)src->behavior_count);
  memcpy(dst->color_tokens, src->color_tokens,
         sizeof(StygianEditorColorToken) * (size_t)src->color_token_count);
  memcpy(dst->timeline_tracks, src->timeline_tracks,
         sizeof(StygianEditorTimelineTrackSlot) *
             (size_t)src->timeline_track_count);
  memcpy(dst->timeline_clips, src->timeline_clips,
         sizeof(StygianEditorTimelineClipSlot) *
             (size_t)src->timeline_clip_count);
  memcpy(dst->drivers, src->drivers,
         sizeof(StygianEditorDriverSlot) * (size_t)src->driver_count);
  memcpy(dst->guides, src->guides,
         sizeof(StygianEditorGuideSlot) * (size_t)src->guide_count);
  dst->viewport = src->viewport;
  dst->grid = src->grid;
  dst->node_count = src->node_count;
  dst->point_count = src->point_count;
  dst->behavior_count = src->behavior_count;
  dst->color_token_count = src->color_token_count;
  dst->timeline_track_count = src->timeline_track_count;
  dst->timeline_clip_count = src->timeline_clip_count;
  dst->driver_count = src->driver_count;
  dst->guide_count = src->guide_count;
  dst->selected_node = src->selected_node;
  dst->next_node_id = src->next_node_id;
  dst->next_path_id = src->next_path_id;
  dst->next_behavior_id = src->next_behavior_id;
  dst->next_timeline_track_id = src->next_timeline_track_id;
  dst->next_timeline_clip_id = src->next_timeline_clip_id;
  dst->next_driver_id = src->next_driver_id;
  dst->next_guide_id = src->next_guide_id;
  dst->ruler_unit = src->ruler_unit;
  dst->snap_sources = src->snap_sources;
  memcpy(dst->text_styles, src->text_styles, sizeof(dst->text_styles));
  memcpy(dst->effect_styles, src->effect_styles, sizeof(dst->effect_styles));
  memcpy(dst->layout_styles, src->layout_styles, sizeof(dst->layout_styles));
  memcpy(dst->variables, src->variables, sizeof(dst->variables));
  memcpy(dst->variable_modes, src->variable_modes, sizeof(dst->variable_modes));
  dst->text_style_count = src->text_style_count;
  dst->effect_style_count = src->effect_style_count;
  dst->layout_style_count = src->layout_style_count;
  dst->variable_count = src->variable_count;
  dst->variable_mode_count = src->variable_mode_count;
  dst->active_variable_mode = src->active_variable_mode;
  dst->path_builder.active = false;
  dst->host = saved_host;
}

static bool editor_project_parse_u32_list(const char *text, char sep,
                                          uint32_t *out_values,
                                          uint32_t max_values,
                                          uint32_t *out_count) {
  const char *at = text;
  uint32_t count = 0u;
  if (!out_values || !out_count)
    return false;
  if (!text || !text[0]) {
    *out_count = 0u;
    return true;
  }
  while (*at) {
    char *end = NULL;
    unsigned long value = strtoul(at, &end, 10);
    if (end == at)
      return false;
    if (count >= max_values)
      return false;
    out_values[count++] = (uint32_t)value;
    at = end;
    if (*at == sep)
      at++;
    else if (*at != '\0')
      return false;
  }
  *out_count = count;
  return true;
}

static bool editor_project_parse_points_text(const char *text,
                                             StygianEditor *editor,
                                             uint32_t *out_first,
                                             uint32_t *out_count) {
  const char *at = text;
  uint32_t first = editor->point_count;
  uint32_t count = 0u;
  if (!text || !editor || !out_first || !out_count)
    return false;
  if (!text[0])
    return false;
  while (*at) {
    char entry[256];
    const char *sep = strchr(at, '|');
    size_t len = sep ? (size_t)(sep - at) : strlen(at);
    StygianEditorPoint point;
    unsigned kind_u = 0u;
    int parsed = 0;
    if (len >= sizeof(entry))
      return false;
    memcpy(entry, at, len);
    entry[len] = '\0';
    memset(&point, 0, sizeof(point));
    parsed = sscanf(entry, "%f,%f,%f,%f,%f,%f,%u", &point.x, &point.y,
                    &point.in_x, &point.in_y, &point.out_x, &point.out_y,
                    &kind_u);
    if (parsed < 2) {
      return false;
    }
    if (parsed >= 7)
      point.kind = (StygianEditorPathPointKind)kind_u;
    if (point.kind > STYGIAN_EDITOR_PATH_POINT_ASYMMETRIC)
      point.kind = STYGIAN_EDITOR_PATH_POINT_CORNER;
    if (parsed < 6) {
      point.in_x = point.x;
      point.in_y = point.y;
      point.out_x = point.x;
      point.out_y = point.y;
      point.has_in_tangent = false;
      point.has_out_tangent = false;
      point.kind = STYGIAN_EDITOR_PATH_POINT_CORNER;
    } else {
      point.has_in_tangent = true;
      point.has_out_tangent = true;
    }
    if (editor->point_count >= editor->max_path_points)
      return false;
    editor->path_points[editor->point_count] = point;
    editor->point_count += 1u;
    count += 1u;
    at = sep ? (sep + 1) : (at + len);
  }
  if (count < 2u)
    return false;
  *out_first = first;
  *out_count = count;
  return true;
}

static bool editor_project_parse_node_record(StygianEditor *editor,
                                             const char *record) {
  StygianEditorNode node;
  char field[2048];
  char kind_text[32];
  bool visible = true;
  bool constraints_explicit = false;

  if (!editor || !record)
    return false;
  if (editor->node_count >= editor->max_nodes)
    return false;

  memset(&node, 0, sizeof(node));

  if (!editor_project_record_u32(record, "id", &node.id) ||
      node.id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  if (!editor_project_record_u32(record, "pid", &node.parent_id))
    node.parent_id = STYGIAN_EDITOR_INVALID_ID;
  if (!editor_project_record_string(record, "k", kind_text, sizeof(kind_text)))
    return false;
  if (!editor_project_parse_kind_name(kind_text, &node.kind))
    return false;
  if (!editor_project_record_bool(record, "v", &visible))
    visible = true;
  node.visible = visible;
  if (!editor_project_record_bool(record, "lock", &node.locked))
    node.locked = false;
  if (!editor_project_record_float(record, "z", &node.z))
    node.z = 0.0f;
  if (!editor_project_record_float(record, "val", &node.value))
    node.value = 0.0f;
  if (!editor_project_record_float(record, "rot", &node.rotation_deg))
    node.rotation_deg = 0.0f;
  if (editor_project_record_string(record, "tok", field, sizeof(field))) {
    size_t len = strlen(field);
    if (len >= sizeof(node.color_token))
      return false;
    memcpy(node.color_token, field, len + 1u);
  } else {
    node.color_token[0] = '\0';
  }
  (void)editor_project_record_string(record, "ts", node.text_style,
                                     sizeof(node.text_style));
  (void)editor_project_record_string(record, "es", node.effect_style,
                                     sizeof(node.effect_style));
  (void)editor_project_record_string(record, "ls", node.layout_style,
                                     sizeof(node.layout_style));
  (void)editor_project_record_string(record, "cvar", node.color_variable,
                                     sizeof(node.color_variable));
  (void)editor_project_record_string(record, "nvar", node.number_variable,
                                     sizeof(node.number_variable));
  {
    uint32_t nprop = 0u;
    if (editor_project_record_u32(record, "nprop", &nprop) &&
        nprop <= (uint32_t)STYGIAN_EDITOR_PROP_SHAPE_KIND) {
      node.number_variable_property = (StygianEditorPropertyKind)nprop;
    } else {
      node.number_variable_property = STYGIAN_EDITOR_PROP_VALUE;
    }
  }
  if (editor_project_record_string(record, "name", node.name, sizeof(node.name))) {
    if (node.name[0] == '\0')
      editor_set_default_node_name(&node);
  } else {
    editor_set_default_node_name(&node);
  }
  node.constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
  node.constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
  node.layout_absolute = false;
  node.layout_sizing_h = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node.layout_sizing_v = STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED;
  node.size_min_w = 0.0f;
  node.size_max_w = 0.0f;
  node.size_min_h = 0.0f;
  node.size_max_h = 0.0f;
  {
    uint32_t mode = 0u;
    if (editor_project_record_u32(record, "hc", &mode) &&
        mode <= (uint32_t)STYGIAN_EDITOR_CONSTRAINT_H_SCALE) {
      node.constraint_h = (StygianEditorConstraintH)mode;
      constraints_explicit = true;
    } else {
      node.constraint_h = STYGIAN_EDITOR_CONSTRAINT_H_LEFT;
    }
    if (editor_project_record_u32(record, "vc", &mode) &&
        mode <= (uint32_t)STYGIAN_EDITOR_CONSTRAINT_V_SCALE) {
      node.constraint_v = (StygianEditorConstraintV)mode;
      constraints_explicit = true;
    } else {
      node.constraint_v = STYGIAN_EDITOR_CONSTRAINT_V_TOP;
    }
  }
  (void)editor_project_record_float(record, "cl", &node.constraint_left);
  (void)editor_project_record_float(record, "cr", &node.constraint_right);
  (void)editor_project_record_float(record, "ct", &node.constraint_top);
  (void)editor_project_record_float(record, "cb", &node.constraint_bottom);
  (void)editor_project_record_float(record, "ccx", &node.constraint_center_dx);
  (void)editor_project_record_float(record, "ccy", &node.constraint_center_dy);
  (void)editor_project_record_float(record, "cxr", &node.constraint_x_ratio);
  (void)editor_project_record_float(record, "cyr", &node.constraint_y_ratio);
  (void)editor_project_record_float(record, "cwr", &node.constraint_w_ratio);
  (void)editor_project_record_float(record, "chr", &node.constraint_h_ratio);
  (void)editor_project_record_bool(record, "la", &node.layout_absolute);
  (void)editor_project_record_float(record, "mnw", &node.size_min_w);
  (void)editor_project_record_float(record, "mxw", &node.size_max_w);
  (void)editor_project_record_float(record, "mnh", &node.size_min_h);
  (void)editor_project_record_float(record, "mxh", &node.size_max_h);
  if (node.size_min_w < 0.0f)
    node.size_min_w = 0.0f;
  if (node.size_min_h < 0.0f)
    node.size_min_h = 0.0f;
  if (node.size_max_w < 0.0f)
    node.size_max_w = 0.0f;
  if (node.size_max_h < 0.0f)
    node.size_max_h = 0.0f;
  if (node.size_max_w > 0.0f && node.size_max_w < node.size_min_w)
    node.size_max_w = node.size_min_w;
  if (node.size_max_h > 0.0f && node.size_max_h < node.size_min_h)
    node.size_max_h = node.size_min_h;
  {
    uint32_t mode = 0u;
    if (editor_project_record_u32(record, "lsw", &mode) &&
        mode <= (uint32_t)STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FILL) {
      node.layout_sizing_h = (StygianEditorAutoLayoutSizing)mode;
    }
    if (editor_project_record_u32(record, "lsh", &mode) &&
        mode <= (uint32_t)STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FILL) {
      node.layout_sizing_v = (StygianEditorAutoLayoutSizing)mode;
    }
  }

  if (node.kind == STYGIAN_EDITOR_SHAPE_RECT) {
    if (!editor_project_record_float(record, "x", &node.as.rect.x) ||
        !editor_project_record_float(record, "y", &node.as.rect.y) ||
        !editor_project_record_float(record, "w", &node.as.rect.w) ||
        !editor_project_record_float(record, "h", &node.as.rect.h)) {
      return false;
    }
    if (!editor_project_record_string(record, "rad", field, sizeof(field)) ||
        !editor_project_parse_radii_text(field, node.as.rect.radius)) {
      return false;
    }
    if (!editor_project_record_string(record, "fill", field, sizeof(field)) ||
        !editor_project_parse_color_text(field, &node.as.rect.fill)) {
      return false;
    }
  } else if (node.kind == STYGIAN_EDITOR_SHAPE_ELLIPSE) {
    if (!editor_project_record_float(record, "x", &node.as.ellipse.x) ||
        !editor_project_record_float(record, "y", &node.as.ellipse.y) ||
        !editor_project_record_float(record, "w", &node.as.ellipse.w) ||
        !editor_project_record_float(record, "h", &node.as.ellipse.h)) {
      return false;
    }
    if (!editor_project_record_string(record, "fill", field, sizeof(field)) ||
        !editor_project_parse_color_text(field, &node.as.ellipse.fill)) {
      return false;
    }
  } else if (node.kind == STYGIAN_EDITOR_SHAPE_PATH) {
    bool closed = false;
    if (!editor_project_record_bool(record, "closed", &closed) ||
        !editor_project_record_float(record, "th", &node.as.path.thickness) ||
        !editor_project_record_string(record, "stroke", field, sizeof(field)) ||
        !editor_project_parse_color_text(field, &node.as.path.stroke)) {
      return false;
    }
    node.as.path.closed = closed;
    if (!editor_project_record_string(record, "pts", field, sizeof(field)) ||
        !editor_project_parse_points_text(field, editor, &node.as.path.first_point,
                                         &node.as.path.point_count)) {
      return false;
    }
    editor_recompute_path_bounds(editor, &node);
  } else if (node.kind == STYGIAN_EDITOR_SHAPE_FRAME) {
    uint32_t mode = 0u;
    if (!editor_project_record_float(record, "x", &node.as.frame.x) ||
        !editor_project_record_float(record, "y", &node.as.frame.y) ||
        !editor_project_record_float(record, "w", &node.as.frame.w) ||
        !editor_project_record_float(record, "h", &node.as.frame.h) ||
        !editor_project_record_bool(record, "clip", &node.as.frame.clip_content) ||
        !editor_project_record_string(record, "fill", field, sizeof(field)) ||
        !editor_project_parse_color_text(field, &node.as.frame.fill)) {
      return false;
    }
    node.as.frame.layout_mode = STYGIAN_EDITOR_AUTO_LAYOUT_OFF;
    node.as.frame.layout_wrap = STYGIAN_EDITOR_AUTO_LAYOUT_NO_WRAP;
    node.as.frame.layout_padding_left = 0.0f;
    node.as.frame.layout_padding_right = 0.0f;
    node.as.frame.layout_padding_top = 0.0f;
    node.as.frame.layout_padding_bottom = 0.0f;
    node.as.frame.layout_gap = 0.0f;
    node.as.frame.layout_primary_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
    node.as.frame.layout_cross_align = STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START;
    node.as.frame.overflow_policy = STYGIAN_EDITOR_FRAME_OVERFLOW_VISIBLE;
    node.as.frame.scroll_x = 0.0f;
    node.as.frame.scroll_y = 0.0f;
    if (editor_project_record_u32(record, "alm", &mode) &&
        mode <= (uint32_t)STYGIAN_EDITOR_AUTO_LAYOUT_VERTICAL) {
      node.as.frame.layout_mode = (StygianEditorAutoLayoutMode)mode;
    }
    if (editor_project_record_u32(record, "alw", &mode) &&
        mode <= (uint32_t)STYGIAN_EDITOR_AUTO_LAYOUT_WRAP) {
      node.as.frame.layout_wrap = (StygianEditorAutoLayoutWrap)mode;
    }
    (void)editor_project_record_float(record, "apl",
                                      &node.as.frame.layout_padding_left);
    (void)editor_project_record_float(record, "apr",
                                      &node.as.frame.layout_padding_right);
    (void)editor_project_record_float(record, "apt",
                                      &node.as.frame.layout_padding_top);
    (void)editor_project_record_float(record, "apb",
                                      &node.as.frame.layout_padding_bottom);
    (void)editor_project_record_float(record, "ag", &node.as.frame.layout_gap);
    if (editor_project_record_u32(record, "apa", &mode) &&
        mode <= (uint32_t)STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_STRETCH) {
      node.as.frame.layout_primary_align = (StygianEditorAutoLayoutAlign)mode;
    }
    if (editor_project_record_u32(record, "aca", &mode) &&
        mode <= (uint32_t)STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_STRETCH) {
      node.as.frame.layout_cross_align = (StygianEditorAutoLayoutAlign)mode;
    }
    if (editor_project_record_u32(record, "ofp", &mode) &&
        mode <= (uint32_t)STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_BOTH) {
      node.as.frame.overflow_policy = (StygianEditorFrameOverflowPolicy)mode;
    }
    (void)editor_project_record_float(record, "sx", &node.as.frame.scroll_x);
    (void)editor_project_record_float(record, "sy", &node.as.frame.scroll_y);
  } else if (node.kind == STYGIAN_EDITOR_SHAPE_TEXT) {
    uint32_t mode = 0u;
    if (!editor_project_record_float(record, "x", &node.as.text.x) ||
        !editor_project_record_float(record, "y", &node.as.text.y) ||
        !editor_project_record_float(record, "w", &node.as.text.w) ||
        !editor_project_record_float(record, "h", &node.as.text.h) ||
        !editor_project_record_float(record, "size", &node.as.text.font_size) ||
        !editor_project_record_string(record, "fill", field, sizeof(field)) ||
        !editor_project_parse_color_text(field, &node.as.text.fill) ||
        !editor_project_record_string(record, "txt", node.as.text.text,
                                      sizeof(node.as.text.text))) {
      return false;
    }
    node.as.text.line_height = 0.0f;
    node.as.text.letter_spacing = 0.0f;
    node.as.text.font_weight = 400u;
    node.as.text.box_mode = STYGIAN_EDITOR_TEXT_BOX_AREA;
    node.as.text.align_h = STYGIAN_EDITOR_TEXT_ALIGN_LEFT;
    node.as.text.align_v = STYGIAN_EDITOR_TEXT_ALIGN_TOP;
    node.as.text.auto_size = STYGIAN_EDITOR_TEXT_AUTOSIZE_NONE;
    node.as.text.span_count = 0u;
    (void)editor_project_record_float(record, "lh", &node.as.text.line_height);
    (void)editor_project_record_float(record, "ls", &node.as.text.letter_spacing);
    (void)editor_project_record_u32(record, "fw", &node.as.text.font_weight);
    if (editor_project_record_u32(record, "tb", &mode) &&
        mode <= (uint32_t)STYGIAN_EDITOR_TEXT_BOX_AREA) {
      node.as.text.box_mode = (StygianEditorTextBoxMode)mode;
    }
    if (editor_project_record_u32(record, "ah", &mode) &&
        mode <= (uint32_t)STYGIAN_EDITOR_TEXT_ALIGN_RIGHT) {
      node.as.text.align_h = (StygianEditorTextHAlign)mode;
    }
    if (editor_project_record_u32(record, "av", &mode) &&
        mode <= (uint32_t)STYGIAN_EDITOR_TEXT_ALIGN_BOTTOM) {
      node.as.text.align_v = (StygianEditorTextVAlign)mode;
    }
    if (editor_project_record_u32(record, "as", &mode) &&
        mode <= (uint32_t)STYGIAN_EDITOR_TEXT_AUTOSIZE_BOTH) {
      node.as.text.auto_size = (StygianEditorTextAutoSize)mode;
    }
    if (editor_project_record_string(record, "tspans", field, sizeof(field))) {
      if (!editor_project_parse_text_spans(field, &node))
        return false;
    }
  } else if (node.kind == STYGIAN_EDITOR_SHAPE_IMAGE) {
    if (!editor_project_record_float(record, "x", &node.as.image.x) ||
        !editor_project_record_float(record, "y", &node.as.image.y) ||
        !editor_project_record_float(record, "w", &node.as.image.w) ||
        !editor_project_record_float(record, "h", &node.as.image.h) ||
        !editor_project_record_u32(record, "fit", &node.as.image.fit_mode) ||
        !editor_project_record_string(record, "src", node.as.image.source,
                                      sizeof(node.as.image.source))) {
      return false;
    }
  } else if (node.kind == STYGIAN_EDITOR_SHAPE_LINE) {
    if (!editor_project_record_float(record, "x1", &node.as.line.x1) ||
        !editor_project_record_float(record, "y1", &node.as.line.y1) ||
        !editor_project_record_float(record, "x2", &node.as.line.x2) ||
        !editor_project_record_float(record, "y2", &node.as.line.y2) ||
        !editor_project_record_float(record, "th", &node.as.line.thickness) ||
        !editor_project_record_string(record, "stroke", field, sizeof(field)) ||
        !editor_project_parse_color_text(field, &node.as.line.stroke)) {
      return false;
    }
  } else if (node.kind == STYGIAN_EDITOR_SHAPE_ARROW) {
    if (!editor_project_record_float(record, "x1", &node.as.arrow.x1) ||
        !editor_project_record_float(record, "y1", &node.as.arrow.y1) ||
        !editor_project_record_float(record, "x2", &node.as.arrow.x2) ||
        !editor_project_record_float(record, "y2", &node.as.arrow.y2) ||
        !editor_project_record_float(record, "th", &node.as.arrow.thickness) ||
        !editor_project_record_float(record, "hs", &node.as.arrow.head_size) ||
        !editor_project_record_string(record, "stroke", field, sizeof(field)) ||
        !editor_project_parse_color_text(field, &node.as.arrow.stroke)) {
      return false;
    }
  } else if (node.kind == STYGIAN_EDITOR_SHAPE_POLYGON) {
    if (!editor_project_record_float(record, "x", &node.as.polygon.x) ||
        !editor_project_record_float(record, "y", &node.as.polygon.y) ||
        !editor_project_record_float(record, "w", &node.as.polygon.w) ||
        !editor_project_record_float(record, "h", &node.as.polygon.h) ||
        !editor_project_record_u32(record, "sides", &node.as.polygon.sides) ||
        !editor_project_record_float(record, "corner", &node.as.polygon.corner_radius) ||
        !editor_project_record_string(record, "fill", field, sizeof(field)) ||
        !editor_project_parse_color_text(field, &node.as.polygon.fill)) {
      return false;
    }
  } else if (node.kind == STYGIAN_EDITOR_SHAPE_STAR) {
    if (!editor_project_record_float(record, "x", &node.as.star.x) ||
        !editor_project_record_float(record, "y", &node.as.star.y) ||
        !editor_project_record_float(record, "w", &node.as.star.w) ||
        !editor_project_record_float(record, "h", &node.as.star.h) ||
        !editor_project_record_u32(record, "pts", &node.as.star.points) ||
        !editor_project_record_float(record, "inner", &node.as.star.inner_ratio) ||
        !editor_project_record_string(record, "fill", field, sizeof(field)) ||
        !editor_project_parse_color_text(field, &node.as.star.fill)) {
      return false;
    }
  } else if (node.kind == STYGIAN_EDITOR_SHAPE_ARC) {
    if (!editor_project_record_float(record, "x", &node.as.arc.x) ||
        !editor_project_record_float(record, "y", &node.as.arc.y) ||
        !editor_project_record_float(record, "w", &node.as.arc.w) ||
        !editor_project_record_float(record, "h", &node.as.arc.h) ||
        !editor_project_record_float(record, "start", &node.as.arc.start_angle) ||
        !editor_project_record_float(record, "sweep", &node.as.arc.sweep_angle) ||
        !editor_project_record_float(record, "th", &node.as.arc.thickness) ||
        !editor_project_record_string(record, "stroke", field, sizeof(field)) ||
        !editor_project_parse_color_text(field, &node.as.arc.stroke)) {
      return false;
    }
  } else if (node.kind == STYGIAN_EDITOR_SHAPE_GROUP) {
    if (!editor_project_record_float(record, "x", &node.as.group.x) ||
        !editor_project_record_float(record, "y", &node.as.group.y) ||
        !editor_project_record_float(record, "w", &node.as.group.w) ||
        !editor_project_record_float(record, "h", &node.as.group.h) ||
        !editor_project_record_bool(record, "clip", &node.as.group.clip_content)) {
      return false;
    }
  } else if (node.kind == STYGIAN_EDITOR_SHAPE_COMPONENT_DEF) {
    if (!editor_project_record_float(record, "x", &node.as.component_def.x) ||
        !editor_project_record_float(record, "y", &node.as.component_def.y) ||
        !editor_project_record_float(record, "w", &node.as.component_def.w) ||
        !editor_project_record_float(record, "h", &node.as.component_def.h) ||
        !editor_project_record_string(record, "sym", node.as.component_def.symbol,
                                      sizeof(node.as.component_def.symbol))) {
      return false;
    }
    (void)editor_project_record_string(record, "vg",
                                       node.as.component_def.variant_group,
                                       sizeof(node.as.component_def.variant_group));
    (void)editor_project_record_string(record, "vn",
                                       node.as.component_def.variant_name,
                                       sizeof(node.as.component_def.variant_name));
    if (editor_project_record_string(record, "cprops", field, sizeof(field))) {
      if (!editor_project_parse_component_properties(field, &node.as.component_def))
        return false;
    }
  } else if (node.kind == STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE) {
    node.as.component_instance.component_def_id = STYGIAN_EDITOR_INVALID_ID;
    node.component_detached = false;
    if (!editor_project_record_float(record, "x", &node.as.component_instance.x) ||
        !editor_project_record_float(record, "y", &node.as.component_instance.y) ||
        !editor_project_record_float(record, "w", &node.as.component_instance.w) ||
        !editor_project_record_float(record, "h", &node.as.component_instance.h) ||
        !editor_project_record_string(record, "ref",
                                      node.as.component_instance.symbol_ref,
                                      sizeof(node.as.component_instance.symbol_ref))) {
      return false;
    }
    (void)editor_project_record_u32(record, "cid",
                                    &node.as.component_instance.component_def_id);
    (void)editor_project_record_bool(record, "idet", &node.component_detached);
    if (!editor_project_parse_component_overrides(
            record, &node.as.component_instance.overrides)) {
      return false;
    }
  } else {
    return false;
  }

  if (editor_node_supports_fill(node.kind)) {
    if (editor_project_record_string(record, "fills", field, sizeof(field))) {
      if (!editor_project_parse_fills_text(field, &node))
        return false;
      if (node.fill_count > 0u) {
        StygianEditorColor primary = editor_node_primary_fill_color(&node);
        editor_node_set_legacy_fill_color(&node, primary);
      }
    } else {
      editor_node_fill_sync_from_legacy(&node);
    }
  }
  if (editor_project_record_string(record, "strokes", field, sizeof(field))) {
    if (!editor_project_parse_strokes_text(field, &node))
      return false;
  } else if (editor_node_supports_stroke(node.kind)) {
    editor_node_seed_legacy_stroke(&node);
  }
  if (editor_project_record_string(record, "effects", field, sizeof(field))) {
    if (!editor_project_parse_effects_text(field, &node))
      return false;
  }
  node.mask_node_id = STYGIAN_EDITOR_INVALID_ID;
  node.mask_mode = STYGIAN_EDITOR_MASK_ALPHA;
  node.mask_invert = false;
  memset(&node.shader_attachment, 0, sizeof(node.shader_attachment));
  if (editor_project_record_u32(record, "mask", &node.mask_node_id)) {
    uint32_t mode = 0u;
    if (editor_project_record_u32(record, "maskm", &mode) &&
        mode <= (uint32_t)STYGIAN_EDITOR_MASK_LUMINANCE) {
      node.mask_mode = (StygianEditorMaskMode)mode;
    }
    (void)editor_project_record_bool(record, "maski", &node.mask_invert);
  }
  (void)editor_project_record_bool(record, "shen", &node.shader_attachment.enabled);
  (void)editor_project_record_string(record, "shslot",
                                     node.shader_attachment.slot_name,
                                     sizeof(node.shader_attachment.slot_name));
  (void)editor_project_record_string(record, "shasset",
                                     node.shader_attachment.asset_path,
                                     sizeof(node.shader_attachment.asset_path));
  (void)editor_project_record_string(record, "shentry",
                                     node.shader_attachment.entry_point,
                                     sizeof(node.shader_attachment.entry_point));
  editor_shader_attachment_sanitize(&node.shader_attachment);
  {
    uint32_t boolop = 0u;
    if (editor_project_record_u32(record, "boolop", &boolop)) {
      if (boolop <= (uint32_t)STYGIAN_EDITOR_BOOLEAN_EXCLUDE) {
        node.boolean_op = (StygianEditorBooleanOp)boolop;
        node.boolean_valid = true;
        if (editor_project_record_string(record, "boolids", field, sizeof(field))) {
          if (!editor_project_parse_u32_list(
                  field, '|', node.boolean_operands,
                  STYGIAN_EDITOR_NODE_BOOLEAN_OPERAND_CAP,
                  &node.boolean_operand_count)) {
            return false;
          }
        }
      } else {
        node.boolean_valid = false;
      }
    }
  }

  node.selected = false;
  if (!constraints_explicit) {
    float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
    if (editor_node_get_local_xy(&node, &x, &y) &&
        editor_node_get_local_wh(&node, &w, &h)) {
      node.constraint_left = x;
      node.constraint_top = y;
      node.constraint_right = 0.0f;
      node.constraint_bottom = 0.0f;
      node.constraint_center_dx = 0.0f;
      node.constraint_center_dy = 0.0f;
      node.constraint_x_ratio = 0.0f;
      node.constraint_y_ratio = 0.0f;
      node.constraint_w_ratio = 0.0f;
      node.constraint_h_ratio = 0.0f;
    }
  }
  editor->nodes[editor->node_count++] = node;
  if (editor->next_node_id <= node.id)
    editor->next_node_id = node.id + 1u;
  return true;
}

static bool editor_project_parse_behavior_record(StygianEditor *editor,
                                                 const char *record) {
  StygianEditorBehaviorSlot slot;
  char field[128];
  uint32_t action_kind = (uint32_t)STYGIAN_EDITOR_ACTION_ANIMATE;
  if (!editor || !record)
    return false;
  if (editor->behavior_count >= editor->max_behaviors)
    return false;
  memset(&slot, 0, sizeof(slot));

  if (!editor_project_record_u32(record, "id", &slot.id) ||
      slot.id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  if (!editor_project_record_u32(record, "trn", &slot.rule.trigger_node))
    return false;
  if (!editor_project_record_string(record, "ev", field, sizeof(field)) ||
      !editor_project_parse_event_name(field, &slot.rule.trigger_event)) {
    return false;
  }
  (void)editor_project_record_u32(record, "act", &action_kind);
  if (action_kind > (uint32_t)STYGIAN_EDITOR_ACTION_NAVIGATE)
    action_kind = (uint32_t)STYGIAN_EDITOR_ACTION_ANIMATE;
  slot.rule.action_kind = (StygianEditorBehaviorActionKind)action_kind;

  if (slot.rule.action_kind == STYGIAN_EDITOR_ACTION_ANIMATE) {
    if (!editor_project_record_u32(record, "tgt", &slot.rule.animate.target_node))
      return false;
    if (!editor_project_record_string(record, "prop", field, sizeof(field)) ||
        !editor_project_parse_property_name(field, &slot.rule.animate.property)) {
      return false;
    }
    if (!editor_project_record_float(record, "from", &slot.rule.animate.from_value) ||
        !editor_project_record_float(record, "to", &slot.rule.animate.to_value) ||
        !editor_project_record_u32(record, "dur", &slot.rule.animate.duration_ms) ||
        !editor_project_record_string(record, "ease", field, sizeof(field)) ||
        !editor_project_parse_easing_name(field, &slot.rule.animate.easing)) {
      return false;
    }
  } else if (slot.rule.action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY) {
    if (!editor_project_record_u32(record, "tgt",
                                   &slot.rule.set_property.target_node)) {
      return false;
    }
    if (!editor_project_record_string(record, "prop", field, sizeof(field)) ||
        !editor_project_parse_property_name(field, &slot.rule.set_property.property) ||
        !editor_project_record_float(record, "val", &slot.rule.set_property.value)) {
      return false;
    }
  } else if (slot.rule.action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY) {
    if (!editor_project_record_u32(record, "tgt",
                                   &slot.rule.toggle_visibility.target_node)) {
      return false;
    }
  } else if (slot.rule.action_kind == STYGIAN_EDITOR_ACTION_SET_VARIABLE) {
    uint32_t vk = 0u;
    (void)editor_project_record_string(record, "var",
                                       slot.rule.set_variable.variable_name,
                                       sizeof(slot.rule.set_variable.variable_name));
    if (editor_project_record_u32(record, "vk", &vk) &&
        vk <= (uint32_t)STYGIAN_EDITOR_VARIABLE_NUMBER) {
      slot.rule.set_variable.variable_kind = (StygianEditorVariableKind)vk;
    } else {
      slot.rule.set_variable.variable_kind = STYGIAN_EDITOR_VARIABLE_NUMBER;
    }
    (void)editor_project_record_bool(record, "vam",
                                     &slot.rule.set_variable.use_active_mode);
    (void)editor_project_record_u32(record, "vm",
                                    &slot.rule.set_variable.mode_index);
    (void)editor_project_record_float(record, "vnum",
                                      &slot.rule.set_variable.number_value);
    if (editor_project_record_string(record, "vcol", field, sizeof(field))) {
      (void)editor_project_parse_color_text(field,
                                            &slot.rule.set_variable.color_value);
    }
  } else if (slot.rule.action_kind == STYGIAN_EDITOR_ACTION_NAVIGATE) {
    (void)editor_project_record_string(record, "nav", slot.rule.navigate.target,
                                       sizeof(slot.rule.navigate.target));
  }

  editor->behaviors[editor->behavior_count++] = slot;
  if (editor->next_behavior_id <= slot.id)
    editor->next_behavior_id = slot.id + 1u;
  return true;
}

static bool editor_project_parse_transition_clip_record(const char *record) {
  uint32_t trigger_node = STYGIAN_EDITOR_INVALID_ID;
  uint32_t target_node = STYGIAN_EDITOR_INVALID_ID;
  uint32_t duration_ms = 0u;
  char field[128];
  StygianEditorEventKind event_kind = STYGIAN_EDITOR_EVENT_PRESS;
  StygianEditorPropertyKind property = STYGIAN_EDITOR_PROP_X;
  StygianEditorEasing easing = STYGIAN_EDITOR_EASING_LINEAR;
  float from_value = NAN;
  float to_value = 0.0f;
  if (!record)
    return false;
  if (!editor_project_record_u32(record, "trn", &trigger_node) ||
      !editor_project_record_string(record, "ev", field, sizeof(field)) ||
      !editor_project_parse_event_name(field, &event_kind) ||
      !editor_project_record_u32(record, "tgt", &target_node) ||
      !editor_project_record_string(record, "prop", field, sizeof(field)) ||
      !editor_project_parse_property_name(field, &property) ||
      !editor_project_record_float(record, "from", &from_value) ||
      !editor_project_record_float(record, "to", &to_value) ||
      !editor_project_record_u32(record, "dur", &duration_ms) ||
      !editor_project_record_string(record, "ease", field, sizeof(field)) ||
      !editor_project_parse_easing_name(field, &easing)) {
    return false;
  }
  return true;
}

static bool editor_project_parse_timeline_track_record(StygianEditor *editor,
                                                       const char *record) {
  StygianEditorTimelineTrackSlot slot;
  char field[512];
  if (!editor || !record)
    return false;
  if (editor->timeline_track_count >= editor->max_timeline_tracks)
    return false;
  memset(&slot, 0, sizeof(slot));
  if (!editor_project_record_u32(record, "id", &slot.id) ||
      slot.id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  slot.track.id = slot.id;
  if (!editor_project_record_u32(record, "node", &slot.track.target_node))
    return false;
  if (editor_project_record_u32(record, "layer", &slot.track.layer)) {
    /* optional */
  }
  if (!editor_project_record_string(record, "prop", field, sizeof(field)) ||
      !editor_project_parse_property_name(field, &slot.track.property)) {
    return false;
  }
  (void)editor_project_record_string(record, "name", slot.track.name,
                                     sizeof(slot.track.name));
  if (editor_project_record_string(record, "keys", field, sizeof(field)) &&
      field[0]) {
    char *at = field;
    while (at && *at) {
      char *sep = strchr(at, '|');
      char item[128];
      unsigned int t = 0u;
      float v = 0.0f;
      char ease[32];
      StygianEditorTimelineKeyframe kf;
      memset(&kf, 0, sizeof(kf));
      if (sep) {
        size_t n = (size_t)(sep - at);
        if (n >= sizeof(item))
          return false;
        memcpy(item, at, n);
        item[n] = '\0';
      } else {
        snprintf(item, sizeof(item), "%s", at);
      }
      if (sscanf(item, "%u:%f:%31s", &t, &v, ease) != 3)
        return false;
      kf.time_ms = (uint32_t)t;
      kf.value = v;
      if (!editor_project_parse_easing_name(ease, &kf.easing))
        return false;
      if (slot.track.keyframe_count >= STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP)
        return false;
      slot.track.keyframes[slot.track.keyframe_count++] = kf;
      at = sep ? (sep + 1) : NULL;
    }
  }
  if (slot.track.keyframe_count > 1u) {
    qsort(slot.track.keyframes, slot.track.keyframe_count,
          sizeof(StygianEditorTimelineKeyframe),
          editor_timeline_keyframe_compare);
  }
  editor->timeline_tracks[editor->timeline_track_count++] = slot;
  if (editor->next_timeline_track_id <= slot.id)
    editor->next_timeline_track_id = slot.id + 1u;
  return true;
}

static bool editor_project_parse_timeline_clip_record(StygianEditor *editor,
                                                      const char *record) {
  StygianEditorTimelineClipSlot slot;
  char field[1024];
  if (!editor || !record)
    return false;
  if (editor->timeline_clip_count >= editor->max_timeline_clips)
    return false;
  memset(&slot, 0, sizeof(slot));
  if (!editor_project_record_u32(record, "id", &slot.id) ||
      slot.id == STYGIAN_EDITOR_INVALID_ID)
    return false;
  slot.clip.id = slot.id;
  (void)editor_project_record_string(record, "name", slot.clip.name,
                                     sizeof(slot.clip.name));
  if (!editor_project_record_u32(record, "start", &slot.clip.start_ms) ||
      !editor_project_record_u32(record, "dur", &slot.clip.duration_ms))
    return false;
  (void)editor_project_record_u32(record, "layer", &slot.clip.layer);
  if (editor_project_record_string(record, "tracks", field, sizeof(field)) &&
      field[0]) {
    if (!editor_project_parse_u32_list(field, '|', slot.clip.track_ids,
                                       STYGIAN_EDITOR_TIMELINE_CLIP_TRACK_CAP,
                                       &slot.clip.track_count)) {
      return false;
    }
  }
  editor->timeline_clips[editor->timeline_clip_count++] = slot;
  if (editor->next_timeline_clip_id <= slot.id)
    editor->next_timeline_clip_id = slot.id + 1u;
  return true;
}

static bool editor_project_parse_driver_record(StygianEditor *editor,
                                               const char *record) {
  StygianEditorDriverSlot slot;
  uint32_t source_property = 0u;
  uint32_t mode_index = 0u;
  if (!editor || !record)
    return false;
  if (editor->driver_count >= editor->max_drivers)
    return false;
  memset(&slot, 0, sizeof(slot));
  if (!editor_project_record_u32(record, "id", &slot.id) ||
      slot.id == STYGIAN_EDITOR_INVALID_ID ||
      !editor_project_record_u32(record, "src", &slot.rule.source_node) ||
      slot.rule.source_node == STYGIAN_EDITOR_INVALID_ID ||
      !editor_project_record_u32(record, "prop", &source_property) ||
      !editor_project_record_string(record, "var", slot.rule.variable_name,
                                    sizeof(slot.rule.variable_name)) ||
      !slot.rule.variable_name[0] ||
      !editor_project_record_float(record, "in0", &slot.rule.in_min) ||
      !editor_project_record_float(record, "in1", &slot.rule.in_max) ||
      !editor_project_record_float(record, "out0", &slot.rule.out_min) ||
      !editor_project_record_float(record, "out1", &slot.rule.out_max) ||
      !editor_project_record_bool(record, "active", &slot.rule.use_active_mode) ||
      !editor_project_record_u32(record, "mode", &mode_index) ||
      !editor_project_record_bool(record, "clamp", &slot.rule.clamp_output)) {
    return false;
  }
  if (source_property > (uint32_t)STYGIAN_EDITOR_PROP_SHAPE_KIND)
    return false;
  slot.rule.source_property = (StygianEditorPropertyKind)source_property;
  slot.rule.mode_index = mode_index;
  editor->drivers[editor->driver_count++] = slot;
  if (editor->next_driver_id <= slot.id)
    editor->next_driver_id = slot.id + 1u;
  return true;
}

static bool editor_project_validate_links(StygianEditor *editor) {
  uint32_t i;
  if (!editor)
    return false;
  for (i = 0u; i < editor->node_count; ++i) {
    const StygianEditorNode *node = &editor->nodes[i];
    if (node->parent_id != STYGIAN_EDITOR_INVALID_ID &&
        editor_find_node_index(editor, node->parent_id) < 0)
      return false;
    if (node->parent_id != STYGIAN_EDITOR_INVALID_ID &&
        editor_node_is_ancestor(editor, node->id, node->parent_id))
      return false;
    if (node->mask_node_id != STYGIAN_EDITOR_INVALID_ID &&
        editor_find_node_index(editor, node->mask_node_id) < 0)
      return false;
    if (node->boolean_valid) {
      uint32_t j;
      if (node->boolean_operand_count == 0u)
        return false;
      for (j = 0u; j < node->boolean_operand_count &&
                  j < STYGIAN_EDITOR_NODE_BOOLEAN_OPERAND_CAP;
           ++j) {
        if (editor_find_node_index(editor, node->boolean_operands[j]) < 0)
          return false;
      }
      if (!editor_boolean_solve_node(editor, &editor->nodes[i]))
        return false;
    }
    if (node->kind == STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE &&
        !node->component_detached &&
        node->as.component_instance.component_def_id !=
            STYGIAN_EDITOR_INVALID_ID) {
      const StygianEditorNode *def = editor_find_node_const(
          editor, node->as.component_instance.component_def_id);
      uint32_t oi;
      if (!def || def->kind != STYGIAN_EDITOR_SHAPE_COMPONENT_DEF)
        return false;
      if ((node->as.component_instance.overrides.mask &
           STYGIAN_EDITOR_COMPONENT_OVERRIDE_SWAP) != 0u) {
        if (!editor_component_swap_is_legal(
                editor, def->id,
                node->as.component_instance.overrides.swap_component_def_id)) {
          return false;
        }
      }
      for (oi = 0u;
           oi < node->as.component_instance.overrides.property_override_count &&
           oi < STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP;
           ++oi) {
        if (!editor_component_property_value_valid_for_instance_context(
                editor, node, def,
                &node->as.component_instance.overrides.property_overrides[oi])) {
          return false;
        }
      }
    }
  }
  for (i = 0u; i < editor->timeline_track_count; ++i) {
    const StygianEditorTimelineTrack *track = &editor->timeline_tracks[i].track;
    if (editor_find_node_index(editor, track->target_node) < 0)
      return false;
    for (uint32_t k = 0u; k < track->keyframe_count; ++k) {
      if (k > 0u && track->keyframes[k].time_ms < track->keyframes[k - 1u].time_ms)
        return false;
    }
  }
  for (i = 0u; i < editor->timeline_clip_count; ++i) {
    const StygianEditorTimelineClip *clip = &editor->timeline_clips[i].clip;
    for (uint32_t k = 0u; k < clip->track_count; ++k) {
      if (editor_find_timeline_track_index(editor, clip->track_ids[k]) < 0)
        return false;
    }
  }
  for (i = 0u; i < editor->behavior_count; ++i) {
    const StygianEditorBehaviorRule *rule = &editor->behaviors[i].rule;
    if (editor_find_node_index(editor, rule->trigger_node) < 0)
      return false;
    if (rule->action_kind == STYGIAN_EDITOR_ACTION_ANIMATE &&
        editor_find_node_index(editor, rule->animate.target_node) < 0)
      return false;
    if (rule->action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY &&
        editor_find_node_index(editor, rule->set_property.target_node) < 0)
      return false;
    if (rule->action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY &&
        editor_find_node_index(editor, rule->toggle_visibility.target_node) < 0)
      return false;
  }
  for (i = 0u; i < editor->driver_count; ++i) {
    const StygianEditorDriverRule *rule = &editor->drivers[i].rule;
    if (editor_find_node_index(editor, rule->source_node) < 0)
      return false;
    if (!rule->variable_name[0] ||
        !editor_property_allows_number_variable(rule->source_property))
      return false;
    if (!rule->use_active_mode && rule->mode_index >= editor->variable_mode_count)
      return false;
    if (editor_find_variable(editor, rule->variable_name,
                             STYGIAN_EDITOR_VARIABLE_NUMBER) < 0) {
      return false;
    }
  }
  for (i = 0u; i < editor->guide_count; ++i) {
    const StygianEditorGuideSlot *guide = &editor->guides[i];
    if (guide->parent_id != STYGIAN_EDITOR_INVALID_ID &&
        editor_find_node_index(editor, guide->parent_id) < 0) {
      return false;
    }
  }
  return true;
}

size_t stygian_editor_build_project_json(const StygianEditor *editor,
                                         char *out_json,
                                         size_t out_capacity) {
  StygianEditorStringBuilder sb;
  uint32_t i;
  uint32_t transition_clip_count = 0u;
  bool first = true;

  if (!editor)
    return 0u;

  sb.dst = out_json;
  sb.cap = out_capacity;
  sb.len = 0u;

  sb_append_raw(&sb, "{\n");
  sb_append_raw(&sb, "  \"schema_name\": ");
  sb_append_json_string(&sb, STYGIAN_EDITOR_PROJECT_SCHEMA_NAME);
  sb_append_raw(&sb, ",\n");
  sb_appendf(&sb, "  \"schema_major\": %u,\n", STYGIAN_EDITOR_PROJECT_SCHEMA_MAJOR);
  sb_appendf(&sb, "  \"schema_minor\": %u,\n", STYGIAN_EDITOR_PROJECT_SCHEMA_MINOR);

  sb_append_raw(&sb, "  \"viewport\": \"");
  sb_appendf(&sb, "w=%.6f;h=%.6f;px=%.6f;py=%.6f;z=%.6f", editor->viewport.width,
             editor->viewport.height, editor->viewport.pan_x,
             editor->viewport.pan_y, editor->viewport.zoom);
  sb_append_raw(&sb, "\",\n");

  sb_append_raw(&sb, "  \"grid\": \"");
  sb_appendf(&sb, "en=%u;sub=%u;maj=%.6f;div=%u;min=%.6f;tol=%.6f",
             editor->grid.enabled ? 1u : 0u,
             editor->grid.sub_snap_enabled ? 1u : 0u, editor->grid.major_step_px,
             editor->grid.sub_divisions, editor->grid.min_minor_px,
             editor->grid.snap_tolerance_px);
  sb_append_raw(&sb, "\",\n");

  sb_append_raw(&sb, "  \"ruler\": \"");
  sb_appendf(&sb, "unit=%u", (uint32_t)editor->ruler_unit);
  sb_append_raw(&sb, "\",\n");

  sb_append_raw(&sb, "  \"snap_sources\": \"");
  sb_appendf(&sb, "grid=%u;guides=%u;bounds=%u;parent=%u",
             editor->snap_sources.use_grid ? 1u : 0u,
             editor->snap_sources.use_guides ? 1u : 0u,
             editor->snap_sources.use_bounds ? 1u : 0u,
             editor->snap_sources.use_parent_edges ? 1u : 0u);
  sb_append_raw(&sb, "\",\n");

  sb_append_raw(&sb, "  \"next_ids\": \"");
  sb_appendf(&sb,
             "node=%u;path=%u;behavior=%u;ttrack=%u;tclip=%u;driver=%u;guide=%u",
             editor->next_node_id, editor->next_path_id, editor->next_behavior_id,
             editor->next_timeline_track_id, editor->next_timeline_clip_id,
             editor->next_driver_id, editor->next_guide_id);
  sb_append_raw(&sb, "\",\n");

  sb_append_raw(&sb, "  \"selection\": \"primary=");
  sb_appendf(&sb, "%u;ids=", editor->selected_node);
  first = true;
  for (i = 0u; i < editor->node_count; ++i) {
    if (!editor->nodes[i].selected)
      continue;
    if (!first)
      sb_append_raw(&sb, "|");
    first = false;
    sb_appendf(&sb, "%u", editor->nodes[i].id);
  }
  sb_append_raw(&sb, "\",\n");

  sb_append_raw(&sb, "  \"color_tokens\": [");
  if (editor->color_token_count > 0u)
    sb_append_raw(&sb, "\n");
  for (i = 0u; i < editor->color_token_count; ++i) {
    const StygianEditorColorToken *token = &editor->color_tokens[i];
    sb_append_raw(&sb, "    \"n=");
    sb_append_record_escaped(&sb, token->name);
    sb_append_raw(&sb, ";c=");
    sb_appendf(&sb, "%.6f,%.6f,%.6f,%.6f", token->color.r, token->color.g,
               token->color.b, token->color.a);
    sb_append_raw(&sb, "\"");
    if (i + 1u < editor->color_token_count)
      sb_append_raw(&sb, ",");
    sb_append_raw(&sb, "\n");
  }
  if (editor->color_token_count > 0u)
    sb_append_raw(&sb, "  ");
  sb_append_raw(&sb, "],\n");

  sb_append_raw(&sb, "  \"guides\": [");
  if (editor->guide_count > 0u)
    sb_append_raw(&sb, "\n");
  for (i = 0u; i < editor->guide_count; ++i) {
    const StygianEditorGuideSlot *guide = &editor->guides[i];
    sb_append_raw(&sb, "    \"id=");
    sb_appendf(&sb, "%u;axis=%u;pos=%.6f;pid=%u\"", guide->id,
               (uint32_t)guide->axis, guide->position, guide->parent_id);
    if (i + 1u < editor->guide_count)
      sb_append_raw(&sb, ",");
    sb_append_raw(&sb, "\n");
  }
  if (editor->guide_count > 0u)
    sb_append_raw(&sb, "  ");
  sb_append_raw(&sb, "],\n");

  sb_append_raw(&sb, "  \"text_styles\": [");
  if (editor->text_style_count > 0u)
    sb_append_raw(&sb, "\n");
  for (i = 0u; i < editor->text_style_count; ++i) {
    const StygianEditorTextStyleDef *def = &editor->text_styles[i].def;
    sb_append_raw(&sb, "    \"n=");
    sb_append_record_escaped(&sb, def->name);
    sb_appendf(&sb, ";fs=%.6f;lh=%.6f;ls=%.6f;fw=%u;c=%.6f,%.6f,%.6f,%.6f\"",
               def->font_size, def->line_height, def->letter_spacing,
               def->font_weight, def->color.r, def->color.g, def->color.b,
               def->color.a);
    if (i + 1u < editor->text_style_count)
      sb_append_raw(&sb, ",");
    sb_append_raw(&sb, "\n");
  }
  if (editor->text_style_count > 0u)
    sb_append_raw(&sb, "  ");
  sb_append_raw(&sb, "],\n");

  sb_append_raw(&sb, "  \"effect_styles\": [");
  if (editor->effect_style_count > 0u)
    sb_append_raw(&sb, "\n");
  for (i = 0u; i < editor->effect_style_count; ++i) {
    const StygianEditorEffectStyleDef *def = &editor->effect_styles[i].def;
    uint32_t e;
    sb_append_raw(&sb, "    \"n=");
    sb_append_record_escaped(&sb, def->name);
    sb_appendf(&sb, ";ec=%u;ef=", def->effect_count);
    for (e = 0u; e < def->effect_count && e < STYGIAN_EDITOR_NODE_EFFECT_CAP; ++e) {
      const StygianEditorNodeEffect *effect = &def->effects[e];
      if (e > 0u)
        sb_append_raw(&sb, "|");
      sb_appendf(&sb, "%u,%u,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f:%.6f:%.6f:%.6f",
                 (uint32_t)effect->kind, effect->visible ? 1u : 0u,
                 effect->opacity, effect->radius, effect->spread, effect->offset_x,
                 effect->offset_y, effect->intensity, effect->color.r,
                 effect->color.g, effect->color.b, effect->color.a);
    }
    sb_append_raw(&sb, "\"");
    if (i + 1u < editor->effect_style_count)
      sb_append_raw(&sb, ",");
    sb_append_raw(&sb, "\n");
  }
  if (editor->effect_style_count > 0u)
    sb_append_raw(&sb, "  ");
  sb_append_raw(&sb, "],\n");

  sb_append_raw(&sb, "  \"layout_styles\": [");
  if (editor->layout_style_count > 0u)
    sb_append_raw(&sb, "\n");
  for (i = 0u; i < editor->layout_style_count; ++i) {
    const StygianEditorLayoutStyleDef *def = &editor->layout_styles[i].def;
    sb_append_raw(&sb, "    \"n=");
    sb_append_record_escaped(&sb, def->name);
    sb_appendf(&sb, ";m=%u;w=%u;pl=%.6f;pr=%.6f;pt=%.6f;pb=%.6f;g=%.6f;pa=%u;ca=%u\"",
               (uint32_t)def->layout.mode, (uint32_t)def->layout.wrap, def->layout.padding_left,
               def->layout.padding_right, def->layout.padding_top,
               def->layout.padding_bottom, def->layout.gap,
               (uint32_t)def->layout.primary_align,
               (uint32_t)def->layout.cross_align);
    if (i + 1u < editor->layout_style_count)
      sb_append_raw(&sb, ",");
    sb_append_raw(&sb, "\n");
  }
  if (editor->layout_style_count > 0u)
    sb_append_raw(&sb, "  ");
  sb_append_raw(&sb, "],\n");

  sb_append_raw(&sb, "  \"variable_modes\": [");
  if (editor->variable_mode_count > 0u)
    sb_append_raw(&sb, "\n");
  for (i = 0u; i < editor->variable_mode_count; ++i) {
    sb_append_raw(&sb, "    \"");
    sb_append_record_escaped(&sb, editor->variable_modes[i]);
    sb_append_raw(&sb, "\"");
    if (i + 1u < editor->variable_mode_count)
      sb_append_raw(&sb, ",");
    sb_append_raw(&sb, "\n");
  }
  if (editor->variable_mode_count > 0u)
    sb_append_raw(&sb, "  ");
  sb_append_raw(&sb, "],\n");

  sb_appendf(&sb, "  \"active_variable_mode\": %u,\n", editor->active_variable_mode);

  sb_append_raw(&sb, "  \"variables\": [");
  if (editor->variable_count > 0u)
    sb_append_raw(&sb, "\n");
  for (i = 0u; i < editor->variable_count; ++i) {
    const StygianEditorVariableDef *def = &editor->variables[i].def;
    uint32_t m;
    sb_append_raw(&sb, "    \"n=");
    sb_append_record_escaped(&sb, def->name);
    sb_appendf(&sb, ";k=%u;vals=", (uint32_t)def->kind);
    for (m = 0u; m < editor->variable_mode_count &&
                m < STYGIAN_EDITOR_VARIABLE_MODE_CAP;
         ++m) {
      if (m > 0u)
        sb_append_raw(&sb, "|");
      if (def->kind == STYGIAN_EDITOR_VARIABLE_COLOR) {
        sb_appendf(&sb, "%.6f,%.6f,%.6f,%.6f", def->color_values[m].r,
                   def->color_values[m].g, def->color_values[m].b,
                   def->color_values[m].a);
      } else {
        sb_appendf(&sb, "%.6f", def->number_values[m]);
      }
    }
    sb_append_raw(&sb, "\"");
    if (i + 1u < editor->variable_count)
      sb_append_raw(&sb, ",");
    sb_append_raw(&sb, "\n");
  }
  if (editor->variable_count > 0u)
    sb_append_raw(&sb, "  ");
  sb_append_raw(&sb, "],\n");

  sb_append_raw(&sb, "  \"nodes\": [");
  if (editor->node_count > 0u)
    sb_append_raw(&sb, "\n");
  for (i = 0u; i < editor->node_count; ++i) {
    const StygianEditorNode *node = &editor->nodes[i];
    sb_append_raw(&sb, "    \"id=");
    sb_appendf(&sb, "%u;pid=%u;k=%s;v=%u;lock=%u;z=%.6f;val=%.6f;rot=%.6f;name=",
               node->id, node->parent_id,
               editor_project_kind_name(node->kind), node->visible ? 1u : 0u,
               node->locked ? 1u : 0u, node->z, node->value, node->rotation_deg);
    sb_append_record_escaped(&sb, node->name);
    sb_appendf(&sb,
               ";hc=%u;vc=%u;cl=%.6f;cr=%.6f;ct=%.6f;cb=%.6f;ccx=%.6f;ccy=%.6f;"
               "cxr=%.6f;cyr=%.6f;cwr=%.6f;chr=%.6f;la=%u;lsw=%u;lsh=%u;mnw=%.6f;mxw=%.6f;mnh=%.6f;mxh=%.6f;tok=",
               (uint32_t)node->constraint_h, (uint32_t)node->constraint_v,
               node->constraint_left, node->constraint_right,
               node->constraint_top, node->constraint_bottom,
               node->constraint_center_dx, node->constraint_center_dy,
               node->constraint_x_ratio, node->constraint_y_ratio,
               node->constraint_w_ratio, node->constraint_h_ratio,
               node->layout_absolute ? 1u : 0u,
               (uint32_t)node->layout_sizing_h,
               (uint32_t)node->layout_sizing_v, node->size_min_w,
               node->size_max_w, node->size_min_h, node->size_max_h);
    sb_append_record_escaped(&sb, node->color_token);
    sb_append_raw(&sb, ";ts=");
    sb_append_record_escaped(&sb, node->text_style);
    sb_append_raw(&sb, ";es=");
    sb_append_record_escaped(&sb, node->effect_style);
    sb_append_raw(&sb, ";ls=");
    sb_append_record_escaped(&sb, node->layout_style);
    sb_append_raw(&sb, ";cvar=");
    sb_append_record_escaped(&sb, node->color_variable);
    sb_append_raw(&sb, ";nvar=");
    sb_append_record_escaped(&sb, node->number_variable);
    sb_appendf(&sb, ";nprop=%u", (uint32_t)node->number_variable_property);
    if (node->kind == STYGIAN_EDITOR_SHAPE_RECT) {
      sb_appendf(&sb,
                 ";x=%.6f;y=%.6f;w=%.6f;h=%.6f;rad=%.6f,%.6f,%.6f,%.6f;fill="
                 "%.6f,%.6f,%.6f,%.6f",
                 node->as.rect.x, node->as.rect.y, node->as.rect.w,
                 node->as.rect.h, node->as.rect.radius[0], node->as.rect.radius[1],
                 node->as.rect.radius[2], node->as.rect.radius[3],
                 node->as.rect.fill.r, node->as.rect.fill.g, node->as.rect.fill.b,
                 node->as.rect.fill.a);
      editor_project_append_fills(&sb, node);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_ELLIPSE) {
      sb_appendf(&sb, ";x=%.6f;y=%.6f;w=%.6f;h=%.6f;fill=%.6f,%.6f,%.6f,%.6f",
                 node->as.ellipse.x, node->as.ellipse.y, node->as.ellipse.w,
                 node->as.ellipse.h, node->as.ellipse.fill.r,
                 node->as.ellipse.fill.g, node->as.ellipse.fill.b,
                 node->as.ellipse.fill.a);
      editor_project_append_fills(&sb, node);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_PATH) {
      uint32_t p;
      sb_appendf(&sb, ";closed=%u;th=%.6f;stroke=%.6f,%.6f,%.6f,%.6f;pts=",
                 node->as.path.closed ? 1u : 0u, node->as.path.thickness,
                 node->as.path.stroke.r, node->as.path.stroke.g,
                 node->as.path.stroke.b, node->as.path.stroke.a);
      for (p = 0u; p < node->as.path.point_count; ++p) {
        StygianEditorPoint point =
            editor->path_points[node->as.path.first_point + p];
        if (p > 0u)
          sb_append_raw(&sb, "|");
        sb_appendf(&sb, "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%u", point.x, point.y,
                   point.in_x, point.in_y, point.out_x, point.out_y,
                   (uint32_t)point.kind);
      }
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_FRAME) {
      sb_appendf(&sb, ";x=%.6f;y=%.6f;w=%.6f;h=%.6f;clip=%u;alm=%u;alw=%u;apl=%.6f;apr=%.6f;apt=%.6f;apb=%.6f;ag=%.6f;apa=%u;aca=%u;ofp=%u;sx=%.6f;sy=%.6f;fill=%.6f,%.6f,%.6f,%.6f",
                 node->as.frame.x, node->as.frame.y, node->as.frame.w,
                 node->as.frame.h, node->as.frame.clip_content ? 1u : 0u,
                 (uint32_t)node->as.frame.layout_mode,
                 (uint32_t)node->as.frame.layout_wrap,
                 node->as.frame.layout_padding_left,
                 node->as.frame.layout_padding_right,
                 node->as.frame.layout_padding_top,
                 node->as.frame.layout_padding_bottom,
                 node->as.frame.layout_gap,
                 (uint32_t)node->as.frame.layout_primary_align,
                 (uint32_t)node->as.frame.layout_cross_align,
                 (uint32_t)node->as.frame.overflow_policy,
                 node->as.frame.scroll_x, node->as.frame.scroll_y,
                 node->as.frame.fill.r, node->as.frame.fill.g,
                 node->as.frame.fill.b, node->as.frame.fill.a);
      editor_project_append_fills(&sb, node);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_TEXT) {
      sb_appendf(&sb,
                 ";x=%.6f;y=%.6f;w=%.6f;h=%.6f;size=%.6f;lh=%.6f;ls=%.6f;fw=%u;tb=%u;ah=%u;av=%u;as=%u;fill=%.6f,%.6f,%.6f,%.6f;txt=",
                 node->as.text.x, node->as.text.y, node->as.text.w,
                 node->as.text.h, node->as.text.font_size,
                 node->as.text.line_height, node->as.text.letter_spacing,
                 node->as.text.font_weight, (uint32_t)node->as.text.box_mode,
                 (uint32_t)node->as.text.align_h,
                 (uint32_t)node->as.text.align_v,
                 (uint32_t)node->as.text.auto_size, node->as.text.fill.r,
                 node->as.text.fill.g, node->as.text.fill.b, node->as.text.fill.a);
      sb_append_record_escaped(&sb, node->as.text.text);
      editor_project_append_fills(&sb, node);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_IMAGE) {
      sb_appendf(&sb, ";x=%.6f;y=%.6f;w=%.6f;h=%.6f;fit=%u;src=",
                 node->as.image.x, node->as.image.y, node->as.image.w,
                 node->as.image.h, node->as.image.fit_mode);
      sb_append_record_escaped(&sb, node->as.image.source);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_LINE) {
      sb_appendf(&sb,
                 ";x1=%.6f;y1=%.6f;x2=%.6f;y2=%.6f;th=%.6f;stroke=%.6f,%.6f,%.6f,%.6f",
                 node->as.line.x1, node->as.line.y1, node->as.line.x2,
                 node->as.line.y2, node->as.line.thickness, node->as.line.stroke.r,
                 node->as.line.stroke.g, node->as.line.stroke.b,
                 node->as.line.stroke.a);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_ARROW) {
      sb_appendf(&sb,
                 ";x1=%.6f;y1=%.6f;x2=%.6f;y2=%.6f;th=%.6f;hs=%.6f;stroke=%.6f,%.6f,%.6f,%.6f",
                 node->as.arrow.x1, node->as.arrow.y1, node->as.arrow.x2,
                 node->as.arrow.y2, node->as.arrow.thickness,
                 node->as.arrow.head_size, node->as.arrow.stroke.r,
                 node->as.arrow.stroke.g, node->as.arrow.stroke.b,
                 node->as.arrow.stroke.a);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_POLYGON) {
      sb_appendf(&sb,
                 ";x=%.6f;y=%.6f;w=%.6f;h=%.6f;sides=%u;corner=%.6f;fill=%.6f,%.6f,%.6f,%.6f",
                 node->as.polygon.x, node->as.polygon.y, node->as.polygon.w,
                 node->as.polygon.h, node->as.polygon.sides,
                 node->as.polygon.corner_radius, node->as.polygon.fill.r,
                 node->as.polygon.fill.g, node->as.polygon.fill.b,
                 node->as.polygon.fill.a);
      editor_project_append_fills(&sb, node);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_STAR) {
      sb_appendf(&sb,
                 ";x=%.6f;y=%.6f;w=%.6f;h=%.6f;pts=%u;inner=%.6f;fill=%.6f,%.6f,%.6f,%.6f",
                 node->as.star.x, node->as.star.y, node->as.star.w,
                 node->as.star.h, node->as.star.points, node->as.star.inner_ratio,
                 node->as.star.fill.r, node->as.star.fill.g, node->as.star.fill.b,
                 node->as.star.fill.a);
      editor_project_append_fills(&sb, node);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_ARC) {
      sb_appendf(&sb,
                 ";x=%.6f;y=%.6f;w=%.6f;h=%.6f;start=%.6f;sweep=%.6f;th=%.6f;stroke=%.6f,%.6f,%.6f,%.6f",
                 node->as.arc.x, node->as.arc.y, node->as.arc.w, node->as.arc.h,
                 node->as.arc.start_angle, node->as.arc.sweep_angle,
                 node->as.arc.thickness, node->as.arc.stroke.r,
                 node->as.arc.stroke.g, node->as.arc.stroke.b,
                 node->as.arc.stroke.a);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_GROUP) {
      sb_appendf(&sb, ";x=%.6f;y=%.6f;w=%.6f;h=%.6f;clip=%u",
                 node->as.group.x, node->as.group.y, node->as.group.w,
                 node->as.group.h, node->as.group.clip_content ? 1u : 0u);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_COMPONENT_DEF) {
      sb_appendf(&sb, ";x=%.6f;y=%.6f;w=%.6f;h=%.6f;sym=",
                 node->as.component_def.x, node->as.component_def.y,
                 node->as.component_def.w, node->as.component_def.h);
      sb_append_record_escaped(&sb, node->as.component_def.symbol);
      sb_append_raw(&sb, ";vg=");
      sb_append_record_escaped(&sb, node->as.component_def.variant_group);
      sb_append_raw(&sb, ";vn=");
      sb_append_record_escaped(&sb, node->as.component_def.variant_name);
      editor_project_append_component_properties(&sb, &node->as.component_def);
    } else if (node->kind == STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE) {
      sb_appendf(&sb, ";x=%.6f;y=%.6f;w=%.6f;h=%.6f;cid=%u;idet=%u;ref=",
                 node->as.component_instance.x, node->as.component_instance.y,
                 node->as.component_instance.w, node->as.component_instance.h,
                 node->as.component_instance.component_def_id,
                 node->component_detached ? 1u : 0u);
      sb_append_record_escaped(&sb, node->as.component_instance.symbol_ref);
      editor_project_append_component_overrides(
          &sb, &node->as.component_instance.overrides);
    } else {
      return 0u;
    }
    editor_project_append_node_extras(&sb, node);
    sb_append_raw(&sb, "\"");
    if (i + 1u < editor->node_count)
      sb_append_raw(&sb, ",");
    sb_append_raw(&sb, "\n");
  }
  if (editor->node_count > 0u)
    sb_append_raw(&sb, "  ");
  sb_append_raw(&sb, "],\n");

  sb_append_raw(&sb, "  \"behaviors\": [");
  if (editor->behavior_count > 0u)
    sb_append_raw(&sb, "\n");
  for (i = 0u; i < editor->behavior_count; ++i) {
    const StygianEditorBehaviorSlot *slot = &editor->behaviors[i];
    const StygianEditorBehaviorRule *rule = &slot->rule;
    sb_append_raw(&sb, "    \"id=");
    sb_appendf(&sb, "%u;trn=%u;ev=%s;act=%u", slot->id, rule->trigger_node,
               editor_project_event_name(rule->trigger_event),
               (uint32_t)rule->action_kind);
    if (rule->action_kind == STYGIAN_EDITOR_ACTION_ANIMATE) {
      sb_appendf(&sb, ";tgt=%u;prop=%s;from=", rule->animate.target_node,
                 editor_project_property_name(rule->animate.property));
      if (editor_float_isnan(rule->animate.from_value))
        sb_append_raw(&sb, "null");
      else
        sb_appendf(&sb, "%.6f", rule->animate.from_value);
      sb_appendf(&sb, ";to=%.6f;dur=%u;ease=%s", rule->animate.to_value,
                 rule->animate.duration_ms,
                 editor_project_easing_name(rule->animate.easing));
    } else if (rule->action_kind == STYGIAN_EDITOR_ACTION_SET_PROPERTY) {
      sb_appendf(&sb, ";tgt=%u;prop=%s;val=%.6f",
                 rule->set_property.target_node,
                 editor_project_property_name(rule->set_property.property),
                 rule->set_property.value);
    } else if (rule->action_kind == STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY) {
      sb_appendf(&sb, ";tgt=%u", rule->toggle_visibility.target_node);
    } else if (rule->action_kind == STYGIAN_EDITOR_ACTION_SET_VARIABLE) {
      sb_appendf(&sb, ";var=");
      sb_append_record_escaped(&sb, rule->set_variable.variable_name);
      sb_appendf(&sb, ";vk=%u;vam=%u;vm=%u;vnum=%.6f;vcol=%.6f,%.6f,%.6f,%.6f",
                 (uint32_t)rule->set_variable.variable_kind,
                 rule->set_variable.use_active_mode ? 1u : 0u,
                 rule->set_variable.mode_index, rule->set_variable.number_value,
                 rule->set_variable.color_value.r, rule->set_variable.color_value.g,
                 rule->set_variable.color_value.b, rule->set_variable.color_value.a);
    } else if (rule->action_kind == STYGIAN_EDITOR_ACTION_NAVIGATE) {
      sb_append_raw(&sb, ";nav=");
      sb_append_record_escaped(&sb, rule->navigate.target);
    }
    sb_append_raw(&sb, "\"");
    if (i + 1u < editor->behavior_count)
      sb_append_raw(&sb, ",");
    sb_append_raw(&sb, "\n");
  }
  if (editor->behavior_count > 0u)
    sb_append_raw(&sb, "  ");
  sb_append_raw(&sb, "],\n");

  sb_append_raw(&sb, "  \"timeline_tracks\": [");
  if (editor->timeline_track_count > 0u)
    sb_append_raw(&sb, "\n");
  for (i = 0u; i < editor->timeline_track_count; ++i) {
    const StygianEditorTimelineTrack *track = &editor->timeline_tracks[i].track;
    sb_append_raw(&sb, "    \"id=");
    sb_appendf(&sb, "%u;node=%u;prop=%s;layer=%u;name=", track->id,
               track->target_node, editor_project_property_name(track->property),
               track->layer);
    sb_append_record_escaped(&sb, track->name);
    sb_append_raw(&sb, ";keys=");
    for (uint32_t k = 0u; k < track->keyframe_count; ++k) {
      if (k > 0u)
        sb_append_raw(&sb, "|");
      sb_appendf(&sb, "%u:%.6f:%s", track->keyframes[k].time_ms,
                 track->keyframes[k].value,
                 editor_project_easing_name(track->keyframes[k].easing));
    }
    sb_append_raw(&sb, "\"");
    if (i + 1u < editor->timeline_track_count)
      sb_append_raw(&sb, ",");
    sb_append_raw(&sb, "\n");
  }
  if (editor->timeline_track_count > 0u)
    sb_append_raw(&sb, "  ");
  sb_append_raw(&sb, "],\n");

  sb_append_raw(&sb, "  \"timeline_clips\": [");
  if (editor->timeline_clip_count > 0u)
    sb_append_raw(&sb, "\n");
  for (i = 0u; i < editor->timeline_clip_count; ++i) {
    const StygianEditorTimelineClip *clip = &editor->timeline_clips[i].clip;
    sb_append_raw(&sb, "    \"id=");
    sb_appendf(&sb, "%u;name=", clip->id);
    sb_append_record_escaped(&sb, clip->name);
    sb_appendf(&sb, ";start=%u;dur=%u;layer=%u;tracks=", clip->start_ms,
               clip->duration_ms, clip->layer);
    for (uint32_t k = 0u; k < clip->track_count; ++k) {
      if (k > 0u)
        sb_append_raw(&sb, "|");
      sb_appendf(&sb, "%u", clip->track_ids[k]);
    }
    sb_append_raw(&sb, "\"");
    if (i + 1u < editor->timeline_clip_count)
      sb_append_raw(&sb, ",");
    sb_append_raw(&sb, "\n");
  }
  if (editor->timeline_clip_count > 0u)
    sb_append_raw(&sb, "  ");
  sb_append_raw(&sb, "],\n");

  sb_append_raw(&sb, "  \"drivers\": [");
  if (editor->driver_count > 0u)
    sb_append_raw(&sb, "\n");
  for (i = 0u; i < editor->driver_count; ++i) {
    const StygianEditorDriverSlot *slot = &editor->drivers[i];
    const StygianEditorDriverRule *rule = &slot->rule;
    sb_append_raw(&sb, "    \"id=");
    sb_appendf(&sb, "%u;src=%u;prop=%u;var=", slot->id, rule->source_node,
               (uint32_t)rule->source_property);
    sb_append_record_escaped(&sb, rule->variable_name);
    sb_appendf(&sb,
               ";active=%u;mode=%u;in0=%.6f;in1=%.6f;out0=%.6f;out1=%.6f;clamp=%u\"",
               rule->use_active_mode ? 1u : 0u, rule->mode_index, rule->in_min,
               rule->in_max, rule->out_min, rule->out_max,
               rule->clamp_output ? 1u : 0u);
    if (i + 1u < editor->driver_count)
      sb_append_raw(&sb, ",");
    sb_append_raw(&sb, "\n");
  }
  if (editor->driver_count > 0u)
    sb_append_raw(&sb, "  ");
  sb_append_raw(&sb, "],\n");

  for (i = 0u; i < editor->behavior_count; ++i) {
    if (editor->behaviors[i].rule.action_kind == STYGIAN_EDITOR_ACTION_ANIMATE)
      transition_clip_count += 1u;
  }
  sb_append_raw(&sb, "  \"transition_clips\": [");
  if (transition_clip_count > 0u)
    sb_append_raw(&sb, "\n");
  {
    uint32_t emitted = 0u;
    for (i = 0u; i < editor->behavior_count; ++i) {
      const StygianEditorBehaviorRule *rule = &editor->behaviors[i].rule;
      if (rule->action_kind != STYGIAN_EDITOR_ACTION_ANIMATE)
        continue;
      sb_append_raw(&sb, "    \"trn=");
      sb_appendf(&sb, "%u;ev=%s;tgt=%u;prop=%s;from=", rule->trigger_node,
                 editor_project_event_name(rule->trigger_event),
                 rule->animate.target_node,
                 editor_project_property_name(rule->animate.property));
      if (editor_float_isnan(rule->animate.from_value))
        sb_append_raw(&sb, "null");
      else
        sb_appendf(&sb, "%.6f", rule->animate.from_value);
      sb_appendf(&sb, ";to=%.6f;dur=%u;ease=%s\"",
                 rule->animate.to_value, rule->animate.duration_ms,
                 editor_project_easing_name(rule->animate.easing));
      emitted += 1u;
      if (emitted < transition_clip_count)
        sb_append_raw(&sb, ",");
      sb_append_raw(&sb, "\n");
    }
  }
  if (transition_clip_count > 0u)
    sb_append_raw(&sb, "  ");
  sb_append_raw(&sb, "]\n");
  sb_append_raw(&sb, "}\n");

  sb_finish(&sb);
  return sb.len + 1u;
}

bool stygian_editor_load_project_json(StygianEditor *editor, const char *json) {
  bool external_load = false;
  StygianEditorConfig cfg;
  StygianEditor *scratch;
  StygianEditorProjectLoadState load_state;
  const char *arr_begin = NULL;
  const char *arr_end = NULL;
  const char *it = NULL;
  bool done = false;
  char text[2048];
  uint32_t schema_major = 0u;
  uint32_t schema_minor = 0u;

  if (!editor || !json)
    return false;
  external_load = !editor->history_replaying && !editor->history_suspended &&
                  !editor->transaction_active;
  memset(&load_state, 0, sizeof(load_state));

  if (!editor_project_find_string(json, "schema_name", text, sizeof(text)))
    return editor_project_fail(editor, "Project is missing schema_name.");
  if (strcmp(text, STYGIAN_EDITOR_PROJECT_SCHEMA_NAME) != 0)
    return editor_project_fail(editor, "Unsupported schema_name '%s'.", text);
  if (!editor_project_find_number(json, "schema_major", &schema_major) ||
      schema_major != STYGIAN_EDITOR_PROJECT_SCHEMA_MAJOR) {
    return editor_project_fail(editor, "Unsupported schema_major %u.", schema_major);
  }
  if (!editor_project_find_number(json, "schema_minor", &schema_minor))
    return editor_project_fail(editor, "Project is missing schema_minor.");
  if (schema_minor > STYGIAN_EDITOR_PROJECT_SCHEMA_MINOR) {
    return editor_project_fail(
        editor,
        "Unsupported schema_minor %u (loader supports up to %u). Upgrade editor.",
        schema_minor, STYGIAN_EDITOR_PROJECT_SCHEMA_MINOR);
  }
  if (schema_minor < STYGIAN_EDITOR_PROJECT_SCHEMA_MINOR) {
    editor_logf(
        editor, STYGIAN_EDITOR_LOG_INFO,
        "Migrating schema_minor %u -> %u with backward-compatible defaults.",
        schema_minor, STYGIAN_EDITOR_PROJECT_SCHEMA_MINOR);
  }

  cfg.max_nodes = editor->max_nodes;
  cfg.max_path_points = editor->max_path_points;
  cfg.max_behaviors = editor->max_behaviors;
  cfg.max_color_tokens = editor->max_color_tokens;
  cfg.max_timeline_tracks = editor->max_timeline_tracks;
  cfg.max_timeline_clips = editor->max_timeline_clips;
  cfg.max_drivers = editor->max_drivers;
  cfg.grid = editor->grid;
  scratch = stygian_editor_create(&cfg, &editor->host);
  if (!scratch)
    return editor_project_fail(editor, "Failed to allocate scratch editor.");

  load_state.selected_cap = scratch->max_nodes;
  load_state.selected_ids = (StygianEditorNodeId *)calloc(
      (size_t)load_state.selected_cap, sizeof(StygianEditorNodeId));
  if (!load_state.selected_ids) {
    stygian_editor_destroy(scratch);
    return editor_project_fail(editor, "Failed to allocate selection scratch.");
  }

  if (editor_project_find_string(json, "viewport", text, sizeof(text))) {
    editor_project_record_float(text, "w", &scratch->viewport.width);
    editor_project_record_float(text, "h", &scratch->viewport.height);
    editor_project_record_float(text, "px", &scratch->viewport.pan_x);
    editor_project_record_float(text, "py", &scratch->viewport.pan_y);
    editor_project_record_float(text, "z", &scratch->viewport.zoom);
    if (scratch->viewport.zoom < 0.0001f)
      scratch->viewport.zoom = 0.0001f;
  }
  if (editor_project_find_string(json, "grid", text, sizeof(text))) {
    editor_project_record_bool(text, "en", &scratch->grid.enabled);
    editor_project_record_bool(text, "sub", &scratch->grid.sub_snap_enabled);
    editor_project_record_float(text, "maj", &scratch->grid.major_step_px);
    editor_project_record_u32(text, "div", &scratch->grid.sub_divisions);
    editor_project_record_float(text, "min", &scratch->grid.min_minor_px);
    editor_project_record_float(text, "tol", &scratch->grid.snap_tolerance_px);
    if (scratch->grid.sub_divisions == 0u)
      scratch->grid.sub_divisions = 1u;
  }
  if (editor_project_find_string(json, "ruler", text, sizeof(text))) {
    uint32_t unit = 0u;
    if (editor_project_record_u32(text, "unit", &unit) &&
        unit <= (uint32_t)STYGIAN_EDITOR_RULER_UNIT_PX) {
      scratch->ruler_unit = (StygianEditorRulerUnit)unit;
    }
  }
  if (editor_project_find_string(json, "snap_sources", text, sizeof(text))) {
    (void)editor_project_record_bool(text, "grid",
                                     &scratch->snap_sources.use_grid);
    (void)editor_project_record_bool(text, "guides",
                                     &scratch->snap_sources.use_guides);
    (void)editor_project_record_bool(text, "bounds",
                                     &scratch->snap_sources.use_bounds);
    (void)editor_project_record_bool(text, "parent",
                                     &scratch->snap_sources.use_parent_edges);
  }
  if (editor_project_find_string(json, "next_ids", text, sizeof(text))) {
    if (editor_project_record_u32(text, "node", &scratch->next_node_id))
      load_state.next_node_explicit = true;
    if (editor_project_record_u32(text, "path", &scratch->next_path_id))
      load_state.next_path_explicit = true;
    if (editor_project_record_u32(text, "behavior", &scratch->next_behavior_id))
      load_state.next_behavior_explicit = true;
    if (editor_project_record_u32(text, "ttrack",
                                  &scratch->next_timeline_track_id)) {
      load_state.next_timeline_track_explicit = true;
    }
    if (editor_project_record_u32(text, "tclip",
                                  &scratch->next_timeline_clip_id)) {
      load_state.next_timeline_clip_explicit = true;
    }
    if (editor_project_record_u32(text, "driver", &scratch->next_driver_id))
      load_state.next_driver_explicit = true;
    if (editor_project_record_u32(text, "guide", &scratch->next_guide_id))
      load_state.next_guide_explicit = true;
  }
  if (editor_project_find_string(json, "selection", text, sizeof(text))) {
    char ids[1024];
    if (editor_project_record_u32(text, "primary", &load_state.primary_node_id))
      load_state.primary_explicit = true;
    if (editor_project_record_field(text, "ids", ids, sizeof(ids))) {
      if (!editor_project_parse_u32_list(ids, '|', load_state.selected_ids,
                                         load_state.selected_cap,
                                         &load_state.selected_count)) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Bad selection ids list.");
      }
    }
  }

  if (editor_project_find_container(json, "color_tokens", '[', ']', &arr_begin,
                                    &arr_end)) {
    it = arr_begin + 1;
    done = false;
    while (!done && editor_project_next_json_string(&it, arr_end, text,
                                                    sizeof(text), &done)) {
      StygianEditorColorToken token;
      char name[64];
      char color_text[128];
      if (done)
        break;
      memset(&token, 0, sizeof(token));
      if (!editor_project_record_string(text, "n", name, sizeof(name)) ||
          !editor_project_record_string(text, "c", color_text, sizeof(color_text)) ||
          !editor_project_parse_color_text(color_text, &token.color)) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Bad color token record.");
      }
      if (strlen(name) >= sizeof(token.name)) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Color token name too long.");
      }
      memcpy(token.name, name, strlen(name) + 1u);
      if (scratch->color_token_count >= scratch->max_color_tokens) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Color token capacity exceeded.");
      }
      scratch->color_tokens[scratch->color_token_count++] = token;
    }
  }

  if (editor_project_find_container(json, "guides", '[', ']', &arr_begin,
                                    &arr_end)) {
    it = arr_begin + 1;
    done = false;
    while (!done && editor_project_next_json_string(&it, arr_end, text,
                                                    sizeof(text), &done)) {
      StygianEditorGuideSlot guide;
      uint32_t axis = 0u;
      if (done)
        break;
      if (scratch->guide_count >= scratch->max_guides) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Guide capacity exceeded.");
      }
      memset(&guide, 0, sizeof(guide));
      if (!editor_project_record_u32(text, "id", &guide.id) || guide.id == 0u ||
          !editor_project_record_u32(text, "axis", &axis) ||
          axis > (uint32_t)STYGIAN_EDITOR_GUIDE_HORIZONTAL ||
          !editor_project_record_float(text, "pos", &guide.position)) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Bad guide record.");
      }
      guide.axis = (StygianEditorGuideAxis)axis;
      if (!editor_project_record_u32(text, "pid", &guide.parent_id))
        guide.parent_id = STYGIAN_EDITOR_INVALID_ID;
      scratch->guides[scratch->guide_count++] = guide;
      if (scratch->next_guide_id <= guide.id)
        scratch->next_guide_id = guide.id + 1u;
    }
  }

  if (editor_project_find_container(json, "text_styles", '[', ']', &arr_begin,
                                    &arr_end)) {
    it = arr_begin + 1;
    done = false;
    while (!done && editor_project_next_json_string(&it, arr_end, text,
                                                    sizeof(text), &done)) {
      StygianEditorTextStyleDef def;
      char color_text[128];
      if (done)
        break;
      if (scratch->text_style_count >= STYGIAN_EDITOR_STYLE_CAP) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Text style capacity exceeded.");
      }
      memset(&def, 0, sizeof(def));
      if (!editor_project_record_string(text, "n", def.name, sizeof(def.name)) ||
          !editor_project_record_float(text, "fs", &def.font_size) ||
          !editor_project_record_float(text, "lh", &def.line_height) ||
          !editor_project_record_float(text, "ls", &def.letter_spacing) ||
          !editor_project_record_u32(text, "fw", &def.font_weight) ||
          !editor_project_record_string(text, "c", color_text,
                                        sizeof(color_text)) ||
          !editor_project_parse_color_text(color_text, &def.color)) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Bad text style record.");
      }
      scratch->text_styles[scratch->text_style_count++].def = def;
    }
  }

  if (editor_project_find_container(json, "effect_styles", '[', ']', &arr_begin,
                                    &arr_end)) {
    it = arr_begin + 1;
    done = false;
    while (!done && editor_project_next_json_string(&it, arr_end, text,
                                                    sizeof(text), &done)) {
      StygianEditorEffectStyleDef def;
      char ef_text[512];
      if (done)
        break;
      if (scratch->effect_style_count >= STYGIAN_EDITOR_STYLE_CAP) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Effect style capacity exceeded.");
      }
      memset(&def, 0, sizeof(def));
      if (!editor_project_record_string(text, "n", def.name, sizeof(def.name))) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Bad effect style record.");
      }
      (void)editor_project_record_u32(text, "ec", &def.effect_count);
      if (def.effect_count > STYGIAN_EDITOR_NODE_EFFECT_CAP)
        def.effect_count = STYGIAN_EDITOR_NODE_EFFECT_CAP;
      if (editor_project_record_string(text, "ef", ef_text, sizeof(ef_text))) {
        StygianEditorNode tmp_node;
        memset(&tmp_node, 0, sizeof(tmp_node));
        if (!editor_project_parse_effects_text(ef_text, &tmp_node)) {
          free(load_state.selected_ids);
          stygian_editor_destroy(scratch);
          return editor_project_fail(editor, "Bad effect style effects.");
        }
        def.effect_count = tmp_node.effect_count;
        memcpy(def.effects, tmp_node.effects, sizeof(def.effects));
      }
      scratch->effect_styles[scratch->effect_style_count++].def = def;
    }
  }

  if (editor_project_find_container(json, "layout_styles", '[', ']', &arr_begin,
                                    &arr_end)) {
    it = arr_begin + 1;
    done = false;
    while (!done && editor_project_next_json_string(&it, arr_end, text,
                                                    sizeof(text), &done)) {
      StygianEditorLayoutStyleDef def;
      uint32_t mode = 0u;
      uint32_t pa = 0u;
      uint32_t ca = 0u;
      if (done)
        break;
      if (scratch->layout_style_count >= STYGIAN_EDITOR_STYLE_CAP) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Layout style capacity exceeded.");
      }
      memset(&def, 0, sizeof(def));
      if (!editor_project_record_string(text, "n", def.name, sizeof(def.name)) ||
          !editor_project_record_u32(text, "m", &mode)) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Bad layout style record.");
      }
      def.layout.mode = mode <= (uint32_t)STYGIAN_EDITOR_AUTO_LAYOUT_VERTICAL
                            ? (StygianEditorAutoLayoutMode)mode
                            : STYGIAN_EDITOR_AUTO_LAYOUT_OFF;
      if (editor_project_record_u32(text, "w", &mode) &&
          mode <= (uint32_t)STYGIAN_EDITOR_AUTO_LAYOUT_WRAP) {
        def.layout.wrap = (StygianEditorAutoLayoutWrap)mode;
      } else {
        def.layout.wrap = STYGIAN_EDITOR_AUTO_LAYOUT_NO_WRAP;
      }
      (void)editor_project_record_float(text, "pl", &def.layout.padding_left);
      (void)editor_project_record_float(text, "pr", &def.layout.padding_right);
      (void)editor_project_record_float(text, "pt", &def.layout.padding_top);
      (void)editor_project_record_float(text, "pb", &def.layout.padding_bottom);
      (void)editor_project_record_float(text, "g", &def.layout.gap);
      if (editor_project_record_u32(text, "pa", &pa) &&
          pa <= (uint32_t)STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_STRETCH) {
        def.layout.primary_align = (StygianEditorAutoLayoutAlign)pa;
      }
      if (editor_project_record_u32(text, "ca", &ca) &&
          ca <= (uint32_t)STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_STRETCH) {
        def.layout.cross_align = (StygianEditorAutoLayoutAlign)ca;
      }
      scratch->layout_styles[scratch->layout_style_count++].def = def;
    }
  }

  if (editor_project_find_container(json, "variable_modes", '[', ']', &arr_begin,
                                    &arr_end)) {
    it = arr_begin + 1;
    done = false;
    scratch->variable_mode_count = 0u;
    memset(scratch->variable_modes, 0, sizeof(scratch->variable_modes));
    while (!done && editor_project_next_json_string(&it, arr_end, text,
                                                    sizeof(text), &done)) {
      if (done)
        break;
      if (scratch->variable_mode_count >= STYGIAN_EDITOR_VARIABLE_MODE_CAP)
        break;
      (void)editor_copy_short_name(
          scratch->variable_modes[scratch->variable_mode_count],
          sizeof(scratch->variable_modes[scratch->variable_mode_count]), text);
      scratch->variable_mode_count += 1u;
    }
    if (scratch->variable_mode_count == 0u) {
      scratch->variable_mode_count = 1u;
      snprintf(scratch->variable_modes[0], sizeof(scratch->variable_modes[0]),
               "default");
    }
  }
  (void)editor_project_find_number(json, "active_variable_mode",
                                   &scratch->active_variable_mode);
  if (scratch->active_variable_mode >= scratch->variable_mode_count)
    scratch->active_variable_mode = 0u;

  if (editor_project_find_container(json, "variables", '[', ']', &arr_begin,
                                    &arr_end)) {
    it = arr_begin + 1;
    done = false;
    while (!done && editor_project_next_json_string(&it, arr_end, text,
                                                    sizeof(text), &done)) {
      StygianEditorVariableDef def;
      char vals[512];
      uint32_t kind = 0u;
      uint32_t mode_i = 0u;
      const char *vals_at;
      if (done)
        break;
      if (scratch->variable_count >= STYGIAN_EDITOR_VARIABLE_CAP) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Variable capacity exceeded.");
      }
      memset(&def, 0, sizeof(def));
      if (!editor_project_record_string(text, "n", def.name, sizeof(def.name)) ||
          !editor_project_record_u32(text, "k", &kind) ||
          !editor_project_record_string(text, "vals", vals, sizeof(vals))) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Bad variable record.");
      }
      def.kind = kind <= (uint32_t)STYGIAN_EDITOR_VARIABLE_NUMBER
                     ? (StygianEditorVariableKind)kind
                     : STYGIAN_EDITOR_VARIABLE_NUMBER;
      vals_at = vals;
      while (vals_at && *vals_at && mode_i < scratch->variable_mode_count &&
             mode_i < STYGIAN_EDITOR_VARIABLE_MODE_CAP) {
        const char *sep = strchr(vals_at, '|');
        size_t span = sep ? (size_t)(sep - vals_at) : strlen(vals_at);
        if (def.kind == STYGIAN_EDITOR_VARIABLE_COLOR) {
          char one[96];
          if (span >= sizeof(one))
            span = sizeof(one) - 1u;
          memcpy(one, vals_at, span);
          one[span] = '\0';
          if (!editor_project_parse_color_text(one, &def.color_values[mode_i])) {
            free(load_state.selected_ids);
            stygian_editor_destroy(scratch);
            return editor_project_fail(editor, "Bad color variable value.");
          }
        } else {
          char one[64];
          char *end = NULL;
          double dv = 0.0;
          if (span >= sizeof(one))
            span = sizeof(one) - 1u;
          memcpy(one, vals_at, span);
          one[span] = '\0';
          dv = strtod(one, &end);
          if (end == one) {
            free(load_state.selected_ids);
            stygian_editor_destroy(scratch);
            return editor_project_fail(editor, "Bad number variable value.");
          }
          def.number_values[mode_i] = (float)dv;
        }
        mode_i += 1u;
        vals_at = sep ? (sep + 1) : NULL;
      }
      scratch->variables[scratch->variable_count++].def = def;
    }
  }

  if (!editor_project_find_container(json, "nodes", '[', ']', &arr_begin, &arr_end)) {
    free(load_state.selected_ids);
    stygian_editor_destroy(scratch);
    return editor_project_fail(editor, "Project is missing nodes.");
  }
  it = arr_begin + 1;
  done = false;
  while (!done && editor_project_next_json_string(&it, arr_end, text, sizeof(text),
                                                  &done)) {
    if (done)
      break;
    if (!editor_project_parse_node_record(scratch, text)) {
      free(load_state.selected_ids);
      stygian_editor_destroy(scratch);
      return editor_project_fail(editor, "Bad node record.");
    }
  }

  if (editor_project_find_container(json, "behaviors", '[', ']', &arr_begin,
                                    &arr_end)) {
    it = arr_begin + 1;
    done = false;
    while (!done && editor_project_next_json_string(&it, arr_end, text,
                                                    sizeof(text), &done)) {
      if (done)
        break;
      if (!editor_project_parse_behavior_record(scratch, text)) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Bad behavior record.");
      }
    }
  }

  if (editor_project_find_container(json, "timeline_tracks", '[', ']',
                                    &arr_begin, &arr_end)) {
    it = arr_begin + 1;
    done = false;
    while (!done && editor_project_next_json_string(&it, arr_end, text,
                                                    sizeof(text), &done)) {
      if (done)
        break;
      if (!editor_project_parse_timeline_track_record(scratch, text)) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Bad timeline track record.");
      }
    }
  }

  if (editor_project_find_container(json, "timeline_clips", '[', ']',
                                    &arr_begin, &arr_end)) {
    it = arr_begin + 1;
    done = false;
    while (!done && editor_project_next_json_string(&it, arr_end, text,
                                                    sizeof(text), &done)) {
      if (done)
        break;
      if (!editor_project_parse_timeline_clip_record(scratch, text)) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Bad timeline clip record.");
      }
    }
  }

  if (editor_project_find_container(json, "drivers", '[', ']', &arr_begin,
                                    &arr_end)) {
    it = arr_begin + 1;
    done = false;
    while (!done && editor_project_next_json_string(&it, arr_end, text,
                                                    sizeof(text), &done)) {
      if (done)
        break;
      if (!editor_project_parse_driver_record(scratch, text)) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Bad driver record.");
      }
    }
  } else if (schema_minor < 3u) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_INFO,
                "Project schema minor %u missing drivers; using empty set.",
                schema_minor);
  }

  if (editor_project_find_container(json, "transition_clips", '[', ']',
                                    &arr_begin, &arr_end)) {
    it = arr_begin + 1;
    done = false;
    while (!done && editor_project_next_json_string(&it, arr_end, text,
                                                    sizeof(text), &done)) {
      if (done)
        break;
      if (!editor_project_parse_transition_clip_record(text)) {
        free(load_state.selected_ids);
        stygian_editor_destroy(scratch);
        return editor_project_fail(editor, "Bad transition clip record.");
      }
    }
  } else if (schema_minor >= 1u) {
    editor_logf(editor, STYGIAN_EDITOR_LOG_INFO,
                "Project schema minor %u missing transition_clips; using empty set.",
                schema_minor);
  }

  if (!editor_project_validate_links(scratch)) {
    free(load_state.selected_ids);
    stygian_editor_destroy(scratch);
    return editor_project_fail(editor, "Project link validation failed.");
  }

  editor_clear_selection(scratch);
  {
    uint32_t i;
    for (i = 0u; i < load_state.selected_count; ++i) {
      int idx = editor_find_node_index(scratch, load_state.selected_ids[i]);
      if (idx >= 0)
        scratch->nodes[idx].selected = true;
    }
    if (load_state.primary_explicit &&
        editor_find_node_index(scratch, load_state.primary_node_id) >= 0) {
      scratch->selected_node = load_state.primary_node_id;
    } else {
      editor_refresh_primary_selected(scratch);
    }
  }

  if (!load_state.next_node_explicit && scratch->next_node_id == 0u)
    scratch->next_node_id = 1u;
  if (!load_state.next_path_explicit && scratch->next_path_id == 0u)
    scratch->next_path_id = 1u;
  if (!load_state.next_behavior_explicit && scratch->next_behavior_id == 0u)
    scratch->next_behavior_id = 1u;
  if (!load_state.next_timeline_track_explicit &&
      scratch->next_timeline_track_id == 0u) {
    scratch->next_timeline_track_id = 1u;
  }
  if (!load_state.next_timeline_clip_explicit &&
      scratch->next_timeline_clip_id == 0u) {
    scratch->next_timeline_clip_id = 1u;
  }
  if (!load_state.next_driver_explicit && scratch->next_driver_id == 0u)
    scratch->next_driver_id = 1u;
  if (!load_state.next_guide_explicit && scratch->next_guide_id == 0u)
    scratch->next_guide_id = 1u;

  editor_project_copy_state(editor, scratch);
  if (external_load) {
    editor_history_clear(editor);
    editor_history_drop_transaction(editor);
  }
  editor_request_repaint(editor, 120u);

  free(load_state.selected_ids);
  stygian_editor_destroy(scratch);
  return true;
}

bool stygian_editor_save_project_file(const StygianEditor *editor,
                                      const char *path) {
  FILE *f = NULL;
  char *json = NULL;
  size_t required;
  bool ok = false;

  if (!editor || !path || !path[0])
    return false;

  required = stygian_editor_build_project_json(editor, NULL, 0u);
  if (required == 0u)
    return false;
  json = (char *)malloc(required);
  if (!json)
    return false;
  stygian_editor_build_project_json(editor, json, required);

  f = fopen(path, "wb");
  if (!f) {
    free(json);
    return false;
  }
  ok = fwrite(json, 1u, required - 1u, f) == required - 1u;
  fclose(f);
  free(json);
  return ok;
}

bool stygian_editor_load_project_file(StygianEditor *editor, const char *path) {
  FILE *f = NULL;
  long size = 0;
  char *json = NULL;
  bool ok = false;
  if (!editor || !path || !path[0])
    return false;
  f = fopen(path, "rb");
  if (!f)
    return false;
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return false;
  }
  size = ftell(f);
  if (size < 0) {
    fclose(f);
    return false;
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return false;
  }
  json = (char *)malloc((size_t)size + 1u);
  if (!json) {
    fclose(f);
    return false;
  }
  if (fread(json, 1u, (size_t)size, f) != (size_t)size) {
    fclose(f);
    free(json);
    return false;
  }
  fclose(f);
  json[size] = '\0';
  ok = stygian_editor_load_project_json(editor, json);
  free(json);
  return ok;
}
