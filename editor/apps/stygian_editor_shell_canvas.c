#include "../../include/stygian.h"
#include "../../include/stygian_clipboard.h"
#include "../include/stygian_editor_icons.h"
#include "../../window/stygian_input.h"
#include "../../widgets/stygian_widgets.h"
#include "../../window/stygian_window.h"
#include "../../src/stb_image.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef STYGIAN_DEMO_VULKAN
#define STYGIAN_EDITOR_SHELL_BACKEND STYGIAN_BACKEND_VULKAN
#define STYGIAN_EDITOR_SHELL_RENDER_FLAG STYGIAN_WINDOW_VULKAN
#else
#define STYGIAN_EDITOR_SHELL_BACKEND STYGIAN_BACKEND_OPENGL
#define STYGIAN_EDITOR_SHELL_RENDER_FLAG STYGIAN_WINDOW_OPENGL
#endif

typedef enum ShellCaptionAction {
  SHELL_CAPTION_NONE = 0,
  SHELL_CAPTION_MINIMIZE = 1,
  SHELL_CAPTION_MAXIMIZE = 2,
  SHELL_CAPTION_CLOSE = 3,
} ShellCaptionAction;

typedef struct ShellLayout {
  float title_h;
  float button_w;
  float button_h;
  float button_gap;
  float button_y;
  float menu_x;
  float menu_y;
  float menu_w;
  float menu_h;
  float shell_pad;
  float shell_radius;
  float min_x;
  float max_x;
  float close_x;
  float shell_x;
  float shell_y;
  float shell_w;
  float shell_h;
  float canvas_x;
  float canvas_y;
  float canvas_w;
  float canvas_h;
  float assets_panel_x;
  float assets_panel_y;
  float assets_panel_w;
  float assets_panel_h;
  float assets_panel_handle_x;
  float assets_panel_handle_y;
  float assets_panel_handle_w;
  float assets_panel_handle_h;
  float right_panel_x;
  float right_panel_y;
  float right_panel_w;
  float right_panel_h;
  float right_panel_handle_x;
  float right_panel_handle_y;
  float right_panel_handle_w;
  float right_panel_handle_h;
  float workspace_x;
  float workspace_y;
  float workspace_w;
  float workspace_h;
  float workspace_seg_w;
} ShellLayout;

typedef struct ShellFrameState {
  float mouse_x;
  float mouse_y;
  float scroll_dy;
  bool event_mutated;
  bool event_requested;
  bool event_eval_requested;
  bool left_down;
  bool left_pressed;
  bool left_released;
  bool middle_down;
  bool middle_pressed;
  bool middle_released;
  int left_clicks;
  bool right_pressed;
  bool enter_pressed;
  bool esc_pressed;
  bool up_pressed;
  bool down_pressed;
  bool left_arrow_pressed;
  bool right_arrow_pressed;
  bool delete_pressed;
  bool backspace_pressed;
  bool d_pressed;
  bool c_pressed;
  bool x_pressed;
  bool v_pressed;
  bool z_pressed;
  bool y_pressed;
  bool ctrl_down;
  bool shift_down;
} ShellFrameState;

typedef struct ShellToolDockLayout {
  float x;
  float y;
  float w;
  float h;
  float visible_y;
  float hidden_y;
  float rail_x;
  float rail_y;
  float rail_w;
  float rail_h;
  float group_gap;
  float button_w;
  float button_h;
  float button_y;
} ShellToolDockLayout;

typedef struct ShellToolDockState {
  int active_tool;
  int open_family;
  bool pointer_inside;
  float visible_t;
  float anim_from_t;
  float anim_to_t;
  uint64_t anim_started_ms;
  uint32_t anim_duration_ms;
  uint64_t hold_until_ms;
  bool animating;
  int hot_index;
  int pressed_index;
  int popup_hot_index;
  int popup_pressed_index;
  int family_choice[10];
} ShellToolDockState;

typedef enum ShellWorkspaceMode {
  SHELL_WORKSPACE_CANVAS = 0,
  SHELL_WORKSPACE_ICON_LAB = 1,
} ShellWorkspaceMode;

typedef enum ShellInspectorTab {
  SHELL_PANEL_DESIGN = 0,
  SHELL_PANEL_MOTION = 1,
} ShellInspectorTab;

typedef enum ShellCanvasFillMode {
  SHELL_FILL_SOLID = 0,
  SHELL_FILL_LINEAR = 1,
} ShellCanvasFillMode;

typedef enum ShellCanvasEffectKind {
  SHELL_EFFECT_NONE = 0,
  SHELL_EFFECT_SHADOW = 1,
  SHELL_EFFECT_BLUR = 2,
  SHELL_EFFECT_GLOW = 3,
} ShellCanvasEffectKind;

typedef enum ShellCanvasTextAlignH {
  SHELL_TEXT_ALIGN_LEFT = 0,
  SHELL_TEXT_ALIGN_CENTER = 1,
  SHELL_TEXT_ALIGN_RIGHT = 2,
} ShellCanvasTextAlignH;

typedef enum ShellCanvasTextAlignV {
  SHELL_TEXT_ALIGN_TOP = 0,
  SHELL_TEXT_ALIGN_MIDDLE = 1,
  SHELL_TEXT_ALIGN_BOTTOM = 2,
} ShellCanvasTextAlignV;

typedef enum ShellCanvasTextOverflow {
  SHELL_TEXT_OVERFLOW_CLIP = 0,
  SHELL_TEXT_OVERFLOW_ELLIPSIS = 1,
  SHELL_TEXT_OVERFLOW_VISIBLE = 2,
} ShellCanvasTextOverflow;

typedef enum ShellCanvasImageFitMode {
  SHELL_IMAGE_FIT_STRETCH = 0,
  SHELL_IMAGE_FIT_CONTAIN = 1,
  SHELL_IMAGE_FIT_COVER = 2,
} ShellCanvasImageFitMode;

typedef struct ShellIconLabState {
  StygianEditorIconKind selected;
  uint64_t copied_until_ms;
} ShellIconLabState;

typedef struct ShellInspectorState {
  int last_selected_id;
  int last_selection_count;
  bool initialized;
  bool editing;
  bool pending_sync;
  int active_field;
  char x_buf[32];
  char y_buf[32];
  char w_buf[32];
  char h_buf[32];
  char rotation_buf[32];
  char radius_buf[32];
  char font_size_buf[32];
  char fill_r_buf[16];
  char fill_g_buf[16];
  char fill_b_buf[16];
  char fill2_r_buf[16];
  char fill2_g_buf[16];
  char fill2_b_buf[16];
  char stroke_r_buf[16];
  char stroke_g_buf[16];
  char stroke_b_buf[16];
  char stroke_width_buf[16];
  char opacity_buf[16];
  char blend_buf[16];
  char radius_tl_buf[16];
  char radius_tr_buf[16];
  char radius_br_buf[16];
  char radius_bl_buf[16];
  char gradient_angle_buf[16];
  char effect_r_buf[16];
  char effect_g_buf[16];
  char effect_b_buf[16];
  char effect_radius_buf[16];
  char effect_spread_buf[16];
  char effect_offset_x_buf[16];
  char effect_offset_y_buf[16];
  char effect_intensity_buf[16];
  char line_height_buf[16];
  char letter_spacing_buf[16];
  char font_weight_buf[16];
  char text_buf[128];
} ShellInspectorState;

typedef enum ShellInspectorField {
  SHELL_INSPECTOR_NONE = -1,
  SHELL_INSPECTOR_X = 0,
  SHELL_INSPECTOR_Y = 1,
  SHELL_INSPECTOR_W = 2,
  SHELL_INSPECTOR_H = 3,
  SHELL_INSPECTOR_ROTATION = 4,
  SHELL_INSPECTOR_RADIUS = 5,
  SHELL_INSPECTOR_FONT_SIZE = 6,
  SHELL_INSPECTOR_TEXT = 7,
  SHELL_INSPECTOR_FILL_R = 8,
  SHELL_INSPECTOR_FILL_G = 9,
  SHELL_INSPECTOR_FILL_B = 10,
  SHELL_INSPECTOR_STROKE_R = 11,
  SHELL_INSPECTOR_STROKE_G = 12,
  SHELL_INSPECTOR_STROKE_B = 13,
  SHELL_INSPECTOR_STROKE_WIDTH = 14,
  SHELL_INSPECTOR_OPACITY = 15,
} ShellInspectorField;

typedef struct ShellMenuState {
  int open_menu;
  int hot_menu;
  int hot_item;
} ShellMenuState;

typedef struct ShellSidebarState {
  float left_panel_width;
  float left_drag_offset_x;
  float right_panel_width;
  float drag_offset_x;
  bool dragging_left_panel;
  bool dragging_right_panel;
} ShellSidebarState;

#define SHELL_PAGE_CAP 8
#define SHELL_ASSET_CAP 64
#define SHELL_CANVAS_FRAME_CAP 64
#define SHELL_PATH_POINT_CAP 32
#define SHELL_HISTORY_CAP 64
#define SHELL_MOTION_TRACK_CAP 128
#define SHELL_MOTION_CLIP_CAP 32
#define SHELL_MOTION_BEHAVIOR_CAP 128
#define SHELL_MOTION_CLIP_TRACK_CAP 16
#define SHELL_MOTION_INSTANCE_CAP 32

typedef enum ShellMotionPropertyKind {
  SHELL_MOTION_PROP_X = 0,
  SHELL_MOTION_PROP_Y = 1,
  SHELL_MOTION_PROP_SCALE = 2,
  SHELL_MOTION_PROP_ROTATION = 3,
  SHELL_MOTION_PROP_OPACITY = 4,
  SHELL_MOTION_PROP_COLOR = 5,
} ShellMotionPropertyKind;

typedef enum ShellMotionEasing {
  SHELL_MOTION_EASE_LINEAR = 0,
  SHELL_MOTION_EASE_OUT_CUBIC = 1,
  SHELL_MOTION_EASE_IN_OUT_CUBIC = 2,
} ShellMotionEasing;

typedef enum ShellMotionEventKind {
  SHELL_MOTION_EVENT_PRESS = 0,
  SHELL_MOTION_EVENT_RELEASE = 1,
  SHELL_MOTION_EVENT_HOVER_ENTER = 2,
  SHELL_MOTION_EVENT_HOVER_LEAVE = 3,
} ShellMotionEventKind;

typedef enum ShellMotionField {
  SHELL_MOTION_FIELD_NONE = -1,
  SHELL_MOTION_FIELD_CLIP_NAME = 0,
  SHELL_MOTION_FIELD_START = 1,
  SHELL_MOTION_FIELD_DURATION = 2,
  SHELL_MOTION_FIELD_FROM = 3,
  SHELL_MOTION_FIELD_TO = 4,
  SHELL_MOTION_FIELD_FROM_R = 5,
  SHELL_MOTION_FIELD_FROM_G = 6,
  SHELL_MOTION_FIELD_FROM_B = 7,
  SHELL_MOTION_FIELD_TO_R = 8,
  SHELL_MOTION_FIELD_TO_G = 9,
  SHELL_MOTION_FIELD_TO_B = 10,
} ShellMotionField;

typedef enum ShellMotionTimelineDragKind {
  SHELL_TIMELINE_DRAG_NONE = 0,
  SHELL_TIMELINE_DRAG_PLAYHEAD = 1,
  SHELL_TIMELINE_DRAG_TRACK_START = 2,
  SHELL_TIMELINE_DRAG_TRACK_END = 3,
} ShellMotionTimelineDragKind;

typedef enum ShellMotionScaffoldKind {
  SHELL_SCAFFOLD_NONE = 0,
  SHELL_SCAFFOLD_HOVER = 1,
  SHELL_SCAFFOLD_PRESS = 2,
  SHELL_SCAFFOLD_TABS = 3,
  SHELL_SCAFFOLD_DRAWER = 4,
  SHELL_SCAFFOLD_MODAL = 5,
} ShellMotionScaffoldKind;

typedef struct ShellMotionTrack {
  bool alive;
  int id;
  int target_node_id;
  ShellMotionPropertyKind property;
  uint32_t start_ms;
  uint32_t duration_ms;
  ShellMotionEasing easing;
  char name[32];
  float from_value;
  float to_value;
  float from_rgba[4];
  float to_rgba[4];
} ShellMotionTrack;

typedef struct ShellMotionClip {
  bool alive;
  int id;
  char name[32];
  bool loop;
  uint32_t duration_ms;
  int track_count;
  int track_ids[SHELL_MOTION_CLIP_TRACK_CAP];
} ShellMotionClip;

typedef struct ShellMotionBehavior {
  bool alive;
  int id;
  int trigger_node_id;
  ShellMotionEventKind event_kind;
  int clip_id;
  bool reverse;
  bool toggle;
  bool toggle_state;
  char name[48];
} ShellMotionBehavior;

typedef struct ShellMotionInstance {
  bool active;
  int clip_id;
  uint64_t started_ms;
  bool reverse;
} ShellMotionInstance;

typedef enum ShellCanvasNodeKind {
  SHELL_NODE_FRAME = 0,
  SHELL_NODE_RECTANGLE = 1,
  SHELL_NODE_ELLIPSE = 2,
  SHELL_NODE_LINE = 3,
  SHELL_NODE_ARROW = 4,
  SHELL_NODE_POLYGON = 5,
  SHELL_NODE_STAR = 6,
  SHELL_NODE_IMAGE = 7,
  SHELL_NODE_PATH = 8,
  SHELL_NODE_TEXT = 9,
} ShellCanvasNodeKind;

typedef enum ShellWorkspacePanelTab {
  SHELL_WORKSPACE_TAB_PAGES = 0,
  SHELL_WORKSPACE_TAB_LAYERS = 1,
  SHELL_WORKSPACE_TAB_ASSETS = 2,
} ShellWorkspacePanelTab;

typedef enum ShellAssetKind {
  SHELL_ASSET_IMAGE = 0,
  SHELL_ASSET_FONT = 1,
  SHELL_ASSET_TEXTURE = 2,
} ShellAssetKind;

typedef struct ShellCanvasPage {
  bool alive;
  int id;
  char name[32];
} ShellCanvasPage;

typedef struct ShellAssetEntry {
  bool alive;
  int id;
  ShellAssetKind kind;
  char path[STYGIAN_FS_PATH_CAP];
  char label[STYGIAN_FS_NAME_CAP];
} ShellAssetEntry;

typedef struct ShellAssetLibrary {
  ShellAssetEntry entries[SHELL_ASSET_CAP];
  uint32_t entry_count;
  int next_asset_id;
  int active_image_id;
  int active_font_id;
  int active_texture_id;
} ShellAssetLibrary;

typedef struct ShellCanvasFrameNode {
  bool alive;
  bool selected;
  bool visible;
  bool locked;
  int id;
  int page_id;
  int parent_id;
  ShellCanvasNodeKind kind;
  float x;
  float y;
  float w;
  float h;
  float rotation_deg;
  float fill_rgba[4];
  float fill_alt_rgba[4];
  float stroke_rgba[4];
  float effect_rgba[4];
  float stroke_width;
  float opacity;
  float blend;
  float corner_radius[4];
  bool corner_custom;
  ShellCanvasFillMode fill_mode;
  float fill_angle_deg;
  ShellCanvasEffectKind effect_kind;
  float effect_radius;
  float effect_spread;
  float effect_offset_x;
  float effect_offset_y;
  float effect_intensity;
  float radius;
  float font_size;
  float line_height;
  float letter_spacing;
  uint32_t font_weight;
  ShellCanvasTextAlignH text_align_h;
  ShellCanvasTextAlignV text_align_v;
  bool text_wrap;
  ShellCanvasTextOverflow text_overflow;
  ShellCanvasImageFitMode image_fit_mode;
  bool top_flat;
  int preset_w;
  int preset_h;
  char name[32];
  char asset_path[STYGIAN_FS_PATH_CAP];
  StygianTexture image_texture;
  int image_w;
  int image_h;
  bool path_closed;
  int point_count;
  int selected_point;
  float path_x[SHELL_PATH_POINT_CAP];
  float path_y[SHELL_PATH_POINT_CAP];
  char text[128];
} ShellCanvasFrameNode;

typedef struct ShellCanvasSceneState {
  ShellCanvasFrameNode frames[SHELL_CANVAS_FRAME_CAP];
  uint32_t frame_count;
  ShellCanvasPage pages[SHELL_PAGE_CAP];
  uint32_t page_count;
  int selected_frame;
  int active_page_id;
  int next_frame_id;
  int next_page_id;
} ShellCanvasSceneState;

typedef struct ShellWorkspacePanelState {
  ShellWorkspacePanelTab active_tab;
  int active_layer_id;
  int editing_name_id;
  bool editing_page_name;
  char name_buf[64];
  float pages_scroll_y;
  float layers_scroll_y;
  float assets_scroll_y;
} ShellWorkspacePanelState;

typedef struct ShellMotionState {
  ShellInspectorTab active_tab;
  ShellMotionTrack tracks[SHELL_MOTION_TRACK_CAP];
  int track_count;
  int next_track_id;
  ShellMotionClip clips[SHELL_MOTION_CLIP_CAP];
  int clip_count;
  int next_clip_id;
  ShellMotionBehavior behaviors[SHELL_MOTION_BEHAVIOR_CAP];
  int behavior_count;
  int next_behavior_id;
  ShellMotionInstance instances[SHELL_MOTION_INSTANCE_CAP];
  int selected_clip_id;
  int selected_track_id;
  int hovered_node_id;
  int pressed_node_id;
  bool playing;
  bool scrub_active;
  bool editing;
  bool preview_snapshot_valid;
  uint64_t play_started_ms;
  uint32_t playhead_ms;
  ShellMotionTimelineDragKind drag_kind;
  int drag_track_id;
  float drag_start_mouse_x;
  uint32_t drag_start_ms;
  uint32_t drag_start_duration_ms;
  int last_clip_id;
  int last_track_id;
  ShellMotionField active_field;
  char clip_name_buf[32];
  char start_buf[16];
  char duration_buf[16];
  char from_buf[16];
  char to_buf[16];
  char from_r_buf[16];
  char from_g_buf[16];
  char from_b_buf[16];
  char to_r_buf[16];
  char to_g_buf[16];
  char to_b_buf[16];
  ShellCanvasSceneState preview_base_scene;
} ShellMotionState;

typedef struct ShellCanvasHistoryState {
  ShellCanvasSceneState undo[SHELL_HISTORY_CAP];
  int undo_count;
  ShellCanvasSceneState redo[SHELL_HISTORY_CAP];
  int redo_count;
} ShellCanvasHistoryState;

typedef struct ShellCanvasClipboardState {
  ShellCanvasFrameNode nodes[SHELL_CANVAS_FRAME_CAP];
  int count;
  int paste_serial;
} ShellCanvasClipboardState;

typedef struct ShellPathDraftState {
  bool active;
  bool closed;
  int point_count;
  float points_x[SHELL_PATH_POINT_CAP];
  float points_y[SHELL_PATH_POINT_CAP];
} ShellPathDraftState;

typedef enum ShellCanvasInteractionMode {
  SHELL_CANVAS_INTERACT_NONE = 0,
  SHELL_CANVAS_INTERACT_MOVE,
  SHELL_CANVAS_INTERACT_MARQUEE,
  SHELL_CANVAS_INTERACT_RESIZE_L,
  SHELL_CANVAS_INTERACT_RESIZE_R,
  SHELL_CANVAS_INTERACT_RESIZE_T,
  SHELL_CANVAS_INTERACT_RESIZE_B,
  SHELL_CANVAS_INTERACT_RESIZE_TL,
  SHELL_CANVAS_INTERACT_RESIZE_TR,
  SHELL_CANVAS_INTERACT_RESIZE_BL,
  SHELL_CANVAS_INTERACT_RESIZE_BR,
  SHELL_CANVAS_INTERACT_ROTATE,
  SHELL_CANVAS_INTERACT_MOVE_POINT,
  SHELL_CANVAS_INTERACT_CREATE_FRAME,
} ShellCanvasInteractionMode;

typedef struct ShellCanvasInteractionState {
  ShellCanvasInteractionMode mode;
  int target_frame;
  int target_point;
  ShellCanvasNodeKind create_kind;
  float create_radius;
  bool group_active;
  bool additive_selection;
  float anchor_world_x;
  float anchor_world_y;
  float origin_world_x;
  float origin_world_y;
  float origin_w;
  float origin_h;
  float origin_rotation_deg;
  float rotate_center_x;
  float rotate_center_y;
  float rotate_anchor_angle_deg;
  float group_origin_x;
  float group_origin_y;
  float group_origin_w;
  float group_origin_h;
  float frame_origin_x[SHELL_CANVAS_FRAME_CAP];
  float frame_origin_y[SHELL_CANVAS_FRAME_CAP];
  float frame_origin_w[SHELL_CANVAS_FRAME_CAP];
  float frame_origin_h[SHELL_CANVAS_FRAME_CAP];
  float frame_origin_rotation[SHELL_CANVAS_FRAME_CAP];
} ShellCanvasInteractionState;

typedef struct ShellCanvasViewState {
  float pan_x;
  float pan_y;
  float zoom;
  bool panning;
  float pan_start_mouse_x;
  float pan_start_mouse_y;
  float pan_origin_x;
  float pan_origin_y;
} ShellCanvasViewState;

typedef struct ShellCanvasSnapSettings {
  bool guide_enabled;
  bool grid_enabled;
  float grid_size;
} ShellCanvasSnapSettings;

typedef struct ShellCanvasGuideState {
  bool x_active;
  bool y_active;
  bool x_from_grid;
  bool y_from_grid;
  float x_world;
  float y_world;
} ShellCanvasGuideState;

typedef enum ShellCanvasAlignKind {
  SHELL_ALIGN_LEFT = 0,
  SHELL_ALIGN_CENTER_X,
  SHELL_ALIGN_RIGHT,
  SHELL_ALIGN_TOP,
  SHELL_ALIGN_CENTER_Y,
  SHELL_ALIGN_BOTTOM
} ShellCanvasAlignKind;

typedef enum ShellCanvasContextAction {
  SHELL_CONTEXT_SELECT_ALL = 0,
  SHELL_CONTEXT_DUPLICATE,
  SHELL_CONTEXT_DELETE,
  SHELL_CONTEXT_ALIGN_LEFT,
  SHELL_CONTEXT_ALIGN_CENTER,
  SHELL_CONTEXT_ALIGN_RIGHT,
  SHELL_CONTEXT_ALIGN_TOP,
  SHELL_CONTEXT_ALIGN_MIDDLE,
  SHELL_CONTEXT_ALIGN_BOTTOM,
  SHELL_CONTEXT_DISTRIBUTE_H,
  SHELL_CONTEXT_DISTRIBUTE_V
} ShellCanvasContextAction;

typedef enum ShellDialogKind {
  SHELL_DIALOG_NONE = 0,
  SHELL_DIALOG_OPEN_PROJECT = 1,
  SHELL_DIALOG_IMPORT_ASSET = 2,
  SHELL_DIALOG_IMPORT_IMAGE = 3,
  SHELL_DIALOG_IMPORT_FONT = 4,
  SHELL_DIALOG_IMPORT_TEXTURE = 5,
} ShellDialogKind;

typedef struct ShellDialogState {
  bool open;
  bool ignore_open_click;
  bool dragging_resize;
  bool dragging_move;
  ShellDialogKind kind;
  float dialog_x;
  float dialog_y;
  float dialog_w;
  float dialog_h;
  float drag_start_mouse_x;
  float drag_start_mouse_y;
  float drag_start_x;
  float drag_start_y;
  float resize_start_mouse_x;
  float resize_start_mouse_y;
  float resize_start_w;
  float resize_start_h;
  char browser_root[STYGIAN_FS_PATH_CAP];
  char suggested_path[STYGIAN_FS_PATH_CAP];
  char search_query[128];
  char preview_path[STYGIAN_FS_PATH_CAP];
  char preview_text[768];
  StygianTexture preview_texture;
  int preview_image_w;
  int preview_image_h;
  int preview_image_comp;
  bool preview_ready;
  bool preview_text_ready;
  bool preview_failed;
  StygianFileExplorer browser;
} ShellDialogState;

static void shell_draw_text_block(StygianContext *ctx, StygianFont font,
                                  const ShellCanvasFrameNode *frame, float x,
                                  float y, float w, float h);
static void shell_draw_image_surface(StygianContext *ctx,
                                     const ShellCanvasFrameNode *frame,
                                     float x, float y, float w, float h);

#define SHELL_RECENT_COMMIT_CAP 4

typedef struct ShellRecentCommit {
  ShellDialogKind kind;
  char path[STYGIAN_FS_PATH_CAP];
} ShellRecentCommit;

typedef struct ShellCommitState {
  ShellRecentCommit recent[SHELL_RECENT_COMMIT_CAP];
  uint32_t recent_count;
  ShellDialogKind last_kind;
  char last_path[STYGIAN_FS_PATH_CAP];
} ShellCommitState;

typedef struct ShellCanvasImportState {
  ShellDialogKind kind;
  char path[STYGIAN_FS_PATH_CAP];
  StygianTexture preview_texture;
  int image_w;
  int image_h;
  int image_comp;
  bool preview_ready;
  bool preview_failed;
} ShellCanvasImportState;

typedef struct ShellRecentHit {
  int index;
  bool remove;
} ShellRecentHit;

static bool shell_point_in_rect(float px, float py, float x, float y, float w,
                                float h);
static void shell_copy_fitted_text(char *out, size_t out_cap, StygianContext *ctx,
                                   StygianFont font, const char *text,
                                   float max_w, float size);
static void shell_canvas_node_release(StygianContext *ctx,
                                      ShellCanvasFrameNode *frame);
static bool shell_canvas_node_load_image(StygianContext *ctx,
                                         ShellCanvasFrameNode *frame);
static void shell_canvas_scene_clear_selection(ShellCanvasSceneState *scene);
static void shell_canvas_scene_select_only(ShellCanvasSceneState *scene, int index);
static void shell_canvas_scene_release(StygianContext *ctx,
                                       ShellCanvasSceneState *scene);
static void shell_canvas_scene_init(ShellCanvasSceneState *scene);
static int shell_canvas_scene_active_page_index(const ShellCanvasSceneState *scene);
static ShellCanvasPage *shell_canvas_scene_active_page(ShellCanvasSceneState *scene);
static const ShellCanvasPage *
shell_canvas_scene_active_page_const(const ShellCanvasSceneState *scene);
static int shell_canvas_scene_find_page_index(const ShellCanvasSceneState *scene,
                                              int page_id);
static ShellCanvasFrameNode *
shell_canvas_scene_find_node_by_id(ShellCanvasSceneState *scene, int node_id);
static const ShellCanvasFrameNode *
shell_canvas_scene_find_node_by_id_const(const ShellCanvasSceneState *scene,
                                         int node_id);
static bool shell_canvas_scene_node_in_active_page(
    const ShellCanvasSceneState *scene, const ShellCanvasFrameNode *node);
static bool shell_canvas_scene_node_effectively_visible(
    const ShellCanvasSceneState *scene, const ShellCanvasFrameNode *node);
static bool shell_canvas_scene_node_effectively_locked(
    const ShellCanvasSceneState *scene, const ShellCanvasFrameNode *node);
static void shell_canvas_scene_clear_selection_for_hidden(
    ShellCanvasSceneState *scene);
static void shell_canvas_scene_move_children(ShellCanvasSceneState *scene,
                                             int parent_id, float dx, float dy);
static bool shell_canvas_scene_is_descendant(const ShellCanvasSceneState *scene,
                                             int node_id, int parent_id);
static void shell_canvas_scene_assign_parent(ShellCanvasSceneState *scene,
                                             ShellCanvasFrameNode *node,
                                             int parent_id);
static int shell_canvas_scene_pick_parent_id(const ShellCanvasSceneState *scene,
                                             const ShellCanvasFrameNode *node);
static bool shell_canvas_scene_add_page(ShellCanvasSceneState *scene,
                                        const char *name);
static bool shell_canvas_scene_delete_active_page(StygianContext *ctx,
                                                  ShellCanvasSceneState *scene);
static bool shell_canvas_scene_activate_page(ShellCanvasSceneState *scene,
                                             int page_id);
static int shell_canvas_scene_node_depth(const ShellCanvasSceneState *scene,
                                         const ShellCanvasFrameNode *node);
static void shell_canvas_scene_cycle_parent(ShellCanvasSceneState *scene,
                                            ShellCanvasFrameNode *node);
static bool shell_canvas_scene_reorder_selected(ShellCanvasSceneState *scene,
                                                int direction);
static bool shell_canvas_scene_bring_selected_to_front(
    ShellCanvasSceneState *scene);
static bool shell_canvas_scene_send_selected_to_back(
    ShellCanvasSceneState *scene);
static const char *shell_asset_kind_label(ShellAssetKind kind);
static void shell_dialog_open(ShellDialogState *state, ShellDialogKind kind);
static void shell_asset_library_add(ShellAssetLibrary *library,
                                    ShellDialogKind dialog_kind,
                                    const char *path);
static const ShellAssetEntry *
shell_asset_library_active_for_dialog(const ShellAssetLibrary *library,
                                      ShellDialogKind dialog_kind);

static void shell_draw_chevron_left(StygianContext *ctx, float cx, float cy,
                                    float size, float thickness, float r,
                                    float g, float b, float a);
static void shell_draw_chevron_right(StygianContext *ctx, float cx, float cy,
                                     float size, float thickness, float r,
                                     float g, float b, float a);

static float shell_clamp01(float v) {
  if (v < 0.0f)
    return 0.0f;
  if (v > 1.0f)
    return 1.0f;
  return v;
}

static float shell_clampf(float v, float lo, float hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static float shell_canvas_zoom_clamp(float zoom) {
  return shell_clampf(zoom, 0.25f, 4.0f);
}

static float shell_canvas_snap_threshold_world(const ShellCanvasViewState *view) {
  float zoom = view && view->zoom > 0.0f ? view->zoom : 1.0f;
  float screen_threshold = 10.0f - (zoom - 1.0f) * 1.75f;
  screen_threshold = shell_clampf(screen_threshold, 6.0f, 16.0f);
  return screen_threshold / zoom;
}

static void shell_canvas_clamp_frame_radius(ShellCanvasFrameNode *frame) {
  float max_radius = 0.0f;
  int i = 0;
  if (!frame)
    return;
  max_radius = fminf(frame->w, frame->h) * 0.5f;
  if (frame->radius > max_radius)
    frame->radius = max_radius;
  if (frame->radius < 0.0f)
    frame->radius = 0.0f;
  for (i = 0; i < 4; ++i) {
    if (frame->corner_radius[i] > max_radius)
      frame->corner_radius[i] = max_radius;
    if (frame->corner_radius[i] < 0.0f)
      frame->corner_radius[i] = 0.0f;
  }
  if (!frame->corner_custom) {
    for (i = 0; i < 4; ++i)
      frame->corner_radius[i] = frame->radius;
  }
}

static float shell_color_to_u8(float channel) {
  return floorf(shell_clampf(channel, 0.0f, 1.0f) * 255.0f + 0.5f);
}

static float shell_u8_to_color(float channel) {
  return shell_clampf(channel, 0.0f, 255.0f) / 255.0f;
}

static bool shell_canvas_node_has_radius(ShellCanvasNodeKind kind) {
  return kind == SHELL_NODE_FRAME || kind == SHELL_NODE_RECTANGLE;
}

static const char *shell_fill_mode_label(ShellCanvasFillMode mode) {
  switch (mode) {
  case SHELL_FILL_LINEAR:
    return "Linear";
  case SHELL_FILL_SOLID:
  default:
    return "Solid";
  }
}

static const char *shell_effect_kind_label(ShellCanvasEffectKind kind) {
  switch (kind) {
  case SHELL_EFFECT_SHADOW:
    return "Shadow";
  case SHELL_EFFECT_BLUR:
    return "Blur";
  case SHELL_EFFECT_GLOW:
    return "Glow";
  case SHELL_EFFECT_NONE:
  default:
    return "None";
  }
}

static const char *shell_motion_property_label(ShellMotionPropertyKind property) {
  switch (property) {
  case SHELL_MOTION_PROP_X:
    return "X";
  case SHELL_MOTION_PROP_Y:
    return "Y";
  case SHELL_MOTION_PROP_SCALE:
    return "Scale";
  case SHELL_MOTION_PROP_ROTATION:
    return "Rotation";
  case SHELL_MOTION_PROP_OPACITY:
    return "Opacity";
  case SHELL_MOTION_PROP_COLOR:
    return "Color";
  default:
    return "Track";
  }
}

static const char *shell_motion_easing_label(ShellMotionEasing easing) {
  switch (easing) {
  case SHELL_MOTION_EASE_OUT_CUBIC:
    return "Out Cubic";
  case SHELL_MOTION_EASE_IN_OUT_CUBIC:
    return "InOut Cubic";
  case SHELL_MOTION_EASE_LINEAR:
  default:
    return "Linear";
  }
}

static const char *shell_motion_event_label(ShellMotionEventKind event_kind) {
  switch (event_kind) {
  case SHELL_MOTION_EVENT_RELEASE:
    return "Release";
  case SHELL_MOTION_EVENT_HOVER_ENTER:
    return "Hover In";
  case SHELL_MOTION_EVENT_HOVER_LEAVE:
    return "Hover Out";
  case SHELL_MOTION_EVENT_PRESS:
  default:
    return "Press";
  }
}

static const char *shell_text_align_h_label(ShellCanvasTextAlignH align) {
  switch (align) {
  case SHELL_TEXT_ALIGN_CENTER:
    return "Center";
  case SHELL_TEXT_ALIGN_RIGHT:
    return "Right";
  case SHELL_TEXT_ALIGN_LEFT:
  default:
    return "Left";
  }
}

static const char *shell_text_align_v_label(ShellCanvasTextAlignV align) {
  switch (align) {
  case SHELL_TEXT_ALIGN_MIDDLE:
    return "Middle";
  case SHELL_TEXT_ALIGN_BOTTOM:
    return "Bottom";
  case SHELL_TEXT_ALIGN_TOP:
  default:
    return "Top";
  }
}

static const char *shell_text_overflow_label(ShellCanvasTextOverflow mode) {
  switch (mode) {
  case SHELL_TEXT_OVERFLOW_ELLIPSIS:
    return "Ellipsis";
  case SHELL_TEXT_OVERFLOW_VISIBLE:
    return "Visible";
  case SHELL_TEXT_OVERFLOW_CLIP:
  default:
    return "Clip";
  }
}

static const char *shell_image_fit_label(ShellCanvasImageFitMode mode) {
  switch (mode) {
  case SHELL_IMAGE_FIT_CONTAIN:
    return "Contain";
  case SHELL_IMAGE_FIT_COVER:
    return "Cover";
  case SHELL_IMAGE_FIT_STRETCH:
  default:
    return "Stretch";
  }
}

static void shell_canvas_node_corner_radii(const ShellCanvasFrameNode *frame,
                                           float out_radius[4]) {
  int i = 0;
  if (!out_radius)
    return;
  if (!frame) {
    for (i = 0; i < 4; ++i)
      out_radius[i] = 0.0f;
    return;
  }
  if (frame->kind != SHELL_NODE_RECTANGLE && frame->kind != SHELL_NODE_FRAME) {
    for (i = 0; i < 4; ++i)
      out_radius[i] = 0.0f;
    return;
  }
  if (frame->corner_custom) {
    for (i = 0; i < 4; ++i)
      out_radius[i] = frame->corner_radius[i];
  } else {
    for (i = 0; i < 4; ++i)
      out_radius[i] = frame->radius;
  }
}

static void shell_canvas_apply_effect(StygianContext *ctx, StygianElement element,
                                      const ShellCanvasFrameNode *frame) {
  float alpha = 0.0f;
  if (!ctx || !element || !frame)
    return;
  alpha = frame->effect_rgba[3] * frame->opacity;
  if (frame->blend > 0.0f)
    stygian_set_blend(ctx, element, frame->blend);
  if (frame->effect_kind == SHELL_EFFECT_NONE || alpha <= 0.0f)
    return;
  if (frame->effect_kind == SHELL_EFFECT_SHADOW) {
    stygian_set_shadow(ctx, element, frame->effect_offset_x, frame->effect_offset_y,
                       frame->effect_radius, frame->effect_spread,
                       frame->effect_rgba[0], frame->effect_rgba[1],
                       frame->effect_rgba[2], alpha);
  } else if (frame->effect_kind == SHELL_EFFECT_BLUR) {
    stygian_set_blur(ctx, element, frame->effect_radius);
  } else if (frame->effect_kind == SHELL_EFFECT_GLOW) {
    stygian_set_glow(ctx, element,
                     frame->effect_intensity > 0.0f ? frame->effect_intensity
                                                    : 1.0f);
  }
}

static bool shell_canvas_node_is_outline_kind(ShellCanvasNodeKind kind) {
  return kind == SHELL_NODE_LINE || kind == SHELL_NODE_ARROW ||
         kind == SHELL_NODE_POLYGON || kind == SHELL_NODE_STAR ||
         kind == SHELL_NODE_PATH;
}

static void shell_canvas_node_set_style(ShellCanvasFrameNode *frame, float fill_r,
                                        float fill_g, float fill_b, float fill_a,
                                        float stroke_r, float stroke_g,
                                        float stroke_b, float stroke_a,
                                        float stroke_width, float opacity) {
  if (!frame)
    return;
  frame->fill_rgba[0] = shell_clampf(fill_r, 0.0f, 1.0f);
  frame->fill_rgba[1] = shell_clampf(fill_g, 0.0f, 1.0f);
  frame->fill_rgba[2] = shell_clampf(fill_b, 0.0f, 1.0f);
  frame->fill_rgba[3] = shell_clampf(fill_a, 0.0f, 1.0f);
  frame->stroke_rgba[0] = shell_clampf(stroke_r, 0.0f, 1.0f);
  frame->stroke_rgba[1] = shell_clampf(stroke_g, 0.0f, 1.0f);
  frame->stroke_rgba[2] = shell_clampf(stroke_b, 0.0f, 1.0f);
  frame->stroke_rgba[3] = shell_clampf(stroke_a, 0.0f, 1.0f);
  frame->stroke_width = shell_clampf(stroke_width, 0.0f, 24.0f);
  frame->opacity = shell_clampf(opacity, 0.0f, 1.0f);
}

static void shell_canvas_apply_default_style(ShellCanvasFrameNode *frame) {
  int i = 0;
  if (!frame)
    return;
  frame->fill_mode = SHELL_FILL_SOLID;
  frame->fill_angle_deg = 0.0f;
  frame->effect_kind = SHELL_EFFECT_NONE;
  frame->effect_radius = 12.0f;
  frame->effect_spread = 0.0f;
  frame->effect_offset_x = 0.0f;
  frame->effect_offset_y = 8.0f;
  frame->effect_intensity = 1.0f;
  frame->blend = 1.0f;
  frame->line_height = 0.0f;
  frame->letter_spacing = 0.0f;
  frame->font_weight = 400u;
  frame->text_align_h = SHELL_TEXT_ALIGN_LEFT;
  frame->text_align_v = SHELL_TEXT_ALIGN_TOP;
  frame->text_wrap = true;
  frame->text_overflow = SHELL_TEXT_OVERFLOW_CLIP;
  frame->image_fit_mode = SHELL_IMAGE_FIT_CONTAIN;
  frame->corner_custom = false;
  for (i = 0; i < 4; ++i) {
    frame->corner_radius[i] = frame->radius;
  }
  switch (frame->kind) {
  case SHELL_NODE_TEXT:
    shell_canvas_node_set_style(frame, 0.96f, 0.96f, 0.98f, 1.0f, 0.0f, 0.0f,
                                0.0f, 0.0f, 0.0f, 1.0f);
    frame->fill_alt_rgba[0] = frame->fill_rgba[0];
    frame->fill_alt_rgba[1] = frame->fill_rgba[1];
    frame->fill_alt_rgba[2] = frame->fill_rgba[2];
    frame->fill_alt_rgba[3] = frame->fill_rgba[3];
    frame->effect_rgba[0] = 0.00f;
    frame->effect_rgba[1] = 0.00f;
    frame->effect_rgba[2] = 0.00f;
    frame->effect_rgba[3] = 0.42f;
    break;
  case SHELL_NODE_IMAGE:
    shell_canvas_node_set_style(frame, 0.10f, 0.10f, 0.11f, 0.96f, 0.92f, 0.92f,
                                0.95f, 0.85f, 2.0f, 1.0f);
    frame->fill_alt_rgba[0] = 0.18f;
    frame->fill_alt_rgba[1] = 0.18f;
    frame->fill_alt_rgba[2] = 0.19f;
    frame->fill_alt_rgba[3] = 0.96f;
    frame->effect_rgba[0] = 0.00f;
    frame->effect_rgba[1] = 0.00f;
    frame->effect_rgba[2] = 0.00f;
    frame->effect_rgba[3] = 0.42f;
    break;
  case SHELL_NODE_LINE:
  case SHELL_NODE_ARROW:
  case SHELL_NODE_POLYGON:
  case SHELL_NODE_STAR:
  case SHELL_NODE_PATH:
    shell_canvas_node_set_style(frame, 0.0f, 0.0f, 0.0f, 0.0f, 0.92f, 0.93f,
                                0.96f, 0.95f, 2.0f, 1.0f);
    frame->fill_alt_rgba[0] = frame->stroke_rgba[0];
    frame->fill_alt_rgba[1] = frame->stroke_rgba[1];
    frame->fill_alt_rgba[2] = frame->stroke_rgba[2];
    frame->fill_alt_rgba[3] = frame->stroke_rgba[3];
    frame->effect_rgba[0] = 0.18f;
    frame->effect_rgba[1] = 0.54f;
    frame->effect_rgba[2] = 0.95f;
    frame->effect_rgba[3] = 0.30f;
    break;
  case SHELL_NODE_ELLIPSE:
    shell_canvas_node_set_style(frame, 0.92f, 0.93f, 0.96f, 0.96f, 0.94f, 0.95f,
                                0.98f, 1.0f, 2.0f, 1.0f);
    frame->fill_alt_rgba[0] = 0.42f;
    frame->fill_alt_rgba[1] = 0.68f;
    frame->fill_alt_rgba[2] = 0.98f;
    frame->fill_alt_rgba[3] = 0.96f;
    frame->effect_rgba[0] = 0.00f;
    frame->effect_rgba[1] = 0.00f;
    frame->effect_rgba[2] = 0.00f;
    frame->effect_rgba[3] = 0.42f;
    break;
  case SHELL_NODE_RECTANGLE:
    shell_canvas_node_set_style(frame, 0.93f, 0.94f, 0.97f, 0.96f, 0.94f, 0.95f,
                                0.98f, 1.0f, 2.0f, 1.0f);
    frame->fill_alt_rgba[0] = 0.36f;
    frame->fill_alt_rgba[1] = 0.63f;
    frame->fill_alt_rgba[2] = 0.96f;
    frame->fill_alt_rgba[3] = 0.96f;
    frame->effect_rgba[0] = 0.00f;
    frame->effect_rgba[1] = 0.00f;
    frame->effect_rgba[2] = 0.00f;
    frame->effect_rgba[3] = 0.42f;
    break;
  case SHELL_NODE_FRAME:
  default:
    shell_canvas_node_set_style(frame, 0.95f, 0.96f, 0.98f, 0.94f, 0.95f, 0.96f,
                                0.99f, 1.0f, 2.0f, 1.0f);
    frame->fill_alt_rgba[0] = 0.22f;
    frame->fill_alt_rgba[1] = 0.56f;
    frame->fill_alt_rgba[2] = 0.96f;
    frame->fill_alt_rgba[3] = 0.94f;
    frame->effect_rgba[0] = 0.00f;
    frame->effect_rgba[1] = 0.00f;
    frame->effect_rgba[2] = 0.00f;
    frame->effect_rgba[3] = 0.42f;
    break;
  }
  if (frame->kind == SHELL_NODE_FRAME || frame->kind == SHELL_NODE_RECTANGLE) {
    for (i = 0; i < 4; ++i)
      frame->corner_radius[i] = frame->radius;
  }
}

static float shell_canvas_node_min_width(ShellCanvasNodeKind kind) {
  switch (kind) {
  case SHELL_NODE_LINE:
    return 2.0f;
  case SHELL_NODE_ARROW:
    return 24.0f;
  case SHELL_NODE_POLYGON:
  case SHELL_NODE_STAR:
    return 24.0f;
  case SHELL_NODE_IMAGE:
    return 48.0f;
  case SHELL_NODE_PATH:
    return 8.0f;
  case SHELL_NODE_TEXT:
    return 48.0f;
  case SHELL_NODE_RECTANGLE:
  case SHELL_NODE_ELLIPSE:
    return 24.0f;
  case SHELL_NODE_FRAME:
  default:
    return 96.0f;
  }
}

static float shell_canvas_node_min_height(ShellCanvasNodeKind kind) {
  switch (kind) {
  case SHELL_NODE_LINE:
    return 2.0f;
  case SHELL_NODE_ARROW:
    return 16.0f;
  case SHELL_NODE_POLYGON:
  case SHELL_NODE_STAR:
    return 24.0f;
  case SHELL_NODE_IMAGE:
    return 48.0f;
  case SHELL_NODE_PATH:
    return 8.0f;
  case SHELL_NODE_TEXT:
    return 24.0f;
  case SHELL_NODE_RECTANGLE:
  case SHELL_NODE_ELLIPSE:
    return 24.0f;
  case SHELL_NODE_FRAME:
  default:
    return 96.0f;
  }
}

static float shell_degrees_to_radians(float degrees) {
  return degrees * 0.01745329251994329577f;
}

static float shell_normalize_degrees(float degrees) {
  while (degrees > 180.0f)
    degrees -= 360.0f;
  while (degrees < -180.0f)
    degrees += 360.0f;
  return degrees;
}

static void shell_canvas_node_center_world(const ShellCanvasFrameNode *frame,
                                           float *x, float *y) {
  if (!frame)
    return;
  if (x)
    *x = frame->x + frame->w * 0.5f;
  if (y)
    *y = frame->y + frame->h * 0.5f;
}

static void shell_rotate_point(float px, float py, float cx, float cy,
                               float radians, float *out_x, float *out_y) {
  float dx = px - cx;
  float dy = py - cy;
  float c = cosf(radians);
  float s = sinf(radians);
  if (out_x)
    *out_x = cx + dx * c - dy * s;
  if (out_y)
    *out_y = cy + dx * s + dy * c;
}

static void shell_canvas_node_world_to_transformed(
    const ShellCanvasFrameNode *frame, float px, float py, float *out_x,
    float *out_y) {
  float cx = 0.0f;
  float cy = 0.0f;
  if (!frame) {
    if (out_x)
      *out_x = px;
    if (out_y)
      *out_y = py;
    return;
  }
  shell_canvas_node_center_world(frame, &cx, &cy);
  shell_rotate_point(px, py, cx, cy, shell_degrees_to_radians(frame->rotation_deg),
                     out_x, out_y);
}

static void shell_canvas_node_world_to_local(const ShellCanvasFrameNode *frame,
                                             float world_x, float world_y,
                                             float *local_x, float *local_y) {
  float cx = 0.0f;
  float cy = 0.0f;
  if (!frame) {
    if (local_x)
      *local_x = world_x;
    if (local_y)
      *local_y = world_y;
    return;
  }
  shell_canvas_node_center_world(frame, &cx, &cy);
  shell_rotate_point(world_x, world_y, cx, cy,
                     -shell_degrees_to_radians(frame->rotation_deg), local_x,
                     local_y);
}

static void shell_canvas_node_world_bounds(const ShellCanvasFrameNode *frame,
                                           float *out_x, float *out_y,
                                           float *out_w, float *out_h) {
  float x0 = 0.0f;
  float y0 = 0.0f;
  float x1 = 0.0f;
  float y1 = 0.0f;
  float x2 = 0.0f;
  float y2 = 0.0f;
  float x3 = 0.0f;
  float y3 = 0.0f;
  float min_x = 0.0f;
  float min_y = 0.0f;
  float max_x = 0.0f;
  float max_y = 0.0f;
  if (!frame)
    return;
  shell_canvas_node_world_to_transformed(frame, frame->x, frame->y, &x0, &y0);
  shell_canvas_node_world_to_transformed(frame, frame->x + frame->w, frame->y,
                                         &x1, &y1);
  shell_canvas_node_world_to_transformed(frame, frame->x + frame->w,
                                         frame->y + frame->h, &x2, &y2);
  shell_canvas_node_world_to_transformed(frame, frame->x, frame->y + frame->h,
                                         &x3, &y3);
  min_x = x0;
  min_y = y0;
  max_x = x0;
  max_y = y0;
  if (x1 < min_x)
    min_x = x1;
  if (y1 < min_y)
    min_y = y1;
  if (x1 > max_x)
    max_x = x1;
  if (y1 > max_y)
    max_y = y1;
  if (x2 < min_x)
    min_x = x2;
  if (y2 < min_y)
    min_y = y2;
  if (x2 > max_x)
    max_x = x2;
  if (y2 > max_y)
    max_y = y2;
  if (x3 < min_x)
    min_x = x3;
  if (y3 < min_y)
    min_y = y3;
  if (x3 > max_x)
    max_x = x3;
  if (y3 > max_y)
    max_y = y3;
  if (out_x)
    *out_x = min_x;
  if (out_y)
    *out_y = min_y;
  if (out_w)
    *out_w = max_x - min_x;
  if (out_h)
    *out_h = max_y - min_y;
}

static bool shell_parse_float(const char *text, float *out_value) {
  char *end = NULL;
  float value = 0.0f;
  if (!text || !text[0])
    return false;
  value = strtof(text, &end);
  if (end == text)
    return false;
  if (out_value)
    *out_value = value;
  return true;
}

static bool shell_inspector_hit_input(float mouse_x, float mouse_y, float x,
                                      float y, float w, float h) {
  return shell_point_in_rect(mouse_x, mouse_y, x, y + 14.0f, w, h - 14.0f);
}

static void shell_inspector_sync_from_frame(ShellInspectorState *state,
                                            const ShellCanvasFrameNode *frame,
                                            int selection_count,
                                            bool force_sync) {
  if (!state || !frame)
    return;
  if (!state->initialized || force_sync ||
      state->last_selected_id != frame->id ||
      state->last_selection_count != selection_count) {
    snprintf(state->x_buf, sizeof(state->x_buf), "%.0f", frame->x);
    snprintf(state->y_buf, sizeof(state->y_buf), "%.0f", frame->y);
    snprintf(state->w_buf, sizeof(state->w_buf), "%.0f", frame->w);
    snprintf(state->h_buf, sizeof(state->h_buf), "%.0f", frame->h);
    snprintf(state->rotation_buf, sizeof(state->rotation_buf), "%.1f",
             frame->rotation_deg);
    snprintf(state->radius_buf, sizeof(state->radius_buf), "%.0f", frame->radius);
    snprintf(state->font_size_buf, sizeof(state->font_size_buf), "%.0f",
             frame->font_size);
    snprintf(state->fill_r_buf, sizeof(state->fill_r_buf), "%.0f",
             shell_color_to_u8(frame->fill_rgba[0]));
    snprintf(state->fill_g_buf, sizeof(state->fill_g_buf), "%.0f",
             shell_color_to_u8(frame->fill_rgba[1]));
    snprintf(state->fill_b_buf, sizeof(state->fill_b_buf), "%.0f",
             shell_color_to_u8(frame->fill_rgba[2]));
    snprintf(state->fill2_r_buf, sizeof(state->fill2_r_buf), "%.0f",
             shell_color_to_u8(frame->fill_alt_rgba[0]));
    snprintf(state->fill2_g_buf, sizeof(state->fill2_g_buf), "%.0f",
             shell_color_to_u8(frame->fill_alt_rgba[1]));
    snprintf(state->fill2_b_buf, sizeof(state->fill2_b_buf), "%.0f",
             shell_color_to_u8(frame->fill_alt_rgba[2]));
    snprintf(state->stroke_r_buf, sizeof(state->stroke_r_buf), "%.0f",
             shell_color_to_u8(frame->stroke_rgba[0]));
    snprintf(state->stroke_g_buf, sizeof(state->stroke_g_buf), "%.0f",
             shell_color_to_u8(frame->stroke_rgba[1]));
    snprintf(state->stroke_b_buf, sizeof(state->stroke_b_buf), "%.0f",
             shell_color_to_u8(frame->stroke_rgba[2]));
    snprintf(state->stroke_width_buf, sizeof(state->stroke_width_buf), "%.1f",
             frame->stroke_width);
    snprintf(state->opacity_buf, sizeof(state->opacity_buf), "%.0f",
             frame->opacity * 100.0f);
    snprintf(state->blend_buf, sizeof(state->blend_buf), "%.2f", frame->blend);
    snprintf(state->radius_tl_buf, sizeof(state->radius_tl_buf), "%.0f",
             frame->corner_radius[0]);
    snprintf(state->radius_tr_buf, sizeof(state->radius_tr_buf), "%.0f",
             frame->corner_radius[1]);
    snprintf(state->radius_br_buf, sizeof(state->radius_br_buf), "%.0f",
             frame->corner_radius[2]);
    snprintf(state->radius_bl_buf, sizeof(state->radius_bl_buf), "%.0f",
             frame->corner_radius[3]);
    snprintf(state->gradient_angle_buf, sizeof(state->gradient_angle_buf), "%.1f",
             frame->fill_angle_deg);
    snprintf(state->effect_r_buf, sizeof(state->effect_r_buf), "%.0f",
             shell_color_to_u8(frame->effect_rgba[0]));
    snprintf(state->effect_g_buf, sizeof(state->effect_g_buf), "%.0f",
             shell_color_to_u8(frame->effect_rgba[1]));
    snprintf(state->effect_b_buf, sizeof(state->effect_b_buf), "%.0f",
             shell_color_to_u8(frame->effect_rgba[2]));
    snprintf(state->effect_radius_buf, sizeof(state->effect_radius_buf), "%.1f",
             frame->effect_radius);
    snprintf(state->effect_spread_buf, sizeof(state->effect_spread_buf), "%.1f",
             frame->effect_spread);
    snprintf(state->effect_offset_x_buf, sizeof(state->effect_offset_x_buf), "%.1f",
             frame->effect_offset_x);
    snprintf(state->effect_offset_y_buf, sizeof(state->effect_offset_y_buf), "%.1f",
             frame->effect_offset_y);
    snprintf(state->effect_intensity_buf,
             sizeof(state->effect_intensity_buf), "%.1f",
             frame->effect_intensity);
    snprintf(state->line_height_buf, sizeof(state->line_height_buf), "%.1f",
             frame->line_height);
    snprintf(state->letter_spacing_buf, sizeof(state->letter_spacing_buf), "%.1f",
             frame->letter_spacing);
    snprintf(state->font_weight_buf, sizeof(state->font_weight_buf), "%u",
             frame->font_weight);
    snprintf(state->text_buf, sizeof(state->text_buf), "%s", frame->text);
    state->last_selected_id = frame->id;
    state->last_selection_count = selection_count;
    state->initialized = true;
  }
}

static bool shell_draw_numeric_input(StygianContext *ctx, StygianFont font,
                                     float x, float y, float w, float h,
                                     const char *label, char *buffer,
                                     int buffer_size, bool active) {
  float input_y = y + 14.0f;
  float input_h = h - 14.0f;
  if (!ctx || !font || !buffer || buffer_size <= 0)
    return false;
  if (active) {
    stygian_rect_rounded(ctx, x - 1.0f, input_y - 1.0f, w + 2.0f,
                         input_h + 2.0f, 0.22f, 0.54f, 0.96f, 0.48f, 7.0f);
  }
  stygian_text(ctx, font, label ? label : "", x + 8.0f, y + 4.0f, 10.0f, 0.57f,
               0.57f, 0.60f, 1.0f);
  return stygian_text_input(ctx, font, x, input_y, w, input_h, buffer,
                            buffer_size);
}

static bool shell_draw_labeled_text_input(StygianContext *ctx, StygianFont font,
                                          float x, float y, float w, float h,
                                          const char *label, char *buffer,
                                          int buffer_size, bool active) {
  float input_y = y + 14.0f;
  float input_h = h - 14.0f;
  if (!ctx || !font || !buffer || buffer_size <= 0)
    return false;
  if (active) {
    stygian_rect_rounded(ctx, x - 1.0f, input_y - 1.0f, w + 2.0f,
                         input_h + 2.0f, 0.22f, 0.54f, 0.96f, 0.48f, 7.0f);
  }
  stygian_text(ctx, font, label ? label : "", x + 8.0f, y + 4.0f, 10.0f, 0.57f,
               0.57f, 0.60f, 1.0f);
  return stygian_text_input(ctx, font, x, input_y, w, input_h, buffer,
                            buffer_size);
}

static bool shell_tool_is_frame_mode(const ShellToolDockState *state) {
  if (!state)
    return false;
  return state->active_tool == 1;
}

static bool shell_tool_is_primitive_mode(const ShellToolDockState *state) {
  if (!state)
    return false;
  return state->active_tool == 5;
}

static bool shell_tool_is_line_mode(const ShellToolDockState *state) {
  if (!state)
    return false;
  return state->active_tool == 2;
}

static bool shell_tool_is_path_mode(const ShellToolDockState *state) {
  if (!state)
    return false;
  return state->active_tool == 3;
}

static bool shell_tool_is_text_mode(const ShellToolDockState *state) {
  if (!state)
    return false;
  return state->active_tool == 4;
}

static bool shell_tool_is_select_mode(const ShellToolDockState *state) {
  if (!state)
    return false;
  return state->active_tool == 0;
}

static void shell_canvas_stage_rect(const ShellLayout *layout,
                                    float left_panel_width,
                                    float right_panel_width,
                                    float *out_x, float *out_y, float *out_w,
                                    float *out_h) {
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
  if (!layout)
    return;
  x = layout->canvas_x + 22.0f;
  y = layout->canvas_y + 18.0f;
  w = layout->canvas_w - 44.0f;
  h = layout->canvas_h - 114.0f;
  if (left_panel_width > 32.0f) {
    x = layout->assets_panel_x + layout->assets_panel_w + 18.0f;
    w = (layout->canvas_x + layout->canvas_w - 22.0f) - x;
  }
  if (right_panel_width > 32.0f) {
    float right_limit = layout->right_panel_x - 18.0f;
    w = right_limit - x;
  }
  if (w < 80.0f)
    w = 80.0f;
  if (h < 80.0f)
    h = 80.0f;
  if (out_x)
    *out_x = x;
  if (out_y)
    *out_y = y;
  if (out_w)
    *out_w = w;
  if (out_h)
    *out_h = h;
}

static void shell_canvas_world_to_screen(const ShellLayout *layout,
                                         float left_panel_width,
                                         float right_panel_width,
                                         const ShellCanvasViewState *view,
                                         float wx, float wy, float *sx,
                                         float *sy) {
  float stage_x = 0.0f;
  float stage_y = 0.0f;
  float stage_w = 0.0f;
  float stage_h = 0.0f;
  float pan_x = 0.0f;
  float pan_y = 0.0f;
  float zoom = 1.0f;
  shell_canvas_stage_rect(layout, left_panel_width, right_panel_width, &stage_x,
                          &stage_y, &stage_w, &stage_h);
  if (view) {
    pan_x = view->pan_x;
    pan_y = view->pan_y;
    zoom = view->zoom;
  }
  if (sx)
    *sx = stage_x + pan_x + wx * zoom;
  if (sy)
    *sy = stage_y + pan_y + wy * zoom;
}

static void shell_canvas_screen_to_world(const ShellLayout *layout,
                                         float left_panel_width,
                                         float right_panel_width,
                                         const ShellCanvasViewState *view,
                                         float sx, float sy, float *wx,
                                         float *wy) {
  float stage_x = 0.0f;
  float stage_y = 0.0f;
  float stage_w = 0.0f;
  float stage_h = 0.0f;
  float pan_x = 0.0f;
  float pan_y = 0.0f;
  float zoom = 1.0f;
  shell_canvas_stage_rect(layout, left_panel_width, right_panel_width, &stage_x,
                          &stage_y, &stage_w, &stage_h);
  if (view) {
    pan_x = view->pan_x;
    pan_y = view->pan_y;
    zoom = view->zoom;
  }
  if (zoom == 0.0f)
    zoom = 1.0f;
  if (wx)
    *wx = (sx - stage_x - pan_x) / zoom;
  if (wy)
    *wy = (sy - stage_y - pan_y) / zoom;
}

static void shell_canvas_frame_screen_rect(const ShellLayout *layout,
                                           float left_panel_width,
                                           float right_panel_width,
                                           const ShellCanvasViewState *view,
                                           const ShellCanvasFrameNode *frame,
                                           float *x, float *y, float *w,
                                           float *h) {
  float sx = 0.0f;
  float sy = 0.0f;
  float zoom = view ? view->zoom : 1.0f;
  if (!frame)
    return;
  shell_canvas_world_to_screen(layout, left_panel_width, right_panel_width, view,
                               frame->x, frame->y, &sx, &sy);
  if (x)
    *x = sx;
  if (y)
    *y = sy;
  if (w)
    *w = frame->w * zoom;
  if (h)
    *h = frame->h * zoom;
}

static void shell_canvas_frame_screen_bounds(const ShellLayout *layout,
                                             float left_panel_width,
                                             float right_panel_width,
                                             const ShellCanvasViewState *view,
                                             const ShellCanvasFrameNode *frame,
                                             float *x, float *y, float *w,
                                             float *h) {
  float world_x = 0.0f;
  float world_y = 0.0f;
  float world_w = 0.0f;
  float world_h = 0.0f;
  float sx0 = 0.0f;
  float sy0 = 0.0f;
  float sx1 = 0.0f;
  float sy1 = 0.0f;
  if (!frame)
    return;
  shell_canvas_node_world_bounds(frame, &world_x, &world_y, &world_w, &world_h);
  shell_canvas_world_to_screen(layout, left_panel_width, right_panel_width, view,
                               world_x, world_y, &sx0, &sy0);
  shell_canvas_world_to_screen(layout, left_panel_width, right_panel_width, view,
                               world_x + world_w, world_y + world_h, &sx1, &sy1);
  if (x)
    *x = sx0;
  if (y)
    *y = sy0;
  if (w)
    *w = sx1 - sx0;
  if (h)
    *h = sy1 - sy0;
}

static bool shell_canvas_push_screen_rotation(StygianContext *ctx,
                                              const ShellCanvasFrameNode *frame) {
  float cx = 0.0f;
  float cy = 0.0f;
  if (!ctx || !frame || fabsf(frame->rotation_deg) < 0.01f)
    return false;
  cx = frame->x + frame->w * 0.5f;
  cy = frame->y + frame->h * 0.5f;
  stygian_transform_push(ctx);
  stygian_transform_translate(ctx, cx, cy);
  stygian_transform_rotate(ctx, shell_degrees_to_radians(frame->rotation_deg));
  stygian_transform_translate(ctx, -cx, -cy);
  return true;
}

static void shell_canvas_path_point_world(const ShellCanvasFrameNode *frame,
                                          int point_index, float *x,
                                          float *y) {
  float point_x = 0.0f;
  float point_y = 0.0f;
  if (!frame || frame->kind != SHELL_NODE_PATH || point_index < 0 ||
      point_index >= frame->point_count)
    return;
  point_x = frame->x + frame->path_x[point_index] * frame->w;
  point_y = frame->y + frame->path_y[point_index] * frame->h;
  shell_canvas_node_world_to_transformed(frame, point_x, point_y, x, y);
}

static void shell_canvas_path_point_screen(const ShellLayout *layout,
                                           float left_panel_width,
                                           float right_panel_width,
                                           const ShellCanvasViewState *view,
                                           const ShellCanvasFrameNode *frame,
                                           int point_index, float *x,
                                           float *y) {
  float world_x = 0.0f;
  float world_y = 0.0f;
  shell_canvas_path_point_world(frame, point_index, &world_x, &world_y);
  shell_canvas_world_to_screen(layout, left_panel_width, right_panel_width, view,
                               world_x, world_y, x, y);
}

static void shell_canvas_scene_init(ShellCanvasSceneState *scene) {
  if (!scene)
    return;
  memset(scene, 0, sizeof(*scene));
  scene->selected_frame = -1;
  scene->active_page_id = 1;
  scene->next_frame_id = 1;
  scene->next_page_id = 2;
  scene->page_count = 1u;
  scene->pages[0].alive = true;
  scene->pages[0].id = 1;
  snprintf(scene->pages[0].name, sizeof(scene->pages[0].name), "%s", "Page 1");
}

static int shell_canvas_scene_find_page_index(const ShellCanvasSceneState *scene,
                                              int page_id) {
  uint32_t i = 0u;
  if (!scene || page_id <= 0)
    return -1;
  for (i = 0u; i < scene->page_count; ++i) {
    if (scene->pages[i].alive && scene->pages[i].id == page_id)
      return (int)i;
  }
  return -1;
}

static int shell_canvas_scene_active_page_index(const ShellCanvasSceneState *scene) {
  if (!scene)
    return -1;
  return shell_canvas_scene_find_page_index(scene, scene->active_page_id);
}

static ShellCanvasPage *shell_canvas_scene_active_page(ShellCanvasSceneState *scene) {
  int index = shell_canvas_scene_active_page_index(scene);
  if (!scene || index < 0)
    return NULL;
  return &scene->pages[index];
}

static const ShellCanvasPage *
shell_canvas_scene_active_page_const(const ShellCanvasSceneState *scene) {
  int index = shell_canvas_scene_active_page_index(scene);
  if (!scene || index < 0)
    return NULL;
  return &scene->pages[index];
}

static ShellCanvasFrameNode *
shell_canvas_scene_find_node_by_id(ShellCanvasSceneState *scene, int node_id) {
  uint32_t i = 0u;
  if (!scene || node_id <= 0)
    return NULL;
  for (i = 0u; i < scene->frame_count; ++i) {
    if (scene->frames[i].alive && scene->frames[i].id == node_id)
      return &scene->frames[i];
  }
  return NULL;
}

static const ShellCanvasFrameNode *
shell_canvas_scene_find_node_by_id_const(const ShellCanvasSceneState *scene,
                                         int node_id) {
  return shell_canvas_scene_find_node_by_id((ShellCanvasSceneState *)scene,
                                            node_id);
}

static bool shell_canvas_scene_node_in_active_page(
    const ShellCanvasSceneState *scene, const ShellCanvasFrameNode *node) {
  if (!scene || !node || !node->alive)
    return false;
  return node->page_id == scene->active_page_id;
}

static bool shell_canvas_scene_node_effectively_visible(
    const ShellCanvasSceneState *scene, const ShellCanvasFrameNode *node) {
  const ShellCanvasFrameNode *parent = NULL;
  if (!scene || !node || !node->alive || !node->visible ||
      !shell_canvas_scene_node_in_active_page(scene, node)) {
    return false;
  }
  if (node->parent_id <= 0)
    return true;
  parent = shell_canvas_scene_find_node_by_id_const(scene, node->parent_id);
  if (!parent)
    return true;
  return shell_canvas_scene_node_effectively_visible(scene, parent);
}

static bool shell_canvas_scene_node_effectively_locked(
    const ShellCanvasSceneState *scene, const ShellCanvasFrameNode *node) {
  const ShellCanvasFrameNode *parent = NULL;
  if (!scene || !node || !node->alive)
    return false;
  if (node->locked)
    return true;
  if (node->parent_id <= 0)
    return false;
  parent = shell_canvas_scene_find_node_by_id_const(scene, node->parent_id);
  if (!parent)
    return false;
  return shell_canvas_scene_node_effectively_locked(scene, parent);
}

static void shell_canvas_scene_clear_selection_for_hidden(
    ShellCanvasSceneState *scene) {
  uint32_t i = 0u;
  int fallback = -1;
  if (!scene)
    return;
  for (i = 0u; i < scene->frame_count; ++i) {
    ShellCanvasFrameNode *node = &scene->frames[i];
    if (!node->alive)
      continue;
    if (node->selected &&
        (!shell_canvas_scene_node_effectively_visible(scene, node) ||
         !shell_canvas_scene_node_in_active_page(scene, node))) {
      node->selected = false;
      node->selected_point = -1;
    }
    if (node->selected)
      fallback = (int)i;
  }
  scene->selected_frame = fallback;
}

static bool shell_canvas_scene_is_descendant(const ShellCanvasSceneState *scene,
                                             int node_id, int parent_id) {
  const ShellCanvasFrameNode *node = NULL;
  int guard = 0;
  if (!scene || node_id <= 0 || parent_id <= 0)
    return false;
  node = shell_canvas_scene_find_node_by_id_const(scene, node_id);
  while (node && node->parent_id > 0 && guard++ < SHELL_CANVAS_FRAME_CAP) {
    if (node->parent_id == parent_id)
      return true;
    node = shell_canvas_scene_find_node_by_id_const(scene, node->parent_id);
  }
  return false;
}

static void shell_canvas_scene_assign_parent(ShellCanvasSceneState *scene,
                                             ShellCanvasFrameNode *node,
                                             int parent_id) {
  const ShellCanvasFrameNode *parent = NULL;
  if (!scene || !node) {
    return;
  }
  if (parent_id == node->id ||
      shell_canvas_scene_is_descendant(scene, parent_id, node->id)) {
    return;
  }
  if (parent_id > 0) {
    parent = shell_canvas_scene_find_node_by_id_const(scene, parent_id);
    if (!parent || parent->kind != SHELL_NODE_FRAME ||
        parent->page_id != node->page_id) {
      parent_id = 0;
    }
  }
  node->parent_id = parent_id;
}

static int shell_canvas_scene_pick_parent_id(const ShellCanvasSceneState *scene,
                                             const ShellCanvasFrameNode *node) {
  uint32_t i = 0u;
  float center_x = 0.0f;
  float center_y = 0.0f;
  float best_area = 0.0f;
  int best_id = 0;
  if (!scene || !node || node->kind == SHELL_NODE_FRAME)
    return 0;
  center_x = node->x + node->w * 0.5f;
  center_y = node->y + node->h * 0.5f;
  for (i = 0u; i < scene->frame_count; ++i) {
    const ShellCanvasFrameNode *candidate = &scene->frames[i];
    float local_x = 0.0f;
    float local_y = 0.0f;
    float area = 0.0f;
    if (!candidate->alive || candidate->id == node->id ||
        candidate->kind != SHELL_NODE_FRAME || candidate->page_id != node->page_id ||
        !candidate->visible) {
      continue;
    }
    shell_canvas_node_world_to_local(candidate, center_x, center_y, &local_x,
                                     &local_y);
    if (!shell_point_in_rect(local_x, local_y, candidate->x, candidate->y,
                             candidate->w, candidate->h)) {
      continue;
    }
    area = candidate->w * candidate->h;
    if (best_id == 0 || area < best_area) {
      best_id = candidate->id;
      best_area = area;
    }
  }
  return best_id;
}

static void shell_canvas_scene_move_children(ShellCanvasSceneState *scene,
                                             int parent_id, float dx, float dy) {
  uint32_t i = 0u;
  if (!scene || parent_id <= 0 || (dx == 0.0f && dy == 0.0f))
    return;
  for (i = 0u; i < scene->frame_count; ++i) {
    ShellCanvasFrameNode *node = &scene->frames[i];
    if (!node->alive || node->parent_id != parent_id)
      continue;
    node->x += dx;
    node->y += dy;
    shell_canvas_scene_move_children(scene, node->id, dx, dy);
  }
}

static bool shell_canvas_scene_add_page(ShellCanvasSceneState *scene,
                                        const char *name) {
  ShellCanvasPage *page = NULL;
  if (!scene || scene->page_count >= SHELL_PAGE_CAP)
    return false;
  page = &scene->pages[scene->page_count++];
  memset(page, 0, sizeof(*page));
  page->alive = true;
  page->id = scene->next_page_id++;
  snprintf(page->name, sizeof(page->name), "%s",
           (name && name[0]) ? name : "Page");
  scene->active_page_id = page->id;
  shell_canvas_scene_clear_selection(scene);
  return true;
}

static bool shell_canvas_scene_activate_page(ShellCanvasSceneState *scene,
                                             int page_id) {
  if (!scene || shell_canvas_scene_find_page_index(scene, page_id) < 0)
    return false;
  scene->active_page_id = page_id;
  shell_canvas_scene_clear_selection(scene);
  return true;
}

static bool shell_canvas_scene_delete_active_page(StygianContext *ctx,
                                                  ShellCanvasSceneState *scene) {
  uint32_t page_index = 0u;
  uint32_t i = 0u;
  uint32_t dst = 0u;
  if (!scene || scene->page_count <= 1u)
    return false;
  page_index = (uint32_t)shell_canvas_scene_active_page_index(scene);
  for (i = 0u; i < scene->frame_count; ++i) {
    if (!scene->frames[i].alive || scene->frames[i].page_id != scene->active_page_id)
      continue;
    shell_canvas_node_release(ctx, &scene->frames[i]);
  }
  for (i = 0u; i < scene->frame_count; ++i) {
    if (!scene->frames[i].alive || scene->frames[i].page_id == scene->active_page_id)
      continue;
    if (dst != i)
      scene->frames[dst] = scene->frames[i];
    dst++;
  }
  scene->frame_count = dst;
  for (i = dst; i < SHELL_CANVAS_FRAME_CAP; ++i)
    memset(&scene->frames[i], 0, sizeof(scene->frames[i]));
  for (i = page_index + 1u; i < scene->page_count; ++i)
    scene->pages[i - 1u] = scene->pages[i];
  scene->page_count--;
  if (page_index >= scene->page_count)
    page_index = scene->page_count - 1u;
  scene->active_page_id = scene->pages[page_index].id;
  shell_canvas_scene_clear_selection(scene);
  return true;
}

static int shell_canvas_scene_node_depth(const ShellCanvasSceneState *scene,
                                         const ShellCanvasFrameNode *node) {
  int depth = 0;
  int guard = 0;
  if (!scene || !node)
    return 0;
  while (node->parent_id > 0 && guard++ < SHELL_CANVAS_FRAME_CAP) {
    node = shell_canvas_scene_find_node_by_id_const(scene, node->parent_id);
    if (!node)
      break;
    depth++;
  }
  return depth;
}

static void shell_canvas_scene_cycle_parent(ShellCanvasSceneState *scene,
                                            ShellCanvasFrameNode *node) {
  int current = 0;
  uint32_t i = 0u;
  bool found_current = false;
  if (!scene || !node)
    return;
  current = node->parent_id;
  for (i = 0u; i < scene->frame_count; ++i) {
    ShellCanvasFrameNode *candidate = &scene->frames[i];
    if (!candidate->alive || candidate->kind != SHELL_NODE_FRAME ||
        candidate->page_id != node->page_id || candidate->id == node->id ||
        shell_canvas_scene_is_descendant(scene, candidate->id, node->id)) {
      continue;
    }
    if (current == 0) {
      shell_canvas_scene_assign_parent(scene, node, candidate->id);
      return;
    }
    if (found_current) {
      shell_canvas_scene_assign_parent(scene, node, candidate->id);
      return;
    }
    if (candidate->id == current)
      found_current = true;
  }
  shell_canvas_scene_assign_parent(scene, node, 0);
}

static bool shell_canvas_scene_reorder_selected(ShellCanvasSceneState *scene,
                                                int direction) {
  int i = 0;
  bool changed = false;
  if (!scene || direction == 0)
    return false;
  if (direction > 0) {
    for (i = (int)scene->frame_count - 2; i >= 0; --i) {
      if (!scene->frames[i].alive || !scene->frames[i].selected ||
          !shell_canvas_scene_node_in_active_page(scene, &scene->frames[i])) {
        continue;
      }
      if (!scene->frames[i + 1].alive ||
          !shell_canvas_scene_node_in_active_page(scene, &scene->frames[i + 1])) {
        continue;
      }
      {
        ShellCanvasFrameNode tmp = scene->frames[i];
        scene->frames[i] = scene->frames[i + 1];
        scene->frames[i + 1] = tmp;
        if (scene->selected_frame == i)
          scene->selected_frame = i + 1;
        else if (scene->selected_frame == i + 1)
          scene->selected_frame = i;
        changed = true;
      }
    }
  } else {
    for (i = 1; i < (int)scene->frame_count; ++i) {
      if (!scene->frames[i].alive || !scene->frames[i].selected ||
          !shell_canvas_scene_node_in_active_page(scene, &scene->frames[i])) {
        continue;
      }
      if (!scene->frames[i - 1].alive ||
          !shell_canvas_scene_node_in_active_page(scene, &scene->frames[i - 1])) {
        continue;
      }
      {
        ShellCanvasFrameNode tmp = scene->frames[i];
        scene->frames[i] = scene->frames[i - 1];
        scene->frames[i - 1] = tmp;
        if (scene->selected_frame == i)
          scene->selected_frame = i - 1;
        else if (scene->selected_frame == i - 1)
          scene->selected_frame = i;
        changed = true;
      }
    }
  }
  return changed;
}

static bool shell_canvas_scene_bring_selected_to_front(
    ShellCanvasSceneState *scene) {
  bool changed = false;
  if (!scene)
    return false;
  while (shell_canvas_scene_reorder_selected(scene, 1))
    changed = true;
  return changed;
}

static bool shell_canvas_scene_send_selected_to_back(
    ShellCanvasSceneState *scene) {
  bool changed = false;
  if (!scene)
    return false;
  while (shell_canvas_scene_reorder_selected(scene, -1))
    changed = true;
  return changed;
}

static const char *shell_asset_kind_label(ShellAssetKind kind) {
  switch (kind) {
  case SHELL_ASSET_IMAGE:
    return "Image";
  case SHELL_ASSET_FONT:
    return "Font";
  case SHELL_ASSET_TEXTURE:
    return "Texture";
  default:
    return "Asset";
  }
}

static void shell_asset_library_add(ShellAssetLibrary *library,
                                    ShellDialogKind dialog_kind,
                                    const char *path) {
  ShellAssetKind kind = SHELL_ASSET_IMAGE;
  uint32_t i = 0u;
  ShellAssetEntry *entry = NULL;
  if (!library || !path || !path[0])
    return;
  if (dialog_kind == SHELL_DIALOG_IMPORT_FONT) {
    kind = SHELL_ASSET_FONT;
  } else if (dialog_kind == SHELL_DIALOG_IMPORT_TEXTURE) {
    kind = SHELL_ASSET_TEXTURE;
  } else {
    kind = SHELL_ASSET_IMAGE;
  }
  for (i = 0u; i < library->entry_count; ++i) {
    entry = &library->entries[i];
    if (entry->alive && entry->kind == kind && strcmp(entry->path, path) == 0)
      break;
  }
  if (i >= library->entry_count) {
    if (library->entry_count >= SHELL_ASSET_CAP)
      return;
    entry = &library->entries[library->entry_count++];
    memset(entry, 0, sizeof(*entry));
    entry->alive = true;
    entry->id = ++library->next_asset_id;
    entry->kind = kind;
    snprintf(entry->path, sizeof(entry->path), "%s", path);
    stygian_fs_path_filename(path, entry->label, sizeof(entry->label));
    if (!entry->label[0])
      snprintf(entry->label, sizeof(entry->label), "%s", path);
  }
  if (kind == SHELL_ASSET_FONT)
    library->active_font_id = entry->id;
  else if (kind == SHELL_ASSET_TEXTURE)
    library->active_texture_id = entry->id;
  else
    library->active_image_id = entry->id;
}

static const ShellAssetEntry *
shell_asset_library_active_for_dialog(const ShellAssetLibrary *library,
                                      ShellDialogKind dialog_kind) {
  int wanted_id = 0;
  uint32_t i = 0u;
  if (!library)
    return NULL;
  if (dialog_kind == SHELL_DIALOG_IMPORT_FONT)
    wanted_id = library->active_font_id;
  else if (dialog_kind == SHELL_DIALOG_IMPORT_TEXTURE)
    wanted_id = library->active_texture_id;
  else
    wanted_id = library->active_image_id;
  for (i = 0u; i < library->entry_count; ++i) {
    if (library->entries[i].alive && library->entries[i].id == wanted_id)
      return &library->entries[i];
  }
  return NULL;
}

static int shell_canvas_path_hit_point(const ShellLayout *layout,
                                       float left_panel_width,
                                       float right_panel_width,
                                       const ShellCanvasViewState *view,
                                       const ShellCanvasFrameNode *frame,
                                       float mouse_x, float mouse_y,
                                       float radius_px) {
  int i = 0;
  if (!frame || frame->kind != SHELL_NODE_PATH)
    return -1;
  for (i = frame->point_count - 1; i >= 0; --i) {
    float px = 0.0f;
    float py = 0.0f;
    shell_canvas_path_point_screen(layout, left_panel_width, right_panel_width,
                                   view, frame, i, &px, &py);
    if (fabsf(mouse_x - px) <= radius_px && fabsf(mouse_y - py) <= radius_px)
      return i;
  }
  return -1;
}

static ShellCanvasFrameNode *shell_canvas_scene_create_path(
    ShellCanvasSceneState *scene, const float *points_x, const float *points_y,
    int point_count, bool closed) {
  float min_x = 0.0f;
  float min_y = 0.0f;
  float max_x = 0.0f;
  float max_y = 0.0f;
  int i = 0;
  ShellCanvasFrameNode *frame = NULL;
  if (!scene || !points_x || !points_y || point_count < 2 ||
      point_count > SHELL_PATH_POINT_CAP ||
      scene->frame_count >= SHELL_CANVAS_FRAME_CAP)
    return NULL;
  min_x = max_x = points_x[0];
  min_y = max_y = points_y[0];
  for (i = 1; i < point_count; ++i) {
    if (points_x[i] < min_x)
      min_x = points_x[i];
    if (points_y[i] < min_y)
      min_y = points_y[i];
    if (points_x[i] > max_x)
      max_x = points_x[i];
    if (points_y[i] > max_y)
      max_y = points_y[i];
  }
  if (max_x - min_x < 8.0f)
    max_x = min_x + 8.0f;
  if (max_y - min_y < 8.0f)
    max_y = min_y + 8.0f;
  frame = &scene->frames[scene->frame_count++];
  memset(frame, 0, sizeof(*frame));
  frame->alive = true;
  frame->selected = true;
  frame->visible = true;
  frame->locked = false;
  frame->id = scene->next_frame_id++;
  frame->page_id = scene->active_page_id;
  frame->parent_id = 0;
  frame->kind = SHELL_NODE_PATH;
  frame->x = min_x;
  frame->y = min_y;
  frame->w = max_x - min_x;
  frame->h = max_y - min_y;
  frame->path_closed = closed;
  frame->point_count = point_count;
  frame->selected_point = -1;
  for (i = 0; i < point_count; ++i) {
    frame->path_x[i] = (points_x[i] - min_x) / frame->w;
    frame->path_y[i] = (points_y[i] - min_y) / frame->h;
  }
  frame->parent_id = shell_canvas_scene_pick_parent_id(scene, frame);
  snprintf(frame->name, sizeof(frame->name), "Path %d", frame->id);
  for (i = 0; i < (int)scene->frame_count; ++i) {
    scene->frames[i].selected = (scene->frames[i].alive &&
                                 i == (int)scene->frame_count - 1);
    if (i != (int)scene->frame_count - 1)
      scene->frames[i].selected_point = -1;
  }
  scene->selected_frame = (int)scene->frame_count - 1;
  return frame;
}

static bool shell_canvas_path_delete_selected_point(
    ShellCanvasFrameNode *frame) {
  int i = 0;
  if (!frame || frame->kind != SHELL_NODE_PATH || frame->selected_point < 0 ||
      frame->selected_point >= frame->point_count || frame->point_count <= 2)
    return false;
  for (i = frame->selected_point + 1; i < frame->point_count; ++i) {
    frame->path_x[i - 1] = frame->path_x[i];
    frame->path_y[i - 1] = frame->path_y[i];
  }
  frame->point_count--;
  if (frame->selected_point >= frame->point_count)
    frame->selected_point = frame->point_count - 1;
  return true;
}

static int shell_canvas_scene_hit_frame(const ShellCanvasSceneState *scene,
                                        const ShellLayout *layout,
                                        float left_panel_width,
                                        float right_panel_width,
                                        const ShellCanvasViewState *view,
                                        float x, float y) {
  int i;
  float world_x = 0.0f;
  float world_y = 0.0f;
  if (!scene)
    return -1;
  shell_canvas_screen_to_world(layout, left_panel_width, right_panel_width, view,
                               x, y, &world_x, &world_y);
  for (i = (int)scene->frame_count - 1; i >= 0; --i) {
    if (scene->frames[i].alive &&
        shell_canvas_scene_node_effectively_visible(scene, &scene->frames[i]) &&
        !shell_canvas_scene_node_effectively_locked(scene, &scene->frames[i])) {
      float local_x = 0.0f;
      float local_y = 0.0f;
      shell_canvas_node_world_to_local(&scene->frames[i], world_x, world_y,
                                       &local_x, &local_y);
      if (shell_point_in_rect(local_x, local_y, scene->frames[i].x,
                              scene->frames[i].y, scene->frames[i].w,
                              scene->frames[i].h))
        return i;
    }
  }
  return -1;
}

static void shell_canvas_scene_clear_selection(ShellCanvasSceneState *scene) {
  uint32_t i = 0u;
  if (!scene)
    return;
  for (i = 0u; i < scene->frame_count; ++i) {
    scene->frames[i].selected = false;
    scene->frames[i].selected_point = -1;
  }
  scene->selected_frame = -1;
}

static int shell_canvas_scene_selection_count(const ShellCanvasSceneState *scene) {
  int count = 0;
  uint32_t i = 0u;
  if (!scene)
    return 0;
  for (i = 0u; i < scene->frame_count; ++i) {
    if (scene->frames[i].alive && scene->frames[i].selected &&
        shell_canvas_scene_node_in_active_page(scene, &scene->frames[i]))
      count++;
  }
  return count;
}

static void shell_canvas_scene_select_only(ShellCanvasSceneState *scene, int index) {
  uint32_t i = 0u;
  if (!scene)
    return;
  for (i = 0u; i < scene->frame_count; ++i) {
    scene->frames[i].selected = ((int)i == index) && scene->frames[i].alive;
    if ((int)i != index)
      scene->frames[i].selected_point = -1;
  }
  scene->selected_frame =
      (index >= 0 && index < (int)scene->frame_count && scene->frames[index].alive)
          ? index
          : -1;
}

static void shell_canvas_scene_toggle_selection(ShellCanvasSceneState *scene,
                                                int index) {
  if (!scene || index < 0 || index >= (int)scene->frame_count ||
      !scene->frames[index].alive)
    return;
  scene->frames[index].selected = !scene->frames[index].selected;
  if (scene->frames[index].selected) {
    scene->selected_frame = index;
  } else if (scene->selected_frame == index) {
    scene->frames[index].selected_point = -1;
    int i = 0;
    scene->selected_frame = -1;
    for (i = (int)scene->frame_count - 1; i >= 0; --i) {
      if (scene->frames[i].alive && scene->frames[i].selected) {
        scene->selected_frame = i;
        break;
      }
    }
  }
}

static bool shell_canvas_scene_selection_bounds(const ShellCanvasSceneState *scene,
                                                float *out_x, float *out_y,
                                                float *out_w, float *out_h) {
  float min_x = 0.0f;
  float min_y = 0.0f;
  float max_x = 0.0f;
  float max_y = 0.0f;
  bool found = false;
  uint32_t i = 0u;
  if (!scene)
    return false;
  for (i = 0u; i < scene->frame_count; ++i) {
    const ShellCanvasFrameNode *frame = &scene->frames[i];
    float frame_x = 0.0f;
    float frame_y = 0.0f;
    float frame_w = 0.0f;
    float frame_h = 0.0f;
    if (!frame->alive || !frame->selected ||
        !shell_canvas_scene_node_in_active_page(scene, frame) ||
        !shell_canvas_scene_node_effectively_visible(scene, frame))
      continue;
    shell_canvas_node_world_bounds(frame, &frame_x, &frame_y, &frame_w, &frame_h);
    if (!found) {
      min_x = frame_x;
      min_y = frame_y;
      max_x = frame_x + frame_w;
      max_y = frame_y + frame_h;
      found = true;
    } else {
      if (frame_x < min_x)
        min_x = frame_x;
      if (frame_y < min_y)
        min_y = frame_y;
      if (frame_x + frame_w > max_x)
        max_x = frame_x + frame_w;
      if (frame_y + frame_h > max_y)
        max_y = frame_y + frame_h;
    }
  }
  if (!found)
    return false;
  if (out_x)
    *out_x = min_x;
  if (out_y)
    *out_y = min_y;
  if (out_w)
    *out_w = max_x - min_x;
  if (out_h)
    *out_h = max_y - min_y;
  return true;
}

static bool shell_canvas_rects_intersect(float ax, float ay, float aw, float ah,
                                         float bx, float by, float bw, float bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static int shell_canvas_scene_collect_selected(const ShellCanvasSceneState *scene,
                                               int *out_indices,
                                               int max_indices) {
  int count = 0;
  uint32_t i = 0u;
  if (!scene || !out_indices || max_indices <= 0)
    return 0;
  for (i = 0u; i < scene->frame_count && count < max_indices; ++i) {
    if (scene->frames[i].alive && scene->frames[i].selected &&
        shell_canvas_scene_node_in_active_page(scene, &scene->frames[i]))
      out_indices[count++] = (int)i;
  }
  return count;
}

static void shell_canvas_scene_select_all(ShellCanvasSceneState *scene) {
  uint32_t i = 0u;
  if (!scene)
    return;
  for (i = 0u; i < scene->frame_count; ++i) {
    if (scene->frames[i].alive &&
        shell_canvas_scene_node_in_active_page(scene, &scene->frames[i]) &&
        shell_canvas_scene_node_effectively_visible(scene, &scene->frames[i]) &&
        !shell_canvas_scene_node_effectively_locked(scene, &scene->frames[i])) {
      scene->frames[i].selected = true;
      scene->frames[i].selected_point = -1;
      scene->selected_frame = (int)i;
    }
  }
}

static void shell_canvas_scene_make_snapshot(const ShellCanvasSceneState *scene,
                                             ShellCanvasSceneState *out) {
  uint32_t i = 0u;
  if (!scene || !out)
    return;
  *out = *scene;
  for (i = 0u; i < SHELL_CANVAS_FRAME_CAP; ++i)
    out->frames[i].image_texture = 0u;
}

static bool shell_canvas_scene_snapshot_equals(const ShellCanvasSceneState *a,
                                               const ShellCanvasSceneState *b) {
  ShellCanvasSceneState sa = {0};
  ShellCanvasSceneState sb = {0};
  if (!a || !b)
    return false;
  shell_canvas_scene_make_snapshot(a, &sa);
  shell_canvas_scene_make_snapshot(b, &sb);
  return memcmp(&sa, &sb, sizeof(sa)) == 0;
}

static void shell_canvas_scene_restore(StygianContext *ctx,
                                       ShellCanvasSceneState *scene,
                                       const ShellCanvasSceneState *snapshot) {
  if (!scene || !snapshot)
    return;
  shell_canvas_scene_release(ctx, scene);
  *scene = *snapshot;
}

static void shell_canvas_history_init(ShellCanvasHistoryState *history,
                                      const ShellCanvasSceneState *scene) {
  if (!history || !scene)
    return;
  memset(history, 0, sizeof(*history));
  shell_canvas_scene_make_snapshot(scene, &history->undo[0]);
  history->undo_count = 1;
}

static bool shell_canvas_history_push(ShellCanvasHistoryState *history,
                                      const ShellCanvasSceneState *scene) {
  ShellCanvasSceneState snapshot = {0};
  if (!history || !scene)
    return false;
  shell_canvas_scene_make_snapshot(scene, &snapshot);
  if (history->undo_count > 0 &&
      shell_canvas_scene_snapshot_equals(&history->undo[history->undo_count - 1],
                                         &snapshot)) {
    return false;
  }
  if (history->undo_count >= SHELL_HISTORY_CAP) {
    memmove(&history->undo[0], &history->undo[1],
            sizeof(history->undo[0]) * (SHELL_HISTORY_CAP - 1));
    history->undo_count = SHELL_HISTORY_CAP - 1;
  }
  history->undo[history->undo_count++] = snapshot;
  history->redo_count = 0;
  return true;
}

static bool shell_canvas_history_undo(StygianContext *ctx,
                                      ShellCanvasHistoryState *history,
                                      ShellCanvasSceneState *scene) {
  ShellCanvasSceneState current = {0};
  if (!ctx || !history || !scene || history->undo_count <= 1)
    return false;
  shell_canvas_scene_make_snapshot(scene, &current);
  if (history->redo_count >= SHELL_HISTORY_CAP) {
    memmove(&history->redo[0], &history->redo[1],
            sizeof(history->redo[0]) * (SHELL_HISTORY_CAP - 1));
    history->redo_count = SHELL_HISTORY_CAP - 1;
  }
  history->redo[history->redo_count++] = current;
  history->undo_count--;
  shell_canvas_scene_restore(ctx, scene, &history->undo[history->undo_count - 1]);
  return true;
}

static bool shell_canvas_history_redo(StygianContext *ctx,
                                      ShellCanvasHistoryState *history,
                                      ShellCanvasSceneState *scene) {
  ShellCanvasSceneState target = {0};
  if (!ctx || !history || !scene || history->redo_count <= 0)
    return false;
  target = history->redo[history->redo_count - 1];
  history->redo_count--;
  if (history->undo_count >= SHELL_HISTORY_CAP) {
    memmove(&history->undo[0], &history->undo[1],
            sizeof(history->undo[0]) * (SHELL_HISTORY_CAP - 1));
    history->undo_count = SHELL_HISTORY_CAP - 1;
  }
  history->undo[history->undo_count++] = target;
  shell_canvas_scene_restore(ctx, scene, &target);
  return true;
}

static float shell_motion_ease_value(ShellMotionEasing easing, float t) {
  t = shell_clamp01(t);
  switch (easing) {
  case SHELL_MOTION_EASE_OUT_CUBIC: {
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
  }
  case SHELL_MOTION_EASE_IN_OUT_CUBIC:
    if (t < 0.5f)
      return 4.0f * t * t * t;
    return 1.0f - powf(-2.0f * t + 2.0f, 3.0f) * 0.5f;
  case SHELL_MOTION_EASE_LINEAR:
  default:
    return t;
  }
}

static ShellMotionTrack *shell_motion_find_track(ShellMotionState *state,
                                                 int track_id) {
  int i = 0;
  if (!state || track_id <= 0)
    return NULL;
  for (i = 0; i < state->track_count; ++i) {
    if (state->tracks[i].alive && state->tracks[i].id == track_id)
      return &state->tracks[i];
  }
  return NULL;
}

static const ShellMotionTrack *shell_motion_find_track_const(
    const ShellMotionState *state, int track_id) {
  int i = 0;
  if (!state || track_id <= 0)
    return NULL;
  for (i = 0; i < state->track_count; ++i) {
    if (state->tracks[i].alive && state->tracks[i].id == track_id)
      return &state->tracks[i];
  }
  return NULL;
}

static ShellMotionClip *shell_motion_find_clip(ShellMotionState *state,
                                               int clip_id) {
  int i = 0;
  if (!state || clip_id <= 0)
    return NULL;
  for (i = 0; i < state->clip_count; ++i) {
    if (state->clips[i].alive && state->clips[i].id == clip_id)
      return &state->clips[i];
  }
  return NULL;
}

static const ShellMotionClip *shell_motion_find_clip_const(
    const ShellMotionState *state, int clip_id) {
  int i = 0;
  if (!state || clip_id <= 0)
    return NULL;
  for (i = 0; i < state->clip_count; ++i) {
    if (state->clips[i].alive && state->clips[i].id == clip_id)
      return &state->clips[i];
  }
  return NULL;
}

static ShellMotionTrack *shell_motion_alloc_track(ShellMotionState *state) {
  ShellMotionTrack *track = NULL;
  if (!state || state->track_count >= SHELL_MOTION_TRACK_CAP)
    return NULL;
  track = &state->tracks[state->track_count++];
  memset(track, 0, sizeof(*track));
  track->alive = true;
  track->id = ++state->next_track_id;
  track->duration_ms = 240u;
  track->easing = SHELL_MOTION_EASE_OUT_CUBIC;
  return track;
}

static ShellMotionClip *shell_motion_alloc_clip(ShellMotionState *state) {
  ShellMotionClip *clip = NULL;
  if (!state || state->clip_count >= SHELL_MOTION_CLIP_CAP)
    return NULL;
  clip = &state->clips[state->clip_count++];
  memset(clip, 0, sizeof(*clip));
  clip->alive = true;
  clip->id = ++state->next_clip_id;
  clip->duration_ms = 240u;
  snprintf(clip->name, sizeof(clip->name), "Clip %d", clip->id);
  state->selected_clip_id = clip->id;
  return clip;
}

static ShellMotionBehavior *shell_motion_alloc_behavior(
    ShellMotionState *state) {
  ShellMotionBehavior *behavior = NULL;
  if (!state || state->behavior_count >= SHELL_MOTION_BEHAVIOR_CAP)
    return NULL;
  behavior = &state->behaviors[state->behavior_count++];
  memset(behavior, 0, sizeof(*behavior));
  behavior->alive = true;
  behavior->id = ++state->next_behavior_id;
  return behavior;
}

static void shell_motion_clip_recompute_duration(ShellMotionState *state,
                                                 ShellMotionClip *clip) {
  uint32_t duration_ms = 0u;
  int i = 0;
  if (!state || !clip)
    return;
  for (i = 0; i < clip->track_count; ++i) {
    const ShellMotionTrack *track =
        shell_motion_find_track_const(state, clip->track_ids[i]);
    uint32_t end_ms = 0u;
    if (!track)
      continue;
    end_ms = track->start_ms + track->duration_ms;
    if (end_ms > duration_ms)
      duration_ms = end_ms;
  }
  clip->duration_ms = duration_ms > 0u ? duration_ms : 240u;
}

static void shell_motion_capture_defaults_from_node(
    const ShellCanvasFrameNode *node, ShellMotionTrack *track) {
  if (!node || !track)
    return;
  switch (track->property) {
  case SHELL_MOTION_PROP_X:
    track->from_value = node->x;
    track->to_value = node->x + 96.0f;
    break;
  case SHELL_MOTION_PROP_Y:
    track->from_value = node->y;
    track->to_value = node->y + 96.0f;
    break;
  case SHELL_MOTION_PROP_SCALE:
    track->from_value = 1.0f;
    track->to_value = 1.08f;
    break;
  case SHELL_MOTION_PROP_ROTATION:
    track->from_value = node->rotation_deg;
    track->to_value = shell_normalize_degrees(node->rotation_deg + 16.0f);
    break;
  case SHELL_MOTION_PROP_OPACITY:
    track->from_value = node->opacity;
    track->to_value = node->opacity > 0.7f ? 0.28f : 1.0f;
    break;
  case SHELL_MOTION_PROP_COLOR:
    memcpy(track->from_rgba, node->fill_rgba, sizeof(track->from_rgba));
    memcpy(track->to_rgba, node->fill_rgba, sizeof(track->to_rgba));
    track->to_rgba[0] = shell_clampf(node->fill_rgba[0] + 0.14f, 0.0f, 1.0f);
    track->to_rgba[1] = shell_clampf(node->fill_rgba[1] + 0.10f, 0.0f, 1.0f);
    track->to_rgba[2] = shell_clampf(node->fill_rgba[2] + 0.18f, 0.0f, 1.0f);
    break;
  default:
    break;
  }
}

static void shell_motion_sync_fields(ShellMotionState *state) {
  const ShellMotionClip *clip = NULL;
  const ShellMotionTrack *track = NULL;
  if (!state)
    return;
  clip = shell_motion_find_clip_const(state, state->selected_clip_id);
  track = shell_motion_find_track_const(state, state->selected_track_id);
  if (clip && state->last_clip_id != clip->id) {
    snprintf(state->clip_name_buf, sizeof(state->clip_name_buf), "%s", clip->name);
    state->last_clip_id = clip->id;
  }
  if (!clip) {
    state->clip_name_buf[0] = '\0';
    state->last_clip_id = -1;
  }
  if (track && state->last_track_id != track->id) {
    snprintf(state->start_buf, sizeof(state->start_buf), "%u", track->start_ms);
    snprintf(state->duration_buf, sizeof(state->duration_buf), "%u",
             track->duration_ms);
    snprintf(state->from_buf, sizeof(state->from_buf), "%.2f", track->from_value);
    snprintf(state->to_buf, sizeof(state->to_buf), "%.2f", track->to_value);
    snprintf(state->from_r_buf, sizeof(state->from_r_buf), "%.0f",
             shell_color_to_u8(track->from_rgba[0]));
    snprintf(state->from_g_buf, sizeof(state->from_g_buf), "%.0f",
             shell_color_to_u8(track->from_rgba[1]));
    snprintf(state->from_b_buf, sizeof(state->from_b_buf), "%.0f",
             shell_color_to_u8(track->from_rgba[2]));
    snprintf(state->to_r_buf, sizeof(state->to_r_buf), "%.0f",
             shell_color_to_u8(track->to_rgba[0]));
    snprintf(state->to_g_buf, sizeof(state->to_g_buf), "%.0f",
             shell_color_to_u8(track->to_rgba[1]));
    snprintf(state->to_b_buf, sizeof(state->to_b_buf), "%.0f",
             shell_color_to_u8(track->to_rgba[2]));
    state->last_track_id = track->id;
  }
  if (!track) {
    state->start_buf[0] = '\0';
    state->duration_buf[0] = '\0';
    state->from_buf[0] = '\0';
    state->to_buf[0] = '\0';
    state->from_r_buf[0] = '\0';
    state->from_g_buf[0] = '\0';
    state->from_b_buf[0] = '\0';
    state->to_r_buf[0] = '\0';
    state->to_g_buf[0] = '\0';
    state->to_b_buf[0] = '\0';
    state->last_track_id = -1;
  }
}

static ShellMotionTrack *shell_motion_add_track_to_clip(
    ShellMotionState *state, const ShellCanvasFrameNode *node,
    ShellMotionClip *clip, ShellMotionPropertyKind property) {
  ShellMotionTrack *track = NULL;
  if (!state || !node || !clip || clip->track_count >= SHELL_MOTION_CLIP_TRACK_CAP)
    return NULL;
  track = shell_motion_alloc_track(state);
  if (!track)
    return NULL;
  track->target_node_id = node->id;
  track->property = property;
  snprintf(track->name, sizeof(track->name), "%s %s", node->name,
           shell_motion_property_label(property));
  shell_motion_capture_defaults_from_node(node, track);
  clip->track_ids[clip->track_count++] = track->id;
  shell_motion_clip_recompute_duration(state, clip);
  state->selected_track_id = track->id;
  shell_motion_sync_fields(state);
  return track;
}

static bool shell_motion_has_active_instances(const ShellMotionState *state) {
  int i = 0;
  if (!state)
    return false;
  for (i = 0; i < SHELL_MOTION_INSTANCE_CAP; ++i) {
    if (state->instances[i].active)
      return true;
  }
  return false;
}

static bool shell_motion_preview_active(const ShellMotionState *state) {
  return state && (state->playing || state->scrub_active ||
                   state->preview_snapshot_valid ||
                   shell_motion_has_active_instances(state));
}

static void shell_motion_capture_preview_scene(ShellMotionState *state,
                                               const ShellCanvasSceneState *scene) {
  if (!state || !scene || state->preview_snapshot_valid)
    return;
  shell_canvas_scene_make_snapshot(scene, &state->preview_base_scene);
  state->preview_snapshot_valid = true;
}

static void shell_motion_clear_instances(ShellMotionState *state) {
  int i = 0;
  if (!state)
    return;
  for (i = 0; i < SHELL_MOTION_INSTANCE_CAP; ++i)
    state->instances[i].active = false;
}

static void shell_motion_reset_preview(StygianContext *ctx,
                                       ShellMotionState *state,
                                       ShellCanvasSceneState *scene) {
  if (!ctx || !state || !scene)
    return;
  if (state->preview_snapshot_valid)
    shell_canvas_scene_restore(ctx, scene, &state->preview_base_scene);
  state->preview_snapshot_valid = false;
  state->playing = false;
  state->scrub_active = false;
  shell_motion_clear_instances(state);
}

static void shell_motion_apply_track(const ShellMotionTrack *track,
                                     ShellCanvasSceneState *scene,
                                     const ShellCanvasSceneState *base_scene,
                                     uint32_t clip_time_ms, bool reverse) {
  ShellCanvasFrameNode *node = NULL;
  const ShellCanvasFrameNode *base_node = NULL;
  float t = 0.0f;
  float eased = 0.0f;
  if (!track || !scene || !base_scene)
    return;
  node = shell_canvas_scene_find_node_by_id(scene, track->target_node_id);
  base_node = shell_canvas_scene_find_node_by_id_const(base_scene, track->target_node_id);
  if (!node || !base_node)
    return;
  if (clip_time_ms <= track->start_ms) {
    t = 0.0f;
  } else {
    uint32_t elapsed = clip_time_ms - track->start_ms;
    uint32_t duration_ms = track->duration_ms > 0u ? track->duration_ms : 1u;
    t = (float)elapsed / (float)duration_ms;
  }
  t = shell_clamp01(t);
  if (reverse)
    t = 1.0f - t;
  eased = shell_motion_ease_value(track->easing, t);
  switch (track->property) {
  case SHELL_MOTION_PROP_X:
    node->x = track->from_value + (track->to_value - track->from_value) * eased;
    break;
  case SHELL_MOTION_PROP_Y:
    node->y = track->from_value + (track->to_value - track->from_value) * eased;
    break;
  case SHELL_MOTION_PROP_SCALE: {
    float scale = track->from_value + (track->to_value - track->from_value) * eased;
    float cx = base_node->x + base_node->w * 0.5f;
    float cy = base_node->y + base_node->h * 0.5f;
    node->w = shell_clampf(base_node->w * scale,
                           shell_canvas_node_min_width(node->kind), 4096.0f);
    node->h = shell_clampf(base_node->h * scale,
                           shell_canvas_node_min_height(node->kind), 4096.0f);
    node->x = cx - node->w * 0.5f;
    node->y = cy - node->h * 0.5f;
    shell_canvas_clamp_frame_radius(node);
  } break;
  case SHELL_MOTION_PROP_ROTATION:
    node->rotation_deg = shell_normalize_degrees(
        track->from_value + (track->to_value - track->from_value) * eased);
    break;
  case SHELL_MOTION_PROP_OPACITY:
    node->opacity =
        shell_clamp01(track->from_value + (track->to_value - track->from_value) * eased);
    break;
  case SHELL_MOTION_PROP_COLOR:
    node->fill_rgba[0] = shell_clamp01(track->from_rgba[0] +
                                       (track->to_rgba[0] - track->from_rgba[0]) *
                                           eased);
    node->fill_rgba[1] = shell_clamp01(track->from_rgba[1] +
                                       (track->to_rgba[1] - track->from_rgba[1]) *
                                           eased);
    node->fill_rgba[2] = shell_clamp01(track->from_rgba[2] +
                                       (track->to_rgba[2] - track->from_rgba[2]) *
                                           eased);
    node->fill_rgba[3] = shell_clamp01(track->from_rgba[3] +
                                       (track->to_rgba[3] - track->from_rgba[3]) *
                                           eased);
    break;
  default:
    break;
  }
}

static void shell_motion_apply_clip_time(const ShellMotionState *state,
                                         const ShellMotionClip *clip,
                                         ShellCanvasSceneState *scene,
                                         const ShellCanvasSceneState *base_scene,
                                         uint32_t clip_time_ms, bool reverse) {
  int i = 0;
  if (!state || !clip || !scene || !base_scene)
    return;
  for (i = 0; i < clip->track_count; ++i) {
    const ShellMotionTrack *track =
        shell_motion_find_track_const(state, clip->track_ids[i]);
    if (!track)
      continue;
    shell_motion_apply_track(track, scene, base_scene, clip_time_ms, reverse);
  }
}

static void shell_motion_start_clip_instance(ShellMotionState *state, int clip_id,
                                             bool reverse, uint64_t now_ms) {
  int i = 0;
  if (!state || clip_id <= 0)
    return;
  for (i = 0; i < SHELL_MOTION_INSTANCE_CAP; ++i) {
    if (!state->instances[i].active) {
      state->instances[i].active = true;
      state->instances[i].clip_id = clip_id;
      state->instances[i].started_ms = now_ms;
      state->instances[i].reverse = reverse;
      return;
    }
  }
}

static void shell_motion_trigger_event(ShellMotionState *state,
                                       ShellCanvasSceneState *scene,
                                       int trigger_node_id,
                                       ShellMotionEventKind event_kind,
                                       uint64_t now_ms) {
  int i = 0;
  if (!state || !scene || trigger_node_id <= 0)
    return;
  shell_motion_capture_preview_scene(state, scene);
  for (i = 0; i < state->behavior_count; ++i) {
    ShellMotionBehavior *behavior = &state->behaviors[i];
    bool reverse = false;
    if (!behavior->alive || behavior->trigger_node_id != trigger_node_id ||
        behavior->event_kind != event_kind) {
      continue;
    }
    reverse = behavior->reverse;
    if (behavior->toggle) {
      reverse = behavior->toggle_state;
      behavior->toggle_state = !behavior->toggle_state;
    }
    shell_motion_start_clip_instance(state, behavior->clip_id, reverse, now_ms);
  }
}

static void shell_motion_tick(StygianContext *ctx, ShellMotionState *state,
                              ShellCanvasSceneState *scene, uint64_t now_ms) {
  bool any_active = false;
  int i = 0;
  if (!ctx || !state || !scene)
    return;
  if (state->playing || state->scrub_active || shell_motion_has_active_instances(state))
    shell_motion_capture_preview_scene(state, scene);
  if (!state->preview_snapshot_valid)
    return;

  shell_canvas_scene_restore(ctx, scene, &state->preview_base_scene);

  if (state->playing) {
    ShellMotionClip *clip = shell_motion_find_clip(state, state->selected_clip_id);
    uint32_t duration_ms = 240u;
    if (clip) {
      duration_ms = clip->duration_ms > 0u ? clip->duration_ms : 240u;
      state->playhead_ms = (uint32_t)(now_ms - state->play_started_ms);
      if (clip->loop && duration_ms > 0u) {
        state->playhead_ms %= duration_ms;
      } else if (state->playhead_ms >= duration_ms) {
        state->playhead_ms = duration_ms;
        state->playing = false;
      }
      shell_motion_apply_clip_time(state, clip, scene, &state->preview_base_scene,
                                   state->playhead_ms, false);
      any_active = true;
    } else {
      state->playing = false;
    }
  } else if (state->scrub_active) {
    ShellMotionClip *clip = shell_motion_find_clip(state, state->selected_clip_id);
    if (clip) {
      shell_motion_apply_clip_time(state, clip, scene, &state->preview_base_scene,
                                   state->playhead_ms, false);
      any_active = true;
    }
  }

  for (i = 0; i < SHELL_MOTION_INSTANCE_CAP; ++i) {
    ShellMotionInstance *instance = &state->instances[i];
    ShellMotionClip *clip = NULL;
    uint32_t elapsed_ms = 0u;
    uint32_t duration_ms = 0u;
    if (!instance->active)
      continue;
    clip = shell_motion_find_clip(state, instance->clip_id);
    if (!clip) {
      instance->active = false;
      continue;
    }
    duration_ms = clip->duration_ms > 0u ? clip->duration_ms : 240u;
    elapsed_ms = (uint32_t)(now_ms - instance->started_ms);
    if (clip->loop && duration_ms > 0u)
      elapsed_ms %= duration_ms;
    shell_motion_apply_clip_time(state, clip, scene, &state->preview_base_scene,
                                 elapsed_ms, instance->reverse);
    any_active = true;
    if (!clip->loop && elapsed_ms >= duration_ms)
      instance->active = false;
  }

  if (!any_active && !state->playing && !state->scrub_active) {
    shell_canvas_scene_restore(ctx, scene, &state->preview_base_scene);
    state->preview_snapshot_valid = false;
  }
}

static const char *shell_canvas_node_base_name(ShellCanvasNodeKind kind) {
  switch (kind) {
  case SHELL_NODE_RECTANGLE:
    return "Rectangle";
  case SHELL_NODE_ELLIPSE:
    return "Ellipse";
  case SHELL_NODE_LINE:
    return "Line";
  case SHELL_NODE_ARROW:
    return "Arrow";
  case SHELL_NODE_POLYGON:
    return "Polygon";
  case SHELL_NODE_STAR:
    return "Star";
  case SHELL_NODE_IMAGE:
    return "Image";
  case SHELL_NODE_PATH:
    return "Path";
  case SHELL_NODE_TEXT:
    return "Text";
  case SHELL_NODE_FRAME:
  default:
    return "Frame";
  }
}

static int shell_canvas_clipboard_copy_selected(
    ShellCanvasClipboardState *clipboard, const ShellCanvasSceneState *scene) {
  uint32_t i = 0u;
  int count = 0;
  if (!clipboard || !scene)
    return 0;
  memset(clipboard, 0, sizeof(*clipboard));
  for (i = 0u; i < scene->frame_count && count < SHELL_CANVAS_FRAME_CAP; ++i) {
    if (!scene->frames[i].alive || !scene->frames[i].selected ||
        !shell_canvas_scene_node_in_active_page(scene, &scene->frames[i]))
      continue;
    clipboard->nodes[count] = scene->frames[i];
    clipboard->nodes[count].selected = false;
    clipboard->nodes[count].selected_point = -1;
    clipboard->nodes[count].image_texture = 0u;
    count++;
  }
  clipboard->count = count;
  clipboard->paste_serial = 0;
  return count;
}

static bool shell_canvas_clipboard_paste(
    ShellCanvasSceneState *scene, ShellCanvasClipboardState *clipboard) {
  int i = 0;
  float offset = 28.0f;
  if (!scene || !clipboard || clipboard->count <= 0)
    return false;
  if (scene->frame_count >= SHELL_CANVAS_FRAME_CAP)
    return false;
  shell_canvas_scene_clear_selection(scene);
  clipboard->paste_serial++;
  offset *= (float)clipboard->paste_serial;
  for (i = 0; i < clipboard->count && scene->frame_count < SHELL_CANVAS_FRAME_CAP;
       ++i) {
    ShellCanvasFrameNode *dst = &scene->frames[scene->frame_count++];
    *dst = clipboard->nodes[i];
    dst->alive = true;
    dst->selected = true;
    dst->page_id = scene->active_page_id;
    dst->selected_point = -1;
    dst->id = scene->next_frame_id++;
    dst->x += offset;
    dst->y += offset;
    dst->image_texture = 0u;
    if (dst->parent_id > 0 &&
        !shell_canvas_scene_find_node_by_id_const(scene, dst->parent_id)) {
      dst->parent_id = 0;
    }
    snprintf(dst->name, sizeof(dst->name), "%s %d",
             shell_canvas_node_base_name(dst->kind), dst->id);
    scene->selected_frame = (int)scene->frame_count - 1;
  }
  return true;
}

static void shell_canvas_align_selected(ShellCanvasSceneState *scene,
                                        ShellCanvasAlignKind kind) {
  float bounds_x = 0.0f;
  float bounds_y = 0.0f;
  float bounds_w = 0.0f;
  float bounds_h = 0.0f;
  uint32_t i = 0u;
  if (!scene)
    return;
  if (!shell_canvas_scene_selection_bounds(scene, &bounds_x, &bounds_y,
                                           &bounds_w, &bounds_h))
    return;
  for (i = 0u; i < scene->frame_count; ++i) {
    ShellCanvasFrameNode *frame = &scene->frames[i];
    if (!frame->alive || !frame->selected ||
        !shell_canvas_scene_node_in_active_page(scene, frame))
      continue;
    switch (kind) {
    case SHELL_ALIGN_LEFT:
      frame->x = bounds_x;
      break;
    case SHELL_ALIGN_CENTER_X:
      frame->x = bounds_x + (bounds_w - frame->w) * 0.5f;
      break;
    case SHELL_ALIGN_RIGHT:
      frame->x = bounds_x + bounds_w - frame->w;
      break;
    case SHELL_ALIGN_TOP:
      frame->y = bounds_y;
      break;
    case SHELL_ALIGN_CENTER_Y:
      frame->y = bounds_y + (bounds_h - frame->h) * 0.5f;
      break;
    case SHELL_ALIGN_BOTTOM:
      frame->y = bounds_y + bounds_h - frame->h;
      break;
    default:
      break;
    }
  }
}

static void shell_canvas_distribute_selected(ShellCanvasSceneState *scene,
                                             bool horizontal) {
  int indices[SHELL_CANVAS_FRAME_CAP];
  int count = 0;
  int i = 0;
  int j = 0;
  if (!scene)
    return;
  count =
      shell_canvas_scene_collect_selected(scene, indices, SHELL_CANVAS_FRAME_CAP);
  if (count < 3)
    return;
  for (i = 0; i < count - 1; ++i) {
    for (j = i + 1; j < count; ++j) {
      ShellCanvasFrameNode *a = &scene->frames[indices[i]];
      ShellCanvasFrameNode *b = &scene->frames[indices[j]];
      float ca = horizontal ? (a->x + a->w * 0.5f) : (a->y + a->h * 0.5f);
      float cb = horizontal ? (b->x + b->w * 0.5f) : (b->y + b->h * 0.5f);
      if (cb < ca) {
        int tmp = indices[i];
        indices[i] = indices[j];
        indices[j] = tmp;
      }
    }
  }
  {
    ShellCanvasFrameNode *first = &scene->frames[indices[0]];
    ShellCanvasFrameNode *last = &scene->frames[indices[count - 1]];
    float min_center =
        horizontal ? (first->x + first->w * 0.5f) : (first->y + first->h * 0.5f);
    float max_center =
        horizontal ? (last->x + last->w * 0.5f) : (last->y + last->h * 0.5f);
    float spacing = (max_center - min_center) / (float)(count - 1);
    for (i = 0; i < count; ++i) {
      ShellCanvasFrameNode *frame = &scene->frames[indices[i]];
      float target_center = min_center + spacing * (float)i;
      if (horizontal) {
        frame->x = target_center - frame->w * 0.5f;
      } else {
        frame->y = target_center - frame->h * 0.5f;
      }
    }
  }
}

static void shell_canvas_scene_select_in_rect(ShellCanvasSceneState *scene,
                                              float x, float y, float w, float h,
                                              bool additive) {
  uint32_t i = 0u;
  int last_selected = -1;
  int previous_active = -1;
  if (!scene)
    return;
  previous_active = scene->selected_frame;
  if (!additive)
    shell_canvas_scene_clear_selection(scene);
  for (i = 0u; i < scene->frame_count; ++i) {
    ShellCanvasFrameNode *frame = &scene->frames[i];
    float frame_x = 0.0f;
    float frame_y = 0.0f;
    float frame_w = 0.0f;
    float frame_h = 0.0f;
    if (!frame->alive || !shell_canvas_scene_node_in_active_page(scene, frame) ||
        !shell_canvas_scene_node_effectively_visible(scene, frame) ||
        shell_canvas_scene_node_effectively_locked(scene, frame))
      continue;
    shell_canvas_node_world_bounds(frame, &frame_x, &frame_y, &frame_w, &frame_h);
    if (shell_canvas_rects_intersect(x, y, w, h, frame_x, frame_y, frame_w,
                                     frame_h)) {
      frame->selected = true;
      last_selected = (int)i;
    }
  }
  scene->selected_frame =
      last_selected >= 0 ? last_selected : (additive ? previous_active : -1);
}

static ShellCanvasFrameNode *shell_canvas_scene_selected_frame(
    ShellCanvasSceneState *scene) {
  if (!scene || scene->selected_frame < 0 ||
      scene->selected_frame >= (int)scene->frame_count)
    return NULL;
  if (!scene->frames[scene->selected_frame].alive ||
      !scene->frames[scene->selected_frame].selected ||
      !shell_canvas_scene_node_in_active_page(scene,
                                              &scene->frames[scene->selected_frame]))
    return NULL;
  return &scene->frames[scene->selected_frame];
}

static void shell_canvas_scene_nudge_selected(ShellCanvasSceneState *scene,
                                              float dx, float dy) {
  uint32_t i = 0u;
  if (!scene || (dx == 0.0f && dy == 0.0f))
    return;
  for (i = 0u; i < scene->frame_count; ++i) {
    if (!scene->frames[i].alive || !scene->frames[i].selected ||
        !shell_canvas_scene_node_in_active_page(scene, &scene->frames[i]))
      continue;
    scene->frames[i].x += dx;
    scene->frames[i].y += dy;
    shell_canvas_scene_move_children(scene, scene->frames[i].id, dx, dy);
  }
}

static bool shell_canvas_scene_delete_selected(StygianContext *ctx,
                                               ShellCanvasSceneState *scene) {
  uint32_t i = 0u;
  uint32_t dst = 0u;
  bool removed = false;
  if (!scene)
    return false;
  for (i = 0u; i < scene->frame_count; ++i) {
    ShellCanvasFrameNode frame = scene->frames[i];
    if (!frame.alive ||
        (frame.selected &&
         shell_canvas_scene_node_in_active_page(scene, &frame))) {
      if (frame.alive && frame.selected &&
          shell_canvas_scene_node_in_active_page(scene, &frame))
        shell_canvas_node_release(ctx, &frame);
      removed = removed || (frame.alive &&
                            shell_canvas_scene_node_in_active_page(scene, &frame));
      continue;
    }
    if (dst != i)
      scene->frames[dst] = frame;
    dst++;
  }
  scene->frame_count = dst;
  for (i = dst; i < SHELL_CANVAS_FRAME_CAP; ++i)
    memset(&scene->frames[i], 0, sizeof(scene->frames[i]));
  if (removed)
    shell_canvas_scene_clear_selection(scene);
  return removed;
}

static bool shell_canvas_scene_duplicate_selected(ShellCanvasSceneState *scene) {
  ShellCanvasFrameNode originals[SHELL_CANVAS_FRAME_CAP];
  uint32_t original_count = 0u;
  uint32_t i = 0u;
  if (!scene)
    return false;
  for (i = 0u; i < scene->frame_count && original_count < SHELL_CANVAS_FRAME_CAP;
       ++i) {
    if (scene->frames[i].alive && scene->frames[i].selected &&
        shell_canvas_scene_node_in_active_page(scene, &scene->frames[i]))
      originals[original_count++] = scene->frames[i];
  }
  if (original_count == 0u)
    return false;
  shell_canvas_scene_clear_selection(scene);
  for (i = 0u; i < original_count && scene->frame_count < SHELL_CANVAS_FRAME_CAP;
       ++i) {
    ShellCanvasFrameNode *dst = &scene->frames[scene->frame_count++];
    const char *base_name = "Frame";
    *dst = originals[i];
    dst->id = scene->next_frame_id++;
    dst->selected = true;
    dst->x += 24.0f;
    dst->y += 24.0f;
    dst->image_texture = 0u;
    dst->image_w = 0;
    dst->image_h = 0;
    if (dst->parent_id > 0 &&
        !shell_canvas_scene_find_node_by_id_const(scene, dst->parent_id)) {
      dst->parent_id = 0;
    }
    switch (dst->kind) {
    case SHELL_NODE_RECTANGLE:
      base_name = "Rectangle";
      break;
    case SHELL_NODE_ELLIPSE:
      base_name = "Ellipse";
      break;
    case SHELL_NODE_LINE:
      base_name = "Line";
      break;
    case SHELL_NODE_ARROW:
      base_name = "Arrow";
      break;
    case SHELL_NODE_POLYGON:
      base_name = "Polygon";
      break;
    case SHELL_NODE_STAR:
      base_name = "Star";
      break;
    case SHELL_NODE_IMAGE:
      base_name = "Image";
      break;
    case SHELL_NODE_PATH:
      base_name = "Path";
      break;
    case SHELL_NODE_TEXT:
      base_name = "Text";
      break;
    case SHELL_NODE_FRAME:
    default:
      break;
    }
    snprintf(dst->name, sizeof(dst->name), "%s %d", base_name, dst->id);
    scene->selected_frame = (int)(scene->frame_count - 1u);
  }
  return true;
}

static void shell_canvas_scene_release(StygianContext *ctx,
                                       ShellCanvasSceneState *scene) {
  uint32_t i = 0u;
  if (!ctx || !scene)
    return;
  for (i = 0u; i < scene->frame_count; ++i)
    shell_canvas_node_release(ctx, &scene->frames[i]);
}

static void shell_canvas_guides_clear(ShellCanvasGuideState *guides) {
  if (!guides)
    return;
  memset(guides, 0, sizeof(*guides));
}

static void shell_canvas_consider_axis_snap(float candidate, float guide,
                                            float threshold, float *best_delta,
                                            float *best_dist,
                                            float *best_guide) {
  float delta = 0.0f;
  float dist = 0.0f;
  if (!best_delta || !best_dist || !best_guide)
    return;
  delta = guide - candidate;
  dist = fabsf(delta);
  if (dist <= threshold && dist < *best_dist) {
    *best_delta = delta;
    *best_dist = dist;
    *best_guide = guide;
  }
}

static bool shell_canvas_find_axis_snap(const ShellCanvasSceneState *scene,
                                        const ShellLayout *layout,
                                        float left_panel_width,
                                        float right_panel_width,
                                        const ShellCanvasViewState *view,
                                        int ignore_index, float candidate_a,
                                        float candidate_b, float candidate_c,
                                        bool use_a, bool use_b, bool use_c,
                                        bool vertical_axis,
                                        float *out_delta,
                                        float *out_guide_world) {
  float stage_x = 0.0f;
  float stage_y = 0.0f;
  float stage_w = 0.0f;
  float stage_h = 0.0f;
  float threshold = 0.0f;
  float best_delta = 0.0f;
  float best_dist = 1e9f;
  float best_guide = 0.0f;
  uint32_t i = 0u;
  if (!scene || !layout || !out_delta || !out_guide_world)
    return false;
  shell_canvas_stage_rect(layout, left_panel_width, right_panel_width, &stage_x,
                          &stage_y, &stage_w, &stage_h);
  (void)stage_x;
  (void)stage_y;
  threshold = shell_canvas_snap_threshold_world(view);
  if (vertical_axis) {
    if (use_a)
      shell_canvas_consider_axis_snap(candidate_a, 0.0f, threshold, &best_delta,
                                      &best_dist, &best_guide);
    if (use_b)
      shell_canvas_consider_axis_snap(candidate_b, stage_w * 0.5f, threshold,
                                      &best_delta, &best_dist, &best_guide);
    if (use_c)
      shell_canvas_consider_axis_snap(candidate_c, stage_w, threshold,
                                      &best_delta, &best_dist, &best_guide);
  } else {
    if (use_a)
      shell_canvas_consider_axis_snap(candidate_a, 0.0f, threshold, &best_delta,
                                      &best_dist, &best_guide);
    if (use_b)
      shell_canvas_consider_axis_snap(candidate_b, stage_h * 0.5f, threshold,
                                      &best_delta, &best_dist, &best_guide);
    if (use_c)
      shell_canvas_consider_axis_snap(candidate_c, stage_h, threshold,
                                      &best_delta, &best_dist, &best_guide);
  }
  for (i = 0u; i < scene->frame_count; ++i) {
    const ShellCanvasFrameNode *frame = &scene->frames[i];
    float a = 0.0f;
    float b = 0.0f;
    float c = 0.0f;
    if (!frame->alive || (int)i == ignore_index)
      continue;
    if (vertical_axis) {
      a = frame->x;
      b = frame->x + frame->w * 0.5f;
      c = frame->x + frame->w;
    } else {
      a = frame->y;
      b = frame->y + frame->h * 0.5f;
      c = frame->y + frame->h;
    }
    if (use_a)
      shell_canvas_consider_axis_snap(candidate_a, a, threshold, &best_delta,
                                      &best_dist, &best_guide);
    if (use_a)
      shell_canvas_consider_axis_snap(candidate_a, b, threshold, &best_delta,
                                      &best_dist, &best_guide);
    if (use_a)
      shell_canvas_consider_axis_snap(candidate_a, c, threshold, &best_delta,
                                      &best_dist, &best_guide);
    if (use_b)
      shell_canvas_consider_axis_snap(candidate_b, a, threshold, &best_delta,
                                      &best_dist, &best_guide);
    if (use_b)
      shell_canvas_consider_axis_snap(candidate_b, b, threshold, &best_delta,
                                      &best_dist, &best_guide);
    if (use_b)
      shell_canvas_consider_axis_snap(candidate_b, c, threshold, &best_delta,
                                      &best_dist, &best_guide);
    if (use_c)
      shell_canvas_consider_axis_snap(candidate_c, a, threshold, &best_delta,
                                      &best_dist, &best_guide);
    if (use_c)
      shell_canvas_consider_axis_snap(candidate_c, b, threshold, &best_delta,
                                      &best_dist, &best_guide);
    if (use_c)
      shell_canvas_consider_axis_snap(candidate_c, c, threshold, &best_delta,
                                      &best_dist, &best_guide);
  }
  if (best_dist >= 1e8f)
    return false;
  *out_delta = best_delta;
  *out_guide_world = best_guide;
  return true;
}

static float shell_canvas_snap_to_grid(float value,
                                       const ShellCanvasSnapSettings *settings) {
  float grid = 0.0f;
  if (!settings || !settings->grid_enabled)
    return value;
  grid = settings->grid_size;
  if (grid <= 0.0f)
    return value;
  return floorf((value / grid) + 0.5f) * grid;
}

static void shell_canvas_capture_group_origins(
    const ShellCanvasSceneState *scene, ShellCanvasInteractionState *interaction) {
  uint32_t i = 0u;
  if (!scene || !interaction)
    return;
  for (i = 0u; i < scene->frame_count && i < SHELL_CANVAS_FRAME_CAP; ++i) {
    interaction->frame_origin_x[i] = scene->frames[i].x;
    interaction->frame_origin_y[i] = scene->frames[i].y;
    interaction->frame_origin_w[i] = scene->frames[i].w;
    interaction->frame_origin_h[i] = scene->frames[i].h;
    interaction->frame_origin_rotation[i] = scene->frames[i].rotation_deg;
  }
  if (!shell_canvas_scene_selection_bounds(scene, &interaction->group_origin_x,
                                           &interaction->group_origin_y,
                                           &interaction->group_origin_w,
                                           &interaction->group_origin_h)) {
    interaction->group_origin_x = 0.0f;
    interaction->group_origin_y = 0.0f;
    interaction->group_origin_w = 0.0f;
    interaction->group_origin_h = 0.0f;
  }
}

static void shell_canvas_apply_group_rotation(
    ShellCanvasSceneState *scene, const ShellCanvasInteractionState *interaction,
    float delta_deg) {
  float center_x = 0.0f;
  float center_y = 0.0f;
  float delta_rad = 0.0f;
  uint32_t i = 0u;
  if (!scene || !interaction)
    return;
  center_x = interaction->group_origin_x + interaction->group_origin_w * 0.5f;
  center_y = interaction->group_origin_y + interaction->group_origin_h * 0.5f;
  delta_rad = shell_degrees_to_radians(delta_deg);
  for (i = 0u; i < scene->frame_count && i < SHELL_CANVAS_FRAME_CAP; ++i) {
    ShellCanvasFrameNode *frame = &scene->frames[i];
    float origin_cx = 0.0f;
    float origin_cy = 0.0f;
    float next_cx = 0.0f;
    float next_cy = 0.0f;
    if (!frame->alive || !frame->selected)
      continue;
    origin_cx = interaction->frame_origin_x[i] +
                interaction->frame_origin_w[i] * 0.5f;
    origin_cy = interaction->frame_origin_y[i] +
                interaction->frame_origin_h[i] * 0.5f;
    shell_rotate_point(origin_cx, origin_cy, center_x, center_y, delta_rad,
                       &next_cx, &next_cy);
    frame->x = next_cx - frame->w * 0.5f;
    frame->y = next_cy - frame->h * 0.5f;
    frame->rotation_deg = shell_normalize_degrees(
        interaction->frame_origin_rotation[i] + delta_deg);
  }
}

static void shell_canvas_apply_group_resize(
    ShellCanvasSceneState *scene, const ShellCanvasInteractionState *interaction,
    float new_x, float new_y, float new_w, float new_h) {
  float origin_x = 0.0f;
  float origin_y = 0.0f;
  float origin_w = 0.0f;
  float origin_h = 0.0f;
  float scale_x = 1.0f;
  float scale_y = 1.0f;
  uint32_t i = 0u;
  if (!scene || !interaction)
    return;
  origin_x = interaction->group_origin_x;
  origin_y = interaction->group_origin_y;
  origin_w = interaction->group_origin_w > 1.0f ? interaction->group_origin_w : 1.0f;
  origin_h = interaction->group_origin_h > 1.0f ? interaction->group_origin_h : 1.0f;
  scale_x = new_w / origin_w;
  scale_y = new_h / origin_h;
  for (i = 0u; i < scene->frame_count && i < SHELL_CANVAS_FRAME_CAP; ++i) {
    ShellCanvasFrameNode *frame = &scene->frames[i];
    float rel_x = 0.0f;
    float rel_y = 0.0f;
    if (!frame->alive || !frame->selected)
      continue;
    rel_x = interaction->frame_origin_x[i] - origin_x;
    rel_y = interaction->frame_origin_y[i] - origin_y;
    frame->x = new_x + rel_x * scale_x;
    frame->y = new_y + rel_y * scale_y;
    frame->w = shell_clampf(interaction->frame_origin_w[i] * scale_x,
                            shell_canvas_node_min_width(frame->kind), 4096.0f);
    frame->h = shell_clampf(interaction->frame_origin_h[i] * scale_y,
                            shell_canvas_node_min_height(frame->kind), 4096.0f);
    shell_canvas_clamp_frame_radius(frame);
  }
}

static ShellCanvasFrameNode *shell_canvas_scene_create_frame(
    ShellCanvasSceneState *scene, const ShellLayout *layout,
    float left_panel_width, float right_panel_width,
    const ShellCanvasViewState *view, float start_world_x, float start_world_y,
    float end_world_x, float end_world_y, int preset_w, int preset_h) {
  float stage_x = 0.0f;
  float stage_y = 0.0f;
  float stage_w = 0.0f;
  float stage_h = 0.0f;
  float x = 0.0f;
  float y = 0.0f;
  float frame_w = 0.0f;
  float frame_h = 0.0f;
  ShellCanvasFrameNode *frame = NULL;
  if (!scene || !layout || scene->frame_count >= SHELL_CANVAS_FRAME_CAP)
    return NULL;
  shell_canvas_stage_rect(layout, left_panel_width, right_panel_width, &stage_x,
                          &stage_y, &stage_w, &stage_h);
  (void)view;
  x = start_world_x < end_world_x ? start_world_x : end_world_x;
  y = start_world_y < end_world_y ? start_world_y : end_world_y;
  frame_w = fabsf(end_world_x - start_world_x);
  frame_h = fabsf(end_world_y - start_world_y);
  if (frame_w < 96.0f || frame_h < 96.0f)
    return NULL;
  frame = &scene->frames[scene->frame_count++];
  memset(frame, 0, sizeof(*frame));
  frame->alive = true;
  frame->selected = true;
  frame->visible = true;
  frame->locked = false;
  frame->id = scene->next_frame_id++;
  frame->page_id = scene->active_page_id;
  frame->parent_id = 0;
  frame->kind = SHELL_NODE_FRAME;
  frame->w = frame_w;
  frame->h = frame_h;
  frame->radius = 18.0f;
  frame->top_flat = false;
  frame->preset_w = preset_w > 0 ? preset_w : 1440;
  frame->preset_h = preset_h > 0 ? preset_h : 900;
  shell_canvas_apply_default_style(frame);
  snprintf(frame->name, sizeof(frame->name), "Frame %d", frame->id);
  frame->x = shell_clampf(x, 0.0f, stage_w - frame_w);
  frame->y = shell_clampf(y, 0.0f, stage_h - frame_h);
  shell_canvas_scene_select_only(scene, (int)scene->frame_count - 1);
  return frame;
}

static ShellCanvasFrameNode *shell_canvas_scene_create_primitive(
    ShellCanvasSceneState *scene, const ShellLayout *layout,
    float left_panel_width, float right_panel_width,
    const ShellCanvasViewState *view, float start_world_x, float start_world_y,
    float end_world_x, float end_world_y, ShellCanvasNodeKind kind,
    float radius) {
  float stage_x = 0.0f;
  float stage_y = 0.0f;
  float stage_w = 0.0f;
  float stage_h = 0.0f;
  float x = 0.0f;
  float y = 0.0f;
  float node_w = 0.0f;
  float node_h = 0.0f;
  const char *base_name = "Rect";
  ShellCanvasFrameNode *frame = NULL;
  if (!scene || !layout || scene->frame_count >= SHELL_CANVAS_FRAME_CAP)
    return NULL;
  shell_canvas_stage_rect(layout, left_panel_width, right_panel_width, &stage_x,
                          &stage_y, &stage_w, &stage_h);
  (void)view;
  x = start_world_x < end_world_x ? start_world_x : end_world_x;
  y = start_world_y < end_world_y ? start_world_y : end_world_y;
  node_w = fabsf(end_world_x - start_world_x);
  node_h = fabsf(end_world_y - start_world_y);
  if (kind == SHELL_NODE_LINE) {
    if (node_w < 24.0f && node_h < 24.0f)
      return NULL;
    if (node_w < 2.0f)
      node_w = 2.0f;
    if (node_h < 2.0f)
      node_h = 2.0f;
  } else if (kind == SHELL_NODE_ARROW) {
    if (node_w < 24.0f)
      node_w = 180.0f;
    if (node_h < 24.0f)
      node_h = 48.0f;
  } else if (kind == SHELL_NODE_IMAGE) {
    if (node_w < 48.0f)
      node_w = 220.0f;
    if (node_h < 48.0f)
      node_h = 160.0f;
  } else if (kind == SHELL_NODE_TEXT) {
    if (node_w < 48.0f)
      node_w = 220.0f;
    if (node_h < 24.0f)
      node_h = 48.0f;
  } else if (node_w < 32.0f || node_h < 32.0f) {
    return NULL;
  }
  frame = &scene->frames[scene->frame_count++];
  memset(frame, 0, sizeof(*frame));
  frame->alive = true;
  frame->selected = true;
  frame->visible = true;
  frame->locked = false;
  frame->id = scene->next_frame_id++;
  frame->page_id = scene->active_page_id;
  frame->parent_id = 0;
  frame->kind = kind;
  frame->w = node_w;
  frame->h = node_h;
  frame->radius = kind == SHELL_NODE_ELLIPSE || kind == SHELL_NODE_LINE ||
                          kind == SHELL_NODE_ARROW ||
                          kind == SHELL_NODE_POLYGON ||
                          kind == SHELL_NODE_STAR
                      ? 0.0f
                      : radius;
  frame->font_size = kind == SHELL_NODE_TEXT ? 28.0f : 14.0f;
  frame->top_flat = false;
  frame->preset_w = 0;
  frame->preset_h = 0;
  shell_canvas_apply_default_style(frame);
  if (kind == SHELL_NODE_ELLIPSE)
    base_name = "Ellipse";
  else if (kind == SHELL_NODE_LINE)
    base_name = "Line";
  else if (kind == SHELL_NODE_ARROW)
    base_name = "Arrow";
  else if (kind == SHELL_NODE_POLYGON)
    base_name = "Polygon";
  else if (kind == SHELL_NODE_STAR)
    base_name = "Star";
  else if (kind == SHELL_NODE_IMAGE)
    base_name = "Image";
  else if (kind == SHELL_NODE_TEXT)
    base_name = "Text";
  else if (radius > 24.0f)
    base_name = "Rounded";
  snprintf(frame->name, sizeof(frame->name), "%s %d", base_name, frame->id);
  if (kind == SHELL_NODE_TEXT)
    snprintf(frame->text, sizeof(frame->text), "Text");
  frame->x = shell_clampf(x, 0.0f, stage_w - node_w);
  frame->y = shell_clampf(y, 0.0f, stage_h - node_h);
  frame->parent_id = shell_canvas_scene_pick_parent_id(scene, frame);
  shell_canvas_clamp_frame_radius(frame);
  shell_canvas_scene_select_only(scene, (int)scene->frame_count - 1);
  return frame;
}

static ShellCanvasFrameNode *shell_canvas_scene_create_image(
    StygianContext *ctx, ShellCanvasSceneState *scene, const ShellLayout *layout,
    float left_panel_width, float right_panel_width,
    const ShellCanvasViewState *view, float start_world_x, float start_world_y,
    float end_world_x, float end_world_y, const char *path) {
  ShellCanvasFrameNode *frame = NULL;
  if (!ctx || !path || !path[0])
    return NULL;
  frame = shell_canvas_scene_create_primitive(
      scene, layout, left_panel_width, right_panel_width, view, start_world_x,
      start_world_y, end_world_x, end_world_y, SHELL_NODE_IMAGE, 0.0f);
  if (!frame)
    return NULL;
  snprintf(frame->asset_path, sizeof(frame->asset_path), "%s", path);
  if (!shell_canvas_node_load_image(ctx, frame))
    return frame;
  if (frame->image_w > 0 && frame->image_h > 0) {
    float aspect = (float)frame->image_w / (float)frame->image_h;
    if (frame->w < 60.0f || frame->h < 60.0f) {
      frame->w = (float)frame->image_w;
      frame->h = (float)frame->image_h;
    } else {
      float target_h = frame->w / aspect;
      if (target_h > frame->h) {
        frame->w = frame->h * aspect;
      } else {
        frame->h = target_h;
      }
    }
  }
  return frame;
}

static bool shell_motion_add_binding(ShellMotionState *state, int trigger_node_id,
                                     ShellMotionEventKind event_kind, int clip_id,
                                     bool reverse, bool toggle,
                                     const char *name) {
  ShellMotionBehavior *behavior = NULL;
  if (!state || trigger_node_id <= 0 || clip_id <= 0)
    return false;
  behavior = shell_motion_alloc_behavior(state);
  if (!behavior)
    return false;
  behavior->trigger_node_id = trigger_node_id;
  behavior->event_kind = event_kind;
  behavior->clip_id = clip_id;
  behavior->reverse = reverse;
  behavior->toggle = toggle;
  behavior->toggle_state = false;
  snprintf(behavior->name, sizeof(behavior->name), "%s",
           name && name[0] ? name : "Behavior");
  return true;
}

static bool shell_motion_bind_selected_clip(ShellMotionState *state,
                                            int trigger_node_id,
                                            ShellMotionEventKind enter_event,
                                            ShellMotionEventKind leave_event,
                                            bool toggle) {
  char label[48];
  if (!state || trigger_node_id <= 0 || state->selected_clip_id <= 0)
    return false;
  if (toggle) {
    snprintf(label, sizeof(label), "toggle %d", trigger_node_id);
    return shell_motion_add_binding(state, trigger_node_id, enter_event,
                                    state->selected_clip_id, false, true, label);
  }
  snprintf(label, sizeof(label), "%s in", shell_motion_event_label(enter_event));
  if (!shell_motion_add_binding(state, trigger_node_id, enter_event,
                                state->selected_clip_id, false, false, label)) {
    return false;
  }
  snprintf(label, sizeof(label), "%s out", shell_motion_event_label(leave_event));
  if (!shell_motion_add_binding(state, trigger_node_id, leave_event,
                                state->selected_clip_id, true, false, label)) {
    return false;
  }
  return true;
}

static ShellMotionClip *shell_motion_create_named_clip(ShellMotionState *state,
                                                       const char *name) {
  ShellMotionClip *clip = shell_motion_alloc_clip(state);
  if (!clip)
    return NULL;
  if (name && name[0])
    snprintf(clip->name, sizeof(clip->name), "%s", name);
  state->selected_clip_id = clip->id;
  shell_motion_sync_fields(state);
  return clip;
}

static bool shell_motion_create_hover_preset(ShellMotionState *state,
                                             const ShellCanvasFrameNode *node) {
  ShellMotionClip *clip = NULL;
  ShellMotionTrack *scale = NULL;
  ShellMotionTrack *color = NULL;
  char name[32];
  if (!state || !node)
    return false;
  snprintf(name, sizeof(name), "%s Hover", node->name);
  clip = shell_motion_create_named_clip(state, name);
  if (!clip)
    return false;
  scale = shell_motion_add_track_to_clip(state, node, clip, SHELL_MOTION_PROP_SCALE);
  color = shell_motion_add_track_to_clip(state, node, clip, SHELL_MOTION_PROP_COLOR);
  if (!scale || !color)
    return false;
  scale->to_value = 1.04f;
  color->to_rgba[0] = shell_clampf(color->from_rgba[0] + 0.10f, 0.0f, 1.0f);
  color->to_rgba[1] = shell_clampf(color->from_rgba[1] + 0.10f, 0.0f, 1.0f);
  color->to_rgba[2] = shell_clampf(color->from_rgba[2] + 0.12f, 0.0f, 1.0f);
  shell_motion_clip_recompute_duration(state, clip);
  return shell_motion_bind_selected_clip(state, node->id,
                                         SHELL_MOTION_EVENT_HOVER_ENTER,
                                         SHELL_MOTION_EVENT_HOVER_LEAVE, false);
}

static bool shell_motion_create_press_preset(ShellMotionState *state,
                                             const ShellCanvasFrameNode *node) {
  ShellMotionClip *clip = NULL;
  ShellMotionTrack *scale = NULL;
  char name[32];
  if (!state || !node)
    return false;
  snprintf(name, sizeof(name), "%s Press", node->name);
  clip = shell_motion_create_named_clip(state, name);
  if (!clip)
    return false;
  scale = shell_motion_add_track_to_clip(state, node, clip, SHELL_MOTION_PROP_SCALE);
  if (!scale)
    return false;
  scale->to_value = 0.96f;
  scale->duration_ms = 140u;
  scale->easing = SHELL_MOTION_EASE_OUT_CUBIC;
  shell_motion_clip_recompute_duration(state, clip);
  return shell_motion_bind_selected_clip(state, node->id,
                                         SHELL_MOTION_EVENT_PRESS,
                                         SHELL_MOTION_EVENT_RELEASE, false);
}

static bool shell_motion_create_drawer_scaffold(
    ShellMotionState *state, ShellCanvasSceneState *scene, const ShellLayout *layout,
    float left_panel_width, float right_panel_width,
    const ShellCanvasViewState *view) {
  ShellCanvasFrameNode *toggle = NULL;
  ShellCanvasFrameNode *drawer = NULL;
  ShellMotionClip *clip = NULL;
  ShellMotionTrack *track = NULL;
  float stage_x = 0.0f;
  float stage_y = 0.0f;
  float stage_w = 0.0f;
  float stage_h = 0.0f;
  (void)view;
  if (!state || !scene || !layout)
    return false;
  shell_canvas_stage_rect(layout, left_panel_width, right_panel_width, &stage_x,
                          &stage_y, &stage_w, &stage_h);
  toggle = shell_canvas_scene_create_primitive(
      scene, layout, left_panel_width, right_panel_width, view, 28.0f, 24.0f,
      140.0f, 66.0f, SHELL_NODE_RECTANGLE, 10.0f);
  if (!toggle)
    return false;
  snprintf(toggle->name, sizeof(toggle->name), "Drawer Toggle %d", toggle->id);
  toggle->fill_rgba[0] = 0.18f;
  toggle->fill_rgba[1] = 0.49f;
  toggle->fill_rgba[2] = 0.95f;

  drawer = shell_canvas_scene_create_frame(scene, layout, left_panel_width,
                                           right_panel_width, view, -240.0f,
                                           84.0f, 20.0f, stage_h - 28.0f, 0, 0);
  if (!drawer)
    return false;
  snprintf(drawer->name, sizeof(drawer->name), "Sidebar Drawer %d", drawer->id);
  drawer->fill_rgba[0] = 0.10f;
  drawer->fill_rgba[1] = 0.12f;
  drawer->fill_rgba[2] = 0.16f;
  drawer->fill_rgba[3] = 1.0f;
  drawer->stroke_width = 0.0f;

  clip = shell_motion_create_named_clip(state, "Sidebar Reveal");
  if (!clip)
    return false;
  track = shell_motion_add_track_to_clip(state, drawer, clip, SHELL_MOTION_PROP_X);
  if (!track)
    return false;
  track->from_value = -240.0f;
  track->to_value = 20.0f;
  track->duration_ms = 260u;
  track->easing = SHELL_MOTION_EASE_OUT_CUBIC;
  shell_motion_clip_recompute_duration(state, clip);
  if (!shell_motion_bind_selected_clip(state, toggle->id, SHELL_MOTION_EVENT_PRESS,
                                       SHELL_MOTION_EVENT_PRESS, true)) {
    return false;
  }
  shell_canvas_scene_select_only(scene, scene->selected_frame);
  return true;
}

static bool shell_motion_create_tabs_scaffold(
    ShellMotionState *state, ShellCanvasSceneState *scene, const ShellLayout *layout,
    float left_panel_width, float right_panel_width,
    const ShellCanvasViewState *view) {
  ShellCanvasFrameNode *buttons[3] = {0};
  ShellCanvasFrameNode *pages[3] = {0};
  float strip_y = 24.0f;
  float page_y = 96.0f;
  float button_x = 180.0f;
  int i = 0;
  if (!state || !scene || !layout)
    return false;
  for (i = 0; i < 3; ++i) {
    char clip_name[32];
    ShellMotionClip *clip = NULL;
    buttons[i] = shell_canvas_scene_create_primitive(
        scene, layout, left_panel_width, right_panel_width, view,
        button_x + i * 112.0f, strip_y, button_x + i * 112.0f + 100.0f,
        strip_y + 44.0f, SHELL_NODE_RECTANGLE, 10.0f);
    if (!buttons[i])
      return false;
    snprintf(buttons[i]->name, sizeof(buttons[i]->name), "Tab %d", i + 1);
    buttons[i]->fill_rgba[0] = i == 0 ? 0.18f : 0.16f;
    buttons[i]->fill_rgba[1] = i == 0 ? 0.49f : 0.17f;
    buttons[i]->fill_rgba[2] = i == 0 ? 0.95f : 0.20f;
    buttons[i]->text[0] = '\0';

    pages[i] = shell_canvas_scene_create_frame(scene, layout, left_panel_width,
                                               right_panel_width, view, 180.0f,
                                               page_y, 180.0f + 360.0f,
                                               page_y + 220.0f, 0, 0);
    if (!pages[i])
      return false;
    snprintf(pages[i]->name, sizeof(pages[i]->name), "Page %d", i + 1);
    pages[i]->fill_rgba[0] = 0.12f + 0.02f * (float)i;
    pages[i]->fill_rgba[1] = 0.15f + 0.03f * (float)i;
    pages[i]->fill_rgba[2] = 0.22f + 0.04f * (float)i;
    pages[i]->opacity = i == 0 ? 1.0f : 0.08f;

    snprintf(clip_name, sizeof(clip_name), "Tab %d Show", i + 1);
    clip = shell_motion_create_named_clip(state, clip_name);
    if (!clip)
      return false;
    for (int j = 0; j < 3; ++j) {
      ShellMotionTrack *page_opacity = shell_motion_add_track_to_clip(
          state, pages[j], clip, SHELL_MOTION_PROP_OPACITY);
      ShellMotionTrack *button_color = shell_motion_add_track_to_clip(
          state, buttons[j], clip, SHELL_MOTION_PROP_COLOR);
      if (!page_opacity || !button_color)
        return false;
      page_opacity->from_value = pages[j]->opacity;
      page_opacity->to_value = (i == j) ? 1.0f : 0.08f;
      page_opacity->duration_ms = 180u;
      button_color->from_rgba[0] = buttons[j]->fill_rgba[0];
      button_color->from_rgba[1] = buttons[j]->fill_rgba[1];
      button_color->from_rgba[2] = buttons[j]->fill_rgba[2];
      button_color->from_rgba[3] = buttons[j]->fill_rgba[3];
      button_color->to_rgba[0] = (i == j) ? 0.18f : 0.16f;
      button_color->to_rgba[1] = (i == j) ? 0.49f : 0.17f;
      button_color->to_rgba[2] = (i == j) ? 0.95f : 0.20f;
      button_color->to_rgba[3] = 1.0f;
      button_color->duration_ms = 180u;
    }
    shell_motion_clip_recompute_duration(state, clip);
    if (!shell_motion_add_binding(state, buttons[i]->id, SHELL_MOTION_EVENT_PRESS,
                                  clip->id, false, false, clip_name)) {
      return false;
    }
  }
  shell_canvas_scene_select_only(scene, scene->selected_frame);
  return true;
}

static bool shell_motion_create_modal_scaffold(
    ShellMotionState *state, ShellCanvasSceneState *scene, const ShellLayout *layout,
    float left_panel_width, float right_panel_width,
    const ShellCanvasViewState *view) {
  ShellCanvasFrameNode *open_button = NULL;
  ShellCanvasFrameNode *overlay = NULL;
  ShellCanvasFrameNode *modal = NULL;
  ShellCanvasFrameNode *close_button = NULL;
  ShellMotionClip *clip = NULL;
  ShellMotionTrack *overlay_opacity = NULL;
  ShellMotionTrack *modal_y = NULL;
  ShellMotionTrack *modal_scale = NULL;
  if (!state || !scene || !layout)
    return false;
  open_button = shell_canvas_scene_create_primitive(
      scene, layout, left_panel_width, right_panel_width, view, 40.0f, 120.0f,
      180.0f, 164.0f, SHELL_NODE_RECTANGLE, 10.0f);
  if (!open_button)
    return false;
  snprintf(open_button->name, sizeof(open_button->name), "Open Modal %d",
           open_button->id);
  open_button->fill_rgba[0] = 0.18f;
  open_button->fill_rgba[1] = 0.49f;
  open_button->fill_rgba[2] = 0.95f;

  overlay = shell_canvas_scene_create_frame(scene, layout, left_panel_width,
                                            right_panel_width, view, 160.0f,
                                            60.0f, 680.0f, 420.0f, 0, 0);
  if (!overlay)
    return false;
  snprintf(overlay->name, sizeof(overlay->name), "Modal Overlay %d", overlay->id);
  overlay->fill_rgba[0] = 0.02f;
  overlay->fill_rgba[1] = 0.03f;
  overlay->fill_rgba[2] = 0.04f;
  overlay->opacity = 0.0f;

  modal = shell_canvas_scene_create_frame(scene, layout, left_panel_width,
                                          right_panel_width, view, 250.0f, 260.0f,
                                          570.0f, 420.0f, 0, 0);
  if (!modal)
    return false;
  snprintf(modal->name, sizeof(modal->name), "Modal Card %d", modal->id);
  modal->fill_rgba[0] = 0.13f;
  modal->fill_rgba[1] = 0.15f;
  modal->fill_rgba[2] = 0.20f;

  close_button = shell_canvas_scene_create_primitive(
      scene, layout, left_panel_width, right_panel_width, view, 528.0f, 274.0f,
      560.0f, 306.0f, SHELL_NODE_RECTANGLE, 8.0f);
  if (!close_button)
    return false;
  snprintf(close_button->name, sizeof(close_button->name), "Close Modal %d",
           close_button->id);
  close_button->fill_rgba[0] = 0.24f;
  close_button->fill_rgba[1] = 0.14f;
  close_button->fill_rgba[2] = 0.16f;

  clip = shell_motion_create_named_clip(state, "Modal Reveal");
  if (!clip)
    return false;
  overlay_opacity = shell_motion_add_track_to_clip(state, overlay, clip,
                                                   SHELL_MOTION_PROP_OPACITY);
  modal_y = shell_motion_add_track_to_clip(state, modal, clip, SHELL_MOTION_PROP_Y);
  modal_scale =
      shell_motion_add_track_to_clip(state, modal, clip, SHELL_MOTION_PROP_SCALE);
  if (!overlay_opacity || !modal_y || !modal_scale)
    return false;
  overlay_opacity->from_value = 0.0f;
  overlay_opacity->to_value = 0.56f;
  modal_y->from_value = 260.0f;
  modal_y->to_value = 220.0f;
  modal_scale->from_value = 0.94f;
  modal_scale->to_value = 1.0f;
  shell_motion_clip_recompute_duration(state, clip);
  if (!shell_motion_add_binding(state, open_button->id, SHELL_MOTION_EVENT_PRESS,
                                clip->id, false, true, "modal toggle"))
    return false;
  if (!shell_motion_add_binding(state, close_button->id, SHELL_MOTION_EVENT_PRESS,
                                clip->id, true, false, "modal close"))
    return false;
  return true;
}

static void shell_canvas_snap_move_rect(const ShellCanvasSceneState *scene,
                                        const ShellLayout *layout,
                                        float left_panel_width,
                                        float right_panel_width,
                                        const ShellCanvasViewState *view,
                                        const ShellCanvasSnapSettings *settings,
                                        int ignore_index, float w, float h,
                                        float *io_x, float *io_y,
                                        ShellCanvasGuideState *guides) {
  float x = 0.0f;
  float y = 0.0f;
  float delta = 0.0f;
  float guide = 0.0f;
  if (!io_x || !io_y)
    return;
  x = *io_x;
  y = *io_y;
  if (guides)
    shell_canvas_guides_clear(guides);
  if (settings && settings->guide_enabled) {
    if (shell_canvas_find_axis_snap(scene, layout, left_panel_width,
                                    right_panel_width, view, ignore_index, x,
                                    x + w * 0.5f, x + w, true, true, true, true,
                                    &delta, &guide)) {
      x += delta;
      if (guides) {
        guides->x_active = true;
        guides->x_world = guide;
      }
    }
    if (shell_canvas_find_axis_snap(scene, layout, left_panel_width,
                                    right_panel_width, view, ignore_index, y,
                                    y + h * 0.5f, y + h, true, true, true,
                                    false, &delta, &guide)) {
      y += delta;
      if (guides) {
        guides->y_active = true;
        guides->y_world = guide;
      }
    }
  }
  if (settings && settings->grid_enabled) {
    if (!(guides && guides->x_active)) {
      float snapped = shell_canvas_snap_to_grid(x, settings);
      if (snapped != x && guides) {
        guides->x_active = true;
        guides->x_from_grid = true;
        guides->x_world = snapped;
      }
      x = snapped;
    }
    if (!(guides && guides->y_active)) {
      float snapped = shell_canvas_snap_to_grid(y, settings);
      if (snapped != y && guides) {
        guides->y_active = true;
        guides->y_from_grid = true;
        guides->y_world = snapped;
      }
      y = snapped;
    }
  }
  *io_x = x;
  *io_y = y;
}

static void shell_canvas_snap_resize_rect(const ShellCanvasSceneState *scene,
                                          const ShellLayout *layout,
                                          float left_panel_width,
                                          float right_panel_width,
                                          const ShellCanvasViewState *view,
                                          const ShellCanvasSnapSettings *settings,
                                          int ignore_index, bool snap_left,
                                          bool snap_right, bool snap_top,
                                          bool snap_bottom, float *io_x,
                                          float *io_y, float *io_w, float *io_h,
                                          ShellCanvasGuideState *guides) {
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
  float delta = 0.0f;
  float guide = 0.0f;
  float left = 0.0f;
  float right = 0.0f;
  float top = 0.0f;
  float bottom = 0.0f;
  if (!io_x || !io_y || !io_w || !io_h)
    return;
  x = *io_x;
  y = *io_y;
  w = *io_w;
  h = *io_h;
  left = x;
  right = x + w;
  top = y;
  bottom = y + h;
  if (guides)
    shell_canvas_guides_clear(guides);
  if (settings && settings->guide_enabled) {
    if (shell_canvas_find_axis_snap(scene, layout, left_panel_width,
                                    right_panel_width, view, ignore_index, left,
                                    x + w * 0.5f, right, snap_left, false,
                                    snap_right, true, &delta, &guide)) {
      if (snap_left) {
        left += delta;
        x = left;
        w = right - left;
      } else if (snap_right) {
        right += delta;
        w = right - x;
      }
      if (guides) {
        guides->x_active = true;
        guides->x_world = guide;
      }
    }
    if (shell_canvas_find_axis_snap(scene, layout, left_panel_width,
                                    right_panel_width, view, ignore_index, top,
                                    y + h * 0.5f, bottom, snap_top, false,
                                    snap_bottom, false, &delta, &guide)) {
      if (snap_top) {
        top += delta;
        y = top;
        h = bottom - top;
      } else if (snap_bottom) {
        bottom += delta;
        h = bottom - y;
      }
      if (guides) {
        guides->y_active = true;
        guides->y_world = guide;
      }
    }
  }
  if (settings && settings->grid_enabled) {
    if (snap_left && !(guides && guides->x_active)) {
      float snapped = shell_canvas_snap_to_grid(left, settings);
      if (guides && snapped != left) {
        guides->x_active = true;
        guides->x_from_grid = true;
        guides->x_world = snapped;
      }
      x = snapped;
      w = right - snapped;
    } else if (snap_right && !(guides && guides->x_active)) {
      float snapped = shell_canvas_snap_to_grid(right, settings);
      if (guides && snapped != right) {
        guides->x_active = true;
        guides->x_from_grid = true;
        guides->x_world = snapped;
      }
      w = snapped - x;
    }
    if (snap_top && !(guides && guides->y_active)) {
      float snapped = shell_canvas_snap_to_grid(top, settings);
      if (guides && snapped != top) {
        guides->y_active = true;
        guides->y_from_grid = true;
        guides->y_world = snapped;
      }
      y = snapped;
      h = bottom - snapped;
    } else if (snap_bottom && !(guides && guides->y_active)) {
      float snapped = shell_canvas_snap_to_grid(bottom, settings);
      if (guides && snapped != bottom) {
        guides->y_active = true;
        guides->y_from_grid = true;
        guides->y_world = snapped;
      }
      h = snapped - y;
    }
  }
  *io_x = x;
  *io_y = y;
  *io_w = w;
  *io_h = h;
}

static void shell_draw_canvas_node_shape(StygianContext *ctx,
                                         const ShellCanvasFrameNode *frame) {
  float fill_r = 0.0f;
  float fill_g = 0.0f;
  float fill_b = 0.0f;
  float fill_a = 0.0f;
  float stroke_r = 0.0f;
  float stroke_g = 0.0f;
  float stroke_b = 0.0f;
  float stroke_a = 0.0f;
  float stroke_width = 0.0f;
  float radius = 0.0f;
  float inset = 0.0f;
  float corners[4] = {0};
  float inner_corners[4] = {0};
  const float pi = 3.1415926535f;
  int i = 0;
  if (!ctx || !frame)
    return;
  fill_r = frame->fill_rgba[0];
  fill_g = frame->fill_rgba[1];
  fill_b = frame->fill_rgba[2];
  fill_a = frame->fill_rgba[3] * frame->opacity;
  stroke_r = frame->stroke_rgba[0];
  stroke_g = frame->stroke_rgba[1];
  stroke_b = frame->stroke_rgba[2];
  stroke_a = frame->stroke_rgba[3] * frame->opacity;
  stroke_width = frame->stroke_width > 0.0f ? frame->stroke_width : 0.0f;
  shell_canvas_node_corner_radii(frame, corners);
  switch (frame->kind) {
  case SHELL_NODE_TEXT:
    break;
  case SHELL_NODE_LINE:
    stygian_line(ctx, frame->x, frame->y + frame->h,
                 frame->x + frame->w, frame->y,
                 stroke_width > 0.0f ? stroke_width : 2.0f, stroke_r, stroke_g,
                 stroke_b, stroke_a);
    break;
  case SHELL_NODE_ARROW: {
    float cy = frame->y + frame->h * 0.5f;
    float tip_x = frame->x + frame->w - 4.0f;
    float tail_x = frame->x + 6.0f;
    float head_x =
        frame->x + frame->w - (frame->w < 32.0f ? frame->w * 0.35f : 18.0f);
    float half_head = frame->h * 0.5f - 4.0f;
    if (half_head < 5.0f)
      half_head = 5.0f;
    stygian_line(ctx, tail_x, cy, tip_x, cy,
                 stroke_width > 0.0f ? stroke_width : 2.0f, stroke_r, stroke_g,
                 stroke_b, stroke_a);
    stygian_line(ctx, head_x, cy - half_head, tip_x, cy,
                 stroke_width > 0.0f ? stroke_width : 2.0f, stroke_r, stroke_g,
                 stroke_b, stroke_a);
    stygian_line(ctx, head_x, cy + half_head, tip_x, cy,
                 stroke_width > 0.0f ? stroke_width : 2.0f, stroke_r, stroke_g,
                 stroke_b, stroke_a);
  } break;
  case SHELL_NODE_POLYGON: {
    float cx = frame->x + frame->w * 0.5f;
    float cy = frame->y + frame->h * 0.5f;
    float rx = frame->w * 0.5f - 3.0f;
    float ry = frame->h * 0.5f - 3.0f;
    float prev_x = 0.0f;
    float prev_y = 0.0f;
    float first_x = 0.0f;
    float first_y = 0.0f;
    for (i = 0; i < 6; ++i) {
      float angle = ((float)i / 6.0f) * pi * 2.0f - pi * 0.5f;
      float px = cx + cosf(angle) * rx;
      float py = cy + sinf(angle) * ry;
      if (i == 0) {
        first_x = px;
        first_y = py;
      } else {
        stygian_line(ctx, prev_x, prev_y, px, py,
                     stroke_width > 0.0f ? stroke_width : 2.0f, stroke_r,
                     stroke_g, stroke_b, stroke_a);
      }
      prev_x = px;
      prev_y = py;
    }
    stygian_line(ctx, prev_x, prev_y, first_x, first_y,
                 stroke_width > 0.0f ? stroke_width : 2.0f, stroke_r, stroke_g,
                 stroke_b, stroke_a);
  } break;
  case SHELL_NODE_STAR: {
    float cx = frame->x + frame->w * 0.5f;
    float cy = frame->y + frame->h * 0.5f;
    float outer_rx = frame->w * 0.5f - 3.0f;
    float outer_ry = frame->h * 0.5f - 3.0f;
    float inner_rx = outer_rx * 0.45f;
    float inner_ry = outer_ry * 0.45f;
    float prev_x = 0.0f;
    float prev_y = 0.0f;
    float first_x = 0.0f;
    float first_y = 0.0f;
    for (i = 0; i < 10; ++i) {
      float angle = ((float)i / 10.0f) * pi * 2.0f - pi * 0.5f;
      float radius_x = (i % 2 == 0) ? outer_rx : inner_rx;
      float radius_y = (i % 2 == 0) ? outer_ry : inner_ry;
      float px = cx + cosf(angle) * radius_x;
      float py = cy + sinf(angle) * radius_y;
      if (i == 0) {
        first_x = px;
        first_y = py;
      } else {
        stygian_line(ctx, prev_x, prev_y, px, py,
                     stroke_width > 0.0f ? stroke_width : 2.0f, stroke_r,
                     stroke_g, stroke_b, stroke_a);
      }
      prev_x = px;
      prev_y = py;
    }
    stygian_line(ctx, prev_x, prev_y, first_x, first_y,
                 stroke_width > 0.0f ? stroke_width : 2.0f, stroke_r, stroke_g,
                 stroke_b, stroke_a);
  } break;
  case SHELL_NODE_PATH: {
    float prev_x = 0.0f;
    float prev_y = 0.0f;
    float first_x = 0.0f;
    float first_y = 0.0f;
    for (i = 0; i < frame->point_count; ++i) {
      float px = frame->x + frame->path_x[i] * frame->w;
      float py = frame->y + frame->path_y[i] * frame->h;
      if (i == 0) {
        first_x = px;
        first_y = py;
      } else {
        stygian_line(ctx, prev_x, prev_y, px, py,
                     stroke_width > 0.0f ? stroke_width : 2.0f, stroke_r,
                     stroke_g, stroke_b, stroke_a);
      }
      prev_x = px;
      prev_y = py;
    }
    if (frame->path_closed && frame->point_count > 2)
      stygian_line(ctx, prev_x, prev_y, first_x, first_y,
                   stroke_width > 0.0f ? stroke_width : 2.0f, stroke_r,
                   stroke_g, stroke_b, stroke_a);
  } break;
  case SHELL_NODE_ELLIPSE:
    radius = frame->w < frame->h ? frame->w * 0.5f : frame->h * 0.5f;
    corners[0] = corners[1] = corners[2] = corners[3] = radius;
    if (stroke_width > 0.0f && stroke_a > 0.0f) {
      StygianElement outer = stygian_rect(ctx, frame->x, frame->y, frame->w,
                                          frame->h, stroke_r, stroke_g,
                                          stroke_b, stroke_a);
      if (outer != 0u) {
        stygian_set_radius(ctx, outer, corners[0], corners[1], corners[2],
                           corners[3]);
        shell_canvas_apply_effect(ctx, outer, frame);
      }
      inset = shell_clampf(fminf(stroke_width, radius - 1.0f), 0.0f, radius);
    }
    if (fill_a > 0.0f) {
      float inner_x = frame->x + inset;
      float inner_y = frame->y + inset;
      float inner_w = frame->w - inset * 2.0f;
      float inner_h = frame->h - inset * 2.0f;
      float inner_radius = radius - inset;
      if (inner_w > 0.0f && inner_h > 0.0f && inner_radius > 0.0f) {
        StygianElement inner = stygian_rect(ctx, inner_x, inner_y, inner_w, inner_h,
                                            fill_r, fill_g, fill_b, fill_a);
        if (inner != 0u) {
          stygian_set_radius(ctx, inner, inner_radius, inner_radius,
                             inner_radius, inner_radius);
          if (frame->fill_mode == SHELL_FILL_LINEAR) {
            stygian_set_gradient(ctx, inner, frame->fill_angle_deg,
                                 fill_r, fill_g, fill_b, fill_a,
                                 frame->fill_alt_rgba[0], frame->fill_alt_rgba[1],
                                 frame->fill_alt_rgba[2],
                                 frame->fill_alt_rgba[3] * frame->opacity);
          }
        }
      }
    }
    break;
  case SHELL_NODE_RECTANGLE:
    if (stroke_width > 0.0f && stroke_a > 0.0f) {
      StygianElement outer = stygian_rect(ctx, frame->x, frame->y, frame->w,
                                          frame->h, stroke_r, stroke_g,
                                          stroke_b, stroke_a);
      if (outer != 0u) {
        stygian_set_radius(ctx, outer, corners[0], corners[1], corners[2],
                           corners[3]);
        shell_canvas_apply_effect(ctx, outer, frame);
      }
      inset = stroke_width;
    }
    if (fill_a > 0.0f) {
      float inner_x = frame->x + inset;
      float inner_y = frame->y + inset;
      float inner_w = frame->w - inset * 2.0f;
      float inner_h = frame->h - inset * 2.0f;
      if (inner_w > 0.0f && inner_h > 0.0f) {
        StygianElement inner = stygian_rect(ctx, inner_x, inner_y, inner_w, inner_h,
                                            fill_r, fill_g, fill_b, fill_a);
        if (inner != 0u) {
          for (i = 0; i < 4; ++i) {
            inner_corners[i] = corners[i] - inset;
            if (inner_corners[i] < 0.0f)
              inner_corners[i] = 0.0f;
          }
          stygian_set_radius(ctx, inner, inner_corners[0], inner_corners[1],
                             inner_corners[2], inner_corners[3]);
          if (frame->fill_mode == SHELL_FILL_LINEAR) {
            stygian_set_gradient(ctx, inner, frame->fill_angle_deg,
                                 fill_r, fill_g, fill_b, fill_a,
                                 frame->fill_alt_rgba[0], frame->fill_alt_rgba[1],
                                 frame->fill_alt_rgba[2],
                                 frame->fill_alt_rgba[3] * frame->opacity);
          }
        }
      }
    }
    break;
  case SHELL_NODE_FRAME:
  default:
    if (stroke_width > 0.0f && stroke_a > 0.0f) {
      StygianElement outer = stygian_rect(ctx, frame->x, frame->y, frame->w,
                                          frame->h, stroke_r, stroke_g,
                                          stroke_b, stroke_a);
      if (outer != 0u) {
        stygian_set_radius(ctx, outer, corners[0], corners[1], corners[2],
                           corners[3]);
        shell_canvas_apply_effect(ctx, outer, frame);
      }
      if (frame->top_flat) {
        stygian_rect(ctx, frame->x, frame->y, frame->w, corners[0] + 2.0f, stroke_r,
                     stroke_g, stroke_b, stroke_a);
      }
      inset = stroke_width;
    }
    if (fill_a > 0.0f) {
      float inner_x = frame->x + inset;
      float inner_y = frame->y + inset;
      float inner_w = frame->w - inset * 2.0f;
      float inner_h = frame->h - inset * 2.0f;
      if (inner_w > 0.0f && inner_h > 0.0f) {
        StygianElement inner = stygian_rect(ctx, inner_x, inner_y, inner_w, inner_h,
                                            fill_r, fill_g, fill_b, fill_a);
        if (inner != 0u) {
          for (i = 0; i < 4; ++i) {
            inner_corners[i] = corners[i] - inset;
            if (inner_corners[i] < 0.0f)
              inner_corners[i] = 0.0f;
          }
          stygian_set_radius(ctx, inner, inner_corners[0], inner_corners[1],
                             inner_corners[2], inner_corners[3]);
          if (frame->fill_mode == SHELL_FILL_LINEAR) {
            stygian_set_gradient(ctx, inner, frame->fill_angle_deg,
                                 fill_r, fill_g, fill_b, fill_a,
                                 frame->fill_alt_rgba[0], frame->fill_alt_rgba[1],
                                 frame->fill_alt_rgba[2],
                                 frame->fill_alt_rgba[3] * frame->opacity);
          }
          if (frame->top_flat) {
            stygian_rect(ctx, inner_x, inner_y, inner_w, inner_corners[0] + 1.0f,
                         fill_r, fill_g, fill_b, fill_a);
          }
        }
      }
    }
    break;
  }
}

static float shell_canvas_ruler_step(const ShellCanvasViewState *view) {
  float zoom = view && view->zoom > 0.0f ? view->zoom : 1.0f;
  float desired_px = 80.0f;
  float world = desired_px / zoom;
  float steps[] = {25.0f, 50.0f, 100.0f, 200.0f, 400.0f, 800.0f};
  size_t i = 0u;
  for (i = 0u; i < sizeof(steps) / sizeof(steps[0]); ++i) {
    if (world <= steps[i])
      return steps[i];
  }
  return 1600.0f;
}

static void shell_draw_canvas_rulers(StygianContext *ctx, StygianFont font,
                                     const ShellLayout *layout,
                                     float left_panel_width,
                                     float right_panel_width,
                                     const ShellCanvasViewState *view) {
  float stage_x = 0.0f;
  float stage_y = 0.0f;
  float stage_w = 0.0f;
  float stage_h = 0.0f;
  float step = 0.0f;
  float world_left = 0.0f;
  float world_top = 0.0f;
  float world_right = 0.0f;
  float world_bottom = 0.0f;
  float tick = 0.0f;
  char label[32];
  if (!ctx || !font || !layout)
    return;
  shell_canvas_stage_rect(layout, left_panel_width, right_panel_width, &stage_x,
                          &stage_y, &stage_w, &stage_h);
  shell_canvas_screen_to_world(layout, left_panel_width, right_panel_width, view,
                               stage_x, stage_y, &world_left, &world_top);
  shell_canvas_screen_to_world(layout, left_panel_width, right_panel_width, view,
                               stage_x + stage_w, stage_y + stage_h, &world_right,
                               &world_bottom);
  step = shell_canvas_ruler_step(view);
  stygian_rect_rounded(ctx, stage_x, stage_y - 24.0f, stage_w, 22.0f, 0.10f, 0.10f,
                       0.11f, 0.96f, 8.0f);
  stygian_rect_rounded(ctx, stage_x - 24.0f, stage_y, 22.0f, stage_h, 0.10f, 0.10f,
                       0.11f, 0.96f, 8.0f);
  for (tick = floorf(world_left / step) * step; tick <= world_right + step;
       tick += step) {
    float sx = 0.0f;
    float sy = 0.0f;
    shell_canvas_world_to_screen(layout, left_panel_width, right_panel_width, view,
                                 tick, 0.0f, &sx, &sy);
    stygian_rect(ctx, sx, stage_y - 8.0f, 1.0f, 6.0f, 0.58f, 0.60f, 0.66f, 0.70f);
    snprintf(label, sizeof(label), "%.0f", tick);
    stygian_text(ctx, font, label, sx + 4.0f, stage_y - 20.0f, 10.0f, 0.72f, 0.74f,
                 0.78f, 1.0f);
  }
  for (tick = floorf(world_top / step) * step; tick <= world_bottom + step;
       tick += step) {
    float sx = 0.0f;
    float sy = 0.0f;
    shell_canvas_world_to_screen(layout, left_panel_width, right_panel_width, view,
                                 0.0f, tick, &sx, &sy);
    stygian_rect(ctx, stage_x - 8.0f, sy, 6.0f, 1.0f, 0.58f, 0.60f, 0.66f, 0.70f);
    snprintf(label, sizeof(label), "%.0f", tick);
    stygian_text(ctx, font, label, stage_x - 22.0f, sy + 2.0f, 10.0f, 0.72f, 0.74f,
                 0.78f, 1.0f);
  }
  if (world_left <= 0.0f && world_right >= 0.0f && world_top <= 0.0f &&
      world_bottom >= 0.0f) {
    float ox = 0.0f;
    float oy = 0.0f;
    shell_canvas_world_to_screen(layout, left_panel_width, right_panel_width, view,
                                 0.0f, 0.0f, &ox, &oy);
    stygian_rect_rounded(ctx, ox - 3.0f, oy - 3.0f, 6.0f, 6.0f, 0.22f, 0.63f, 0.98f,
                         1.0f, 3.0f);
    stygian_rect(ctx, ox, stage_y, 1.0f, stage_h, 0.22f, 0.63f, 0.98f, 0.18f);
    stygian_rect(ctx, stage_x, oy, stage_w, 1.0f, 0.22f, 0.63f, 0.98f, 0.18f);
  }
}

static void shell_draw_canvas_frames(StygianContext *ctx, StygianFont font,
                                     const ShellLayout *layout,
                                     const ShellCanvasSceneState *scene,
                                     float left_panel_width,
                                     float right_panel_width,
                                     const ShellCanvasViewState *view,
                                     const ShellCanvasInteractionState *interaction,
                                     const ShellCanvasGuideState *guides,
                                     const ShellToolDockState *dock_state,
                                     const ShellPathDraftState *path_draft,
                                     float mouse_x, float mouse_y) {
  uint32_t i;
  float stage_x = 0.0f;
  float stage_y = 0.0f;
  float stage_w = 0.0f;
  float stage_h = 0.0f;
  int selected_count = 0;
  float selection_x = 0.0f;
  float selection_y = 0.0f;
  float selection_w = 0.0f;
  float selection_h = 0.0f;
  bool has_selection_bounds = false;
  if (!ctx || !font || !layout || !scene)
    return;
  shell_canvas_stage_rect(layout, left_panel_width, right_panel_width, &stage_x,
                          &stage_y, &stage_w, &stage_h);
  selected_count = shell_canvas_scene_selection_count(scene);
  has_selection_bounds =
      shell_canvas_scene_selection_bounds(scene, &selection_x, &selection_y,
                                         &selection_w, &selection_h);
  stygian_clip_push(ctx, layout->canvas_x, layout->canvas_y, layout->canvas_w,
                    layout->canvas_h);
  for (i = 0u; i < scene->frame_count; ++i) {
    const ShellCanvasFrameNode *frame = &scene->frames[i];
    float draw_x = 0.0f;
    float draw_y = 0.0f;
    float draw_w = 0.0f;
    float draw_h = 0.0f;
    bool selected = frame->selected;
    bool active = (int)i == scene->selected_frame;
    bool pushed_transform = false;
    char badge[64];
    char fitted_text[128];
    if (!frame->alive || !shell_canvas_scene_node_effectively_visible(scene, frame))
      continue;
    shell_canvas_frame_screen_rect(layout, left_panel_width, right_panel_width,
                                   view, frame, &draw_x, &draw_y, &draw_w,
                                   &draw_h);
    ShellCanvasFrameNode screen_frame = *frame;
    screen_frame.x = draw_x;
    screen_frame.y = draw_y;
    screen_frame.w = draw_w;
    screen_frame.h = draw_h;
    screen_frame.rotation_deg = frame->rotation_deg;
    screen_frame.radius = frame->radius * (view ? view->zoom : 1.0f);
    screen_frame.stroke_width = frame->stroke_width * (view ? view->zoom : 1.0f);
    if (screen_frame.stroke_width < 1.0f && frame->stroke_width > 0.0f)
      screen_frame.stroke_width = 1.0f;
    pushed_transform = shell_canvas_push_screen_rotation(ctx, &screen_frame);
    if (screen_frame.kind == SHELL_NODE_TEXT) {
      screen_frame.font_size = frame->font_size * (view ? view->zoom : 1.0f);
      screen_frame.line_height =
          frame->line_height > 0.0f ? frame->line_height * (view ? view->zoom : 1.0f)
                                    : 0.0f;
      screen_frame.letter_spacing =
          frame->letter_spacing * (view ? view->zoom : 1.0f);
      if (screen_frame.font_size < 10.0f)
        screen_frame.font_size = 10.0f;
      if (selected) {
        stygian_rect_rounded(ctx, draw_x, draw_y, draw_w, draw_h, 0.14f, 0.19f,
                             0.28f, active ? 0.22f : 0.14f, 8.0f);
      }
      shell_draw_text_block(ctx, font, &screen_frame, draw_x, draw_y, draw_w,
                            draw_h);
    } else if (screen_frame.kind == SHELL_NODE_IMAGE) {
      if (selected) {
        stygian_rect_rounded(ctx, draw_x, draw_y, draw_w, draw_h, 0.14f, 0.19f,
                             0.28f, active ? 0.20f : 0.12f, 8.0f);
      }
      if (shell_canvas_node_load_image(ctx, &screen_frame) &&
          screen_frame.image_texture != 0u) {
        shell_draw_canvas_node_shape(ctx, &screen_frame);
        shell_draw_image_surface(ctx, &screen_frame, draw_x + 2.0f,
                                 draw_y + 2.0f, draw_w - 4.0f, draw_h - 4.0f);
        ((ShellCanvasFrameNode *)frame)->image_texture = screen_frame.image_texture;
        ((ShellCanvasFrameNode *)frame)->image_w = screen_frame.image_w;
        ((ShellCanvasFrameNode *)frame)->image_h = screen_frame.image_h;
      } else {
        shell_draw_canvas_node_shape(ctx, &screen_frame);
        char image_name[STYGIAN_FS_NAME_CAP] = {0};
        stygian_fs_path_filename(frame->asset_path, image_name,
                                 sizeof(image_name));
        shell_copy_fitted_text(fitted_text, sizeof(fitted_text), ctx, font,
                               image_name[0] ? image_name : "Image", draw_w - 20.0f,
                               14.0f);
        stygian_text(ctx, font, fitted_text, draw_x + 10.0f,
                     draw_y + draw_h * 0.5f - 8.0f, 14.0f, 0.78f, 0.80f, 0.84f,
                     1.0f);
      }
    } else if (screen_frame.kind == SHELL_NODE_LINE ||
               screen_frame.kind == SHELL_NODE_ARROW ||
               screen_frame.kind == SHELL_NODE_POLYGON ||
               screen_frame.kind == SHELL_NODE_STAR ||
               screen_frame.kind == SHELL_NODE_PATH) {
      if (selected) {
        stygian_rect_rounded(ctx, draw_x, draw_y, draw_w, draw_h, 0.14f, 0.19f,
                             0.28f, active ? 0.18f : 0.10f, 8.0f);
      }
      shell_draw_canvas_node_shape(ctx, &screen_frame);
      if (screen_frame.kind == SHELL_NODE_PATH && selected) {
        int path_i = 0;
        for (path_i = 0; path_i < screen_frame.point_count; ++path_i) {
          float px = screen_frame.x + screen_frame.path_x[path_i] * screen_frame.w;
          float py = screen_frame.y + screen_frame.path_y[path_i] * screen_frame.h;
          bool point_active = path_i == frame->selected_point;
          stygian_rect_rounded(ctx, px - 4.0f, py - 4.0f, 8.0f, 8.0f,
                               point_active ? 0.22f : 0.14f,
                               point_active ? 0.63f : 0.18f,
                               point_active ? 0.98f : 0.88f, 1.0f, 2.5f);
          stygian_rect_rounded(ctx, px - 2.5f, py - 2.5f, 5.0f, 5.0f, 0.98f,
                               0.98f, 0.99f, point_active ? 1.0f : 0.82f, 2.0f);
        }
      }
    } else {
      shell_draw_canvas_node_shape(ctx, &screen_frame);
      if (selected) {
        float border_alpha = active ? 0.95f : 0.78f;
        stygian_rect(ctx, draw_x, draw_y, draw_w, 1.0f, 0.22f, 0.63f, 0.98f,
                     border_alpha);
        stygian_rect(ctx, draw_x, draw_y + draw_h, draw_w, 1.0f, 0.22f, 0.63f,
                     0.98f, border_alpha);
        stygian_rect(ctx, draw_x, draw_y, 1.0f, draw_h, 0.22f, 0.63f, 0.98f,
                     border_alpha);
        stygian_rect(ctx, draw_x + draw_w, draw_y, 1.0f, draw_h + 1.0f, 0.22f,
                     0.63f, 0.98f, border_alpha);
      }
    }
    if (pushed_transform)
      stygian_transform_pop(ctx);
    if (shell_canvas_scene_node_effectively_locked(scene, frame)) {
      stygian_rect_rounded(ctx, draw_x, draw_y, draw_w, draw_h, 0.08f, 0.08f,
                           0.09f, 0.28f, 8.0f);
    }
    if (frame->kind == SHELL_NODE_FRAME) {
      snprintf(badge, sizeof(badge), "%s  %d x %d", frame->name, frame->preset_w,
               frame->preset_h);
    } else {
      snprintf(badge, sizeof(badge), "%s  %.0f x %.0f", frame->name, frame->w,
               frame->h);
    }
    stygian_rect_rounded(ctx, draw_x, draw_y - 30.0f, 150.0f, 24.0f,
                         0.09f, 0.09f, 0.10f, 0.96f, 10.0f);
    stygian_rect_rounded(ctx, draw_x + 1.0f, draw_y - 29.0f, 148.0f, 22.0f,
                         0.14f, 0.14f, 0.15f, 0.78f, 9.0f);
    shell_copy_fitted_text(badge, sizeof(badge), ctx, font, badge, 134.0f,
                           12.0f);
    stygian_text(ctx, font, badge, draw_x + 10.0f, draw_y - 24.0f, 12.0f,
                 active ? 0.92f : 0.86f, active ? 0.95f : 0.86f,
                 active ? 0.99f : 0.88f, 1.0f);
  }
  if (has_selection_bounds && selected_count > 0) {
    float sx = 0.0f;
    float sy = 0.0f;
    float sw = 0.0f;
    float sh = 0.0f;
    float handle = 10.0f;
    float rotate_x = 0.0f;
    float rotate_y = 0.0f;
    shell_canvas_world_to_screen(layout, left_panel_width, right_panel_width,
                                 view, selection_x, selection_y, &sx, &sy);
    sw = selection_w * (view ? view->zoom : 1.0f);
    sh = selection_h * (view ? view->zoom : 1.0f);
    stygian_rect(ctx, sx, sy, sw, 1.0f, 0.22f, 0.63f, 0.98f, 0.95f);
    stygian_rect(ctx, sx, sy + sh, sw, 1.0f, 0.22f, 0.63f, 0.98f, 0.95f);
    stygian_rect(ctx, sx, sy, 1.0f, sh, 0.22f, 0.63f, 0.98f, 0.95f);
    stygian_rect(ctx, sx + sw, sy, 1.0f, sh + 1.0f, 0.22f, 0.63f, 0.98f, 0.95f);
    stygian_rect_rounded(ctx, sx - 5.0f, sy - 5.0f, handle, handle, 0.22f, 0.63f,
                         0.98f, 1.0f, 2.5f);
    stygian_rect_rounded(ctx, sx + sw - 5.0f, sy - 5.0f, handle, handle, 0.22f,
                         0.63f, 0.98f, 1.0f, 2.5f);
    stygian_rect_rounded(ctx, sx - 5.0f, sy + sh - 5.0f, handle, handle, 0.22f,
                         0.63f, 0.98f, 1.0f, 2.5f);
    stygian_rect_rounded(ctx, sx + sw - 5.0f, sy + sh - 5.0f, handle, handle,
                         0.22f, 0.63f, 0.98f, 1.0f, 2.5f);
    stygian_rect_rounded(ctx, sx + sw * 0.5f - 5.0f, sy - 5.0f, handle, handle,
                         0.22f, 0.63f, 0.98f, 1.0f, 2.5f);
    stygian_rect_rounded(ctx, sx + sw * 0.5f - 5.0f, sy + sh - 5.0f, handle,
                         handle, 0.22f, 0.63f, 0.98f, 1.0f, 2.5f);
    stygian_rect_rounded(ctx, sx - 5.0f, sy + sh * 0.5f - 5.0f, handle, handle,
                         0.22f, 0.63f, 0.98f, 1.0f, 2.5f);
    stygian_rect_rounded(ctx, sx + sw - 5.0f, sy + sh * 0.5f - 5.0f, handle,
                         handle, 0.22f, 0.63f, 0.98f, 1.0f, 2.5f);
    if (selected_count >= 1) {
      rotate_x = sx + sw * 0.5f;
      rotate_y = sy - 28.0f;
      stygian_rect(ctx, rotate_x, sy - 18.0f, 1.0f, 14.0f, 0.22f, 0.63f, 0.98f,
                   0.82f);
      stygian_rect_rounded(ctx, rotate_x - 6.0f, rotate_y - 6.0f, 12.0f, 12.0f,
                           0.22f, 0.63f, 0.98f, 1.0f, 6.0f);
      stygian_rect_rounded(ctx, rotate_x - 3.0f, rotate_y - 3.0f, 6.0f, 6.0f,
                           0.98f, 0.98f, 0.99f, 1.0f, 3.0f);
    }
  }
  if (interaction && interaction->mode == SHELL_CANVAS_INTERACT_CREATE_FRAME &&
      (shell_tool_is_frame_mode(dock_state) ||
       shell_tool_is_primitive_mode(dock_state))) {
    float start_x = 0.0f;
    float start_y = 0.0f;
    float end_x = 0.0f;
    float end_y = 0.0f;
    float min_x = 0.0f;
    float min_y = 0.0f;
    float max_x = 0.0f;
    float max_y = 0.0f;
    shell_canvas_world_to_screen(layout, left_panel_width, right_panel_width,
                                 view, interaction->anchor_world_x,
                                 interaction->anchor_world_y, &start_x, &start_y);
    shell_canvas_world_to_screen(layout, left_panel_width, right_panel_width,
                                 view, interaction->origin_world_x,
                                 interaction->origin_world_y, &end_x, &end_y);
    min_x = start_x < end_x ? start_x : end_x;
    min_y = start_y < end_y ? start_y : end_y;
    max_x = start_x > end_x ? start_x : end_x;
    max_y = start_y > end_y ? start_y : end_y;
    {
      float preview_radius = interaction->create_kind == SHELL_NODE_ELLIPSE
                                 ? ((max_x - min_x) < (max_y - min_y)
                                        ? (max_x - min_x) * 0.5f
                                        : (max_y - min_y) * 0.5f)
                                 : interaction->create_radius;
      ShellCanvasFrameNode preview = {0};
      preview.kind = interaction->create_kind;
      preview.x = min_x;
      preview.y = min_y;
      preview.w = max_x - min_x;
      preview.h = max_y - min_y;
      preview.radius = preview_radius;
      shell_canvas_apply_default_style(&preview);
      preview.opacity = preview.kind == SHELL_NODE_LINE ||
                                preview.kind == SHELL_NODE_ARROW ||
                                preview.kind == SHELL_NODE_POLYGON ||
                                preview.kind == SHELL_NODE_STAR
                            ? 0.85f
                            : 0.30f;
      if (preview.kind == SHELL_NODE_LINE || preview.kind == SHELL_NODE_ARROW ||
          preview.kind == SHELL_NODE_POLYGON || preview.kind == SHELL_NODE_STAR) {
        preview.stroke_rgba[0] = 0.96f;
        preview.stroke_rgba[1] = 0.96f;
        preview.stroke_rgba[2] = 0.98f;
        shell_draw_canvas_node_shape(ctx, &preview);
      } else {
        preview.fill_rgba[0] = 0.96f;
        preview.fill_rgba[1] = 0.96f;
        preview.fill_rgba[2] = 0.98f;
        preview.stroke_rgba[0] = 0.96f;
        preview.stroke_rgba[1] = 0.96f;
        preview.stroke_rgba[2] = 0.98f;
        shell_draw_canvas_node_shape(ctx, &preview);
      }
    }
    stygian_rect(ctx, min_x, min_y, max_x - min_x, 1.0f, 0.22f, 0.63f, 0.98f,
                 1.0f);
  } else if (path_draft && path_draft->active && path_draft->point_count > 0) {
    int point_i = 0;
    float prev_sx = 0.0f;
    float prev_sy = 0.0f;
    float first_sx = 0.0f;
    float first_sy = 0.0f;
    for (point_i = 0; point_i < path_draft->point_count; ++point_i) {
      float sx = 0.0f;
      float sy = 0.0f;
      shell_canvas_world_to_screen(layout, left_panel_width, right_panel_width,
                                   view, path_draft->points_x[point_i],
                                   path_draft->points_y[point_i], &sx, &sy);
      if (point_i == 0) {
        first_sx = sx;
        first_sy = sy;
      } else {
        stygian_line(ctx, prev_sx, prev_sy, sx, sy, 2.0f, 0.96f, 0.96f, 0.98f,
                     0.90f);
      }
      stygian_rect_rounded(ctx, sx - 4.0f, sy - 4.0f, 8.0f, 8.0f,
                           point_i == 0 ? 0.22f : 0.16f, 0.63f,
                           point_i == 0 ? 0.98f : 0.88f, 1.0f, 2.0f);
      prev_sx = sx;
      prev_sy = sy;
    }
    stygian_line(ctx, prev_sx, prev_sy, mouse_x, mouse_y, 1.5f, 0.96f, 0.96f,
                 0.98f, 0.68f);
    if (path_draft->closed && path_draft->point_count > 2)
      stygian_line(ctx, prev_sx, prev_sy, first_sx, first_sy, 2.0f, 0.96f, 0.96f,
                   0.98f, 0.90f);
  } else if (interaction &&
             interaction->mode == SHELL_CANVAS_INTERACT_MARQUEE) {
    float start_x = 0.0f;
    float start_y = 0.0f;
    float end_x = 0.0f;
    float end_y = 0.0f;
    float min_x = 0.0f;
    float min_y = 0.0f;
    float max_x = 0.0f;
    float max_y = 0.0f;
    shell_canvas_world_to_screen(layout, left_panel_width, right_panel_width,
                                 view, interaction->anchor_world_x,
                                 interaction->anchor_world_y, &start_x, &start_y);
    shell_canvas_world_to_screen(layout, left_panel_width, right_panel_width,
                                 view, interaction->origin_world_x,
                                 interaction->origin_world_y, &end_x, &end_y);
    min_x = start_x < end_x ? start_x : end_x;
    min_y = start_y < end_y ? start_y : end_y;
    max_x = start_x > end_x ? start_x : end_x;
    max_y = start_y > end_y ? start_y : end_y;
    stygian_rect_rounded(ctx, min_x, min_y, max_x - min_x, max_y - min_y, 0.22f,
                         0.63f, 0.98f, 0.10f, 10.0f);
    stygian_rect(ctx, min_x, min_y, max_x - min_x, 1.0f, 0.22f, 0.63f, 0.98f,
                 0.95f);
    stygian_rect(ctx, min_x, max_y, max_x - min_x, 1.0f, 0.22f, 0.63f, 0.98f,
                 0.95f);
    stygian_rect(ctx, min_x, min_y, 1.0f, max_y - min_y, 0.22f, 0.63f, 0.98f,
                 0.95f);
    stygian_rect(ctx, max_x, min_y, 1.0f, max_y - min_y, 0.22f, 0.63f, 0.98f,
                 0.95f);
  }
  if (guides && (guides->x_active || guides->y_active)) {
    if (guides->x_active) {
      float gx = 0.0f;
      float gy = 0.0f;
      const char *label = guides->x_from_grid ? "Grid" : "Guide";
      shell_canvas_world_to_screen(layout, left_panel_width, right_panel_width,
                                   view, guides->x_world, 0.0f, &gx, &gy);
      stygian_rect(ctx, gx, stage_y, 1.0f, stage_h,
                   guides->x_from_grid ? 0.36f : 0.22f,
                   guides->x_from_grid ? 0.74f : 0.63f,
                   0.98f, 0.90f);
      stygian_rect_rounded(ctx, gx + 6.0f, stage_y + 12.0f, 44.0f, 18.0f,
                           0.11f, 0.15f, 0.22f, 0.96f, 8.0f);
      stygian_text(ctx, font, label, gx + 12.0f, stage_y + 16.0f, 11.0f,
                   0.95f, 0.95f, 0.98f, 1.0f);
    }
    if (guides->y_active) {
      float gx = 0.0f;
      float gy = 0.0f;
      const char *label = guides->y_from_grid ? "Grid" : "Guide";
      shell_canvas_world_to_screen(layout, left_panel_width, right_panel_width,
                                   view, 0.0f, guides->y_world, &gx, &gy);
      stygian_rect(ctx, stage_x, gy, stage_w, 1.0f,
                   guides->y_from_grid ? 0.36f : 0.22f,
                   guides->y_from_grid ? 0.74f : 0.63f,
                   0.98f, 0.90f);
      stygian_rect_rounded(ctx, stage_x + 12.0f, gy + 6.0f, 44.0f, 18.0f,
                           0.11f, 0.15f, 0.22f, 0.96f, 8.0f);
      stygian_text(ctx, font, label, stage_x + 18.0f, gy + 10.0f, 11.0f,
                   0.95f, 0.95f, 0.98f, 1.0f);
    }
  }
  stygian_clip_pop(ctx);
}

static void shell_tool_dock_begin_anim(ShellToolDockState *state, float to_t,
                                       uint64_t now_ms,
                                       uint32_t duration_ms) {
  if (!state)
    return;
  state->anim_from_t = state->visible_t;
  state->anim_to_t = shell_clamp01(to_t);
  state->anim_started_ms = now_ms;
  state->anim_duration_ms = duration_ms > 0u ? duration_ms : 1u;
  state->animating = (state->anim_from_t != state->anim_to_t);
}

static void shell_tool_dock_step_anim(ShellToolDockState *state,
                                      uint64_t now_ms) {
  float t = 1.0f;
  float eased = 1.0f;
  if (!state)
    return;
  if (!state->animating) {
    state->visible_t = shell_clamp01(state->visible_t);
    return;
  }
  if (now_ms > state->anim_started_ms) {
    t = (float)(now_ms - state->anim_started_ms) /
        (float)(state->anim_duration_ms > 0u ? state->anim_duration_ms : 1u);
  } else {
    t = 0.0f;
  }
  t = shell_clamp01(t);
  eased = 1.0f - (1.0f - t) * (1.0f - t);
  state->visible_t =
      state->anim_from_t + (state->anim_to_t - state->anim_from_t) * eased;
  state->visible_t = shell_clamp01(state->visible_t);
  if (t >= 1.0f) {
    state->visible_t = state->anim_to_t;
    state->animating = false;
  }
}

static bool shell_point_in_rect(float px, float py, float x, float y, float w,
                                float h) {
  return px >= x && py >= y && px < x + w && py < y + h;
}

static const char *shell_fs_type_label(StygianFsEntryType type) {
  switch (type) {
  case STYGIAN_FS_ENTRY_DIRECTORY:
    return "Folder";
  case STYGIAN_FS_ENTRY_FILE:
    return "File";
  case STYGIAN_FS_ENTRY_OTHER:
    return "Other";
  default:
    return "Unknown";
  }
}

static const char *shell_commit_kind_label(ShellDialogKind kind) {
  switch (kind) {
  case SHELL_DIALOG_OPEN_PROJECT:
    return "Project";
  case SHELL_DIALOG_IMPORT_ASSET:
    return "Asset";
  case SHELL_DIALOG_IMPORT_IMAGE:
    return "Image";
  case SHELL_DIALOG_IMPORT_FONT:
    return "Font";
  case SHELL_DIALOG_IMPORT_TEXTURE:
    return "Texture";
  default:
    return "Dialog";
  }
}

static void shell_format_bytes(uint64_t size_bytes, char *out, size_t out_cap) {
  static const char *units[] = {"B", "KB", "MB", "GB", "TB"};
  double value = (double)size_bytes;
  int unit = 0;
  if (!out || out_cap == 0u)
    return;
  while (value >= 1024.0 && unit < 4) {
    value /= 1024.0;
    unit++;
  }
  if (unit == 0) {
    snprintf(out, out_cap, "%llu %s", (unsigned long long)size_bytes, units[unit]);
  } else {
    snprintf(out, out_cap, "%.1f %s", value, units[unit]);
  }
}

static bool shell_path_is_previewable_image(const char *path) {
  return path && path[0] &&
         (stygian_fs_path_has_extension(path, ".png", false) ||
          stygian_fs_path_has_extension(path, ".jpg", false) ||
          stygian_fs_path_has_extension(path, ".jpeg", false) ||
          stygian_fs_path_has_extension(path, ".bmp", false) ||
          stygian_fs_path_has_extension(path, ".tga", false));
}

static bool shell_path_is_font_file(const char *path) {
  return path && path[0] &&
         (stygian_fs_path_has_extension(path, ".ttf", false) ||
          stygian_fs_path_has_extension(path, ".otf", false));
}

static const char *shell_font_format_label(const char *path) {
  if (stygian_fs_path_has_extension(path, ".ttf", false))
    return "TrueType";
  if (stygian_fs_path_has_extension(path, ".otf", false))
    return "OpenType";
  return "Font";
}

static bool shell_path_is_text_previewable(const char *path) {
  return path && path[0] &&
         (stygian_fs_path_has_extension(path, ".json", false) ||
          stygian_fs_path_has_extension(path, ".txt", false) ||
          stygian_fs_path_has_extension(path, ".md", false) ||
          stygian_fs_path_has_extension(path, ".toml", false) ||
          stygian_fs_path_has_extension(path, ".c", false) ||
          stygian_fs_path_has_extension(path, ".h", false) ||
          stygian_fs_path_has_extension(path, ".inl", false));
}

static bool shell_read_text_preview(const char *path, char *out, size_t out_cap) {
  FILE *fp;
  size_t read_count;
  size_t i;
  bool saw_text = false;
  if (!path || !path[0] || !out || out_cap < 2u)
    return false;
  fp = fopen(path, "rb");
  if (!fp)
    return false;
  read_count = fread(out, 1u, out_cap - 1u, fp);
  fclose(fp);
  out[read_count] = '\0';
  for (i = 0u; i < read_count; ++i) {
    unsigned char c = (unsigned char)out[i];
    if (c == '\r') {
      out[i] = ' ';
    } else if (c == '\t') {
      out[i] = ' ';
      saw_text = true;
    } else if (c == '\n') {
      saw_text = true;
    } else if (c < 32u && c != '\n') {
      out[i] = ' ';
    } else {
      saw_text = true;
    }
  }
  return saw_text;
}

static const char *shell_canvas_node_kind_label(ShellCanvasNodeKind kind) {
  switch (kind) {
  case SHELL_NODE_FRAME:
    return "Frame";
  case SHELL_NODE_RECTANGLE:
    return "Rectangle";
  case SHELL_NODE_ELLIPSE:
    return "Ellipse";
  case SHELL_NODE_LINE:
    return "Line";
  case SHELL_NODE_ARROW:
    return "Arrow";
  case SHELL_NODE_POLYGON:
    return "Polygon";
  case SHELL_NODE_STAR:
    return "Star";
  case SHELL_NODE_IMAGE:
    return "Image";
  case SHELL_NODE_PATH:
    return "Path";
  case SHELL_NODE_TEXT:
    return "Text";
  default:
    return "Node";
  }
}

static void shell_copy_fitted_text(char *out, size_t out_cap, StygianContext *ctx,
                                   StygianFont font, const char *text,
                                   float max_w, float size) {
  size_t len = 0u;
  if (!out || out_cap == 0u) {
    return;
  }
  out[0] = '\0';
  if (!ctx || !font || !text || !text[0] || max_w <= 4.0f) {
    return;
  }
  snprintf(out, out_cap, "%s", text);
  if (stygian_text_width(ctx, font, out, size) <= max_w) {
    return;
  }
  len = strlen(out);
  while (len > 0u) {
    if (len >= 3u) {
      out[len - 1u] = '\0';
      out[len - 2u] = '.';
      out[len - 3u] = '.';
      if (stygian_text_width(ctx, font, out, size) <= max_w) {
        return;
      }
    }
    len--;
    out[len] = '\0';
  }
}

static int shell_draw_wrapped_text(StygianContext *ctx, StygianFont font,
                                   const char *text, float x, float y,
                                   float max_w, float size, float line_h,
                                   int max_lines, float r, float g, float b,
                                   float a) {
  const char *cursor = text;
  int lines = 0;
  if (!ctx || !font || !text || !text[0] || max_w <= 4.0f || max_lines <= 0) {
    return 0;
  }
  while (*cursor && lines < max_lines) {
    const char *line_start = cursor;
    const char *scan = cursor;
    const char *best_break = cursor;
    while (*scan && *scan != '\n') {
      char candidate[256];
      size_t span = 0u;
      const char *next_break = scan + 1;
      while (*next_break && *next_break != ' ' && *next_break != '\n') {
        next_break++;
      }
      span = (size_t)(next_break - line_start);
      if (span >= sizeof(candidate)) {
        span = sizeof(candidate) - 1u;
      }
      memcpy(candidate, line_start, span);
      candidate[span] = '\0';
      if (stygian_text_width(ctx, font, candidate, size) > max_w) {
        break;
      }
      best_break = next_break;
      scan = next_break;
      while (*scan == ' ') {
        scan++;
        best_break = scan;
      }
    }
    if (best_break == line_start) {
      char fitted[256];
      shell_copy_fitted_text(fitted, sizeof(fitted), ctx, font, line_start,
                             max_w, size);
      stygian_text(ctx, font, fitted, x, y + lines * line_h, size, r, g, b, a);
      while (*cursor && *cursor != ' ' && *cursor != '\n') {
        cursor++;
      }
    } else {
      char line_buf[256];
      size_t span = (size_t)(best_break - line_start);
      while (span > 0u && line_start[span - 1u] == ' ') {
        span--;
      }
      if (span >= sizeof(line_buf)) {
        span = sizeof(line_buf) - 1u;
      }
      memcpy(line_buf, line_start, span);
      line_buf[span] = '\0';
      stygian_text(ctx, font, line_buf, x, y + lines * line_h, size, r, g, b,
                   a);
      cursor = best_break;
    }
    while (*cursor == ' ') {
      cursor++;
    }
    if (*cursor == '\n') {
      cursor++;
    }
    lines++;
  }
  return lines;
}

typedef struct ShellTextLayoutLine {
  char text[256];
  float width;
} ShellTextLayoutLine;

static float shell_text_line_width(StygianContext *ctx, StygianFont font,
                                   const char *text, float size,
                                   float letter_spacing) {
  size_t len = 0u;
  float width = 0.0f;
  if (!ctx || !font || !text || !text[0])
    return 0.0f;
  width = stygian_text_width(ctx, font, text, size);
  len = strlen(text);
  if (len > 1u && letter_spacing > 0.0f)
    width += (float)(len - 1u) * letter_spacing;
  return width;
}

static int shell_text_layout_lines(StygianContext *ctx, StygianFont font,
                                   const char *text, float max_w, float size,
                                   float letter_spacing, bool wrap,
                                   ShellTextLayoutLine *out_lines,
                                   int max_lines, bool *out_truncated) {
  const char *cursor = text;
  int lines = 0;
  bool truncated = false;
  if (out_truncated)
    *out_truncated = false;
  if (!ctx || !font || !text || !out_lines || max_lines <= 0)
    return 0;
  while (*cursor && lines < max_lines) {
    const char *line_start = cursor;
    const char *best_break = cursor;
    const char *scan = cursor;
    char line_buf[256];
    size_t span = 0u;
    if (!wrap || max_w <= 4.0f) {
      while (*scan && *scan != '\n')
        scan++;
      span = (size_t)(scan - line_start);
    } else {
      while (*scan && *scan != '\n') {
        const char *next_break = scan + 1;
        size_t cand_span = 0u;
        while (*next_break && *next_break != ' ' && *next_break != '\n')
          next_break++;
        cand_span = (size_t)(next_break - line_start);
        if (cand_span >= sizeof(line_buf))
          cand_span = sizeof(line_buf) - 1u;
        memcpy(line_buf, line_start, cand_span);
        line_buf[cand_span] = '\0';
        if (shell_text_line_width(ctx, font, line_buf, size, letter_spacing) >
            max_w) {
          break;
        }
        best_break = next_break;
        scan = next_break;
        while (*scan == ' ') {
          scan++;
          best_break = scan;
        }
      }
      if (best_break == line_start) {
        while (*scan && *scan != ' ' && *scan != '\n')
          scan++;
        span = (size_t)(scan - line_start);
      } else {
        span = (size_t)(best_break - line_start);
      }
    }
    while (span > 0u && line_start[span - 1u] == ' ')
      span--;
    if (span >= sizeof(out_lines[lines].text))
      span = sizeof(out_lines[lines].text) - 1u;
    memcpy(out_lines[lines].text, line_start, span);
    out_lines[lines].text[span] = '\0';
    out_lines[lines].width = shell_text_line_width(
        ctx, font, out_lines[lines].text, size, letter_spacing);
    cursor = line_start + span;
    while (*cursor == ' ')
      cursor++;
    if (*cursor == '\n')
      cursor++;
    lines++;
  }
  if (*cursor)
    truncated = true;
  if (out_truncated)
    *out_truncated = truncated;
  return lines;
}

static StygianElement shell_draw_text_line_weighted(
    StygianContext *ctx, StygianFont font, const char *text, float x, float y,
    float size, uint32_t weight, float r, float g, float b, float a,
    const ShellCanvasFrameNode *frame) {
  StygianElement element = 0u;
  if (!ctx || !font || !text || !text[0])
    return 0u;
  element = stygian_text(ctx, font, text, x, y, size, r, g, b, a);
  if (element != 0u)
    shell_canvas_apply_effect(ctx, element, frame);
  if (weight >= 600u) {
    StygianElement extra = stygian_text(ctx, font, text, x + 0.4f, y, size, r, g,
                                        b, a * 0.72f);
    if (extra != 0u)
      shell_canvas_apply_effect(ctx, extra, frame);
  }
  return element;
}

static void shell_draw_text_block(StygianContext *ctx, StygianFont font,
                                  const ShellCanvasFrameNode *frame, float x,
                                  float y, float w, float h) {
  ShellTextLayoutLine lines[24];
  int line_count = 0;
  int visible_lines = 0;
  int i = 0;
  float pad = 6.0f;
  float draw_w = w - pad * 2.0f;
  float draw_h = h - pad * 2.0f;
  float font_size = 16.0f;
  float line_h = 0.0f;
  float total_h = 0.0f;
  float start_y = 0.0f;
  bool truncated = false;
  bool clip = false;
  if (!ctx || !font || !frame || !frame->text[0] || draw_w <= 4.0f)
    return;
  font_size = frame->font_size > 0.0f ? frame->font_size : 16.0f;
  line_h = frame->line_height > 0.0f ? frame->line_height : font_size * 1.2f;
  if (line_h < font_size)
    line_h = font_size * 1.1f;
  line_count = shell_text_layout_lines(
      ctx, font, frame->text, draw_w, font_size, frame->letter_spacing,
      frame->text_wrap, lines, (int)(sizeof(lines) / sizeof(lines[0])), &truncated);
  if (line_count <= 0)
    return;
  visible_lines = line_count;
  if (frame->text_overflow != SHELL_TEXT_OVERFLOW_VISIBLE && draw_h > 4.0f) {
    int max_lines = (int)floorf(draw_h / line_h);
    if (max_lines < 1)
      max_lines = 1;
    if (visible_lines > max_lines)
      visible_lines = max_lines;
    clip = frame->text_overflow == SHELL_TEXT_OVERFLOW_CLIP ||
           frame->text_overflow == SHELL_TEXT_OVERFLOW_ELLIPSIS;
  }
  if (frame->text_overflow == SHELL_TEXT_OVERFLOW_ELLIPSIS &&
      (truncated || visible_lines < line_count)) {
    char ellipsis[256];
    snprintf(ellipsis, sizeof(ellipsis), "%s...", lines[visible_lines - 1].text);
    shell_copy_fitted_text(lines[visible_lines - 1].text,
                           sizeof(lines[visible_lines - 1].text), ctx, font,
                           ellipsis, draw_w, font_size);
    lines[visible_lines - 1].width = shell_text_line_width(
        ctx, font, lines[visible_lines - 1].text, font_size,
        frame->letter_spacing);
  }
  total_h = (float)visible_lines * line_h;
  start_y = y + pad;
  if (frame->text_align_v == SHELL_TEXT_ALIGN_MIDDLE)
    start_y = y + pad + fmaxf(0.0f, (draw_h - total_h) * 0.5f);
  else if (frame->text_align_v == SHELL_TEXT_ALIGN_BOTTOM)
    start_y = y + pad + fmaxf(0.0f, draw_h - total_h);
  if (clip) {
    stygian_clip_push(ctx, x + pad, y + pad, draw_w, draw_h);
  }
  for (i = 0; i < visible_lines; ++i) {
    float line_x = x + pad;
    if (frame->text_align_h == SHELL_TEXT_ALIGN_CENTER)
      line_x = x + pad + fmaxf(0.0f, (draw_w - lines[i].width) * 0.5f);
    else if (frame->text_align_h == SHELL_TEXT_ALIGN_RIGHT)
      line_x = x + pad + fmaxf(0.0f, draw_w - lines[i].width);
    (void)shell_draw_text_line_weighted(
        ctx, font, lines[i].text, line_x, start_y + (float)i * line_h, font_size,
        frame->font_weight, frame->fill_rgba[0], frame->fill_rgba[1],
        frame->fill_rgba[2], frame->fill_rgba[3] * frame->opacity, frame);
  }
  if (clip)
    stygian_clip_pop(ctx);
}

static void shell_draw_image_surface(StygianContext *ctx,
                                     const ShellCanvasFrameNode *frame, float x,
                                     float y, float w, float h) {
  float u0 = 0.0f;
  float v0 = 0.0f;
  float u1 = 1.0f;
  float v1 = 1.0f;
  float draw_x = x;
  float draw_y = y;
  float draw_w = w;
  float draw_h = h;
  StygianElement element = 0u;
  if (!ctx || !frame || frame->image_texture == 0u)
    return;
  if (frame->image_w > 0 && frame->image_h > 0) {
    float src_aspect = (float)frame->image_w / (float)frame->image_h;
    float dst_aspect = h > 0.0f ? w / h : src_aspect;
    if (frame->image_fit_mode == SHELL_IMAGE_FIT_CONTAIN) {
      if (src_aspect > dst_aspect) {
        draw_h = w / src_aspect;
        draw_y = y + (h - draw_h) * 0.5f;
      } else {
        draw_w = h * src_aspect;
        draw_x = x + (w - draw_w) * 0.5f;
      }
    } else if (frame->image_fit_mode == SHELL_IMAGE_FIT_COVER) {
      if (src_aspect > dst_aspect) {
        float visible = dst_aspect / src_aspect;
        u0 = (1.0f - visible) * 0.5f;
        u1 = u0 + visible;
      } else {
        float visible = src_aspect / dst_aspect;
        v0 = (1.0f - visible) * 0.5f;
        v1 = v0 + visible;
      }
    }
  }
  element = stygian_element(ctx);
  if (!element)
    return;
  stygian_set_type(ctx, element, STYGIAN_TEXTURE);
  stygian_set_bounds(ctx, element, draw_x, draw_y, draw_w, draw_h);
  stygian_set_color(ctx, element, 1.0f, 1.0f, 1.0f, frame->opacity);
  stygian_set_texture(ctx, element, frame->image_texture, u0, v0, u1, v1);
  shell_canvas_apply_effect(ctx, element, frame);
}

static void shell_draw_preview_text_block(StygianContext *ctx, StygianFont font,
                                          float x, float y, float w, float h,
                                          const char *text) {
  const char *cursor = text;
  int line = 0;
  if (!ctx || !font || !text || !text[0])
    return;
  stygian_rect_rounded(ctx, x, y, w, h, 0.10f, 0.10f, 0.11f, 0.98f, 10.0f);
  stygian_rect_rounded(ctx, x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f, 0.15f, 0.15f,
                       0.16f, 0.52f, 9.0f);
  stygian_clip_push(ctx, x + 8.0f, y + 8.0f, w - 16.0f, h - 16.0f);
  while (*cursor && line < 7) {
    char line_buf[96];
    size_t len = 0u;
    while (cursor[len] && cursor[len] != '\n' && len < sizeof(line_buf) - 1u) {
      line_buf[len] = cursor[len];
      len++;
    }
    line_buf[len] = '\0';
    shell_copy_fitted_text(line_buf, sizeof(line_buf), ctx, font, line_buf,
                           w - 24.0f, 12.0f);
    stygian_text(ctx, font, line_buf, x + 12.0f, y + 12.0f + line * 18.0f,
                 12.0f, 0.80f, 0.82f, 0.86f, 1.0f);
    cursor += len;
    if (*cursor == '\n')
      cursor++;
    line++;
  }
  stygian_clip_pop(ctx);
}

static void shell_dialog_release_preview(StygianContext *ctx,
                                         ShellDialogState *state) {
  if (!ctx || !state)
    return;
  if (state->preview_texture != 0u) {
    stygian_texture_destroy(ctx, state->preview_texture);
    state->preview_texture = 0u;
  }
  state->preview_path[0] = '\0';
  state->preview_text[0] = '\0';
  state->preview_image_w = 0;
  state->preview_image_h = 0;
  state->preview_image_comp = 0;
  state->preview_ready = false;
  state->preview_text_ready = false;
  state->preview_failed = false;
}

static void shell_dialog_close(StygianContext *ctx, ShellDialogState *state) {
  if (!state)
    return;
  shell_dialog_release_preview(ctx, state);
  state->open = false;
  state->dragging_resize = false;
  state->ignore_open_click = false;
  state->kind = SHELL_DIALOG_NONE;
}

static void shell_commit_push(ShellCommitState *state, ShellDialogKind kind,
                              const char *path) {
  ShellRecentCommit recent = {0};
  uint32_t i;
  uint32_t dst = 1u;
  if (!state || !path || !path[0])
    return;
  recent.kind = kind;
  snprintf(recent.path, sizeof(recent.path), "%s", path);
  for (i = 0u; i < state->recent_count; ++i) {
    if (strcmp(state->recent[i].path, path) == 0 &&
        state->recent[i].kind == kind) {
      continue;
    }
    if (dst >= SHELL_RECENT_COMMIT_CAP)
      break;
    state->recent[dst++] = state->recent[i];
  }
  state->recent[0] = recent;
  state->recent_count = dst > SHELL_RECENT_COMMIT_CAP ? SHELL_RECENT_COMMIT_CAP : dst;
  state->last_kind = kind;
  snprintf(state->last_path, sizeof(state->last_path), "%s", path);
}

static void shell_commit_remove(ShellCommitState *state, uint32_t index) {
  uint32_t i;
  if (!state || index >= state->recent_count)
    return;
  for (i = index + 1u; i < state->recent_count; ++i) {
    state->recent[i - 1u] = state->recent[i];
  }
  if (state->recent_count > 0u)
    state->recent_count--;
  if (state->recent_count > 0u) {
    state->last_kind = state->recent[0].kind;
    snprintf(state->last_path, sizeof(state->last_path), "%s",
             state->recent[0].path);
  } else {
    state->last_kind = SHELL_DIALOG_NONE;
    state->last_path[0] = '\0';
  }
}

static void shell_dialog_sync_preview(StygianContext *ctx, ShellDialogState *state,
                                      const char *path,
                                      const StygianFsStat *stat) {
  int image_w = 0;
  int image_h = 0;
  int image_comp = 0;
  unsigned char *rgba = NULL;
  StygianTexture tex = 0u;
  bool wants_image_preview = false;
  bool wants_text_preview = false;
  if (!ctx || !state) {
    return;
  }
  wants_image_preview =
      path && path[0] && stat && stat->exists &&
      stat->type == STYGIAN_FS_ENTRY_FILE && shell_path_is_previewable_image(path);
  wants_text_preview =
      path && path[0] && stat && stat->exists &&
      stat->type == STYGIAN_FS_ENTRY_FILE && shell_path_is_text_previewable(path);
  if (!wants_image_preview && !wants_text_preview) {
    shell_dialog_release_preview(ctx, state);
    return;
  }
  if (strcmp(state->preview_path, path) == 0) {
    return;
  }
  shell_dialog_release_preview(ctx, state);
  snprintf(state->preview_path, sizeof(state->preview_path), "%s", path);
  if (wants_text_preview) {
    if (shell_read_text_preview(path, state->preview_text,
                                sizeof(state->preview_text))) {
      state->preview_text_ready = true;
      state->preview_failed = false;
      return;
    }
    state->preview_failed = true;
    return;
  }
  if (!stbi_info(path, &image_w, &image_h, &image_comp)) {
    state->preview_failed = true;
    return;
  }
  state->preview_image_w = image_w;
  state->preview_image_h = image_h;
  state->preview_image_comp = image_comp;
  // This leans on stb_image from the engine tree for now. It keeps the shell
  // honest until we give core a small image probe/load helper.
  rgba = stbi_load(path, &image_w, &image_h, &image_comp, 4);
  if (!rgba) {
    state->preview_failed = true;
    return;
  }
  tex = stygian_texture_create(ctx, image_w, image_h, rgba);
  stbi_image_free(rgba);
  if (tex == 0u) {
    state->preview_failed = true;
    return;
  }
  state->preview_texture = tex;
  state->preview_image_w = image_w;
  state->preview_image_h = image_h;
  state->preview_image_comp = image_comp;
  state->preview_ready = true;
  state->preview_failed = false;
}

static void shell_canvas_import_release(StygianContext *ctx,
                                        ShellCanvasImportState *state) {
  if (!ctx || !state)
    return;
  if (state->preview_texture != 0u) {
    stygian_texture_destroy(ctx, state->preview_texture);
    state->preview_texture = 0u;
  }
  state->kind = SHELL_DIALOG_NONE;
  state->path[0] = '\0';
  state->image_w = 0;
  state->image_h = 0;
  state->image_comp = 0;
  state->preview_ready = false;
  state->preview_failed = false;
}

static void shell_canvas_import_apply(StygianContext *ctx,
                                      ShellCanvasImportState *state,
                                      ShellDialogKind kind, const char *path) {
  int image_w = 0;
  int image_h = 0;
  int image_comp = 0;
  unsigned char *rgba = NULL;
  if (!ctx || !state || !path || !path[0])
    return;
  if (state->kind == kind && strcmp(state->path, path) == 0)
    return;
  shell_canvas_import_release(ctx, state);
  state->kind = kind;
  snprintf(state->path, sizeof(state->path), "%s", path);
  if ((kind == SHELL_DIALOG_IMPORT_IMAGE || kind == SHELL_DIALOG_IMPORT_TEXTURE) &&
      shell_path_is_previewable_image(path) &&
      stbi_info(path, &image_w, &image_h, &image_comp)) {
    rgba = stbi_load(path, &image_w, &image_h, &image_comp, 4);
    if (rgba) {
      state->preview_texture = stygian_texture_create(ctx, image_w, image_h, rgba);
      stbi_image_free(rgba);
      if (state->preview_texture != 0u) {
        state->image_w = image_w;
        state->image_h = image_h;
        state->image_comp = image_comp;
        state->preview_ready = true;
        return;
      }
    }
    state->preview_failed = true;
  }
}

static void shell_canvas_node_release(StygianContext *ctx,
                                      ShellCanvasFrameNode *frame) {
  if (!ctx || !frame)
    return;
  if (frame->image_texture != 0u) {
    stygian_texture_destroy(ctx, frame->image_texture);
    frame->image_texture = 0u;
  }
  frame->image_w = 0;
  frame->image_h = 0;
}

static bool shell_canvas_node_load_image(StygianContext *ctx,
                                         ShellCanvasFrameNode *frame) {
  int image_w = 0;
  int image_h = 0;
  int image_comp = 0;
  unsigned char *rgba = NULL;
  if (!ctx || !frame || frame->kind != SHELL_NODE_IMAGE ||
      !frame->asset_path[0] || frame->image_texture != 0u)
    return frame && frame->image_texture != 0u;
  if (!shell_path_is_previewable_image(frame->asset_path) ||
      !stbi_info(frame->asset_path, &image_w, &image_h, &image_comp))
    return false;
  rgba = stbi_load(frame->asset_path, &image_w, &image_h, &image_comp, 4);
  if (!rgba)
    return false;
  frame->image_texture = stygian_texture_create(ctx, image_w, image_h, rgba);
  stbi_image_free(rgba);
  if (frame->image_texture == 0u)
    return false;
  frame->image_w = image_w;
  frame->image_h = image_h;
  return true;
}

static void shell_draw_panel_block(StygianContext *ctx, float x, float y, float w,
                                   float h, float radius, float tone) {
  stygian_rect_rounded(ctx, x, y, w, h, 0.09f + tone, 0.09f + tone,
                       0.10f + tone, 0.985f, radius);
  stygian_rect_rounded(ctx, x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f,
                       0.14f + tone * 0.5f, 0.14f + tone * 0.5f,
                       0.15f + tone * 0.5f, 0.82f, radius - 1.0f);
}

static void shell_draw_field_box(StygianContext *ctx, StygianFont font, float x,
                                 float y, float w, float h,
                                 const char *label, const char *value,
                                 bool compact) {
  char fitted[256];
  if (!ctx || !font)
    return;
  stygian_rect_rounded(ctx, x, y, w, h, 0.12f, 0.12f, 0.13f, 0.96f, 8.0f);
  stygian_rect_rounded(ctx, x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f, 0.16f,
                       0.16f, 0.17f, 0.52f, 7.0f);
  if (!compact && label && label[0]) {
    shell_copy_fitted_text(fitted, sizeof(fitted), ctx, font, label, w - 20.0f,
                           10.0f);
    stygian_text(ctx, font, fitted, x + 10.0f, y + 5.0f, 10.0f, 0.57f, 0.57f,
                 0.60f, 1.0f);
    shell_copy_fitted_text(fitted, sizeof(fitted), ctx, font,
                           value ? value : "", w - 20.0f, 13.0f);
    stygian_text(ctx, font, fitted, x + 10.0f, y + 18.0f, 13.0f, 0.93f, 0.93f,
                 0.95f, 1.0f);
  } else {
    shell_copy_fitted_text(fitted, sizeof(fitted), ctx, font,
                           value ? value : "", w - 20.0f, 13.0f);
    stygian_text(ctx, font, fitted, x + 10.0f, y + 8.0f, 13.0f, 0.93f, 0.93f,
                 0.95f, 1.0f);
  }
}

static void shell_draw_color_preview(StygianContext *ctx, StygianFont font,
                                     float x, float y, float w, float h,
                                     const char *label,
                                     const float rgba[4]) {
  char value[32];
  float swatch_y = y + 16.0f;
  float swatch_h = h - 18.0f;
  float alpha = rgba ? rgba[3] : 1.0f;
  if (!ctx || !font)
    return;
  stygian_text(ctx, font, label ? label : "", x + 8.0f, y + 4.0f, 10.0f, 0.57f,
               0.57f, 0.60f, 1.0f);
  stygian_rect_rounded(ctx, x, swatch_y, w, swatch_h, 0.12f, 0.12f, 0.13f, 0.96f,
                       8.0f);
  stygian_rect_rounded(ctx, x + 1.0f, swatch_y + 1.0f, w - 2.0f, swatch_h - 2.0f,
                       0.16f, 0.16f, 0.17f, 0.52f, 7.0f);
  stygian_rect_rounded(ctx, x + 8.0f, swatch_y + 7.0f, 22.0f, swatch_h - 14.0f,
                       rgba ? rgba[0] : 1.0f, rgba ? rgba[1] : 1.0f,
                       rgba ? rgba[2] : 1.0f, alpha, 5.0f);
  snprintf(value, sizeof(value), "%02X%02X%02X",
           (int)shell_color_to_u8(rgba ? rgba[0] : 1.0f),
           (int)shell_color_to_u8(rgba ? rgba[1] : 1.0f),
           (int)shell_color_to_u8(rgba ? rgba[2] : 1.0f));
  stygian_text(ctx, font, value, x + 38.0f, swatch_y + 9.0f, 12.0f, 0.93f, 0.93f,
               0.95f, 1.0f);
}

static void shell_draw_section_rule(StygianContext *ctx, float x, float y,
                                    float w) {
  if (!ctx)
    return;
  stygian_rect(ctx, x, y, w, 1.0f, 0.22f, 0.22f, 0.23f, 1.0f);
}

static bool shell_hit_left_panel_handle(const ShellLayout *layout, float mouse_x,
                                        float mouse_y) {
  if (!layout)
    return false;
  return shell_point_in_rect(mouse_x, mouse_y, layout->assets_panel_handle_x,
                             layout->assets_panel_handle_y,
                             layout->assets_panel_handle_w,
                             layout->assets_panel_handle_h);
}

static void shell_draw_assets_panel(StygianContext *ctx, StygianFont font,
                                    const ShellLayout *layout, float mouse_x,
                                    float mouse_y, float panel_width,
                                    bool dragging,
                                    const ShellCommitState *commits,
                                    const ShellCanvasImportState *active_import) {
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
  bool expanded = false;
  bool handle_hover = false;
  float handle_alpha = 0.0f;
  bool hover_import_image = false;
  bool hover_import_font = false;
  bool hover_import_texture = false;
  if (!ctx || !font || !layout)
    return;

  x = layout->assets_panel_x;
  y = layout->assets_panel_y;
  w = layout->assets_panel_w;
  h = layout->assets_panel_h;
  expanded = panel_width > 32.0f;
  handle_hover = shell_hit_left_panel_handle(layout, mouse_x, mouse_y);
  handle_alpha = dragging ? 1.0f : (handle_hover ? 0.98f : 0.84f);

  hover_import_image =
      shell_point_in_rect(mouse_x, mouse_y, x + 14.0f, y + 40.0f, w - 28.0f, 28.0f);
  hover_import_font =
      shell_point_in_rect(mouse_x, mouse_y, x + 14.0f, y + 74.0f, w - 28.0f, 28.0f);
  hover_import_texture =
      shell_point_in_rect(mouse_x, mouse_y, x + 14.0f, y + 108.0f, w - 28.0f,
                          28.0f);

  if (expanded) {
    shell_draw_panel_block(ctx, x, y, w, h, 18.0f, -0.01f);
    stygian_rect(ctx, layout->assets_panel_handle_x, layout->assets_panel_handle_y,
                 layout->assets_panel_handle_w, layout->assets_panel_handle_h, 0.08f,
                 0.08f, 0.09f, 0.98f);
    stygian_rect(ctx, layout->assets_panel_handle_x,
                 layout->assets_panel_handle_y + 1.0f,
                 layout->assets_panel_handle_w - 1.0f,
                 layout->assets_panel_handle_h - 2.0f,
                 handle_hover || dragging ? 0.13f : 0.10f,
                 handle_hover || dragging ? 0.18f : 0.10f,
                 handle_hover || dragging ? 0.24f : 0.11f, 0.90f);

    stygian_text(ctx, font, "ASSETS / IMPORTS", x + 18.0f, y + 16.0f, 14.0f,
                 0.93f, 0.93f, 0.95f, 1.0f);

    stygian_rect_rounded(ctx, x + 14.0f, y + 46.0f, w - 28.0f, 32.0f,
                         hover_import_image ? 0.13f : 0.11f,
                         hover_import_image ? 0.17f : 0.11f,
                         hover_import_image ? 0.23f : 0.12f, 0.96f, 9.0f);
    stygian_rect_rounded(ctx, x + 14.0f, y + 84.0f, w - 28.0f, 32.0f,
                         hover_import_font ? 0.13f : 0.11f,
                         hover_import_font ? 0.17f : 0.11f,
                         hover_import_font ? 0.23f : 0.12f, 0.96f, 9.0f);
    stygian_rect_rounded(ctx, x + 14.0f, y + 122.0f, w - 28.0f, 32.0f,
                         hover_import_texture ? 0.13f : 0.11f,
                         hover_import_texture ? 0.17f : 0.11f,
                         hover_import_texture ? 0.23f : 0.12f, 0.96f, 9.0f);
    stygian_text(ctx, font, "Import image", x + 28.0f, y + 55.0f, 13.0f, 0.92f,
                 0.92f, 0.94f, 1.0f);
    stygian_text(ctx, font, "Import font", x + 28.0f, y + 93.0f, 13.0f, 0.92f,
                 0.92f, 0.94f, 1.0f);
    stygian_text(ctx, font, "Import texture", x + 28.0f, y + 131.0f, 13.0f,
                 0.92f, 0.92f, 0.94f, 1.0f);
    if (active_import && active_import->path[0] &&
        (active_import->kind == SHELL_DIALOG_IMPORT_IMAGE ||
         active_import->kind == SHELL_DIALOG_IMPORT_TEXTURE)) {
      char fitted[STYGIAN_FS_NAME_CAP] = {0};
      char filename[STYGIAN_FS_NAME_CAP] = {0};
      stygian_fs_path_filename(active_import->path, filename, sizeof(filename));
      shell_copy_fitted_text(fitted, sizeof(fitted), ctx, font,
                             filename[0] ? filename : active_import->path,
                             w - 44.0f, 11.0f);
      stygian_text(ctx, font, "Active image", x + 18.0f, y + 162.0f, 10.0f,
                   0.50f, 0.68f, 0.94f, 1.0f);
      stygian_text(ctx, font, fitted, x + 18.0f, y + 174.0f, 11.0f, 0.80f, 0.82f,
                   0.86f, 1.0f);
      stygian_text(ctx, font, "Pick Image in the primitive menu, then place it.",
                   x + 18.0f, y + 188.0f, 11.0f, 0.54f, 0.54f, 0.58f, 1.0f);
    }

    shell_draw_section_rule(ctx, x + 14.0f, y + 206.0f, w - 28.0f);
    stygian_text(ctx, font, "Motion Scaffolds", x + 16.0f, y + 220.0f, 12.0f,
                 0.58f, 0.58f, 0.61f, 1.0f);
    {
      static const char *scaffold_labels[] = {
          "Hover preset", "Press preset", "Build tab set", "Build drawer",
          "Build modal"};
      for (int i = 0; i < 5; ++i) {
        float row_y = y + 240.0f + i * 36.0f;
        bool hovered =
            shell_point_in_rect(mouse_x, mouse_y, x + 14.0f, row_y, w - 28.0f, 30.0f);
        stygian_rect_rounded(ctx, x + 14.0f, row_y, w - 28.0f, 30.0f,
                             hovered ? 0.13f : 0.11f, hovered ? 0.17f : 0.11f,
                             hovered ? 0.23f : 0.12f, 0.96f, 9.0f);
        stygian_text(ctx, font, scaffold_labels[i], x + 26.0f, row_y + 7.0f,
                     12.0f, 0.92f, 0.92f, 0.94f, 1.0f);
      }
    }
    shell_draw_section_rule(ctx, x + 14.0f, y + 430.0f, w - 28.0f);
    stygian_text(ctx, font, "Recent", x + 16.0f, y + 444.0f, 12.0f, 0.58f, 0.58f,
                 0.61f, 1.0f);
    stygian_rect_rounded(ctx, x + 14.0f, y + 464.0f, w - 28.0f,
                         h - 482.0f, 0.11f, 0.11f, 0.12f, 0.94f, 12.0f);
    stygian_rect_rounded(ctx, x + 15.0f, y + 465.0f, w - 30.0f,
                         h - 484.0f, 0.14f, 0.14f, 0.15f, 0.56f, 11.0f);
    if (commits && commits->recent_count > 0u) {
      float row_y = y + 474.0f;
      uint32_t i;
      for (i = 0u; i < commits->recent_count; ++i) {
        char filename[STYGIAN_FS_NAME_CAP] = {0};
        char parent[STYGIAN_FS_PATH_CAP] = {0};
        float row_x = x + 18.0f;
        float row_w = w - 36.0f;
        float remove_x = row_x + row_w - 28.0f;
        bool row_hovered =
            shell_point_in_rect(mouse_x, mouse_y, row_x, row_y, row_w, 50.0f);
        bool remove_hovered = shell_point_in_rect(mouse_x, mouse_y, remove_x,
                                                  row_y + 10.0f, 18.0f, 18.0f);
        bool active_row = active_import && active_import->path[0] &&
                          active_import->kind == commits->recent[i].kind &&
                          strcmp(active_import->path, commits->recent[i].path) == 0;
        stygian_fs_path_filename(commits->recent[i].path, filename,
                                 sizeof(filename));
        stygian_fs_path_parent(commits->recent[i].path, parent, sizeof(parent));
        stygian_rect_rounded(ctx, row_x, row_y, row_w, 50.0f,
                             active_row ? 0.14f : (row_hovered ? 0.14f : 0.13f),
                             active_row ? 0.20f : (row_hovered ? 0.16f : 0.13f),
                             active_row ? 0.30f : (row_hovered ? 0.22f : 0.14f),
                             0.96f, 10.0f);
        stygian_rect_rounded(ctx, row_x + 1.0f, row_y + 1.0f, row_w - 2.0f, 48.0f,
                             active_row ? 0.18f : 0.16f, active_row ? 0.24f : 0.16f,
                             active_row ? 0.32f : 0.17f,
                             active_row ? 0.60f : 0.50f, 9.0f);
        stygian_text(ctx, font, shell_commit_kind_label(commits->recent[i].kind),
                     x + 30.0f, row_y + 7.0f, 10.0f, 0.48f, 0.68f, 0.94f, 1.0f);
        stygian_text(ctx, font, filename[0] ? filename : commits->recent[i].path,
                     x + 30.0f, row_y + 21.0f, 13.0f, 0.92f, 0.92f, 0.94f, 1.0f);
        stygian_text(ctx, font, parent[0] ? parent : "(root)", x + 30.0f,
                     row_y + 36.0f, 11.0f, 0.52f, 0.52f, 0.55f, 1.0f);
        stygian_rect_rounded(ctx, remove_x, row_y + 10.0f, 18.0f, 18.0f,
                             remove_hovered ? 0.24f : 0.15f,
                             remove_hovered ? 0.12f : 0.15f,
                             remove_hovered ? 0.14f : 0.16f, 0.96f, 6.0f);
        stygian_line(ctx, remove_x + 5.0f, row_y + 15.0f, remove_x + 13.0f,
                     row_y + 23.0f, 1.3f, 0.95f, 0.95f, 0.97f, 1.0f);
        stygian_line(ctx, remove_x + 13.0f, row_y + 15.0f, remove_x + 5.0f,
                     row_y + 23.0f, 1.3f, 0.95f, 0.95f, 0.97f, 1.0f);
        row_y += 58.0f;
        if (row_y + 50.0f > y + h - 14.0f)
          break;
      }
    } else {
      stygian_text(ctx, font, "No imports yet", x + 28.0f, y + 486.0f, 13.0f,
                   0.78f, 0.78f, 0.81f, 1.0f);
      stygian_text(ctx, font, "Bring something in and it lands here.", x + 28.0f,
                   y + 506.0f, 12.0f, 0.50f, 0.50f, 0.53f, 1.0f);
    }
  } else {
    stygian_rect_rounded(ctx, x, y, w, h, 0.08f, 0.08f, 0.09f, 0.98f, 8.0f);
    stygian_rect(ctx, x, y + 8.0f, w, h - 16.0f, 0.08f, 0.08f, 0.09f, 0.98f);
    stygian_rect_rounded(ctx, x, y, w + 1.0f, 22.0f,
                         handle_hover || dragging ? 0.13f : 0.10f,
                         handle_hover || dragging ? 0.18f : 0.10f,
                         handle_hover || dragging ? 0.24f : 0.11f,
                         0.94f, 8.0f);
  }

  stygian_line(ctx, layout->assets_panel_handle_x +
                        layout->assets_panel_handle_w * 0.5f,
               layout->assets_panel_handle_y + 22.0f,
               layout->assets_panel_handle_x +
                        layout->assets_panel_handle_w * 0.5f,
               layout->assets_panel_handle_y + layout->assets_panel_handle_h - 22.0f,
               1.0f, 0.45f, 0.49f, 0.56f, 0.44f + handle_alpha * 0.26f);
  if (expanded) {
    shell_draw_chevron_left(ctx,
                            layout->assets_panel_handle_x +
                                layout->assets_panel_handle_w * 0.5f,
                            layout->assets_panel_handle_y +
                                layout->assets_panel_handle_h * 0.5f,
                            3.0f, 1.25f, 0.88f, 0.88f, 0.90f, handle_alpha);
  } else {
    shell_draw_chevron_right(ctx,
                             layout->assets_panel_handle_x +
                                 layout->assets_panel_handle_w * 0.5f,
                             layout->assets_panel_handle_y +
                                 layout->assets_panel_handle_h * 0.5f,
                             3.0f, 1.25f, 0.88f, 0.88f, 0.90f, handle_alpha);
  }
}

static void shell_draw_workspace_tab_button(StygianContext *ctx, StygianFont font,
                                            float x, float y, float w, float h,
                                            const char *label, bool active,
                                            bool hovered) {
  if (!ctx || !font || !label)
    return;
  stygian_rect_rounded(ctx, x, y, w, h, active ? 0.10f : 0.11f,
                       active ? 0.44f : 0.13f, active ? 0.84f : 0.16f, 0.98f,
                       10.0f);
  stygian_rect_rounded(ctx, x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f,
                       0.16f, active ? 0.54f : 0.17f,
                       active ? 0.94f : (hovered ? 0.24f : 0.18f),
                       active ? 0.22f : (hovered ? 0.28f : 0.18f), 9.0f);
  stygian_text(ctx, font, label, x + 12.0f, y + 8.0f, 12.0f,
               active ? 0.98f : 0.88f, active ? 0.98f : 0.88f,
               active ? 0.99f : 0.90f, 1.0f);
}

static ShellDialogKind shell_asset_kind_dialog_kind(ShellAssetKind kind) {
  switch (kind) {
  case SHELL_ASSET_FONT:
    return SHELL_DIALOG_IMPORT_FONT;
  case SHELL_ASSET_TEXTURE:
    return SHELL_DIALOG_IMPORT_TEXTURE;
  case SHELL_ASSET_IMAGE:
  default:
    return SHELL_DIALOG_IMPORT_IMAGE;
  }
}

static void shell_draw_workspace_panel(
    StygianContext *ctx, StygianFont font, const ShellLayout *layout,
    const ShellFrameState *frame_state, float panel_width, bool dragging,
    ShellCanvasSceneState *scene, ShellWorkspacePanelState *workspace,
    ShellMotionState *motion_state, const ShellCanvasViewState *view,
    float left_panel_width, float right_panel_width, ShellAssetLibrary *assets,
    ShellDialogState *dialog_state, ShellCommitState *commits,
    ShellCanvasImportState *active_import, bool *out_dirty) {
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
  bool expanded = false;
  bool handle_hover = false;
  float handle_alpha = 0.0f;
  bool panel_dirty = false;
  if (!ctx || !font || !layout || !frame_state || !scene || !workspace ||
      !motion_state || !view || !assets || !dialog_state) {
    if (out_dirty)
      *out_dirty = false;
    return;
  }

  x = layout->assets_panel_x;
  y = layout->assets_panel_y;
  w = layout->assets_panel_w;
  h = layout->assets_panel_h;
  expanded = panel_width > 32.0f;
  handle_hover = shell_hit_left_panel_handle(layout, frame_state->mouse_x,
                                             frame_state->mouse_y);
  handle_alpha = dragging ? 1.0f : (handle_hover ? 0.98f : 0.84f);

  if (expanded) {
    float tab_y = y + 14.0f;
    float tab_w = (w - 42.0f) / 3.0f;
    float content_x = x + 14.0f;
    float content_w = w - 28.0f;
    float body_y = y + 52.0f;
    float body_h = h - 66.0f;
    bool hover_pages = shell_point_in_rect(frame_state->mouse_x, frame_state->mouse_y,
                                           content_x, tab_y, tab_w, 30.0f);
    bool hover_layers = shell_point_in_rect(
        frame_state->mouse_x, frame_state->mouse_y, content_x + tab_w + 7.0f,
        tab_y, tab_w, 30.0f);
    bool hover_assets = shell_point_in_rect(
        frame_state->mouse_x, frame_state->mouse_y,
        content_x + (tab_w + 7.0f) * 2.0f, tab_y, tab_w, 30.0f);
    shell_draw_panel_block(ctx, x, y, w, h, 18.0f, -0.01f);
    stygian_rect(ctx, layout->assets_panel_handle_x, layout->assets_panel_handle_y,
                 layout->assets_panel_handle_w, layout->assets_panel_handle_h, 0.08f,
                 0.08f, 0.09f, 0.98f);
    stygian_rect(ctx, layout->assets_panel_handle_x,
                 layout->assets_panel_handle_y + 1.0f,
                 layout->assets_panel_handle_w - 1.0f,
                 layout->assets_panel_handle_h - 2.0f,
                 handle_hover || dragging ? 0.13f : 0.10f,
                 handle_hover || dragging ? 0.18f : 0.10f,
                 handle_hover || dragging ? 0.24f : 0.11f, 0.90f);
    stygian_text(ctx, font, "Workspace", x + 18.0f, y + 16.0f, 14.0f, 0.93f,
                 0.93f, 0.95f, 1.0f);

    if (frame_state->left_pressed) {
      if (hover_pages)
        workspace->active_tab = SHELL_WORKSPACE_TAB_PAGES;
      else if (hover_layers)
        workspace->active_tab = SHELL_WORKSPACE_TAB_LAYERS;
      else if (hover_assets)
        workspace->active_tab = SHELL_WORKSPACE_TAB_ASSETS;
    }

    shell_draw_workspace_tab_button(
        ctx, font, content_x, tab_y, tab_w, 30.0f, "Pages",
        workspace->active_tab == SHELL_WORKSPACE_TAB_PAGES, hover_pages);
    shell_draw_workspace_tab_button(
        ctx, font, content_x + tab_w + 7.0f, tab_y, tab_w, 30.0f, "Layers",
        workspace->active_tab == SHELL_WORKSPACE_TAB_LAYERS, hover_layers);
    shell_draw_workspace_tab_button(
        ctx, font, content_x + (tab_w + 7.0f) * 2.0f, tab_y, tab_w, 30.0f,
        "Assets", workspace->active_tab == SHELL_WORKSPACE_TAB_ASSETS,
        hover_assets);

    if (workspace->active_tab == SHELL_WORKSPACE_TAB_PAGES) {
      ShellCanvasPage *active_page = shell_canvas_scene_active_page(scene);
      float row_y = body_y + 12.0f;
      float list_y = row_y + 88.0f;
      float list_h = body_h - 104.0f;
      float total_h = scene->page_count * 40.0f;
      bool hover_add = shell_point_in_rect(frame_state->mouse_x, frame_state->mouse_y,
                                           content_x, row_y, 92.0f, 30.0f);
      bool hover_delete = shell_point_in_rect(frame_state->mouse_x,
                                              frame_state->mouse_y,
                                              content_x + 100.0f, row_y, 92.0f,
                                              30.0f);
      if (active_page && workspace->editing_name_id != active_page->id) {
        workspace->editing_name_id = active_page->id;
        snprintf(workspace->name_buf, sizeof(workspace->name_buf), "%s",
                 active_page->name);
      }
      stygian_rect_rounded(ctx, content_x, row_y, 92.0f, 30.0f,
                           hover_add ? 0.13f : 0.11f, hover_add ? 0.17f : 0.11f,
                           hover_add ? 0.24f : 0.12f, 0.96f, 9.0f);
      stygian_rect_rounded(ctx, content_x + 100.0f, row_y, 92.0f, 30.0f,
                           hover_delete ? 0.18f : 0.11f,
                           hover_delete ? 0.14f : 0.11f,
                           hover_delete ? 0.16f : 0.12f, 0.96f, 9.0f);
      stygian_text(ctx, font, "Add Page", content_x + 16.0f, row_y + 7.0f, 12.0f,
                   0.94f, 0.94f, 0.96f, 1.0f);
      stygian_text(ctx, font, "Delete", content_x + 128.0f, row_y + 7.0f, 12.0f,
                   0.94f, 0.94f, 0.96f, 1.0f);
      if (frame_state->left_pressed && hover_add) {
        char page_name[32];
        snprintf(page_name, sizeof(page_name), "Page %u", scene->page_count + 1u);
        if (shell_canvas_scene_add_page(scene, page_name)) {
          workspace->editing_name_id = -1;
          panel_dirty = true;
        }
      } else if (frame_state->left_pressed && hover_delete) {
        if (shell_canvas_scene_delete_active_page(ctx, scene)) {
          workspace->editing_name_id = -1;
          panel_dirty = true;
        }
      }

      if (active_page) {
        if (shell_draw_labeled_text_input(ctx, font, content_x, row_y + 42.0f,
                                          content_w, 38.0f, "Name",
                                          workspace->name_buf,
                                          (int)sizeof(workspace->name_buf), true)) {
          snprintf(active_page->name, sizeof(active_page->name), "%s",
                   workspace->name_buf[0] ? workspace->name_buf : "Page");
          panel_dirty = true;
        }
      }
      shell_draw_section_rule(ctx, content_x, list_y - 8.0f, content_w);
      stygian_text(ctx, font, "Document Pages", content_x, list_y - 22.0f, 12.0f,
                   0.58f, 0.58f, 0.61f, 1.0f);
      if (shell_point_in_rect(frame_state->mouse_x, frame_state->mouse_y,
                              content_x, list_y, content_w, list_h) &&
          frame_state->scroll_dy != 0.0f) {
        workspace->pages_scroll_y = shell_clampf(
            workspace->pages_scroll_y - frame_state->scroll_dy * 18.0f, 0.0f,
            total_h > list_h ? total_h - list_h : 0.0f);
      }
      stygian_clip_push(ctx, content_x, list_y, content_w, list_h);
      for (uint32_t i = 0u; i < scene->page_count; ++i) {
        ShellCanvasPage *page = &scene->pages[i];
        float item_y = list_y + i * 40.0f - workspace->pages_scroll_y;
        bool active = page->id == scene->active_page_id;
        bool hovered = shell_point_in_rect(frame_state->mouse_x, frame_state->mouse_y,
                                           content_x, item_y, content_w, 34.0f);
        int node_count = 0;
        char meta[32];
        if (item_y + 34.0f < list_y || item_y > list_y + list_h)
          continue;
        for (uint32_t n = 0u; n < scene->frame_count; ++n) {
          if (scene->frames[n].alive && scene->frames[n].page_id == page->id)
            node_count++;
        }
        stygian_rect_rounded(ctx, content_x, item_y, content_w, 34.0f,
                             active ? 0.11f : 0.10f, active ? 0.24f : 0.11f,
                             active ? 0.48f : (hovered ? 0.18f : 0.12f), 0.96f,
                             10.0f);
        stygian_text(ctx, font, page->name, content_x + 12.0f, item_y + 8.0f,
                     13.0f, 0.94f, 0.94f, 0.96f, 1.0f);
        snprintf(meta, sizeof(meta), "%d nodes", node_count);
        stygian_text(ctx, font, meta, content_x + content_w - 72.0f, item_y + 9.0f,
                     11.0f, 0.62f, 0.64f, 0.68f, 1.0f);
        if (frame_state->left_pressed && hovered) {
          if (shell_canvas_scene_activate_page(scene, page->id))
            panel_dirty = true;
        }
      }
      stygian_clip_pop(ctx);
    } else if (workspace->active_tab == SHELL_WORKSPACE_TAB_LAYERS) {
      ShellCanvasFrameNode *selected = shell_canvas_scene_selected_frame(scene);
      const ShellCanvasFrameNode *parent = NULL;
      float row_y = body_y + 12.0f;
      float list_y = row_y + 150.0f;
      float list_h = body_h - 166.0f;
      float button_w = (content_w - 9.0f) * 0.25f - 0.1f;
      float total_rows = 0.0f;
      if (selected && workspace->active_layer_id != selected->id) {
        workspace->active_layer_id = selected->id;
        workspace->editing_name_id = selected->id;
        snprintf(workspace->name_buf, sizeof(workspace->name_buf), "%s",
                 selected->name);
      }
      for (uint32_t i = 0u; i < scene->frame_count; ++i) {
        if (scene->frames[i].alive &&
            shell_canvas_scene_node_in_active_page(scene, &scene->frames[i])) {
          total_rows += 32.0f;
        }
      }
      {
        const char *labels[4] = {"Back", "Down", "Up", "Front"};
        float bx = content_x;
        for (int i = 0; i < 4; ++i) {
          bool hovered = shell_point_in_rect(frame_state->mouse_x,
                                             frame_state->mouse_y, bx, row_y,
                                             button_w, 30.0f);
          stygian_rect_rounded(ctx, bx, row_y, button_w, 30.0f,
                               hovered ? 0.13f : 0.11f,
                               hovered ? 0.17f : 0.11f,
                               hovered ? 0.24f : 0.12f, 0.96f, 9.0f);
          stygian_text(ctx, font, labels[i], bx + 14.0f, row_y + 7.0f, 12.0f,
                       0.94f, 0.94f, 0.96f, 1.0f);
          if (selected && frame_state->left_pressed && hovered) {
            if (i == 0)
              panel_dirty = shell_canvas_scene_send_selected_to_back(scene);
            else if (i == 1)
              panel_dirty = shell_canvas_scene_reorder_selected(scene, -1);
            else if (i == 2)
              panel_dirty = shell_canvas_scene_reorder_selected(scene, 1);
            else
              panel_dirty = shell_canvas_scene_bring_selected_to_front(scene);
          }
          bx += button_w + 3.0f;
        }
      }
      if (selected) {
        if (shell_draw_labeled_text_input(ctx, font, content_x, row_y + 40.0f,
                                          content_w, 38.0f, "Name",
                                          workspace->name_buf,
                                          (int)sizeof(workspace->name_buf), true)) {
          snprintf(selected->name, sizeof(selected->name), "%s",
                   workspace->name_buf[0] ? workspace->name_buf : "Node");
          panel_dirty = true;
        }
        parent = selected->parent_id > 0
                     ? shell_canvas_scene_find_node_by_id_const(scene,
                                                                selected->parent_id)
                     : NULL;
        shell_draw_field_box(ctx, font, content_x, row_y + 88.0f,
                             content_w * 0.5f - 4.0f, 34.0f, "Parent",
                             parent ? parent->name : "Root", false);
        shell_draw_field_box(ctx, font, content_x + content_w * 0.5f + 4.0f,
                             row_y + 88.0f, content_w * 0.5f - 4.0f, 34.0f,
                             "State",
                             selected->locked ? "Locked"
                                              : (selected->visible ? "Visible"
                                                                   : "Hidden"),
                             false);
        if (frame_state->left_pressed &&
            shell_point_in_rect(frame_state->mouse_x, frame_state->mouse_y,
                                content_x, row_y + 88.0f,
                                content_w * 0.5f - 4.0f, 34.0f)) {
          shell_canvas_scene_cycle_parent(scene, selected);
          panel_dirty = true;
        }
        if (frame_state->left_pressed &&
            shell_point_in_rect(frame_state->mouse_x, frame_state->mouse_y,
                                content_x + content_w * 0.5f + 4.0f,
                                row_y + 88.0f, content_w * 0.5f - 4.0f, 34.0f)) {
          if (!selected->visible) {
            selected->visible = true;
          } else {
            selected->locked = !selected->locked;
          }
          panel_dirty = true;
        }
      } else {
        stygian_text(ctx, font, "Pick a layer to rename, reparent, or reorder it.",
                     content_x, row_y + 52.0f, 12.0f, 0.62f, 0.64f, 0.68f, 1.0f);
      }
      shell_draw_section_rule(ctx, content_x, list_y - 8.0f, content_w);
      stygian_text(ctx, font, "Scene Layers", content_x, list_y - 22.0f, 12.0f,
                   0.58f, 0.58f, 0.61f, 1.0f);
      if (shell_point_in_rect(frame_state->mouse_x, frame_state->mouse_y,
                              content_x, list_y, content_w, list_h) &&
          frame_state->scroll_dy != 0.0f) {
        workspace->layers_scroll_y = shell_clampf(
            workspace->layers_scroll_y - frame_state->scroll_dy * 18.0f, 0.0f,
            total_rows > list_h ? total_rows - list_h : 0.0f);
      }
      stygian_clip_push(ctx, content_x, list_y, content_w, list_h);
      {
        float item_y = list_y - workspace->layers_scroll_y;
        for (int i = (int)scene->frame_count - 1; i >= 0; --i) {
          ShellCanvasFrameNode *node = &scene->frames[i];
          int depth = 0;
          float indent = 0.0f;
          float name_x = 0.0f;
          bool hovered = false;
          bool row_active = false;
          bool eye_hover = false;
          bool lock_hover = false;
          if (!node->alive || !shell_canvas_scene_node_in_active_page(scene, node))
            continue;
          if (item_y + 28.0f >= list_y && item_y <= list_y + list_h) {
            depth = shell_canvas_scene_node_depth(scene, node);
            indent = (float)depth * 12.0f;
            name_x = content_x + 14.0f + indent;
            hovered = shell_point_in_rect(frame_state->mouse_x, frame_state->mouse_y,
                                          content_x, item_y, content_w, 28.0f);
            row_active = node->selected;
            eye_hover = shell_point_in_rect(frame_state->mouse_x,
                                            frame_state->mouse_y,
                                            content_x + content_w - 46.0f, item_y + 6.0f,
                                            14.0f, 14.0f);
            lock_hover = shell_point_in_rect(frame_state->mouse_x,
                                             frame_state->mouse_y,
                                             content_x + content_w - 24.0f,
                                             item_y + 6.0f, 14.0f, 14.0f);
            stygian_rect_rounded(ctx, content_x, item_y, content_w, 28.0f,
                                 row_active ? 0.11f : 0.10f,
                                 row_active ? 0.24f : 0.11f,
                                 row_active ? 0.46f : (hovered ? 0.18f : 0.12f),
                                 0.96f, 8.0f);
            if (node->parent_id > 0) {
              stygian_rect_rounded(ctx, name_x - 10.0f, item_y + 10.0f, 4.0f, 4.0f,
                                   0.54f, 0.56f, 0.62f, 0.88f, 2.0f);
            }
            stygian_text(ctx, font, node->name, name_x, item_y + 7.0f, 12.0f,
                         node->visible ? 0.92f : 0.46f, node->visible ? 0.92f : 0.46f,
                         node->visible ? 0.95f : 0.50f, 1.0f);
            stygian_text(ctx, font, node->visible ? "V" : "-", content_x + content_w - 44.0f,
                         item_y + 7.0f, 12.0f,
                         eye_hover ? 0.98f : 0.72f, eye_hover ? 0.98f : 0.72f,
                         eye_hover ? 0.99f : 0.76f, 1.0f);
            stygian_text(ctx, font, node->locked ? "L" : "-", content_x + content_w - 22.0f,
                         item_y + 7.0f, 12.0f,
                         lock_hover ? 0.98f : 0.72f, lock_hover ? 0.98f : 0.72f,
                         lock_hover ? 0.99f : 0.76f, 1.0f);
            if (frame_state->left_pressed && eye_hover) {
              node->visible = !node->visible;
              if (!node->visible) {
                node->locked = false;
                shell_canvas_scene_clear_selection_for_hidden(scene);
              }
              panel_dirty = true;
            } else if (frame_state->left_pressed && lock_hover) {
              node->visible = true;
              node->locked = !node->locked;
              panel_dirty = true;
            } else if (frame_state->left_pressed && hovered) {
              if (frame_state->shift_down)
                shell_canvas_scene_toggle_selection(scene, i);
              else
                shell_canvas_scene_select_only(scene, i);
              panel_dirty = true;
            }
          }
          item_y += 32.0f;
        }
      }
      stygian_clip_pop(ctx);
    } else {
      float row_y = body_y + 12.0f;
      float list_y = row_y + 116.0f;
      float list_h = body_h - 132.0f;
      float total_h = assets->entry_count * 34.0f + 220.0f;
      bool hover_image = shell_point_in_rect(frame_state->mouse_x, frame_state->mouse_y,
                                             content_x, row_y, content_w, 30.0f);
      bool hover_font = shell_point_in_rect(frame_state->mouse_x, frame_state->mouse_y,
                                            content_x, row_y + 36.0f, content_w,
                                            30.0f);
      bool hover_texture =
          shell_point_in_rect(frame_state->mouse_x, frame_state->mouse_y,
                              content_x, row_y + 72.0f, content_w, 30.0f);
      stygian_rect_rounded(ctx, content_x, row_y, content_w, 30.0f,
                           hover_image ? 0.13f : 0.11f,
                           hover_image ? 0.17f : 0.11f,
                           hover_image ? 0.24f : 0.12f, 0.96f, 9.0f);
      stygian_rect_rounded(ctx, content_x, row_y + 36.0f, content_w, 30.0f,
                           hover_font ? 0.13f : 0.11f,
                           hover_font ? 0.17f : 0.11f,
                           hover_font ? 0.24f : 0.12f, 0.96f, 9.0f);
      stygian_rect_rounded(ctx, content_x, row_y + 72.0f, content_w, 30.0f,
                           hover_texture ? 0.13f : 0.11f,
                           hover_texture ? 0.17f : 0.11f,
                           hover_texture ? 0.24f : 0.12f, 0.96f, 9.0f);
      stygian_text(ctx, font, "Import image", content_x + 14.0f, row_y + 7.0f,
                   12.0f, 0.94f, 0.94f, 0.96f, 1.0f);
      stygian_text(ctx, font, "Import font", content_x + 14.0f, row_y + 43.0f,
                   12.0f, 0.94f, 0.94f, 0.96f, 1.0f);
      stygian_text(ctx, font, "Import texture", content_x + 14.0f, row_y + 79.0f,
                   12.0f, 0.94f, 0.94f, 0.96f, 1.0f);
      if (frame_state->left_pressed && hover_image) {
        shell_dialog_open(dialog_state, SHELL_DIALOG_IMPORT_IMAGE);
      } else if (frame_state->left_pressed && hover_font) {
        shell_dialog_open(dialog_state, SHELL_DIALOG_IMPORT_FONT);
      } else if (frame_state->left_pressed && hover_texture) {
        shell_dialog_open(dialog_state, SHELL_DIALOG_IMPORT_TEXTURE);
      }
      shell_draw_section_rule(ctx, content_x, list_y - 8.0f, content_w);
      stygian_text(ctx, font, "Imported Assets", content_x, list_y - 22.0f, 12.0f,
                   0.58f, 0.58f, 0.61f, 1.0f);
      if (shell_point_in_rect(frame_state->mouse_x, frame_state->mouse_y,
                              content_x, list_y, content_w, list_h) &&
          frame_state->scroll_dy != 0.0f) {
        workspace->assets_scroll_y = shell_clampf(
            workspace->assets_scroll_y - frame_state->scroll_dy * 18.0f, 0.0f,
            total_h > list_h ? total_h - list_h : 0.0f);
      }
      stygian_clip_push(ctx, content_x, list_y, content_w, list_h);
      {
        float item_y = list_y - workspace->assets_scroll_y;
        const char *section = "";
        for (int kind = 0; kind < 3; ++kind) {
          section = kind == 0 ? "Images" : (kind == 1 ? "Fonts" : "Textures");
          stygian_text(ctx, font, section, content_x, item_y, 12.0f, 0.62f, 0.64f,
                       0.68f, 1.0f);
          item_y += 18.0f;
          for (uint32_t i = 0u; i < assets->entry_count; ++i) {
            ShellAssetEntry *entry = &assets->entries[i];
            bool active = false;
            bool hovered = false;
            if (!entry->alive || (int)entry->kind != kind)
              continue;
            active = (kind == 0 && assets->active_image_id == entry->id) ||
                     (kind == 1 && assets->active_font_id == entry->id) ||
                     (kind == 2 && assets->active_texture_id == entry->id);
            hovered = shell_point_in_rect(frame_state->mouse_x, frame_state->mouse_y,
                                          content_x, item_y, content_w, 28.0f);
            stygian_rect_rounded(ctx, content_x, item_y, content_w, 28.0f,
                                 active ? 0.11f : 0.10f, active ? 0.24f : 0.11f,
                                 active ? 0.46f : (hovered ? 0.18f : 0.12f), 0.96f,
                                 8.0f);
            stygian_text(ctx, font, entry->label, content_x + 12.0f, item_y + 7.0f,
                         12.0f, 0.92f, 0.92f, 0.95f, 1.0f);
            stygian_text(ctx, font, shell_asset_kind_label(entry->kind),
                         content_x + content_w - 62.0f, item_y + 8.0f, 10.0f,
                         0.56f, 0.58f, 0.62f, 1.0f);
            if (frame_state->left_pressed && hovered) {
              shell_canvas_import_apply(ctx, active_import,
                                        shell_asset_kind_dialog_kind(entry->kind),
                                        entry->path);
              if (entry->kind == SHELL_ASSET_FONT)
                assets->active_font_id = entry->id;
              else if (entry->kind == SHELL_ASSET_TEXTURE)
                assets->active_texture_id = entry->id;
              else
                assets->active_image_id = entry->id;
              panel_dirty = true;
            }
            item_y += 32.0f;
          }
          item_y += 10.0f;
        }
        stygian_text(ctx, font, "Templates", content_x, item_y + 2.0f, 12.0f, 0.62f,
                     0.64f, 0.68f, 1.0f);
        item_y += 20.0f;
        {
          const char *labels[5] = {"Hover", "Press", "Tabs", "Drawer", "Modal"};
          for (int i = 0; i < 5; ++i) {
            bool hovered = shell_point_in_rect(frame_state->mouse_x,
                                               frame_state->mouse_y, content_x,
                                               item_y + i * 34.0f, content_w,
                                               28.0f);
            stygian_rect_rounded(ctx, content_x, item_y + i * 34.0f, content_w, 28.0f,
                                 hovered ? 0.13f : 0.11f,
                                 hovered ? 0.17f : 0.11f,
                                 hovered ? 0.24f : 0.12f, 0.96f, 8.0f);
            stygian_text(ctx, font, labels[i], content_x + 12.0f,
                         item_y + i * 34.0f + 6.0f, 12.0f, 0.94f, 0.94f, 0.96f,
                         1.0f);
            if (frame_state->left_pressed && hovered) {
              ShellCanvasFrameNode *selected =
                  shell_canvas_scene_selected_frame(scene);
              bool ok = false;
              if (i == 0 && selected) {
                ok = shell_motion_create_hover_preset(motion_state, selected);
              } else if (i == 1 && selected) {
                ok = shell_motion_create_press_preset(motion_state, selected);
              } else if (i == 2) {
                ok = shell_motion_create_tabs_scaffold(
                    motion_state, scene, layout, left_panel_width,
                    right_panel_width, view);
              } else if (i == 3) {
                ok = shell_motion_create_drawer_scaffold(
                    motion_state, scene, layout, left_panel_width,
                    right_panel_width, view);
              } else if (i == 4) {
                ok = shell_motion_create_modal_scaffold(
                    motion_state, scene, layout, left_panel_width,
                    right_panel_width, view);
              }
              if (ok) {
                motion_state->active_tab = SHELL_PANEL_MOTION;
                panel_dirty = true;
              }
            }
          }
        }
      }
      stygian_clip_pop(ctx);
      if (commits && commits->last_path[0]) {
        stygian_text(ctx, font, "Last import", content_x, y + h - 34.0f, 11.0f,
                     0.50f, 0.52f, 0.56f, 1.0f);
        stygian_text(ctx, font, commits->last_path, content_x + 64.0f,
                     y + h - 34.0f, 11.0f, 0.72f, 0.74f, 0.78f, 1.0f);
      }
    }
  } else {
    stygian_rect_rounded(ctx, x, y, w, h, 0.08f, 0.08f, 0.09f, 0.98f, 8.0f);
    stygian_rect(ctx, x, y + 8.0f, w, h - 16.0f, 0.08f, 0.08f, 0.09f, 0.98f);
    stygian_rect_rounded(ctx, x, y, w + 1.0f, 22.0f,
                         handle_hover || dragging ? 0.13f : 0.10f,
                         handle_hover || dragging ? 0.18f : 0.10f,
                         handle_hover || dragging ? 0.24f : 0.11f,
                         0.94f, 8.0f);
  }

  stygian_line(ctx, layout->assets_panel_handle_x +
                        layout->assets_panel_handle_w * 0.5f,
               layout->assets_panel_handle_y + 22.0f,
               layout->assets_panel_handle_x +
                        layout->assets_panel_handle_w * 0.5f,
               layout->assets_panel_handle_y + layout->assets_panel_handle_h - 22.0f,
               1.0f, 0.45f, 0.49f, 0.56f, 0.44f + handle_alpha * 0.26f);
  if (expanded) {
    shell_draw_chevron_left(ctx,
                            layout->assets_panel_handle_x +
                                layout->assets_panel_handle_w * 0.5f,
                            layout->assets_panel_handle_y +
                                layout->assets_panel_handle_h * 0.5f,
                            3.0f, 1.25f, 0.88f, 0.88f, 0.90f, handle_alpha);
  } else {
    shell_draw_chevron_right(ctx,
                             layout->assets_panel_handle_x +
                                 layout->assets_panel_handle_w * 0.5f,
                             layout->assets_panel_handle_y +
                                 layout->assets_panel_handle_h * 0.5f,
                             3.0f, 1.25f, 0.88f, 0.88f, 0.90f, handle_alpha);
  }
  if (out_dirty)
    *out_dirty = panel_dirty;
}

static void shell_draw_canvas_import_surface(StygianContext *ctx, StygianFont font,
                                             const ShellLayout *layout,
                                             const ShellCanvasImportState *state) {
  float left = 0.0f;
  float right = 0.0f;
  float top = 0.0f;
  float bottom = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
  float x = 0.0f;
  float y = 0.0f;
  char filename[STYGIAN_FS_NAME_CAP] = {0};
  if (!ctx || !font || !layout || !state || !state->path[0] ||
      state->kind == SHELL_DIALOG_OPEN_PROJECT) {
    return;
  }
  left = layout->assets_panel_x + layout->assets_panel_w + 28.0f;
  right = layout->right_panel_x - 28.0f;
  top = layout->canvas_y + 30.0f;
  bottom = layout->canvas_y + layout->canvas_h - 64.0f;
  if (right - left < 180.0f || bottom - top < 140.0f)
    return;
  w = 420.0f;
  h = 280.0f;
  if (w > right - left)
    w = right - left;
  if (h > bottom - top)
    h = bottom - top;
  x = left + (right - left - w) * 0.5f;
  y = top + (bottom - top - h) * 0.5f;
  stygian_fs_path_filename(state->path, filename, sizeof(filename));
  shell_draw_panel_block(ctx, x, y, w, h, 18.0f, -0.01f);
  stygian_text(ctx, font, "Canvas import", x + 18.0f, y + 16.0f, 12.0f, 0.54f,
               0.67f, 0.92f, 1.0f);
  stygian_text(ctx, font, filename[0] ? filename : state->path, x + 18.0f, y + 34.0f,
               16.0f, 0.94f, 0.94f, 0.97f, 1.0f);
  if (state->preview_ready && state->preview_texture != 0u && state->image_w > 0 &&
      state->image_h > 0) {
    float box_x = x + 18.0f;
    float box_y = y + 60.0f;
    float box_w = w - 36.0f;
    float box_h = h - 78.0f;
    float scale_x = box_w / (float)state->image_w;
    float scale_y = box_h / (float)state->image_h;
    float scale = scale_x < scale_y ? scale_x : scale_y;
    float draw_w = (float)state->image_w * scale;
    float draw_h = (float)state->image_h * scale;
    float draw_x = box_x + (box_w - draw_w) * 0.5f;
    float draw_y = box_y + (box_h - draw_h) * 0.5f;
    stygian_rect_rounded(ctx, box_x, box_y, box_w, box_h, 0.10f, 0.10f, 0.11f,
                         0.98f, 12.0f);
    stygian_rect_rounded(ctx, box_x + 1.0f, box_y + 1.0f, box_w - 2.0f, box_h - 2.0f,
                         0.15f, 0.15f, 0.16f, 0.52f, 11.0f);
    stygian_image(ctx, state->preview_texture, draw_x, draw_y, draw_w, draw_h);
  } else if (state->kind == SHELL_DIALOG_IMPORT_FONT) {
    stygian_text(ctx, font, "Imported font linked into the editor session.", x + 18.0f,
                 y + 88.0f, 14.0f, 0.86f, 0.86f, 0.90f, 1.0f);
    stygian_text(ctx, font, "Aa  Hamburgefontsiv", x + 18.0f, y + 128.0f, 28.0f,
                 0.94f, 0.94f, 0.96f, 1.0f);
    stygian_text(ctx, font,
                 "Real imported-face rendering wants direct TTF/OTF ingest in core.",
                 x + 18.0f, y + 168.0f, 12.0f, 0.60f, 0.60f, 0.64f, 1.0f);
  } else {
    stygian_text(ctx, font, "Imported asset is now part of the working shell state.",
                 x + 18.0f, y + 88.0f, 14.0f, 0.86f, 0.86f, 0.90f, 1.0f);
    stygian_text(ctx, font, state->path, x + 18.0f, y + 124.0f, 12.0f, 0.64f, 0.64f,
                 0.68f, 1.0f);
  }
}

static ShellDialogKind shell_hit_assets_dialog(const ShellLayout *layout,
                                               float mouse_x, float mouse_y,
                                               float panel_width) {
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  if (!layout || panel_width <= 32.0f)
    return SHELL_DIALOG_NONE;
  x = layout->assets_panel_x;
  y = layout->assets_panel_y;
  w = layout->assets_panel_w;
  if (shell_point_in_rect(mouse_x, mouse_y, x + 14.0f, y + 46.0f, w - 28.0f,
                          32.0f))
    return SHELL_DIALOG_IMPORT_IMAGE;
  if (shell_point_in_rect(mouse_x, mouse_y, x + 14.0f, y + 84.0f, w - 28.0f,
                          32.0f))
    return SHELL_DIALOG_IMPORT_FONT;
  if (shell_point_in_rect(mouse_x, mouse_y, x + 14.0f, y + 122.0f, w - 28.0f,
                          32.0f))
    return SHELL_DIALOG_IMPORT_TEXTURE;
  return SHELL_DIALOG_NONE;
}

static ShellRecentHit shell_hit_assets_recent(const ShellLayout *layout,
                                              float mouse_x, float mouse_y,
                                              float panel_width,
                                              const ShellCommitState *commits) {
  ShellRecentHit hit = {.index = -1, .remove = false};
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float row_y = 0.0f;
  uint32_t i;
  if (!layout || !commits || commits->recent_count == 0u || panel_width <= 32.0f)
    return hit;
  x = layout->assets_panel_x;
  y = layout->assets_panel_y;
  w = layout->assets_panel_w;
  row_y = y + 474.0f;
  for (i = 0u; i < commits->recent_count; ++i) {
    float row_x = x + 18.0f;
    float row_w = w - 36.0f;
    float remove_x = row_x + row_w - 28.0f;
    if (shell_point_in_rect(mouse_x, mouse_y, remove_x, row_y + 10.0f, 18.0f,
                            18.0f)) {
      hit.index = (int)i;
      hit.remove = true;
      return hit;
    }
    if (shell_point_in_rect(mouse_x, mouse_y, row_x, row_y, row_w, 50.0f)) {
      hit.index = (int)i;
      return hit;
    }
    row_y += 58.0f;
    if (row_y + 50.0f > y + layout->assets_panel_h - 14.0f)
      break;
  }
  return hit;
}

static ShellMotionScaffoldKind shell_hit_assets_scaffold(
    const ShellLayout *layout, float mouse_x, float mouse_y, float panel_width) {
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  int i = 0;
  static const ShellMotionScaffoldKind kinds[] = {
      SHELL_SCAFFOLD_HOVER, SHELL_SCAFFOLD_PRESS, SHELL_SCAFFOLD_TABS,
      SHELL_SCAFFOLD_DRAWER, SHELL_SCAFFOLD_MODAL};
  if (!layout || panel_width <= 32.0f)
    return SHELL_SCAFFOLD_NONE;
  x = layout->assets_panel_x + 14.0f;
  y = layout->assets_panel_y + 240.0f;
  w = layout->assets_panel_w - 28.0f;
  for (i = 0; i < 5; ++i) {
    if (shell_point_in_rect(mouse_x, mouse_y, x, y + i * 36.0f, w, 30.0f))
      return kinds[i];
  }
  return SHELL_SCAFFOLD_NONE;
}

static const char *shell_dialog_title(ShellDialogKind kind) {
  switch (kind) {
  case SHELL_DIALOG_OPEN_PROJECT:
    return "Open Project";
  case SHELL_DIALOG_IMPORT_ASSET:
    return "Import Asset";
  case SHELL_DIALOG_IMPORT_IMAGE:
    return "Import Image";
  case SHELL_DIALOG_IMPORT_FONT:
    return "Import Font";
  case SHELL_DIALOG_IMPORT_TEXTURE:
    return "Import Texture";
  default:
    return "Dialog";
  }
}

static const char *shell_dialog_primary_label(ShellDialogKind kind) {
  switch (kind) {
  case SHELL_DIALOG_OPEN_PROJECT:
    return "Open";
  case SHELL_DIALOG_IMPORT_ASSET:
  case SHELL_DIALOG_IMPORT_IMAGE:
  case SHELL_DIALOG_IMPORT_FONT:
  case SHELL_DIALOG_IMPORT_TEXTURE:
    return "Import";
  default:
    return "OK";
  }
}

static const char *shell_dialog_kicker(ShellDialogKind kind) {
  switch (kind) {
  case SHELL_DIALOG_OPEN_PROJECT:
    return "Project flow";
  case SHELL_DIALOG_IMPORT_ASSET:
  case SHELL_DIALOG_IMPORT_IMAGE:
  case SHELL_DIALOG_IMPORT_FONT:
  case SHELL_DIALOG_IMPORT_TEXTURE:
    return "Asset flow";
  default:
    return "Dialog";
  }
}

static const char *shell_dialog_support_line(ShellDialogKind kind) {
  switch (kind) {
  case SHELL_DIALOG_OPEN_PROJECT:
    return "Pick a project file from the in-editor browser. No native picker garbage involved.";
  case SHELL_DIALOG_IMPORT_ASSET:
    return "Generic asset browser. Type-specific filters can layer on top without changing the shell.";
  case SHELL_DIALOG_IMPORT_IMAGE:
    return "Image imports start here. Preview and richer filters can come next.";
  case SHELL_DIALOG_IMPORT_FONT:
    return "Font imports share the same browser spine. Metadata can sit on top later.";
  case SHELL_DIALOG_IMPORT_TEXTURE:
    return "Texture imports share the same browser spine and can grow extra metadata later.";
  default:
    return "";
  }
}

static const char *shell_dialog_path_label(ShellDialogKind kind) {
  switch (kind) {
  case SHELL_DIALOG_OPEN_PROJECT:
    return "Selected project";
  case SHELL_DIALOG_IMPORT_ASSET:
    return "Selected asset";
  case SHELL_DIALOG_IMPORT_IMAGE:
    return "Selected image";
  case SHELL_DIALOG_IMPORT_FONT:
    return "Selected font";
  case SHELL_DIALOG_IMPORT_TEXTURE:
    return "Selected texture";
  default:
    return "Selected path";
  }
}

static void shell_dialog_open(ShellDialogState *state, ShellDialogKind kind) {
  const char *root = ".";
  const char *ext = NULL;
  if (!state || kind == SHELL_DIALOG_NONE)
    return;
  memset(&state->browser, 0, sizeof(state->browser));
  memset(state->browser_root, 0, sizeof(state->browser_root));
  memset(state->suggested_path, 0, sizeof(state->suggested_path));
  memset(state->search_query, 0, sizeof(state->search_query));
  switch (kind) {
  case SHELL_DIALOG_OPEN_PROJECT:
    root = "editor/fixtures";
    ext = ".json";
    break;
  case SHELL_DIALOG_IMPORT_ASSET:
    root = "assets";
    break;
  case SHELL_DIALOG_IMPORT_IMAGE:
    root = "assets/images";
    ext = ".png";
    break;
  case SHELL_DIALOG_IMPORT_FONT:
    root = "assets/fonts";
    ext = ".ttf";
    break;
  case SHELL_DIALOG_IMPORT_TEXTURE:
    root = "assets/textures";
    ext = ".png";
    break;
  default:
    break;
  }
  snprintf(state->browser_root, sizeof(state->browser_root), "%s", root);
  snprintf(state->suggested_path, sizeof(state->suggested_path), "%s", root);
  state->browser.root_path = state->browser_root;
  state->browser.extension_filter = ext;
  state->browser.name_filter = NULL;
  stygian_fs_path_normalize(root, state->browser.current_path,
                            sizeof(state->browser.current_path));
  state->browser.selected_path[0] = '\0';
  state->browser.scroll_y = 0.0f;
  state->dragging_resize = false;
  if (state->dialog_w <= 0.0f)
    state->dialog_w = 760.0f;
  if (state->dialog_h <= 0.0f)
    state->dialog_h = 478.0f;
  state->dialog_x = 0.0f;
  state->dialog_y = 0.0f;
  // Menus and import rows open this on mouse-down. If we don't swallow that
  // first click, the dialog sees its own birth as a backdrop click and bails.
  state->ignore_open_click = true;
  state->kind = kind;
  state->open = true;
}

static void shell_draw_dialog_button(StygianContext *ctx, StygianFont font,
                                     float x, float y, float w, float h,
                                     const char *label, bool hovered,
                                     bool primary, bool enabled) {
  float br = primary ? 0.10f : 0.11f;
  float bg = primary ? 0.45f : 0.12f;
  float bb = primary ? 0.88f : 0.13f;
  float inner_a = hovered ? 0.32f : 0.18f;
  float text_a = 1.0f;
  if (!ctx || !font || !label)
    return;
  if (!enabled) {
    br = 0.10f;
    bg = 0.10f;
    bb = 0.11f;
    inner_a = 0.10f;
    text_a = 0.46f;
    hovered = false;
  }
  stygian_rect_rounded(ctx, x, y, w, h, br, bg, bb, 0.98f, 11.0f);
  stygian_rect_rounded(ctx, x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f,
                       primary ? 0.16f : 0.17f, primary ? 0.56f : 0.17f,
                       primary ? 0.96f : 0.18f, inner_a, 10.0f);
  stygian_text(ctx, font, label, x + 16.0f, y + 9.0f, 13.0f, 0.96f, 0.96f,
               0.98f, text_a);
}

static void shell_draw_dialog(StygianContext *ctx, StygianFont font,
                              const ShellLayout *layout,
                              const ShellFrameState *frame,
                              ShellDialogState *state,
                              ShellCommitState *commit_state,
                              ShellCanvasImportState *canvas_import_state,
                              ShellAssetLibrary *asset_library) {
  float w = 760.0f;
  float h = 478.0f;
  float x = 0.0f;
  float y = 0.0f;
  float close_x = 0.0f;
  float close_y = 0.0f;
  float close_w = 28.0f;
  float close_h = 28.0f;
  float secondary_x = 0.0f;
  float primary_x = 0.0f;
  float button_y = 0.0f;
  float search_x = 0.0f;
  float search_y = 0.0f;
  float search_w = 0.0f;
  float support_y = 0.0f;
  float browser_x = 0.0f;
  float browser_y = 0.0f;
  float browser_w = 0.0f;
  float browser_h = 0.0f;
  float preview_x = 0.0f;
  float preview_y = 0.0f;
  float preview_w = 0.0f;
  float preview_h = 0.0f;
  float selection_y = 0.0f;
  float grip_x = 0.0f;
  float grip_y = 0.0f;
  float grip_s = 18.0f;
  char fitted[256];
  bool hover_header = false;
  bool hover_close = false;
  bool hover_primary = false;
  bool hover_secondary = false;
  bool hover_grip = false;
  bool inside = false;
  const char *active_path = NULL;
  StygianFsStat active_stat = {0};
  bool active_exists = false;
  bool can_commit = false;
  if (!ctx || !font || !layout || !frame || !state || !state->open)
    return;

  w = shell_clampf(state->dialog_w > 0.0f ? state->dialog_w : w, 620.0f,
                   layout->canvas_w - 48.0f);
  h = shell_clampf(state->dialog_h > 0.0f ? state->dialog_h : h, 420.0f,
                   layout->canvas_h - 48.0f);
  if (state->dialog_x <= 0.0f && state->dialog_y <= 0.0f) {
    state->dialog_x = layout->canvas_x + (layout->canvas_w - w) * 0.5f;
    state->dialog_y = layout->canvas_y + (layout->canvas_h - h) * 0.5f;
  }
  x = shell_clampf(state->dialog_x, layout->canvas_x + 24.0f,
                   layout->canvas_x + layout->canvas_w - w - 24.0f);
  y = shell_clampf(state->dialog_y, layout->canvas_y + 24.0f,
                   layout->canvas_y + layout->canvas_h - h - 24.0f);
  state->dialog_x = x;
  state->dialog_y = y;
  state->dialog_w = w;
  state->dialog_h = h;
  close_x = x + w - 44.0f;
  close_y = y + 18.0f;
  button_y = y + h - 52.0f;
  primary_x = x + w - 134.0f;
  secondary_x = primary_x - 108.0f;
  search_x = x + 18.0f;
  support_y = y + 78.0f;
  search_y = y + 126.0f;
  search_w = w - 36.0f;
  browser_x = x + 18.0f;
  browser_y = y + 170.0f;
  browser_w = 412.0f;
  browser_h = shell_clampf(h - 286.0f, 164.0f, 320.0f);
  preview_x = browser_x + browser_w + 14.0f;
  preview_y = browser_y;
  preview_w = x + w - 18.0f - preview_x;
  preview_h = browser_h;
  selection_y = y + h - 106.0f;
  grip_x = x + w - grip_s - 10.0f;
  grip_y = y + h - grip_s - 10.0f;
  hover_header =
      shell_point_in_rect(frame->mouse_x, frame->mouse_y, x, y, w, 58.0f);
  inside = shell_point_in_rect(frame->mouse_x, frame->mouse_y, x, y, w, h);
  hover_close =
      shell_point_in_rect(frame->mouse_x, frame->mouse_y, close_x, close_y,
                          close_w, close_h);
  hover_secondary =
      shell_point_in_rect(frame->mouse_x, frame->mouse_y, secondary_x, button_y,
                          92.0f, 34.0f);
  hover_primary =
      shell_point_in_rect(frame->mouse_x, frame->mouse_y, primary_x, button_y,
                          110.0f, 34.0f);
  hover_grip =
      shell_point_in_rect(frame->mouse_x, frame->mouse_y, grip_x, grip_y,
                          grip_s, grip_s);

  if (!state->ignore_open_click && frame->left_pressed && hover_header &&
      !hover_close && !hover_grip && !state->dragging_resize) {
    state->dragging_move = true;
    state->drag_start_mouse_x = frame->mouse_x;
    state->drag_start_mouse_y = frame->mouse_y;
    state->drag_start_x = x;
    state->drag_start_y = y;
  }

  if (!state->ignore_open_click && frame->left_pressed && hover_grip) {
    state->dragging_resize = true;
    state->resize_start_mouse_x = frame->mouse_x;
    state->resize_start_mouse_y = frame->mouse_y;
    state->resize_start_w = w;
    state->resize_start_h = h;
  }
  if (state->dragging_resize) {
    if (frame->left_down) {
      w = shell_clampf(state->resize_start_w +
                           (frame->mouse_x - state->resize_start_mouse_x),
                       620.0f, layout->canvas_w - 48.0f);
      h = shell_clampf(state->resize_start_h +
                           (frame->mouse_y - state->resize_start_mouse_y),
                       420.0f, layout->canvas_h - 48.0f);
      state->dialog_w = w;
      state->dialog_h = h;
      close_x = x + w - 44.0f;
      primary_x = x + w - 134.0f;
      secondary_x = primary_x - 108.0f;
      search_w = w - 36.0f;
      browser_w = shell_clampf(412.0f + (w - 760.0f) * 0.35f, 360.0f, 520.0f);
      browser_h = shell_clampf(h - 286.0f, 164.0f, 320.0f);
      preview_x = browser_x + browser_w + 14.0f;
      preview_y = browser_y;
      preview_w = x + w - 18.0f - preview_x;
      preview_h = browser_h;
      selection_y = y + h - 106.0f;
      button_y = y + h - 52.0f;
      grip_x = x + w - grip_s - 10.0f;
      grip_y = y + h - grip_s - 10.0f;
    } else {
      state->dragging_resize = false;
    }
  }

  if (state->dragging_move) {
    if (frame->left_down) {
      x = state->drag_start_x + (frame->mouse_x - state->drag_start_mouse_x);
      y = state->drag_start_y + (frame->mouse_y - state->drag_start_mouse_y);
      x = shell_clampf(x, layout->canvas_x + 24.0f,
                       layout->canvas_x + layout->canvas_w - w - 24.0f);
      y = shell_clampf(y, layout->canvas_y + 24.0f,
                       layout->canvas_y + layout->canvas_h - h - 24.0f);
      state->dialog_x = x;
      state->dialog_y = y;
      close_x = x + w - 44.0f;
      close_y = y + 18.0f;
      button_y = y + h - 52.0f;
      primary_x = x + w - 134.0f;
      secondary_x = primary_x - 108.0f;
      search_x = x + 18.0f;
      support_y = y + 78.0f;
      search_y = y + 126.0f;
      search_w = w - 36.0f;
      browser_x = x + 18.0f;
      browser_y = y + 170.0f;
      preview_x = browser_x + browser_w + 14.0f;
      preview_y = browser_y;
      preview_w = x + w - 18.0f - preview_x;
      preview_h = browser_h;
      selection_y = y + h - 106.0f;
      grip_x = x + w - grip_s - 10.0f;
      grip_y = y + h - grip_s - 10.0f;
    } else {
      state->dragging_move = false;
    }
  }

  if (state->dragging_move || state->dragging_resize) {
    hover_header =
        shell_point_in_rect(frame->mouse_x, frame->mouse_y, x, y, w, 58.0f);
    inside = shell_point_in_rect(frame->mouse_x, frame->mouse_y, x, y, w, h);
    hover_close =
        shell_point_in_rect(frame->mouse_x, frame->mouse_y, close_x, close_y,
                            close_w, close_h);
    hover_secondary =
        shell_point_in_rect(frame->mouse_x, frame->mouse_y, secondary_x, button_y,
                            92.0f, 34.0f);
    hover_primary =
        shell_point_in_rect(frame->mouse_x, frame->mouse_y, primary_x, button_y,
                            110.0f, 34.0f);
    hover_grip =
        shell_point_in_rect(frame->mouse_x, frame->mouse_y, grip_x, grip_y,
                            grip_s, grip_s);
  }

  if (state->ignore_open_click) {
    if (!frame->left_down)
      state->ignore_open_click = false;
  }

  stygian_rect(ctx, layout->shell_x, layout->shell_y, layout->shell_w,
               layout->shell_h, 0.01f, 0.01f, 0.02f, 0.58f);
  shell_draw_panel_block(ctx, x, y, w, h, 20.0f, -0.01f);
  stygian_rect_rounded(ctx, x, y, w, 58.0f, 0.10f, 0.10f, 0.11f, 0.98f, 20.0f);
  if (hover_header || state->dragging_move) {
    float glow = state->dragging_move ? 0.18f : 0.10f;
    stygian_rect_rounded(ctx, x + 1.0f, y + 1.0f, w - 2.0f, 56.0f, 0.16f, 0.18f,
                         0.22f, 0.26f + glow, 19.0f);
    stygian_rect(ctx, x + 18.0f, y + 47.0f, 54.0f, 2.0f, 0.24f, 0.57f, 0.96f,
                 0.64f);
  }
  stygian_rect(ctx, x, y + 58.0f, w, 1.0f, 0.22f, 0.22f, 0.24f, 1.0f);

  stygian_text(ctx, font, shell_dialog_kicker(state->kind), x + 18.0f,
               y + 16.0f, 12.0f, 0.54f, 0.67f, 0.92f, 1.0f);
  stygian_text(ctx, font, shell_dialog_title(state->kind), x + 18.0f,
               y + 34.0f, 20.0f, 0.96f, 0.96f, 0.98f, 1.0f);
  stygian_text(ctx, font, "Drag", close_x - 62.0f, y + 21.0f, 11.0f, 0.49f,
               0.51f, 0.56f, hover_header || state->dragging_move ? 0.94f : 0.58f);
  stygian_line(ctx, close_x - 22.0f, y + 24.0f, close_x - 12.0f, y + 24.0f, 1.0f,
               0.55f, 0.57f, 0.62f, 0.70f);
  stygian_line(ctx, close_x - 22.0f, y + 28.0f, close_x - 12.0f, y + 28.0f, 1.0f,
               0.55f, 0.57f, 0.62f, 0.70f);

  stygian_rect_rounded(ctx, close_x, close_y, close_w, close_h,
                       hover_close ? 0.24f : 0.14f,
                       hover_close ? 0.14f : 0.14f,
                       hover_close ? 0.16f : 0.15f, 0.98f, 9.0f);
  stygian_line(ctx, close_x + 9.0f, close_y + 9.0f, close_x + 19.0f,
               close_y + 19.0f, 1.5f, 0.95f, 0.95f, 0.97f, 1.0f);
  stygian_line(ctx, close_x + 19.0f, close_y + 9.0f, close_x + 9.0f,
               close_y + 19.0f, 1.5f, 0.95f, 0.95f, 0.97f, 1.0f);
  stygian_rect_rounded(ctx, grip_x, grip_y, grip_s, grip_s,
                       hover_grip || state->dragging_resize ? 0.16f : 0.12f,
                       hover_grip || state->dragging_resize ? 0.17f : 0.12f,
                       hover_grip || state->dragging_resize ? 0.20f : 0.13f,
                       0.92f, 7.0f);
  stygian_line(ctx, grip_x + 7.0f, grip_y + 13.0f, grip_x + 13.0f,
               grip_y + 7.0f, 1.2f, 0.72f, 0.74f, 0.78f, 1.0f);
  stygian_line(ctx, grip_x + 4.0f, grip_y + 13.0f, grip_x + 13.0f,
               grip_y + 4.0f, 1.2f, 0.62f, 0.64f, 0.68f, 1.0f);

  stygian_clip_push(ctx, x + 18.0f, support_y - 1.0f, w - 36.0f, 40.0f);
  shell_draw_wrapped_text(ctx, font, shell_dialog_support_line(state->kind),
                          x + 18.0f, support_y, w - 36.0f, 13.0f, 16.0f, 2,
                          0.66f, 0.68f, 0.72f, 1.0f);
  stygian_clip_pop(ctx);

  stygian_text(ctx, font, "Search", search_x, search_y - 14.0f, 12.0f, 0.56f,
               0.58f, 0.62f, 1.0f);
  if (stygian_text_input(ctx, font, search_x, search_y, search_w, 34.0f,
                         state->search_query, (int)sizeof(state->search_query))) {
    state->browser.scroll_y = 0.0f;
  }
  if (!state->search_query[0]) {
    stygian_text(ctx, font, "Filter by name", search_x + 10.0f, search_y + 8.0f,
                 13.0f, 0.46f, 0.46f, 0.49f, 1.0f);
  }

  state->browser.x = browser_x;
  state->browser.y = browser_y;
  state->browser.w = browser_w;
  state->browser.h = browser_h;
  state->browser.name_filter =
      state->search_query[0] ? state->search_query : NULL;
  state->browser.input_mouse_x = frame->mouse_x;
  state->browser.input_mouse_y = frame->mouse_y;
  state->browser.input_scroll_dy = frame->scroll_dy;
  state->browser.input_left_pressed = frame->left_pressed;
  state->browser.input_left_clicks = frame->left_clicks;
  state->browser.input_confirm_pressed = frame->enter_pressed;
  state->browser.input_up_pressed = frame->up_pressed;
  state->browser.input_down_pressed = frame->down_pressed;
  if (stygian_file_explorer(ctx, font, &state->browser)) {
    if (state->browser.selected_path[0]) {
      snprintf(state->suggested_path, sizeof(state->suggested_path), "%s",
               state->browser.selected_path);
    } else if (state->browser.current_path[0]) {
      snprintf(state->suggested_path, sizeof(state->suggested_path), "%s",
               state->browser.current_path);
    }
  }

  active_path = state->browser.selected_path[0] ? state->browser.selected_path
                                                : state->browser.current_path;
  if (!active_path || !active_path[0])
    active_path = state->suggested_path[0] ? state->suggested_path : "(unset)";
  if (state->browser.activated_path[0]) {
    active_path = state->browser.activated_path;
  }
  active_exists = stygian_fs_stat(active_path, &active_stat);
  can_commit =
      active_exists && active_stat.exists &&
      active_stat.type == STYGIAN_FS_ENTRY_FILE;
  shell_dialog_sync_preview(ctx, state, active_path, &active_stat);

  {
    char filename[STYGIAN_FS_NAME_CAP] = {0};
    char size_text[32] = {0};
    char meta_text[128] = {0};
    char channels_text[24] = {0};
    stygian_fs_path_filename(active_path, filename, sizeof(filename));
    shell_draw_panel_block(ctx, preview_x, preview_y, preview_w, preview_h, 14.0f,
                           -0.01f);
    stygian_text(ctx, font, "Preview", preview_x + 14.0f, preview_y + 14.0f,
                 12.0f, 0.56f, 0.58f, 0.62f, 1.0f);
    if (!active_exists || !active_stat.exists || !can_commit) {
      stygian_clip_push(ctx, preview_x + 18.0f, preview_y + 40.0f,
                        preview_w - 36.0f, 54.0f);
      shell_draw_wrapped_text(ctx, font, "Pick a file to inspect it here.",
                              preview_x + 18.0f, preview_y + 42.0f,
                              preview_w - 36.0f, 13.0f, 16.0f, 2, 0.78f, 0.79f,
                              0.82f, 1.0f);
      shell_draw_wrapped_text(
          ctx, font,
          "Folders still browse fine, but the action button only commits files.",
          preview_x + 18.0f, preview_y + 60.0f, preview_w - 36.0f, 11.0f, 14.0f,
          3, 0.50f, 0.50f, 0.53f, 1.0f);
      stygian_clip_pop(ctx);
    } else {
      shell_format_bytes(active_stat.size_bytes, size_text, sizeof(size_text));
      shell_copy_fitted_text(fitted, sizeof(fitted), ctx, font,
                             filename[0] ? filename : active_path,
                             preview_w - 36.0f, 15.0f);
      stygian_text(ctx, font, fitted, preview_x + 18.0f, preview_y + 40.0f,
                   15.0f, 0.94f, 0.94f, 0.97f, 1.0f);
      if (state->preview_ready && state->preview_image_w > 0 &&
          state->preview_image_h > 0) {
        snprintf(meta_text, sizeof(meta_text), "%s  |  %s  |  %d x %d",
                 shell_fs_type_label(active_stat.type), size_text,
                 state->preview_image_w, state->preview_image_h);
      } else {
        snprintf(meta_text, sizeof(meta_text), "%s  |  %s",
                 shell_fs_type_label(active_stat.type), size_text);
      }
      shell_copy_fitted_text(fitted, sizeof(fitted), ctx, font, meta_text,
                             preview_w - 36.0f, 11.0f);
      stygian_text(ctx, font, fitted, preview_x + 18.0f, preview_y + 60.0f,
                   11.0f, 0.62f, 0.64f, 0.68f, 1.0f);
      if (state->preview_ready && state->preview_texture != 0u &&
          state->preview_image_w > 0 && state->preview_image_h > 0) {
        float box_x = preview_x + 18.0f;
        float box_y = preview_y + 82.0f;
        float box_w = preview_w - 36.0f;
        float box_h = preview_h - 100.0f;
        float scale_x = box_w / (float)state->preview_image_w;
        float scale_y = box_h / (float)state->preview_image_h;
        float scale = scale_x < scale_y ? scale_x : scale_y;
        float draw_w = (float)state->preview_image_w * scale;
        float draw_h = (float)state->preview_image_h * scale;
        float draw_x = box_x + (box_w - draw_w) * 0.5f;
        float draw_y = box_y + (box_h - draw_h) * 0.5f;
        char dims[32];
        snprintf(channels_text, sizeof(channels_text), "%d channels",
                 state->preview_image_comp > 0 ? state->preview_image_comp : 4);
        snprintf(dims, sizeof(dims), "%d x %d", state->preview_image_w,
                 state->preview_image_h);
        stygian_rect_rounded(ctx, box_x, box_y, box_w, box_h, 0.10f, 0.10f, 0.11f,
                             0.98f, 10.0f);
        stygian_rect_rounded(ctx, box_x + 1.0f, box_y + 1.0f, box_w - 2.0f,
                             box_h - 2.0f, 0.15f, 0.15f, 0.16f, 0.52f, 9.0f);
        stygian_image(ctx, state->preview_texture, draw_x, draw_y, draw_w, draw_h);
        stygian_text(ctx, font, dims, box_x + 10.0f, box_y + box_h - 20.0f, 11.0f,
                     0.74f, 0.74f, 0.77f, 1.0f);
        stygian_text(ctx, font, channels_text, box_x + box_w - 88.0f,
                     box_y + box_h - 20.0f, 11.0f, 0.74f, 0.74f, 0.77f, 1.0f);
      } else if (state->preview_text_ready) {
        shell_draw_preview_text_block(ctx, font, preview_x + 18.0f,
                                      preview_y + 82.0f, preview_w - 36.0f,
                                      preview_h - 100.0f, state->preview_text);
      } else if (state->preview_failed) {
        stygian_text(ctx, font, "Preview load failed.", preview_x + 18.0f,
                     preview_y + 88.0f, 13.0f, 0.82f, 0.63f, 0.63f, 1.0f);
      } else if (shell_path_is_font_file(active_path)) {
        shell_draw_field_box(ctx, font, preview_x + 18.0f, preview_y + 84.0f,
                             preview_w - 36.0f, 34.0f, "Format",
                             shell_font_format_label(active_path), false);
        stygian_text(ctx, font, "Font metadata preview", preview_x + 30.0f,
                     preview_y + 136.0f, 13.0f, 0.84f, 0.85f, 0.88f, 1.0f);
        stygian_clip_push(ctx, preview_x + 30.0f, preview_y + 154.0f,
                          preview_w - 48.0f, preview_h - 168.0f);
        shell_draw_wrapped_text(
            ctx, font,
            "Live glyph rendering wants a direct TTF/OTF ingest path in core.",
            preview_x + 30.0f, preview_y + 158.0f, preview_w - 48.0f, 11.0f,
            14.0f, 2, 0.56f, 0.56f, 0.60f, 1.0f);
        shell_draw_wrapped_text(
            ctx, font,
            "This keeps the browser honest until that exists.",
            preview_x + 30.0f, preview_y + 188.0f, preview_w - 48.0f, 11.0f,
            14.0f, 2, 0.56f, 0.56f, 0.60f, 1.0f);
        stygian_clip_pop(ctx);
      } else {
        stygian_clip_push(ctx, preview_x + 18.0f, preview_y + 84.0f,
                          preview_w - 36.0f, 40.0f);
        shell_draw_wrapped_text(ctx, font,
                                "No inline preview for this file type yet.",
                                preview_x + 18.0f, preview_y + 88.0f,
                                preview_w - 36.0f, 13.0f, 16.0f, 2, 0.78f,
                                0.79f, 0.82f, 1.0f);
        stygian_clip_pop(ctx);
      }
    }
  }

  stygian_text(ctx, font, shell_dialog_path_label(state->kind), x + 18.0f,
               selection_y, 12.0f, 0.56f, 0.58f, 0.62f, 1.0f);
  shell_draw_field_box(ctx, font, x + 18.0f, selection_y + 18.0f, w - 36.0f,
                       32.0f, "", active_path, true);

  shell_draw_dialog_button(ctx, font, secondary_x, button_y, 92.0f, 34.0f,
                           "Cancel", hover_secondary, false, true);
  shell_draw_dialog_button(ctx, font, primary_x, button_y, 110.0f, 34.0f,
                           shell_dialog_primary_label(state->kind),
                           hover_primary, true, can_commit);

  if (frame->esc_pressed) {
    shell_dialog_close(ctx, state);
  } else if (!state->ignore_open_click && frame->left_pressed && !inside) {
    shell_dialog_close(ctx, state);
  } else if (!state->ignore_open_click && frame->left_pressed && hover_close) {
    shell_dialog_close(ctx, state);
  } else if (!state->ignore_open_click && frame->left_pressed &&
             hover_secondary) {
    shell_dialog_close(ctx, state);
  } else if (!state->ignore_open_click && state->browser.activated_path[0] &&
             can_commit) {
    snprintf(state->suggested_path, sizeof(state->suggested_path), "%s",
             state->browser.activated_path);
    if (commit_state)
      shell_commit_push(commit_state, state->kind, state->browser.activated_path);
    if (canvas_import_state && state->kind != SHELL_DIALOG_OPEN_PROJECT) {
      shell_canvas_import_apply(ctx, canvas_import_state, state->kind,
                                state->browser.activated_path);
      if (asset_library)
        shell_asset_library_add(asset_library, state->kind,
                                state->browser.activated_path);
    }
    shell_dialog_close(ctx, state);
  } else if (!state->ignore_open_click && frame->left_pressed &&
             hover_primary && can_commit) {
    if (active_path && active_path[0]) {
      snprintf(state->suggested_path, sizeof(state->suggested_path), "%s",
               active_path);
    }
    if (commit_state && active_path && active_path[0]) {
      shell_commit_push(commit_state, state->kind, active_path);
    }
    if (canvas_import_state && state->kind != SHELL_DIALOG_OPEN_PROJECT &&
        active_path && active_path[0]) {
      shell_canvas_import_apply(ctx, canvas_import_state, state->kind, active_path);
      if (asset_library)
        shell_asset_library_add(asset_library, state->kind, active_path);
    }
    shell_dialog_close(ctx, state);
  }
}

static int shell_top_menu_count(void) { return 5; }

static const char *shell_top_menu_label(int index) {
  static const char *labels[] = {"File", "Edit", "View", "Selection", "Help"};
  if (index < 0 || index >= (int)(sizeof(labels) / sizeof(labels[0])))
    return "";
  return labels[index];
}

static float shell_top_menu_gap(void) { return 4.0f; }

static float shell_top_menu_slot_width(int index) {
  switch (index) {
  case 0:
    return 42.0f;
  case 1:
    return 42.0f;
  case 2:
    return 44.0f;
  case 3:
    return 74.0f;
  case 4:
    return 42.0f;
  default:
    return 44.0f;
  }
}

static float shell_top_menu_slot_x(const ShellLayout *layout, int index) {
  float x = 0.0f;
  int i = 0;
  if (!layout)
    return 0.0f;
  x = layout->menu_x;
  for (i = 0; i < index; ++i) {
    x += shell_top_menu_slot_width(i);
    x += shell_top_menu_gap();
  }
  return x;
}

static int shell_menu_item_count(int menu_index) {
  switch (menu_index) {
  case 0:
    return 4;
  case 1:
    return 3;
  case 2:
    return 4;
  case 3:
    return 4;
  case 4:
    return 3;
  default:
    return 0;
  }
}

static const char *shell_menu_item_label(int menu_index, int item_index,
                                         bool show_tool_dock,
                                         bool grid_snap_enabled) {
  switch (menu_index) {
  case 0:
    switch (item_index) {
    case 0:
      return "New Canvas";
    case 1:
      return "Open Project...";
    case 2:
      return "Import Asset...";
    case 3:
      return "Quit";
    default:
      break;
    }
    break;
  case 1:
    switch (item_index) {
    case 0:
      return "Copy Icon Call";
    case 1:
      return "Reset Tool Families";
    case 2:
      return "Close Menu";
    default:
      break;
    }
    break;
  case 2:
    switch (item_index) {
    case 0:
      return "Canvas Workspace";
    case 1:
      return "Icon Workspace";
    case 2:
      return show_tool_dock ? "Hide Tools Dock" : "Show Tools Dock";
    case 3:
      return grid_snap_enabled ? "Disable Grid Snap" : "Enable Grid Snap";
    default:
      break;
    }
    break;
  case 3:
    switch (item_index) {
    case 0:
      return "Select Cursor";
    case 1:
      return "Select Frame";
    case 2:
      return "Select Pen";
    case 3:
      return "Select Text";
    default:
      break;
    }
    break;
  case 4:
    switch (item_index) {
    case 0:
      return "Shell Canvas";
    case 1:
      return "Icon Workspace";
    case 2:
      return "Shortcuts";
    default:
      break;
    }
    break;
  default:
    break;
  }
  return "";
}

static float shell_menu_popup_width(int menu_index) {
  switch (menu_index) {
  case 0:
    return 184.0f;
  case 1:
    return 184.0f;
  case 2:
    return 204.0f;
  case 3:
    return 160.0f;
  case 4:
    return 154.0f;
  default:
    return 164.0f;
  }
}

static void shell_compute_menu_popup_rect(const ShellLayout *layout,
                                          int menu_index, float *out_x,
                                          float *out_y, float *out_w,
                                          float *out_h) {
  float w = 0.0f;
  float h = 0.0f;
  float x = 0.0f;
  float y = 0.0f;
  int count = 0;
  if (!layout || !out_x || !out_y || !out_w || !out_h)
    return;
  w = shell_menu_popup_width(menu_index);
  count = shell_menu_item_count(menu_index);
  h = 10.0f + count * 28.0f;
  x = shell_top_menu_slot_x(layout, menu_index);
  y = layout->shell_y + layout->title_h - 1.0f;
  if (x + w > layout->shell_x + layout->shell_w - 8.0f)
    x = layout->shell_x + layout->shell_w - w - 8.0f;
  *out_x = x;
  *out_y = y;
  *out_w = w;
  *out_h = h;
}

static float shell_top_menu_trailing_x(const ShellLayout *layout) {
  float x = 0.0f;
  int i = 0;
  if (!layout)
    return 0.0f;
  x = layout->menu_x;
  for (i = 0; i < shell_top_menu_count(); ++i) {
    x += shell_top_menu_slot_width(i);
    if (i + 1 < shell_top_menu_count())
      x += shell_top_menu_gap();
  }
  return x;
}

static int shell_hit_top_menu(const ShellLayout *layout, float mouse_x,
                              float mouse_y) {
  float x = 0.0f;
  int i = 0;
  if (!layout)
    return -1;
  x = layout->menu_x;
  for (i = 0; i < shell_top_menu_count(); ++i) {
    float slot_w = shell_top_menu_slot_width(i);
    if (shell_point_in_rect(mouse_x, mouse_y, x, layout->menu_y, slot_w,
                            layout->menu_h))
      return i;
    x += slot_w;
    if (i + 1 < shell_top_menu_count())
      x += shell_top_menu_gap();
  }
  return -1;
}

static int shell_hit_menu_item(const ShellLayout *layout, int menu_index,
                               float mouse_x, float mouse_y) {
  float popup_x = 0.0f;
  float popup_y = 0.0f;
  float popup_w = 0.0f;
  float popup_h = 0.0f;
  int i = 0;
  int item_count = 0;
  if (!layout || menu_index < 0)
    return -1;
  shell_compute_menu_popup_rect(layout, menu_index, &popup_x, &popup_y, &popup_w,
                                &popup_h);
  item_count = shell_menu_item_count(menu_index);
  for (i = 0; i < item_count; ++i) {
    float row_x = popup_x + 6.0f;
    float row_y = popup_y + 6.0f + i * 28.0f;
    float row_w = popup_w - 12.0f;
    float row_h = 24.0f;
    if (shell_point_in_rect(mouse_x, mouse_y, row_x, row_y, row_w, row_h))
      return i;
  }
  return -1;
}

static bool shell_point_in_open_menu_popup(const ShellLayout *layout,
                                           int menu_index, float mouse_x,
                                           float mouse_y) {
  float popup_x = 0.0f;
  float popup_y = 0.0f;
  float popup_w = 0.0f;
  float popup_h = 0.0f;
  if (!layout || menu_index < 0)
    return false;
  shell_compute_menu_popup_rect(layout, menu_index, &popup_x, &popup_y, &popup_w,
                                &popup_h);
  return shell_point_in_rect(mouse_x, mouse_y, popup_x, popup_y, popup_w,
                             popup_h);
}

static uint64_t shell_now_ms(void) {
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}

static void shell_query_live_mouse(StygianWindow *window, float *out_x,
                                   float *out_y) {
  int x = 0;
  int y = 0;
  if (!out_x || !out_y)
    return;
  *out_x = 0.0f;
  *out_y = 0.0f;
  if (!window)
    return;
#ifdef _WIN32
  {
    HWND hwnd = (HWND)stygian_window_native_handle(window);
    POINT pt;
    if (hwnd && GetCursorPos(&pt) && ScreenToClient(hwnd, &pt)) {
      *out_x = (float)pt.x;
      *out_y = (float)pt.y;
      return;
    }
  }
#endif
  stygian_mouse_pos(window, &x, &y);
  *out_x = (float)x;
  *out_y = (float)y;
}

static bool shell_path_up_one(char *path) {
  size_t len = 0u;
  if (!path || !path[0])
    return false;
  len = strlen(path);
  while (len > 0u && (path[len - 1u] == '/' || path[len - 1u] == '\\')) {
    path[--len] = '\0';
  }
  while (len > 0u && path[len - 1u] != '/' && path[len - 1u] != '\\') {
    path[--len] = '\0';
  }
  while (len > 0u && (path[len - 1u] == '/' || path[len - 1u] == '\\')) {
    path[--len] = '\0';
  }
  return len > 0u;
}

static bool shell_resolve_repo_resource_paths(char *out_shader_dir,
                                              size_t out_shader_dir_size,
                                              char *out_atlas_png,
                                              size_t out_atlas_png_size,
                                              char *out_atlas_json,
                                              size_t out_atlas_json_size) {
  char module_path[1024];
  DWORD len = 0u;
  if (!out_shader_dir || !out_atlas_png || !out_atlas_json)
    return false;
#ifdef _WIN32
  len = GetModuleFileNameA(NULL, module_path, (DWORD)sizeof(module_path));
  if (len == 0u || len >= sizeof(module_path))
    return false;
  module_path[len] = '\0';
  for (char *p = module_path; *p; ++p) {
    if (*p == '\\')
      *p = '/';
  }

  // exe lives under .../editor/build/windows; four climbs gets us repo root.
  if (!shell_path_up_one(module_path) || !shell_path_up_one(module_path) ||
      !shell_path_up_one(module_path) || !shell_path_up_one(module_path)) {
    return false;
  }

  snprintf(out_shader_dir, out_shader_dir_size, "%s/shaders", module_path);
  snprintf(out_atlas_png, out_atlas_png_size, "%s/assets/atlas.png",
           module_path);
  snprintf(out_atlas_json, out_atlas_json_size, "%s/assets/atlas.json",
           module_path);
  return true;
#else
  (void)out_shader_dir;
  (void)out_shader_dir_size;
  (void)out_atlas_png;
  (void)out_atlas_png_size;
  (void)out_atlas_json;
  (void)out_atlas_json_size;
  return false;
#endif
}

static void shell_compute_layout(ShellLayout *layout, int width, int height,
                                 const StygianTitlebarHints *hints,
                                 float left_panel_width,
                                 float right_panel_width) {
  const float panel_gutter = 18.0f;
  const float handle_w = 12.0f;
  float left_w = 0.0f;
  float panel_w = 0.0f;
  if (!layout)
    return;
  memset(layout, 0, sizeof(*layout));
  layout->title_h =
      hints && hints->recommended_titlebar_height > 0.0f
          ? hints->recommended_titlebar_height
          : 36.0f;
  if (layout->title_h < 40.0f)
    layout->title_h = 40.0f;
  layout->button_w =
      hints && hints->recommended_button_width > 0.0f
          ? hints->recommended_button_width
          : 28.0f;
  layout->button_h =
      hints && hints->recommended_button_height > 0.0f
          ? hints->recommended_button_height
          : 24.0f;
  layout->button_gap =
      hints && hints->recommended_button_gap > 0.0f
          ? hints->recommended_button_gap
          : 8.0f;
  // The shell needs to meet the shaped window closely or Windows' remaining
  // frame treatment peeks through as a cheap fringe.
  layout->shell_pad = 1.0f;
  layout->shell_radius = 28.0f;
  layout->button_y = (layout->title_h - layout->button_h) * 0.5f;
  if (layout->button_y < 4.0f)
    layout->button_y = 4.0f;
  layout->shell_x = layout->shell_pad;
  layout->shell_y = layout->shell_pad;
  layout->shell_w = (float)width - layout->shell_pad * 2.0f;
  layout->shell_h = (float)height - layout->shell_pad * 2.0f;
  layout->close_x = layout->shell_x + layout->shell_w - layout->button_gap -
                    layout->button_w;
  layout->max_x = layout->close_x - layout->button_gap - layout->button_w;
  layout->min_x = layout->max_x - layout->button_gap - layout->button_w;
  layout->menu_x = layout->shell_x + 14.0f;
  layout->menu_y = layout->shell_y + (layout->title_h - 28.0f) * 0.5f;
  layout->menu_h = 28.0f;
  layout->menu_w = shell_top_menu_trailing_x(layout) - layout->menu_x;
  layout->workspace_w = 136.0f;
  layout->workspace_h = 26.0f;
  layout->workspace_seg_w = layout->workspace_w * 0.5f;
  layout->workspace_x = shell_top_menu_trailing_x(layout) + 34.0f;
  if (layout->workspace_x + layout->workspace_w > layout->min_x - 18.0f)
    layout->workspace_x = layout->min_x - 18.0f - layout->workspace_w;
  layout->workspace_y = layout->shell_y +
                        (layout->title_h - layout->workspace_h) * 0.5f;
  layout->canvas_x = layout->shell_x + 1.0f;
  layout->canvas_y = layout->shell_y + layout->title_h;
  layout->canvas_w = layout->shell_w - 2.0f;
  layout->canvas_h = layout->shell_h - layout->title_h - 1.0f;

  layout->assets_panel_x = layout->canvas_x + panel_gutter;
  layout->assets_panel_y = layout->canvas_y + panel_gutter;
  left_w = shell_clampf(left_panel_width, handle_w, 420.0f);
  layout->assets_panel_w = left_w;
  layout->assets_panel_h = layout->canvas_h - panel_gutter * 2.0f;
  layout->assets_panel_handle_w = handle_w;
  layout->assets_panel_handle_h = layout->assets_panel_h;
  layout->assets_panel_handle_x =
      layout->assets_panel_x + layout->assets_panel_w - handle_w;
  layout->assets_panel_handle_y = layout->assets_panel_y;

  panel_w = shell_clampf(right_panel_width, handle_w, 420.0f);
  layout->right_panel_w = panel_w;
  layout->right_panel_h = layout->canvas_h - panel_gutter * 2.0f;
  layout->right_panel_x =
      layout->canvas_x + layout->canvas_w - panel_gutter - layout->right_panel_w;
  layout->right_panel_y = layout->canvas_y + panel_gutter;

  layout->right_panel_handle_w = handle_w;
  layout->right_panel_handle_h = layout->right_panel_h;
  layout->right_panel_handle_x = layout->right_panel_x;
  layout->right_panel_handle_y = layout->right_panel_y;
}

static void shell_compute_tool_dock_layout(ShellToolDockLayout *layout,
                                           const ShellLayout *shell) {
  if (!layout || !shell)
    return;
  memset(layout, 0, sizeof(*layout));
  layout->w = 486.0f;
  layout->h = 60.0f;
  layout->x = shell->canvas_x + (shell->canvas_w - layout->w) * 0.5f;
  layout->visible_y = shell->canvas_y + shell->canvas_h - layout->h - 24.0f;
  layout->hidden_y = shell->canvas_y + shell->canvas_h - 18.0f;
  layout->y = layout->visible_y;
  layout->rail_x = layout->x + 12.0f;
  layout->rail_y = layout->y + 11.0f;
  layout->rail_w = layout->w - 24.0f;
  layout->rail_h = layout->h - 22.0f;
  layout->group_gap = 12.0f;
  layout->button_w = 38.0f;
  layout->button_h = 34.0f;
  layout->button_y = layout->y + 13.0f;
}

static ShellCaptionAction shell_hit_caption_button(const ShellLayout *layout,
                                                   float mouse_x,
                                                   float mouse_y) {
  if (!layout)
    return SHELL_CAPTION_NONE;
  if (shell_point_in_rect(mouse_x, mouse_y, layout->min_x, layout->button_y,
                          layout->button_w, layout->button_h)) {
    return SHELL_CAPTION_MINIMIZE;
  }
  if (shell_point_in_rect(mouse_x, mouse_y, layout->max_x, layout->button_y,
                          layout->button_w, layout->button_h)) {
    return SHELL_CAPTION_MAXIMIZE;
  }
  if (shell_point_in_rect(mouse_x, mouse_y, layout->close_x, layout->button_y,
                          layout->button_w, layout->button_h)) {
    return SHELL_CAPTION_CLOSE;
  }
  return SHELL_CAPTION_NONE;
}

static void shell_draw_caption_button(StygianContext *ctx, StygianFont font,
                                      const ShellLayout *layout, float x,
                                      const char *label, bool hover,
                                      bool danger) {
  float bg = danger ? 0.74f : 0.17f;
  float g = danger ? 0.11f : 0.17f;
  float b = danger ? 0.14f : 0.18f;
  if (hover) {
    bg += danger ? 0.08f : 0.05f;
    g += danger ? 0.05f : 0.05f;
    b += danger ? 0.05f : 0.05f;
  }
  stygian_rect_rounded(ctx, x, layout->button_y, layout->button_w,
                       layout->button_h, bg, g, b, 1.0f, 7.0f);
  stygian_text(ctx, font, label, x + 9.0f, layout->button_y + 4.0f, 14.0f,
               0.96f, 0.96f, 0.97f, 1.0f);
}

static void shell_draw_chevron_down(StygianContext *ctx, float cx, float cy,
                                    float size, float thickness, float r,
                                    float g, float b, float a) {
  stygian_line(ctx, cx - size, cy - size * 0.35f, cx, cy + size * 0.45f,
               thickness, r, g, b, a);
  stygian_line(ctx, cx, cy + size * 0.45f, cx + size, cy - size * 0.35f,
               thickness, r, g, b, a);
}

static void shell_draw_chevron_left(StygianContext *ctx, float cx, float cy,
                                    float size, float thickness, float r,
                                    float g, float b, float a) {
  stygian_line(ctx, cx + size * 0.35f, cy - size, cx - size * 0.45f, cy,
               thickness, r, g, b, a);
  stygian_line(ctx, cx - size * 0.45f, cy, cx + size * 0.35f, cy + size,
               thickness, r, g, b, a);
}

static void shell_draw_chevron_right(StygianContext *ctx, float cx, float cy,
                                     float size, float thickness, float r,
                                     float g, float b, float a) {
  stygian_line(ctx, cx - size * 0.35f, cy - size, cx + size * 0.45f, cy,
               thickness, r, g, b, a);
  stygian_line(ctx, cx + size * 0.45f, cy, cx - size * 0.35f, cy + size,
               thickness, r, g, b, a);
}

static void shell_draw_tool_icon(StygianContext *ctx, int icon_kind, float x,
                                 float y, float w, float h, float r, float g,
                                 float b, float a) {
  stygian_editor_draw_icon(ctx, (StygianEditorIconKind)icon_kind, x, y, w, h,
                           r, g, b, a);
}

static bool shell_tool_has_family(int tool_index) {
  return tool_index == 0 || tool_index == 1 || tool_index == 3 ||
         tool_index == 5 ||
         tool_index == 6;
}

static int shell_tool_family_count(int tool_index) {
  switch (tool_index) {
  case 0:
  case 1:
  case 3:
  case 6:
    return 3;
  case 5:
    return 7;
  default:
    return 0;
  }
}

static const char *shell_tool_family_label(int tool_index, int item_index) {
  switch (tool_index) {
  case 0:
    switch (item_index) {
    case 0:
      return "Select";
    case 1:
      return "Move";
    case 2:
      return "Lasso";
    default:
      break;
    }
    break;
  case 1:
    switch (item_index) {
    case 0:
      return "Desktop";
    case 1:
      return "Tablet";
    case 2:
      return "Phone";
    default:
      break;
    }
    break;
  case 3:
    switch (item_index) {
    case 0:
      return "Pen";
    case 1:
      return "Pencil";
    case 2:
      return "Bend";
    default:
      break;
    }
    break;
  case 5:
    switch (item_index) {
    case 0:
      return "Rectangle";
    case 1:
      return "Ellipse";
    case 2:
      return "Rounded";
    case 3:
      return "Arrow";
    case 4:
      return "Polygon";
    case 5:
      return "Star";
    case 6:
      return "Image";
    default:
      break;
    }
    break;
  case 6:
    switch (item_index) {
    case 0:
      return "Component";
    case 1:
      return "Set";
    case 2:
      return "Instance";
    default:
      break;
    }
    break;
  default:
    break;
  }
  return "";
}

static int shell_tool_family_icon_kind(int tool_index, int item_index) {
  switch (tool_index) {
  case 0:
    return item_index == 1 ? 10 : (item_index == 2 ? 17 : 0);
  case 1:
    return 1;
  case 3:
    return item_index == 1 ? 13 : (item_index == 2 ? 14 : 3);
  case 5:
    switch (item_index) {
    case 1:
      return 5;
    case 3:
      return 2;
    case 4:
      return 12;
    case 5:
      return 6;
    case 6:
      return 1;
    default:
      return 11;
    }
  case 6:
    return item_index == 1 ? 15 : (item_index == 2 ? 16 : 6);
  default:
    return tool_index;
  }
}

static int shell_tool_button_icon_kind(const ShellToolDockState *state,
                                       int tool_index) {
  int choice = 0;
  if (!state || !shell_tool_has_family(tool_index))
    return tool_index;
  choice = state->family_choice[tool_index];
  if (choice < 0 || choice >= shell_tool_family_count(tool_index))
    choice = 0;
  return shell_tool_family_icon_kind(tool_index, choice);
}

static void shell_draw_tool_family_item(StygianContext *ctx, StygianFont font,
                                        float x, float y, float w, float h,
                                        int icon_kind, const char *label,
                                        bool active, bool hovered,
                                        bool pressed, float alpha) {
  float bg = active ? 0.10f : 0.08f;
  float fg = active ? 0.96f : 0.86f;
  if (hovered) {
    bg += 0.06f;
    fg = 0.97f;
  }
  if (pressed)
    bg -= 0.03f;
  stygian_rect_rounded(ctx, x, y, w, h, bg, bg, bg + 0.01f, 0.94f * alpha,
                       10.0f);
  if (hovered || active) {
    stygian_rect_rounded(ctx, x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f, 0.18f,
                         0.23f, 0.29f, (hovered ? 0.30f : 0.18f) * alpha,
                         9.0f);
  }
  shell_draw_tool_icon(ctx, icon_kind, x + 8.0f, y + 4.0f, 22.0f, 22.0f, fg,
                       fg, fg, alpha);
  stygian_text(ctx, font, label, x + 36.0f, y + 6.0f, 13.0f, fg, fg, fg,
               0.98f * alpha);
}

static void shell_draw_tool_button(StygianContext *ctx, float x, float y,
                                   float w, float h, int icon_kind, bool active,
                                   bool hovered, bool pressed, bool muted,
                                   float alpha) {
  float bg_r = active ? 0.08f : 0.15f;
  float bg_g = active ? 0.48f : 0.15f;
  float bg_b = active ? 0.88f : 0.16f;
  float fg = active ? 0.98f : (muted ? 0.72f : 0.90f);
  float surface_alpha = active ? 1.0f : 0.90f;
  if (hovered && !active) {
    bg_r += 0.12f;
    bg_g += 0.12f;
    bg_b += 0.14f;
  }
  if (pressed) {
    bg_r -= 0.03f;
    bg_g -= 0.03f;
    bg_b -= 0.03f;
  }
  stygian_rect_rounded(ctx, x, y, w, h, bg_r, bg_g, bg_b,
                       surface_alpha * alpha, 10.0f);
  stygian_rect_rounded(ctx, x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f,
                       bg_r + 0.03f, bg_g + 0.03f, bg_b + 0.03f,
                       (0.28f + (hovered ? 0.22f : 0.0f)) * alpha, 9.0f);
  if (hovered || active) {
    stygian_rect(ctx, x + 1.0f, y + 1.0f, w - 2.0f, 1.0f, 0.55f, 0.64f, 0.76f,
                 (hovered ? 0.52f : 0.36f) * alpha);
    stygian_rect_rounded(ctx, x - 1.0f, y - 1.0f, w + 2.0f, h + 2.0f, 0.27f,
                         0.63f, 0.98f, (hovered ? 0.10f : 0.06f) * alpha,
                         11.0f);
  }
  shell_draw_tool_icon(ctx, icon_kind, x, y, w, h, fg, fg, fg, alpha);
}

static void shell_draw_workspace_switch(StygianContext *ctx, StygianFont font,
                                        const ShellLayout *layout,
                                        ShellWorkspaceMode mode,
                                        bool hover_canvas, bool hover_icons) {
  float x = layout->workspace_x;
  float y = layout->workspace_y;
  float seg_w = layout->workspace_seg_w;
  float w = layout->workspace_w;
  float h = layout->workspace_h;
  float canvas_bg = mode == SHELL_WORKSPACE_CANVAS ? 0.10f : 0.07f;
  float icons_bg = mode == SHELL_WORKSPACE_ICON_LAB ? 0.10f : 0.07f;

  if (hover_canvas && mode != SHELL_WORKSPACE_CANVAS)
    canvas_bg += 0.04f;
  if (hover_icons && mode != SHELL_WORKSPACE_ICON_LAB)
    icons_bg += 0.04f;

  stygian_rect_rounded(ctx, x, y, w, h, 0.07f, 0.07f, 0.08f, 0.98f, 10.0f);
  stygian_rect_rounded(ctx, x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f, 0.12f,
                       0.12f, 0.13f, 0.68f, 9.0f);

  stygian_rect_rounded(ctx, x + 2.0f, y + 2.0f, seg_w - 3.0f, h - 4.0f,
                       canvas_bg, mode == SHELL_WORKSPACE_CANVAS ? 0.46f : canvas_bg,
                       mode == SHELL_WORKSPACE_CANVAS ? 0.86f : canvas_bg + 0.01f,
                       mode == SHELL_WORKSPACE_CANVAS ? 1.0f : 0.92f, 8.0f);
  stygian_rect_rounded(ctx, x + seg_w + 1.0f, y + 2.0f, seg_w - 3.0f, h - 4.0f,
                       icons_bg, mode == SHELL_WORKSPACE_ICON_LAB ? 0.46f : icons_bg,
                       mode == SHELL_WORKSPACE_ICON_LAB ? 0.86f : icons_bg + 0.01f,
                       mode == SHELL_WORKSPACE_ICON_LAB ? 1.0f : 0.92f, 8.0f);

  stygian_text(ctx, font, "Canvas", x + 12.0f, y + 6.0f, 13.0f,
               mode == SHELL_WORKSPACE_CANVAS ? 0.98f : 0.84f,
               mode == SHELL_WORKSPACE_CANVAS ? 0.98f : 0.84f,
               mode == SHELL_WORKSPACE_CANVAS ? 0.99f : 0.86f, 1.0f);
  stygian_text(ctx, font, "Icons", x + seg_w + 18.0f, y + 6.0f, 13.0f,
               mode == SHELL_WORKSPACE_ICON_LAB ? 0.98f : 0.84f,
               mode == SHELL_WORKSPACE_ICON_LAB ? 0.98f : 0.84f,
               mode == SHELL_WORKSPACE_ICON_LAB ? 0.99f : 0.86f, 1.0f);
}

static bool shell_hit_right_panel_handle(const ShellLayout *layout, float mouse_x,
                                         float mouse_y) {
  if (!layout)
    return false;
  return shell_point_in_rect(mouse_x, mouse_y, layout->right_panel_handle_x,
                             layout->right_panel_handle_y,
                             layout->right_panel_handle_w,
                             layout->right_panel_handle_h);
}

static ShellMotionEasing shell_motion_next_easing(ShellMotionEasing easing) {
  switch (easing) {
  case SHELL_MOTION_EASE_LINEAR:
    return SHELL_MOTION_EASE_OUT_CUBIC;
  case SHELL_MOTION_EASE_OUT_CUBIC:
    return SHELL_MOTION_EASE_IN_OUT_CUBIC;
  case SHELL_MOTION_EASE_IN_OUT_CUBIC:
  default:
    return SHELL_MOTION_EASE_LINEAR;
  }
}

static ShellMotionPropertyKind
shell_motion_next_property(ShellMotionPropertyKind property) {
  switch (property) {
  case SHELL_MOTION_PROP_X:
    return SHELL_MOTION_PROP_Y;
  case SHELL_MOTION_PROP_Y:
    return SHELL_MOTION_PROP_SCALE;
  case SHELL_MOTION_PROP_SCALE:
    return SHELL_MOTION_PROP_ROTATION;
  case SHELL_MOTION_PROP_ROTATION:
    return SHELL_MOTION_PROP_OPACITY;
  case SHELL_MOTION_PROP_OPACITY:
    return SHELL_MOTION_PROP_COLOR;
  case SHELL_MOTION_PROP_COLOR:
  default:
    return SHELL_MOTION_PROP_X;
  }
}

static bool shell_motion_clip_targets_node(const ShellMotionState *state,
                                           const ShellMotionClip *clip,
                                           int node_id) {
  int i = 0;
  if (!state || !clip || node_id <= 0)
    return false;
  for (i = 0; i < clip->track_count; ++i) {
    const ShellMotionTrack *track =
        shell_motion_find_track_const(state, clip->track_ids[i]);
    if (track && track->target_node_id == node_id)
      return true;
  }
  return false;
}

static ShellMotionClip *shell_motion_find_clip_for_node(ShellMotionState *state,
                                                        int node_id) {
  int i = 0;
  if (!state || node_id <= 0)
    return NULL;
  for (i = 0; i < state->clip_count; ++i) {
    if (state->clips[i].alive &&
        shell_motion_clip_targets_node(state, &state->clips[i], node_id)) {
      return &state->clips[i];
    }
  }
  return NULL;
}

static void shell_draw_motion_curve_preview(StygianContext *ctx, float x, float y,
                                            float w, float h,
                                            ShellMotionEasing easing) {
  float prev_x = x + 8.0f;
  float prev_y = y + h - 8.0f;
  int i = 0;
  if (!ctx)
    return;
  stygian_rect_rounded(ctx, x, y, w, h, 0.11f, 0.11f, 0.12f, 0.96f, 10.0f);
  stygian_rect_rounded(ctx, x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f, 0.15f, 0.15f,
                       0.16f, 0.56f, 9.0f);
  for (i = 1; i <= 24; ++i) {
    float t = (float)i / 24.0f;
    float eased = shell_motion_ease_value(easing, t);
    float px = x + 8.0f + (w - 16.0f) * t;
    float py = y + h - 8.0f - (h - 16.0f) * eased;
    stygian_line(ctx, prev_x, prev_y, px, py, 1.5f, 0.24f, 0.64f, 0.98f, 1.0f);
    prev_x = px;
    prev_y = py;
  }
}

static uint32_t shell_motion_timeline_ms_from_mouse(float mouse_x, float lane_x,
                                                    float lane_w,
                                                    uint32_t duration_ms) {
  float t = 0.0f;
  if (lane_w <= 1.0f || duration_ms == 0u)
    return 0u;
  t = (mouse_x - lane_x) / lane_w;
  t = shell_clamp01(t);
  return (uint32_t)(t * (float)duration_ms + 0.5f);
}

static void shell_draw_motion_panel(
    StygianContext *ctx, StygianFont font, const ShellLayout *layout,
    float mouse_x, float mouse_y, const ShellFrameState *frame,
    ShellCanvasSceneState *scene, ShellMotionState *motion_state, uint64_t now_ms,
    bool *out_dirty) {
  ShellCanvasFrameNode *selected = shell_canvas_scene_selected_frame(scene);
  ShellMotionClip *clip = NULL;
  ShellMotionTrack *track = NULL;
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float row_y = 0.0f;
  bool dirty = false;
  bool hit_any = false;
  if (!ctx || !font || !layout || !motion_state) {
    if (out_dirty)
      *out_dirty = false;
    return;
  }
  if (selected && motion_state->selected_clip_id <= 0) {
    ShellMotionClip *first = shell_motion_find_clip_for_node(motion_state, selected->id);
    if (first)
      motion_state->selected_clip_id = first->id;
  }
  clip = shell_motion_find_clip(motion_state, motion_state->selected_clip_id);
  if (clip && selected && !shell_motion_clip_targets_node(motion_state, clip, selected->id)) {
    clip = shell_motion_find_clip_for_node(motion_state, selected->id);
    motion_state->selected_clip_id = clip ? clip->id : -1;
  }
  track = shell_motion_find_track(motion_state, motion_state->selected_track_id);
  shell_motion_sync_fields(motion_state);

  x = layout->right_panel_x + 18.0f;
  y = layout->right_panel_y + 74.0f;
  w = layout->right_panel_w - 36.0f;
  stygian_text(ctx, font, selected ? selected->name : "No selection", x, y, 18.0f,
               0.95f, 0.95f, 0.97f, 1.0f);
  stygian_text(ctx, font,
               "Clips author motion. Bind them to hover, press, or toggle without code.",
               x, y + 22.0f, 11.0f, 0.56f, 0.56f, 0.60f, 1.0f);
  shell_draw_section_rule(ctx, x, y + 44.0f, w);

  row_y = y + 58.0f;
  if (frame && frame->left_pressed && shell_point_in_rect(mouse_x, mouse_y, x + w - 78.0f,
                                                          row_y, 78.0f, 30.0f)) {
    ShellMotionClip *new_clip = shell_motion_create_named_clip(
        motion_state, selected ? selected->name : "Motion");
    if (new_clip) {
      clip = new_clip;
      dirty = true;
    }
  }
  if (frame && frame->left_pressed && clip &&
      shell_point_in_rect(mouse_x, mouse_y, x, row_y + 38.0f, 72.0f, 30.0f)) {
    if (motion_state->playing) {
      shell_motion_reset_preview(ctx, motion_state, scene);
    } else {
      shell_motion_capture_preview_scene(motion_state, scene);
      motion_state->playing = true;
      motion_state->scrub_active = false;
      motion_state->play_started_ms = now_ms - motion_state->playhead_ms;
    }
    dirty = true;
  }
  if (frame && frame->left_pressed &&
      shell_point_in_rect(mouse_x, mouse_y, x + 78.0f, row_y + 38.0f, 72.0f,
                          30.0f)) {
    shell_motion_reset_preview(ctx, motion_state, scene);
    motion_state->playhead_ms = 0u;
    dirty = true;
  }
  if (frame && frame->left_pressed && clip &&
      shell_point_in_rect(mouse_x, mouse_y, x + 156.0f, row_y + 38.0f, 72.0f,
                          30.0f)) {
    clip->loop = !clip->loop;
    dirty = true;
  }

  if (frame && frame->left_pressed) {
    if (shell_inspector_hit_input(mouse_x, mouse_y, x, row_y, w - 86.0f, 34.0f)) {
      motion_state->active_field = SHELL_MOTION_FIELD_CLIP_NAME;
      hit_any = true;
    }
  }
  motion_state->editing = hit_any;
  if (clip) {
    if (shell_draw_labeled_text_input(ctx, font, x, row_y, w - 86.0f, 34.0f,
                                      "Clip", motion_state->clip_name_buf,
                                      (int)sizeof(motion_state->clip_name_buf),
                                      motion_state->active_field ==
                                          SHELL_MOTION_FIELD_CLIP_NAME)) {
      snprintf(clip->name, sizeof(clip->name), "%s", motion_state->clip_name_buf);
      dirty = true;
    }
  } else {
    shell_draw_field_box(ctx, font, x, row_y, w - 86.0f, 34.0f, "Clip",
                         "Create one to animate the current node.", false);
  }
  stygian_rect_rounded(ctx, x + w - 78.0f, row_y, 78.0f, 30.0f, 0.12f, 0.16f,
                       0.23f, 0.96f, 9.0f);
  stygian_text(ctx, font, "New Clip", x + w - 62.0f, row_y + 7.0f, 12.0f, 0.96f,
               0.96f, 0.98f, 1.0f);
  stygian_rect_rounded(ctx, x, row_y + 38.0f, 72.0f, 30.0f, 0.11f, 0.16f, 0.23f,
                       0.96f, 9.0f);
  stygian_rect_rounded(ctx, x + 78.0f, row_y + 38.0f, 72.0f, 30.0f, 0.11f, 0.12f,
                       0.13f, 0.96f, 9.0f);
  stygian_rect_rounded(ctx, x + 156.0f, row_y + 38.0f, 72.0f, 30.0f,
                       clip && clip->loop ? 0.16f : 0.11f,
                       clip && clip->loop ? 0.33f : 0.12f,
                       clip && clip->loop ? 0.60f : 0.13f, 0.96f, 9.0f);
  stygian_text(ctx, font, motion_state->playing ? "Pause" : "Play", x + 22.0f,
               row_y + 45.0f, 12.0f, 0.96f, 0.96f, 0.98f, 1.0f);
  stygian_text(ctx, font, "Reset", x + 98.0f, row_y + 45.0f, 12.0f, 0.96f, 0.96f,
               0.98f, 1.0f);
  stygian_text(ctx, font, clip && clip->loop ? "Loop On" : "Loop Off",
               x + 172.0f, row_y + 45.0f, 12.0f, 0.96f, 0.96f, 0.98f, 1.0f);

  row_y += 84.0f;
  shell_draw_section_rule(ctx, x, row_y, w);
  stygian_text(ctx, font, "Tracks", x, row_y + 14.0f, 12.0f, 0.58f, 0.58f, 0.61f,
               1.0f);
  {
    static const ShellMotionPropertyKind props[] = {
        SHELL_MOTION_PROP_X,       SHELL_MOTION_PROP_Y,      SHELL_MOTION_PROP_SCALE,
        SHELL_MOTION_PROP_ROTATION, SHELL_MOTION_PROP_OPACITY, SHELL_MOTION_PROP_COLOR};
    float bx = x;
    for (int i = 0; i < 6; ++i) {
      float bw = (i == 5) ? 68.0f : 54.0f;
      bool hovered = shell_point_in_rect(mouse_x, mouse_y, bx, row_y + 34.0f, bw, 28.0f);
      if (frame && frame->left_pressed && hovered && clip && selected) {
        if (shell_motion_add_track_to_clip(motion_state, selected, clip, props[i])) {
          track = shell_motion_find_track(motion_state, motion_state->selected_track_id);
          dirty = true;
        }
      }
      stygian_rect_rounded(ctx, bx, row_y + 34.0f, bw, 28.0f, hovered ? 0.13f : 0.11f,
                           hovered ? 0.17f : 0.11f, hovered ? 0.23f : 0.12f, 0.96f,
                           8.0f);
      stygian_text(ctx, font, shell_motion_property_label(props[i]), bx + 10.0f,
                   row_y + 41.0f, 11.0f, 0.96f, 0.96f, 0.98f, 1.0f);
      bx += bw + 6.0f;
    }
  }

  row_y += 74.0f;
  if (clip && clip->track_count > 0) {
    for (int i = 0; i < clip->track_count && i < 5; ++i) {
      const ShellMotionTrack *clip_track =
          shell_motion_find_track_const(motion_state, clip->track_ids[i]);
      bool hovered = shell_point_in_rect(mouse_x, mouse_y, x, row_y, w, 28.0f);
      bool active = clip_track && motion_state->selected_track_id == clip_track->id;
      if (!clip_track)
        continue;
      if (frame && frame->left_pressed && hovered) {
        motion_state->selected_track_id = clip_track->id;
        motion_state->last_track_id = -1;
        shell_motion_sync_fields(motion_state);
        track = shell_motion_find_track(motion_state, motion_state->selected_track_id);
        dirty = true;
      }
      stygian_rect_rounded(ctx, x, row_y, w, 28.0f, active ? 0.12f : 0.10f,
                           active ? 0.22f : 0.11f, active ? 0.36f : 0.12f, 0.96f,
                           8.0f);
      stygian_text(ctx, font, shell_motion_property_label(clip_track->property),
                   x + 10.0f, row_y + 7.0f, 12.0f, 0.95f, 0.95f, 0.97f, 1.0f);
      stygian_text(ctx, font, shell_motion_easing_label(clip_track->easing),
                   x + w - 86.0f, row_y + 7.0f, 11.0f, 0.58f, 0.58f, 0.61f, 1.0f);
      row_y += 32.0f;
    }
  } else {
    stygian_text(ctx, font, "No tracks in this clip yet.", x, row_y + 4.0f, 12.0f,
                 0.66f, 0.66f, 0.70f, 1.0f);
    row_y += 28.0f;
  }

  track = shell_motion_find_track(motion_state, motion_state->selected_track_id);
  if (clip && track) {
    float half_w = w * 0.5f - 4.0f;
    float right_x = x + half_w + 8.0f;
    bool input_hit = false;
    row_y += 8.0f;
    shell_draw_section_rule(ctx, x, row_y, w);
    stygian_text(ctx, font, "Selected Track", x, row_y + 14.0f, 12.0f, 0.58f, 0.58f,
                 0.61f, 1.0f);
    row_y += 24.0f;
    if (frame && frame->left_pressed) {
      if (shell_inspector_hit_input(mouse_x, mouse_y, x, row_y, half_w, 34.0f)) {
        motion_state->active_field = SHELL_MOTION_FIELD_START;
        input_hit = true;
      }
      if (shell_inspector_hit_input(mouse_x, mouse_y, right_x, row_y, half_w, 34.0f)) {
        motion_state->active_field = SHELL_MOTION_FIELD_DURATION;
        input_hit = true;
      }
    }
    if (frame && frame->left_pressed)
      motion_state->editing = input_hit;
    if (!motion_state->editing)
      shell_motion_sync_fields(motion_state);
    if (shell_draw_numeric_input(ctx, font, x, row_y, half_w, 34.0f, "Start",
                                 motion_state->start_buf,
                                 (int)sizeof(motion_state->start_buf),
                                 motion_state->active_field == SHELL_MOTION_FIELD_START)) {
      float parsed = 0.0f;
      if (shell_parse_float(motion_state->start_buf, &parsed)) {
        track->start_ms = (uint32_t)shell_clampf(parsed, 0.0f, 10000.0f);
        shell_motion_clip_recompute_duration(motion_state, clip);
        dirty = true;
      }
    }
    if (shell_draw_numeric_input(ctx, font, right_x, row_y, half_w, 34.0f, "Duration",
                                 motion_state->duration_buf,
                                 (int)sizeof(motion_state->duration_buf),
                                 motion_state->active_field ==
                                     SHELL_MOTION_FIELD_DURATION)) {
      float parsed = 0.0f;
      if (shell_parse_float(motion_state->duration_buf, &parsed)) {
        track->duration_ms = (uint32_t)shell_clampf(parsed, 1.0f, 10000.0f);
        shell_motion_clip_recompute_duration(motion_state, clip);
        dirty = true;
      }
    }
    row_y += 42.0f;
    shell_draw_field_box(ctx, font, x, row_y, half_w, 34.0f, "Property",
                         shell_motion_property_label(track->property), false);
    shell_draw_field_box(ctx, font, right_x, row_y, half_w, 34.0f, "Easing",
                         shell_motion_easing_label(track->easing), false);
    if (frame && frame->left_pressed &&
        shell_point_in_rect(mouse_x, mouse_y, x, row_y, half_w, 34.0f)) {
      track->property = shell_motion_next_property(track->property);
      shell_motion_capture_defaults_from_node(selected, track);
      shell_motion_sync_fields(motion_state);
      dirty = true;
    }
    if (frame && frame->left_pressed &&
        shell_point_in_rect(mouse_x, mouse_y, right_x, row_y, half_w, 34.0f)) {
      track->easing = shell_motion_next_easing(track->easing);
      dirty = true;
    }
    row_y += 42.0f;
    if (track->property == SHELL_MOTION_PROP_COLOR) {
      shell_draw_motion_curve_preview(ctx, x, row_y, w, 58.0f, track->easing);
      row_y += 64.0f;
      if (shell_draw_numeric_input(ctx, font, x, row_y, half_w / 1.5f, 34.0f, "From R",
                                   motion_state->from_r_buf,
                                   (int)sizeof(motion_state->from_r_buf), false)) {
        float parsed = 0.0f;
        if (shell_parse_float(motion_state->from_r_buf, &parsed)) {
          track->from_rgba[0] = shell_u8_to_color(parsed);
          dirty = true;
        }
      }
      if (shell_draw_numeric_input(ctx, font, x + half_w / 1.5f + 6.0f, row_y,
                                   half_w / 1.5f, 34.0f, "From G",
                                   motion_state->from_g_buf,
                                   (int)sizeof(motion_state->from_g_buf), false)) {
        float parsed = 0.0f;
        if (shell_parse_float(motion_state->from_g_buf, &parsed)) {
          track->from_rgba[1] = shell_u8_to_color(parsed);
          dirty = true;
        }
      }
      if (shell_draw_numeric_input(ctx, font, right_x, row_y, half_w / 1.5f, 34.0f,
                                   "To R", motion_state->to_r_buf,
                                   (int)sizeof(motion_state->to_r_buf), false)) {
        float parsed = 0.0f;
        if (shell_parse_float(motion_state->to_r_buf, &parsed)) {
          track->to_rgba[0] = shell_u8_to_color(parsed);
          dirty = true;
        }
      }
      if (shell_draw_numeric_input(ctx, font, right_x + half_w / 1.5f + 6.0f,
                                   row_y, half_w / 1.5f, 34.0f, "To G",
                                   motion_state->to_g_buf,
                                   (int)sizeof(motion_state->to_g_buf), false)) {
        float parsed = 0.0f;
        if (shell_parse_float(motion_state->to_g_buf, &parsed)) {
          track->to_rgba[1] = shell_u8_to_color(parsed);
          dirty = true;
        }
      }
      row_y += 42.0f;
      if (shell_draw_numeric_input(ctx, font, x, row_y, half_w, 34.0f, "From B",
                                   motion_state->from_b_buf,
                                   (int)sizeof(motion_state->from_b_buf), false)) {
        float parsed = 0.0f;
        if (shell_parse_float(motion_state->from_b_buf, &parsed)) {
          track->from_rgba[2] = shell_u8_to_color(parsed);
          dirty = true;
        }
      }
      if (shell_draw_numeric_input(ctx, font, right_x, row_y, half_w, 34.0f, "To B",
                                   motion_state->to_b_buf,
                                   (int)sizeof(motion_state->to_b_buf), false)) {
        float parsed = 0.0f;
        if (shell_parse_float(motion_state->to_b_buf, &parsed)) {
          track->to_rgba[2] = shell_u8_to_color(parsed);
          dirty = true;
        }
      }
    } else {
      shell_draw_motion_curve_preview(ctx, x, row_y, w, 58.0f, track->easing);
      row_y += 64.0f;
      if (shell_draw_numeric_input(ctx, font, x, row_y, half_w, 34.0f, "From",
                                   motion_state->from_buf,
                                   (int)sizeof(motion_state->from_buf), false)) {
        float parsed = 0.0f;
        if (shell_parse_float(motion_state->from_buf, &parsed)) {
          track->from_value = parsed;
          dirty = true;
        }
      }
      if (shell_draw_numeric_input(ctx, font, right_x, row_y, half_w, 34.0f, "To",
                                   motion_state->to_buf,
                                   (int)sizeof(motion_state->to_buf), false)) {
        float parsed = 0.0f;
        if (shell_parse_float(motion_state->to_buf, &parsed)) {
          track->to_value = parsed;
          dirty = true;
        }
      }
    }
    row_y += 54.0f;
    shell_draw_section_rule(ctx, x, row_y, w);
    stygian_text(ctx, font, "Bindings", x, row_y + 14.0f, 12.0f, 0.58f, 0.58f, 0.61f,
                 1.0f);
    row_y += 24.0f;
    {
      struct {
        const char *label;
        float bx;
        float bw;
      } bind_buttons[] = {
          {"Bind Hover", x, 86.0f},
          {"Bind Press", x + 92.0f, 86.0f},
          {"Bind Toggle", x + 184.0f, 92.0f},
      };
      for (int i = 0; i < 3; ++i) {
        bool hovered = shell_point_in_rect(mouse_x, mouse_y, bind_buttons[i].bx, row_y,
                                           bind_buttons[i].bw, 30.0f);
        stygian_rect_rounded(ctx, bind_buttons[i].bx, row_y, bind_buttons[i].bw, 30.0f,
                             hovered ? 0.13f : 0.11f, hovered ? 0.17f : 0.11f,
                             hovered ? 0.23f : 0.12f, 0.96f, 8.0f);
        stygian_text(ctx, font, bind_buttons[i].label, bind_buttons[i].bx + 10.0f,
                     row_y + 7.0f, 11.0f, 0.96f, 0.96f, 0.98f, 1.0f);
        if (frame && frame->left_pressed && hovered && selected) {
          if (i == 0) {
            dirty = shell_motion_bind_selected_clip(
                        motion_state, selected->id, SHELL_MOTION_EVENT_HOVER_ENTER,
                        SHELL_MOTION_EVENT_HOVER_LEAVE, false) ||
                    dirty;
          } else if (i == 1) {
            dirty = shell_motion_bind_selected_clip(
                        motion_state, selected->id, SHELL_MOTION_EVENT_PRESS,
                        SHELL_MOTION_EVENT_RELEASE, false) ||
                    dirty;
          } else {
            dirty = shell_motion_bind_selected_clip(
                        motion_state, selected->id, SHELL_MOTION_EVENT_PRESS,
                        SHELL_MOTION_EVENT_PRESS, true) ||
                    dirty;
          }
        }
      }
    }
  }

  if (out_dirty)
    *out_dirty = dirty;
}

static void shell_draw_motion_timeline_panel(
    StygianContext *ctx, StygianFont font, const ShellLayout *layout,
    const ShellFrameState *frame, float left_panel_width, float right_panel_width,
    ShellCanvasSceneState *scene, ShellMotionState *state, uint64_t now_ms,
    bool *out_dirty) {
  float stage_x = 0.0f;
  float stage_y = 0.0f;
  float stage_w = 0.0f;
  float stage_h = 0.0f;
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
  bool dirty = false;
  ShellMotionClip *clip = NULL;
  if (!ctx || !font || !layout || !frame || !state ||
      state->active_tab != SHELL_PANEL_MOTION) {
    if (out_dirty)
      *out_dirty = false;
    return;
  }
  clip = shell_motion_find_clip(state, state->selected_clip_id);
  shell_canvas_stage_rect(layout, left_panel_width, right_panel_width, &stage_x,
                          &stage_y, &stage_w, &stage_h);
  x = stage_x;
  y = stage_y + stage_h + 10.0f;
  w = stage_w;
  h = layout->canvas_y + layout->canvas_h - 18.0f - y;
  if (h < 70.0f)
    return;

  stygian_rect_rounded(ctx, x, y, w, h, 0.08f, 0.08f, 0.09f, 0.96f, 14.0f);
  stygian_rect_rounded(ctx, x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f, 0.14f, 0.14f,
                       0.15f, 0.58f, 13.0f);
  stygian_text(ctx, font, clip ? clip->name : "Timeline", x + 16.0f, y + 12.0f, 14.0f,
               0.94f, 0.94f, 0.97f, 1.0f);
  stygian_text(ctx, font,
               "Drag keys directly here. Scrub the playhead to preview authored motion.",
               x + 16.0f, y + 30.0f, 11.0f, 0.58f, 0.58f, 0.61f, 1.0f);
  if (!clip) {
    if (out_dirty)
      *out_dirty = false;
    return;
  }
  {
    float lane_x = x + 144.0f;
    float lane_w = w - 164.0f;
    float row_y = y + 56.0f;
    float lane_h = h - 68.0f;
    uint32_t duration_ms = clip->duration_ms > 0u ? clip->duration_ms : 240u;
    if (frame->left_pressed &&
        shell_point_in_rect(frame->mouse_x, frame->mouse_y, lane_x, row_y - 12.0f,
                            lane_w, lane_h + 12.0f)) {
      state->drag_kind = SHELL_TIMELINE_DRAG_PLAYHEAD;
      state->scrub_active = true;
      state->playing = false;
      state->drag_start_mouse_x = frame->mouse_x;
      shell_motion_capture_preview_scene(state, scene);
      state->playhead_ms =
          shell_motion_timeline_ms_from_mouse(frame->mouse_x, lane_x, lane_w, duration_ms);
      dirty = true;
    }
    if (state->drag_kind == SHELL_TIMELINE_DRAG_PLAYHEAD) {
      if (frame->left_down) {
        state->playhead_ms =
            shell_motion_timeline_ms_from_mouse(frame->mouse_x, lane_x, lane_w, duration_ms);
        state->scrub_active = true;
        dirty = true;
      } else {
        state->drag_kind = SHELL_TIMELINE_DRAG_NONE;
        state->scrub_active = false;
        shell_motion_reset_preview(ctx, state, scene);
        dirty = true;
      }
    }

    for (int i = 0; i < clip->track_count && i < 6; ++i) {
      ShellMotionTrack *track = shell_motion_find_track(state, clip->track_ids[i]);
      float ty = row_y + i * 28.0f;
      float start_x = 0.0f;
      float end_x = 0.0f;
      bool active = false;
      if (!track)
        continue;
      start_x = lane_x + lane_w * ((float)track->start_ms / (float)duration_ms);
      end_x = lane_x + lane_w *
                           ((float)(track->start_ms + track->duration_ms) /
                            (float)duration_ms);
      active = state->selected_track_id == track->id;
      stygian_text(ctx, font, shell_motion_property_label(track->property), x + 16.0f,
                   ty + 3.0f, 11.0f, 0.90f, 0.90f, 0.93f, 1.0f);
      stygian_rect_rounded(ctx, lane_x, ty + 3.0f, lane_w, 12.0f, 0.10f, 0.10f, 0.11f,
                           0.92f, 6.0f);
      stygian_rect_rounded(ctx, start_x, ty + 2.0f, end_x - start_x, 14.0f,
                           active ? 0.17f : 0.12f, active ? 0.40f : 0.22f,
                           active ? 0.88f : 0.52f, 0.96f, 7.0f);
      stygian_rect_rounded(ctx, start_x - 4.0f, ty, 8.0f, 18.0f, 0.90f, 0.90f, 0.94f,
                           1.0f, 4.0f);
      stygian_rect_rounded(ctx, end_x - 4.0f, ty, 8.0f, 18.0f, 0.90f, 0.90f, 0.94f,
                           1.0f, 4.0f);
      if (frame->left_pressed &&
          shell_point_in_rect(frame->mouse_x, frame->mouse_y, lane_x, ty, lane_w, 18.0f)) {
        state->selected_track_id = track->id;
        state->last_track_id = -1;
        shell_motion_sync_fields(state);
      }
      if (frame->left_pressed &&
          shell_point_in_rect(frame->mouse_x, frame->mouse_y, start_x - 6.0f, ty - 4.0f,
                              12.0f, 24.0f)) {
        state->drag_kind = SHELL_TIMELINE_DRAG_TRACK_START;
        state->drag_track_id = track->id;
        state->drag_start_mouse_x = frame->mouse_x;
        state->drag_start_ms = track->start_ms;
        dirty = true;
      } else if (frame->left_pressed &&
                 shell_point_in_rect(frame->mouse_x, frame->mouse_y, end_x - 6.0f,
                                     ty - 4.0f, 12.0f, 24.0f)) {
        state->drag_kind = SHELL_TIMELINE_DRAG_TRACK_END;
        state->drag_track_id = track->id;
        state->drag_start_mouse_x = frame->mouse_x;
        state->drag_start_duration_ms = track->duration_ms;
        dirty = true;
      }
    }

    if (state->drag_kind == SHELL_TIMELINE_DRAG_TRACK_START ||
        state->drag_kind == SHELL_TIMELINE_DRAG_TRACK_END) {
      ShellMotionTrack *track = shell_motion_find_track(state, state->drag_track_id);
      if (track && frame->left_down) {
        uint32_t ms =
            shell_motion_timeline_ms_from_mouse(frame->mouse_x, lane_x, lane_w, duration_ms);
        if (state->drag_kind == SHELL_TIMELINE_DRAG_TRACK_START) {
          uint32_t end_ms = track->start_ms + track->duration_ms;
          track->start_ms = ms > end_ms - 1u ? end_ms - 1u : ms;
          track->duration_ms = end_ms - track->start_ms;
        } else {
          uint32_t new_end = ms > track->start_ms + 1u ? ms : track->start_ms + 1u;
          track->duration_ms = new_end - track->start_ms;
        }
        shell_motion_clip_recompute_duration(state, clip);
        shell_motion_sync_fields(state);
        dirty = true;
      } else if (!frame->left_down) {
        state->drag_kind = SHELL_TIMELINE_DRAG_NONE;
        state->drag_track_id = -1;
      }
    }

    {
      float playhead_x =
          lane_x + lane_w * ((float)state->playhead_ms / (float)duration_ms);
      stygian_rect(ctx, playhead_x, row_y - 6.0f, 2.0f, lane_h, 0.22f, 0.63f, 0.98f,
                   0.95f);
    }
  }
  if (out_dirty)
    *out_dirty = dirty;
}

static void shell_draw_right_panel(StygianContext *ctx, StygianFont font,
                                   const ShellLayout *layout, float mouse_x,
                                   float mouse_y, float panel_width,
                                   bool dragging,
                                   ShellCanvasSceneState *scene,
                                   ShellInspectorState *inspector,
                                   ShellMotionState *motion_state,
                                   const ShellFrameState *frame_state,
                                   uint64_t now_ms, bool left_pressed,
                                   bool force_sync,
                                   bool *out_dirty) {
  bool handle_hover = false;
  bool expanded = false;
  float handle_alpha = 0.0f;
  float tab_x = 0.0f;
  float section_x = 0.0f;
  float content_w = 0.0f;
  ShellCanvasFrameNode *selected_frame = NULL;
  int selection_count = 0;
  bool panel_dirty = false;
  if (!ctx || !font || !layout || !motion_state) {
    if (out_dirty)
      *out_dirty = false;
    return;
  }

  handle_hover =
      shell_hit_right_panel_handle(layout, mouse_x, mouse_y);
  expanded = panel_width > 32.0f;
  handle_alpha = dragging ? 1.0f : (handle_hover ? 0.98f : 0.84f);
  selected_frame = shell_canvas_scene_selected_frame(scene);
  selection_count = shell_canvas_scene_selection_count(scene);
  if (inspector && (!selected_frame || selection_count != 1)) {
    inspector->editing = false;
    inspector->active_field = SHELL_INSPECTOR_NONE;
  }

  if (expanded) {
    shell_draw_panel_block(ctx, layout->right_panel_x, layout->right_panel_y,
                           layout->right_panel_w, layout->right_panel_h, 18.0f,
                           -0.01f);
    if (panel_width > 120.0f) {
      float header_y = layout->right_panel_y + 16.0f;
      float label_y = 0.0f;
      float row_y = 0.0f;
      tab_x = layout->right_panel_x + 16.0f;
      section_x = layout->right_panel_x + 18.0f;
      content_w = layout->right_panel_w - 36.0f;
      if (frame_state && frame_state->left_pressed &&
          shell_point_in_rect(mouse_x, mouse_y, tab_x, header_y, 74.0f, 26.0f)) {
        motion_state->active_tab = SHELL_PANEL_DESIGN;
      } else if (frame_state && frame_state->left_pressed &&
                 shell_point_in_rect(mouse_x, mouse_y, tab_x + 82.0f, header_y,
                                     74.0f, 26.0f)) {
        motion_state->active_tab = SHELL_PANEL_MOTION;
      }

      stygian_rect_rounded(ctx, tab_x, header_y, 74.0f, 26.0f,
                           motion_state->active_tab == SHELL_PANEL_DESIGN ? 0.09f : 0.11f,
                           motion_state->active_tab == SHELL_PANEL_DESIGN ? 0.46f : 0.14f,
                           motion_state->active_tab == SHELL_PANEL_DESIGN ? 0.86f : 0.18f,
                           1.0f, 12.0f);
      stygian_rect_rounded(ctx, tab_x + 1.0f, header_y + 1.0f, 72.0f, 24.0f,
                           0.17f,
                           motion_state->active_tab == SHELL_PANEL_DESIGN ? 0.54f : 0.18f,
                           motion_state->active_tab == SHELL_PANEL_DESIGN ? 0.94f : 0.20f,
                           motion_state->active_tab == SHELL_PANEL_DESIGN ? 0.24f : 0.18f,
                           11.0f);
      stygian_rect_rounded(ctx, tab_x + 82.0f, header_y, 74.0f, 26.0f,
                           motion_state->active_tab == SHELL_PANEL_MOTION ? 0.09f : 0.11f,
                           motion_state->active_tab == SHELL_PANEL_MOTION ? 0.46f : 0.14f,
                           motion_state->active_tab == SHELL_PANEL_MOTION ? 0.86f : 0.18f,
                           1.0f, 12.0f);
      stygian_rect_rounded(ctx, tab_x + 83.0f, header_y + 1.0f, 72.0f, 24.0f,
                           0.17f,
                           motion_state->active_tab == SHELL_PANEL_MOTION ? 0.54f : 0.18f,
                           motion_state->active_tab == SHELL_PANEL_MOTION ? 0.94f : 0.20f,
                           motion_state->active_tab == SHELL_PANEL_MOTION ? 0.24f : 0.18f,
                           11.0f);
      stygian_text(ctx, font, "Design", tab_x + 18.0f, header_y + 6.0f, 14.0f,
                   motion_state->active_tab == SHELL_PANEL_DESIGN ? 0.98f : 0.84f,
                   motion_state->active_tab == SHELL_PANEL_DESIGN ? 0.98f : 0.84f,
                   motion_state->active_tab == SHELL_PANEL_DESIGN ? 0.99f : 0.86f, 1.0f);
      stygian_text(ctx, font, "Motion", tab_x + 100.0f, header_y + 6.0f, 14.0f,
                   motion_state->active_tab == SHELL_PANEL_MOTION ? 0.98f : 0.84f,
                   motion_state->active_tab == SHELL_PANEL_MOTION ? 0.98f : 0.84f,
                   motion_state->active_tab == SHELL_PANEL_MOTION ? 0.99f : 0.86f, 1.0f);
      if (motion_state->active_tab == SHELL_PANEL_MOTION) {
        shell_draw_motion_panel(ctx, font, layout, mouse_x, mouse_y, frame_state,
                                scene, motion_state, now_ms, &panel_dirty);
      } else if (selected_frame && selection_count <= 1) {
        char value[64];
        char aux_value[64];
        bool is_frame = false;
        bool is_ellipse = false;
        bool is_line = false;
        bool is_outline = false;
        bool is_radiusless = false;
        bool is_image = false;
        bool is_path = false;
        bool is_text = false;
        bool allow_fill = false;
        bool allow_stroke = false;
        bool top_hover = false;
        bool hit_any = false;
        float row_h = 34.0f;
        float half_w = 0.0f;
        float third_w = 0.0f;
        float right_x = 0.0f;
        float min_w = 0.0f;
        float min_h = 0.0f;
        const ShellCanvasPage *active_page = NULL;
        const ShellCanvasFrameNode *parent_frame = NULL;
        is_frame = selected_frame->kind == SHELL_NODE_FRAME;
        is_ellipse = selected_frame->kind == SHELL_NODE_ELLIPSE;
        is_line = selected_frame->kind == SHELL_NODE_LINE ||
                  selected_frame->kind == SHELL_NODE_ARROW;
        is_image = selected_frame->kind == SHELL_NODE_IMAGE;
        is_path = selected_frame->kind == SHELL_NODE_PATH;
        is_outline = shell_canvas_node_is_outline_kind(selected_frame->kind);
        is_radiusless = !shell_canvas_node_has_radius(selected_frame->kind);
        is_text = selected_frame->kind == SHELL_NODE_TEXT;
        allow_fill = !is_outline;
        allow_stroke = !is_text;
        min_w = shell_canvas_node_min_width(selected_frame->kind);
        min_h = shell_canvas_node_min_height(selected_frame->kind);
        active_page = shell_canvas_scene_active_page_const(scene);
        parent_frame = selected_frame->parent_id > 0
                           ? shell_canvas_scene_find_node_by_id_const(
                                 scene, selected_frame->parent_id)
                           : NULL;
        stygian_text(ctx, font, shell_canvas_node_kind_label(selected_frame->kind),
                     layout->right_panel_x + 18.0f,
                     header_y + 42.0f, 11.0f, 0.60f, 0.60f, 0.63f, 1.0f);
        stygian_text(ctx, font, selected_frame->name,
                     layout->right_panel_x + 18.0f, header_y + 58.0f, 20.0f,
                     0.95f, 0.95f, 0.97f, 1.0f);
        shell_draw_section_rule(ctx, section_x, header_y + 88.0f, content_w);

        label_y = header_y + 104.0f;
        stygian_text(ctx, font, "Transform", section_x, label_y, 13.0f, 0.91f,
                     0.91f, 0.93f, 1.0f);
        row_y = label_y + 18.0f;
        half_w = content_w * 0.5f - 4.0f;
        right_x = section_x + content_w * 0.5f + 4.0f;
        if (inspector && left_pressed)
          inspector->active_field = SHELL_INSPECTOR_NONE;
        if (left_pressed) {
          if (shell_inspector_hit_input(mouse_x, mouse_y, section_x, row_y,
                                        half_w, row_h)) {
            if (inspector)
              inspector->active_field = SHELL_INSPECTOR_X;
            hit_any = true;
          }
          if (shell_inspector_hit_input(mouse_x, mouse_y, right_x, row_y,
                                        half_w, row_h)) {
            if (inspector)
              inspector->active_field = SHELL_INSPECTOR_Y;
            hit_any = true;
          }
        }
        if (inspector && left_pressed)
          inspector->editing = hit_any;
        if (inspector && !inspector->editing) {
          shell_inspector_sync_from_frame(inspector, selected_frame,
                                          selection_count, force_sync);
        }
        if (inspector) {
          if (shell_draw_numeric_input(ctx, font, section_x, row_y, half_w,
                                       row_h, "X", inspector->x_buf,
                                       (int)sizeof(inspector->x_buf),
                                       inspector->editing &&
                                           inspector->active_field ==
                                               SHELL_INSPECTOR_X)) {
            float parsed = 0.0f;
            if (shell_parse_float(inspector->x_buf, &parsed)) {
              if (selected_frame->x != parsed) {
                selected_frame->x = parsed;
                panel_dirty = true;
              }
            }
          }
          if (shell_draw_numeric_input(ctx, font, right_x, row_y, half_w,
                                       row_h, "Y", inspector->y_buf,
                                       (int)sizeof(inspector->y_buf),
                                       inspector->editing &&
                                           inspector->active_field ==
                                               SHELL_INSPECTOR_Y)) {
            float parsed = 0.0f;
            if (shell_parse_float(inspector->y_buf, &parsed)) {
              if (selected_frame->y != parsed) {
                selected_frame->y = parsed;
                panel_dirty = true;
              }
            }
          }
        }
        row_y += 42.0f;
        if (left_pressed) {
          if (shell_inspector_hit_input(mouse_x, mouse_y, section_x, row_y,
                                        half_w, row_h)) {
            if (inspector)
              inspector->active_field = SHELL_INSPECTOR_W;
            hit_any = true;
          }
          if (shell_inspector_hit_input(mouse_x, mouse_y, right_x, row_y,
                                        half_w, row_h)) {
            if (inspector)
              inspector->active_field = SHELL_INSPECTOR_H;
            hit_any = true;
          }
        }
        if (inspector && left_pressed)
          inspector->editing = hit_any;
        if (inspector) {
          if (shell_draw_numeric_input(ctx, font, section_x, row_y, half_w,
                                       row_h, "W", inspector->w_buf,
                                       (int)sizeof(inspector->w_buf),
                                       inspector->editing &&
                                           inspector->active_field ==
                                               SHELL_INSPECTOR_W)) {
            float parsed = 0.0f;
            if (shell_parse_float(inspector->w_buf, &parsed)) {
              float clamped = shell_clampf(parsed, min_w, 4096.0f);
              if (selected_frame->w != clamped) {
                selected_frame->w = clamped;
                shell_canvas_clamp_frame_radius(selected_frame);
                panel_dirty = true;
              }
              if (clamped != parsed) {
                snprintf(inspector->w_buf, sizeof(inspector->w_buf), "%.0f",
                         clamped);
              }
            }
          }
          if (shell_draw_numeric_input(ctx, font, right_x, row_y, half_w, row_h,
                                       "H", inspector->h_buf,
                                       (int)sizeof(inspector->h_buf),
                                       inspector->editing &&
                                           inspector->active_field ==
                                               SHELL_INSPECTOR_H)) {
            float parsed = 0.0f;
            if (shell_parse_float(inspector->h_buf, &parsed)) {
              float clamped = shell_clampf(parsed, min_h, 4096.0f);
              if (selected_frame->h != clamped) {
                selected_frame->h = clamped;
                shell_canvas_clamp_frame_radius(selected_frame);
                panel_dirty = true;
              }
              if (clamped != parsed) {
                snprintf(inspector->h_buf, sizeof(inspector->h_buf), "%.0f",
                         clamped);
              }
            }
          }
        }
        row_y += 42.0f;
        hit_any = false;
        if (left_pressed &&
            shell_inspector_hit_input(mouse_x, mouse_y, section_x, row_y,
                                      content_w, row_h)) {
          if (inspector)
            inspector->active_field = SHELL_INSPECTOR_ROTATION;
          hit_any = true;
        }
        if (inspector && left_pressed)
          inspector->editing = hit_any;
        if (inspector) {
          if (shell_draw_numeric_input(ctx, font, section_x, row_y, content_w,
                                       row_h, "Rotation",
                                       inspector->rotation_buf,
                                       (int)sizeof(inspector->rotation_buf),
                                       inspector->editing &&
                                           inspector->active_field ==
                                               SHELL_INSPECTOR_ROTATION)) {
            float parsed = 0.0f;
            if (shell_parse_float(inspector->rotation_buf, &parsed)) {
              float normalized = shell_normalize_degrees(parsed);
              if (selected_frame->rotation_deg != normalized) {
                selected_frame->rotation_deg = normalized;
                panel_dirty = true;
              }
              if (normalized != parsed) {
                snprintf(inspector->rotation_buf,
                         sizeof(inspector->rotation_buf), "%.1f", normalized);
              }
            }
          }
        }
        row_y += 42.0f;
        if (is_frame &&
            selected_frame->preset_w > 0 && selected_frame->preset_h > 0 &&
            fabsf(selected_frame->w - (float)selected_frame->preset_w) < 0.5f &&
            fabsf(selected_frame->h - (float)selected_frame->preset_h) < 0.5f) {
          snprintf(value, sizeof(value), "%d x %d", selected_frame->preset_w,
                   selected_frame->preset_h);
        } else {
          snprintf(value, sizeof(value), "%s",
                   selected_frame->kind == SHELL_NODE_FRAME
                       ? "Custom"
                       : shell_canvas_node_kind_label(selected_frame->kind));
        }
        shell_draw_field_box(ctx, font, section_x, row_y, content_w, row_h,
                             is_frame ? "Preset" : "Kind",
                             value, false);

        shell_draw_section_rule(ctx, section_x, row_y + 50.0f, content_w);
        label_y = row_y + 66.0f;
        stygian_text(ctx, font, "Layout", section_x, label_y, 13.0f, 0.91f,
                     0.91f, 0.93f, 1.0f);
        row_y = label_y + 18.0f;
        shell_draw_field_box(ctx, font, section_x, row_y, half_w, row_h, "Page",
                             active_page ? active_page->name : "Page", false);
        shell_draw_field_box(ctx, font, right_x, row_y, half_w, row_h, "Parent",
                             parent_frame ? parent_frame->name : "Root", false);
        row_y += 42.0f;
        snprintf(aux_value, sizeof(aux_value), "%s / %s",
                 selected_frame->visible ? "Visible" : "Hidden",
                 selected_frame->locked ? "Locked" : "Editable");
        shell_draw_field_box(ctx, font, section_x, row_y, half_w, row_h, "State",
                             aux_value, false);
        shell_draw_field_box(ctx, font, right_x, row_y, half_w, row_h,
                             is_frame ? "Top Edge" : "Node",
                             is_frame ? (selected_frame->top_flat ? "Flat"
                                                                  : "Rounded")
                                      : shell_canvas_node_kind_label(
                                            selected_frame->kind),
                             false);
        if (shell_point_in_rect(mouse_x, mouse_y, section_x, row_y, half_w, row_h) &&
            left_pressed) {
          if (!selected_frame->visible) {
            selected_frame->visible = true;
          } else {
            selected_frame->locked = !selected_frame->locked;
          }
          panel_dirty = true;
        }
        if (shell_point_in_rect(mouse_x, mouse_y, right_x, row_y, half_w, row_h) &&
            left_pressed) {
          if (is_frame) {
            selected_frame->top_flat = !selected_frame->top_flat;
          } else {
            shell_canvas_scene_cycle_parent(scene, selected_frame);
          }
          panel_dirty = true;
        }

        shell_draw_section_rule(ctx, section_x, row_y + 50.0f, content_w);
        label_y = row_y + 66.0f;
        stygian_text(ctx, font, "Appearance", section_x, label_y, 13.0f, 0.91f,
                     0.91f, 0.93f, 1.0f);
        row_y = label_y + 18.0f;
        shell_draw_field_box(ctx, font, section_x, row_y, half_w, row_h,
                             allow_fill ? "Fill Style" : "Surface",
                             allow_fill ? shell_fill_mode_label(
                                              selected_frame->fill_mode)
                                        : "Stroke only",
                             false);
        shell_draw_field_box(ctx, font, right_x, row_y, half_w, row_h, "Blend",
                             selected_frame->blend >= 0.999f ? "Normal"
                                                             : "Mixed",
                             false);
        if (allow_fill &&
            shell_point_in_rect(mouse_x, mouse_y, section_x, row_y, half_w, row_h) &&
            left_pressed) {
          selected_frame->fill_mode =
              selected_frame->fill_mode == SHELL_FILL_SOLID ? SHELL_FILL_LINEAR
                                                            : SHELL_FILL_SOLID;
          panel_dirty = true;
        }
        row_y += 42.0f;
        third_w = (content_w - 8.0f) / 3.0f;
        hit_any = false;
        {
          float hit_row_y = row_y;
          if (left_pressed) {
            if (allow_fill) {
              if (shell_inspector_hit_input(mouse_x, mouse_y, section_x,
                                            hit_row_y + 42.0f, third_w, row_h)) {
                if (inspector)
                  inspector->active_field = SHELL_INSPECTOR_FILL_R;
                hit_any = true;
              }
              if (shell_inspector_hit_input(mouse_x, mouse_y,
                                            section_x + third_w + 4.0f,
                                            hit_row_y + 42.0f, third_w, row_h)) {
                if (inspector)
                  inspector->active_field = SHELL_INSPECTOR_FILL_G;
                hit_any = true;
              }
              if (shell_inspector_hit_input(
                      mouse_x, mouse_y, section_x + (third_w + 4.0f) * 2.0f,
                      hit_row_y + 42.0f, third_w, row_h)) {
                if (inspector)
                  inspector->active_field = SHELL_INSPECTOR_FILL_B;
                hit_any = true;
              }
              hit_row_y += 42.0f;
            }
            if (allow_stroke) {
              if (shell_inspector_hit_input(mouse_x, mouse_y, section_x,
                                            hit_row_y + 42.0f, third_w, row_h)) {
                if (inspector)
                  inspector->active_field = SHELL_INSPECTOR_STROKE_R;
                hit_any = true;
              }
              if (shell_inspector_hit_input(mouse_x, mouse_y,
                                            section_x + third_w + 4.0f,
                                            hit_row_y + 42.0f, third_w, row_h)) {
                if (inspector)
                  inspector->active_field = SHELL_INSPECTOR_STROKE_G;
                hit_any = true;
              }
              if (shell_inspector_hit_input(
                      mouse_x, mouse_y, section_x + (third_w + 4.0f) * 2.0f,
                      hit_row_y + 42.0f, third_w, row_h)) {
                if (inspector)
                  inspector->active_field = SHELL_INSPECTOR_STROKE_B;
                hit_any = true;
              }
              hit_row_y += 42.0f;
            }
            if (allow_stroke &&
                shell_inspector_hit_input(mouse_x, mouse_y, section_x,
                                          hit_row_y, half_w, row_h)) {
              if (inspector)
                inspector->active_field = SHELL_INSPECTOR_STROKE_WIDTH;
              hit_any = true;
            }
            if (shell_inspector_hit_input(mouse_x, mouse_y, right_x,
                                          hit_row_y, half_w, row_h)) {
              if (inspector)
                inspector->active_field = SHELL_INSPECTOR_OPACITY;
              hit_any = true;
            }
            hit_row_y += 42.0f;
            if (is_text) {
              if (shell_inspector_hit_input(mouse_x, mouse_y, section_x,
                                            hit_row_y, half_w, row_h)) {
                if (inspector)
                  inspector->active_field = SHELL_INSPECTOR_FONT_SIZE;
                hit_any = true;
              }
              if (shell_inspector_hit_input(mouse_x, mouse_y, section_x,
                                            hit_row_y + 42.0f, content_w,
                                            row_h)) {
                if (inspector)
                  inspector->active_field = SHELL_INSPECTOR_TEXT;
                hit_any = true;
              }
            } else if (!is_image && !is_path && !is_outline) {
              if (shell_inspector_hit_input(mouse_x, mouse_y, section_x,
                                            hit_row_y, half_w, row_h)) {
                if (inspector)
                  inspector->active_field = SHELL_INSPECTOR_RADIUS;
                hit_any = true;
              }
            }
          }
          if (inspector && left_pressed)
            inspector->editing = hit_any;
        }

        if (allow_fill && allow_stroke) {
          shell_draw_color_preview(ctx, font, section_x, row_y, half_w, 40.0f,
                                   "Fill", selected_frame->fill_rgba);
          shell_draw_color_preview(ctx, font, right_x, row_y, half_w, 40.0f,
                                   "Stroke", selected_frame->stroke_rgba);
        } else if (allow_fill) {
          shell_draw_color_preview(ctx, font, section_x, row_y, content_w, 40.0f,
                                   "Fill", selected_frame->fill_rgba);
        } else if (allow_stroke) {
          shell_draw_color_preview(ctx, font, section_x, row_y, content_w, 40.0f,
                                   "Stroke", selected_frame->stroke_rgba);
        }
        row_y += 42.0f;

        if (inspector && allow_fill) {
          if (shell_draw_numeric_input(ctx, font, section_x, row_y, third_w,
                                       row_h, "R", inspector->fill_r_buf,
                                       (int)sizeof(inspector->fill_r_buf),
                                       inspector->editing &&
                                           inspector->active_field ==
                                               SHELL_INSPECTOR_FILL_R)) {
            float parsed = 0.0f;
            if (shell_parse_float(inspector->fill_r_buf, &parsed)) {
              float clamped = shell_clampf(parsed, 0.0f, 255.0f);
              float next = shell_u8_to_color(clamped);
              if (fabsf(selected_frame->fill_rgba[0] - next) > 0.0001f) {
                selected_frame->fill_rgba[0] = next;
                panel_dirty = true;
              }
              if (clamped != parsed)
                snprintf(inspector->fill_r_buf, sizeof(inspector->fill_r_buf),
                         "%.0f", clamped);
            }
          }
          if (shell_draw_numeric_input(ctx, font, section_x + third_w + 4.0f,
                                       row_y, third_w, row_h, "G",
                                       inspector->fill_g_buf,
                                       (int)sizeof(inspector->fill_g_buf),
                                       inspector->editing &&
                                           inspector->active_field ==
                                               SHELL_INSPECTOR_FILL_G)) {
            float parsed = 0.0f;
            if (shell_parse_float(inspector->fill_g_buf, &parsed)) {
              float clamped = shell_clampf(parsed, 0.0f, 255.0f);
              float next = shell_u8_to_color(clamped);
              if (fabsf(selected_frame->fill_rgba[1] - next) > 0.0001f) {
                selected_frame->fill_rgba[1] = next;
                panel_dirty = true;
              }
              if (clamped != parsed)
                snprintf(inspector->fill_g_buf, sizeof(inspector->fill_g_buf),
                         "%.0f", clamped);
            }
          }
          if (shell_draw_numeric_input(
                  ctx, font, section_x + (third_w + 4.0f) * 2.0f, row_y, third_w,
                  row_h, "B", inspector->fill_b_buf,
                  (int)sizeof(inspector->fill_b_buf),
                  inspector->editing &&
                      inspector->active_field == SHELL_INSPECTOR_FILL_B)) {
            float parsed = 0.0f;
            if (shell_parse_float(inspector->fill_b_buf, &parsed)) {
              float clamped = shell_clampf(parsed, 0.0f, 255.0f);
              float next = shell_u8_to_color(clamped);
              if (fabsf(selected_frame->fill_rgba[2] - next) > 0.0001f) {
                selected_frame->fill_rgba[2] = next;
                panel_dirty = true;
              }
              if (clamped != parsed)
                snprintf(inspector->fill_b_buf, sizeof(inspector->fill_b_buf),
                         "%.0f", clamped);
            }
          }
          row_y += 42.0f;
        }

        if (allow_fill && selected_frame->fill_mode == SHELL_FILL_LINEAR) {
          shell_draw_color_preview(ctx, font, section_x, row_y, half_w, 40.0f,
                                   "Fill B", selected_frame->fill_alt_rgba);
          if (inspector) {
            if (shell_draw_numeric_input(ctx, font, section_x, row_y + 42.0f,
                                         third_w, row_h, "R",
                                         inspector->fill2_r_buf,
                                         (int)sizeof(inspector->fill2_r_buf),
                                         false)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->fill2_r_buf, &parsed)) {
                float clamped = shell_clampf(parsed, 0.0f, 255.0f);
                selected_frame->fill_alt_rgba[0] = shell_u8_to_color(clamped);
                panel_dirty = true;
              }
            }
            if (shell_draw_numeric_input(ctx, font,
                                         section_x + third_w + 4.0f,
                                         row_y + 42.0f, third_w, row_h, "G",
                                         inspector->fill2_g_buf,
                                         (int)sizeof(inspector->fill2_g_buf),
                                         false)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->fill2_g_buf, &parsed)) {
                float clamped = shell_clampf(parsed, 0.0f, 255.0f);
                selected_frame->fill_alt_rgba[1] = shell_u8_to_color(clamped);
                panel_dirty = true;
              }
            }
            if (shell_draw_numeric_input(
                    ctx, font, section_x + (third_w + 4.0f) * 2.0f,
                    row_y + 42.0f, third_w, row_h, "B", inspector->fill2_b_buf,
                    (int)sizeof(inspector->fill2_b_buf), false)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->fill2_b_buf, &parsed)) {
                float clamped = shell_clampf(parsed, 0.0f, 255.0f);
                selected_frame->fill_alt_rgba[2] = shell_u8_to_color(clamped);
                panel_dirty = true;
              }
            }
            if (shell_draw_numeric_input(
                    ctx, font, right_x, row_y, half_w, row_h, "Angle",
                    inspector->gradient_angle_buf,
                    (int)sizeof(inspector->gradient_angle_buf), false)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->gradient_angle_buf, &parsed)) {
                selected_frame->fill_angle_deg = shell_normalize_degrees(parsed);
                panel_dirty = true;
              }
            }
          }
          row_y += 84.0f;
        }

        if (inspector && allow_stroke) {
          if (shell_draw_numeric_input(ctx, font, section_x, row_y, third_w,
                                       row_h, "R", inspector->stroke_r_buf,
                                       (int)sizeof(inspector->stroke_r_buf),
                                       inspector->editing &&
                                           inspector->active_field ==
                                               SHELL_INSPECTOR_STROKE_R)) {
            float parsed = 0.0f;
            if (shell_parse_float(inspector->stroke_r_buf, &parsed)) {
              float clamped = shell_clampf(parsed, 0.0f, 255.0f);
              float next = shell_u8_to_color(clamped);
              if (fabsf(selected_frame->stroke_rgba[0] - next) > 0.0001f) {
                selected_frame->stroke_rgba[0] = next;
                panel_dirty = true;
              }
              if (clamped != parsed)
                snprintf(inspector->stroke_r_buf,
                         sizeof(inspector->stroke_r_buf), "%.0f", clamped);
            }
          }
          if (shell_draw_numeric_input(ctx, font, section_x + third_w + 4.0f,
                                       row_y, third_w, row_h, "G",
                                       inspector->stroke_g_buf,
                                       (int)sizeof(inspector->stroke_g_buf),
                                       inspector->editing &&
                                           inspector->active_field ==
                                               SHELL_INSPECTOR_STROKE_G)) {
            float parsed = 0.0f;
            if (shell_parse_float(inspector->stroke_g_buf, &parsed)) {
              float clamped = shell_clampf(parsed, 0.0f, 255.0f);
              float next = shell_u8_to_color(clamped);
              if (fabsf(selected_frame->stroke_rgba[1] - next) > 0.0001f) {
                selected_frame->stroke_rgba[1] = next;
                panel_dirty = true;
              }
              if (clamped != parsed)
                snprintf(inspector->stroke_g_buf,
                         sizeof(inspector->stroke_g_buf), "%.0f", clamped);
            }
          }
          if (shell_draw_numeric_input(
                  ctx, font, section_x + (third_w + 4.0f) * 2.0f, row_y, third_w,
                  row_h, "B", inspector->stroke_b_buf,
                  (int)sizeof(inspector->stroke_b_buf),
                  inspector->editing &&
                      inspector->active_field == SHELL_INSPECTOR_STROKE_B)) {
            float parsed = 0.0f;
            if (shell_parse_float(inspector->stroke_b_buf, &parsed)) {
              float clamped = shell_clampf(parsed, 0.0f, 255.0f);
              float next = shell_u8_to_color(clamped);
              if (fabsf(selected_frame->stroke_rgba[2] - next) > 0.0001f) {
                selected_frame->stroke_rgba[2] = next;
                panel_dirty = true;
              }
              if (clamped != parsed)
                snprintf(inspector->stroke_b_buf,
                         sizeof(inspector->stroke_b_buf), "%.0f", clamped);
            }
          }
          row_y += 42.0f;
        }

        if (inspector && allow_stroke) {
          if (shell_draw_numeric_input(
                  ctx, font, section_x, row_y, half_w, row_h, "Stroke",
                  inspector->stroke_width_buf,
                  (int)sizeof(inspector->stroke_width_buf),
                  inspector->editing &&
                      inspector->active_field == SHELL_INSPECTOR_STROKE_WIDTH)) {
            float parsed = 0.0f;
            if (shell_parse_float(inspector->stroke_width_buf, &parsed)) {
              float clamped = shell_clampf(parsed, 0.0f, 24.0f);
              if (fabsf(selected_frame->stroke_width - clamped) > 0.0001f) {
                selected_frame->stroke_width = clamped;
                panel_dirty = true;
              }
              if (clamped != parsed) {
                snprintf(inspector->stroke_width_buf,
                         sizeof(inspector->stroke_width_buf), "%.1f", clamped);
              }
            }
          }
        } else {
          shell_draw_field_box(ctx, font, section_x, row_y, half_w, row_h,
                               "Stroke", "N/A", false);
        }
        if (inspector) {
          if (shell_draw_numeric_input(ctx, font, right_x, row_y, half_w, row_h,
                                       "Opacity", inspector->opacity_buf,
                                       (int)sizeof(inspector->opacity_buf),
                                       inspector->editing &&
                                           inspector->active_field ==
                                               SHELL_INSPECTOR_OPACITY)) {
            float parsed = 0.0f;
            if (shell_parse_float(inspector->opacity_buf, &parsed)) {
              float clamped = shell_clampf(parsed, 0.0f, 100.0f);
              float next = clamped / 100.0f;
              if (fabsf(selected_frame->opacity - next) > 0.0001f) {
                selected_frame->opacity = next;
                panel_dirty = true;
              }
              if (clamped != parsed) {
                snprintf(inspector->opacity_buf, sizeof(inspector->opacity_buf),
                         "%.0f", clamped);
              }
            }
          }
        }
        if (inspector) {
          if (shell_draw_numeric_input(ctx, font, section_x, row_y, half_w, row_h,
                                       "Blend", inspector->blend_buf,
                                       (int)sizeof(inspector->blend_buf),
                                       false)) {
            float parsed = 0.0f;
            if (shell_parse_float(inspector->blend_buf, &parsed)) {
              selected_frame->blend = shell_clampf(parsed, 0.05f, 1.0f);
              panel_dirty = true;
            }
          }
        }
        row_y += 50.0f;
        shell_draw_section_rule(ctx, section_x, row_y - 8.0f, content_w);
        stygian_text(ctx, font, "Effects", section_x, row_y + 8.0f, 13.0f, 0.91f,
                     0.91f, 0.93f, 1.0f);
        shell_draw_field_box(ctx, font, section_x, row_y + 26.0f, half_w, row_h,
                             "Effect",
                             shell_effect_kind_label(selected_frame->effect_kind),
                             false);
        if (shell_point_in_rect(mouse_x, mouse_y, section_x, row_y + 26.0f, half_w,
                                row_h) &&
            left_pressed) {
          if (selected_frame->effect_kind == SHELL_EFFECT_GLOW)
            selected_frame->effect_kind = SHELL_EFFECT_NONE;
          else
            selected_frame->effect_kind =
                (ShellCanvasEffectKind)(selected_frame->effect_kind + 1);
          panel_dirty = true;
        }
        row_y += 68.0f;

        if (selected_frame->effect_kind != SHELL_EFFECT_NONE) {
          shell_draw_color_preview(ctx, font, section_x, row_y, half_w, 40.0f,
                                   "FX Color", selected_frame->effect_rgba);
          if (inspector) {
            if (shell_draw_numeric_input(ctx, font, section_x, row_y + 42.0f,
                                         third_w, row_h, "R",
                                         inspector->effect_r_buf,
                                         (int)sizeof(inspector->effect_r_buf),
                                         false)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->effect_r_buf, &parsed)) {
                selected_frame->effect_rgba[0] =
                    shell_u8_to_color(shell_clampf(parsed, 0.0f, 255.0f));
                panel_dirty = true;
              }
            }
            if (shell_draw_numeric_input(ctx, font,
                                         section_x + third_w + 4.0f,
                                         row_y + 42.0f, third_w, row_h, "G",
                                         inspector->effect_g_buf,
                                         (int)sizeof(inspector->effect_g_buf),
                                         false)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->effect_g_buf, &parsed)) {
                selected_frame->effect_rgba[1] =
                    shell_u8_to_color(shell_clampf(parsed, 0.0f, 255.0f));
                panel_dirty = true;
              }
            }
            if (shell_draw_numeric_input(
                    ctx, font, section_x + (third_w + 4.0f) * 2.0f,
                    row_y + 42.0f, third_w, row_h, "B", inspector->effect_b_buf,
                    (int)sizeof(inspector->effect_b_buf), false)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->effect_b_buf, &parsed)) {
                selected_frame->effect_rgba[2] =
                    shell_u8_to_color(shell_clampf(parsed, 0.0f, 255.0f));
                panel_dirty = true;
              }
            }
          }
          row_y += 84.0f;
          if (inspector) {
            if (shell_draw_numeric_input(ctx, font, section_x, row_y, half_w, row_h,
                                         "FX Radius",
                                         inspector->effect_radius_buf,
                                         (int)sizeof(inspector->effect_radius_buf),
                                         false)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->effect_radius_buf, &parsed)) {
                selected_frame->effect_radius =
                    shell_clampf(parsed, 0.0f, 64.0f);
                panel_dirty = true;
              }
            }
            if (shell_draw_numeric_input(ctx, font, right_x, row_y, half_w, row_h,
                                         "FX Spread",
                                         inspector->effect_spread_buf,
                                         (int)sizeof(inspector->effect_spread_buf),
                                         false)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->effect_spread_buf, &parsed)) {
                selected_frame->effect_spread =
                    shell_clampf(parsed, 0.0f, 32.0f);
                panel_dirty = true;
              }
            }
          }
          row_y += 42.0f;
          if (inspector) {
            if (shell_draw_numeric_input(ctx, font, section_x, row_y, half_w, row_h,
                                         "Offset X",
                                         inspector->effect_offset_x_buf,
                                         (int)sizeof(inspector->effect_offset_x_buf),
                                         false)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->effect_offset_x_buf, &parsed)) {
                selected_frame->effect_offset_x =
                    shell_clampf(parsed, -128.0f, 128.0f);
                panel_dirty = true;
              }
            }
            if (shell_draw_numeric_input(ctx, font, right_x, row_y, half_w, row_h,
                                         "Offset Y",
                                         inspector->effect_offset_y_buf,
                                         (int)sizeof(inspector->effect_offset_y_buf),
                                         false)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->effect_offset_y_buf, &parsed)) {
                selected_frame->effect_offset_y =
                    shell_clampf(parsed, -128.0f, 128.0f);
                panel_dirty = true;
              }
            }
          }
          row_y += 42.0f;
          if (selected_frame->effect_kind == SHELL_EFFECT_GLOW && inspector) {
            if (shell_draw_numeric_input(
                    ctx, font, section_x, row_y, half_w, row_h, "Intensity",
                    inspector->effect_intensity_buf,
                    (int)sizeof(inspector->effect_intensity_buf), false)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->effect_intensity_buf, &parsed)) {
                selected_frame->effect_intensity =
                    shell_clampf(parsed, 0.0f, 16.0f);
                panel_dirty = true;
              }
            }
            row_y += 42.0f;
          }
        }

        if (is_text) {
          shell_draw_section_rule(ctx, section_x, row_y - 8.0f, content_w);
          stygian_text(ctx, font, "Text", section_x, row_y + 8.0f, 13.0f, 0.91f,
                       0.91f, 0.93f, 1.0f);
          row_y += 26.0f;
          if (inspector) {
            if (shell_draw_numeric_input(
                    ctx, font, section_x, row_y, half_w, row_h, "Font",
                    inspector->font_size_buf,
                    (int)sizeof(inspector->font_size_buf),
                    inspector->editing &&
                        inspector->active_field == SHELL_INSPECTOR_FONT_SIZE)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->font_size_buf, &parsed)) {
                float clamped = shell_clampf(parsed, 8.0f, 256.0f);
                if (selected_frame->font_size != clamped) {
                  selected_frame->font_size = clamped;
                  panel_dirty = true;
                }
                if (clamped != parsed) {
                  snprintf(inspector->font_size_buf,
                           sizeof(inspector->font_size_buf), "%.0f", clamped);
                }
              }
            }
            if (shell_draw_numeric_input(
                    ctx, font, right_x, row_y, half_w, row_h, "Weight",
                    inspector->font_weight_buf,
                    (int)sizeof(inspector->font_weight_buf), false)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->font_weight_buf, &parsed)) {
                selected_frame->font_weight =
                    (uint32_t)shell_clampf(parsed, 100.0f, 900.0f);
                panel_dirty = true;
              }
            }
            row_y += 42.0f;
            if (shell_draw_numeric_input(
                    ctx, font, section_x, row_y, half_w, row_h, "Line H",
                    inspector->line_height_buf,
                    (int)sizeof(inspector->line_height_buf), false)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->line_height_buf, &parsed)) {
                selected_frame->line_height = shell_clampf(parsed, 0.0f, 256.0f);
                panel_dirty = true;
              }
            }
            if (shell_draw_numeric_input(
                    ctx, font, right_x, row_y, half_w, row_h, "Track",
                    inspector->letter_spacing_buf,
                    (int)sizeof(inspector->letter_spacing_buf), false)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->letter_spacing_buf, &parsed)) {
                selected_frame->letter_spacing =
                    shell_clampf(parsed, -4.0f, 32.0f);
                panel_dirty = true;
              }
            }
            row_y += 42.0f;
            shell_draw_field_box(ctx, font, section_x, row_y, half_w, row_h,
                                 "Align H",
                                 shell_text_align_h_label(
                                     selected_frame->text_align_h),
                                 false);
            shell_draw_field_box(ctx, font, right_x, row_y, half_w, row_h,
                                 "Align V",
                                 shell_text_align_v_label(
                                     selected_frame->text_align_v),
                                 false);
            if (shell_point_in_rect(mouse_x, mouse_y, section_x, row_y, half_w,
                                    row_h) &&
                left_pressed) {
              if (selected_frame->text_align_h == SHELL_TEXT_ALIGN_RIGHT)
                selected_frame->text_align_h = SHELL_TEXT_ALIGN_LEFT;
              else
                selected_frame->text_align_h =
                    (ShellCanvasTextAlignH)(selected_frame->text_align_h + 1);
              panel_dirty = true;
            }
            if (shell_point_in_rect(mouse_x, mouse_y, right_x, row_y, half_w,
                                    row_h) &&
                left_pressed) {
              if (selected_frame->text_align_v == SHELL_TEXT_ALIGN_BOTTOM)
                selected_frame->text_align_v = SHELL_TEXT_ALIGN_TOP;
              else
                selected_frame->text_align_v =
                    (ShellCanvasTextAlignV)(selected_frame->text_align_v + 1);
              panel_dirty = true;
            }
            row_y += 42.0f;
            shell_draw_field_box(ctx, font, section_x, row_y, half_w, row_h,
                                 "Wrap", selected_frame->text_wrap ? "On" : "Off",
                                 false);
            shell_draw_field_box(ctx, font, right_x, row_y, half_w, row_h,
                                 "Overflow",
                                 shell_text_overflow_label(
                                     selected_frame->text_overflow),
                                 false);
            if (shell_point_in_rect(mouse_x, mouse_y, section_x, row_y, half_w,
                                    row_h) &&
                left_pressed) {
              selected_frame->text_wrap = !selected_frame->text_wrap;
              panel_dirty = true;
            }
            if (shell_point_in_rect(mouse_x, mouse_y, right_x, row_y, half_w,
                                    row_h) &&
                left_pressed) {
              if (selected_frame->text_overflow == SHELL_TEXT_OVERFLOW_VISIBLE)
                selected_frame->text_overflow = SHELL_TEXT_OVERFLOW_CLIP;
              else
                selected_frame->text_overflow =
                    (ShellCanvasTextOverflow)(selected_frame->text_overflow + 1);
              panel_dirty = true;
            }
            if (shell_draw_labeled_text_input(
                    ctx, font, section_x, row_y + 42.0f, content_w, row_h,
                    "Content", inspector->text_buf,
                    (int)sizeof(inspector->text_buf),
                    inspector->editing &&
                        inspector->active_field == SHELL_INSPECTOR_TEXT)) {
              if (strcmp(selected_frame->text, inspector->text_buf) != 0) {
                snprintf(selected_frame->text, sizeof(selected_frame->text),
                         "%s", inspector->text_buf);
                panel_dirty = true;
              }
            }
          }
        } else if (is_image) {
          char filename[STYGIAN_FS_NAME_CAP] = {0};
          char dims[64] = {0};
          stygian_fs_path_filename(selected_frame->asset_path, filename,
                                   sizeof(filename));
          if (selected_frame->image_w > 0 && selected_frame->image_h > 0) {
            snprintf(dims, sizeof(dims), "%d x %d", selected_frame->image_w,
                     selected_frame->image_h);
          } else {
            snprintf(dims, sizeof(dims), "%s", "Pending");
          }
          shell_draw_field_box(ctx, font, section_x, row_y, half_w, row_h,
                               "Source",
                               filename[0] ? filename : "Imported image", false);
          shell_draw_field_box(ctx, font, right_x, row_y, half_w, row_h, "Pixels",
                               dims, false);
          row_y += 42.0f;
          shell_draw_field_box(ctx, font, section_x, row_y, half_w, row_h, "Fit",
                               shell_image_fit_label(
                                   selected_frame->image_fit_mode),
                               false);
          shell_draw_field_box(ctx, font, right_x, row_y, half_w, row_h,
                               "Crop",
                               selected_frame->image_fit_mode ==
                                       SHELL_IMAGE_FIT_COVER
                                   ? "Cover"
                                   : "None",
                               false);
          if (shell_point_in_rect(mouse_x, mouse_y, section_x, row_y, half_w,
                                  row_h) &&
              left_pressed) {
            if (selected_frame->image_fit_mode == SHELL_IMAGE_FIT_COVER)
              selected_frame->image_fit_mode = SHELL_IMAGE_FIT_STRETCH;
            else
              selected_frame->image_fit_mode =
                  (ShellCanvasImageFitMode)(selected_frame->image_fit_mode + 1);
            panel_dirty = true;
          }
          if (shell_point_in_rect(mouse_x, mouse_y, right_x, row_y, half_w,
                                  row_h) &&
              left_pressed) {
            if (selected_frame->image_fit_mode == SHELL_IMAGE_FIT_COVER)
              selected_frame->image_fit_mode = SHELL_IMAGE_FIT_CONTAIN;
            else
              selected_frame->image_fit_mode = SHELL_IMAGE_FIT_COVER;
            panel_dirty = true;
          }
        } else if (is_path || is_outline) {
          char point_count_text[32];
          if (is_path) {
            snprintf(point_count_text, sizeof(point_count_text), "%d",
                     selected_frame->point_count);
            shell_draw_field_box(ctx, font, section_x, row_y, half_w, row_h,
                                 "Points", point_count_text, false);
            shell_draw_field_box(ctx, font, right_x, row_y, half_w, row_h,
                                 "Shape",
                                 selected_frame->path_closed ? "Closed"
                                                             : "Open",
                                 false);
          } else {
            shell_draw_field_box(ctx, font, section_x, row_y, half_w, row_h,
                                 "Stroke",
                                 is_line ? "Single segment" : "Outline", false);
            shell_draw_field_box(ctx, font, right_x, row_y, half_w, row_h,
                                 "Shape",
                                 shell_canvas_node_kind_label(
                                     selected_frame->kind),
                                 false);
          }
        } else {
          if (inspector && !is_radiusless && !selected_frame->corner_custom) {
            if (shell_draw_numeric_input(ctx, font, section_x, row_y, half_w,
                                         row_h, "Radius", inspector->radius_buf,
                                         (int)sizeof(inspector->radius_buf),
                                         inspector->editing &&
                                             inspector->active_field ==
                                                 SHELL_INSPECTOR_RADIUS)) {
              float parsed = 0.0f;
              if (shell_parse_float(inspector->radius_buf, &parsed)) {
                float max_radius =
                    fminf(selected_frame->w, selected_frame->h) * 0.5f;
                float clamped = shell_clampf(parsed, 0.0f, max_radius);
                int ci = 0;
                selected_frame->radius = clamped;
                for (ci = 0; ci < 4; ++ci)
                  selected_frame->corner_radius[ci] = clamped;
                panel_dirty = true;
                if (clamped != parsed) {
                  snprintf(inspector->radius_buf,
                           sizeof(inspector->radius_buf), "%.0f", clamped);
                }
              }
            }
          } else {
            shell_draw_field_box(ctx, font, section_x, row_y, half_w, row_h,
                                 "Radius", is_ellipse ? "Auto" : "N/A", false);
          }
          shell_draw_field_box(ctx, font, right_x, row_y, half_w, row_h,
                               is_frame ? "Top Edge" : "Shape",
                               is_frame
                                   ? (selected_frame->top_flat ? "Flat"
                                                               : "Rounded")
                                   : shell_canvas_node_kind_label(
                                         selected_frame->kind),
                               false);
          top_hover =
              shell_point_in_rect(mouse_x, mouse_y, right_x, row_y, half_w,
                                  row_h);
          if (is_frame && top_hover && left_pressed) {
            selected_frame->top_flat = !selected_frame->top_flat;
            panel_dirty = true;
          }
          if (!is_radiusless) {
            row_y += 42.0f;
            shell_draw_field_box(ctx, font, section_x, row_y, half_w, row_h,
                                 "Corners",
                                 selected_frame->corner_custom ? "Custom"
                                                               : "Uniform",
                                 false);
            if (shell_point_in_rect(mouse_x, mouse_y, section_x, row_y, half_w,
                                    row_h) &&
                left_pressed) {
              selected_frame->corner_custom = !selected_frame->corner_custom;
              shell_canvas_clamp_frame_radius(selected_frame);
              panel_dirty = true;
            }
            if (selected_frame->corner_custom && inspector) {
              if (shell_draw_numeric_input(ctx, font, right_x, row_y, half_w,
                                           row_h, "TL",
                                           inspector->radius_tl_buf,
                                           (int)sizeof(inspector->radius_tl_buf),
                                           false)) {
                float parsed = 0.0f;
                if (shell_parse_float(inspector->radius_tl_buf, &parsed)) {
                  selected_frame->corner_radius[0] = shell_clampf(
                      parsed, 0.0f, fminf(selected_frame->w, selected_frame->h) *
                                         0.5f);
                  panel_dirty = true;
                }
              }
              row_y += 42.0f;
              if (shell_draw_numeric_input(ctx, font, section_x, row_y, half_w,
                                           row_h, "TR",
                                           inspector->radius_tr_buf,
                                           (int)sizeof(inspector->radius_tr_buf),
                                           false)) {
                float parsed = 0.0f;
                if (shell_parse_float(inspector->radius_tr_buf, &parsed)) {
                  selected_frame->corner_radius[1] = shell_clampf(
                      parsed, 0.0f, fminf(selected_frame->w, selected_frame->h) *
                                         0.5f);
                  panel_dirty = true;
                }
              }
              if (shell_draw_numeric_input(ctx, font, right_x, row_y, half_w,
                                           row_h, "BR",
                                           inspector->radius_br_buf,
                                           (int)sizeof(inspector->radius_br_buf),
                                           false)) {
                float parsed = 0.0f;
                if (shell_parse_float(inspector->radius_br_buf, &parsed)) {
                  selected_frame->corner_radius[2] = shell_clampf(
                      parsed, 0.0f, fminf(selected_frame->w, selected_frame->h) *
                                         0.5f);
                  panel_dirty = true;
                }
              }
              row_y += 42.0f;
              if (shell_draw_numeric_input(ctx, font, section_x, row_y, half_w,
                                           row_h, "BL",
                                           inspector->radius_bl_buf,
                                           (int)sizeof(inspector->radius_bl_buf),
                                           false)) {
                float parsed = 0.0f;
                if (shell_parse_float(inspector->radius_bl_buf, &parsed)) {
                  selected_frame->corner_radius[3] = shell_clampf(
                      parsed, 0.0f, fminf(selected_frame->w, selected_frame->h) *
                                         0.5f);
                  panel_dirty = true;
                }
              }
            }
          }
        }
        shell_draw_section_rule(ctx, section_x, row_y + 26.0f, content_w);
        stygian_text(ctx, font, "Interaction", section_x, row_y + 42.0f, 13.0f,
                     0.91f, 0.91f, 0.93f, 1.0f);
        snprintf(value, sizeof(value), "%d clips",
                 shell_motion_find_clip_for_node(motion_state, selected_frame->id)
                     ? 1
                     : 0);
        shell_draw_field_box(ctx, font, section_x, row_y + 60.0f, half_w, row_h,
                             "Motion", value, false);
        shell_draw_field_box(ctx, font, right_x, row_y + 60.0f, half_w, row_h,
                             "Selection",
                             selected_frame->selected ? "Active" : "Idle", false);
        if (left_pressed &&
            shell_point_in_rect(mouse_x, mouse_y, section_x, row_y + 60.0f, half_w,
                                row_h)) {
          motion_state->active_tab = SHELL_PANEL_MOTION;
        }
        if (panel_dirty)
          shell_inspector_sync_from_frame(inspector, selected_frame,
                                          selection_count, true);
      } else if (selection_count > 1) {
        char value[64];
        stygian_text(ctx, font, "Selection", layout->right_panel_x + 18.0f,
                     header_y + 42.0f, 11.0f, 0.60f, 0.60f, 0.63f, 1.0f);
        snprintf(value, sizeof(value), "%d frames selected", selection_count);
        stygian_text(ctx, font, value, layout->right_panel_x + 18.0f,
                     header_y + 58.0f, 18.0f, 0.95f, 0.95f, 0.97f, 1.0f);
        stygian_text(ctx, font,
                     "Move or resize them as one. Guides snap off the shared bounds.",
                     layout->right_panel_x + 18.0f, header_y + 88.0f, 13.0f,
                     0.62f, 0.64f, 0.68f, 1.0f);
      } else {
        stygian_text(ctx, font, "Selection", layout->right_panel_x + 18.0f,
                     header_y + 42.0f, 11.0f, 0.60f, 0.60f, 0.63f, 1.0f);
        stygian_text(ctx, font, "No frame selected", layout->right_panel_x + 18.0f,
                     header_y + 58.0f, 18.0f, 0.95f, 0.95f, 0.97f, 1.0f);
        stygian_text(ctx, font,
                     "Pick Select and click a frame, or use the Frame tool to place one.",
                     layout->right_panel_x + 18.0f, header_y + 88.0f, 13.0f,
                     0.62f, 0.64f, 0.68f, 1.0f);
      }
    }
  }

  if (!expanded) {
    stygian_rect_rounded(ctx, layout->right_panel_x, layout->right_panel_y,
                         layout->right_panel_w, layout->right_panel_h, 0.08f,
                         0.08f, 0.09f, 0.98f, 8.0f);
    stygian_rect(ctx, layout->right_panel_x, layout->right_panel_y + 8.0f,
                 layout->right_panel_w, layout->right_panel_h - 16.0f, 0.08f, 0.08f,
                 0.09f, 0.98f);
    stygian_rect_rounded(ctx, layout->right_panel_x, layout->right_panel_y,
                         layout->right_panel_w + 1.0f, 22.0f,
                         handle_hover || dragging ? 0.13f : 0.10f,
                         handle_hover || dragging ? 0.18f : 0.10f,
                         handle_hover || dragging ? 0.24f : 0.11f,
                         0.94f, 8.0f);
  } else {
    stygian_rect(ctx, layout->right_panel_handle_x, layout->right_panel_handle_y,
                 layout->right_panel_handle_w, layout->right_panel_handle_h, 0.08f,
                 0.08f, 0.09f, 0.98f);
    stygian_rect(ctx, layout->right_panel_handle_x + 1.0f,
                 layout->right_panel_handle_y + 1.0f,
                 layout->right_panel_handle_w - 1.0f,
                 layout->right_panel_handle_h - 2.0f,
                 handle_hover || dragging ? 0.13f : 0.10f,
                 handle_hover || dragging ? 0.18f : 0.10f,
                 handle_hover || dragging ? 0.24f : 0.11f, 0.90f);
  }
  stygian_line(ctx, layout->right_panel_handle_x + layout->right_panel_handle_w * 0.5f,
               layout->right_panel_handle_y + 22.0f,
               layout->right_panel_handle_x + layout->right_panel_handle_w * 0.5f,
               layout->right_panel_handle_y + layout->right_panel_handle_h - 22.0f,
               1.0f, 0.45f, 0.49f, 0.56f, 0.44f + handle_alpha * 0.26f);

  if (expanded) {
    shell_draw_chevron_right(ctx,
                             layout->right_panel_handle_x +
                                 layout->right_panel_handle_w * 0.5f,
                             layout->right_panel_handle_y +
                                 layout->right_panel_handle_h * 0.5f,
                             3.0f, 1.25f, 0.88f, 0.88f, 0.90f, handle_alpha);
  } else {
    shell_draw_chevron_left(ctx,
                            layout->right_panel_handle_x +
                                layout->right_panel_handle_w * 0.5f,
                            layout->right_panel_handle_y +
                                layout->right_panel_handle_h * 0.5f,
                            3.0f, 1.25f, 0.88f, 0.88f, 0.90f, handle_alpha);
  }
  if (out_dirty)
    *out_dirty = panel_dirty;
}

static void shell_frame_family_preset(int choice, int *out_w, int *out_h) {
  int w = 1440;
  int h = 900;
  switch (choice) {
  case 1:
    w = 1024;
    h = 768;
    break;
  case 2:
    w = 430;
    h = 932;
    break;
  default:
    break;
  }
  if (out_w)
    *out_w = w;
  if (out_h)
    *out_h = h;
}

static void shell_draw_top_menu_strip(StygianContext *ctx, StygianFont font,
                                      const ShellLayout *layout, float mouse_x,
                                      float mouse_y, int open_menu) {
  float x = 0.0f;
  int i = 0;
  if (!ctx || !layout)
    return;
  x = layout->menu_x;
  for (i = 0; i < shell_top_menu_count(); ++i) {
    const char *label = shell_top_menu_label(i);
    float slot_w = shell_top_menu_slot_width(i);
    bool hovered =
        shell_point_in_rect(mouse_x, mouse_y, x, layout->menu_y, slot_w,
                            layout->menu_h);
    if (hovered || open_menu == i) {
      stygian_rect_rounded(ctx, x, layout->menu_y, slot_w, layout->menu_h,
                           open_menu == i ? 0.12f : 0.13f,
                           open_menu == i ? 0.17f : 0.13f,
                           open_menu == i ? 0.26f : 0.14f, 0.94f, 8.0f);
      stygian_rect_rounded(ctx, x + 1.0f, layout->menu_y + 1.0f, slot_w - 2.0f,
                           layout->menu_h - 2.0f, 0.18f, 0.18f, 0.19f,
                           open_menu == i ? 0.34f : 0.28f,
                           7.0f);
    }
    stygian_text(ctx, font, label, x + 10.0f, layout->menu_y + 6.0f, 14.0f,
                 (hovered || open_menu == i) ? 0.97f : 0.92f,
                 (hovered || open_menu == i) ? 0.97f : 0.92f,
                 (hovered || open_menu == i) ? 0.98f : 0.93f, 1.0f);
    x += slot_w;
    if (i + 1 < shell_top_menu_count())
      x += shell_top_menu_gap();
  }
}

static void shell_draw_top_menu_popup(StygianContext *ctx, StygianFont font,
                                      const ShellLayout *layout, float mouse_x,
                                      float mouse_y, int open_menu,
                                      bool show_tool_dock,
                                      bool grid_snap_enabled) {
  float popup_x = 0.0f;
  float popup_y = 0.0f;
  float popup_w = 0.0f;
  float popup_h = 0.0f;
  int item_count = 0;
  int i = 0;
  if (!ctx || !layout || open_menu < 0)
    return;

  item_count = shell_menu_item_count(open_menu);
  shell_compute_menu_popup_rect(layout, open_menu, &popup_x, &popup_y, &popup_w,
                                &popup_h);
  stygian_rect_rounded(ctx, popup_x, popup_y, popup_w, popup_h, 0.05f, 0.05f,
                       0.06f, 0.98f, 12.0f);
  stygian_rect_rounded(ctx, popup_x + 1.0f, popup_y + 1.0f, popup_w - 2.0f,
                       popup_h - 2.0f, 0.12f, 0.12f, 0.13f, 0.92f, 11.0f);
  for (i = 0; i < item_count; ++i) {
    float row_x = popup_x + 6.0f;
    float row_y = popup_y + 6.0f + i * 28.0f;
    float row_w = popup_w - 12.0f;
    float row_h = 24.0f;
    bool hovered =
        shell_point_in_rect(mouse_x, mouse_y, row_x, row_y, row_w, row_h);
    if (hovered) {
      stygian_rect_rounded(ctx, row_x, row_y, row_w, row_h, 0.11f, 0.17f,
                           0.25f, 0.92f, 8.0f);
      stygian_rect_rounded(ctx, row_x + 1.0f, row_y + 1.0f, row_w - 2.0f,
                           row_h - 2.0f, 0.17f, 0.22f, 0.30f, 0.28f, 7.0f);
    }
    stygian_text(ctx, font,
                 shell_menu_item_label(open_menu, i, show_tool_dock,
                                       grid_snap_enabled),
                 row_x + 10.0f, row_y + 5.0f, 13.0f, hovered ? 0.97f : 0.90f,
                 hovered ? 0.97f : 0.90f, hovered ? 0.98f : 0.92f, 1.0f);
  }
}

static void shell_draw_icon_lab_grid(StygianContext *ctx, float x, float y,
                                     float w, float h, float step) {
  int ix = 0;
  int iy = 0;
  int cols = (int)(w / step);
  int rows = (int)(h / step);
  for (ix = 0; ix <= cols; ++ix) {
    float lx = x + ix * step;
    float alpha = (ix % 4 == 0) ? 0.10f : 0.05f;
    stygian_line(ctx, lx, y, lx, y + h, 1.0f, 0.42f, 0.47f, 0.54f, alpha);
  }
  for (iy = 0; iy <= rows; ++iy) {
    float ly = y + iy * step;
    float alpha = (iy % 4 == 0) ? 0.10f : 0.05f;
    stygian_line(ctx, x, ly, x + w, ly, 1.0f, 0.42f, 0.47f, 0.54f, alpha);
  }
}

static void shell_copy_icon_call(StygianContext *ctx,
                                 StygianEditorIconKind kind) {
  char text[256];
  snprintf(text, sizeof(text),
           "stygian_editor_draw_icon(ctx, %s, x, y, w, h, 1.0f, 1.0f, 1.0f, "
           "1.0f);",
           stygian_editor_icon_token(kind));
  stygian_clipboard_push(ctx, text, "stygian-editor-icon-call");
}

static void shell_copy_shortcuts_note(StygianContext *ctx) {
  const char *text =
      "Canvas / Icons switch in title bar\n"
      "Toolbar families: press chevron\n"
      "Right click icon preview: copy icon call";
  stygian_clipboard_push(ctx, text, "stygian-editor-shortcuts");
}

static void shell_execute_menu_action(StygianWindow *window, StygianContext *ctx,
                                      ShellWorkspaceMode *workspace_mode,
                                      ShellIconLabState *icon_lab_state,
                                      ShellToolDockState *dock_state,
                                      bool *show_tool_dock,
                                      ShellCanvasSnapSettings *canvas_snap,
                                      ShellDialogState *dialog_state,
                                      int menu_index,
                                      int item_index) {
  if (!workspace_mode || !icon_lab_state || !dock_state || !show_tool_dock ||
      !canvas_snap || !dialog_state)
    return;
  switch (menu_index) {
  case 0:
    switch (item_index) {
    case 0:
      *workspace_mode = SHELL_WORKSPACE_CANVAS;
      break;
    case 1:
      shell_dialog_open(dialog_state, SHELL_DIALOG_OPEN_PROJECT);
      break;
    case 2:
      shell_dialog_open(dialog_state, SHELL_DIALOG_IMPORT_ASSET);
      break;
    case 3:
      if (window)
        stygian_window_request_close(window);
      break;
    default:
      break;
    }
    break;
  case 1:
    switch (item_index) {
    case 0:
      shell_copy_icon_call(ctx, icon_lab_state->selected);
      icon_lab_state->copied_until_ms = shell_now_ms() + 1800u;
      break;
    case 1:
      memset(dock_state->family_choice, 0, sizeof(dock_state->family_choice));
      dock_state->active_tool = 0;
      dock_state->open_family = -1;
      break;
    case 2:
    default:
      break;
    }
    break;
  case 2:
    switch (item_index) {
    case 0:
      *workspace_mode = SHELL_WORKSPACE_CANVAS;
      break;
    case 1:
      *workspace_mode = SHELL_WORKSPACE_ICON_LAB;
      break;
    case 2:
      *show_tool_dock = !*show_tool_dock;
      if (!*show_tool_dock) {
        dock_state->visible_t = 0.0f;
        dock_state->animating = false;
        dock_state->open_family = -1;
      }
      break;
    case 3:
      canvas_snap->grid_enabled = !canvas_snap->grid_enabled;
      break;
    default:
      break;
    }
    break;
  case 3:
    *workspace_mode = SHELL_WORKSPACE_CANVAS;
    switch (item_index) {
    case 0:
      dock_state->active_tool = 0;
      break;
    case 1:
      dock_state->active_tool = 1;
      break;
    case 2:
      dock_state->active_tool = 3;
      break;
    case 3:
      dock_state->active_tool = 4;
      break;
    default:
      break;
    }
    break;
  case 4:
    switch (item_index) {
    case 0:
      *workspace_mode = SHELL_WORKSPACE_CANVAS;
      break;
    case 1:
      *workspace_mode = SHELL_WORKSPACE_ICON_LAB;
      break;
    case 2:
      shell_copy_shortcuts_note(ctx);
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
}

static void shell_draw_icon_lab_workspace(StygianContext *ctx, StygianFont font,
                                          const ShellLayout *layout,
                                          const ShellFrameState *frame,
                                          ShellIconLabState *state,
                                          uint64_t now_ms) {
  float outer_x = layout->canvas_x + 14.0f;
  float outer_y = layout->canvas_y + 14.0f;
  float outer_w = layout->canvas_w - 28.0f;
  float outer_h = layout->canvas_h - 28.0f;
  float sidebar_w = 230.0f;
  float info_w = 320.0f;
  float stage_x = outer_x + sidebar_w + 16.0f;
  float info_x = outer_x + outer_w - info_w;
  float stage_w = info_x - 16.0f - stage_x;
  float preview_x = stage_x + 18.0f;
  float preview_y = outer_y + 74.0f;
  float preview_w = stage_w - 36.0f;
  float preview_h = outer_h - 148.0f;
  float copy_x = info_x + 18.0f;
  float copy_y = outer_y + 258.0f;
  float copy_w = info_w - 36.0f;
  float copy_h = 34.0f;
  int i = 0;

  stygian_rect_rounded(ctx, outer_x, outer_y, sidebar_w, outer_h, 0.10f, 0.10f,
                       0.11f, 1.0f, 18.0f);
  stygian_rect_rounded(ctx, stage_x, outer_y, stage_w, outer_h, 0.10f, 0.10f,
                       0.11f, 1.0f, 18.0f);
  stygian_rect_rounded(ctx, info_x, outer_y, info_w, outer_h, 0.10f, 0.10f,
                       0.11f, 1.0f, 18.0f);

  stygian_text(ctx, font, "Icon Lab", outer_x + 18.0f, outer_y + 18.0f, 20.0f,
               0.95f, 0.95f, 0.96f, 1.0f);
  stygian_text(ctx, font, "Shared SDF icon workspace", outer_x + 18.0f,
               outer_y + 44.0f, 13.0f, 0.62f, 0.64f, 0.68f, 1.0f);

  for (i = 0; i < STYGIAN_EDITOR_ICON_COUNT; ++i) {
    float row_x = outer_x + 12.0f;
    float row_y = outer_y + 82.0f + i * 38.0f;
    float row_w = sidebar_w - 24.0f;
    float row_h = 30.0f;
    bool hovered = shell_point_in_rect(frame->mouse_x, frame->mouse_y, row_x,
                                       row_y, row_w, row_h);
    bool active = state->selected == (StygianEditorIconKind)i;
    float bg = active ? 0.11f : 0.08f;
    if (hovered)
      bg += 0.05f;
    stygian_rect_rounded(ctx, row_x, row_y, row_w, row_h, bg, bg, bg + 0.01f,
                         0.96f, 10.0f);
    if (hovered || active) {
      stygian_rect_rounded(ctx, row_x + 1.0f, row_y + 1.0f, row_w - 2.0f,
                           row_h - 2.0f, 0.17f, 0.22f, 0.29f,
                           active ? 0.34f : 0.18f, 9.0f);
    }
    stygian_editor_draw_icon(ctx, (StygianEditorIconKind)i, row_x + 8.0f,
                             row_y + 4.0f, 22.0f, 22.0f, 0.93f, 0.93f, 0.95f,
                             1.0f);
    stygian_text(ctx, font,
                 stygian_editor_icon_name((StygianEditorIconKind)i),
                 row_x + 38.0f, row_y + 6.0f, 13.0f, 0.88f, 0.89f, 0.91f, 1.0f);
    if (hovered && frame->left_pressed)
      state->selected = (StygianEditorIconKind)i;
  }

  stygian_text(ctx, font, "Stage", stage_x + 18.0f, outer_y + 18.0f, 20.0f,
               0.95f, 0.95f, 0.96f, 1.0f);
  stygian_text(ctx, font, "Tune silhouette, spacing, and state read.", stage_x + 18.0f,
               outer_y + 44.0f, 13.0f, 0.62f, 0.64f, 0.68f, 1.0f);
  stygian_rect_rounded(ctx, preview_x, preview_y, preview_w, preview_h, 0.08f,
                       0.08f, 0.09f, 1.0f, 16.0f);
  shell_draw_icon_lab_grid(ctx, preview_x + 14.0f, preview_y + 14.0f,
                           preview_w - 28.0f, preview_h - 28.0f, 20.0f);
  stygian_line(ctx, preview_x + preview_w * 0.5f, preview_y + 20.0f,
               preview_x + preview_w * 0.5f, preview_y + preview_h - 20.0f,
               1.0f, 0.30f, 0.34f, 0.39f, 0.25f);
  stygian_line(ctx, preview_x + 20.0f, preview_y + preview_h * 0.5f,
               preview_x + preview_w - 20.0f, preview_y + preview_h * 0.5f,
               1.0f, 0.30f, 0.34f, 0.39f, 0.25f);

  stygian_editor_draw_icon(ctx, state->selected, preview_x + 120.0f,
                           preview_y + 110.0f, 84.0f, 84.0f, 0.97f, 0.97f,
                           0.99f, 1.0f);
  stygian_text(ctx, font, "Large", preview_x + 126.0f, preview_y + 208.0f,
               12.0f, 0.62f, 0.64f, 0.68f, 1.0f);

  stygian_editor_draw_icon(ctx, state->selected, preview_x + 280.0f,
                           preview_y + 118.0f, 48.0f, 48.0f, 0.94f, 0.94f,
                           0.96f, 1.0f);
  stygian_editor_draw_icon(ctx, state->selected, preview_x + 352.0f,
                           preview_y + 124.0f, 36.0f, 36.0f, 0.94f, 0.94f,
                           0.96f, 1.0f);
  stygian_editor_draw_icon(ctx, state->selected, preview_x + 412.0f,
                           preview_y + 129.0f, 26.0f, 26.0f, 0.94f, 0.94f,
                           0.96f, 1.0f);
  stygian_text(ctx, font, "48", preview_x + 294.0f, preview_y + 181.0f, 12.0f,
               0.62f, 0.64f, 0.68f, 1.0f);
  stygian_text(ctx, font, "36", preview_x + 362.0f, preview_y + 181.0f, 12.0f,
               0.62f, 0.64f, 0.68f, 1.0f);
  stygian_text(ctx, font, "26", preview_x + 419.0f, preview_y + 181.0f, 12.0f,
               0.62f, 0.64f, 0.68f, 1.0f);

  {
    float state_x = preview_x + 120.0f;
    float state_y = preview_y + 280.0f;
    float button_w = 56.0f;
    float button_h = 56.0f;
    const char *labels[3] = {"Idle", "Hover", "Active"};
    for (i = 0; i < 3; ++i) {
      float bx = state_x + i * 88.0f;
      float bg_r = i == 2 ? 0.09f : 0.14f;
      float bg_g = i == 2 ? 0.46f : 0.14f;
      float bg_b = i == 2 ? 0.86f : 0.15f;
      stygian_rect_rounded(ctx, bx, state_y, button_w, button_h, bg_r, bg_g,
                           bg_b, 1.0f, 12.0f);
      stygian_rect_rounded(ctx, bx + 1.0f, state_y + 1.0f, button_w - 2.0f,
                           button_h - 2.0f, bg_r + 0.03f, bg_g + 0.03f,
                           bg_b + 0.03f, i == 1 ? 0.34f : 0.20f, 11.0f);
      if (i > 0) {
        stygian_rect_rounded(ctx, bx - 1.0f, state_y - 1.0f, button_w + 2.0f,
                             button_h + 2.0f, 0.27f, 0.63f, 0.98f,
                             i == 1 ? 0.12f : 0.08f, 13.0f);
      }
      stygian_editor_draw_icon(ctx, state->selected, bx + 15.0f, state_y + 15.0f,
                               26.0f, 26.0f, 0.97f, 0.97f, 0.99f, 1.0f);
      stygian_text(ctx, font, labels[i], bx + 11.0f, state_y + 68.0f, 12.0f,
                   0.64f, 0.66f, 0.69f, 1.0f);
    }
  }

  stygian_text(ctx, font, "Usage", info_x + 18.0f, outer_y + 18.0f, 20.0f,
               0.95f, 0.95f, 0.96f, 1.0f);
  stygian_text(ctx, font, stygian_editor_icon_name(state->selected),
               info_x + 18.0f, outer_y + 56.0f, 16.0f, 0.85f, 0.89f, 0.98f,
               1.0f);
  stygian_text(ctx, font,
               "Same shell, separate workspace. This is where icon tuning belongs.",
               info_x + 18.0f, outer_y + 88.0f, 13.0f, 0.62f, 0.64f, 0.68f,
               1.0f);
  stygian_text(ctx, font,
               "The editor uses shared C icon draw code, not exported assets.",
               info_x + 18.0f, outer_y + 108.0f, 13.0f, 0.62f, 0.64f, 0.68f,
               1.0f);
  stygian_text(ctx, font,
               "Right click the preview or press Copy Call to push a snippet.",
               info_x + 18.0f, outer_y + 128.0f, 13.0f, 0.62f, 0.64f, 0.68f,
               1.0f);

  stygian_rect_rounded(ctx, info_x + 18.0f, outer_y + 160.0f, info_w - 36.0f,
                       78.0f, 0.08f, 0.08f, 0.09f, 1.0f, 12.0f);
  stygian_text(ctx, font, "Call:", info_x + 32.0f, outer_y + 174.0f, 13.0f,
               0.72f, 0.74f, 0.78f, 1.0f);
  stygian_text(ctx, font, "stygian_editor_draw_icon(...)", info_x + 32.0f,
               outer_y + 196.0f, 13.0f, 0.92f, 0.92f, 0.94f, 1.0f);
  stygian_text(ctx, font, "enum: use the selected icon kind", info_x + 32.0f,
               outer_y + 216.0f, 13.0f, 0.62f, 0.64f, 0.68f, 1.0f);

  {
    bool copy_hovered = shell_point_in_rect(frame->mouse_x, frame->mouse_y,
                                            copy_x, copy_y, copy_w, copy_h);
    float bg = copy_hovered ? 0.15f : 0.11f;
    stygian_rect_rounded(ctx, copy_x, copy_y, copy_w, copy_h, bg, bg + 0.02f,
                         bg + 0.04f, 1.0f, 11.0f);
    stygian_text(ctx, font, "Copy Call Snippet", copy_x + 18.0f, copy_y + 8.0f,
                 14.0f, 0.96f, 0.96f, 0.97f, 1.0f);
    if ((copy_hovered && frame->left_pressed) ||
        (frame->right_pressed &&
         shell_point_in_rect(frame->mouse_x, frame->mouse_y, preview_x,
                             preview_y, preview_w, preview_h))) {
      shell_copy_icon_call(ctx, state->selected);
      state->copied_until_ms = now_ms + 1800u;
    }
  }

  if (state->copied_until_ms > now_ms) {
    uint32_t delay = (uint32_t)(state->copied_until_ms - now_ms);
    stygian_rect_rounded(ctx, info_x + 18.0f, outer_y + 310.0f, info_w - 36.0f,
                         30.0f, 0.11f, 0.18f, 0.13f, 1.0f, 10.0f);
    stygian_text(ctx, font, "Copied icon call snippet to clipboard.",
                 info_x + 30.0f, outer_y + 318.0f, 13.0f, 0.82f, 0.93f, 0.84f,
                 1.0f);
    if (delay < 1u)
      delay = 1u;
    stygian_request_repaint_after_ms(ctx, delay);
  }
}

static void shell_update_tool_dock_state(ShellToolDockState *state,
                                         const ShellToolDockLayout *layout,
                                         const ShellFrameState *frame,
                                         StygianWindow *window,
                                         uint64_t now_ms) {
  const uint32_t show_duration_ms = 140u;
  const uint32_t hide_duration_ms = 140u;
  const uint32_t hold_duration_ms = 1400u;
  bool focused = true;
  if (!state || !layout || !frame)
    return;
  if (window) {
    focused = stygian_window_is_focused(window);
  }
  state->pointer_inside = shell_point_in_rect(frame->mouse_x, frame->mouse_y,
                                              layout->x - 48.0f,
                                              layout->visible_y - 156.0f,
                                              layout->w + 96.0f,
                                              layout->h + 190.0f);
  shell_tool_dock_step_anim(state, now_ms);

  if (state->pointer_inside || !focused || state->open_family >= 0) {
    state->hold_until_ms = now_ms + hold_duration_ms;
    if (state->visible_t < 1.0f &&
        (!state->animating || state->anim_to_t < 1.0f)) {
      shell_tool_dock_begin_anim(state, 1.0f, now_ms, show_duration_ms);
      shell_tool_dock_step_anim(state, now_ms);
    }
    return;
  }

  if (state->hold_until_ms == 0u)
    state->hold_until_ms = now_ms + hold_duration_ms;

  if (now_ms >= state->hold_until_ms && state->visible_t > 0.0f &&
      (!state->animating || state->anim_to_t > 0.0f)) {
    shell_tool_dock_begin_anim(state, 0.0f, now_ms, hide_duration_ms);
    shell_tool_dock_step_anim(state, now_ms);
  }
}

static void shell_handle_event(StygianWindow *window, StygianContext *ctx,
                               const ShellLayout *layout, int *width,
                               int *height, StygianEvent *event,
                               ShellFrameState *state) {
  StygianWidgetEventImpact impact;
  ShellCaptionAction action;
  bool in_title = false;
  bool in_controls = false;
  bool in_workspace = false;

  if (!window || !ctx || !layout || !width || !height || !event || !state)
    return;

  impact = stygian_widgets_process_event_ex(ctx, event);
  if (impact & STYGIAN_IMPACT_MUTATED_STATE)
    state->event_mutated = true;
  if (impact & STYGIAN_IMPACT_REQUEST_REPAINT)
    state->event_requested = true;
  if (impact & STYGIAN_IMPACT_REQUEST_EVAL)
    state->event_eval_requested = true;

  if (event->type == STYGIAN_EVENT_MOUSE_MOVE) {
    state->mouse_x = (float)event->mouse_move.x;
    state->mouse_y = (float)event->mouse_move.y;
    state->event_requested = true;
  } else if (event->type == STYGIAN_EVENT_MOUSE_DOWN ||
             event->type == STYGIAN_EVENT_MOUSE_UP) {
    state->mouse_x = (float)event->mouse_button.x;
    state->mouse_y = (float)event->mouse_button.y;
    state->event_requested = true;
  } else if (event->type == STYGIAN_EVENT_SCROLL) {
    state->mouse_x = (float)event->scroll.x;
    state->mouse_y = (float)event->scroll.y;
    state->scroll_dy += event->scroll.dy;
    state->event_requested = true;
  }

  if (event->type == STYGIAN_EVENT_CLOSE) {
    stygian_window_request_close(window);
    state->event_requested = true;
    return;
  }

  if (event->type == STYGIAN_EVENT_RESIZE) {
    *width = event->resize.width;
    *height = event->resize.height;
    state->event_mutated = true;
    state->event_requested = true;
    return;
  }

  if (event->type == STYGIAN_EVENT_MOUSE_DOWN &&
      event->mouse_button.button == STYGIAN_MOUSE_LEFT) {
    state->left_down = true;
    state->left_pressed = true;
    state->left_clicks = event->mouse_button.clicks;
  } else if (event->type == STYGIAN_EVENT_MOUSE_DOWN &&
             event->mouse_button.button == STYGIAN_MOUSE_MIDDLE) {
    state->middle_down = true;
    state->middle_pressed = true;
  } else if (event->type == STYGIAN_EVENT_MOUSE_DOWN &&
             event->mouse_button.button == STYGIAN_MOUSE_RIGHT) {
    state->right_pressed = true;
  } else if (event->type == STYGIAN_EVENT_MOUSE_UP &&
             event->mouse_button.button == STYGIAN_MOUSE_LEFT) {
    state->left_down = false;
    state->left_released = true;
  } else if (event->type == STYGIAN_EVENT_MOUSE_UP &&
             event->mouse_button.button == STYGIAN_MOUSE_MIDDLE) {
    state->middle_down = false;
    state->middle_released = true;
  }

  if (event->type == STYGIAN_EVENT_KEY_DOWN &&
      event->key.key == STYGIAN_KEY_ENTER) {
    state->enter_pressed = true;
    state->event_requested = true;
  } else if (event->type == STYGIAN_EVENT_KEY_DOWN &&
             event->key.key == STYGIAN_KEY_ESCAPE) {
    state->esc_pressed = true;
    state->event_requested = true;
  } else if (event->type == STYGIAN_EVENT_KEY_DOWN &&
             event->key.key == STYGIAN_KEY_UP) {
    state->up_pressed = true;
    state->event_requested = true;
  } else if (event->type == STYGIAN_EVENT_KEY_DOWN &&
             event->key.key == STYGIAN_KEY_DOWN) {
    state->down_pressed = true;
    state->event_requested = true;
  } else if (event->type == STYGIAN_EVENT_KEY_DOWN &&
             event->key.key == STYGIAN_KEY_LEFT) {
    state->left_arrow_pressed = true;
    state->event_requested = true;
  } else if (event->type == STYGIAN_EVENT_KEY_DOWN &&
             event->key.key == STYGIAN_KEY_RIGHT) {
    state->right_arrow_pressed = true;
    state->event_requested = true;
  } else if (event->type == STYGIAN_EVENT_KEY_DOWN &&
             event->key.key == STYGIAN_KEY_DELETE) {
    state->delete_pressed = true;
    state->event_requested = true;
  } else if (event->type == STYGIAN_EVENT_KEY_DOWN &&
             event->key.key == STYGIAN_KEY_BACKSPACE) {
    state->backspace_pressed = true;
    state->event_requested = true;
  } else if (event->type == STYGIAN_EVENT_KEY_DOWN &&
             event->key.key == STYGIAN_KEY_D) {
    state->d_pressed = true;
    state->event_requested = true;
  } else if (event->type == STYGIAN_EVENT_KEY_DOWN &&
             event->key.key == STYGIAN_KEY_C) {
    state->c_pressed = true;
    state->event_requested = true;
  } else if (event->type == STYGIAN_EVENT_KEY_DOWN &&
             event->key.key == STYGIAN_KEY_X) {
    state->x_pressed = true;
    state->event_requested = true;
  } else if (event->type == STYGIAN_EVENT_KEY_DOWN &&
             event->key.key == STYGIAN_KEY_V) {
    state->v_pressed = true;
    state->event_requested = true;
  } else if (event->type == STYGIAN_EVENT_KEY_DOWN &&
             event->key.key == STYGIAN_KEY_Z) {
    state->z_pressed = true;
    state->event_requested = true;
  } else if (event->type == STYGIAN_EVENT_KEY_DOWN &&
             event->key.key == STYGIAN_KEY_Y) {
    state->y_pressed = true;
    state->event_requested = true;
  }

  if (event->type != STYGIAN_EVENT_MOUSE_DOWN ||
      event->mouse_button.button != STYGIAN_MOUSE_LEFT) {
    return;
  }

  action = shell_hit_caption_button(layout, state->mouse_x, state->mouse_y);
  in_title = event->mouse_button.y >= 0 &&
             event->mouse_button.y < (int)(layout->shell_y + layout->title_h);
  in_title = in_title && event->mouse_button.x >= (int)layout->shell_x &&
             event->mouse_button.x < (int)(layout->shell_x + layout->shell_w) &&
             event->mouse_button.y >= (int)layout->shell_y;
  in_controls =
      event->mouse_button.y >= (int)layout->button_y &&
      event->mouse_button.y < (int)(layout->button_y + layout->button_h) &&
      event->mouse_button.x >= (int)(layout->min_x - layout->button_gap) &&
      event->mouse_button.x <
          (int)(layout->close_x + layout->button_w + layout->button_gap);
  in_controls = in_controls ||
                shell_point_in_rect(state->mouse_x, state->mouse_y,
                                    layout->menu_x, layout->menu_y,
                                    layout->menu_w, layout->menu_h);
  in_workspace = shell_point_in_rect(state->mouse_x, state->mouse_y,
                                     layout->workspace_x, layout->workspace_y,
                                     layout->workspace_w, layout->workspace_h);
  in_controls = in_controls || in_workspace;

  if (action == SHELL_CAPTION_MINIMIZE) {
    stygian_window_minimize(window);
    state->event_mutated = true;
    state->event_requested = true;
  } else if (action == SHELL_CAPTION_MAXIMIZE) {
    if (stygian_window_is_maximized(window))
      stygian_window_restore(window);
    else
      stygian_window_maximize(window);
    state->event_mutated = true;
    state->event_requested = true;
  } else if (action == SHELL_CAPTION_CLOSE) {
    stygian_window_request_close(window);
    state->event_mutated = true;
    state->event_requested = true;
  } else if (in_title && !in_controls) {
    if (event->mouse_button.clicks >= 2)
      stygian_window_titlebar_double_click(window);
    else
      stygian_window_begin_system_move(window);
    state->event_mutated = true;
    state->event_requested = true;
  }
}

int main(void) {
  StygianWindowConfig win_cfg = {
      .width = 1440,
      .height = 900,
      .title = "Stygian Editor Shell",
      .flags = STYGIAN_WINDOW_RESIZABLE | STYGIAN_WINDOW_BORDERLESS |
               STYGIAN_EDITOR_SHELL_RENDER_FLAG,
  };
  StygianTitlebarBehavior titlebar_behavior = {
      .double_click_mode = STYGIAN_TITLEBAR_DBLCLICK_MAXIMIZE_RESTORE,
      .hover_menu_enabled = false,
  };
  StygianWindow *window = NULL;
  StygianContext *ctx = NULL;
  StygianConfig cfg = {0};
  StygianFont font = 0u;
  ShellWorkspaceMode workspace_mode = SHELL_WORKSPACE_CANVAS;
  ShellIconLabState icon_lab_state = {
      .selected = STYGIAN_EDITOR_ICON_SELECT,
      .copied_until_ms = 0u,
  };
  ShellMenuState menu_state = {
      .open_menu = -1,
      .hot_menu = -1,
      .hot_item = -1,
  };
  StygianContextMenu canvas_context_menu = {
      .open = false,
      .x = 0.0f,
      .y = 0.0f,
      .w = 208.0f,
      .item_h = 28.0f,
  };
  // This app keeps history and browser state around for the whole session.
  // Putting that pile on the stack was a dumb way to lose a startup fight.
  static ShellDialogState dialog_state = {
      .open = false,
      .kind = SHELL_DIALOG_NONE,
      .suggested_path = {0},
  };
  static ShellCommitState commit_state = {0};
  static ShellAssetLibrary asset_library = {0};
  static ShellCanvasImportState canvas_import_state = {0};
  static ShellCanvasSceneState canvas_scene = {0};
  static ShellMotionState motion_state = {
      .active_tab = SHELL_PANEL_DESIGN,
      .selected_clip_id = -1,
      .selected_track_id = -1,
      .hovered_node_id = -1,
      .pressed_node_id = -1,
      .drag_kind = SHELL_TIMELINE_DRAG_NONE,
      .drag_track_id = -1,
      .last_clip_id = -1,
      .last_track_id = -1,
      .active_field = SHELL_MOTION_FIELD_NONE,
  };
  static ShellCanvasHistoryState canvas_history = {0};
  static ShellCanvasClipboardState canvas_clipboard = {0};
  static ShellWorkspacePanelState workspace_panel = {
      .active_tab = SHELL_WORKSPACE_TAB_LAYERS,
      .active_layer_id = -1,
      .editing_name_id = -1,
      .editing_page_name = false,
      .name_buf = {0},
  };
  ShellCanvasInteractionState canvas_interaction = {
      .mode = SHELL_CANVAS_INTERACT_NONE,
      .target_frame = -1,
      .target_point = -1,
  };
  ShellPathDraftState path_draft = {0};
  ShellCanvasViewState canvas_view = {
      .pan_x = 32.0f,
      .pan_y = 24.0f,
      .zoom = 1.0f,
      .panning = false,
  };
  ShellCanvasSnapSettings canvas_snap = {
      .guide_enabled = true,
      .grid_enabled = false,
      .grid_size = 8.0f,
  };
  ShellCanvasGuideState canvas_guides = {0};
  ShellInspectorState inspector_state = {
      .last_selected_id = -1,
      .last_selection_count = 0,
      .initialized = false,
      .editing = false,
      .pending_sync = false,
      .active_field = SHELL_INSPECTOR_NONE,
  };
  ShellSidebarState sidebar_state = {
      .left_panel_width = 286.0f,
      .left_drag_offset_x = 0.0f,
      .right_panel_width = 12.0f,
      .drag_offset_x = 0.0f,
      .dragging_left_panel = false,
      .dragging_right_panel = false,
  };
  bool show_tool_dock = true;
  ShellToolDockState dock_state = {.active_tool = 0,
                                   .open_family = -1,
                                   .visible_t = 0.0f,
                                   .anim_from_t = 0.0f,
                                   .anim_to_t = 0.0f,
                                   .anim_started_ms = 0u,
                                   .anim_duration_ms = 0u,
                                   .hold_until_ms = 0u,
                                   .animating = false,
                                   .hot_index = -1,
                                   .pressed_index = -1,
                                   .popup_hot_index = -1,
                                   .popup_pressed_index = -1};
  bool first_frame = true;
  bool has_repo_paths = false;
  char shader_dir[1024];
  char atlas_png[1024];
  char atlas_json[1024];
  char repo_root[1024];

  window = stygian_window_create(&win_cfg);
  if (!window)
    return 1;
  stygian_window_set_titlebar_behavior(window, &titlebar_behavior);

  has_repo_paths = shell_resolve_repo_resource_paths(
      shader_dir, sizeof(shader_dir), atlas_png, sizeof(atlas_png), atlas_json,
      sizeof(atlas_json));
#ifdef _WIN32
  if (has_repo_paths) {
    strncpy(repo_root, shader_dir, sizeof(repo_root) - 1u);
    repo_root[sizeof(repo_root) - 1u] = '\0';
    if (!shell_path_up_one(repo_root)) {
      repo_root[0] = '\0';
    } else {
      SetCurrentDirectoryA(repo_root);
    }
  }
#endif
  cfg.backend = STYGIAN_EDITOR_SHELL_BACKEND;
  cfg.window = window;
  if (has_repo_paths) {
    cfg.shader_dir = shader_dir;
    cfg.default_font_atlas_png = atlas_png;
    cfg.default_font_atlas_json = atlas_json;
  }
  ctx = stygian_create(&cfg);
  if (!ctx) {
    stygian_window_destroy(window);
    return 1;
  }

  font = stygian_get_default_font(ctx);
  shell_canvas_scene_init(&canvas_scene);
  shell_canvas_history_init(&canvas_history, &canvas_scene);

  while (!stygian_window_should_close(window)) {
    StygianEvent event;
    StygianTitlebarHints titlebar_hints;
    ShellLayout layout;
    ShellToolDockLayout tool_dock;
    ShellFrameState frame = {0};
    int width = 0;
    int height = 0;
    uint64_t now_ms = shell_now_ms();
    float prev_visible_t = dock_state.visible_t;
    uint32_t deferred_repaint_ms = 0u;
    uint32_t wait_ms = stygian_next_repaint_wait_ms(ctx, 250u);

    stygian_window_get_titlebar_hints(window, &titlebar_hints);
    stygian_window_get_size(window, &width, &height);
    shell_compute_layout(&layout, width, height, &titlebar_hints,
                         sidebar_state.left_panel_width,
                         sidebar_state.right_panel_width);
    shell_compute_tool_dock_layout(&tool_dock, &layout);
    stygian_widgets_begin_frame(ctx);

    while (stygian_window_poll_event(window, &event)) {
      shell_handle_event(window, ctx, &layout, &width, &height, &event, &frame);
      shell_compute_layout(&layout, width, height, &titlebar_hints,
                           sidebar_state.left_panel_width,
                           sidebar_state.right_panel_width);
      shell_compute_tool_dock_layout(&tool_dock, &layout);
    }

    if (!first_frame && !frame.event_mutated && !frame.event_requested &&
        !stygian_has_pending_repaint(ctx) && !frame.event_eval_requested) {
      if (!stygian_window_wait_event_timeout(window, &event, wait_ms))
        continue;
      shell_handle_event(window, ctx, &layout, &width, &height, &event, &frame);
      shell_compute_layout(&layout, width, height, &titlebar_hints,
                           sidebar_state.left_panel_width,
                           sidebar_state.right_panel_width);
      shell_compute_tool_dock_layout(&tool_dock, &layout);
      if (!frame.event_mutated && !frame.event_requested &&
          !frame.event_eval_requested && !stygian_has_pending_repaint(ctx)) {
        continue;
      }
    }

    first_frame = false;
    shell_query_live_mouse(window, &frame.mouse_x, &frame.mouse_y);
    frame.left_down = stygian_mouse_down(window, STYGIAN_MOUSE_LEFT);
    frame.middle_down = stygian_mouse_down(window, STYGIAN_MOUSE_MIDDLE);
    frame.ctrl_down = stygian_key_down(window, STYGIAN_KEY_CTRL);
    frame.shift_down = stygian_key_down(window, STYGIAN_KEY_SHIFT);
    shell_canvas_guides_clear(&canvas_guides);
    bool canvas_overlay_open =
        dialog_state.open || menu_state.open_menu >= 0 || canvas_context_menu.open;
    bool inspector_force_sync = inspector_state.pending_sync;
    inspector_state.pending_sync = false;

    if (!canvas_overlay_open && workspace_mode == SHELL_WORKSPACE_CANVAS) {
      if (frame.ctrl_down && frame.z_pressed && frame.shift_down) {
        if (shell_canvas_history_redo(ctx, &canvas_history, &canvas_scene)) {
          inspector_force_sync = true;
          frame.event_requested = true;
        }
      } else if (frame.ctrl_down && frame.z_pressed) {
        if (shell_canvas_history_undo(ctx, &canvas_history, &canvas_scene)) {
          inspector_force_sync = true;
          frame.event_requested = true;
        }
      } else if (frame.ctrl_down && frame.y_pressed) {
        if (shell_canvas_history_redo(ctx, &canvas_history, &canvas_scene)) {
          inspector_force_sync = true;
          frame.event_requested = true;
        }
      }
      if (frame.ctrl_down && frame.c_pressed) {
        int copied = shell_canvas_clipboard_copy_selected(&canvas_clipboard,
                                                          &canvas_scene);
        if (copied > 0) {
          char clip_text[64];
          snprintf(clip_text, sizeof(clip_text), "Stygian Editor nodes (%d)",
                   copied);
          stygian_clipboard_push(ctx, clip_text, "stygian-editor-scene-nodes");
          frame.event_requested = true;
        }
      }
      if (frame.ctrl_down && frame.x_pressed) {
        int copied = shell_canvas_clipboard_copy_selected(&canvas_clipboard,
                                                          &canvas_scene);
        if (copied > 0) {
          char clip_text[64];
          snprintf(clip_text, sizeof(clip_text), "Stygian Editor nodes (%d)",
                   copied);
          stygian_clipboard_push(ctx, clip_text, "stygian-editor-scene-nodes");
          if (shell_canvas_scene_delete_selected(ctx, &canvas_scene)) {
            inspector_force_sync = true;
            frame.event_requested = true;
          }
        }
      }
      if (frame.ctrl_down && frame.v_pressed) {
        if (shell_canvas_clipboard_paste(&canvas_scene, &canvas_clipboard)) {
          inspector_force_sync = true;
          frame.event_requested = true;
        }
      }
      if (frame.delete_pressed || frame.backspace_pressed) {
        ShellCanvasFrameNode *selected =
            shell_canvas_scene_selected_frame(&canvas_scene);
        if (selected && selected->kind == SHELL_NODE_PATH &&
            selected->selected_point >= 0) {
          if (shell_canvas_path_delete_selected_point(selected)) {
            inspector_force_sync = true;
            frame.event_requested = true;
          }
        } else if (shell_canvas_scene_delete_selected(ctx, &canvas_scene)) {
          frame.event_requested = true;
        }
      }
      if (frame.ctrl_down && frame.d_pressed) {
        if (shell_canvas_scene_duplicate_selected(&canvas_scene))
          frame.event_requested = true;
      }
      if (shell_tool_is_select_mode(&dock_state)) {
        float nudge = frame.shift_down ? 10.0f : 1.0f;
        if (frame.left_arrow_pressed) {
          shell_canvas_scene_nudge_selected(&canvas_scene, -nudge, 0.0f);
          frame.event_requested = true;
          inspector_force_sync = true;
        }
        if (frame.right_arrow_pressed) {
          shell_canvas_scene_nudge_selected(&canvas_scene, nudge, 0.0f);
          frame.event_requested = true;
          inspector_force_sync = true;
        }
        if (frame.up_pressed) {
          shell_canvas_scene_nudge_selected(&canvas_scene, 0.0f, -nudge);
          frame.event_requested = true;
          inspector_force_sync = true;
        }
        if (frame.down_pressed) {
          shell_canvas_scene_nudge_selected(&canvas_scene, 0.0f, nudge);
          frame.event_requested = true;
          inspector_force_sync = true;
        }
      }
    }

    if (!canvas_overlay_open && workspace_mode == SHELL_WORKSPACE_CANVAS &&
        frame.left_pressed &&
        shell_hit_left_panel_handle(&layout, frame.mouse_x, frame.mouse_y)) {
      float left_edge = layout.canvas_x + 18.0f;
      sidebar_state.dragging_left_panel = true;
      sidebar_state.left_drag_offset_x =
          sidebar_state.left_panel_width - (frame.mouse_x - left_edge);
      frame.event_requested = true;
    }
    if (sidebar_state.dragging_left_panel) {
      float left_edge = layout.canvas_x + 18.0f;
      sidebar_state.left_panel_width =
          shell_clampf(frame.mouse_x - left_edge + sidebar_state.left_drag_offset_x,
                       12.0f, 420.0f);
      shell_compute_layout(&layout, width, height, &titlebar_hints,
                           sidebar_state.left_panel_width,
                           sidebar_state.right_panel_width);
      shell_compute_tool_dock_layout(&tool_dock, &layout);
      frame.event_requested = true;
      if (!frame.left_down || frame.left_released) {
        if (sidebar_state.left_panel_width < 20.0f)
          sidebar_state.left_panel_width = 12.0f;
        sidebar_state.dragging_left_panel = false;
        shell_compute_layout(&layout, width, height, &titlebar_hints,
                             sidebar_state.left_panel_width,
                             sidebar_state.right_panel_width);
        shell_compute_tool_dock_layout(&tool_dock, &layout);
      }
    }

    if (!canvas_overlay_open && workspace_mode == SHELL_WORKSPACE_CANVAS &&
        frame.left_pressed &&
        shell_hit_right_panel_handle(&layout, frame.mouse_x, frame.mouse_y)) {
      float right_edge = layout.canvas_x + layout.canvas_w - 18.0f;
      sidebar_state.dragging_right_panel = true;
      sidebar_state.drag_offset_x =
          sidebar_state.right_panel_width - (right_edge - frame.mouse_x);
      frame.event_requested = true;
    }
    if (sidebar_state.dragging_right_panel) {
      float right_edge = layout.canvas_x + layout.canvas_w - 18.0f;
      sidebar_state.right_panel_width =
          shell_clampf(right_edge - frame.mouse_x + sidebar_state.drag_offset_x,
                       12.0f, 420.0f);
      shell_compute_layout(&layout, width, height, &titlebar_hints,
                           sidebar_state.left_panel_width,
                           sidebar_state.right_panel_width);
      shell_compute_tool_dock_layout(&tool_dock, &layout);
      frame.event_requested = true;
      if (!frame.left_down || frame.left_released) {
        if (sidebar_state.right_panel_width < 20.0f)
          sidebar_state.right_panel_width = 12.0f;
        sidebar_state.dragging_right_panel = false;
        shell_compute_layout(&layout, width, height, &titlebar_hints,
                             sidebar_state.left_panel_width,
                             sidebar_state.right_panel_width);
        shell_compute_tool_dock_layout(&tool_dock, &layout);
      }
    }

    if (!canvas_overlay_open && workspace_mode == SHELL_WORKSPACE_CANVAS &&
        !sidebar_state.dragging_left_panel &&
        !sidebar_state.dragging_right_panel) {
      float stage_x = 0.0f;
      float stage_y = 0.0f;
      float stage_w = 0.0f;
      float stage_h = 0.0f;
      float world_x = 0.0f;
      float world_y = 0.0f;
      bool in_stage = false;
      shell_canvas_stage_rect(&layout, sidebar_state.left_panel_width,
                              sidebar_state.right_panel_width, &stage_x,
                              &stage_y, &stage_w, &stage_h);
      in_stage = shell_point_in_rect(frame.mouse_x, frame.mouse_y, stage_x,
                                     stage_y, stage_w, stage_h);
      shell_canvas_screen_to_world(&layout, sidebar_state.left_panel_width,
                                   sidebar_state.right_panel_width, &canvas_view,
                                   frame.mouse_x, frame.mouse_y, &world_x,
                                   &world_y);
      shell_motion_tick(ctx, &motion_state, &canvas_scene, now_ms);

      if (!canvas_overlay_open && workspace_mode == SHELL_WORKSPACE_CANVAS &&
          !dialog_state.open && canvas_interaction.mode == SHELL_CANVAS_INTERACT_NONE) {
        int hover_hit = -1;
        if (in_stage) {
          hover_hit = shell_canvas_scene_hit_frame(
              &canvas_scene, &layout, sidebar_state.left_panel_width,
              sidebar_state.right_panel_width, &canvas_view, frame.mouse_x,
              frame.mouse_y);
        }
        if (hover_hit >= 0)
          hover_hit = canvas_scene.frames[hover_hit].id;
        if (hover_hit != motion_state.hovered_node_id) {
          if (motion_state.hovered_node_id > 0) {
            shell_motion_trigger_event(&motion_state, &canvas_scene,
                                       motion_state.hovered_node_id,
                                       SHELL_MOTION_EVENT_HOVER_LEAVE, now_ms);
          }
          motion_state.hovered_node_id = hover_hit;
          if (motion_state.hovered_node_id > 0) {
            shell_motion_trigger_event(&motion_state, &canvas_scene,
                                       motion_state.hovered_node_id,
                                       SHELL_MOTION_EVENT_HOVER_ENTER, now_ms);
          }
          if (hover_hit >= 0)
            frame.event_requested = true;
        }
      }

      if (stygian_context_menu_trigger_region(ctx, &canvas_context_menu, stage_x,
                                              stage_y, stage_w, stage_h)) {
        dock_state.open_family = -1;
        menu_state.open_menu = -1;
        frame.event_requested = true;
      }

      if (!canvas_overlay_open && frame.left_pressed && in_stage) {
        int press_hit = shell_canvas_scene_hit_frame(
            &canvas_scene, &layout, sidebar_state.left_panel_width,
            sidebar_state.right_panel_width, &canvas_view, frame.mouse_x,
            frame.mouse_y);
        if (press_hit >= 0) {
          motion_state.pressed_node_id = canvas_scene.frames[press_hit].id;
          shell_motion_trigger_event(&motion_state, &canvas_scene,
                                     motion_state.pressed_node_id,
                                     SHELL_MOTION_EVENT_PRESS, now_ms);
          frame.event_requested = true;
        } else {
          motion_state.pressed_node_id = -1;
        }
      } else if (frame.left_released && motion_state.pressed_node_id > 0) {
        shell_motion_trigger_event(&motion_state, &canvas_scene,
                                   motion_state.pressed_node_id,
                                   SHELL_MOTION_EVENT_RELEASE, now_ms);
        motion_state.pressed_node_id = -1;
        frame.event_requested = true;
      }

      if (in_stage && frame.scroll_dy != 0.0f) {
        float old_zoom = canvas_view.zoom;
        float new_zoom = shell_canvas_zoom_clamp(
            old_zoom * (frame.scroll_dy > 0.0f ? 1.10f : 0.90f));
        if (new_zoom != old_zoom) {
          float before_x = world_x;
          float before_y = world_y;
          canvas_view.zoom = new_zoom;
          canvas_view.pan_x = frame.mouse_x - stage_x - before_x * new_zoom;
          canvas_view.pan_y = frame.mouse_y - stage_y - before_y * new_zoom;
          frame.event_requested = true;
        }
      }

      if (frame.middle_pressed && in_stage) {
        canvas_view.panning = true;
        canvas_view.pan_start_mouse_x = frame.mouse_x;
        canvas_view.pan_start_mouse_y = frame.mouse_y;
        canvas_view.pan_origin_x = canvas_view.pan_x;
        canvas_view.pan_origin_y = canvas_view.pan_y;
      }
      if (canvas_view.panning) {
        if (frame.middle_down) {
          canvas_view.pan_x =
              canvas_view.pan_origin_x + (frame.mouse_x - canvas_view.pan_start_mouse_x);
          canvas_view.pan_y =
              canvas_view.pan_origin_y + (frame.mouse_y - canvas_view.pan_start_mouse_y);
          frame.event_requested = true;
        } else {
          canvas_view.panning = false;
        }
      }

      if (path_draft.active && frame.esc_pressed) {
        memset(&path_draft, 0, sizeof(path_draft));
        frame.event_requested = true;
      } else if (path_draft.active && frame.enter_pressed &&
                 path_draft.point_count >= 2) {
        if (shell_canvas_scene_create_path(&canvas_scene, path_draft.points_x,
                                           path_draft.points_y,
                                           path_draft.point_count, false)) {
          inspector_state.pending_sync = true;
          dock_state.active_tool = 0;
        }
        memset(&path_draft, 0, sizeof(path_draft));
        frame.event_requested = true;
      }
      if (frame.esc_pressed && shell_motion_preview_active(&motion_state) &&
          !path_draft.active) {
        shell_motion_reset_preview(ctx, &motion_state, &canvas_scene);
        frame.event_requested = true;
      }

      if (shell_tool_is_frame_mode(&dock_state) ||
          shell_tool_is_primitive_mode(&dock_state) ||
          shell_tool_is_line_mode(&dock_state) ||
          shell_tool_is_text_mode(&dock_state)) {
        if (frame.left_pressed && in_stage) {
          int preset_w = 1440;
          int preset_h = 900;
          canvas_interaction.mode = SHELL_CANVAS_INTERACT_CREATE_FRAME;
          canvas_interaction.target_frame = -1;
          canvas_interaction.group_active = false;
          canvas_interaction.anchor_world_x = world_x;
          canvas_interaction.anchor_world_y = world_y;
          canvas_interaction.origin_world_x = world_x;
          canvas_interaction.origin_world_y = world_y;
          canvas_interaction.create_radius = 18.0f;
          if (shell_tool_is_frame_mode(&dock_state)) {
            canvas_interaction.create_kind = SHELL_NODE_FRAME;
            shell_frame_family_preset(dock_state.family_choice[1], &preset_w,
                                      &preset_h);
            canvas_interaction.origin_w = (float)preset_w;
            canvas_interaction.origin_h = (float)preset_h;
          } else if (shell_tool_is_text_mode(&dock_state)) {
            canvas_interaction.create_kind = SHELL_NODE_TEXT;
            canvas_interaction.create_radius = 0.0f;
            canvas_interaction.origin_w = 220.0f;
            canvas_interaction.origin_h = 48.0f;
          } else if (shell_tool_is_line_mode(&dock_state)) {
            canvas_interaction.create_kind = SHELL_NODE_LINE;
            canvas_interaction.create_radius = 0.0f;
            canvas_interaction.origin_w = 180.0f;
            canvas_interaction.origin_h = 2.0f;
          } else {
            int choice = dock_state.family_choice[5];
            canvas_interaction.create_kind =
                choice == 1   ? SHELL_NODE_ELLIPSE
                : choice == 2 ? SHELL_NODE_RECTANGLE
                : choice == 3 ? SHELL_NODE_ARROW
                : choice == 4 ? SHELL_NODE_POLYGON
                              : choice == 5 ? SHELL_NODE_STAR
                              : choice == 6 ? SHELL_NODE_IMAGE
                                            : SHELL_NODE_RECTANGLE;
            canvas_interaction.create_radius = choice == 2 ? 34.0f : 18.0f;
            canvas_interaction.origin_w =
                choice == 3 ? 180.0f
                : choice == 6 ? 220.0f
                              : (choice >= 4 ? 120.0f : 180.0f);
            canvas_interaction.origin_h =
                choice == 3 ? 48.0f
                : choice == 6 ? 160.0f
                              : 120.0f;
          }
          frame.event_requested = true;
        }
        if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_CREATE_FRAME) {
          if (frame.left_down) {
            float preview_x = world_x;
            float preview_y = world_y;
            float preview_min_x = 0.0f;
            float preview_min_y = 0.0f;
            float preview_w = 0.0f;
            float preview_h = 0.0f;
            bool snap_left = false;
            bool snap_right = false;
            bool snap_top = false;
            bool snap_bottom = false;
            preview_min_x = canvas_interaction.anchor_world_x < preview_x
                                ? canvas_interaction.anchor_world_x
                                : preview_x;
            preview_min_y = canvas_interaction.anchor_world_y < preview_y
                                ? canvas_interaction.anchor_world_y
                                : preview_y;
            preview_w = fabsf(preview_x - canvas_interaction.anchor_world_x);
            preview_h = fabsf(preview_y - canvas_interaction.anchor_world_y);
            snap_left = preview_x < canvas_interaction.anchor_world_x;
            snap_right = !snap_left;
            snap_top = preview_y < canvas_interaction.anchor_world_y;
            snap_bottom = !snap_top;
            shell_canvas_snap_resize_rect(
                &canvas_scene, &layout, sidebar_state.left_panel_width,
                sidebar_state.right_panel_width, &canvas_view, &canvas_snap, -1,
                snap_left, snap_right, snap_top, snap_bottom, &preview_min_x,
                &preview_min_y, &preview_w, &preview_h, &canvas_guides);
            if (snap_left)
              canvas_interaction.origin_world_x = preview_min_x;
            else
              canvas_interaction.origin_world_x = preview_min_x + preview_w;
            if (snap_top)
              canvas_interaction.origin_world_y = preview_min_y;
            else
              canvas_interaction.origin_world_y = preview_min_y + preview_h;
            frame.event_requested = true;
          } else {
            float drag_threshold =
                6.0f / (canvas_view.zoom > 0.0f ? canvas_view.zoom : 1.0f);
            float create_start_x = canvas_interaction.anchor_world_x;
            float create_start_y = canvas_interaction.anchor_world_y;
            float create_end_x = canvas_interaction.origin_world_x;
            float create_end_y = canvas_interaction.origin_world_y;
            if (fabsf(create_end_x - create_start_x) < drag_threshold &&
                fabsf(create_end_y - create_start_y) < drag_threshold) {
              float preset_w = canvas_interaction.origin_w;
              float preset_h = canvas_interaction.origin_h;
              create_start_x = canvas_interaction.anchor_world_x - preset_w * 0.5f;
              create_start_y = canvas_interaction.anchor_world_y - preset_h * 0.5f;
              create_end_x = create_start_x + preset_w;
              create_end_y = create_start_y + preset_h;
            }
            if ((canvas_interaction.create_kind == SHELL_NODE_FRAME &&
                 shell_canvas_scene_create_frame(
                     &canvas_scene, &layout, sidebar_state.left_panel_width,
                     sidebar_state.right_panel_width, &canvas_view,
                     create_start_x, create_start_y, create_end_x, create_end_y,
                     (int)canvas_interaction.origin_w,
                     (int)canvas_interaction.origin_h)) ||
                (canvas_interaction.create_kind == SHELL_NODE_IMAGE &&
                 canvas_import_state.path[0] &&
                 shell_canvas_scene_create_image(
                     ctx, &canvas_scene, &layout, sidebar_state.left_panel_width,
                     sidebar_state.right_panel_width, &canvas_view,
                     create_start_x, create_start_y, create_end_x, create_end_y,
                     canvas_import_state.path)) ||
                (canvas_interaction.create_kind != SHELL_NODE_FRAME &&
                 canvas_interaction.create_kind != SHELL_NODE_IMAGE &&
                 shell_canvas_scene_create_primitive(
                     &canvas_scene, &layout, sidebar_state.left_panel_width,
                     sidebar_state.right_panel_width, &canvas_view,
                     create_start_x, create_start_y, create_end_x, create_end_y,
                     canvas_interaction.create_kind,
                     canvas_interaction.create_radius))) {
              dock_state.active_tool = 0;
              inspector_state.pending_sync = true;
              frame.event_requested = true;
            }
            canvas_interaction.mode = SHELL_CANVAS_INTERACT_NONE;
            canvas_interaction.target_frame = -1;
          }
        }
      } else if (shell_tool_is_path_mode(&dock_state)) {
        if (frame.left_pressed && in_stage) {
          bool close_path = false;
          if (!path_draft.active) {
            memset(&path_draft, 0, sizeof(path_draft));
            path_draft.active = true;
          } else if (path_draft.point_count >= 3) {
            float first_sx = 0.0f;
            float first_sy = 0.0f;
            shell_canvas_world_to_screen(
                &layout, sidebar_state.left_panel_width,
                sidebar_state.right_panel_width, &canvas_view,
                path_draft.points_x[0], path_draft.points_y[0], &first_sx,
                &first_sy);
            close_path = fabsf(frame.mouse_x - first_sx) <= 8.0f &&
                         fabsf(frame.mouse_y - first_sy) <= 8.0f;
          }
          if (close_path) {
            if (shell_canvas_scene_create_path(&canvas_scene, path_draft.points_x,
                                               path_draft.points_y,
                                               path_draft.point_count, true)) {
              inspector_state.pending_sync = true;
              dock_state.active_tool = 0;
            }
            memset(&path_draft, 0, sizeof(path_draft));
          } else if (path_draft.point_count < SHELL_PATH_POINT_CAP) {
            path_draft.points_x[path_draft.point_count] = world_x;
            path_draft.points_y[path_draft.point_count] = world_y;
            path_draft.point_count++;
          }
          frame.event_requested = true;
        }
      } else if (shell_tool_is_select_mode(&dock_state)) {
        if (frame.left_pressed && in_stage) {
          ShellCanvasFrameNode *selected_path =
              shell_canvas_scene_selected_frame(&canvas_scene);
          int path_point_hit = -1;
          int hit = shell_canvas_scene_hit_frame(
              &canvas_scene, &layout, sidebar_state.left_panel_width,
              sidebar_state.right_panel_width, &canvas_view, frame.mouse_x,
              frame.mouse_y);
          int selection_count = shell_canvas_scene_selection_count(&canvas_scene);
          float group_x = 0.0f;
          float group_y = 0.0f;
          float group_w = 0.0f;
          float group_h = 0.0f;
          float group_sx = 0.0f;
          float group_sy = 0.0f;
          float group_sw = 0.0f;
          float group_sh = 0.0f;
          float handle = 20.0f;
          bool group_bounds_valid =
              selection_count > 0 &&
              shell_canvas_scene_selection_bounds(&canvas_scene, &group_x, &group_y,
                                                 &group_w, &group_h);
          ShellCanvasFrameNode *single_selected =
              selection_count == 1
                  ? shell_canvas_scene_selected_frame(&canvas_scene)
                  : NULL;
          bool rotate_handle_hit = false;
          if (selected_path &&
              selected_path->kind == SHELL_NODE_PATH &&
              selection_count == 1) {
            path_point_hit = shell_canvas_path_hit_point(
                &layout, sidebar_state.left_panel_width,
                sidebar_state.right_panel_width, &canvas_view, selected_path,
                frame.mouse_x, frame.mouse_y, 8.0f);
          }
          canvas_interaction.mode = SHELL_CANVAS_INTERACT_NONE;
          canvas_interaction.target_frame = -1;
          canvas_interaction.target_point = -1;
          canvas_interaction.group_active = false;
          if (group_bounds_valid) {
            shell_canvas_world_to_screen(&layout, sidebar_state.left_panel_width,
                                         sidebar_state.right_panel_width,
                                         &canvas_view, group_x, group_y,
                                         &group_sx, &group_sy);
            group_sw = group_w * canvas_view.zoom;
            group_sh = group_h * canvas_view.zoom;
            rotate_handle_hit =
                shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                    group_sx + group_sw * 0.5f - 10.0f,
                                    group_sy - 38.0f, handle, handle);
          }
          if (path_point_hit >= 0 && selected_path) {
            selected_path->selected_point = path_point_hit;
            canvas_interaction.mode = SHELL_CANVAS_INTERACT_MOVE_POINT;
            canvas_interaction.target_frame = canvas_scene.selected_frame;
            canvas_interaction.target_point = path_point_hit;
            frame.event_requested = true;
          } else if (selected_path && selected_path->kind == SHELL_NODE_PATH) {
            selected_path->selected_point = -1;
          }
          if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_NONE &&
              rotate_handle_hit && selection_count > 0 && !frame.shift_down) {
            float center_x = 0.0f;
            float center_y = 0.0f;
            if (selection_count > 1) {
              center_x = group_x + group_w * 0.5f;
              center_y = group_y + group_h * 0.5f;
              canvas_interaction.group_active = true;
              shell_canvas_capture_group_origins(&canvas_scene, &canvas_interaction);
            } else if (single_selected) {
              shell_canvas_node_center_world(single_selected, &center_x,
                                             &center_y);
            }
            canvas_interaction.mode = SHELL_CANVAS_INTERACT_ROTATE;
            canvas_interaction.target_frame = canvas_scene.selected_frame;
            canvas_interaction.anchor_world_x = world_x;
            canvas_interaction.anchor_world_y = world_y;
            canvas_interaction.origin_rotation_deg =
                single_selected ? single_selected->rotation_deg : 0.0f;
            canvas_interaction.rotate_center_x = center_x;
            canvas_interaction.rotate_center_y = center_y;
            canvas_interaction.rotate_anchor_angle_deg =
                atan2f(world_y - center_y, world_x - center_x) *
                57.29577951308232f;
            frame.event_requested = true;
          }
          if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_NONE &&
              hit < 0 && group_bounds_valid &&
              selection_count > 1) {
            canvas_interaction.target_frame = canvas_scene.selected_frame;
            canvas_interaction.anchor_world_x = world_x;
            canvas_interaction.anchor_world_y = world_y;
            canvas_interaction.group_active = true;
            shell_canvas_capture_group_origins(&canvas_scene, &canvas_interaction);
            if (shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                    group_sx + group_sw * 0.5f - 10.0f,
                                    group_sy - 10.0f, handle, handle)) {
              canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_T;
            } else if (shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                           group_sx + group_sw * 0.5f - 10.0f,
                                           group_sy + group_sh - 10.0f, handle,
                                           handle)) {
              canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_B;
            } else if (shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                           group_sx - 10.0f,
                                           group_sy + group_sh * 0.5f - 10.0f,
                                           handle, handle)) {
              canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_L;
            } else if (shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                           group_sx + group_sw - 10.0f,
                                           group_sy + group_sh * 0.5f - 10.0f,
                                           handle, handle)) {
              canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_R;
            } else if (shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                           group_sx - 10.0f, group_sy - 10.0f,
                                           handle, handle)) {
              canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_TL;
            } else if (shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                           group_sx + group_sw - 10.0f,
                                           group_sy - 10.0f, handle, handle)) {
              canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_TR;
            } else if (shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                           group_sx - 10.0f,
                                           group_sy + group_sh - 10.0f, handle,
                                           handle)) {
              canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_BL;
            } else if (shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                           group_sx + group_sw - 10.0f,
                                           group_sy + group_sh - 10.0f, handle,
                                           handle)) {
              canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_BR;
            }
          }
          if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_NONE && hit >= 0) {
            float sx = 0.0f;
            float sy = 0.0f;
            float sw = 0.0f;
            float sh = 0.0f;
            float bounds_x = 0.0f;
            float bounds_y = 0.0f;
            float bounds_w = 0.0f;
            float bounds_h = 0.0f;
            float draw_x = 0.0f;
            float draw_y = 0.0f;
            float draw_w = 0.0f;
            float draw_h = 0.0f;
            bool use_group_bounds = false;
            ShellCanvasFrameNode *hit_frame = &canvas_scene.frames[hit];
            if (frame.shift_down) {
              shell_canvas_scene_toggle_selection(&canvas_scene, hit);
              frame.event_requested = true;
            } else {
              if (!hit_frame->selected)
                shell_canvas_scene_select_only(&canvas_scene, hit);
              selection_count = shell_canvas_scene_selection_count(&canvas_scene);
              use_group_bounds =
                  selection_count > 1 &&
                  shell_canvas_scene_selection_bounds(&canvas_scene, &bounds_x,
                                                     &bounds_y, &bounds_w,
                                                     &bounds_h);
              if (use_group_bounds) {
                shell_canvas_world_to_screen(
                    &layout, sidebar_state.left_panel_width,
                    sidebar_state.right_panel_width, &canvas_view, bounds_x,
                    bounds_y, &draw_x, &draw_y);
                draw_w = bounds_w * canvas_view.zoom;
                draw_h = bounds_h * canvas_view.zoom;
              } else {
                shell_canvas_frame_screen_bounds(
                    &layout, sidebar_state.left_panel_width,
                    sidebar_state.right_panel_width, &canvas_view, hit_frame,
                    &draw_x, &draw_y, &draw_w, &draw_h);
              }
              canvas_interaction.target_frame = hit;
              canvas_interaction.anchor_world_x = world_x;
              canvas_interaction.anchor_world_y = world_y;
              canvas_interaction.origin_world_x = hit_frame->x;
              canvas_interaction.origin_world_y = hit_frame->y;
              canvas_interaction.origin_w = hit_frame->w;
              canvas_interaction.origin_h = hit_frame->h;
              canvas_interaction.group_active = use_group_bounds;
              shell_canvas_capture_group_origins(&canvas_scene, &canvas_interaction);
              sx = draw_x;
              sy = draw_y;
              sw = draw_w;
              sh = draw_h;
              if (shell_point_in_rect(frame.mouse_x, frame.mouse_y, sx - 10.0f,
                                      sy - 10.0f, handle, handle)) {
                canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_TL;
              } else if (shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                             sx + sw - 10.0f, sy - 10.0f,
                                             handle, handle)) {
                canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_TR;
              } else if (shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                             sx - 10.0f, sy + sh - 10.0f,
                                             handle, handle)) {
                canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_BL;
              } else if (shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                             sx + sw - 10.0f, sy + sh - 10.0f,
                                             handle, handle)) {
                canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_BR;
              } else if (shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                             sx + sw * 0.5f - 10.0f, sy - 10.0f,
                                             handle, handle)) {
                canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_T;
              } else if (shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                             sx + sw * 0.5f - 10.0f,
                                             sy + sh - 10.0f, handle, handle)) {
                canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_B;
              } else if (shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                             sx - 10.0f, sy + sh * 0.5f - 10.0f,
                                             handle, handle)) {
                canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_L;
              } else if (shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                             sx + sw - 10.0f,
                                             sy + sh * 0.5f - 10.0f, handle,
                                             handle)) {
                canvas_interaction.mode = SHELL_CANVAS_INTERACT_RESIZE_R;
              } else {
                canvas_interaction.mode = SHELL_CANVAS_INTERACT_MOVE;
              }
            }
          } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_NONE) {
            canvas_interaction.mode = SHELL_CANVAS_INTERACT_MARQUEE;
            canvas_interaction.target_frame = -1;
            canvas_interaction.group_active = false;
            canvas_interaction.additive_selection = frame.shift_down;
            canvas_interaction.anchor_world_x = world_x;
            canvas_interaction.anchor_world_y = world_y;
            canvas_interaction.origin_world_x = world_x;
            canvas_interaction.origin_world_y = world_y;
          }
          frame.event_requested = true;
        }
        if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_MARQUEE) {
          if (frame.left_down) {
            canvas_interaction.origin_world_x = world_x;
            canvas_interaction.origin_world_y = world_y;
            frame.event_requested = true;
          } else {
            float min_x = canvas_interaction.anchor_world_x <
                                  canvas_interaction.origin_world_x
                              ? canvas_interaction.anchor_world_x
                              : canvas_interaction.origin_world_x;
            float min_y = canvas_interaction.anchor_world_y <
                                  canvas_interaction.origin_world_y
                              ? canvas_interaction.anchor_world_y
                              : canvas_interaction.origin_world_y;
            float box_w = fabsf(canvas_interaction.origin_world_x -
                                canvas_interaction.anchor_world_x);
            float box_h = fabsf(canvas_interaction.origin_world_y -
                                canvas_interaction.anchor_world_y);
            if (box_w >= 4.0f && box_h >= 4.0f) {
              shell_canvas_scene_select_in_rect(
                  &canvas_scene, min_x, min_y, box_w, box_h,
                  canvas_interaction.additive_selection);
            } else if (!canvas_interaction.additive_selection) {
              shell_canvas_scene_clear_selection(&canvas_scene);
            }
            canvas_interaction.mode = SHELL_CANVAS_INTERACT_NONE;
            canvas_interaction.additive_selection = false;
            frame.event_requested = true;
          }
        } else if (canvas_interaction.target_frame >= 0 &&
            canvas_interaction.target_frame < (int)canvas_scene.frame_count) {
          ShellCanvasFrameNode *target =
              &canvas_scene.frames[canvas_interaction.target_frame];
          float min_w = shell_canvas_node_min_width(target->kind);
          float min_h = shell_canvas_node_min_height(target->kind);
          if (frame.left_down) {
            float dx = world_x - canvas_interaction.anchor_world_x;
            float dy = world_y - canvas_interaction.anchor_world_y;
            if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_MOVE_POINT &&
                target->kind == SHELL_NODE_PATH &&
                canvas_interaction.target_point >= 0 &&
                canvas_interaction.target_point < target->point_count) {
              float local_world_x = 0.0f;
              float local_world_y = 0.0f;
              float local_x = 0.0f;
              float local_y = 0.0f;
              shell_canvas_node_world_to_local(target, world_x, world_y,
                                               &local_world_x, &local_world_y);
              local_x = (local_world_x - target->x) /
                        (target->w > 0.0f ? target->w : 1.0f);
              local_y = (local_world_y - target->y) /
                        (target->h > 0.0f ? target->h : 1.0f);
              target->path_x[canvas_interaction.target_point] =
                  shell_clamp01(local_x);
              target->path_y[canvas_interaction.target_point] =
                  shell_clamp01(local_y);
              target->selected_point = canvas_interaction.target_point;
            } else if (canvas_interaction.mode ==
                       SHELL_CANVAS_INTERACT_ROTATE) {
              float current_angle_deg =
                  atan2f(world_y - canvas_interaction.rotate_center_y,
                         world_x - canvas_interaction.rotate_center_x) *
                  57.29577951308232f;
              float delta_rotation =
                  current_angle_deg -
                  canvas_interaction.rotate_anchor_angle_deg;
              float next_rotation =
                  canvas_interaction.origin_rotation_deg + delta_rotation;
              if (frame.shift_down) {
                next_rotation = roundf(next_rotation / 15.0f) * 15.0f;
              }
              delta_rotation =
                  next_rotation - canvas_interaction.origin_rotation_deg;
              if (canvas_interaction.group_active) {
                shell_canvas_apply_group_rotation(&canvas_scene,
                                                 &canvas_interaction,
                                                 delta_rotation);
              } else {
                target->rotation_deg = shell_normalize_degrees(next_rotation);
              }
            } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_MOVE) {
              if (canvas_interaction.group_active) {
                float group_x = canvas_interaction.group_origin_x + dx;
                float group_y = canvas_interaction.group_origin_y + dy;
                uint32_t i = 0u;
                shell_canvas_snap_move_rect(
                    &canvas_scene, &layout, sidebar_state.left_panel_width,
                    sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                    canvas_interaction.target_frame,
                    canvas_interaction.group_origin_w,
                    canvas_interaction.group_origin_h, &group_x, &group_y,
                    &canvas_guides);
                for (i = 0u; i < canvas_scene.frame_count &&
                            i < SHELL_CANVAS_FRAME_CAP;
                     ++i) {
                  if (!canvas_scene.frames[i].alive ||
                      !canvas_scene.frames[i].selected)
                    continue;
                  canvas_scene.frames[i].x =
                      canvas_interaction.frame_origin_x[i] +
                      (group_x - canvas_interaction.group_origin_x);
                  canvas_scene.frames[i].y =
                      canvas_interaction.frame_origin_y[i] +
                      (group_y - canvas_interaction.group_origin_y);
                }
              } else {
                target->x = canvas_interaction.origin_world_x + dx;
                target->y = canvas_interaction.origin_world_y + dy;
                shell_canvas_snap_move_rect(
                    &canvas_scene, &layout, sidebar_state.left_panel_width,
                    sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                    canvas_interaction.target_frame, target->w, target->h,
                    &target->x, &target->y, &canvas_guides);
              }
            } else if (canvas_interaction.group_active) {
              float group_x = canvas_interaction.group_origin_x;
              float group_y = canvas_interaction.group_origin_y;
              float group_w = canvas_interaction.group_origin_w;
              float group_h = canvas_interaction.group_origin_h;
              if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_BR) {
                group_w = shell_clampf(canvas_interaction.group_origin_w + dx,
                                       96.0f, 4096.0f);
                group_h = shell_clampf(canvas_interaction.group_origin_h + dy,
                                       96.0f, 4096.0f);
                shell_canvas_snap_resize_rect(
                    &canvas_scene, &layout, sidebar_state.left_panel_width,
                    sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                    canvas_interaction.target_frame, false, true, false, true,
                    &group_x, &group_y, &group_w, &group_h, &canvas_guides);
              } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_TL) {
                group_x = canvas_interaction.group_origin_x + dx;
                group_y = canvas_interaction.group_origin_y + dy;
                group_w = shell_clampf(canvas_interaction.group_origin_w - dx,
                                       96.0f, 4096.0f);
                group_h = shell_clampf(canvas_interaction.group_origin_h - dy,
                                       96.0f, 4096.0f);
                shell_canvas_snap_resize_rect(
                    &canvas_scene, &layout, sidebar_state.left_panel_width,
                    sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                    canvas_interaction.target_frame, true, false, true, false,
                    &group_x, &group_y, &group_w, &group_h, &canvas_guides);
              } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_TR) {
                group_y = canvas_interaction.group_origin_y + dy;
                group_w = shell_clampf(canvas_interaction.group_origin_w + dx,
                                       96.0f, 4096.0f);
                group_h = shell_clampf(canvas_interaction.group_origin_h - dy,
                                       96.0f, 4096.0f);
                shell_canvas_snap_resize_rect(
                    &canvas_scene, &layout, sidebar_state.left_panel_width,
                    sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                    canvas_interaction.target_frame, false, true, true, false,
                    &group_x, &group_y, &group_w, &group_h, &canvas_guides);
              } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_BL) {
                group_x = canvas_interaction.group_origin_x + dx;
                group_w = shell_clampf(canvas_interaction.group_origin_w - dx,
                                       96.0f, 4096.0f);
                group_h = shell_clampf(canvas_interaction.group_origin_h + dy,
                                       96.0f, 4096.0f);
                shell_canvas_snap_resize_rect(
                    &canvas_scene, &layout, sidebar_state.left_panel_width,
                    sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                    canvas_interaction.target_frame, true, false, false, true,
                    &group_x, &group_y, &group_w, &group_h, &canvas_guides);
              } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_L) {
                group_x = canvas_interaction.group_origin_x + dx;
                group_w = shell_clampf(canvas_interaction.group_origin_w - dx,
                                       96.0f, 4096.0f);
                shell_canvas_snap_resize_rect(
                    &canvas_scene, &layout, sidebar_state.left_panel_width,
                    sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                    canvas_interaction.target_frame, true, false, false, false,
                    &group_x, &group_y, &group_w, &group_h, &canvas_guides);
              } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_R) {
                group_w = shell_clampf(canvas_interaction.group_origin_w + dx,
                                       96.0f, 4096.0f);
                shell_canvas_snap_resize_rect(
                    &canvas_scene, &layout, sidebar_state.left_panel_width,
                    sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                    canvas_interaction.target_frame, false, true, false, false,
                    &group_x, &group_y, &group_w, &group_h, &canvas_guides);
              } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_T) {
                group_y = canvas_interaction.group_origin_y + dy;
                group_h = shell_clampf(canvas_interaction.group_origin_h - dy,
                                       96.0f, 4096.0f);
                shell_canvas_snap_resize_rect(
                    &canvas_scene, &layout, sidebar_state.left_panel_width,
                    sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                    canvas_interaction.target_frame, false, false, true, false,
                    &group_x, &group_y, &group_w, &group_h, &canvas_guides);
              } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_B) {
                group_h = shell_clampf(canvas_interaction.group_origin_h + dy,
                                       96.0f, 4096.0f);
                shell_canvas_snap_resize_rect(
                    &canvas_scene, &layout, sidebar_state.left_panel_width,
                    sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                    canvas_interaction.target_frame, false, false, false, true,
                    &group_x, &group_y, &group_w, &group_h, &canvas_guides);
              }
              shell_canvas_apply_group_resize(&canvas_scene, &canvas_interaction,
                                              group_x, group_y, group_w, group_h);
            } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_BR) {
              target->w = shell_clampf(canvas_interaction.origin_w + dx, min_w, 4096.0f);
              target->h = shell_clampf(canvas_interaction.origin_h + dy, min_h, 4096.0f);
              shell_canvas_snap_resize_rect(
                  &canvas_scene, &layout, sidebar_state.left_panel_width,
                  sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                  canvas_interaction.target_frame, false, true, false, true,
                  &target->x, &target->y, &target->w, &target->h,
                  &canvas_guides);
            } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_TL) {
              target->x = canvas_interaction.origin_world_x + dx;
              target->y = canvas_interaction.origin_world_y + dy;
              target->w = shell_clampf(canvas_interaction.origin_w - dx, min_w, 4096.0f);
              target->h = shell_clampf(canvas_interaction.origin_h - dy, min_h, 4096.0f);
              shell_canvas_snap_resize_rect(
                  &canvas_scene, &layout, sidebar_state.left_panel_width,
                  sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                  canvas_interaction.target_frame, true, false, true, false,
                  &target->x, &target->y, &target->w, &target->h,
                  &canvas_guides);
            } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_TR) {
              target->y = canvas_interaction.origin_world_y + dy;
              target->w = shell_clampf(canvas_interaction.origin_w + dx, min_w, 4096.0f);
              target->h = shell_clampf(canvas_interaction.origin_h - dy, min_h, 4096.0f);
              shell_canvas_snap_resize_rect(
                  &canvas_scene, &layout, sidebar_state.left_panel_width,
                  sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                  canvas_interaction.target_frame, false, true, true, false,
                  &target->x, &target->y, &target->w, &target->h,
                  &canvas_guides);
            } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_BL) {
              target->x = canvas_interaction.origin_world_x + dx;
              target->w = shell_clampf(canvas_interaction.origin_w - dx, min_w, 4096.0f);
              target->h = shell_clampf(canvas_interaction.origin_h + dy, min_h, 4096.0f);
              shell_canvas_snap_resize_rect(
                  &canvas_scene, &layout, sidebar_state.left_panel_width,
                  sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                  canvas_interaction.target_frame, true, false, false, true,
                  &target->x, &target->y, &target->w, &target->h,
                  &canvas_guides);
            } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_L) {
              target->x = canvas_interaction.origin_world_x + dx;
              target->w = shell_clampf(canvas_interaction.origin_w - dx, min_w, 4096.0f);
              shell_canvas_snap_resize_rect(
                  &canvas_scene, &layout, sidebar_state.left_panel_width,
                  sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                  canvas_interaction.target_frame, true, false, false, false,
                  &target->x, &target->y, &target->w, &target->h,
                  &canvas_guides);
            } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_R) {
              target->w = shell_clampf(canvas_interaction.origin_w + dx, min_w, 4096.0f);
              shell_canvas_snap_resize_rect(
                  &canvas_scene, &layout, sidebar_state.left_panel_width,
                  sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                  canvas_interaction.target_frame, false, true, false, false,
                  &target->x, &target->y, &target->w, &target->h,
                  &canvas_guides);
            } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_T) {
              target->y = canvas_interaction.origin_world_y + dy;
              target->h = shell_clampf(canvas_interaction.origin_h - dy, min_h, 4096.0f);
              shell_canvas_snap_resize_rect(
                  &canvas_scene, &layout, sidebar_state.left_panel_width,
                  sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                  canvas_interaction.target_frame, false, false, true, false,
                  &target->x, &target->y, &target->w, &target->h,
                  &canvas_guides);
            } else if (canvas_interaction.mode == SHELL_CANVAS_INTERACT_RESIZE_B) {
              target->h = shell_clampf(canvas_interaction.origin_h + dy, min_h, 4096.0f);
              shell_canvas_snap_resize_rect(
                  &canvas_scene, &layout, sidebar_state.left_panel_width,
                  sidebar_state.right_panel_width, &canvas_view, &canvas_snap,
                  canvas_interaction.target_frame, false, false, false, true,
                  &target->x, &target->y, &target->w, &target->h,
                  &canvas_guides);
            }
            if (target->w < min_w)
              target->w = min_w;
            if (target->h < min_h)
              target->h = min_h;
            shell_canvas_clamp_frame_radius(target);
            frame.event_requested = true;
          } else {
            canvas_interaction.mode = SHELL_CANVAS_INTERACT_NONE;
            canvas_interaction.target_frame = -1;
            canvas_interaction.target_point = -1;
          }
        }
      } else if (!frame.left_down) {
        if (canvas_interaction.mode != SHELL_CANVAS_INTERACT_NONE)
          inspector_state.pending_sync = true;
        canvas_interaction.mode = SHELL_CANVAS_INTERACT_NONE;
        canvas_interaction.target_frame = -1;
        canvas_interaction.target_point = -1;
      }
    }

    if (!canvas_overlay_open && workspace_mode == SHELL_WORKSPACE_CANVAS &&
        !frame.left_down && !frame.middle_down && !inspector_state.editing &&
        !path_draft.active && canvas_interaction.mode == SHELL_CANVAS_INTERACT_NONE &&
        !shell_motion_preview_active(&motion_state)) {
      if (shell_canvas_history_push(&canvas_history, &canvas_scene))
        frame.event_requested = true;
    }

    if (!dialog_state.open) {
      menu_state.hot_menu =
          shell_hit_top_menu(&layout, frame.mouse_x, frame.mouse_y);
      menu_state.hot_item =
          shell_hit_menu_item(&layout, menu_state.open_menu, frame.mouse_x,
                              frame.mouse_y);
      if (menu_state.open_menu >= 0 && menu_state.hot_menu >= 0 &&
          menu_state.hot_menu != menu_state.open_menu) {
        menu_state.open_menu = menu_state.hot_menu;
        menu_state.hot_item =
            shell_hit_menu_item(&layout, menu_state.open_menu, frame.mouse_x,
                                frame.mouse_y);
        frame.event_requested = true;
      }
      if (frame.left_pressed) {
        if (menu_state.hot_menu >= 0) {
          menu_state.open_menu = menu_state.open_menu == menu_state.hot_menu
                                     ? -1
                                     : menu_state.hot_menu;
          menu_state.hot_item = -1;
          frame.event_requested = true;
        } else if (menu_state.open_menu >= 0 && menu_state.hot_item >= 0) {
          shell_execute_menu_action(window, ctx, &workspace_mode,
                                    &icon_lab_state, &dock_state,
                                    &show_tool_dock, &canvas_snap,
                                    &dialog_state,
                                    menu_state.open_menu, menu_state.hot_item);
          menu_state.open_menu = -1;
          menu_state.hot_item = -1;
          frame.event_requested = true;
        } else if (menu_state.open_menu >= 0 &&
                   !shell_point_in_open_menu_popup(&layout, menu_state.open_menu,
                                                  frame.mouse_x, frame.mouse_y)) {
          menu_state.open_menu = -1;
          menu_state.hot_item = -1;
          frame.event_requested = true;
        }
      }
      {
        bool hover_canvas = shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                                layout.workspace_x,
                                                layout.workspace_y,
                                                layout.workspace_seg_w,
                                                layout.workspace_h);
        bool hover_icons = shell_point_in_rect(
            frame.mouse_x, frame.mouse_y,
            layout.workspace_x + layout.workspace_seg_w, layout.workspace_y,
            layout.workspace_seg_w, layout.workspace_h);
        if (frame.left_pressed && hover_canvas) {
          workspace_mode = SHELL_WORKSPACE_CANVAS;
          dock_state.open_family = -1;
          frame.event_requested = true;
        } else if (frame.left_pressed && hover_icons) {
          workspace_mode = SHELL_WORKSPACE_ICON_LAB;
          dock_state.open_family = -1;
          frame.event_requested = true;
        }
      }
    } else {
      menu_state.open_menu = -1;
      menu_state.hot_menu = -1;
      menu_state.hot_item = -1;
      dock_state.open_family = -1;
    }
    if (!canvas_overlay_open && workspace_mode == SHELL_WORKSPACE_CANVAS &&
        show_tool_dock) {
      shell_update_tool_dock_state(&dock_state, &tool_dock, &frame, window,
                                   now_ms);
      if (dock_state.visible_t != prev_visible_t) {
        frame.event_requested = true;
      }
      if (dock_state.animating) {
        deferred_repaint_ms = 16u;
      } else if (!dock_state.pointer_inside && dock_state.visible_t > 0.0f &&
                 dock_state.hold_until_ms > now_ms) {
        deferred_repaint_ms = (uint32_t)(dock_state.hold_until_ms - now_ms);
        if (deferred_repaint_ms < 1u)
          deferred_repaint_ms = 1u;
      }
    } else if (!show_tool_dock) {
      dock_state.visible_t = 0.0f;
      dock_state.animating = false;
    }
    if (workspace_mode == SHELL_WORKSPACE_CANVAS &&
        !canvas_overlay_open &&
        (canvas_view.panning ||
         canvas_interaction.mode != SHELL_CANVAS_INTERACT_NONE ||
         shell_motion_preview_active(&motion_state) ||
         sidebar_state.dragging_left_panel ||
         sidebar_state.dragging_right_panel ||
         frame.left_down || frame.middle_down)) {
      if (deferred_repaint_ms == 0u || deferred_repaint_ms > 16u)
        deferred_repaint_ms = 16u;
    }
    stygian_begin_frame_intent(ctx, width, height, STYGIAN_FRAME_RENDER);
    if (deferred_repaint_ms > 0u) {
      stygian_request_repaint_after_ms(ctx, deferred_repaint_ms);
    }

    stygian_rect(ctx, 0.0f, 0.0f, (float)width, (float)height, 0.06f, 0.06f,
                 0.07f, 1.0f);
    stygian_rect_rounded(ctx, layout.shell_x, layout.shell_y, layout.shell_w,
                         layout.shell_h, 0.11f, 0.11f, 0.12f, 1.0f,
                         layout.shell_radius);
    stygian_rect_rounded(ctx, layout.shell_x, layout.shell_y, layout.shell_w,
                         layout.title_h + 8.0f, 0.09f, 0.09f, 0.10f, 1.0f,
                         layout.shell_radius);
    stygian_rect(ctx, layout.shell_x, layout.shell_y + layout.title_h,
                 layout.shell_w, 1.0f, 0.18f, 0.18f, 0.19f, 1.0f);
    stygian_rect(ctx, layout.canvas_x, layout.canvas_y, layout.canvas_w,
                 layout.canvas_h, 0.17f, 0.17f, 0.18f, 1.0f);

    shell_draw_top_menu_strip(ctx, font, &layout, frame.mouse_x, frame.mouse_y,
                              menu_state.open_menu);
    {
      bool hover_canvas = shell_point_in_rect(frame.mouse_x, frame.mouse_y,
                                              layout.workspace_x,
                                              layout.workspace_y,
                                              layout.workspace_seg_w,
                                              layout.workspace_h);
      bool hover_icons = shell_point_in_rect(
          frame.mouse_x, frame.mouse_y, layout.workspace_x + layout.workspace_seg_w,
          layout.workspace_y, layout.workspace_seg_w, layout.workspace_h);
      shell_draw_workspace_switch(ctx, font, &layout, workspace_mode,
                                  hover_canvas, hover_icons);
    }

    if (workspace_mode == SHELL_WORKSPACE_CANVAS) {
      float canvas_mouse_x =
          canvas_overlay_open ? -10000.0f : frame.mouse_x;
      float canvas_mouse_y =
          canvas_overlay_open ? -10000.0f : frame.mouse_y;
      shell_draw_canvas_rulers(ctx, font, &layout, sidebar_state.left_panel_width,
                               sidebar_state.right_panel_width, &canvas_view);
      shell_draw_canvas_frames(ctx, font, &layout, &canvas_scene,
                               sidebar_state.left_panel_width,
                               sidebar_state.right_panel_width, &canvas_view,
                               &canvas_interaction, &canvas_guides,
                               &dock_state, &path_draft, canvas_mouse_x,
                               canvas_mouse_y);
      shell_draw_canvas_import_surface(ctx, font, &layout, &canvas_import_state);
      {
        bool left_panel_dirty = false;
        shell_draw_workspace_panel(
            ctx, font, &layout, &frame, sidebar_state.left_panel_width,
            sidebar_state.dragging_left_panel, &canvas_scene, &workspace_panel,
            &motion_state, &canvas_view, sidebar_state.left_panel_width,
            sidebar_state.right_panel_width, &asset_library, &dialog_state,
            &commit_state, &canvas_import_state, &left_panel_dirty);
        if (left_panel_dirty)
          frame.event_requested = true;
      }
      bool right_panel_dirty = false;
      shell_draw_right_panel(ctx, font, &layout, canvas_mouse_x, canvas_mouse_y,
                             sidebar_state.right_panel_width,
                             sidebar_state.dragging_right_panel, &canvas_scene,
                             &inspector_state, &motion_state, &frame, now_ms,
                             frame.left_pressed,
                             inspector_force_sync ||
                                 canvas_interaction.mode != SHELL_CANVAS_INTERACT_NONE,
                             &right_panel_dirty);
      if (right_panel_dirty)
        frame.event_requested = true;
      {
        bool timeline_dirty = false;
        shell_draw_motion_timeline_panel(
            ctx, font, &layout, &frame, sidebar_state.left_panel_width,
            sidebar_state.right_panel_width, &canvas_scene, &motion_state,
            now_ms, &timeline_dirty);
        if (timeline_dirty)
          frame.event_requested = true;
      }
    } else {
      shell_draw_icon_lab_workspace(ctx, font, &layout, &frame, &icon_lab_state,
                                    now_ms);
    }

    if (workspace_mode == SHELL_WORKSPACE_CANVAS && show_tool_dock) {
      bool dock_open = dock_state.visible_t > 0.08f;
      bool buttons_live = dock_state.visible_t > 0.72f;
      float panel_alpha = 0.16f + dock_state.visible_t * 0.76f;
      float content_alpha = dock_state.visible_t;
      float reveal_y = tool_dock.hidden_y -
                       (tool_dock.hidden_y - tool_dock.visible_y) *
                           dock_state.visible_t;
      float x = tool_dock.rail_x;
      float y = reveal_y + (tool_dock.h - tool_dock.button_h) * 0.5f;
      float accent_x = x;
      float button_xs[10];
      float button_ws[10];
      float popup_x = 0.0f;
      float popup_y = 0.0f;
      float popup_w = 164.0f;
      float popup_h = 0.0f;
      bool any_button_hovered = false;
      bool popup_hovered = false;
      int i;
      dock_state.hot_index = -1;
      dock_state.popup_hot_index = -1;

      x = tool_dock.rail_x;
      for (i = 0; i < 10; ++i) {
        float w = (i == 0) ? 40.0f : tool_dock.button_w;
        if (i == 1 || i == 4 || i == 7)
          x += tool_dock.group_gap;
        button_xs[i] = x;
        button_ws[i] = w;
        x += w + 6.0f;
      }

      stygian_rect_rounded(ctx, tool_dock.x, reveal_y, tool_dock.w,
                           tool_dock.h, 0.07f, 0.07f, 0.08f, panel_alpha, 16.0f);
      stygian_rect_rounded(ctx, tool_dock.x + 1.0f, reveal_y + 1.0f,
                           tool_dock.w - 2.0f, tool_dock.h - 2.0f, 0.14f,
                           0.14f, 0.15f, 0.10f + dock_state.visible_t * 0.68f, 15.0f);
      stygian_rect_rounded(ctx, tool_dock.x + tool_dock.w * 0.5f - 22.0f,
                           reveal_y + tool_dock.h - 5.0f, 44.0f, 3.0f,
                           0.58f, 0.58f, 0.60f, 0.22f + dock_state.visible_t * 0.30f,
                           2.0f);

      if (dock_open) {
        for (i = 0; i < 10; ++i) {
          bool active = dock_state.active_tool == i;
          bool muted = (i == 7 || i == 8 || i == 9);
          bool has_family = shell_tool_has_family(i);
          float w = button_ws[i];
          float bx = button_xs[i];
          float chevron_x = bx + w - 15.0f;
          bool chevron_hover = false;
          bool hovered = false;
          bool pressed = false;
          int icon_kind = shell_tool_button_icon_kind(&dock_state, i);
          hovered = shell_point_in_rect(frame.mouse_x, frame.mouse_y, bx, y, w,
                                        tool_dock.button_h);
          chevron_hover =
              has_family &&
              shell_point_in_rect(frame.mouse_x, frame.mouse_y, chevron_x, y,
                                  15.0f, tool_dock.button_h);
          any_button_hovered = any_button_hovered || hovered;
          if (hovered) {
            dock_state.hot_index = i;
            dock_state.hold_until_ms = now_ms + 1400u;
          }
          if (buttons_live && hovered && frame.left_pressed) {
            if (chevron_hover) {
              dock_state.open_family = dock_state.open_family == i ? -1 : i;
              dock_state.pressed_index = -1;
              dock_state.popup_pressed_index = -1;
            } else {
              dock_state.pressed_index = i;
              dock_state.active_tool = i;
              dock_state.open_family = -1;
            }
            dock_state.hold_until_ms = now_ms + 1400u;
            frame.event_requested = true;
          }
          pressed = (dock_state.pressed_index == i) && frame.left_down;
          shell_draw_tool_button(ctx, bx, y, w, tool_dock.button_h, icon_kind,
                                 active, hovered, pressed, muted,
                                 content_alpha);
          if (frame.left_released && dock_state.pressed_index == i) {
            dock_state.pressed_index = -1;
            frame.event_requested = true;
          }
          if (has_family) {
            float chevron_alpha =
                (dock_state.open_family == i || chevron_hover) ? 0.96f : 0.78f;
            shell_draw_chevron_down(ctx, bx + w - 8.0f, y + 14.0f, 2.6f, 1.2f,
                                    0.86f, 0.86f, 0.88f,
                                    chevron_alpha * content_alpha);
          }
        }

        if (dock_state.open_family >= 0 &&
            shell_tool_has_family(dock_state.open_family)) {
          int family = dock_state.open_family;
          int popup_items = shell_tool_family_count(family);
          popup_h = 10.0f + popup_items * 30.0f;
          popup_x = button_xs[family] - 4.0f;
          popup_y = reveal_y - popup_h - 10.0f;
          if (popup_x + popup_w > tool_dock.x + tool_dock.w - 8.0f)
            popup_x = tool_dock.x + tool_dock.w - popup_w - 8.0f;
          if (popup_x < tool_dock.x + 8.0f)
            popup_x = tool_dock.x + 8.0f;
          stygian_rect_rounded(ctx, popup_x, popup_y, popup_w, popup_h, 0.05f,
                               0.05f, 0.06f, 0.98f * content_alpha, 13.0f);
          stygian_rect_rounded(ctx, popup_x + 1.0f, popup_y + 1.0f,
                               popup_w - 2.0f, popup_h - 2.0f, 0.12f, 0.12f,
                               0.13f, 0.92f * content_alpha, 12.0f);
          stygian_rect_rounded(ctx,
                               button_xs[family] + button_ws[family] * 0.5f - 6.0f,
                               popup_y + popup_h - 1.0f, 12.0f, 8.0f, 0.08f,
                               0.08f, 0.09f, 0.96f * content_alpha, 2.0f);
          popup_hovered =
              shell_point_in_rect(frame.mouse_x, frame.mouse_y, popup_x, popup_y,
                                  popup_w, popup_h);
          if (popup_hovered)
            dock_state.hold_until_ms = now_ms + 1400u;
          for (i = 0; i < popup_items; ++i) {
            float row_x = popup_x + 6.0f;
            float row_y = popup_y + 6.0f + i * 30.0f;
            float row_w = popup_w - 12.0f;
            float row_h = 26.0f;
            bool row_hovered =
                shell_point_in_rect(frame.mouse_x, frame.mouse_y, row_x, row_y,
                                    row_w, row_h);
            bool row_pressed = row_hovered && frame.left_down;
            int choice = dock_state.family_choice[family];
            if (row_hovered) {
              dock_state.popup_hot_index = i;
              any_button_hovered = true;
            }
            if (buttons_live && row_hovered && frame.left_pressed) {
              dock_state.family_choice[family] = i;
              dock_state.active_tool = family;
              dock_state.popup_pressed_index = i;
              dock_state.open_family = -1;
              dock_state.hold_until_ms = now_ms + 1400u;
              frame.event_requested = true;
            }
            shell_draw_tool_family_item(
                ctx, font, row_x, row_y, row_w, row_h,
                shell_tool_family_icon_kind(family, i),
                shell_tool_family_label(family, i), choice == i, row_hovered,
                row_pressed, content_alpha);
          }
          if (frame.left_released)
            dock_state.popup_pressed_index = -1;
        }

        if (buttons_live && frame.left_pressed && dock_state.open_family >= 0 &&
            !any_button_hovered && !popup_hovered) {
          dock_state.open_family = -1;
          dock_state.popup_pressed_index = -1;
          frame.event_requested = true;
        }
      }

      accent_x = tool_dock.x + 14.0f;
      stygian_rect_rounded(ctx, accent_x, y - 1.0f, 1.5f,
                           tool_dock.button_h + 2.0f, 0.19f, 0.55f, 0.96f,
                           0.22f + dock_state.visible_t * 0.45f, 1.5f);
    }

    if (canvas_context_menu.open &&
        workspace_mode == SHELL_WORKSPACE_CANVAS &&
        stygian_context_menu_begin(ctx, font, &canvas_context_menu, 14)) {
      int selection_count = shell_canvas_scene_selection_count(&canvas_scene);
      bool has_selection = selection_count > 0;
      bool multi = selection_count > 1;
      if (stygian_context_menu_item(ctx, font, &canvas_context_menu,
                                    "Select All", 0)) {
        shell_canvas_scene_select_all(&canvas_scene);
        inspector_state.pending_sync = true;
      }
      if (stygian_context_menu_item(ctx, font, &canvas_context_menu,
                                    "Copy", 1)) {
        int copied = shell_canvas_clipboard_copy_selected(&canvas_clipboard,
                                                          &canvas_scene);
        if (copied > 0) {
          char clip_text[64];
          snprintf(clip_text, sizeof(clip_text), "Stygian Editor nodes (%d)",
                   copied);
          stygian_clipboard_push(ctx, clip_text, "stygian-editor-scene-nodes");
        }
      }
      if (stygian_context_menu_item(ctx, font, &canvas_context_menu,
                                    "Cut", 2)) {
        int copied = shell_canvas_clipboard_copy_selected(&canvas_clipboard,
                                                          &canvas_scene);
        if (copied > 0) {
          char clip_text[64];
          snprintf(clip_text, sizeof(clip_text), "Stygian Editor nodes (%d)",
                   copied);
          stygian_clipboard_push(ctx, clip_text, "stygian-editor-scene-nodes");
          if (shell_canvas_scene_delete_selected(ctx, &canvas_scene))
            inspector_state.pending_sync = true;
        }
      }
      if (stygian_context_menu_item(ctx, font, &canvas_context_menu,
                                    "Paste", 3)) {
        if (shell_canvas_clipboard_paste(&canvas_scene, &canvas_clipboard))
          inspector_state.pending_sync = true;
      }
      if (stygian_context_menu_item(ctx, font, &canvas_context_menu,
                                    "Duplicate", 4)) {
        if (has_selection) {
          shell_canvas_scene_duplicate_selected(&canvas_scene);
          inspector_state.pending_sync = true;
        }
      }
      if (stygian_context_menu_item(ctx, font, &canvas_context_menu,
                                    "Delete", 5)) {
        if (has_selection) {
          shell_canvas_scene_delete_selected(ctx, &canvas_scene);
          inspector_state.pending_sync = true;
        }
      }
      if (stygian_context_menu_item(ctx, font, &canvas_context_menu,
                                    "Align Left", 6)) {
        if (has_selection) {
          shell_canvas_align_selected(&canvas_scene, SHELL_ALIGN_LEFT);
          inspector_state.pending_sync = true;
        }
      }
      if (stygian_context_menu_item(ctx, font, &canvas_context_menu,
                                    "Align Center", 7)) {
        if (has_selection) {
          shell_canvas_align_selected(&canvas_scene, SHELL_ALIGN_CENTER_X);
          inspector_state.pending_sync = true;
        }
      }
      if (stygian_context_menu_item(ctx, font, &canvas_context_menu,
                                    "Align Right", 8)) {
        if (has_selection) {
          shell_canvas_align_selected(&canvas_scene, SHELL_ALIGN_RIGHT);
          inspector_state.pending_sync = true;
        }
      }
      if (stygian_context_menu_item(ctx, font, &canvas_context_menu,
                                    "Align Top", 9)) {
        if (has_selection) {
          shell_canvas_align_selected(&canvas_scene, SHELL_ALIGN_TOP);
          inspector_state.pending_sync = true;
        }
      }
      if (stygian_context_menu_item(ctx, font, &canvas_context_menu,
                                    "Align Middle", 10)) {
        if (has_selection) {
          shell_canvas_align_selected(&canvas_scene, SHELL_ALIGN_CENTER_Y);
          inspector_state.pending_sync = true;
        }
      }
      if (stygian_context_menu_item(ctx, font, &canvas_context_menu,
                                    "Align Bottom", 11)) {
        if (has_selection) {
          shell_canvas_align_selected(&canvas_scene, SHELL_ALIGN_BOTTOM);
          inspector_state.pending_sync = true;
        }
      }
      if (stygian_context_menu_item(ctx, font, &canvas_context_menu,
                                    "Distribute Horizontal", 12)) {
        if (multi) {
          shell_canvas_distribute_selected(&canvas_scene, true);
          inspector_state.pending_sync = true;
        }
      }
      if (stygian_context_menu_item(ctx, font, &canvas_context_menu,
                                    "Distribute Vertical", 13)) {
        if (multi) {
          shell_canvas_distribute_selected(&canvas_scene, false);
          inspector_state.pending_sync = true;
        }
      }
      stygian_context_menu_end(ctx, &canvas_context_menu);
    }

    shell_draw_top_menu_popup(ctx, font, &layout, frame.mouse_x, frame.mouse_y,
                              menu_state.open_menu, show_tool_dock,
                              canvas_snap.grid_enabled);
    shell_draw_dialog(ctx, font, &layout, &frame, &dialog_state, &commit_state,
                      &canvas_import_state, &asset_library);

    shell_draw_caption_button(
        ctx, font, &layout, layout.min_x, "-",
        shell_hit_caption_button(&layout, frame.mouse_x, frame.mouse_y) ==
            SHELL_CAPTION_MINIMIZE,
        false);
    shell_draw_caption_button(
        ctx, font, &layout, layout.max_x,
        stygian_window_is_maximized(window) ? "o" : "+",
        shell_hit_caption_button(&layout, frame.mouse_x, frame.mouse_y) ==
            SHELL_CAPTION_MAXIMIZE,
        false);
    shell_draw_caption_button(
        ctx, font, &layout, layout.close_x, "x",
        shell_hit_caption_button(&layout, frame.mouse_x, frame.mouse_y) ==
            SHELL_CAPTION_CLOSE,
        true);

    stygian_end_frame(ctx);
  }

  shell_dialog_release_preview(ctx, &dialog_state);
  shell_canvas_import_release(ctx, &canvas_import_state);
  shell_canvas_scene_release(ctx, &canvas_scene);
  stygian_destroy(ctx);
  stygian_window_destroy(window);
  return 0;
}
