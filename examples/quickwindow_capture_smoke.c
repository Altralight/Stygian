#include "stygian.h"
#include "stygian_window.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(STYGIAN_CAPTURE_ENABLED)
#include "stygian_capture.h"
#endif

#ifdef STYGIAN_DEMO_VULKAN
#define STYGIAN_SMOKE_BACKEND STYGIAN_BACKEND_VULKAN
#define STYGIAN_SMOKE_WINDOW_RENDER_FLAG STYGIAN_WINDOW_VULKAN
#define STYGIAN_SMOKE_BACKEND_NAME "Vulkan"
#else
#define STYGIAN_SMOKE_BACKEND STYGIAN_BACKEND_OPENGL
#define STYGIAN_SMOKE_WINDOW_RENDER_FLAG STYGIAN_WINDOW_OPENGL
#define STYGIAN_SMOKE_BACKEND_NAME "OpenGL"
#endif

static uint64_t stygian_smoke_now_ms(void) {
#ifdef _WIN32
  return (uint64_t)GetTickCount64();
#elif defined(CLOCK_MONOTONIC)
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
  }
#endif
  return (uint64_t)(clock() * 1000u / CLOCKS_PER_SEC);
}

static void stygian_smoke_sleep_ms(uint32_t ms) {
#ifdef _WIN32
  Sleep(ms);
#else
  struct timespec req;
  req.tv_sec = (time_t)(ms / 1000u);
  req.tv_nsec = (long)((ms % 1000u) * 1000000u);
  nanosleep(&req, NULL);
#endif
}

static uint64_t stygian_smoke_capture_duration_ms(void) {
  const char *seconds_env = getenv("STYGIAN_CAPTURE_SMOKE_SECONDS");
  unsigned long long seconds = 5ull;
  if (seconds_env && seconds_env[0]) {
    char *end = NULL;
    unsigned long long parsed = strtoull(seconds_env, &end, 10);
    if (end && *end == '\0' && parsed > 0ull) {
      seconds = parsed;
    }
  }
  if (seconds > 3600ull) {
    seconds = 3600ull;
  }
  return seconds * 1000ull;
}

int main(void) {
  const uint64_t capture_duration_ms = stygian_smoke_capture_duration_ms();
  StygianWindowConfig win_cfg = {
      .width = 1920,
      .height = 1080,
      .title = "Stygian Capture Smoke",
      .flags = STYGIAN_WINDOW_RESIZABLE | STYGIAN_SMOKE_WINDOW_RENDER_FLAG,
  };
  StygianWindow *window = stygian_window_create(&win_cfg);
  if (!window)
    return 1;

  StygianConfig cfg = {
      .backend = STYGIAN_SMOKE_BACKEND,
      .window = window,
  };
  StygianContext *ctx = stygian_create(&cfg);
  if (!ctx) {
    stygian_window_destroy(window);
    return 1;
  }

  StygianFont font =
      stygian_font_load(ctx, "assets/atlas.png", "assets/atlas.json");

#if defined(STYGIAN_CAPTURE_ENABLED)
  const char *capture_output_path = getenv("STYGIAN_CAPTURE_OUTPUT");
  if (!capture_output_path || !capture_output_path[0]) {
    capture_output_path = "quickwindow_capture_smoke.mp4";
  }
  StygianCaptureConfig capture_cfg = {
      .container = STYGIAN_CAPTURE_CONTAINER_MP4,
      .fps_num = 60u,
      .fps_den = 1u,
      .output_path = capture_output_path,
  };
  StygianCaptureTelemetry capture_telemetry = {0};
  StygianCapture *capture = NULL;
  bool capture_active = false;
  bool capture_info_printed = false;
  StygianAP *ap = stygian_get_ap(ctx);
  if (ap) {
    capture = stygian_capture_create(ap, &capture_cfg);
    if (capture && stygian_capture_is_available(capture) &&
        stygian_capture_begin(capture)) {
      capture_active = true;
      printf("[capture] started (%s, %llums)\n", capture_output_path,
             (unsigned long long)capture_duration_ms);
    } else if (capture) {
      const char *err = stygian_capture_last_error(capture);
      printf("[capture] unavailable: %s\n", err ? err : "unknown");
    } else {
      printf("[capture] create failed\n");
    }
  } else {
    printf("[capture] no AP available\n");
  }
#else
  printf("[capture] disabled in this build. Rebuild with -EnableCapture.\n");
#endif

  uint64_t start_ms = stygian_smoke_now_ms();
  uint64_t frame_index = 0u;
  while (!stygian_window_should_close(window)) {
    StygianEvent event;
    uint64_t frame_begin_ms = stygian_smoke_now_ms();
    while (stygian_window_poll_event(window, &event)) {
      if (event.type == STYGIAN_EVENT_CLOSE) {
        stygian_window_request_close(window);
      }
    }

    if (frame_begin_ms - start_ms >= capture_duration_ms) {
      stygian_window_request_close(window);
    }

    {
      int width = 0;
      int height = 0;
      float panel_w = 360.0f;
      float bar_w = 160.0f;
      float bar_x = 0.0f;
      stygian_window_get_size(window, &width, &height);
      stygian_begin_frame(ctx, width, height);
      stygian_rect(ctx, 0.0f, 0.0f, (float)width, (float)height, 0.08f, 0.1f,
                   0.13f, 1.0f);
      stygian_rect(ctx, 0.0f, 0.0f, (float)width, 44.0f, 0.11f, 0.14f, 0.18f,
                   1.0f);
      stygian_rect(ctx, 16.0f, 68.0f, panel_w, 120.0f, 0.13f, 0.18f, 0.24f,
                   1.0f);
      if (width > 0) {
        bar_x = (float)((frame_index * 6u) % (uint64_t)(width + 220)) - bar_w;
      }
      stygian_rect(ctx, bar_x, (float)height * 0.5f, bar_w, 20.0f, 0.2f, 0.45f,
                   0.9f, 1.0f);
      if (font) {
        stygian_text(ctx, font, "Capture Smoke", 16.0f, 12.0f, 16.0f, 0.92f,
                     0.95f, 1.0f, 1.0f);
        stygian_text(ctx, font, STYGIAN_SMOKE_BACKEND_NAME, 18.0f, 88.0f, 18.0f,
                     0.9f, 0.94f, 1.0f, 1.0f);
        stygian_text(ctx, font, "Runs 5 seconds and logs capture telemetry.",
                     18.0f, 118.0f, 14.0f, 0.8f, 0.86f, 0.94f, 1.0f);
      }
      stygian_end_frame(ctx);
    }

#if defined(STYGIAN_CAPTURE_ENABLED)
    if (capture_active) {
      StygianAPCaptureFrameInfo info = {0};
      (void)stygian_capture_capture_frame(capture, NULL, 0u, &info);
      if (!capture_info_printed && info.width > 0 && info.height > 0 &&
          info.stride_bytes > 0u) {
        printf("[capture] frame: %dx%d stride=%u\n", info.width, info.height,
               info.stride_bytes);
        capture_info_printed = true;
      }
    }
#endif

    frame_index++;
    {
      uint64_t frame_ms = stygian_smoke_now_ms() - frame_begin_ms;
      if (frame_ms < 16u) {
        stygian_smoke_sleep_ms((uint32_t)(16u - frame_ms));
      }
    }
  }

#if defined(STYGIAN_CAPTURE_ENABLED)
  if (capture) {
    if (capture_active) {
      stygian_capture_end(capture);
      printf("[capture] output: %s\n", capture_output_path);
    } else {
      printf("[capture] no output file (capture unsupported on this backend in v1)\n");
    }
    stygian_capture_get_telemetry(capture, &capture_telemetry);
    printf("[capture] requested=%llu captured=%llu encoded=%llu dropped=%llu "
           "readback_failures=%llu encode_failures=%llu\n",
           (unsigned long long)capture_telemetry.frames_requested,
           (unsigned long long)capture_telemetry.frames_captured,
           (unsigned long long)capture_telemetry.frames_encoded,
           (unsigned long long)capture_telemetry.frames_dropped,
           (unsigned long long)capture_telemetry.readback_failures,
           (unsigned long long)capture_telemetry.encode_failures);
    {
      const char *err = stygian_capture_last_error(capture);
      if (err && err[0]) {
        printf("[capture] last_error: %s\n", err);
      }
    }
    stygian_capture_destroy(capture);
  }
#endif

  if (font)
    stygian_font_destroy(ctx, font);
  stygian_destroy(ctx);
  stygian_window_destroy(window);
  return 0;
}
