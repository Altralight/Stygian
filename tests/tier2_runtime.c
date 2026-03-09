#include "../include/stygian.h"
#include "../window/stygian_window.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

typedef struct TestEnv {
  StygianWindow *window;
  StygianContext *ctx;
} TestEnv;

static int g_failures = 0;

#define CHECK(cond, name)                                                      \
  do {                                                                         \
    if (cond) {                                                                \
      printf("[PASS] %s\n", name);                                             \
    } else {                                                                   \
      fprintf(stderr, "[FAIL] %s\n", name);                                    \
      g_failures++;                                                            \
    }                                                                          \
  } while (0)

static int test_env_init(TestEnv *env) {
  StygianWindowConfig win_cfg = {
      .width = 360,
      .height = 260,
      .title = "stygian_tier2_runtime",
      .flags = STYGIAN_WINDOW_OPENGL,
  };
  StygianConfig cfg;
  if (!env)
    return 0;
  memset(env, 0, sizeof(*env));

  env->window = stygian_window_create(&win_cfg);
  if (!env->window)
    return 0;

  memset(&cfg, 0, sizeof(cfg));
  cfg.backend = STYGIAN_BACKEND_OPENGL;
  cfg.max_elements = 256;
  cfg.max_textures = 64;
  cfg.window = env->window;
  env->ctx = stygian_create(&cfg);
  if (!env->ctx) {
    stygian_window_destroy(env->window);
    env->window = NULL;
    return 0;
  }
  return 1;
}

static void test_env_destroy(TestEnv *env) {
  if (!env)
    return;
  if (env->ctx) {
    stygian_destroy(env->ctx);
    env->ctx = NULL;
  }
  if (env->window) {
    stygian_window_destroy(env->window);
    env->window = NULL;
  }
}

static void begin_render_frame(TestEnv *env) {
  stygian_request_repaint_after_ms(env->ctx, 0u);
  stygian_begin_frame(env->ctx, 360, 260);
}

static void build_scope_rects(TestEnv *env, StygianScopeId id, int count) {
  int i;
  stygian_scope_begin(env->ctx, id);
  for (i = 0; i < count; i++) {
    stygian_rect(env->ctx, 10.0f + (float)(i * 8), 12.0f, 18.0f, 14.0f, 1.0f,
                 0.6f, 0.2f, 1.0f);
  }
  stygian_scope_end(env->ctx);
}

static void test_scope_replay_and_invalidation(TestEnv *env) {
  const StygianScopeId id = 0x90010001u;
  CHECK(stygian_scope_is_dirty(env->ctx, id), "unknown scope reports dirty");

  begin_render_frame(env);
  build_scope_rects(env, id, 1);
  stygian_end_frame(env->ctx);
  CHECK(!stygian_scope_is_dirty(env->ctx, id), "scope clean after first build");

  begin_render_frame(env);
  build_scope_rects(env, id, 1);
  stygian_end_frame(env->ctx);
  CHECK(stygian_get_last_frame_scope_replay_hits(env->ctx) >= 1u,
        "scope replay hit recorded");

  begin_render_frame(env);
  stygian_scope_begin(env->ctx, id);
  stygian_scope_end(env->ctx);
  stygian_end_frame(env->ctx);
  CHECK(stygian_get_last_frame_scope_forced_rebuilds(env->ctx) >= 1u,
        "scope replay mismatch forces rebuild");
  CHECK(stygian_scope_is_dirty(env->ctx, id), "scope dirty after forced rebuild");

  stygian_scope_invalidate_now(env->ctx, id);
  CHECK(stygian_scope_is_dirty(env->ctx, id), "invalidate_now marks scope dirty");
  begin_render_frame(env);
  build_scope_rects(env, id, 1);
  stygian_end_frame(env->ctx);
  CHECK(!stygian_scope_is_dirty(env->ctx, id),
        "dirty scope rebuilds and becomes clean");

  stygian_scope_invalidate_next(env->ctx, id);
  CHECK(stygian_scope_is_dirty(env->ctx, id),
        "invalidate_next marks scope pending dirty");
  begin_render_frame(env);
  build_scope_rects(env, id, 1);
  stygian_end_frame(env->ctx);
  CHECK(!stygian_scope_is_dirty(env->ctx, id),
        "scope clean again after rebuild from invalidate_next");
}

static void test_overlay_invalidation_isolated(TestEnv *env) {
  const StygianScopeId base_scope = 0x90020001u;
  const StygianScopeId overlay_scope =
      STYGIAN_OVERLAY_SCOPE_BASE | (StygianScopeId)0x33u;

  begin_render_frame(env);
  build_scope_rects(env, base_scope, 1);
  build_scope_rects(env, overlay_scope, 1);
  stygian_end_frame(env->ctx);
  CHECK(!stygian_scope_is_dirty(env->ctx, base_scope), "base scope starts clean");
  CHECK(!stygian_scope_is_dirty(env->ctx, overlay_scope),
        "overlay scope starts clean");

  stygian_invalidate_overlay_scopes(env->ctx);
  CHECK(!stygian_scope_is_dirty(env->ctx, base_scope),
        "overlay invalidation leaves base clean");
  CHECK(stygian_scope_is_dirty(env->ctx, overlay_scope),
        "overlay invalidation marks overlay dirty");
}

static void test_clip_runtime_behavior(TestEnv *env) {
  uint8_t clip_id;
  StygianElement e;
  int pushes = 0;
  int i;

  begin_render_frame(env);
  clip_id = stygian_clip_push(env->ctx, 4.0f, 4.0f, 100.0f, 100.0f);
  CHECK(clip_id != 0u, "clip_push returns valid id");
  e = stygian_element(env->ctx);
  CHECK(e != 0u, "element alloc for clip test");
  stygian_set_clip(env->ctx, e, clip_id);
  stygian_set_clip(env->ctx, e, 250u);
  stygian_clip_pop(env->ctx);
  stygian_end_frame(env->ctx);
  CHECK(stygian_get_last_frame_clip_count(env->ctx) == 2u,
        "frame clip count includes pushed clip");
  stygian_element_free(env->ctx, e);

  begin_render_frame(env);
  for (i = 0; i < 300; i++) {
    uint8_t id = stygian_clip_push(env->ctx, (float)i, 0.0f, 2.0f, 2.0f);
    if (id == 0u)
      break;
    pushes++;
  }
  stygian_end_frame(env->ctx);
  CHECK(pushes == 255, "clip ids are bounded to 255 user clips");
}

static void test_transient_cleanup_determinism(TestEnv *env) {
  StygianElement transient;
  uint32_t cap = stygian_get_element_capacity(env->ctx);

  begin_render_frame(env);
  transient = stygian_element_transient(env->ctx);
  CHECK(transient != 0u, "transient element allocated");
  CHECK(stygian_element_is_valid(env->ctx, transient), "transient handle valid");
  stygian_end_frame(env->ctx);

  begin_render_frame(env);
  CHECK(!stygian_element_is_valid(env->ctx, transient),
        "transient handle invalid after next frame reset");
  CHECK(stygian_get_free_element_count(env->ctx) == cap,
        "element free count resets to full capacity");
  stygian_end_frame(env->ctx);
}

static void test_color_icc_d50_adaptation_roundtrip(void) {
  static const float srgb_to_xyz[9] = {
      0.4124564f, 0.3575761f, 0.1804375f, 0.2126729f, 0.7151522f,
      0.0721750f, 0.0193339f, 0.1191920f, 0.9503041f,
  };
  StygianColorProfile src = {0};
  StygianColorProfile icc_like = {0};
  float in_r = 0.62f;
  float in_g = 0.44f;
  float in_b = 0.31f;
  float r = in_r;
  float g = in_g;
  float b = in_b;
  float shifted;
  float roundtrip_err;

  stygian_color_profile_init_builtin(&src, STYGIAN_COLOR_SPACE_SRGB);
  CHECK(stygian_color_profile_init_custom(&icc_like, "ICC synthetic",
                                          srgb_to_xyz, true, 2.4f),
        "custom ICC-like profile init succeeds");

  stygian_color_transform_rgb_f32(&src, &icc_like, &r, &g, &b);
  shifted = fabsf(r - in_r) + fabsf(g - in_g) + fabsf(b - in_b);
  CHECK(shifted > 0.0005f,
        "ICC-like transform applies whitepoint adaptation shift");

  stygian_color_transform_rgb_f32(&icc_like, &src, &r, &g, &b);
  roundtrip_err = fabsf(r - in_r) + fabsf(g - in_g) + fabsf(b - in_b);
  CHECK(roundtrip_err < 0.02f,
        "ICC-like whitepoint adaptation roundtrip remains stable");
}

static void test_output_icc_auto_controls(TestEnv *env) {
  bool refreshed = false;
  StygianICCInfo info;

  memset(&info, 0, sizeof(info));
#ifdef _WIN32
  CHECK(stygian_get_output_icc_auto(env->ctx),
        "output ICC auto defaults enabled on win32");
  CHECK(stygian_set_output_icc_auto(env->ctx, true),
        "output ICC auto enable succeeds");
  CHECK(stygian_get_output_icc_auto(env->ctx),
        "output ICC auto reports enabled");
  refreshed = stygian_refresh_output_icc_from_monitor(env->ctx, &info);
  CHECK(true, "output ICC monitor refresh call completed");
  if (refreshed) {
    CHECK(info.loaded, "output ICC monitor refresh returns loaded profile info");
  } else {
    CHECK(true, "output ICC monitor refresh tolerates unavailable profile");
  }
#else
  CHECK(!stygian_get_output_icc_auto(env->ctx),
        "output ICC auto defaults disabled off win32");
  CHECK(!stygian_set_output_icc_auto(env->ctx, true),
        "output ICC auto enable unsupported off win32");
#endif

  CHECK(stygian_set_output_color_space(env->ctx, STYGIAN_COLOR_SPACE_SRGB),
        "manual output color space set succeeds");
  CHECK(!stygian_get_output_icc_auto(env->ctx),
        "manual output color space set disables auto ICC");
  CHECK(stygian_set_output_icc_auto(env->ctx, false),
        "output ICC auto disable succeeds");
  CHECK(!stygian_get_output_icc_auto(env->ctx),
        "output ICC auto reports disabled");
}

#ifdef _WIN32
static void test_display_change_serial(TestEnv *env) {
  HWND hwnd;
  uint32_t before;
  uint32_t after;
  if (!env || !env->window)
    return;
  hwnd = (HWND)stygian_window_native_handle(env->window);
  CHECK(hwnd != NULL, "display serial test has native window handle");
  if (!hwnd)
    return;
  before = stygian_window_get_display_change_serial(env->window);
  SendMessage(hwnd, WM_DISPLAYCHANGE, 0, 0);
  after = stygian_window_get_display_change_serial(env->window);
  CHECK(after > before, "display change serial increments on WM_DISPLAYCHANGE");
}
#else
static void test_display_change_serial(TestEnv *env) {
  (void)env;
  CHECK(true, "display serial test skipped on non-Windows");
}
#endif

#ifdef _WIN32
static void pump_window_events(StygianWindow *window, int loops, DWORD sleep_ms) {
  int i;
  for (i = 0; i < loops; i++) {
    StygianEvent event;
    while (stygian_window_poll_event(window, &event)) {
    }
    if (sleep_ms > 0)
      Sleep(sleep_ms);
  }
}

typedef struct BorderlessWorkAreaProbe {
  RECT work_rect;
  RECT window_rect;
  POINT client_tl;
  POINT client_br;
  bool geometry_ok;
  bool matches_work_area;
  bool non_topmost;
} BorderlessWorkAreaProbe;

static bool wait_for_window_maximized_state(StygianWindow *window, bool expected,
                                            int loops, DWORD sleep_ms) {
  int i;
  if (!window)
    return false;
  for (i = 0; i < loops; i++) {
    StygianEvent event;
    while (stygian_window_poll_event(window, &event)) {
    }
    if (stygian_window_is_maximized(window) == expected)
      return true;
    if (sleep_ms > 0)
      Sleep(sleep_ms);
  }
  return stygian_window_is_maximized(window) == expected;
}

static bool probe_borderless_work_area(StygianWindow *window,
                                       BorderlessWorkAreaProbe *out_probe) {
  BorderlessWorkAreaProbe probe;
  HWND hwnd;
  HMONITOR monitor;
  MONITORINFO monitor_info;
  RECT client_rect;
  LONG_PTR ex_style;
  bool window_rect_ok = false;
  bool client_rect_ok = false;
  if (!window || !out_probe)
    return false;

  memset(&probe, 0, sizeof(probe));
  memset(&monitor_info, 0, sizeof(monitor_info));
  memset(&client_rect, 0, sizeof(client_rect));
  monitor_info.cbSize = sizeof(monitor_info);

  hwnd = (HWND)stygian_window_native_handle(window);
  monitor = (hwnd != NULL) ? MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST)
                           : NULL;
  window_rect_ok = (hwnd != NULL) && (GetWindowRect(hwnd, &probe.window_rect) != 0);
  if (hwnd != NULL && GetClientRect(hwnd, &client_rect) != 0) {
    probe.client_tl.x = client_rect.left;
    probe.client_tl.y = client_rect.top;
    probe.client_br.x = client_rect.right;
    probe.client_br.y = client_rect.bottom;
    client_rect_ok = (ClientToScreen(hwnd, &probe.client_tl) != 0) &&
                     (ClientToScreen(hwnd, &probe.client_br) != 0);
  }

  probe.geometry_ok =
      (hwnd != NULL) && (monitor != NULL) &&
      (GetMonitorInfo(monitor, &monitor_info) != 0) &&
      (window_rect_ok || client_rect_ok);
  if (probe.geometry_ok) {
    bool window_within_work_area;
    bool client_within_work_area;
    probe.work_rect = monitor_info.rcWork;
    window_within_work_area =
        (probe.window_rect.left >= probe.work_rect.left) &&
        (probe.window_rect.top >= probe.work_rect.top) &&
        (probe.window_rect.right <= probe.work_rect.right) &&
        (probe.window_rect.bottom <= probe.work_rect.bottom);
    client_within_work_area =
        (probe.client_tl.x >= probe.work_rect.left) &&
        (probe.client_tl.y >= probe.work_rect.top) &&
        (probe.client_br.x <= probe.work_rect.right) &&
        (probe.client_br.y <= probe.work_rect.bottom);
    probe.matches_work_area = window_within_work_area || client_within_work_area;
    ex_style = (hwnd != NULL) ? GetWindowLongPtr(hwnd, GWL_EXSTYLE) : 0;
    probe.non_topmost = (ex_style & WS_EX_TOPMOST) == 0;
  }

  *out_probe = probe;
  return probe.geometry_ok;
}

static bool wait_for_borderless_work_area_match(StygianWindow *window,
                                                BorderlessWorkAreaProbe *out_probe,
                                                int loops, DWORD sleep_ms) {
  BorderlessWorkAreaProbe probe;
  int i;
  if (!window || !out_probe)
    return false;
  memset(&probe, 0, sizeof(probe));
  for (i = 0; i < loops; i++) {
    StygianEvent event;
    while (stygian_window_poll_event(window, &event)) {
    }
    if (probe_borderless_work_area(window, &probe) && probe.matches_work_area) {
      *out_probe = probe;
      return true;
    }
    if (sleep_ms > 0)
      Sleep(sleep_ms);
  }
  (void)probe_borderless_work_area(window, &probe);
  *out_probe = probe;
  return false;
}

static void log_borderless_work_area_mismatch(const BorderlessWorkAreaProbe *probe) {
  if (!probe)
    return;
  fprintf(stderr,
          "[tier2] borderless maximize mismatch "
          "work=(%ld,%ld)-(%ld,%ld) "
          "window=(%ld,%ld)-(%ld,%ld) "
          "client=(%ld,%ld)-(%ld,%ld)\n",
          probe->work_rect.left, probe->work_rect.top, probe->work_rect.right,
          probe->work_rect.bottom, probe->window_rect.left,
          probe->window_rect.top, probe->window_rect.right,
          probe->window_rect.bottom, probe->client_tl.x, probe->client_tl.y,
          probe->client_br.x, probe->client_br.y);
}

typedef struct MonitorICCProbe {
  RECT rect;
  char icc_path[260];
  bool has_icc_path;
} MonitorICCProbe;

typedef struct MonitorICCProbeState {
  MonitorICCProbe monitors[8];
  uint32_t count;
} MonitorICCProbeState;

static BOOL CALLBACK monitor_icc_enum_proc(HMONITOR monitor, HDC hdc,
                                           LPRECT rect, LPARAM user_data) {
  MonitorICCProbeState *state = (MonitorICCProbeState *)user_data;
  MONITORINFOEXW monitor_info;
  HDC monitor_dc = NULL;
  DWORD profile_len = 0u;
  WCHAR *profile_w = NULL;
  int utf8_len = 0;
  MonitorICCProbe *probe;
  (void)hdc;
  if (!state || state->count >= 8u)
    return FALSE;
  probe = &state->monitors[state->count];
  memset(probe, 0, sizeof(*probe));
  if (rect) {
    probe->rect = *rect;
  }
  memset(&monitor_info, 0, sizeof(monitor_info));
  monitor_info.cbSize = sizeof(monitor_info);
  if (!GetMonitorInfoW(monitor, (MONITORINFO *)&monitor_info)) {
    state->count++;
    return TRUE;
  }
  probe->rect = monitor_info.rcMonitor;
  monitor_dc = CreateDCW(L"DISPLAY", monitor_info.szDevice, NULL, NULL);
  if (!monitor_dc) {
    state->count++;
    return TRUE;
  }
  if (GetICMProfileW(monitor_dc, &profile_len, NULL) && profile_len > 0u) {
    profile_w = (WCHAR *)malloc((size_t)profile_len * sizeof(WCHAR));
    if (profile_w &&
        GetICMProfileW(monitor_dc, &profile_len, profile_w) &&
        profile_w[0] != L'\0') {
      utf8_len = WideCharToMultiByte(CP_UTF8, 0, profile_w, -1, probe->icc_path,
                                     (int)sizeof(probe->icc_path), NULL, NULL);
      if (utf8_len > 0) {
        probe->has_icc_path = true;
      }
    }
  }
  if (profile_w) {
    free(profile_w);
  }
  DeleteDC(monitor_dc);
  state->count++;
  return TRUE;
}

static void test_dual_monitor_icc_adaptation(TestEnv *env) {
  MonitorICCProbeState state;
  int center_x0;
  int center_y0;
  int center_x1;
  int center_y1;
  bool monitor_pair_ready = false;
  StygianICCInfo info0;
  StygianICCInfo info1;

  if (!env || !env->ctx || !env->window)
    return;

  memset(&state, 0, sizeof(state));
  EnumDisplayMonitors(NULL, NULL, monitor_icc_enum_proc, (LPARAM)&state);
  if (state.count < 2u) {
    CHECK(true, "dual-monitor ICC adaptation skipped (single monitor)");
    return;
  }

  monitor_pair_ready = state.monitors[0].has_icc_path && state.monitors[1].has_icc_path &&
                       strcmp(state.monitors[0].icc_path,
                              state.monitors[1].icc_path) != 0;
  if (!monitor_pair_ready) {
    CHECK(true, "dual-monitor ICC adaptation skipped (profiles same/unavailable)");
    return;
  }

  center_x0 = (state.monitors[0].rect.left + state.monitors[0].rect.right) / 2 - 200;
  center_y0 = (state.monitors[0].rect.top + state.monitors[0].rect.bottom) / 2 - 150;
  center_x1 = (state.monitors[1].rect.left + state.monitors[1].rect.right) / 2 - 200;
  center_y1 = (state.monitors[1].rect.top + state.monitors[1].rect.bottom) / 2 - 150;

  CHECK(stygian_set_output_icc_auto(env->ctx, true),
        "dual-monitor ICC adaptation auto enable succeeds");

  stygian_window_set_position(env->window, center_x0, center_y0);
  pump_window_events(env->window, 60, 8);
  memset(&info0, 0, sizeof(info0));
  CHECK(stygian_refresh_output_icc_from_monitor(env->ctx, &info0),
        "dual-monitor ICC adaptation refresh on monitor 0 succeeds");

  stygian_window_set_position(env->window, center_x1, center_y1);
  pump_window_events(env->window, 60, 8);
  memset(&info1, 0, sizeof(info1));
  CHECK(stygian_refresh_output_icc_from_monitor(env->ctx, &info1),
        "dual-monitor ICC adaptation refresh on monitor 1 succeeds");

  CHECK(strcmp(info0.path, info1.path) != 0,
        "dual-monitor ICC adaptation switches profile path across monitors");
}

static void test_borderless_maximize_uses_work_area(void) {
  StygianWindowConfig win_cfg = {
      .width = 520,
      .height = 340,
      .title = "stygian_tier2_borderless_maximize",
      .flags = STYGIAN_WINDOW_OPENGL | STYGIAN_WINDOW_RESIZABLE |
               STYGIAN_WINDOW_BORDERLESS | STYGIAN_WINDOW_CENTERED,
  };
  StygianConfig cfg;
  StygianWindow *window = NULL;
  StygianContext *ctx = NULL;
  BorderlessWorkAreaProbe probe;
  bool maximized = false;
  bool restored = false;

  memset(&cfg, 0, sizeof(cfg));
  window = stygian_window_create(&win_cfg);
  CHECK(window != NULL, "borderless maximize fixture window created");
  if (!window)
    return;

  cfg.backend = STYGIAN_BACKEND_OPENGL;
  cfg.max_elements = 128;
  cfg.max_textures = 32;
  cfg.window = window;
  ctx = stygian_create(&cfg);
  CHECK(ctx != NULL, "borderless maximize fixture context created");
  if (!ctx) {
    stygian_window_destroy(window);
    return;
  }

  stygian_window_maximize(window);
  maximized = wait_for_window_maximized_state(window, true, 240, 8);
  CHECK(maximized, "borderless maximize reaches maximized state");

  if (maximized) {
    CHECK(wait_for_borderless_work_area_match(window, &probe, 240, 8),
          "borderless maximize monitor geometry query succeeds");
    if (probe.geometry_ok) {
      if (!probe.matches_work_area)
        log_borderless_work_area_mismatch(&probe);
      CHECK(probe.matches_work_area, "borderless maximize uses monitor work area");
      CHECK(probe.non_topmost, "borderless maximize keeps window non-topmost");
    }

    stygian_window_restore(window);
    restored = wait_for_window_maximized_state(window, false, 240, 8);
    CHECK(restored, "borderless restore clears maximized state");
  }

  stygian_destroy(ctx);
  stygian_window_destroy(window);
}

static void test_borderless_manual_rect_apply_repeatability(void) {
  StygianWindowConfig cfg = {
      .width = 540,
      .height = 360,
      .title = "stygian_tier2_borderless_rect_apply",
      .flags = STYGIAN_WINDOW_OPENGL | STYGIAN_WINDOW_RESIZABLE |
               STYGIAN_WINDOW_BORDERLESS | STYGIAN_WINDOW_CENTERED,
  };
  StygianWindow *window = stygian_window_create(&cfg);
  int pass;

  CHECK(window != NULL, "borderless rect-apply fixture window created");
  if (!window)
    return;

  for (pass = 0; pass < 4; pass++) {
    BorderlessWorkAreaProbe probe;
    bool maximized;
    bool restored;

    stygian_window_maximize(window);
    maximized = wait_for_window_maximized_state(window, true, 240, 8);
    if (!maximized) {
      fprintf(stderr, "[tier2] rect-apply pass %d failed to maximize\n", pass);
    }
    CHECK(maximized, "borderless rect apply reaches maximized state repeatedly");
    if (!maximized)
      break;

    CHECK(wait_for_borderless_work_area_match(window, &probe, 240, 8),
          "borderless rect apply geometry query succeeds repeatedly");
    if (probe.geometry_ok) {
      if (!probe.matches_work_area)
        log_borderless_work_area_mismatch(&probe);
      CHECK(probe.matches_work_area,
            "borderless rect apply stays within monitor work area repeatedly");
      CHECK(probe.non_topmost,
            "borderless rect apply stays non-topmost repeatedly");
    }

    stygian_window_restore(window);
    restored = wait_for_window_maximized_state(window, false, 240, 8);
    if (!restored) {
      fprintf(stderr, "[tier2] rect-apply pass %d failed to restore\n", pass);
    }
    CHECK(restored, "borderless rect apply restores repeatedly");
    if (!restored)
      break;
  }

  stygian_window_destroy(window);
}

static void test_borderless_style_routing(void) {
  StygianWindowConfig gl_cfg = {
      .width = 500,
      .height = 320,
      .title = "stygian_tier2_borderless_gl_style",
      .flags = STYGIAN_WINDOW_OPENGL | STYGIAN_WINDOW_RESIZABLE |
               STYGIAN_WINDOW_BORDERLESS | STYGIAN_WINDOW_CENTERED,
      .role = STYGIAN_ROLE_MAIN,
  };
  StygianWindowConfig vk_cfg = {
      .width = 500,
      .height = 320,
      .title = "stygian_tier2_borderless_vk_style",
      .flags = STYGIAN_WINDOW_VULKAN | STYGIAN_WINDOW_RESIZABLE |
               STYGIAN_WINDOW_BORDERLESS | STYGIAN_WINDOW_CENTERED,
      .role = STYGIAN_ROLE_MAIN,
  };
  StygianWindow *gl_window = stygian_window_create(&gl_cfg);
  StygianWindow *vk_window = NULL;

  CHECK(gl_window != NULL, "opengl borderless style fixture window created");
  if (gl_window) {
    HWND hwnd = (HWND)stygian_window_native_handle(gl_window);
    LONG_PTR style = (hwnd != NULL) ? GetWindowLongPtr(hwnd, GWL_STYLE) : 0;
    CHECK((style & WS_POPUP) != 0, "opengl borderless main keeps popup style");
    stygian_window_destroy(gl_window);
  }

  vk_window = stygian_window_create(&vk_cfg);
  CHECK(vk_window != NULL, "vulkan borderless style fixture window created");
  if (vk_window) {
    HWND hwnd = (HWND)stygian_window_native_handle(vk_window);
    LONG_PTR style = (hwnd != NULL) ? GetWindowLongPtr(hwnd, GWL_STYLE) : 0;
    CHECK((style & WS_POPUP) != 0, "vulkan borderless main keeps popup style");
    stygian_window_destroy(vk_window);
  }
}

static void test_titlebar_behavior_and_actions(void) {
  StygianWindowConfig cfg = {
      .width = 640,
      .height = 420,
      .title = "stygian_tier2_titlebar_behavior",
      .flags = STYGIAN_WINDOW_OPENGL | STYGIAN_WINDOW_RESIZABLE |
               STYGIAN_WINDOW_BORDERLESS | STYGIAN_WINDOW_CENTERED,
      .role = STYGIAN_ROLE_MAIN,
  };
  StygianWindow *window = stygian_window_create(&cfg);
  StygianTitlebarBehavior behavior = {0};
  StygianTitlebarHints hints = {0};
  StygianTitlebarMenuAction actions[16];
  uint32_t action_count = 0u;
  bool has_maximize = false;
  bool has_fullscreen = false;
  bool has_snap_right = false;
  bool fullscreen_toggled = false;
  bool fullscreen_restored = false;
  bool begin_move_ok = false;
  bool snap_applied = false;

  CHECK(window != NULL, "titlebar behavior fixture window created");
  if (!window)
    return;

  stygian_window_get_titlebar_hints(window, &hints);
  CHECK(hints.button_order == STYGIAN_TITLEBAR_BUTTONS_RIGHT,
        "win32 titlebar hints default to right button order");
  CHECK(hints.supports_hover_menu, "win32 titlebar hints expose hover menu");
  CHECK(hints.supports_snap_actions, "win32 titlebar hints expose snap actions");

  stygian_window_get_titlebar_behavior(window, &behavior);
  CHECK(behavior.double_click_mode == STYGIAN_TITLEBAR_DBLCLICK_MAXIMIZE_RESTORE,
        "titlebar double-click defaults to maximize/restore");

  stygian_window_titlebar_double_click(window);
  pump_window_events(window, 80, 8);
  CHECK(stygian_window_is_maximized(window),
        "titlebar double-click toggles to maximized");
  stygian_window_titlebar_double_click(window);
  pump_window_events(window, 80, 8);
  CHECK(!stygian_window_is_maximized(window),
        "titlebar double-click toggles restore");

  behavior.double_click_mode = STYGIAN_TITLEBAR_DBLCLICK_FULLSCREEN_TOGGLE;
  behavior.hover_menu_enabled = true;
  stygian_window_set_titlebar_behavior(window, &behavior);
  stygian_window_titlebar_double_click(window);
  pump_window_events(window, 60, 8);
  fullscreen_toggled = stygian_window_is_fullscreen(window);
  CHECK(fullscreen_toggled, "fullscreen policy toggles on double-click");
  stygian_window_titlebar_double_click(window);
  pump_window_events(window, 60, 8);
  fullscreen_restored = !stygian_window_is_fullscreen(window);
  CHECK(fullscreen_restored, "fullscreen policy toggles back on double-click");

  action_count = stygian_window_get_titlebar_menu_actions(
      window, actions, (uint32_t)(sizeof(actions) / sizeof(actions[0])));
  CHECK(action_count >= 8u, "titlebar menu exposes native preset action set");
  if (action_count > 0u) {
    uint32_t i;
    for (i = 0u; i < action_count; i++) {
      if (actions[i] == STYGIAN_TITLEBAR_ACTION_MAXIMIZE ||
          actions[i] == STYGIAN_TITLEBAR_ACTION_RESTORE) {
        has_maximize = true;
      }
      if (actions[i] == STYGIAN_TITLEBAR_ACTION_ENTER_FULLSCREEN ||
          actions[i] == STYGIAN_TITLEBAR_ACTION_EXIT_FULLSCREEN) {
        has_fullscreen = true;
      }
      if (actions[i] == STYGIAN_TITLEBAR_ACTION_SNAP_RIGHT) {
        has_snap_right = true;
      }
    }
  }
  CHECK(has_maximize, "titlebar menu includes maximize/restore action");
  CHECK(has_fullscreen, "titlebar menu includes fullscreen action");
  CHECK(has_snap_right, "titlebar menu includes snap action");

  snap_applied = stygian_window_apply_titlebar_menu_action(
      window, STYGIAN_TITLEBAR_ACTION_SNAP_RIGHT);
  CHECK(snap_applied, "snap-right titlebar action applies");
  if (snap_applied) {
    HWND hwnd = (HWND)stygian_window_native_handle(window);
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info;
    RECT rect;
    bool geometry_ok;
    bool right_half_ok = false;
    LONG expected_left;

    memset(&monitor_info, 0, sizeof(monitor_info));
    monitor_info.cbSize = sizeof(monitor_info);
    memset(&rect, 0, sizeof(rect));
    pump_window_events(window, 60, 8);
    geometry_ok =
        (hwnd != NULL) && (monitor != NULL) &&
        (GetMonitorInfo(monitor, &monitor_info) != 0) &&
        (GetWindowRect(hwnd, &rect) != 0);
    CHECK(geometry_ok, "snap-right geometry query succeeds");
    if (geometry_ok) {
      LONG half_width = (monitor_info.rcWork.right - monitor_info.rcWork.left) / 2;
      expected_left = monitor_info.rcWork.right - half_width;
      right_half_ok = (rect.right <= monitor_info.rcWork.right + 1) &&
                      (rect.right >= monitor_info.rcWork.right - 1) &&
                      (rect.left >= expected_left - 1) &&
                      (rect.left <= expected_left + 1);
      CHECK(right_half_ok, "snap-right aligns to monitor work-area right half");
    }
  }

  begin_move_ok = stygian_window_begin_system_move(window);
  CHECK(begin_move_ok, "begin_system_move returns success on win32");
  pump_window_events(window, 30, 8);

  stygian_window_destroy(window);
}
#else
static void test_borderless_maximize_uses_work_area(void) {
  CHECK(true, "borderless maximize work-area check skipped on non-Windows");
}

static void test_borderless_style_routing(void) {
  CHECK(true, "borderless style routing check skipped on non-Windows");
}

static void test_borderless_manual_rect_apply_repeatability(void) {
  CHECK(true, "borderless rect-apply repeatability check skipped on non-Windows");
}

static void test_titlebar_behavior_and_actions(void) {
  CHECK(true, "titlebar behavior checks skipped on non-Windows");
}
#endif

int main(void) {
  TestEnv env;
  test_color_icc_d50_adaptation_roundtrip();
  if (!test_env_init(&env)) {
    fprintf(stderr, "[ERROR] failed to initialize tier2 runtime test env\n");
    return 2;
  }

  test_scope_replay_and_invalidation(&env);
  test_overlay_invalidation_isolated(&env);
  test_clip_runtime_behavior(&env);
  test_transient_cleanup_determinism(&env);
  test_output_icc_auto_controls(&env);
  test_display_change_serial(&env);
#ifdef _WIN32
  test_dual_monitor_icc_adaptation(&env);
#endif
  test_borderless_maximize_uses_work_area();
  test_borderless_manual_rect_apply_repeatability();
  test_borderless_style_routing();
  test_titlebar_behavior_and_actions();

  test_env_destroy(&env);

  if (g_failures == 0) {
    printf("[PASS] tier2 runtime suite complete\n");
    return 0;
  }
  fprintf(stderr, "[FAIL] tier2 runtime suite failures=%d\n", g_failures);
  return 1;
}
