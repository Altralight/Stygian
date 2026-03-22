#ifndef STYGIAN_EDITOR_H
#define STYGIAN_EDITOR_H

#include "stygian.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STYGIAN_EDITOR_INVALID_ID 0u
#define STYGIAN_EDITOR_PROJECT_SCHEMA_NAME "stygian-editor-project"
#define STYGIAN_EDITOR_PROJECT_SCHEMA_MAJOR 0u
#define STYGIAN_EDITOR_PROJECT_SCHEMA_MINOR 5u
#define STYGIAN_EDITOR_NODE_FILL_CAP 8u
#define STYGIAN_EDITOR_NODE_STROKE_CAP 8u
#define STYGIAN_EDITOR_NODE_EFFECT_CAP 8u
#define STYGIAN_EDITOR_NODE_BOOLEAN_OPERAND_CAP 16u
#define STYGIAN_EDITOR_STROKE_DASH_CAP 8u
#define STYGIAN_EDITOR_TEXT_SPAN_CAP 16u
#define STYGIAN_EDITOR_COMPONENT_PROPERTY_CAP 8u
#define STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP 8u
#define STYGIAN_EDITOR_COMPONENT_ENUM_OPTION_CAP 8u
#define STYGIAN_EDITOR_VARIABLE_MODE_CAP 8u
#define STYGIAN_EDITOR_STYLE_CAP 64u
#define STYGIAN_EDITOR_VARIABLE_CAP 128u
#define STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP 64u
#define STYGIAN_EDITOR_TIMELINE_CLIP_TRACK_CAP 64u

typedef struct StygianEditor StygianEditor;
typedef uint32_t StygianEditorNodeId;
typedef uint32_t StygianEditorPathId;
typedef uint32_t StygianEditorBehaviorId;
typedef uint32_t StygianEditorTimelineTrackId;
typedef uint32_t StygianEditorTimelineClipId;
typedef uint32_t StygianEditorDriverId;

typedef enum StygianEditorLogLevel {
  STYGIAN_EDITOR_LOG_DEBUG = 0,
  STYGIAN_EDITOR_LOG_INFO = 1,
  STYGIAN_EDITOR_LOG_WARN = 2,
  STYGIAN_EDITOR_LOG_ERROR = 3,
} StygianEditorLogLevel;

typedef enum StygianEditorShapeKind {
  STYGIAN_EDITOR_SHAPE_RECT = 0,
  STYGIAN_EDITOR_SHAPE_ELLIPSE = 1,
  STYGIAN_EDITOR_SHAPE_PATH = 2,
  STYGIAN_EDITOR_SHAPE_FRAME = 3,
  STYGIAN_EDITOR_SHAPE_TEXT = 4,
  STYGIAN_EDITOR_SHAPE_IMAGE = 5,
  STYGIAN_EDITOR_SHAPE_LINE = 6,
  STYGIAN_EDITOR_SHAPE_ARROW = 7,
  STYGIAN_EDITOR_SHAPE_POLYGON = 8,
  STYGIAN_EDITOR_SHAPE_STAR = 9,
  STYGIAN_EDITOR_SHAPE_ARC = 10,
  STYGIAN_EDITOR_SHAPE_GROUP = 11,
  STYGIAN_EDITOR_SHAPE_COMPONENT_DEF = 12,
  STYGIAN_EDITOR_SHAPE_COMPONENT_INSTANCE = 13,
} StygianEditorShapeKind;

typedef enum StygianEditorEventKind {
  STYGIAN_EDITOR_EVENT_PRESS = 0,
  STYGIAN_EDITOR_EVENT_RELEASE = 1,
  STYGIAN_EDITOR_EVENT_HOVER_ENTER = 2,
  STYGIAN_EDITOR_EVENT_HOVER_LEAVE = 3,
  STYGIAN_EDITOR_EVENT_DRAG_START = 4,
  STYGIAN_EDITOR_EVENT_DRAG_MOVE = 5,
  STYGIAN_EDITOR_EVENT_DRAG_END = 6,
  STYGIAN_EDITOR_EVENT_SCROLL = 7,
  STYGIAN_EDITOR_EVENT_VALUE_CHANGED = 8,
  STYGIAN_EDITOR_EVENT_FOCUS_ENTER = 9,
  STYGIAN_EDITOR_EVENT_FOCUS_LEAVE = 10,
} StygianEditorEventKind;

typedef enum StygianEditorBehaviorActionKind {
  STYGIAN_EDITOR_ACTION_ANIMATE = 0,
  STYGIAN_EDITOR_ACTION_SET_PROPERTY = 1,
  STYGIAN_EDITOR_ACTION_TOGGLE_VISIBILITY = 2,
  STYGIAN_EDITOR_ACTION_SET_VARIABLE = 3,
  STYGIAN_EDITOR_ACTION_NAVIGATE = 4,
} StygianEditorBehaviorActionKind;

typedef enum StygianEditorPropertyKind {
  STYGIAN_EDITOR_PROP_X = 0,
  STYGIAN_EDITOR_PROP_Y = 1,
  STYGIAN_EDITOR_PROP_WIDTH = 2,
  STYGIAN_EDITOR_PROP_HEIGHT = 3,
  STYGIAN_EDITOR_PROP_OPACITY = 4,
  STYGIAN_EDITOR_PROP_RADIUS_TL = 5,
  STYGIAN_EDITOR_PROP_RADIUS_TR = 6,
  STYGIAN_EDITOR_PROP_RADIUS_BR = 7,
  STYGIAN_EDITOR_PROP_RADIUS_BL = 8,
  STYGIAN_EDITOR_PROP_VALUE = 9,
  STYGIAN_EDITOR_PROP_ROTATION_DEG = 10,
  STYGIAN_EDITOR_PROP_VISIBLE = 11,
  STYGIAN_EDITOR_PROP_FILL_COLOR = 12,
  STYGIAN_EDITOR_PROP_COLOR_TOKEN = 13,
  STYGIAN_EDITOR_PROP_SHAPE_KIND = 14,
} StygianEditorPropertyKind;

typedef enum StygianEditorTransformCommandKind {
  STYGIAN_EDITOR_TRANSFORM_MOVE = 0,
  STYGIAN_EDITOR_TRANSFORM_SCALE = 1,
  STYGIAN_EDITOR_TRANSFORM_ROTATE = 2,
  STYGIAN_EDITOR_TRANSFORM_REPARENT = 3,
} StygianEditorTransformCommandKind;

typedef enum StygianEditorConstraintH {
  STYGIAN_EDITOR_CONSTRAINT_H_LEFT = 0,
  STYGIAN_EDITOR_CONSTRAINT_H_RIGHT = 1,
  STYGIAN_EDITOR_CONSTRAINT_H_LEFT_RIGHT = 2,
  STYGIAN_EDITOR_CONSTRAINT_H_CENTER = 3,
  STYGIAN_EDITOR_CONSTRAINT_H_SCALE = 4,
} StygianEditorConstraintH;

typedef enum StygianEditorConstraintV {
  STYGIAN_EDITOR_CONSTRAINT_V_TOP = 0,
  STYGIAN_EDITOR_CONSTRAINT_V_BOTTOM = 1,
  STYGIAN_EDITOR_CONSTRAINT_V_TOP_BOTTOM = 2,
  STYGIAN_EDITOR_CONSTRAINT_V_CENTER = 3,
  STYGIAN_EDITOR_CONSTRAINT_V_SCALE = 4,
} StygianEditorConstraintV;

typedef enum StygianEditorAutoLayoutMode {
  STYGIAN_EDITOR_AUTO_LAYOUT_OFF = 0,
  STYGIAN_EDITOR_AUTO_LAYOUT_HORIZONTAL = 1,
  STYGIAN_EDITOR_AUTO_LAYOUT_VERTICAL = 2,
} StygianEditorAutoLayoutMode;

typedef enum StygianEditorAutoLayoutAlign {
  STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_START = 0,
  STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_CENTER = 1,
  STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_END = 2,
  STYGIAN_EDITOR_AUTO_LAYOUT_ALIGN_STRETCH = 3,
} StygianEditorAutoLayoutAlign;

typedef enum StygianEditorAutoLayoutSizing {
  STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FIXED = 0,
  STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_HUG = 1,
  STYGIAN_EDITOR_AUTO_LAYOUT_SIZING_FILL = 2,
} StygianEditorAutoLayoutSizing;

typedef enum StygianEditorAutoLayoutWrap {
  STYGIAN_EDITOR_AUTO_LAYOUT_NO_WRAP = 0,
  STYGIAN_EDITOR_AUTO_LAYOUT_WRAP = 1,
} StygianEditorAutoLayoutWrap;

typedef enum StygianEditorFrameOverflowPolicy {
  STYGIAN_EDITOR_FRAME_OVERFLOW_VISIBLE = 0,
  STYGIAN_EDITOR_FRAME_OVERFLOW_CLIP = 1,
  STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_X = 2,
  STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_Y = 3,
  STYGIAN_EDITOR_FRAME_OVERFLOW_SCROLL_BOTH = 4,
} StygianEditorFrameOverflowPolicy;

typedef struct StygianEditorTransformCommand {
  StygianEditorTransformCommandKind kind;
  float dx;
  float dy;
  float sx;
  float sy;
  float degrees;
  float pivot_world_x;
  float pivot_world_y;
  StygianEditorNodeId new_parent_id;
  bool keep_world_transform;
} StygianEditorTransformCommand;

typedef enum StygianEditorEasing {
  STYGIAN_EDITOR_EASING_LINEAR = 0,
  STYGIAN_EDITOR_EASING_OUT_CUBIC = 1,
  STYGIAN_EDITOR_EASING_IN_OUT_CUBIC = 2,
} StygianEditorEasing;

typedef enum StygianEditorDiagnosticSeverity {
  STYGIAN_EDITOR_DIAG_INFO = 0,
  STYGIAN_EDITOR_DIAG_WARNING = 1,
  STYGIAN_EDITOR_DIAG_ERROR = 2,
} StygianEditorDiagnosticSeverity;

typedef struct StygianEditorExportDiagnostic {
  StygianEditorDiagnosticSeverity severity;
  StygianEditorNodeId node_id;
  char feature[48];
  char message[160];
} StygianEditorExportDiagnostic;

typedef struct StygianEditorHost {
  void *user_data;
  uint64_t (*now_ms)(void *user_data);
  void (*log)(void *user_data, StygianEditorLogLevel level,
              const char *message);
  void (*request_repaint_hz)(void *user_data, uint32_t hz);
  void (*navigate)(void *user_data, const char *target);
} StygianEditorHost;

typedef struct StygianEditorColor {
  float r;
  float g;
  float b;
  float a;
} StygianEditorColor;

typedef enum StygianEditorFillKind {
  STYGIAN_EDITOR_FILL_SOLID = 0,
  STYGIAN_EDITOR_FILL_LINEAR_GRADIENT = 1,
  STYGIAN_EDITOR_FILL_RADIAL_GRADIENT = 2,
  STYGIAN_EDITOR_FILL_IMAGE = 3,
} StygianEditorFillKind;

typedef struct StygianEditorGradientStop {
  float position;
  StygianEditorColor color;
} StygianEditorGradientStop;

typedef struct StygianEditorGradientTransform {
  float origin_x;
  float origin_y;
  float scale_x;
  float scale_y;
  float rotation_deg;
} StygianEditorGradientTransform;

typedef struct StygianEditorNodeFill {
  StygianEditorFillKind kind;
  bool visible;
  float opacity;
  StygianEditorColor solid;
  StygianEditorGradientStop stops[2];
  float gradient_angle_deg;
  float radial_cx;
  float radial_cy;
  float radial_r;
  char image_asset[64];
} StygianEditorNodeFill;

typedef enum StygianEditorStrokeCap {
  STYGIAN_EDITOR_STROKE_CAP_BUTT = 0,
  STYGIAN_EDITOR_STROKE_CAP_ROUND = 1,
  STYGIAN_EDITOR_STROKE_CAP_SQUARE = 2,
} StygianEditorStrokeCap;

typedef enum StygianEditorStrokeJoin {
  STYGIAN_EDITOR_STROKE_JOIN_MITER = 0,
  STYGIAN_EDITOR_STROKE_JOIN_ROUND = 1,
  STYGIAN_EDITOR_STROKE_JOIN_BEVEL = 2,
} StygianEditorStrokeJoin;

typedef enum StygianEditorStrokeAlign {
  STYGIAN_EDITOR_STROKE_ALIGN_CENTER = 0,
  STYGIAN_EDITOR_STROKE_ALIGN_INSIDE = 1,
  STYGIAN_EDITOR_STROKE_ALIGN_OUTSIDE = 2,
} StygianEditorStrokeAlign;

typedef struct StygianEditorNodeStroke {
  bool visible;
  float opacity;
  float thickness;
  StygianEditorColor color;
  StygianEditorStrokeCap cap;
  StygianEditorStrokeJoin join;
  StygianEditorStrokeAlign align;
  float miter_limit;
  uint32_t dash_count;
  float dash_pattern[STYGIAN_EDITOR_STROKE_DASH_CAP];
} StygianEditorNodeStroke;

typedef enum StygianEditorEffectKind {
  STYGIAN_EDITOR_EFFECT_DROP_SHADOW = 0,
  STYGIAN_EDITOR_EFFECT_INNER_SHADOW = 1,
  STYGIAN_EDITOR_EFFECT_LAYER_BLUR = 2,
  STYGIAN_EDITOR_EFFECT_GLOW = 3,
  STYGIAN_EDITOR_EFFECT_NOISE = 4,
} StygianEditorEffectKind;

typedef struct StygianEditorNodeEffect {
  StygianEditorEffectKind kind;
  bool visible;
  float opacity;
  float radius;
  float spread;
  float offset_x;
  float offset_y;
  float intensity;
  StygianEditorColor color;
} StygianEditorNodeEffect;

typedef struct StygianEditorEffectTransform {
  float scale_x;
  float scale_y;
  float rotation_deg;
} StygianEditorEffectTransform;

typedef struct StygianEditorShaderAttachment {
  bool enabled;
  char slot_name[32];
  char asset_path[128];
  char entry_point[64];
} StygianEditorShaderAttachment;

typedef enum StygianEditorStyleKind {
  STYGIAN_EDITOR_STYLE_TEXT = 0,
  STYGIAN_EDITOR_STYLE_EFFECT = 1,
  STYGIAN_EDITOR_STYLE_LAYOUT = 2,
} StygianEditorStyleKind;

typedef enum StygianEditorVariableKind {
  STYGIAN_EDITOR_VARIABLE_COLOR = 0,
  STYGIAN_EDITOR_VARIABLE_NUMBER = 1,
} StygianEditorVariableKind;

typedef enum StygianEditorComponentPropertyType {
  STYGIAN_EDITOR_COMPONENT_PROPERTY_BOOL = 0,
  STYGIAN_EDITOR_COMPONENT_PROPERTY_ENUM = 1,
} StygianEditorComponentPropertyType;

typedef enum StygianEditorComponentOverrideBits {
  STYGIAN_EDITOR_COMPONENT_OVERRIDE_TEXT = 1u << 0,
  STYGIAN_EDITOR_COMPONENT_OVERRIDE_VISIBLE = 1u << 1,
  STYGIAN_EDITOR_COMPONENT_OVERRIDE_SWAP = 1u << 2,
  STYGIAN_EDITOR_COMPONENT_OVERRIDE_X = 1u << 3,
  STYGIAN_EDITOR_COMPONENT_OVERRIDE_Y = 1u << 4,
  STYGIAN_EDITOR_COMPONENT_OVERRIDE_W = 1u << 5,
  STYGIAN_EDITOR_COMPONENT_OVERRIDE_H = 1u << 6,
  STYGIAN_EDITOR_COMPONENT_OVERRIDE_STYLE_BINDING = 1u << 7,
} StygianEditorComponentOverrideBits;

typedef enum StygianEditorMaskMode {
  STYGIAN_EDITOR_MASK_ALPHA = 0,
  STYGIAN_EDITOR_MASK_LUMINANCE = 1,
} StygianEditorMaskMode;

typedef enum StygianEditorPathPointKind {
  STYGIAN_EDITOR_PATH_POINT_CORNER = 0,
  STYGIAN_EDITOR_PATH_POINT_SMOOTH = 1,
  STYGIAN_EDITOR_PATH_POINT_MIRRORED = 2,
  STYGIAN_EDITOR_PATH_POINT_ASYMMETRIC = 3,
} StygianEditorPathPointKind;

typedef enum StygianEditorBooleanOp {
  STYGIAN_EDITOR_BOOLEAN_UNION = 0,
  STYGIAN_EDITOR_BOOLEAN_SUBTRACT = 1,
  STYGIAN_EDITOR_BOOLEAN_INTERSECT = 2,
  STYGIAN_EDITOR_BOOLEAN_EXCLUDE = 3,
} StygianEditorBooleanOp;

typedef enum StygianEditorTextBoxMode {
  STYGIAN_EDITOR_TEXT_BOX_POINT = 0,
  STYGIAN_EDITOR_TEXT_BOX_AREA = 1,
} StygianEditorTextBoxMode;

typedef enum StygianEditorTextHAlign {
  STYGIAN_EDITOR_TEXT_ALIGN_LEFT = 0,
  STYGIAN_EDITOR_TEXT_ALIGN_CENTER = 1,
  STYGIAN_EDITOR_TEXT_ALIGN_RIGHT = 2,
} StygianEditorTextHAlign;

typedef enum StygianEditorTextVAlign {
  STYGIAN_EDITOR_TEXT_ALIGN_TOP = 0,
  STYGIAN_EDITOR_TEXT_ALIGN_MIDDLE = 1,
  STYGIAN_EDITOR_TEXT_ALIGN_BOTTOM = 2,
} StygianEditorTextVAlign;

typedef enum StygianEditorTextAutoSize {
  STYGIAN_EDITOR_TEXT_AUTOSIZE_NONE = 0,
  STYGIAN_EDITOR_TEXT_AUTOSIZE_WIDTH = 1,
  STYGIAN_EDITOR_TEXT_AUTOSIZE_HEIGHT = 2,
  STYGIAN_EDITOR_TEXT_AUTOSIZE_BOTH = 3,
} StygianEditorTextAutoSize;

typedef struct StygianEditorTextStyleSpan {
  uint32_t start;
  uint32_t length;
  float font_size;
  float line_height;
  float letter_spacing;
  uint32_t weight;
  StygianEditorColor color;
} StygianEditorTextStyleSpan;

typedef enum StygianEditorPropertyValueType {
  STYGIAN_EDITOR_VALUE_FLOAT = 0,
  STYGIAN_EDITOR_VALUE_BOOL = 1,
  STYGIAN_EDITOR_VALUE_COLOR = 2,
  STYGIAN_EDITOR_VALUE_STRING = 3,
  STYGIAN_EDITOR_VALUE_ENUM = 4,
} StygianEditorPropertyValueType;

typedef struct StygianEditorPropertyValue {
  StygianEditorPropertyValueType type;
  union {
    float number;
    bool boolean;
    StygianEditorColor color;
    uint32_t enum_value;
    char string[64];
  } as;
} StygianEditorPropertyValue;

typedef struct StygianEditorPropertyDescriptor {
  StygianEditorPropertyKind kind;
  StygianEditorPropertyValueType value_type;
  bool writable;
  bool animatable;
  bool supports_mixed;
  bool has_numeric_range;
  float min_value;
  float max_value;
} StygianEditorPropertyDescriptor;

typedef struct StygianEditorGridConfig {
  bool enabled;
  bool sub_snap_enabled;
  float major_step_px;
  uint32_t sub_divisions;
  float min_minor_px;
  float snap_tolerance_px;
} StygianEditorGridConfig;

typedef struct StygianEditorViewport2D {
  float width;
  float height;
  float pan_x;
  float pan_y;
  float zoom;
} StygianEditorViewport2D;

typedef struct StygianEditorGridLevel {
  float step_world;
  float step_screen;
  float alpha;
  bool major;
} StygianEditorGridLevel;

typedef enum StygianEditorRulerUnit {
  STYGIAN_EDITOR_RULER_UNIT_PX = 0,
} StygianEditorRulerUnit;

typedef enum StygianEditorGuideAxis {
  STYGIAN_EDITOR_GUIDE_VERTICAL = 0,
  STYGIAN_EDITOR_GUIDE_HORIZONTAL = 1,
} StygianEditorGuideAxis;

typedef struct StygianEditorGuide {
  uint32_t id;
  StygianEditorGuideAxis axis;
  float position;
  StygianEditorNodeId parent_id;
} StygianEditorGuide;

typedef struct StygianEditorSnapSources {
  bool use_grid;
  bool use_guides;
  bool use_bounds;
  bool use_parent_edges;
} StygianEditorSnapSources;

typedef struct StygianEditorPoint {
  float x;
  float y;
  bool has_in_tangent;
  float in_x;
  float in_y;
  bool has_out_tangent;
  float out_x;
  float out_y;
  StygianEditorPathPointKind kind;
} StygianEditorPoint;

typedef struct StygianEditorRectDesc {
  float x;
  float y;
  float w;
  float h;
  float radius[4];
  StygianEditorColor fill;
  bool visible;
  float z;
} StygianEditorRectDesc;

typedef struct StygianEditorEllipseDesc {
  float x;
  float y;
  float w;
  float h;
  StygianEditorColor fill;
  bool visible;
  float z;
} StygianEditorEllipseDesc;

typedef struct StygianEditorFrameDesc {
  float x;
  float y;
  float w;
  float h;
  bool clip_content;
  StygianEditorColor fill;
  bool visible;
  float z;
} StygianEditorFrameDesc;

typedef struct StygianEditorLineDesc {
  float x1;
  float y1;
  float x2;
  float y2;
  float thickness;
  StygianEditorColor stroke;
  bool visible;
  float z;
} StygianEditorLineDesc;

typedef struct StygianEditorArrowDesc {
  float x1;
  float y1;
  float x2;
  float y2;
  float thickness;
  float head_size;
  StygianEditorColor stroke;
  bool visible;
  float z;
} StygianEditorArrowDesc;

typedef struct StygianEditorPolygonDesc {
  float x;
  float y;
  float w;
  float h;
  uint32_t sides;
  float corner_radius;
  StygianEditorColor fill;
  bool visible;
  float z;
} StygianEditorPolygonDesc;

typedef struct StygianEditorStarDesc {
  float x;
  float y;
  float w;
  float h;
  uint32_t points;
  float inner_ratio;
  StygianEditorColor fill;
  bool visible;
  float z;
} StygianEditorStarDesc;

typedef struct StygianEditorArcDesc {
  float x;
  float y;
  float w;
  float h;
  float start_angle;
  float sweep_angle;
  float thickness;
  StygianEditorColor stroke;
  bool visible;
  float z;
} StygianEditorArcDesc;

typedef struct StygianEditorFrameAutoLayout {
  StygianEditorAutoLayoutMode mode;
  StygianEditorAutoLayoutWrap wrap;
  float padding_left;
  float padding_right;
  float padding_top;
  float padding_bottom;
  float gap;
  StygianEditorAutoLayoutAlign primary_align;
  StygianEditorAutoLayoutAlign cross_align;
} StygianEditorFrameAutoLayout;

typedef struct StygianEditorTextStyleDef {
  char name[32];
  float font_size;
  float line_height;
  float letter_spacing;
  uint32_t font_weight;
  StygianEditorColor color;
} StygianEditorTextStyleDef;

typedef struct StygianEditorEffectStyleDef {
  char name[32];
  uint32_t effect_count;
  StygianEditorNodeEffect effects[STYGIAN_EDITOR_NODE_EFFECT_CAP];
} StygianEditorEffectStyleDef;

typedef struct StygianEditorLayoutStyleDef {
  char name[32];
  StygianEditorFrameAutoLayout layout;
} StygianEditorLayoutStyleDef;

typedef struct StygianEditorVariableDef {
  char name[32];
  StygianEditorVariableKind kind;
  float number_values[STYGIAN_EDITOR_VARIABLE_MODE_CAP];
  StygianEditorColor color_values[STYGIAN_EDITOR_VARIABLE_MODE_CAP];
} StygianEditorVariableDef;

typedef struct StygianEditorComponentPropertyDef {
  char name[32];
  StygianEditorComponentPropertyType type;
  bool default_bool;
  char default_enum[32];
  uint32_t enum_option_count;
  char enum_options[STYGIAN_EDITOR_COMPONENT_ENUM_OPTION_CAP][32];
} StygianEditorComponentPropertyDef;

typedef struct StygianEditorComponentPropertyValue {
  char name[32];
  StygianEditorComponentPropertyType type;
  bool bool_value;
  char enum_value[32];
} StygianEditorComponentPropertyValue;

typedef struct StygianEditorComponentOverrideState {
  uint32_t mask;
  char text[96];
  bool visible;
  StygianEditorNodeId swap_component_def_id;
  float x;
  float y;
  float w;
  float h;
  char style_binding[32];
  uint32_t property_override_count;
  StygianEditorComponentPropertyValue
      property_overrides[STYGIAN_EDITOR_COMPONENT_PROPERTY_VALUE_CAP];
} StygianEditorComponentOverrideState;

typedef struct StygianEditorNodeLayoutOptions {
  bool absolute_position;
  StygianEditorAutoLayoutSizing sizing_h;
  StygianEditorAutoLayoutSizing sizing_v;
} StygianEditorNodeLayoutOptions;

typedef struct StygianEditorPathDesc {
  StygianEditorColor stroke;
  float thickness;
  bool closed;
  bool visible;
  float z;
} StygianEditorPathDesc;

typedef struct StygianEditorTextDesc {
  float x;
  float y;
  float w;
  float h;
  float font_size;
  float line_height;
  float letter_spacing;
  uint32_t font_weight;
  StygianEditorTextBoxMode box_mode;
  StygianEditorTextHAlign align_h;
  StygianEditorTextVAlign align_v;
  StygianEditorTextAutoSize auto_size;
  StygianEditorColor fill;
  const char *text;
  bool visible;
  float z;
} StygianEditorTextDesc;

typedef struct StygianEditorImageDesc {
  float x;
  float y;
  float w;
  float h;
  uint32_t fit_mode;
  const char *source;
  bool visible;
  float z;
} StygianEditorImageDesc;

typedef struct StygianEditorComponentDefDesc {
  float x;
  float y;
  float w;
  float h;
  const char *symbol;
  bool visible;
  float z;
} StygianEditorComponentDefDesc;

typedef struct StygianEditorComponentInstanceDesc {
  StygianEditorNodeId component_def_id;
  float x;
  float y;
  float w;
  float h;
  bool visible;
  float z;
} StygianEditorComponentInstanceDesc;

typedef struct StygianEditorAnimateAction {
  StygianEditorNodeId target_node;
  StygianEditorPropertyKind property;
  float from_value;
  float to_value;
  uint32_t duration_ms;
  StygianEditorEasing easing;
} StygianEditorAnimateAction;

typedef struct StygianEditorSetPropertyAction {
  StygianEditorNodeId target_node;
  StygianEditorPropertyKind property;
  float value;
} StygianEditorSetPropertyAction;

typedef struct StygianEditorToggleVisibilityAction {
  StygianEditorNodeId target_node;
} StygianEditorToggleVisibilityAction;

typedef struct StygianEditorSetVariableAction {
  char variable_name[32];
  StygianEditorVariableKind variable_kind;
  bool use_active_mode;
  uint32_t mode_index;
  float number_value;
  StygianEditorColor color_value;
} StygianEditorSetVariableAction;

typedef struct StygianEditorNavigateAction {
  char target[64];
} StygianEditorNavigateAction;

typedef struct StygianEditorBehaviorRule {
  StygianEditorNodeId trigger_node;
  StygianEditorEventKind trigger_event;
  StygianEditorBehaviorActionKind action_kind;
  union {
    StygianEditorAnimateAction animate;
    StygianEditorSetPropertyAction set_property;
    StygianEditorToggleVisibilityAction toggle_visibility;
    StygianEditorSetVariableAction set_variable;
    StygianEditorNavigateAction navigate;
  };
} StygianEditorBehaviorRule;

typedef struct StygianEditorTimelineKeyframe {
  uint32_t time_ms;
  float value;
  StygianEditorEasing easing;
} StygianEditorTimelineKeyframe;

typedef struct StygianEditorTimelineTrack {
  StygianEditorTimelineTrackId id;
  StygianEditorNodeId target_node;
  StygianEditorPropertyKind property;
  uint32_t layer;
  char name[32];
  uint32_t keyframe_count;
  StygianEditorTimelineKeyframe
      keyframes[STYGIAN_EDITOR_TIMELINE_KEYFRAME_CAP];
} StygianEditorTimelineTrack;

typedef struct StygianEditorTimelineClip {
  StygianEditorTimelineClipId id;
  char name[32];
  uint32_t start_ms;
  uint32_t duration_ms;
  uint32_t layer;
  uint32_t track_count;
  StygianEditorTimelineTrackId
      track_ids[STYGIAN_EDITOR_TIMELINE_CLIP_TRACK_CAP];
} StygianEditorTimelineClip;

typedef struct StygianEditorDriverRule {
  StygianEditorNodeId source_node;
  StygianEditorPropertyKind source_property;
  char variable_name[32];
  bool use_active_mode;
  uint32_t mode_index;
  float in_min;
  float in_max;
  float out_min;
  float out_max;
  bool clamp_output;
} StygianEditorDriverRule;

typedef enum StygianEditorControlRecipeKind {
  STYGIAN_EDITOR_RECIPE_BUTTON = 0,
  STYGIAN_EDITOR_RECIPE_SLIDER = 1,
  STYGIAN_EDITOR_RECIPE_TOGGLE = 2,
  STYGIAN_EDITOR_RECIPE_SCROLL_REGION = 3,
  STYGIAN_EDITOR_RECIPE_TABS = 4,
  STYGIAN_EDITOR_RECIPE_ACCORDION = 5,
  STYGIAN_EDITOR_RECIPE_CHECKBOX = 6,
  STYGIAN_EDITOR_RECIPE_RADIO_GROUP = 7,
  STYGIAN_EDITOR_RECIPE_INPUT_FIELD = 8,
  STYGIAN_EDITOR_RECIPE_DROPDOWN = 9,
  STYGIAN_EDITOR_RECIPE_DATA_TABLE = 10,
  STYGIAN_EDITOR_RECIPE_DATA_CARD = 11,
} StygianEditorControlRecipeKind;

typedef struct StygianEditorConfig {
  uint32_t max_nodes;
  uint32_t max_path_points;
  uint32_t max_behaviors;
  uint32_t max_color_tokens;
  uint32_t max_timeline_tracks;
  uint32_t max_timeline_clips;
  uint32_t max_drivers;
  StygianEditorGridConfig grid;
} StygianEditorConfig;

StygianEditorConfig stygian_editor_config_default(void);
StygianEditorColor stygian_editor_color_rgba(float r, float g, float b,
                                             float a);

StygianEditor *stygian_editor_create(const StygianEditorConfig *config,
                                     const StygianEditorHost *host);
void stygian_editor_destroy(StygianEditor *editor);
void stygian_editor_reset(StygianEditor *editor);

void stygian_editor_set_host(StygianEditor *editor,
                             const StygianEditorHost *host);

void stygian_editor_set_viewport(StygianEditor *editor,
                                 const StygianEditorViewport2D *viewport);
StygianEditorViewport2D stygian_editor_get_viewport(const StygianEditor *editor);

void stygian_editor_set_grid_config(StygianEditor *editor,
                                    const StygianEditorGridConfig *grid);
StygianEditorGridConfig
stygian_editor_get_grid_config(const StygianEditor *editor);
uint32_t stygian_editor_get_grid_levels(const StygianEditor *editor,
                                        StygianEditorGridLevel *out_levels,
                                        uint32_t max_levels);

void stygian_editor_world_to_view(const StygianEditor *editor, float world_x,
                                  float world_y, float *out_view_x,
                                  float *out_view_y);
void stygian_editor_view_to_world(const StygianEditor *editor, float view_x,
                                  float view_y, float *out_world_x,
                                  float *out_world_y);

float stygian_editor_snap_world_scalar(const StygianEditor *editor,
                                       float world_value);
void stygian_editor_snap_world_point(const StygianEditor *editor,
                                     float world_x, float world_y,
                                     float *out_world_x, float *out_world_y);
void stygian_editor_set_ruler_unit(StygianEditor *editor,
                                   StygianEditorRulerUnit unit);
StygianEditorRulerUnit
stygian_editor_get_ruler_unit(const StygianEditor *editor);
void stygian_editor_set_snap_sources(StygianEditor *editor,
                                     const StygianEditorSnapSources *sources);
StygianEditorSnapSources
stygian_editor_get_snap_sources(const StygianEditor *editor);
uint32_t stygian_editor_add_guide(StygianEditor *editor,
                                  const StygianEditorGuide *guide);
bool stygian_editor_remove_guide(StygianEditor *editor, uint32_t guide_id);
uint32_t stygian_editor_guide_count(const StygianEditor *editor);
bool stygian_editor_get_guide(const StygianEditor *editor, uint32_t index,
                              StygianEditorGuide *out_guide);

StygianEditorNodeId stygian_editor_add_rect(StygianEditor *editor,
                                            const StygianEditorRectDesc *desc);
StygianEditorNodeId
stygian_editor_add_ellipse(StygianEditor *editor,
                           const StygianEditorEllipseDesc *desc);
StygianEditorNodeId stygian_editor_add_frame(StygianEditor *editor,
                                             const StygianEditorFrameDesc *desc);
StygianEditorNodeId stygian_editor_add_line(StygianEditor *editor,
                                            const StygianEditorLineDesc *desc);
StygianEditorNodeId stygian_editor_add_arrow(
    StygianEditor *editor, const StygianEditorArrowDesc *desc);
StygianEditorNodeId stygian_editor_add_polygon(
    StygianEditor *editor, const StygianEditorPolygonDesc *desc);
StygianEditorNodeId
stygian_editor_add_star(StygianEditor *editor,
                        const StygianEditorStarDesc *desc);
StygianEditorNodeId stygian_editor_add_arc(StygianEditor *editor,
                                           const StygianEditorArcDesc *desc);
StygianEditorNodeId stygian_editor_add_text(StygianEditor *editor,
                                            const StygianEditorTextDesc *desc);
StygianEditorNodeId stygian_editor_add_image(StygianEditor *editor,
                                             const StygianEditorImageDesc *desc);
StygianEditorNodeId stygian_editor_add_component_def(
    StygianEditor *editor, const StygianEditorComponentDefDesc *desc);
StygianEditorNodeId stygian_editor_add_component_instance(
    StygianEditor *editor, const StygianEditorComponentInstanceDesc *desc);
bool stygian_editor_component_def_get_symbol(
    const StygianEditor *editor, StygianEditorNodeId component_def_id,
    char *out_symbol, size_t out_symbol_capacity);
bool stygian_editor_component_def_set_symbol(
    StygianEditor *editor, StygianEditorNodeId component_def_id,
    const char *symbol);
bool stygian_editor_component_instance_get_def(
    const StygianEditor *editor, StygianEditorNodeId instance_id,
    StygianEditorNodeId *out_component_def_id);
bool stygian_editor_component_instance_set_def(
    StygianEditor *editor, StygianEditorNodeId instance_id,
    StygianEditorNodeId component_def_id);
bool stygian_editor_component_apply_definition(
    StygianEditor *editor, StygianEditorNodeId component_def_id);
bool stygian_editor_component_instance_set_override_state(
    StygianEditor *editor, StygianEditorNodeId instance_id,
    const StygianEditorComponentOverrideState *state);
bool stygian_editor_component_instance_get_override_state(
    const StygianEditor *editor, StygianEditorNodeId instance_id,
    StygianEditorComponentOverrideState *out_state);
bool stygian_editor_component_instance_reset_overrides(
    StygianEditor *editor, StygianEditorNodeId instance_id);
bool stygian_editor_component_instance_detach(
    StygianEditor *editor, StygianEditorNodeId instance_id);
bool stygian_editor_component_instance_is_detached(
    const StygianEditor *editor, StygianEditorNodeId instance_id,
    bool *out_detached);
bool stygian_editor_component_instance_repair(
    StygianEditor *editor, StygianEditorNodeId instance_id,
    StygianEditorNodeId preferred_component_def_id, bool preserve_overrides);
bool stygian_editor_component_def_set_variant(StygianEditor *editor,
                                              StygianEditorNodeId component_def_id,
                                              const char *group_name,
                                              const char *variant_name);
bool stygian_editor_component_def_get_variant(
    const StygianEditor *editor, StygianEditorNodeId component_def_id,
    char *out_group_name, size_t out_group_name_capacity,
    char *out_variant_name, size_t out_variant_name_capacity);
bool stygian_editor_component_def_set_property(
    StygianEditor *editor, StygianEditorNodeId component_def_id,
    const StygianEditorComponentPropertyDef *property_def);
uint32_t stygian_editor_component_def_property_count(
    const StygianEditor *editor, StygianEditorNodeId component_def_id);
bool stygian_editor_component_def_get_property(
    const StygianEditor *editor, StygianEditorNodeId component_def_id,
    uint32_t index, StygianEditorComponentPropertyDef *out_property_def);
bool stygian_editor_component_resolve_variant(
    const StygianEditor *editor, StygianEditorNodeId seed_component_def_id,
    const StygianEditorComponentPropertyValue *properties,
    uint32_t property_count, StygianEditorNodeId *out_component_def_id,
    uint32_t *out_match_score, bool *out_exact_match);
bool stygian_editor_component_instance_set_property_override(
    StygianEditor *editor, StygianEditorNodeId instance_id,
    const StygianEditorComponentPropertyValue *property_override);
bool stygian_editor_component_instance_get_property_override(
    const StygianEditor *editor, StygianEditorNodeId instance_id,
    const char *name, StygianEditorComponentPropertyValue *out_property_override);
bool stygian_editor_component_instance_clear_property_override(
    StygianEditor *editor, StygianEditorNodeId instance_id, const char *name);
bool stygian_editor_component_defs_compatible(
    const StygianEditor *editor, StygianEditorNodeId component_def_a,
    StygianEditorNodeId component_def_b);

StygianEditorPathId stygian_editor_path_begin(StygianEditor *editor,
                                              const StygianEditorPathDesc *desc);
bool stygian_editor_path_add_point(StygianEditor *editor, StygianEditorPathId id,
                                   float x, float y, bool snap_to_grid);
StygianEditorNodeId stygian_editor_path_commit(StygianEditor *editor,
                                               StygianEditorPathId id);
void stygian_editor_path_cancel(StygianEditor *editor, StygianEditorPathId id);

uint32_t stygian_editor_node_count(const StygianEditor *editor);
bool stygian_editor_node_set_position(StygianEditor *editor,
                                      StygianEditorNodeId node_id, float x,
                                      float y, bool snap_to_grid);
bool stygian_editor_node_set_size(StygianEditor *editor,
                                  StygianEditorNodeId node_id, float w,
                                  float h);
bool stygian_editor_node_set_color(StygianEditor *editor,
                                   StygianEditorNodeId node_id,
                                   StygianEditorColor color);
bool stygian_editor_node_get_color(const StygianEditor *editor,
                                   StygianEditorNodeId node_id,
                                   StygianEditorColor *out_color);
uint32_t stygian_editor_node_fill_count(const StygianEditor *editor,
                                        StygianEditorNodeId node_id);
bool stygian_editor_node_get_fill(const StygianEditor *editor,
                                  StygianEditorNodeId node_id, uint32_t index,
                                  StygianEditorNodeFill *out_fill);
bool stygian_editor_node_set_fills(StygianEditor *editor,
                                   StygianEditorNodeId node_id,
                                   const StygianEditorNodeFill *fills,
                                   uint32_t fill_count);
bool stygian_editor_node_set_fill_gradient_transform(
    StygianEditor *editor, StygianEditorNodeId node_id, uint32_t fill_index,
    const StygianEditorGradientTransform *transform);
bool stygian_editor_node_get_fill_gradient_transform(
    const StygianEditor *editor, StygianEditorNodeId node_id,
    uint32_t fill_index, StygianEditorGradientTransform *out_transform);
bool stygian_editor_node_set_text_content(StygianEditor *editor,
                                          StygianEditorNodeId node_id,
                                          const char *text);
bool stygian_editor_node_get_text_content(const StygianEditor *editor,
                                          StygianEditorNodeId node_id,
                                          char *out_text,
                                          size_t out_text_capacity);
bool stygian_editor_node_set_text_layout(StygianEditor *editor,
                                         StygianEditorNodeId node_id,
                                         StygianEditorTextBoxMode box_mode,
                                         StygianEditorTextHAlign align_h,
                                         StygianEditorTextVAlign align_v,
                                         StygianEditorTextAutoSize auto_size);
bool stygian_editor_node_get_text_layout(
    const StygianEditor *editor, StygianEditorNodeId node_id,
    StygianEditorTextBoxMode *out_box_mode, StygianEditorTextHAlign *out_align_h,
    StygianEditorTextVAlign *out_align_v, StygianEditorTextAutoSize *out_auto_size);
bool stygian_editor_node_set_text_typography(StygianEditor *editor,
                                             StygianEditorNodeId node_id,
                                             float font_size, float line_height,
                                             float letter_spacing,
                                             uint32_t font_weight);
bool stygian_editor_node_get_text_typography(
    const StygianEditor *editor, StygianEditorNodeId node_id,
    float *out_font_size, float *out_line_height, float *out_letter_spacing,
    uint32_t *out_font_weight);
uint32_t stygian_editor_node_text_span_count(const StygianEditor *editor,
                                             StygianEditorNodeId node_id);
bool stygian_editor_node_get_text_span(const StygianEditor *editor,
                                       StygianEditorNodeId node_id,
                                       uint32_t index,
                                       StygianEditorTextStyleSpan *out_span);
bool stygian_editor_node_set_text_spans(StygianEditor *editor,
                                        StygianEditorNodeId node_id,
                                        const StygianEditorTextStyleSpan *spans,
                                        uint32_t span_count);
bool stygian_editor_node_measure_text(const StygianEditor *editor,
                                      StygianEditorNodeId node_id,
                                      float *out_width, float *out_height);
bool stygian_editor_node_set_image_source(StygianEditor *editor,
                                          StygianEditorNodeId node_id,
                                          const char *source);
bool stygian_editor_node_get_image_source(const StygianEditor *editor,
                                          StygianEditorNodeId node_id,
                                          char *out_source,
                                          size_t out_source_capacity);
bool stygian_editor_node_set_image_fit_mode(StygianEditor *editor,
                                            StygianEditorNodeId node_id,
                                            uint32_t fit_mode);
bool stygian_editor_node_get_image_fit_mode(const StygianEditor *editor,
                                            StygianEditorNodeId node_id,
                                            uint32_t *out_fit_mode);
uint32_t stygian_editor_node_stroke_count(const StygianEditor *editor,
                                          StygianEditorNodeId node_id);
bool stygian_editor_node_get_stroke(const StygianEditor *editor,
                                    StygianEditorNodeId node_id, uint32_t index,
                                    StygianEditorNodeStroke *out_stroke);
bool stygian_editor_node_set_strokes(StygianEditor *editor,
                                     StygianEditorNodeId node_id,
                                     const StygianEditorNodeStroke *strokes,
                                     uint32_t stroke_count);
uint32_t stygian_editor_node_effect_count(const StygianEditor *editor,
                                          StygianEditorNodeId node_id);
bool stygian_editor_node_get_effect(const StygianEditor *editor,
                                    StygianEditorNodeId node_id, uint32_t index,
                                    StygianEditorNodeEffect *out_effect);
bool stygian_editor_node_set_effects(StygianEditor *editor,
                                     StygianEditorNodeId node_id,
                                     const StygianEditorNodeEffect *effects,
                                     uint32_t effect_count);
bool stygian_editor_node_set_effect_transform(
    StygianEditor *editor, StygianEditorNodeId node_id, uint32_t effect_index,
    const StygianEditorEffectTransform *transform);
bool stygian_editor_node_get_effect_transform(
    const StygianEditor *editor, StygianEditorNodeId node_id,
    uint32_t effect_index, StygianEditorEffectTransform *out_transform);
bool stygian_editor_node_set_mask(StygianEditor *editor,
                                  StygianEditorNodeId node_id,
                                  StygianEditorNodeId mask_node_id,
                                  StygianEditorMaskMode mode, bool invert);
bool stygian_editor_node_get_mask(const StygianEditor *editor,
                                  StygianEditorNodeId node_id,
                                  StygianEditorNodeId *out_mask_node_id,
                                  StygianEditorMaskMode *out_mode,
                                  bool *out_invert);
bool stygian_editor_node_set_shader_attachment(
    StygianEditor *editor, StygianEditorNodeId node_id,
    const StygianEditorShaderAttachment *attachment);
bool stygian_editor_node_get_shader_attachment(
    const StygianEditor *editor, StygianEditorNodeId node_id,
    StygianEditorShaderAttachment *out_attachment);
bool stygian_editor_node_set_opacity(StygianEditor *editor,
                                     StygianEditorNodeId node_id, float opacity);
bool stygian_editor_node_set_rotation(StygianEditor *editor,
                                      StygianEditorNodeId node_id,
                                      float rotation_degrees);
bool stygian_editor_node_get_rotation(const StygianEditor *editor,
                                      StygianEditorNodeId node_id,
                                      float *out_rotation_degrees);
bool stygian_editor_node_get_shape_kind(const StygianEditor *editor,
                                        StygianEditorNodeId node_id,
                                        StygianEditorShapeKind *out_kind);
bool stygian_editor_node_set_corner_radii(StygianEditor *editor,
                                          StygianEditorNodeId node_id,
                                          float top_left, float top_right,
                                          float bottom_right,
                                          float bottom_left);
bool stygian_editor_node_get_corner_radii(const StygianEditor *editor,
                                          StygianEditorNodeId node_id,
                                          float *out_top_left,
                                          float *out_top_right,
                                          float *out_bottom_right,
                                          float *out_bottom_left);
bool stygian_editor_node_get_bounds(const StygianEditor *editor,
                                    StygianEditorNodeId node_id, float *out_x,
                                    float *out_y, float *out_w, float *out_h);
bool stygian_editor_node_get_parent(const StygianEditor *editor,
                                    StygianEditorNodeId node_id,
                                    StygianEditorNodeId *out_parent_id);
bool stygian_editor_reparent_node(StygianEditor *editor,
                                  StygianEditorNodeId node_id,
                                  StygianEditorNodeId new_parent_id,
                                  bool keep_world_transform);
bool stygian_editor_apply_transform(StygianEditor *editor,
                                    const StygianEditorNodeId *node_ids,
                                    uint32_t node_count,
                                    const StygianEditorTransformCommand *command);
bool stygian_editor_delete_node(StygianEditor *editor,
                                StygianEditorNodeId node_id);
bool stygian_editor_property_get_descriptor(
    StygianEditorPropertyKind kind, StygianEditorPropertyDescriptor *out_desc);
bool stygian_editor_node_get_property_value(
    const StygianEditor *editor, StygianEditorNodeId node_id,
    StygianEditorPropertyKind property, StygianEditorPropertyValue *out_value);
bool stygian_editor_node_set_property_value(
    StygianEditor *editor, StygianEditorNodeId node_id,
    StygianEditorPropertyKind property, const StygianEditorPropertyValue *value);
bool stygian_editor_selection_get_property_value(
    const StygianEditor *editor, StygianEditorPropertyKind property,
    StygianEditorPropertyValue *out_value, bool *out_mixed,
    uint32_t *out_supported_count);
bool stygian_editor_selection_set_property_value(
    StygianEditor *editor, StygianEditorPropertyKind property,
    const StygianEditorPropertyValue *value, uint32_t *out_applied_count);
bool stygian_editor_node_set_name(StygianEditor *editor,
                                  StygianEditorNodeId node_id,
                                  const char *name);
bool stygian_editor_node_get_name(const StygianEditor *editor,
                                  StygianEditorNodeId node_id, char *out_name,
                                  size_t out_name_capacity);
bool stygian_editor_node_set_locked(StygianEditor *editor,
                                    StygianEditorNodeId node_id, bool locked);
bool stygian_editor_node_get_locked(const StygianEditor *editor,
                                    StygianEditorNodeId node_id,
                                    bool *out_locked);
bool stygian_editor_node_set_visible(StygianEditor *editor,
                                     StygianEditorNodeId node_id,
                                     bool visible);
bool stygian_editor_node_get_visible(const StygianEditor *editor,
                                     StygianEditorNodeId node_id,
                                     bool *out_visible);
uint32_t stygian_editor_tree_list_children(const StygianEditor *editor,
                                           StygianEditorNodeId parent_id,
                                           StygianEditorNodeId *out_ids,
                                           uint32_t max_ids);
bool stygian_editor_tree_reorder_child(StygianEditor *editor,
                                       StygianEditorNodeId node_id,
                                       uint32_t new_sibling_index);
bool stygian_editor_node_set_constraints(StygianEditor *editor,
                                         StygianEditorNodeId node_id,
                                         StygianEditorConstraintH horizontal,
                                         StygianEditorConstraintV vertical);
bool stygian_editor_node_get_constraints(const StygianEditor *editor,
                                         StygianEditorNodeId node_id,
                                         StygianEditorConstraintH *out_horizontal,
                                         StygianEditorConstraintV *out_vertical);
bool stygian_editor_resize_frame(StygianEditor *editor,
                                 StygianEditorNodeId frame_node_id,
                                 float new_width, float new_height);
bool stygian_editor_frame_set_auto_layout(
    StygianEditor *editor, StygianEditorNodeId frame_node_id,
    const StygianEditorFrameAutoLayout *layout);
bool stygian_editor_frame_get_auto_layout(
    const StygianEditor *editor, StygianEditorNodeId frame_node_id,
    StygianEditorFrameAutoLayout *out_layout);
bool stygian_editor_node_set_layout_options(
    StygianEditor *editor, StygianEditorNodeId node_id,
    const StygianEditorNodeLayoutOptions *options);
bool stygian_editor_node_get_layout_options(
    const StygianEditor *editor, StygianEditorNodeId node_id,
    StygianEditorNodeLayoutOptions *out_options);
bool stygian_editor_frame_recompute_layout(StygianEditor *editor,
                                           StygianEditorNodeId frame_node_id);
bool stygian_editor_node_set_size_limits(StygianEditor *editor,
                                         StygianEditorNodeId node_id,
                                         float min_width, float max_width,
                                         float min_height, float max_height);
bool stygian_editor_node_get_size_limits(const StygianEditor *editor,
                                         StygianEditorNodeId node_id,
                                         float *out_min_width,
                                         float *out_max_width,
                                         float *out_min_height,
                                         float *out_max_height);
bool stygian_editor_frame_set_overflow_policy(
    StygianEditor *editor, StygianEditorNodeId frame_node_id,
    StygianEditorFrameOverflowPolicy policy);
bool stygian_editor_frame_get_overflow_policy(
    const StygianEditor *editor, StygianEditorNodeId frame_node_id,
    StygianEditorFrameOverflowPolicy *out_policy);
bool stygian_editor_frame_set_scroll(StygianEditor *editor,
                                     StygianEditorNodeId frame_node_id,
                                     float scroll_x, float scroll_y);
bool stygian_editor_frame_get_scroll(const StygianEditor *editor,
                                     StygianEditorNodeId frame_node_id,
                                     float *out_scroll_x, float *out_scroll_y);

StygianEditorNodeId stygian_editor_select_at(StygianEditor *editor, float x,
                                             float y, bool additive);
bool stygian_editor_select_node(StygianEditor *editor,
                                StygianEditorNodeId node_id, bool additive);
StygianEditorNodeId stygian_editor_selected_node(const StygianEditor *editor);
uint32_t stygian_editor_selected_count(const StygianEditor *editor);
bool stygian_editor_node_is_selected(const StygianEditor *editor,
                                     StygianEditorNodeId node_id);
uint32_t stygian_editor_selected_nodes(const StygianEditor *editor,
                                       StygianEditorNodeId *out_ids,
                                       uint32_t max_ids);
StygianEditorNodeId stygian_editor_hit_test_at(const StygianEditor *editor,
                                               float x, float y);
uint32_t stygian_editor_select_in_rect(StygianEditor *editor, float x0, float y0,
                                       float x1, float y1, bool additive);
uint32_t stygian_editor_path_point_count(const StygianEditor *editor,
                                         StygianEditorNodeId path_node_id);
bool stygian_editor_path_get_point(const StygianEditor *editor,
                                   StygianEditorNodeId path_node_id,
                                   uint32_t point_index,
                                   StygianEditorPoint *out_point);
bool stygian_editor_path_set_point(StygianEditor *editor,
                                   StygianEditorNodeId path_node_id,
                                   uint32_t point_index,
                                   const StygianEditorPoint *point);
bool stygian_editor_path_insert_point(StygianEditor *editor,
                                      StygianEditorNodeId path_node_id,
                                      uint32_t point_index,
                                      const StygianEditorPoint *point);
bool stygian_editor_path_remove_point(StygianEditor *editor,
                                      StygianEditorNodeId path_node_id,
                                      uint32_t point_index);
bool stygian_editor_path_set_closed(StygianEditor *editor,
                                    StygianEditorNodeId path_node_id,
                                    bool closed);
StygianEditorNodeId stygian_editor_boolean_compose(
    StygianEditor *editor, StygianEditorBooleanOp op,
    const StygianEditorNodeId *operand_ids, uint32_t operand_count,
    bool keep_sources);
bool stygian_editor_boolean_flatten(StygianEditor *editor,
                                    StygianEditorNodeId boolean_node_id,
                                    StygianEditorNodeId *out_path_node_id);
bool stygian_editor_node_get_boolean(const StygianEditor *editor,
                                     StygianEditorNodeId node_id,
                                     StygianEditorBooleanOp *out_op,
                                     StygianEditorNodeId *out_operand_ids,
                                     uint32_t max_operands,
                                     uint32_t *out_operand_count);

bool stygian_editor_set_color_token(StygianEditor *editor, const char *name,
                                    StygianEditorColor color);
bool stygian_editor_get_color_token(const StygianEditor *editor,
                                    const char *name,
                                    StygianEditorColor *out_color);
bool stygian_editor_apply_color_token(StygianEditor *editor,
                                      StygianEditorNodeId node_id,
                                      const char *name);
bool stygian_editor_set_text_style(StygianEditor *editor,
                                   const StygianEditorTextStyleDef *style_def);
bool stygian_editor_get_text_style(const StygianEditor *editor, const char *name,
                                   StygianEditorTextStyleDef *out_style_def);
bool stygian_editor_apply_text_style(StygianEditor *editor,
                                     StygianEditorNodeId node_id,
                                     const char *name);
bool stygian_editor_set_effect_style(
    StygianEditor *editor, const StygianEditorEffectStyleDef *style_def);
bool stygian_editor_get_effect_style(
    const StygianEditor *editor, const char *name,
    StygianEditorEffectStyleDef *out_style_def);
bool stygian_editor_apply_effect_style(StygianEditor *editor,
                                       StygianEditorNodeId node_id,
                                       const char *name);
bool stygian_editor_set_layout_style(
    StygianEditor *editor, const StygianEditorLayoutStyleDef *style_def);
bool stygian_editor_get_layout_style(
    const StygianEditor *editor, const char *name,
    StygianEditorLayoutStyleDef *out_style_def);
bool stygian_editor_apply_layout_style(StygianEditor *editor,
                                       StygianEditorNodeId frame_node_id,
                                       const char *name);
uint32_t stygian_editor_style_usage_count(const StygianEditor *editor,
                                          StygianEditorStyleKind kind,
                                          const char *name);
bool stygian_editor_set_variable_mode(StygianEditor *editor, uint32_t mode_index,
                                      const char *mode_name);
uint32_t stygian_editor_variable_mode_count(const StygianEditor *editor);
bool stygian_editor_get_variable_mode(const StygianEditor *editor,
                                      uint32_t mode_index, char *out_mode_name,
                                      size_t out_mode_name_capacity);
bool stygian_editor_set_active_variable_mode(StygianEditor *editor,
                                             uint32_t mode_index);
uint32_t stygian_editor_get_active_variable_mode(const StygianEditor *editor);
bool stygian_editor_set_color_variable(StygianEditor *editor, const char *name,
                                       uint32_t mode_index,
                                       StygianEditorColor value);
bool stygian_editor_get_color_variable(const StygianEditor *editor,
                                       const char *name, uint32_t mode_index,
                                       StygianEditorColor *out_value);
bool stygian_editor_set_number_variable(StygianEditor *editor, const char *name,
                                        uint32_t mode_index, float value);
bool stygian_editor_get_number_variable(const StygianEditor *editor,
                                        const char *name, uint32_t mode_index,
                                        float *out_value);
bool stygian_editor_bind_node_color_variable(StygianEditor *editor,
                                             StygianEditorNodeId node_id,
                                             const char *variable_name);
bool stygian_editor_bind_node_number_variable(
    StygianEditor *editor, StygianEditorNodeId node_id,
    StygianEditorPropertyKind property, const char *variable_name);
bool stygian_editor_apply_active_variable_mode(StygianEditor *editor);
bool stygian_editor_add_control_recipe(StygianEditor *editor,
                                       StygianEditorControlRecipeKind kind,
                                       float x, float y, float w, float h,
                                       const char *name,
                                       StygianEditorNodeId *out_root_id);

bool stygian_editor_add_behavior(StygianEditor *editor,
                                 const StygianEditorBehaviorRule *rule,
                                 StygianEditorBehaviorId *out_behavior_id);
bool stygian_editor_remove_behavior(StygianEditor *editor,
                                    StygianEditorBehaviorId behavior_id);
uint32_t stygian_editor_behavior_count(const StygianEditor *editor);
bool stygian_editor_get_behavior_rule(const StygianEditor *editor,
                                      uint32_t index,
                                      StygianEditorBehaviorId *out_behavior_id,
                                      StygianEditorBehaviorRule *out_rule);
bool stygian_editor_add_driver(StygianEditor *editor,
                               const StygianEditorDriverRule *rule,
                               StygianEditorDriverId *out_driver_id);
bool stygian_editor_remove_driver(StygianEditor *editor,
                                  StygianEditorDriverId driver_id);
uint32_t stygian_editor_driver_count(const StygianEditor *editor);
bool stygian_editor_get_driver_rule(const StygianEditor *editor, uint32_t index,
                                    StygianEditorDriverId *out_driver_id,
                                    StygianEditorDriverRule *out_rule);
bool stygian_editor_apply_driver_sample(StygianEditor *editor,
                                        StygianEditorDriverId driver_id,
                                        float source_value);
bool stygian_editor_timeline_add_track(
    StygianEditor *editor, const StygianEditorTimelineTrack *track_template,
    StygianEditorTimelineTrackId *out_track_id);
bool stygian_editor_timeline_remove_track(StygianEditor *editor,
                                          StygianEditorTimelineTrackId track_id);
uint32_t stygian_editor_timeline_track_count(const StygianEditor *editor);
bool stygian_editor_timeline_get_track(const StygianEditor *editor,
                                       uint32_t index,
                                       StygianEditorTimelineTrack *out_track);
bool stygian_editor_timeline_set_track_keyframes(
    StygianEditor *editor, StygianEditorTimelineTrackId track_id,
    const StygianEditorTimelineKeyframe *keyframes, uint32_t keyframe_count);
bool stygian_editor_timeline_add_clip(
    StygianEditor *editor, const StygianEditorTimelineClip *clip_template,
    StygianEditorTimelineClipId *out_clip_id);
bool stygian_editor_timeline_remove_clip(StygianEditor *editor,
                                         StygianEditorTimelineClipId clip_id);
uint32_t stygian_editor_timeline_clip_count(const StygianEditor *editor);
bool stygian_editor_timeline_get_clip(const StygianEditor *editor,
                                      uint32_t index,
                                      StygianEditorTimelineClip *out_clip);
bool stygian_editor_timeline_set_clip_tracks(
    StygianEditor *editor, StygianEditorTimelineClipId clip_id,
    const StygianEditorTimelineTrackId *track_ids, uint32_t track_count);
void stygian_editor_trigger_event(StygianEditor *editor,
                                  StygianEditorNodeId trigger_node,
                                  StygianEditorEventKind event_kind);
void stygian_editor_tick(StygianEditor *editor, uint64_t now_ms);
bool stygian_editor_begin_transaction(StygianEditor *editor);
bool stygian_editor_end_transaction(StygianEditor *editor, bool commit);
bool stygian_editor_undo(StygianEditor *editor);
bool stygian_editor_redo(StygianEditor *editor);

void stygian_editor_render_viewport2d(const StygianEditor *editor,
                                      StygianContext *ctx, bool draw_grid,
                                      bool draw_selection);

size_t stygian_editor_build_project_json(const StygianEditor *editor,
                                         char *out_json, size_t out_capacity);
bool stygian_editor_load_project_json(StygianEditor *editor, const char *json);
bool stygian_editor_save_project_file(const StygianEditor *editor,
                                      const char *path);
bool stygian_editor_load_project_file(StygianEditor *editor, const char *path);

size_t stygian_editor_build_c23(const StygianEditor *editor, char *out_code,
                                size_t out_capacity);
uint32_t stygian_editor_collect_export_diagnostics(
    const StygianEditor *editor, StygianEditorExportDiagnostic *out_diags,
    uint32_t max_diags);
size_t stygian_editor_build_c23_with_diagnostics(
    const StygianEditor *editor, char *out_code, size_t out_capacity,
    StygianEditorExportDiagnostic *out_diags, uint32_t max_diags,
    uint32_t *out_diag_count, bool *out_has_error);

#ifdef __cplusplus
}
#endif

#endif // STYGIAN_EDITOR_H

