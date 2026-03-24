// stygian_ap_gl.c - OpenGL 4.3+ access-point backend
#include "../include/stygian.h" // Public enums/types shared with AP.
#include "../include/stygian_memory.h"
#include "../src/stygian_internal.h" // For SoA struct types
#include "../window/stygian_window.h"
#include "stygian_ap.h"

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#include <strings.h>
#elif defined(_WIN32)
#include <GL/gl.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#include <strings.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")
#endif

// ============================================================================
// OpenGL Types & Constants
// ============================================================================

#ifdef _WIN32
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;
typedef unsigned char GLboolean;
typedef float GLfloat;
typedef unsigned int GLbitfield;
typedef unsigned long long GLuint64;
#ifndef __GLsync
typedef struct __GLsync *GLsync;
#endif
#endif

// Use ifndef guards to avoid conflicts with system gl.h
#ifndef GL_FALSE
#define GL_FALSE 0
#endif
#ifndef GL_TRUE
#define GL_TRUE 1
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif
#ifndef GL_FLOAT
#define GL_FLOAT 0x1406
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_PIXEL_PACK_BUFFER
#define GL_PIXEL_PACK_BUFFER 0x88EB
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_STREAM_READ
#define GL_STREAM_READ 0x88E1
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_VALIDATE_STATUS
#define GL_VALIDATE_STATUS 0x8B83
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COLOR_BUFFER_BIT
#define GL_COLOR_BUFFER_BIT 0x00004000
#endif
#ifndef GL_BLEND
#define GL_BLEND 0x0BE2
#endif
#ifndef GL_SRC_ALPHA
#define GL_SRC_ALPHA 0x0302
#endif
#ifndef GL_ONE_MINUS_SRC_ALPHA
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#endif
#ifndef GL_TEXTURE_2D
#define GL_TEXTURE_2D 0x0DE1
#endif
#ifndef GL_TEXTURE_MIN_FILTER
#define GL_TEXTURE_MIN_FILTER 0x2801
#endif
#ifndef GL_TEXTURE_MAG_FILTER
#define GL_TEXTURE_MAG_FILTER 0x2800
#endif
#ifndef GL_LINEAR
#define GL_LINEAR 0x2601
#endif
#ifndef GL_RGBA
#define GL_RGBA 0x1908
#endif
#ifndef GL_UNSIGNED_BYTE
#define GL_UNSIGNED_BYTE 0x1401
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_MAX_TEXTURE_IMAGE_UNITS
#define GL_MAX_TEXTURE_IMAGE_UNITS 0x8872
#endif
#ifndef GL_MAX_FRAGMENT_UNIFORM_COMPONENTS
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS 0x8B49
#endif
#ifndef GL_MAX_VARYING_COMPONENTS
#define GL_MAX_VARYING_COMPONENTS 0x8B4B
#endif
#ifndef GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#endif
#ifndef GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS
#define GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS 0x90D6
#endif
#ifndef GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS
#define GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS 0x90DA
#endif
#ifndef GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS
#define GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS 0x90DC
#endif
#ifndef GL_TIME_ELAPSED
#define GL_TIME_ELAPSED 0x88BF
#endif
#ifndef GL_QUERY_RESULT
#define GL_QUERY_RESULT 0x8866
#endif
#ifndef GL_QUERY_RESULT_AVAILABLE
#define GL_QUERY_RESULT_AVAILABLE 0x8867
#endif
#ifndef GL_MAP_READ_BIT
#define GL_MAP_READ_BIT 0x0001
#endif
#ifndef GL_SYNC_GPU_COMMANDS_COMPLETE
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#endif
#ifndef GL_ALREADY_SIGNALED
#define GL_ALREADY_SIGNALED 0x911A
#endif
#ifndef GL_TIMEOUT_EXPIRED
#define GL_TIMEOUT_EXPIRED 0x911B
#endif
#ifndef GL_CONDITION_SATISFIED
#define GL_CONDITION_SATISFIED 0x911C
#endif
#ifndef GL_WAIT_FAILED
#define GL_WAIT_FAILED 0x911D
#endif

// ============================================================================
// OpenGL Function Pointers
// ============================================================================

typedef void (*StygianPFNGLDrawArraysInstancedBaseInstanceProc)(
    GLenum, GLint, GLsizei, GLsizei, GLuint);
typedef void (*StygianPFNGLActiveTextureProc)(GLenum);

#ifdef _WIN32
typedef void (*PFNGLGENBUFFERSPROC)(GLsizei, GLuint *);
typedef void (*PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void (*PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const void *, GLenum);
typedef void (*PFNGLBUFFERSUBDATAPROC)(GLenum, GLsizeiptr, GLsizeiptr,
                                       const void *);
typedef void (*PFNGLBINDBUFFERBASEPROC)(GLenum, GLuint, GLuint);
typedef GLuint (*PFNGLCREATESHADERPROC)(GLenum);
typedef void (*PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar **,
                                      const GLint *);
typedef void (*PFNGLCOMPILESHADERPROC)(GLuint);
typedef void (*PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint *);
typedef void (*PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei *, GLchar *);
typedef GLuint (*PFNGLCREATEPROGRAMPROC)(void);
typedef void (*PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void (*PFNGLLINKPROGRAMPROC)(GLuint);
typedef void (*PFNGLUSEPROGRAMPROC)(GLuint);
typedef void (*PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint *);
typedef void (*PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei *,
                                           GLchar *);
typedef GLint (*PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar *);
typedef void (*PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void (*PFNGLUNIFORM1IVPROC)(GLint, GLsizei, const GLint *);
typedef void (*PFNGLUNIFORM1FPROC)(GLint, GLfloat);
typedef void (*PFNGLUNIFORM2FPROC)(GLint, GLfloat, GLfloat);
typedef void (*PFNGLUNIFORMMATRIX3FVPROC)(GLint, GLsizei, GLboolean,
                                          const GLfloat *);
typedef void (*PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void (*PFNGLVERTEXATTRIBPOINTERPROC)(GLuint, GLint, GLenum, GLboolean,
                                             GLsizei, const void *);
typedef void (*PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint *);
typedef void (*PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void (*PFNGLDRAWARRAYSINSTANCEDPROC)(GLenum, GLint, GLsizei, GLsizei);
typedef void (*PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC)(GLenum, GLint, GLsizei,
                                                         GLsizei, GLuint);
typedef void (*PFNGLGENFRAMEBUFFERSPROC)(GLsizei, GLuint *);
typedef void (*PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei, const GLuint *);
typedef void (*PFNGLBINDFRAMEBUFFERPROC)(GLenum, GLuint);
typedef GLenum (*PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum);
typedef void (*PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum, GLenum, GLenum, GLuint,
                                              GLint);
typedef void (*PFNGLDELETEBUFFERSPROC)(GLsizei, const GLuint *);
typedef void (*PFNGLDELETEPROGRAMPROC)(GLuint);
typedef void (*PFNGLACTIVETEXTUREPROC)(GLenum);
#endif

#if !defined(_WIN32)
#define glGenBuffers stygian_glGenBuffers
#define glBindBuffer stygian_glBindBuffer
#define glBufferData stygian_glBufferData
#define glBufferSubData stygian_glBufferSubData
#define glBindBufferBase stygian_glBindBufferBase
#define glCreateShader stygian_glCreateShader
#define glShaderSource stygian_glShaderSource
#define glCompileShader stygian_glCompileShader
#define glGetShaderiv stygian_glGetShaderiv
#define glGetShaderInfoLog stygian_glGetShaderInfoLog
#define glCreateProgram stygian_glCreateProgram
#define glAttachShader stygian_glAttachShader
#define glLinkProgram stygian_glLinkProgram
#define glUseProgram stygian_glUseProgram
#define glGetProgramiv stygian_glGetProgramiv
#define glGetProgramInfoLog stygian_glGetProgramInfoLog
#define glGetUniformLocation stygian_glGetUniformLocation
#define glUniform1i stygian_glUniform1i
#define glUniform1iv stygian_glUniform1iv
#define glUniform1f stygian_glUniform1f
#define glUniform2f stygian_glUniform2f
#define glUniformMatrix3fv stygian_glUniformMatrix3fv
#define glEnableVertexAttribArray stygian_glEnableVertexAttribArray
#define glVertexAttribPointer stygian_glVertexAttribPointer
#define glGenVertexArrays stygian_glGenVertexArrays
#define glBindVertexArray stygian_glBindVertexArray
#define glDrawArraysInstanced stygian_glDrawArraysInstanced
#define glDrawArraysInstancedBaseInstance stygian_glDrawArraysInstancedBaseInstance
#define glGenFramebuffers stygian_glGenFramebuffers
#define glDeleteFramebuffers stygian_glDeleteFramebuffers
#define glBindFramebuffer stygian_glBindFramebuffer
#define glCheckFramebufferStatus stygian_glCheckFramebufferStatus
#define glFramebufferTexture2D stygian_glFramebufferTexture2D
#define glDeleteBuffers stygian_glDeleteBuffers
#define glDeleteProgram stygian_glDeleteProgram
#define glActiveTexture stygian_glActiveTexture
#define glDeleteShader stygian_glDeleteShader
#define glValidateProgram stygian_glValidateProgram
#define glGenQueries stygian_glGenQueries
#define glDeleteQueries stygian_glDeleteQueries
#define glBeginQuery stygian_glBeginQuery
#define glEndQuery stygian_glEndQuery
#define glGetQueryObjectiv stygian_glGetQueryObjectiv
#define glGetQueryObjectui64v stygian_glGetQueryObjectui64v
#define glFenceSync stygian_glFenceSync
#define glDeleteSync stygian_glDeleteSync
#define glClientWaitSync stygian_glClientWaitSync
#define glMapBufferRange stygian_glMapBufferRange
#define glUnmapBuffer stygian_glUnmapBuffer
#endif

static PFNGLGENBUFFERSPROC glGenBuffers;
static PFNGLBINDBUFFERPROC glBindBuffer;
static PFNGLBUFFERDATAPROC glBufferData;
static PFNGLBUFFERSUBDATAPROC glBufferSubData;
static PFNGLBINDBUFFERBASEPROC glBindBufferBase;
static PFNGLCREATESHADERPROC glCreateShader;
static PFNGLSHADERSOURCEPROC glShaderSource;
static PFNGLCOMPILESHADERPROC glCompileShader;
static PFNGLGETSHADERIVPROC glGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
static PFNGLCREATEPROGRAMPROC glCreateProgram;
static PFNGLATTACHSHADERPROC glAttachShader;
static PFNGLLINKPROGRAMPROC glLinkProgram;
static PFNGLUSEPROGRAMPROC glUseProgram;
static PFNGLGETPROGRAMIVPROC glGetProgramiv;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
static PFNGLUNIFORM1IPROC glUniform1i;
static PFNGLUNIFORM1IVPROC glUniform1iv;
static PFNGLUNIFORM1FPROC glUniform1f;
static PFNGLUNIFORM2FPROC glUniform2f;
static PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
static PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
static PFNGLDRAWARRAYSINSTANCEDPROC glDrawArraysInstanced;
static StygianPFNGLDrawArraysInstancedBaseInstanceProc
    glDrawArraysInstancedBaseInstance;
static PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
static PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
static PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
static PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
static PFNGLDELETEBUFFERSPROC glDeleteBuffers;
static PFNGLDELETEPROGRAMPROC glDeleteProgram;
static StygianPFNGLActiveTextureProc glActiveTexture;

// Shader cleanup and validation
#ifdef _WIN32
typedef void (*PFNGLDELETESHADERPROC)(GLuint);
typedef void (*PFNGLVALIDATEPROGRAMPROC)(GLuint);
typedef void (*PFNGLGENQUERIESPROC)(GLsizei, GLuint *);
typedef void (*PFNGLDELETEQUERIESPROC)(GLsizei, const GLuint *);
typedef void (*PFNGLBEGINQUERYPROC)(GLenum, GLuint);
typedef void (*PFNGLENDQUERYPROC)(GLenum);
typedef void (*PFNGLGETQUERYOBJECTIVPROC)(GLuint, GLenum, GLint *);
typedef void (*PFNGLGETQUERYOBJECTUI64VPROC)(GLuint, GLenum, uint64_t *);
typedef GLsync (*PFNGLFENCESYNCPROC)(GLenum, GLbitfield);
typedef void (*PFNGLDELETESYNCPROC)(GLsync);
typedef GLenum (*PFNGLCLIENTWAITSYNCPROC)(GLsync, GLbitfield, GLuint64);
typedef void *(*PFNGLMAPBUFFERRANGEPROC)(GLenum, GLintptr, GLsizeiptr,
                                         GLbitfield);
typedef GLboolean (*PFNGLUNMAPBUFFERPROC)(GLenum);
#endif
static PFNGLDELETESHADERPROC glDeleteShader;
static PFNGLVALIDATEPROGRAMPROC glValidateProgram;
static PFNGLGENQUERIESPROC glGenQueries;
static PFNGLDELETEQUERIESPROC glDeleteQueries;
static PFNGLBEGINQUERYPROC glBeginQuery;
static PFNGLENDQUERYPROC glEndQuery;
static PFNGLGETQUERYOBJECTIVPROC glGetQueryObjectiv;
static PFNGLGETQUERYOBJECTUI64VPROC glGetQueryObjectui64v;
static PFNGLFENCESYNCPROC glFenceSync;
static PFNGLDELETESYNCPROC glDeleteSync;
static PFNGLCLIENTWAITSYNCPROC glClientWaitSync;
static PFNGLMAPBUFFERRANGEPROC glMapBufferRange;
static PFNGLUNMAPBUFFERPROC glUnmapBuffer;

static void load_gl(void **ptr, const char *name) {
  *ptr = stygian_window_gl_get_proc_address(name);
}
#define LOAD_GL(fn) load_gl((void **)&fn, #fn)

// ============================================================================
// Access Point Structure
// ============================================================================

struct StygianAP {
  StygianWindow *window;
  uint32_t max_elements;
  StygianAllocator *allocator;

  void *gl_context;

  // GPU resources
  GLuint clip_ssbo;
  GLuint vao;
  GLuint vbo;
  GLuint program;

  // Uniform locations
  GLint loc_screen_size;
  GLint loc_font_tex;
  GLint loc_image_tex;
  GLint loc_atlas_size;
  GLint loc_px_range;
  GLint loc_output_transform_enabled;
  GLint loc_output_matrix;
  GLint loc_output_src_srgb;
  GLint loc_output_src_gamma;
  GLint loc_output_dst_srgb;
  GLint loc_output_dst_gamma;
  int cached_screen_w;
  int cached_screen_h;
  GLuint current_program;
  GLuint current_vao;
  GLuint current_shader_storage_buffer;
  GLuint current_ssbo_bindings[8];
  bool sampler_uniforms_dirty;
  bool output_uniforms_dirty;
  GLuint bound_image_textures[16];
  uint32_t cached_clip_count;
  float cached_clips[STYGIAN_MAX_CLIPS * 4];

  // State
  uint32_t element_count;
  bool initialized;
  StygianAPAdapterClass adapter_class;
  bool present_enabled;
  bool output_color_transform_enabled;
  float output_color_matrix[9];
  bool output_src_srgb_transfer;
  float output_src_gamma;
  bool output_dst_srgb_transfer;
  float output_dst_gamma;

  // Shader paths for hot reload
  char shader_dir[256];

  // Shader file modification times for auto-reload
  uint64_t shader_load_time; // Time when shaders were last loaded

  uint32_t last_upload_bytes;
  uint32_t last_upload_ranges;
  float last_gpu_ms;
  GLuint gpu_queries[8];
  uint8_t gpu_query_index;
  bool gpu_query_initialized;
  bool gpu_query_in_flight;
  bool gpu_query_pending[8];

  // Raw/no-present frames draw here so window-system pacing stays out.
  bool raw_target_supported;
  GLuint raw_fbo;
  GLuint raw_color_tex;
  int raw_target_width;
  int raw_target_height;

  // SoA SSBOs (4 buffers: hot, appearance, effects, transform)
  GLuint soa_ssbo_hot;
  GLuint soa_ssbo_appearance;
  GLuint soa_ssbo_effects;
  GLuint soa_ssbo_transform;
  // GPU-side version tracking per chunk
  uint32_t *gpu_hot_versions;
  uint32_t *gpu_appearance_versions;
  uint32_t *gpu_effects_versions;
  uint32_t *gpu_transform_versions;
  uint32_t soa_chunk_count;

  // Remapped hot stream submitted to GPU (texture handles -> sampler slots).
  StygianSoAHot *submit_hot;
  const StygianSoAHot *submit_hot_src;

  // Capture readback ring (PBO)
  bool capture_supported;
  bool capture_active;
  int capture_width;
  int capture_height;
  uint32_t capture_stride_bytes;
  uint32_t capture_frame_bytes;
  GLuint capture_pbos[4];
  GLsync capture_fences[4];
  bool capture_in_flight[4];
  uint64_t capture_slot_frame_index[4];
  uint8_t capture_write_slot;
  uint8_t capture_read_slot;
  uint64_t capture_frame_counter;
};

#define STYGIAN_GL_IMAGE_SAMPLERS 8
#define STYGIAN_GL_IMAGE_UNIT_BASE 2 // units 0,1 reserved for font atlas etc.
#define STYGIAN_GL_CAPTURE_PBO_SLOTS 4
#define STYGIAN_GL_GPU_QUERY_SLOTS 8

// Safe string copy: deterministic, no printf overhead, always NUL-terminates.
// copy_cstr removed — use stygian_cpystr from stygian_internal.h

// Allocator helpers: use AP allocator when set, else CRT (bootstrap/fallback)
static void *ap_alloc(StygianAP *ap, size_t size, size_t alignment) {
  if (ap->allocator && ap->allocator->alloc)
    return ap->allocator->alloc(ap->allocator, size, alignment);
  (void)alignment;
  return malloc(size);
}
static void ap_free(StygianAP *ap, void *ptr) {
  if (!ptr)
    return;
  if (ap->allocator && ap->allocator->free)
    ap->allocator->free(ap->allocator, ptr);
  else
    free(ptr);
}

// Config-based allocator helpers for bootstrap (before AP struct exists)
static void *cfg_alloc(StygianAllocator *allocator, size_t size,
                       size_t alignment) {
  if (allocator && allocator->alloc)
    return allocator->alloc(allocator, size, alignment);
  (void)alignment;
  return malloc(size);
}
static void cfg_free(StygianAllocator *allocator, void *ptr) {
  if (!ptr)
    return;
  if (allocator && allocator->free)
    allocator->free(allocator, ptr);
  else
    free(ptr);
}

static void stygian_ap_raw_target_reset(StygianAP *ap) {
  if (!ap)
    return;
  ap->raw_fbo = 0u;
  ap->raw_color_tex = 0u;
  ap->raw_target_width = 0;
  ap->raw_target_height = 0;
}

static void stygian_ap_raw_target_release(StygianAP *ap) {
  if (!ap)
    return;
  if (ap->raw_fbo && glDeleteFramebuffers) {
    glDeleteFramebuffers(1, &ap->raw_fbo);
    ap->raw_fbo = 0u;
  }
  if (ap->raw_color_tex) {
    glDeleteTextures(1, &ap->raw_color_tex);
    ap->raw_color_tex = 0u;
  }
  ap->raw_target_width = 0;
  ap->raw_target_height = 0;
}

static bool stygian_ap_raw_target_configure(StygianAP *ap, int width,
                                            int height) {
  GLenum status = 0u;
  if (!ap || !ap->raw_target_supported || width <= 0 || height <= 0)
    return false;
  if (ap->raw_fbo && ap->raw_color_tex && ap->raw_target_width == width &&
      ap->raw_target_height == height) {
    return true;
  }

  stygian_ap_raw_target_release(ap);

  glGenFramebuffers(1, &ap->raw_fbo);
  glGenTextures(1, &ap->raw_color_tex);
  if (!ap->raw_fbo || !ap->raw_color_tex) {
    stygian_ap_raw_target_release(ap);
    return false;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, ap->raw_fbo);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, ap->raw_color_tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, NULL);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         ap->raw_color_tex, 0);
  status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    // don't keep a half-baked target around; the default framebuffer is safer
    stygian_ap_raw_target_release(ap);
    return false;
  }

  ap->raw_target_width = width;
  ap->raw_target_height = height;
  return true;
}

static void stygian_ap_bind_render_target(StygianAP *ap, int width,
                                          int height) {
  if (!ap || !glBindFramebuffer)
    return;
  if (!ap->present_enabled && ap->raw_target_supported &&
      stygian_ap_raw_target_configure(ap, width, height)) {
    glBindFramebuffer(GL_FRAMEBUFFER, ap->raw_fbo);
    return;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void stygian_ap_capture_reset_state(StygianAP *ap) {
  if (!ap)
    return;
  ap->capture_width = 0;
  ap->capture_height = 0;
  ap->capture_stride_bytes = 0u;
  ap->capture_frame_bytes = 0u;
  ap->capture_write_slot = 0u;
  ap->capture_read_slot = 0u;
  ap->capture_frame_counter = 0u;
  for (uint32_t i = 0; i < STYGIAN_GL_CAPTURE_PBO_SLOTS; ++i) {
    ap->capture_pbos[i] = 0u;
    ap->capture_fences[i] = NULL;
    ap->capture_in_flight[i] = false;
    ap->capture_slot_frame_index[i] = 0u;
  }
}

static void stygian_ap_capture_release(StygianAP *ap) {
  if (!ap)
    return;
  for (uint32_t i = 0; i < STYGIAN_GL_CAPTURE_PBO_SLOTS; ++i) {
    if (ap->capture_fences[i] && glDeleteSync) {
      glDeleteSync(ap->capture_fences[i]);
      ap->capture_fences[i] = NULL;
    }
    ap->capture_in_flight[i] = false;
    ap->capture_slot_frame_index[i] = 0u;
  }
  if (glDeleteBuffers) {
    for (uint32_t i = 0; i < STYGIAN_GL_CAPTURE_PBO_SLOTS; ++i) {
      if (ap->capture_pbos[i]) {
        glDeleteBuffers(1, &ap->capture_pbos[i]);
        ap->capture_pbos[i] = 0u;
      }
    }
  }
  ap->capture_active = false;
  ap->capture_width = 0;
  ap->capture_height = 0;
  ap->capture_stride_bytes = 0u;
  ap->capture_frame_bytes = 0u;
  ap->capture_write_slot = 0u;
  ap->capture_read_slot = 0u;
}

static bool stygian_ap_capture_configure(StygianAP *ap, int width, int height) {
  if (!ap || width <= 0 || height <= 0 || !glGenBuffers || !glBindBuffer ||
      !glBufferData)
    return false;

  uint32_t stride = (uint32_t)width * 4u;
  uint32_t bytes = stride * (uint32_t)height;
  if (bytes == 0u)
    return false;

  stygian_ap_capture_release(ap);
  glGenBuffers(STYGIAN_GL_CAPTURE_PBO_SLOTS, ap->capture_pbos);
  for (uint32_t i = 0; i < STYGIAN_GL_CAPTURE_PBO_SLOTS; ++i) {
    if (!ap->capture_pbos[i]) {
      stygian_ap_capture_release(ap);
      return false;
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, ap->capture_pbos[i]);
    glBufferData(GL_PIXEL_PACK_BUFFER, (GLsizeiptr)bytes, NULL, GL_STREAM_READ);
    ap->capture_in_flight[i] = false;
    ap->capture_slot_frame_index[i] = 0u;
    ap->capture_fences[i] = NULL;
  }
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

  ap->capture_width = width;
  ap->capture_height = height;
  ap->capture_stride_bytes = stride;
  ap->capture_frame_bytes = bytes;
  ap->capture_write_slot = 0u;
  ap->capture_read_slot = 0u;
  ap->capture_frame_counter = 0u;
  return true;
}

static bool contains_nocase(const char *haystack, const char *needle) {
  size_t nlen;
  const char *p;
  if (!haystack || !needle)
    return false;
  nlen = strlen(needle);
  if (nlen == 0)
    return false;
  for (p = haystack; *p; p++) {
#ifdef _WIN32
    if (_strnicmp(p, needle, nlen) == 0)
      return true;
#else
    if (strncasecmp(p, needle, nlen) == 0)
      return true;
#endif
  }
  return false;
}

static StygianAPAdapterClass classify_renderer(const char *renderer) {
  if (!renderer || !renderer[0])
    return STYGIAN_AP_ADAPTER_UNKNOWN;
  if (contains_nocase(renderer, "intel") || contains_nocase(renderer, "iris") ||
      contains_nocase(renderer, "uhd")) {
    return STYGIAN_AP_ADAPTER_IGPU;
  }
  if (contains_nocase(renderer, "nvidia") ||
      contains_nocase(renderer, "geforce") ||
      contains_nocase(renderer, "radeon") || contains_nocase(renderer, "rtx") ||
      contains_nocase(renderer, "gtx")) {
    return STYGIAN_AP_ADAPTER_DGPU;
  }
  return STYGIAN_AP_ADAPTER_UNKNOWN;
}

// ============================================================================
// File Modification Time (for shader hot-reload)
// ============================================================================

// Portable file modification time
#ifdef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// Get file modification time as uint64 (0 on error)
static uint64_t get_file_mod_time(const char *path) {
#ifdef _WIN32
  struct _stat st;
  if (_stat(path, &st) == 0) {
    return (uint64_t)st.st_mtime;
  }
#else
  struct stat st;
  if (stat(path, &st) == 0) {
    return (uint64_t)st.st_mtime;
  }
#endif
  return 0;
}

// Get newest modification time of all shader files
static uint64_t get_shader_newest_mod_time(const char *shader_dir) {
  static const char *shader_files[] = {"stygian.vert",    "stygian.frag",
                                       "sdf_common.glsl", "window.glsl",
                                       "ui.glsl",         "text.glsl"};

  uint64_t newest = 0;
  char path[512];

  for (int i = 0; i < 6; i++) {
    snprintf(path, sizeof(path), "%s/%s", shader_dir, shader_files[i]);
    uint64_t mod_time = get_file_mod_time(path);
    if (mod_time > newest)
      newest = mod_time;
  }

  return newest;
}

// ============================================================================
// Shader Compilation
// ============================================================================

static GLuint compile_shader(GLenum type, const char *source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (!status) {
    char log[1024];
    glGetShaderInfoLog(shader, sizeof(log), NULL, log);
    printf("[Stygian AP] Shader compile error:\n%s\n", log);
    return 0;
  }
  return shader;
}

// Load preprocessed shader file from build/ subdirectory
// Shaders are preprocessed by shaderc (glslc -E) to resolve #includes
static char *load_shader_file(StygianAP *ap, const char *filename) {
  char path[512];
  snprintf(path, sizeof(path), "%s/build/%s.glsl", ap->shader_dir, filename);

  FILE *f = fopen(path, "rb");
  if (!f) {
    printf("[Stygian AP] Shader not found at '%s', trying fallback...\n", path);
    snprintf(path, sizeof(path), "%s/%s", ap->shader_dir, filename);
    f = fopen(path, "rb");
  }
  if (!f) {
    printf("[Stygian AP] Failed to load shader '%s'\n", path);
    return NULL;
  }
  printf("[Stygian AP] Loaded shader: %s\n", path);

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *source = (char *)ap_alloc(ap, (size_t)size + 1u, 1);
  if (!source) {
    fclose(f);
    return NULL;
  }

  fread(source, 1, (size_t)size, f);
  source[size] = '\0';
  fclose(f);

  return source;
}

#if 0
static GLuint compile_program_source_pair(const char *tag, const char *vert_src,
                                          const char *frag_src) {
  GLuint vs = compile_shader(GL_VERTEX_SHADER, vert_src);
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, frag_src);
  GLuint program;
  GLint status;

  if (!vs || !fs) {
    if (vs)
      glDeleteShader(vs);
    if (fs)
      glDeleteShader(fs);
    fprintf(stderr, "[Stygian AP] %s compile failed\n", tag);
    fflush(stderr);
    return 0;
  }

  program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);
  glDeleteShader(vs);
  glDeleteShader(fs);

  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (!status) {
    char log[4096];
    glGetProgramInfoLog(program, sizeof(log), NULL, log);
    fprintf(stderr, "[Stygian AP] %s link error:\n%s\n", tag, log);
    fflush(stderr);
    glDeleteProgram(program);
    return 0;
  }

  fprintf(stderr, "[Stygian AP] %s link ok\n", tag);
  fflush(stderr);
  return program;
}

static void stygian_ap_run_link_smoke_tests(void) {
  static const char *k_vert_min =
      "#version 430 core\n"
      "layout(location = 0) in vec2 aPos;\n"
      "layout(location = 0) flat out vec4 vColor;\n"
      "void main() {\n"
      "  gl_Position = vec4(aPos, 0.0, 1.0);\n"
      "  vColor = vec4(1.0);\n"
      "}\n";
  static const char *k_frag_min =
      "#version 430 core\n"
      "layout(location = 0) flat in vec4 vColor;\n"
      "layout(location = 0) out vec4 fragColor;\n"
      "void main() {\n"
      "  fragColor = vColor;\n"
      "}\n";
  static const char *k_vert_iface =
      "#version 430 core\n"
      "layout(location = 0) in vec2 aPos;\n"
      "layout(location = 0) flat out vec4 vColor;\n"
      "layout(location = 1) flat out vec4 vBorderColor;\n"
      "layout(location = 2) flat out vec4 vRadius;\n"
      "layout(location = 3) flat out vec4 vUV;\n"
      "layout(location = 4) flat out uint vType;\n"
      "layout(location = 5) flat out float vBlend;\n"
      "layout(location = 6) flat out float vHover;\n"
      "layout(location = 7) out vec2 vLocalPos;\n"
      "layout(location = 8) out vec2 vSize;\n"
      "layout(location = 9) flat out uint vInstanceID;\n"
      "layout(location = 10) flat out uint vTextureID;\n"
      "layout(location = 11) flat out vec4 vReserved0;\n"
      "layout(location = 12) out vec2 vWorldPos;\n"
      "layout(location = 13) flat out uint vClipID;\n"
      "layout(location = 14) flat out vec4 vClipRect;\n"
      "layout(location = 15) flat out float vGlowIntensity;\n"
      "void main() {\n"
      "  gl_Position = vec4(aPos, 0.0, 1.0);\n"
      "  vColor = vec4(1.0);\n"
      "  vBorderColor = vec4(0.0);\n"
      "  vRadius = vec4(4.0);\n"
      "  vUV = vec4(0.0, 0.0, 1.0, 1.0);\n"
      "  vType = 0u;\n"
      "  vBlend = 1.0;\n"
      "  vHover = 0.0;\n"
      "  vLocalPos = aPos;\n"
      "  vSize = vec2(32.0, 32.0);\n"
      "  vInstanceID = 0u;\n"
      "  vTextureID = 0u;\n"
      "  vReserved0 = vec4(0.0);\n"
      "  vWorldPos = aPos;\n"
      "  vClipID = 0u;\n"
      "  vClipRect = vec4(0.0);\n"
      "  vGlowIntensity = 0.0;\n"
      "}\n";
  static const char *k_vert_resources =
      "#version 430 core\n"
      "layout(location = 0) in vec2 aPos;\n"
      "struct SoAHot { float x, y, w, h; vec4 color; uint texture_id; uint type; uint flags; float z; };\n"
      "struct SoAAppearance { vec4 border_color; vec4 radius; vec4 uv; vec4 control_points; };\n"
      "struct SoAEffects { vec2 shadow_offset; float shadow_blur; float shadow_spread; vec4 shadow_color; vec4 gradient_start; vec4 gradient_end; float hover; float blend; float gradient_angle; float blur_radius; float glow_intensity; uint parent_id; vec2 _pad; };\n"
      "struct SoATransform { vec4 row0; vec4 row1; };\n"
      "layout(std430, binding = 4) readonly buffer SoAHotBuffer { SoAHot soa_hot[]; };\n"
      "layout(std430, binding = 3) readonly buffer ClipBuffer { vec4 clip_rects[]; };\n"
      "layout(std430, binding = 5) readonly buffer SoAAppearanceBuffer { SoAAppearance soa_appearance[]; };\n"
      "layout(std430, binding = 6) readonly buffer SoAEffectsBuffer { SoAEffects soa_effects[]; };\n"
      "layout(std430, binding = 7) readonly buffer SoATransformBuffer { SoATransform soa_transform[]; };\n"
      "uniform vec2 uScreenSize;\n"
      "layout(location = 0) flat out vec4 vColor;\n"
      "layout(location = 1) flat out vec4 vBorderColor;\n"
      "layout(location = 2) flat out vec4 vRadius;\n"
      "layout(location = 3) flat out vec4 vUV;\n"
      "layout(location = 4) flat out uint vType;\n"
      "layout(location = 5) flat out float vBlend;\n"
      "layout(location = 6) flat out float vHover;\n"
      "layout(location = 7) out vec2 vLocalPos;\n"
      "layout(location = 8) out vec2 vSize;\n"
      "layout(location = 9) flat out uint vInstanceID;\n"
      "layout(location = 10) flat out uint vTextureID;\n"
      "layout(location = 11) flat out vec4 vReserved0;\n"
      "layout(location = 12) out vec2 vWorldPos;\n"
      "layout(location = 13) flat out uint vClipID;\n"
      "layout(location = 14) flat out vec4 vClipRect;\n"
      "layout(location = 15) flat out float vGlowIntensity;\n"
      "void main() {\n"
      "  SoAHot h = soa_hot[gl_InstanceID];\n"
      "  SoAAppearance a = soa_appearance[gl_InstanceID];\n"
      "  SoAEffects fx = soa_effects[gl_InstanceID];\n"
      "  SoATransform t = soa_transform[gl_InstanceID];\n"
      "  vec2 uv01 = aPos * 0.5 + 0.5;\n"
      "  vec2 size = vec2(h.w, h.h);\n"
      "  vec2 localPos = vec2(uv01.x, 1.0 - uv01.y) * size;\n"
      "  vec2 elementPos = vec2(h.x, h.y) + localPos;\n"
      "  vec2 pixelPos = vec2(t.row0.x * elementPos.x + t.row0.y * elementPos.y + t.row0.z,\n"
      "                      t.row1.x * elementPos.x + t.row1.y * elementPos.y + t.row1.z);\n"
      "  vec2 ndc = (pixelPos / uScreenSize) * 2.0 - 1.0;\n"
      "  ndc.y = -ndc.y;\n"
      "  gl_Position = vec4(ndc, h.z, 1.0);\n"
      "  vColor = h.color;\n"
      "  vBorderColor = a.border_color;\n"
      "  vRadius = a.radius;\n"
      "  vUV = a.uv;\n"
      "  vType = h.type;\n"
      "  vBlend = fx.blend;\n"
      "  vHover = fx.hover;\n"
      "  vLocalPos = localPos;\n"
      "  vSize = size;\n"
      "  vInstanceID = gl_InstanceID;\n"
      "  vTextureID = h.texture_id;\n"
      "  vReserved0 = a.control_points;\n"
      "  vWorldPos = pixelPos;\n"
      "  vClipID = (h.flags & 0x0000FF00u) >> 8u;\n"
      "  vClipRect = (vClipID != 0u) ? clip_rects[vClipID] : vec4(0.0);\n"
      "  vGlowIntensity = fx.glow_intensity;\n"
      "}\n";
  static const char *k_vert_hot_only =
      "#version 430 core\n"
      "layout(location = 0) in vec2 aPos;\n"
      "struct SoAHot { float x, y, w, h; vec4 color; uint texture_id; uint type; uint flags; float z; };\n"
      "layout(std430, binding = 4) readonly buffer SoAHotBuffer { SoAHot soa_hot[]; };\n"
      "uniform vec2 uScreenSize;\n"
      "layout(location = 0) flat out vec4 vColor;\n"
      "layout(location = 1) flat out vec4 vBorderColor;\n"
      "layout(location = 2) flat out vec4 vRadius;\n"
      "layout(location = 3) flat out vec4 vUV;\n"
      "layout(location = 4) flat out uint vType;\n"
      "layout(location = 5) flat out float vBlend;\n"
      "layout(location = 6) flat out float vHover;\n"
      "layout(location = 7) out vec2 vLocalPos;\n"
      "layout(location = 8) out vec2 vSize;\n"
      "layout(location = 9) flat out uint vInstanceID;\n"
      "layout(location = 10) flat out uint vTextureID;\n"
      "layout(location = 11) flat out vec4 vReserved0;\n"
      "layout(location = 12) out vec2 vWorldPos;\n"
      "layout(location = 13) flat out uint vClipID;\n"
      "layout(location = 14) flat out vec4 vClipRect;\n"
      "layout(location = 15) flat out float vGlowIntensity;\n"
      "void main() {\n"
      "  SoAHot h = soa_hot[gl_InstanceID];\n"
      "  vec2 uv01 = aPos * 0.5 + 0.5;\n"
      "  vec2 size = vec2(max(h.w, 1.0), max(h.h, 1.0));\n"
      "  vec2 localPos = vec2(uv01.x, 1.0 - uv01.y) * size;\n"
      "  vec2 pixelPos = vec2(h.x, h.y) + localPos;\n"
      "  vec2 ndc = (pixelPos / uScreenSize) * 2.0 - 1.0;\n"
      "  ndc.y = -ndc.y;\n"
      "  gl_Position = vec4(ndc, h.z, 1.0);\n"
      "  vColor = h.color;\n"
      "  vBorderColor = vec4(0.0);\n"
      "  vRadius = vec4(0.0);\n"
      "  vUV = vec4(0.0, 0.0, 1.0, 1.0);\n"
      "  vType = h.type;\n"
      "  vBlend = 1.0;\n"
      "  vHover = 0.0;\n"
      "  vLocalPos = localPos;\n"
      "  vSize = size;\n"
      "  vInstanceID = gl_InstanceID;\n"
      "  vTextureID = h.texture_id;\n"
      "  vReserved0 = vec4(0.0);\n"
      "  vWorldPos = pixelPos;\n"
      "  vClipID = 0u;\n"
      "  vClipRect = vec4(0.0);\n"
      "  vGlowIntensity = 0.0;\n"
      "}\n";
  static const char *k_vert_hot_appearance =
      "#version 430 core\n"
      "layout(location = 0) in vec2 aPos;\n"
      "struct SoAHot { float x, y, w, h; vec4 color; uint texture_id; uint type; uint flags; float z; };\n"
      "struct SoAAppearance { vec4 border_color; vec4 radius; vec4 uv; vec4 control_points; };\n"
      "layout(std430, binding = 4) readonly buffer SoAHotBuffer { SoAHot soa_hot[]; };\n"
      "layout(std430, binding = 5) readonly buffer SoAAppearanceBuffer { SoAAppearance soa_appearance[]; };\n"
      "uniform vec2 uScreenSize;\n"
      "layout(location = 0) flat out vec4 vColor;\n"
      "layout(location = 1) flat out vec4 vBorderColor;\n"
      "layout(location = 2) flat out vec4 vRadius;\n"
      "layout(location = 3) flat out vec4 vUV;\n"
      "layout(location = 4) flat out uint vType;\n"
      "layout(location = 5) flat out float vBlend;\n"
      "layout(location = 6) flat out float vHover;\n"
      "layout(location = 7) out vec2 vLocalPos;\n"
      "layout(location = 8) out vec2 vSize;\n"
      "layout(location = 9) flat out uint vInstanceID;\n"
      "layout(location = 10) flat out uint vTextureID;\n"
      "layout(location = 11) flat out vec4 vReserved0;\n"
      "layout(location = 12) out vec2 vWorldPos;\n"
      "layout(location = 13) flat out uint vClipID;\n"
      "layout(location = 14) flat out vec4 vClipRect;\n"
      "layout(location = 15) flat out float vGlowIntensity;\n"
      "void main() {\n"
      "  SoAHot h = soa_hot[gl_InstanceID];\n"
      "  SoAAppearance a = soa_appearance[gl_InstanceID];\n"
      "  vec2 uv01 = aPos * 0.5 + 0.5;\n"
      "  vec2 size = vec2(max(h.w, 1.0), max(h.h, 1.0));\n"
      "  vec2 localPos = vec2(uv01.x, 1.0 - uv01.y) * size;\n"
      "  vec2 pixelPos = vec2(h.x, h.y) + localPos;\n"
      "  vec2 ndc = (pixelPos / uScreenSize) * 2.0 - 1.0;\n"
      "  ndc.y = -ndc.y;\n"
      "  gl_Position = vec4(ndc, h.z, 1.0);\n"
      "  vColor = h.color;\n"
      "  vBorderColor = a.border_color;\n"
      "  vRadius = a.radius;\n"
      "  vUV = a.uv;\n"
      "  vType = h.type;\n"
      "  vBlend = 1.0;\n"
      "  vHover = 0.0;\n"
      "  vLocalPos = localPos;\n"
      "  vSize = size;\n"
      "  vInstanceID = gl_InstanceID;\n"
      "  vTextureID = h.texture_id;\n"
      "  vReserved0 = a.control_points;\n"
      "  vWorldPos = pixelPos;\n"
      "  vClipID = 0u;\n"
      "  vClipRect = vec4(0.0);\n"
      "  vGlowIntensity = 0.0;\n"
      "}\n";
  static const char *k_vert_hot_appearance_clip =
      "#version 430 core\n"
      "layout(location = 0) in vec2 aPos;\n"
      "struct SoAHot { float x, y, w, h; vec4 color; uint texture_id; uint type; uint flags; float z; };\n"
      "struct SoAAppearance { vec4 border_color; vec4 radius; vec4 uv; vec4 control_points; };\n"
      "layout(std430, binding = 3) readonly buffer ClipBuffer { vec4 clip_rects[]; };\n"
      "layout(std430, binding = 4) readonly buffer SoAHotBuffer { SoAHot soa_hot[]; };\n"
      "layout(std430, binding = 5) readonly buffer SoAAppearanceBuffer { SoAAppearance soa_appearance[]; };\n"
      "uniform vec2 uScreenSize;\n"
      "layout(location = 0) flat out vec4 vColor;\n"
      "layout(location = 1) flat out vec4 vBorderColor;\n"
      "layout(location = 2) flat out vec4 vRadius;\n"
      "layout(location = 3) flat out vec4 vUV;\n"
      "layout(location = 4) flat out uint vType;\n"
      "layout(location = 5) flat out float vBlend;\n"
      "layout(location = 6) flat out float vHover;\n"
      "layout(location = 7) out vec2 vLocalPos;\n"
      "layout(location = 8) out vec2 vSize;\n"
      "layout(location = 9) flat out uint vInstanceID;\n"
      "layout(location = 10) flat out uint vTextureID;\n"
      "layout(location = 11) flat out vec4 vReserved0;\n"
      "layout(location = 12) out vec2 vWorldPos;\n"
      "layout(location = 13) flat out uint vClipID;\n"
      "layout(location = 14) flat out vec4 vClipRect;\n"
      "layout(location = 15) flat out float vGlowIntensity;\n"
      "void main() {\n"
      "  SoAHot h = soa_hot[gl_InstanceID];\n"
      "  SoAAppearance a = soa_appearance[gl_InstanceID];\n"
      "  vec2 uv01 = aPos * 0.5 + 0.5;\n"
      "  vec2 size = vec2(max(h.w, 1.0), max(h.h, 1.0));\n"
      "  vec2 localPos = vec2(uv01.x, 1.0 - uv01.y) * size;\n"
      "  vec2 pixelPos = vec2(h.x, h.y) + localPos;\n"
      "  vec2 ndc = (pixelPos / uScreenSize) * 2.0 - 1.0;\n"
      "  ndc.y = -ndc.y;\n"
      "  gl_Position = vec4(ndc, h.z, 1.0);\n"
      "  vColor = h.color;\n"
      "  vBorderColor = a.border_color;\n"
      "  vRadius = a.radius;\n"
      "  vUV = a.uv;\n"
      "  vType = h.type;\n"
      "  vBlend = 1.0;\n"
      "  vHover = 0.0;\n"
      "  vLocalPos = localPos;\n"
      "  vSize = size;\n"
      "  vInstanceID = gl_InstanceID;\n"
      "  vTextureID = h.texture_id;\n"
      "  vReserved0 = a.control_points;\n"
      "  vWorldPos = pixelPos;\n"
      "  vClipID = (h.flags & 0x0000FF00u) >> 8u;\n"
      "  vClipRect = (vClipID != 0u && clip_rects.length() > int(vClipID)) ? clip_rects[vClipID] : vec4(0.0);\n"
      "  vGlowIntensity = 0.0;\n"
      "}\n";
  static const char *k_vert_hot_appearance_effects =
      "#version 430 core\n"
      "layout(location = 0) in vec2 aPos;\n"
      "struct SoAHot { float x, y, w, h; vec4 color; uint texture_id; uint type; uint flags; float z; };\n"
      "struct SoAAppearance { vec4 border_color; vec4 radius; vec4 uv; vec4 control_points; };\n"
      "struct SoAEffects { vec2 shadow_offset; float shadow_blur; float shadow_spread; vec4 shadow_color; vec4 gradient_start; vec4 gradient_end; float hover; float blend; float gradient_angle; float blur_radius; float glow_intensity; uint parent_id; vec2 _pad; };\n"
      "layout(std430, binding = 4) readonly buffer SoAHotBuffer { SoAHot soa_hot[]; };\n"
      "layout(std430, binding = 5) readonly buffer SoAAppearanceBuffer { SoAAppearance soa_appearance[]; };\n"
      "layout(std430, binding = 6) readonly buffer SoAEffectsBuffer { SoAEffects soa_effects[]; };\n"
      "uniform vec2 uScreenSize;\n"
      "layout(location = 0) flat out vec4 vColor;\n"
      "layout(location = 1) flat out vec4 vBorderColor;\n"
      "layout(location = 2) flat out vec4 vRadius;\n"
      "layout(location = 3) flat out vec4 vUV;\n"
      "layout(location = 4) flat out uint vType;\n"
      "layout(location = 5) flat out float vBlend;\n"
      "layout(location = 6) flat out float vHover;\n"
      "layout(location = 7) out vec2 vLocalPos;\n"
      "layout(location = 8) out vec2 vSize;\n"
      "layout(location = 9) flat out uint vInstanceID;\n"
      "layout(location = 10) flat out uint vTextureID;\n"
      "layout(location = 11) flat out vec4 vReserved0;\n"
      "layout(location = 12) out vec2 vWorldPos;\n"
      "layout(location = 13) flat out uint vClipID;\n"
      "layout(location = 14) flat out vec4 vClipRect;\n"
      "layout(location = 15) flat out float vGlowIntensity;\n"
      "void main() {\n"
      "  SoAHot h = soa_hot[gl_InstanceID];\n"
      "  SoAAppearance a = soa_appearance[gl_InstanceID];\n"
      "  SoAEffects fx = soa_effects[gl_InstanceID];\n"
      "  vec2 uv01 = aPos * 0.5 + 0.5;\n"
      "  vec2 size = vec2(max(h.w, 1.0), max(h.h, 1.0));\n"
      "  vec2 localPos = vec2(uv01.x, 1.0 - uv01.y) * size;\n"
      "  vec2 pixelPos = vec2(h.x, h.y) + localPos;\n"
      "  vec2 ndc = (pixelPos / uScreenSize) * 2.0 - 1.0;\n"
      "  ndc.y = -ndc.y;\n"
      "  gl_Position = vec4(ndc, h.z, 1.0);\n"
      "  vColor = h.color;\n"
      "  vBorderColor = a.border_color;\n"
      "  vRadius = a.radius;\n"
      "  vUV = a.uv;\n"
      "  vType = h.type;\n"
      "  vBlend = fx.blend;\n"
      "  vHover = fx.hover;\n"
      "  vLocalPos = localPos;\n"
      "  vSize = size;\n"
      "  vInstanceID = gl_InstanceID;\n"
      "  vTextureID = h.texture_id;\n"
      "  vReserved0 = a.control_points;\n"
      "  vWorldPos = pixelPos;\n"
      "  vClipID = 0u;\n"
      "  vClipRect = vec4(0.0);\n"
      "  vGlowIntensity = fx.glow_intensity;\n"
      "}\n";
  static const char *k_vert_hot_appearance_transform =
      "#version 430 core\n"
      "layout(location = 0) in vec2 aPos;\n"
      "struct SoAHot { float x, y, w, h; vec4 color; uint texture_id; uint type; uint flags; float z; };\n"
      "struct SoAAppearance { vec4 border_color; vec4 radius; vec4 uv; vec4 control_points; };\n"
      "struct SoATransform { vec4 row0; vec4 row1; };\n"
      "layout(std430, binding = 4) readonly buffer SoAHotBuffer { SoAHot soa_hot[]; };\n"
      "layout(std430, binding = 5) readonly buffer SoAAppearanceBuffer { SoAAppearance soa_appearance[]; };\n"
      "layout(std430, binding = 7) readonly buffer SoATransformBuffer { SoATransform soa_transform[]; };\n"
      "uniform vec2 uScreenSize;\n"
      "layout(location = 0) flat out vec4 vColor;\n"
      "layout(location = 1) flat out vec4 vBorderColor;\n"
      "layout(location = 2) flat out vec4 vRadius;\n"
      "layout(location = 3) flat out vec4 vUV;\n"
      "layout(location = 4) flat out uint vType;\n"
      "layout(location = 5) flat out float vBlend;\n"
      "layout(location = 6) flat out float vHover;\n"
      "layout(location = 7) out vec2 vLocalPos;\n"
      "layout(location = 8) out vec2 vSize;\n"
      "layout(location = 9) flat out uint vInstanceID;\n"
      "layout(location = 10) flat out uint vTextureID;\n"
      "layout(location = 11) flat out vec4 vReserved0;\n"
      "layout(location = 12) out vec2 vWorldPos;\n"
      "layout(location = 13) flat out uint vClipID;\n"
      "layout(location = 14) flat out vec4 vClipRect;\n"
      "layout(location = 15) flat out float vGlowIntensity;\n"
      "void main() {\n"
      "  SoAHot h = soa_hot[gl_InstanceID];\n"
      "  SoAAppearance a = soa_appearance[gl_InstanceID];\n"
      "  SoATransform t = soa_transform[gl_InstanceID];\n"
      "  vec2 uv01 = aPos * 0.5 + 0.5;\n"
      "  vec2 size = vec2(max(h.w, 1.0), max(h.h, 1.0));\n"
      "  vec2 localPos = vec2(uv01.x, 1.0 - uv01.y) * size;\n"
      "  vec2 elementPos = vec2(h.x, h.y) + localPos;\n"
      "  vec2 pixelPos = vec2(t.row0.x * elementPos.x + t.row0.y * elementPos.y + t.row0.z,\n"
      "                      t.row1.x * elementPos.x + t.row1.y * elementPos.y + t.row1.z);\n"
      "  vec2 ndc = (pixelPos / uScreenSize) * 2.0 - 1.0;\n"
      "  ndc.y = -ndc.y;\n"
      "  gl_Position = vec4(ndc, h.z, 1.0);\n"
      "  vColor = h.color;\n"
      "  vBorderColor = a.border_color;\n"
      "  vRadius = a.radius;\n"
      "  vUV = a.uv;\n"
      "  vType = h.type;\n"
      "  vBlend = 1.0;\n"
      "  vHover = 0.0;\n"
      "  vLocalPos = localPos;\n"
      "  vSize = size;\n"
      "  vInstanceID = gl_InstanceID;\n"
      "  vTextureID = h.texture_id;\n"
      "  vReserved0 = a.control_points;\n"
      "  vWorldPos = pixelPos;\n"
      "  vClipID = 0u;\n"
      "  vClipRect = vec4(0.0);\n"
      "  vGlowIntensity = 0.0;\n"
      "}\n";
  static const char *k_frag_iface =
      "#version 430 core\n"
      "layout(location = 0) flat in vec4 vColor;\n"
      "layout(location = 1) flat in vec4 vBorderColor;\n"
      "layout(location = 2) flat in vec4 vRadius;\n"
      "layout(location = 3) flat in vec4 vUV;\n"
      "layout(location = 4) flat in uint vType;\n"
      "layout(location = 5) flat in float vBlend;\n"
      "layout(location = 6) flat in float vHover;\n"
      "layout(location = 7) in vec2 vLocalPos;\n"
      "layout(location = 8) in vec2 vSize;\n"
      "layout(location = 9) flat in uint vInstanceID;\n"
      "layout(location = 10) flat in uint vTextureID;\n"
      "layout(location = 11) flat in vec4 vReserved0;\n"
      "layout(location = 12) in vec2 vWorldPos;\n"
      "layout(location = 13) flat in uint vClipID;\n"
      "layout(location = 14) flat in vec4 vClipRect;\n"
      "layout(location = 15) flat in float vGlowIntensity;\n"
      "layout(location = 0) out vec4 fragColor;\n"
      "void main() {\n"
      "  fragColor = vColor + vBorderColor * 0.0 + vec4(vRadius.xy, vUV.xy) * 0.0 +\n"
      "              vec4(vBlend + vHover + float(vType) + float(vInstanceID) + float(vTextureID),\n"
      "                   vLocalPos.x + vSize.x + vReserved0.x + vWorldPos.x + float(vClipID) + vGlowIntensity,\n"
      "                   0.0, 1.0) * 0.0;\n"
      "  fragColor += vClipRect * 0.0;\n"
      "}\n";
  static const char *k_frag_iface_samplers =
      "#version 430 core\n"
      "layout(location = 0) flat in vec4 vColor;\n"
      "layout(location = 1) flat in vec4 vBorderColor;\n"
      "layout(location = 2) flat in vec4 vRadius;\n"
      "layout(location = 3) flat in vec4 vUV;\n"
      "layout(location = 4) flat in uint vType;\n"
      "layout(location = 5) flat in float vBlend;\n"
      "layout(location = 6) flat in float vHover;\n"
      "layout(location = 7) in vec2 vLocalPos;\n"
      "layout(location = 8) in vec2 vSize;\n"
      "layout(location = 9) flat in uint vInstanceID;\n"
      "layout(location = 10) flat in uint vTextureID;\n"
      "layout(location = 11) flat in vec4 vReserved0;\n"
      "layout(location = 12) in vec2 vWorldPos;\n"
      "layout(location = 13) flat in uint vClipID;\n"
      "layout(location = 14) flat in vec4 vClipRect;\n"
      "layout(location = 15) flat in float vGlowIntensity;\n"
      "layout(location = 0) out vec4 fragColor;\n"
      "layout(binding = 1) uniform sampler2D uFontTex;\n"
      "layout(binding = 2) uniform sampler2D uImageTex[16];\n"
      "uniform vec2 uAtlasSize;\n"
      "uniform float uPxRange;\n"
      "void main() {\n"
      "  vec4 s = texture(uFontTex, vec2(0.0)) * 0.0 + texture(uImageTex[0], vec2(0.0)) * 0.0;\n"
      "  s += vBorderColor * 0.0 + vec4(vRadius.xy, vUV.xy) * 0.0 + vClipRect * 0.0;\n"
      "  s += vec4(vBlend + vHover + float(vType) + float(vInstanceID) + float(vTextureID),\n"
      "            vLocalPos.x + vSize.x + vReserved0.x + vWorldPos.x + float(vClipID) + vGlowIntensity,\n"
      "            uAtlasSize.x + uPxRange, 1.0) * 0.0;\n"
      "  fragColor = vColor + s;\n"
      "}\n";
  static const char *k_frag_resources =
      "#version 430 core\n"
      "layout(location = 0) flat in vec4 vColor;\n"
      "layout(location = 0) out vec4 fragColor;\n"
      "layout(std430, binding = 3) readonly buffer ClipBuffer { vec4 clip_rects[]; };\n"
      "struct SoAHot { float x, y, w, h; vec4 color; uint texture_id; uint type; uint flags; float z; };\n"
      "struct SoAAppearance { vec4 border_color; vec4 radius; vec4 uv; vec4 control_points; };\n"
      "struct SoAEffects { vec2 shadow_offset; float shadow_blur; float shadow_spread; vec4 shadow_color; vec4 gradient_start; vec4 gradient_end; float hover; float blend; float gradient_angle; float blur_radius; float glow_intensity; uint parent_id; vec2 _pad; };\n"
      "layout(std430, binding = 4) readonly buffer SoAHotBuffer { SoAHot soa_hot[]; };\n"
      "layout(std430, binding = 5) readonly buffer SoAAppearanceBuffer { SoAAppearance soa_appearance[]; };\n"
      "layout(std430, binding = 6) readonly buffer SoAEffectsBuffer { SoAEffects soa_effects[]; };\n"
      "layout(binding = 1) uniform sampler2D uFontTex;\n"
      "layout(binding = 2) uniform sampler2D uImageTex[16];\n"
      "uniform vec2 uAtlasSize;\n"
      "uniform float uPxRange;\n"
      "void main() {\n"
      "  vec4 s = texture(uFontTex, vec2(0.0)) * 0.0 + texture(uImageTex[0], vec2(0.0)) * 0.0;\n"
      "  if (clip_rects.length() > 0) s += clip_rects[0] * 0.0;\n"
      "  if (soa_hot.length() > 0) s += soa_hot[0].color * 0.0;\n"
      "  if (soa_appearance.length() > 0) s += soa_appearance[0].uv * 0.0;\n"
      "  if (soa_effects.length() > 0) s += vec4(soa_effects[0].glow_intensity) * 0.0;\n"
      "  s += vec4(uAtlasSize, uPxRange, 0.0) * 0.0;\n"
      "  fragColor = vColor + s;\n"
      "}\n";
  static const char *k_frag_samplers =
      "#version 430 core\n"
      "layout(location = 0) flat in vec4 vColor;\n"
      "layout(location = 0) out vec4 fragColor;\n"
      "layout(binding = 1) uniform sampler2D uFontTex;\n"
      "layout(binding = 2) uniform sampler2D uImageTex[16];\n"
      "uniform vec2 uAtlasSize;\n"
      "uniform float uPxRange;\n"
      "void main() {\n"
      "  vec4 s = texture(uFontTex, vec2(0.0)) * 0.0 + texture(uImageTex[0], vec2(0.0)) * 0.0;\n"
      "  s += vec4(uAtlasSize, uPxRange, 0.0) * 0.0;\n"
      "  fragColor = vColor + s;\n"
      "}\n";
  static const char *k_frag_ssbo =
      "#version 430 core\n"
      "layout(location = 0) flat in vec4 vColor;\n"
      "layout(location = 0) out vec4 fragColor;\n"
      "layout(std430, binding = 3) readonly buffer ClipBuffer { vec4 clip_rects[]; };\n"
      "struct SoAHot { float x, y, w, h; vec4 color; uint texture_id; uint type; uint flags; float z; };\n"
      "struct SoAAppearance { vec4 border_color; vec4 radius; vec4 uv; vec4 control_points; };\n"
      "struct SoAEffects { vec2 shadow_offset; float shadow_blur; float shadow_spread; vec4 shadow_color; vec4 gradient_start; vec4 gradient_end; float hover; float blend; float gradient_angle; float blur_radius; float glow_intensity; uint parent_id; vec2 _pad; };\n"
      "layout(std430, binding = 4) readonly buffer SoAHotBuffer { SoAHot soa_hot[]; };\n"
      "layout(std430, binding = 5) readonly buffer SoAAppearanceBuffer { SoAAppearance soa_appearance[]; };\n"
      "layout(std430, binding = 6) readonly buffer SoAEffectsBuffer { SoAEffects soa_effects[]; };\n"
      "void main() {\n"
      "  vec4 s = vec4(0.0);\n"
      "  if (clip_rects.length() > 0) s += clip_rects[0] * 0.0;\n"
      "  if (soa_hot.length() > 0) s += soa_hot[0].color * 0.0;\n"
      "  if (soa_appearance.length() > 0) s += soa_appearance[0].uv * 0.0;\n"
      "  if (soa_effects.length() > 0) s += vec4(soa_effects[0].glow_intensity) * 0.0;\n"
      "  fragColor = vColor + s;\n"
      "}\n";
  static const char *k_frag_clip_ssbo =
      "#version 430 core\n"
      "layout(location = 0) flat in vec4 vColor;\n"
      "layout(location = 0) out vec4 fragColor;\n"
      "layout(std430, binding = 3) readonly buffer ClipBuffer { vec4 clip_rects[]; };\n"
      "void main() {\n"
      "  vec4 s = vec4(0.0);\n"
      "  if (clip_rects.length() > 0) s += clip_rects[0] * 0.0;\n"
      "  fragColor = vColor + s;\n"
      "}\n";
  static const char *k_frag_effects_ssbo =
      "#version 430 core\n"
      "layout(location = 0) flat in vec4 vColor;\n"
      "layout(location = 0) out vec4 fragColor;\n"
      "struct SoAEffects { vec2 shadow_offset; float shadow_blur; float shadow_spread; vec4 shadow_color; vec4 gradient_start; vec4 gradient_end; float hover; float blend; float gradient_angle; float blur_radius; float glow_intensity; uint parent_id; vec2 _pad; };\n"
      "layout(std430, binding = 6) readonly buffer SoAEffectsBuffer { SoAEffects soa_effects[]; };\n"
      "void main() {\n"
      "  vec4 s = vec4(0.0);\n"
      "  if (soa_effects.length() > 0) s += vec4(soa_effects[0].glow_intensity) * 0.0;\n"
      "  fragColor = vColor + s;\n"
      "}\n";
  GLuint program = 0;

  program = compile_program_source_pair("Link smoke/minimal", k_vert_min,
                                        k_frag_min);
  if (program)
    glDeleteProgram(program);

  program = compile_program_source_pair("Link smoke/interface", k_vert_iface,
                                        k_frag_iface);
  if (program)
    glDeleteProgram(program);

  program = compile_program_source_pair("Link smoke/hot-only+interface",
                                        k_vert_hot_only, k_frag_iface);
  if (program)
    glDeleteProgram(program);

  program = compile_program_source_pair("Link smoke/hot+appearance+interface",
                                        k_vert_hot_appearance, k_frag_iface);
  if (program)
    glDeleteProgram(program);

  program = compile_program_source_pair("Link smoke/hot+appearance+clip+interface",
                                        k_vert_hot_appearance_clip,
                                        k_frag_iface);
  if (program)
    glDeleteProgram(program);

  program =
      compile_program_source_pair("Link smoke/hot+appearance+effects+interface",
                                  k_vert_hot_appearance_effects,
                                  k_frag_iface);
  if (program)
    glDeleteProgram(program);

  program = compile_program_source_pair(
      "Link smoke/hot+appearance+transform+interface",
      k_vert_hot_appearance_transform, k_frag_iface);
  if (program)
    glDeleteProgram(program);

  program = compile_program_source_pair("Link smoke/vertex-resources+interface",
                                        k_vert_resources, k_frag_iface);
  if (program)
    glDeleteProgram(program);

  program = compile_program_source_pair("Link smoke/interface+samplers",
                                        k_vert_iface, k_frag_iface_samplers);
  if (program)
    glDeleteProgram(program);

  program = compile_program_source_pair(
      "Link smoke/vertex-resources+interface+samplers", k_vert_resources,
      k_frag_iface_samplers);
  if (program)
    glDeleteProgram(program);

  fprintf(stderr, "[Stygian AP] Running Link smoke/samplers\n");
  fflush(stderr);
  program = compile_program_source_pair("Link smoke/samplers", k_vert_min,
                                        k_frag_samplers);
  if (program)
    glDeleteProgram(program);

  fprintf(stderr, "[Stygian AP] Running Link smoke/fragment-ssbo\n");
  fflush(stderr);
  program = compile_program_source_pair("Link smoke/clip-ssbo", k_vert_min,
                                        k_frag_clip_ssbo);
  if (program)
    glDeleteProgram(program);

  program = compile_program_source_pair("Link smoke/effects-ssbo", k_vert_min,
                                        k_frag_effects_ssbo);
  if (program)
    glDeleteProgram(program);

  program = compile_program_source_pair("Link smoke/fragment-ssbo", k_vert_min,
                                        k_frag_ssbo);
  if (program)
    glDeleteProgram(program);

  program = compile_program_source_pair("Link smoke/resources", k_vert_min,
                                        k_frag_resources);
  if (program)
    glDeleteProgram(program);
}
#endif

static GLuint compile_program_with_sources(
    const char *tag, const char *vert_src, const char *frag_src,
    GLint *out_loc_screen_size, GLint *out_loc_font_tex,
    GLint *out_loc_image_tex, GLint *out_loc_atlas_size,
    GLint *out_loc_px_range, GLint *out_loc_output_transform_enabled,
    GLint *out_loc_output_matrix, GLint *out_loc_output_src_srgb,
    GLint *out_loc_output_src_gamma, GLint *out_loc_output_dst_srgb,
    GLint *out_loc_output_dst_gamma) {
  GLuint vs = compile_shader(GL_VERTEX_SHADER, vert_src);
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, frag_src);
  GLuint program;
  GLint status;

  if (!vs || !fs) {
    if (vs)
      glDeleteShader(vs);
    if (fs)
      glDeleteShader(fs);
    fprintf(stderr, "[Stygian AP] %s compile failed\n", tag);
    fflush(stderr);
    return 0;
  }

  program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);
  glDeleteShader(vs);
  glDeleteShader(fs);

  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (!status) {
    char log[4096];
    glGetProgramInfoLog(program, sizeof(log), NULL, log);
    fprintf(stderr, "[Stygian AP] %s link error:\n%s\n", tag, log);
    fflush(stderr);
    glDeleteProgram(program);
    return 0;
  }

  glValidateProgram(program);
  glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
  if (!status) {
    char log[4096];
    glGetProgramInfoLog(program, sizeof(log), NULL, log);
    fprintf(stderr, "[Stygian AP] %s validation warning:\n%s\n", tag, log);
    fflush(stderr);
  }

  if (out_loc_screen_size)
    *out_loc_screen_size = glGetUniformLocation(program, "uScreenSize");
  if (out_loc_font_tex)
    *out_loc_font_tex = glGetUniformLocation(program, "uFontTex");
  if (out_loc_image_tex)
    *out_loc_image_tex = glGetUniformLocation(program, "uImageTex[0]");
  if (out_loc_atlas_size)
    *out_loc_atlas_size = glGetUniformLocation(program, "uAtlasSize");
  if (out_loc_px_range)
    *out_loc_px_range = glGetUniformLocation(program, "uPxRange");
  if (out_loc_output_transform_enabled)
    *out_loc_output_transform_enabled =
        glGetUniformLocation(program, "uOutputColorTransformEnabled");
  if (out_loc_output_matrix)
    *out_loc_output_matrix =
        glGetUniformLocation(program, "uOutputColorMatrix");
  if (out_loc_output_src_srgb)
    *out_loc_output_src_srgb =
        glGetUniformLocation(program, "uOutputSrcIsSRGB");
  if (out_loc_output_src_gamma)
    *out_loc_output_src_gamma =
        glGetUniformLocation(program, "uOutputSrcGamma");
  if (out_loc_output_dst_srgb)
    *out_loc_output_dst_srgb =
        glGetUniformLocation(program, "uOutputDstIsSRGB");
  if (out_loc_output_dst_gamma)
    *out_loc_output_dst_gamma =
        glGetUniformLocation(program, "uOutputDstGamma");

  fprintf(stderr, "[Stygian AP] %s link ok\n", tag);
  fflush(stderr);
  return program;
}

static GLuint compile_gl_compat_program(
    const char *tag, GLint *out_loc_screen_size, GLint *out_loc_font_tex,
    GLint *out_loc_image_tex, GLint *out_loc_atlas_size,
    GLint *out_loc_px_range, GLint *out_loc_output_transform_enabled,
    GLint *out_loc_output_matrix, GLint *out_loc_output_src_srgb,
    GLint *out_loc_output_src_gamma, GLint *out_loc_output_dst_srgb,
    GLint *out_loc_output_dst_gamma) {
  static const char *k_vert =
      "#version 430 core\n"
      "layout(location = 0) in vec2 aPos;\n"
      "struct SoAHot { float x, y, w, h; vec4 color; uint texture_id; uint type; uint flags; float z; };\n"
      "struct SoAAppearance { vec4 border_color; vec4 radius; vec4 uv; vec4 control_points; };\n"
      "layout(std430, binding = 4) readonly buffer SoAHotBuffer { SoAHot soa_hot[]; };\n"
      "layout(std430, binding = 5) readonly buffer SoAAppearanceBuffer { SoAAppearance soa_appearance[]; };\n"
      "uniform vec2 uScreenSize;\n"
      "layout(location = 0) flat out vec4 vColor;\n"
      "layout(location = 1) flat out vec4 vBorderColor;\n"
      "layout(location = 2) flat out vec4 vRadius;\n"
      "layout(location = 3) flat out vec4 vUV;\n"
      "layout(location = 4) flat out uint vType;\n"
      "layout(location = 5) out vec2 vLocalPos;\n"
      "layout(location = 6) out vec2 vSize;\n"
      "layout(location = 7) flat out uint vTextureID;\n"
      "void main() {\n"
      "  SoAHot h = soa_hot[gl_InstanceID];\n"
      "  if ((h.flags & 1u) == 0u) { gl_Position = vec4(-2.0, -2.0, 0.0, 1.0); return; }\n"
      "  SoAAppearance a = soa_appearance[gl_InstanceID];\n"
      "  vec2 uv01 = aPos * 0.5 + 0.5;\n"
      "  vec2 size = vec2(h.w, h.h);\n"
      "  vec2 localPos = vec2(uv01.x, 1.0 - uv01.y) * size;\n"
      "  vec2 elementPos = vec2(h.x, h.y) + localPos;\n"
      "  vec2 pixelPos = elementPos;\n"
      "  vec2 ndc = (pixelPos / uScreenSize) * 2.0 - 1.0;\n"
      "  ndc.y = -ndc.y;\n"
      "  gl_Position = vec4(ndc, h.z, 1.0);\n"
      "  vColor = h.color;\n"
      "  vBorderColor = a.border_color;\n"
      "  vRadius = a.radius;\n"
      "  vUV = a.uv;\n"
      "  vType = h.type & 0x0000ffffu;\n"
      "  vLocalPos = localPos;\n"
      "  vSize = size;\n"
      "  vTextureID = h.texture_id;\n"
      "}\n";
  static const char *k_frag =
      "#version 430 core\n"
      "layout(location = 0) flat in vec4 vColor;\n"
      "layout(location = 1) flat in vec4 vBorderColor;\n"
      "layout(location = 2) flat in vec4 vRadius;\n"
      "layout(location = 3) flat in vec4 vUV;\n"
      "layout(location = 4) flat in uint vType;\n"
      "layout(location = 5) in vec2 vLocalPos;\n"
      "layout(location = 6) in vec2 vSize;\n"
      "layout(location = 7) flat in uint vTextureID;\n"
      "layout(location = 0) out vec4 fragColor;\n"
      "layout(binding = 1) uniform sampler2D uFontTex;\n"
      "layout(binding = 2) uniform sampler2D uImageTex[8];\n"
      "uniform vec2 uAtlasSize;\n"
      "uniform float uPxRange;\n"
      "float sdRoundedBox(vec2 p, vec2 b, vec4 r) { r.xy = (p.x > 0.0) ? r.yz : r.xw; r.x = (p.y > 0.0) ? r.y : r.x; vec2 q = abs(p) - b + r.x; return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r.x; }\n"
      "float sdBox(vec2 p, vec2 b) { vec2 d = abs(p) - b; return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0); }\n"
      "float sdCircle(vec2 p, float r) { return length(p) - r; }\n"
      "float sdSegment(vec2 p, vec2 a, vec2 b) { vec2 pa = p - a, ba = b - a; float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0); return length(pa - ba * h); }\n"
      "float render_rect(vec2 p, vec2 center, vec4 r) { return sdRoundedBox(p, center - 1.0, vec4(r.z, r.y, r.w, r.x)); }\n"
      "float render_rect_outline(vec2 p, vec2 center, vec4 r) { float outer = sdRoundedBox(p, center - 1.0, vec4(r.z, r.y, r.w, r.x)); float inner = sdRoundedBox(p, center - 3.0, vec4(max(0.0, r.z - 2.0), max(0.0, r.y - 2.0), max(0.0, r.w - 2.0), max(0.0, r.x - 2.0))); return max(outer, -inner); }\n"
      "float render_circle(vec2 p, vec2 center) { return sdCircle(p, min(center.x, center.y) - 1.0); }\n"
      "float render_separator(vec2 p) { return abs(p.y) - 0.5; }\n"
      "float render_icon_close(vec2 p, vec2 center) { float arm = min(center.x, center.y) * 0.35; float d1 = sdSegment(p, vec2(-arm, -arm), vec2(arm, arm)) - 1.5; float d2 = sdSegment(p, vec2(-arm, arm), vec2(arm, -arm)) - 1.5; return min(d1, d2); }\n"
      "float render_icon_maximize(vec2 p, vec2 center) { float box_size = min(center.x, center.y) * 0.4; float outer = sdBox(p, vec2(box_size)); float inner = sdBox(p, vec2(box_size - 1.5)); return max(outer, -inner); }\n"
      "float render_icon_minimize(vec2 p, vec2 center) { return sdBox(p, vec2(center.x * 0.5, 1.0)); }\n"
      "float render_icon_plus(vec2 p, vec2 center) { float arm = min(center.x, center.y) * 0.46; float d1 = sdSegment(p, vec2(-arm, 0.0), vec2(arm, 0.0)) - 1.35; float d2 = sdSegment(p, vec2(0.0, -arm), vec2(0.0, arm)) - 1.35; return min(d1, d2); }\n"
      "float render_icon_chevron(vec2 p, vec2 center) { float arm = min(center.x, center.y) * 0.50; vec2 left = vec2(-arm, -arm * 0.25); vec2 apex = vec2(0.0, arm * 0.60); vec2 right = vec2(arm, -arm * 0.25); float d1 = sdSegment(p, left, apex) - 1.45; float d2 = sdSegment(p, apex, right) - 1.45; return min(d1, d2); }\n"
      "vec4 render_text(vec2 local_pos, vec2 size, vec4 uv, vec4 color, float blend) { vec2 uv_norm = local_pos / size; uv_norm.y = 1.0 - uv_norm.y; vec2 tex_coord = mix(uv.xy, uv.zw, uv_norm); vec4 mtsdf = texture(uFontTex, tex_coord); float sd = max(min(mtsdf.r, mtsdf.g), min(max(mtsdf.r, mtsdf.g), mtsdf.b)); vec2 glyph_texel_span = max(abs(uv.zw - uv.xy) * uAtlasSize, vec2(1.0)); vec2 axis_px_range = vec2(uPxRange) * size / glyph_texel_span; float screen_px_range = max(0.5 * (axis_px_range.x + axis_px_range.y), 1.0); float alpha = clamp((sd - 0.5) * screen_px_range + 0.5, 0.0, 1.0); return vec4(color.rgb, alpha * color.a * blend); }\n"
      "vec4 render_texture(vec2 local_pos, vec2 size, vec4 uv_rect, vec4 color, uint tex_slot) { vec2 uv01 = local_pos / size; vec2 uv = mix(uv_rect.xy, uv_rect.zw, uv01); if (tex_slot >= uint(8)) return vec4(1.0, 0.0, 1.0, 1.0) * color; return texture(uImageTex[int(tex_slot)], uv) * color; }\n"
      "void main() {\n"
      "  vec4 col = vColor;\n"
      "  vec2 center = vSize * 0.5;\n"
      "  vec2 p = vLocalPos - center;\n"
      "  float d = 1000.0;\n"
      "  float aa = 1.5;\n"
      "  if (vType == 6u) { fragColor = render_text(vLocalPos, vSize, vUV, col, 1.0); if (fragColor.a < 0.01) discard; return; }\n"
      "  if (vType == 10u) { fragColor = render_texture(vLocalPos, vSize, vUV, col, vTextureID); return; }\n"
      "  if (vType == 0u) { d = render_rect(p, center, vRadius); aa = fwidth(d) * 1.5; }\n"
      "  else if (vType == 1u) { d = render_rect_outline(p, center, vRadius); aa = fwidth(d) * 1.5; }\n"
      "  else if (vType == 2u) { d = render_circle(p, center); aa = fwidth(d) * 1.5; }\n"
      "  else if (vType == 5u) { d = render_rect(p, center, vRadius); aa = fwidth(d) * 1.5; }\n"
      "  else if (vType == 7u) { d = render_icon_close(p, center); }\n"
      "  else if (vType == 8u) { d = render_icon_maximize(p, center); }\n"
      "  else if (vType == 9u) { d = render_icon_minimize(p, center); }\n"
      "  else if (vType == 11u) { d = render_separator(p); }\n"
      "  else if (vType == 13u) { d = render_icon_plus(p, center); }\n"
      "  else if (vType == 14u) { d = render_icon_chevron(p, center); }\n"
      "  else { d = render_rect(p, center, vRadius); aa = fwidth(d) * 1.5; }\n"
      "  col.a *= (1.0 - smoothstep(-aa, aa, d));\n"
      "  if (col.a < 0.01) discard;\n"
      "  fragColor = col;\n"
      "}\n";

  return compile_program_with_sources(
      tag ? tag : "GL compat renderer", k_vert, k_frag, out_loc_screen_size,
      out_loc_font_tex, out_loc_image_tex, out_loc_atlas_size, out_loc_px_range,
      out_loc_output_transform_enabled, out_loc_output_matrix,
      out_loc_output_src_srgb, out_loc_output_src_gamma, out_loc_output_dst_srgb,
      out_loc_output_dst_gamma);
}

// Compile and link shader program, returns program handle or 0 on failure
// Does NOT modify ap->program - caller decides what to do with result
static GLuint compile_program_internal(
    StygianAP *ap, GLint *out_loc_screen_size, GLint *out_loc_font_tex,
    GLint *out_loc_image_tex, GLint *out_loc_atlas_size,
    GLint *out_loc_px_range, GLint *out_loc_output_transform_enabled,
    GLint *out_loc_output_matrix, GLint *out_loc_output_src_srgb,
    GLint *out_loc_output_src_gamma, GLint *out_loc_output_dst_srgb,
    GLint *out_loc_output_dst_gamma) {
  if (ap && ap->adapter_class == STYGIAN_AP_ADAPTER_IGPU) {
    fprintf(stderr,
            "[Stygian AP] Using GL integrated-GPU renderer on this adapter.\n");
    fflush(stderr);
    return compile_gl_compat_program(
        "GL integrated-GPU renderer", out_loc_screen_size, out_loc_font_tex,
        out_loc_image_tex, out_loc_atlas_size, out_loc_px_range,
        out_loc_output_transform_enabled, out_loc_output_matrix,
        out_loc_output_src_srgb, out_loc_output_src_gamma,
        out_loc_output_dst_srgb, out_loc_output_dst_gamma);
  }

  char *vert_src = load_shader_file(ap, "stygian.vert");
  GLuint program = 0;
  if (!vert_src)
    return 0;

  char *frag_src = load_shader_file(ap, "stygian.frag");
  if (!frag_src) {
    ap_free(ap, vert_src);
    return 0;
  }

  program = compile_program_with_sources(
      "Main GL program", vert_src, frag_src, out_loc_screen_size, out_loc_font_tex,
      out_loc_image_tex, out_loc_atlas_size, out_loc_px_range,
      out_loc_output_transform_enabled, out_loc_output_matrix,
      out_loc_output_src_srgb, out_loc_output_src_gamma, out_loc_output_dst_srgb,
      out_loc_output_dst_gamma);
  ap_free(ap, vert_src);
  ap_free(ap, frag_src);
  if (program)
    return program;

  fprintf(stderr,
          "[Stygian AP] Main GL program failed, falling back to GL compat path.\n");
  fflush(stderr);
  program = compile_gl_compat_program(
      "GL fallback renderer", out_loc_screen_size, out_loc_font_tex,
      out_loc_image_tex,
      out_loc_atlas_size, out_loc_px_range, out_loc_output_transform_enabled,
      out_loc_output_matrix, out_loc_output_src_srgb, out_loc_output_src_gamma,
      out_loc_output_dst_srgb, out_loc_output_dst_gamma);
  return program;
}

static bool create_program(StygianAP *ap) {
  GLuint program = compile_program_internal(
      ap, &ap->loc_screen_size, &ap->loc_font_tex, &ap->loc_image_tex,
      &ap->loc_atlas_size, &ap->loc_px_range, &ap->loc_output_transform_enabled,
      &ap->loc_output_matrix, &ap->loc_output_src_srgb,
      &ap->loc_output_src_gamma, &ap->loc_output_dst_srgb,
      &ap->loc_output_dst_gamma);

  if (!program)
    return false;

  ap->program = program;
  ap->current_program = 0u;
  ap->cached_screen_w = -1;
  ap->cached_screen_h = -1;
  ap->sampler_uniforms_dirty = true;
  ap->output_uniforms_dirty = true;
  ap->shader_load_time = get_shader_newest_mod_time(ap->shader_dir);
  printf("[Stygian AP] Shaders loaded from: %s\n", ap->shader_dir);
  return true;
}

static void upload_output_color_transform_uniforms(StygianAP *ap) {
  if (!ap || !ap->program)
    return;
  if (ap->loc_output_transform_enabled >= 0) {
    glUniform1i(ap->loc_output_transform_enabled,
                ap->output_color_transform_enabled ? 1 : 0);
  }
  if (ap->loc_output_matrix >= 0 && glUniformMatrix3fv) {
    glUniformMatrix3fv(ap->loc_output_matrix, 1, GL_TRUE,
                       ap->output_color_matrix);
  }
  if (ap->loc_output_src_srgb >= 0) {
    glUniform1i(ap->loc_output_src_srgb, ap->output_src_srgb_transfer ? 1 : 0);
  }
  if (ap->loc_output_src_gamma >= 0) {
    glUniform1f(ap->loc_output_src_gamma, ap->output_src_gamma);
  }
  if (ap->loc_output_dst_srgb >= 0) {
    glUniform1i(ap->loc_output_dst_srgb, ap->output_dst_srgb_transfer ? 1 : 0);
  }
  if (ap->loc_output_dst_gamma >= 0) {
    glUniform1f(ap->loc_output_dst_gamma, ap->output_dst_gamma);
  }
}

#if 0
static void stygian_ap_log_limit(const char *label, GLenum pname) {
  GLint value = 0;
  glGetIntegerv(pname, &value);
  fprintf(stderr, "[Stygian AP] %s: %d\n", label, (int)value);
  fflush(stderr);
}
#endif

static void stygian_ap_bind_program_if_needed(StygianAP *ap) {
  if (!ap || !ap->program)
    return;
  if (ap->current_program == ap->program)
    return;
  glUseProgram(ap->program);
  ap->current_program = ap->program;
}

static void stygian_ap_bind_vao_if_needed(StygianAP *ap) {
  if (!ap)
    return;
  if (ap->current_vao == ap->vao)
    return;
  glBindVertexArray(ap->vao);
  ap->current_vao = ap->vao;
}

static void stygian_ap_sync_common_uniforms(StygianAP *ap, int logical_w,
                                            int logical_h) {
  if (!ap)
    return;

  stygian_ap_bind_program_if_needed(ap);

  if (ap->cached_screen_w != logical_w || ap->cached_screen_h != logical_h) {
    glUniform2f(ap->loc_screen_size, (float)logical_w, (float)logical_h);
    ap->cached_screen_w = logical_w;
    ap->cached_screen_h = logical_h;
  }

  if (ap->sampler_uniforms_dirty) {
    if (ap->loc_font_tex >= 0)
      glUniform1i(ap->loc_font_tex, 1);
    if (ap->loc_image_tex >= 0 && glUniform1iv) {
      GLint units[STYGIAN_GL_IMAGE_SAMPLERS];
      for (int i = 0; i < STYGIAN_GL_IMAGE_SAMPLERS; ++i) {
        units[i] = STYGIAN_GL_IMAGE_UNIT_BASE + i;
      }
      glUniform1iv(ap->loc_image_tex, STYGIAN_GL_IMAGE_SAMPLERS, units);
    }
    ap->sampler_uniforms_dirty = false;
  }

  if (ap->output_uniforms_dirty) {
    upload_output_color_transform_uniforms(ap);
    ap->output_uniforms_dirty = false;
  }
}

static void stygian_ap_bind_scene_buffers(StygianAP *ap) {
  if (!ap)
    return;
  if (ap->current_ssbo_bindings[3] != ap->clip_ssbo) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ap->clip_ssbo);
    ap->current_ssbo_bindings[3] = ap->clip_ssbo;
  }
  if (ap->current_ssbo_bindings[4] != ap->soa_ssbo_hot) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ap->soa_ssbo_hot);
    ap->current_ssbo_bindings[4] = ap->soa_ssbo_hot;
  }
  if (ap->current_ssbo_bindings[5] != ap->soa_ssbo_appearance) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ap->soa_ssbo_appearance);
    ap->current_ssbo_bindings[5] = ap->soa_ssbo_appearance;
  }
  if (ap->current_ssbo_bindings[6] != ap->soa_ssbo_effects) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ap->soa_ssbo_effects);
    ap->current_ssbo_bindings[6] = ap->soa_ssbo_effects;
  }
  if (ap->current_ssbo_bindings[7] != ap->soa_ssbo_transform) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ap->soa_ssbo_transform);
    ap->current_ssbo_bindings[7] = ap->soa_ssbo_transform;
  }
}

static void stygian_ap_bind_shader_storage_buffer_if_needed(StygianAP *ap,
                                                            GLuint buffer) {
  if (!ap || ap->current_shader_storage_buffer == buffer)
    return;
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
  ap->current_shader_storage_buffer = buffer;
}

static void stygian_ap_upload_range(StygianAP *ap, GLuint *bound_buffer,
                                    GLuint buffer, uint32_t abs_min,
                                    uint32_t range_count, size_t elem_size,
                                    const void *src) {
  if (!ap || !bound_buffer || !buffer || range_count == 0u || !src)
    return;
  if (*bound_buffer != buffer) {
    stygian_ap_bind_shader_storage_buffer_if_needed(ap, buffer);
    *bound_buffer = buffer;
  }
  glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                  (intptr_t)abs_min * (intptr_t)elem_size,
                  (intptr_t)range_count * (intptr_t)elem_size, src);
  ap->last_upload_bytes += range_count * (uint32_t)elem_size;
  ap->last_upload_ranges++;
}

// ============================================================================
// Lifecycle
// ============================================================================

StygianAP *stygian_ap_create(const StygianAPConfig *config) {
  if (!config || !config->window) {
    printf("[Stygian AP] Error: window required\n");
    return NULL;
  }

  StygianAP *ap = (StygianAP *)cfg_alloc(config->allocator, sizeof(StygianAP),
                                         _Alignof(StygianAP));
  if (!ap)
    return NULL;
  memset(ap, 0, sizeof(StygianAP));
  ap->allocator = config->allocator;
  ap->adapter_class = STYGIAN_AP_ADAPTER_UNKNOWN;

  ap->window = config->window;
  ap->max_elements = config->max_elements > 0 ? config->max_elements : 16384;
  ap->output_color_transform_enabled = false;
  ap->output_src_srgb_transfer = true;
  ap->output_dst_srgb_transfer = true;
  ap->output_src_gamma = 2.4f;
  ap->output_dst_gamma = 2.4f;
  ap->present_enabled = true;
  stygian_ap_raw_target_reset(ap);
  ap->cached_screen_w = -1;
  ap->cached_screen_h = -1;
  ap->current_shader_storage_buffer = 0u;
  ap->sampler_uniforms_dirty = true;
  ap->output_uniforms_dirty = true;
  ap->cached_clip_count = 0u;
  memset(ap->output_color_matrix, 0, sizeof(ap->output_color_matrix));
  ap->output_color_matrix[0] = 1.0f;
  ap->output_color_matrix[4] = 1.0f;
  ap->output_color_matrix[8] = 1.0f;

  // Copy shader directory (already resolved by core)
  stygian_cpystr(ap->shader_dir, sizeof(ap->shader_dir),
                 (config->shader_dir && config->shader_dir[0])
                     ? config->shader_dir
                     : "shaders");

  if (!stygian_window_gl_set_pixel_format(config->window)) {
    printf("[Stygian AP] Failed to set pixel format\n");
    cfg_free(config->allocator, ap);
    return NULL;
  }

  ap->gl_context = stygian_window_gl_create_context(config->window, NULL);
  if (!ap->gl_context) {
    printf("[Stygian AP] Failed to create OpenGL context\n");
    cfg_free(config->allocator, ap);
    return NULL;
  }

  if (!stygian_window_gl_make_current(config->window, ap->gl_context)) {
    printf("[Stygian AP] Failed to make OpenGL context current\n");
    stygian_window_gl_destroy_context(ap->gl_context);
    cfg_free(config->allocator, ap);
    return NULL;
  }

  stygian_window_gl_set_vsync(config->window, true);
  printf("[Stygian AP] VSync enabled\n");

  // Load GL extensions
  LOAD_GL(glGenBuffers);
  LOAD_GL(glBindBuffer);
  LOAD_GL(glBufferData);
  LOAD_GL(glBufferSubData);
  LOAD_GL(glBindBufferBase);
  LOAD_GL(glCreateShader);
  LOAD_GL(glShaderSource);
  LOAD_GL(glCompileShader);
  LOAD_GL(glGetShaderiv);
  LOAD_GL(glGetShaderInfoLog);
  LOAD_GL(glCreateProgram);
  LOAD_GL(glAttachShader);
  LOAD_GL(glLinkProgram);
  LOAD_GL(glUseProgram);
  LOAD_GL(glGetProgramiv);
  LOAD_GL(glGetProgramInfoLog);
  LOAD_GL(glGetUniformLocation);
  LOAD_GL(glUniform1i);
  LOAD_GL(glUniform1iv);
  LOAD_GL(glUniform1f);
  LOAD_GL(glUniform2f);
  LOAD_GL(glUniformMatrix3fv);
  LOAD_GL(glEnableVertexAttribArray);
  LOAD_GL(glVertexAttribPointer);
  LOAD_GL(glGenVertexArrays);
  LOAD_GL(glBindVertexArray);
  LOAD_GL(glDrawArraysInstanced);
  LOAD_GL(glDrawArraysInstancedBaseInstance);
  LOAD_GL(glGenFramebuffers);
  LOAD_GL(glDeleteFramebuffers);
  LOAD_GL(glBindFramebuffer);
  LOAD_GL(glCheckFramebufferStatus);
  LOAD_GL(glFramebufferTexture2D);
  LOAD_GL(glDeleteBuffers);
  LOAD_GL(glDeleteProgram);
  LOAD_GL(glActiveTexture);
  LOAD_GL(glDeleteShader);
  LOAD_GL(glValidateProgram);
  LOAD_GL(glGenQueries);
  LOAD_GL(glDeleteQueries);
  LOAD_GL(glBeginQuery);
  LOAD_GL(glEndQuery);
  LOAD_GL(glGetQueryObjectiv);
  LOAD_GL(glGetQueryObjectui64v);
  LOAD_GL(glFenceSync);
  LOAD_GL(glDeleteSync);
  LOAD_GL(glClientWaitSync);
  LOAD_GL(glMapBufferRange);
  LOAD_GL(glUnmapBuffer);
  ap->raw_target_supported = (glGenFramebuffers && glDeleteFramebuffers &&
                              glBindFramebuffer && glCheckFramebufferStatus &&
                              glFramebufferTexture2D);

  stygian_ap_capture_reset_state(ap);
  ap->capture_supported =
      (glFenceSync && glDeleteSync && glClientWaitSync && glMapBufferRange &&
       glUnmapBuffer && glBindBuffer && glBufferData && glGenBuffers &&
       glDeleteBuffers);

  // Check GL version
  const char *version = (const char *)glGetString(GL_VERSION);
  const char *renderer = (const char *)glGetString(GL_RENDERER);
  ap->adapter_class = classify_renderer(renderer);
  if (renderer && renderer[0]) {
    printf("[Stygian AP] Renderer: %s\n", renderer);
  }
  if (version) {
    int major = version[0] - '0';
    int minor = version[2] - '0';
    printf("[Stygian AP] OpenGL %d.%d detected\n", major, minor);
    if (major < 4 || (major == 4 && minor < 3)) {
      printf("[Stygian AP] Warning: OpenGL 4.3+ required for SSBO\n");
    }
  } else {
    printf("[Stygian AP] Warning: Could not get GL version\n");
  }
  // Create shader program
  if (!create_program(ap)) {
    stygian_ap_destroy(ap);
    return NULL;
  }

  // Create SSBO for clip rects (binding 3)
  glGenBuffers(1, &ap->clip_ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ap->clip_ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, STYGIAN_MAX_CLIPS * sizeof(float) * 4,
               NULL, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ap->clip_ssbo);

  // Create SoA SSBOs (bindings 4, 5, 6, 7)
  glGenBuffers(1, &ap->soa_ssbo_hot);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ap->soa_ssbo_hot);
  glBufferData(GL_SHADER_STORAGE_BUFFER,
               ap->max_elements * sizeof(StygianSoAHot), NULL, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ap->soa_ssbo_hot);

  glGenBuffers(1, &ap->soa_ssbo_appearance);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ap->soa_ssbo_appearance);
  glBufferData(GL_SHADER_STORAGE_BUFFER,
               ap->max_elements * sizeof(StygianSoAAppearance), NULL,
               GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ap->soa_ssbo_appearance);

  glGenBuffers(1, &ap->soa_ssbo_effects);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ap->soa_ssbo_effects);
  glBufferData(GL_SHADER_STORAGE_BUFFER,
               ap->max_elements * sizeof(StygianSoAEffects), NULL,
               GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ap->soa_ssbo_effects);

  glGenBuffers(1, &ap->soa_ssbo_transform);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ap->soa_ssbo_transform);
  glBufferData(GL_SHADER_STORAGE_BUFFER,
               ap->max_elements * sizeof(StygianSoATransform), NULL,
               GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ap->soa_ssbo_transform);

  // Optional GPU timing queries (GL_TIME_ELAPSED).
  ap->gpu_query_initialized =
      (glGenQueries && glDeleteQueries && glBeginQuery && glEndQuery &&
       glGetQueryObjectiv && glGetQueryObjectui64v);
  ap->gpu_query_in_flight = false;
  ap->gpu_query_index = 0u;
  ap->last_gpu_ms = 0.0f;
  memset(ap->gpu_queries, 0, sizeof(ap->gpu_queries));
  memset(ap->gpu_query_pending, 0, sizeof(ap->gpu_query_pending));
  if (ap->gpu_query_initialized) {
    glGenQueries(STYGIAN_GL_GPU_QUERY_SLOTS, ap->gpu_queries);
  }

  // Allocate GPU-side version tracking for SoA chunk upload
  // Default chunk_size 256 → chunk_count = ceil(max_elements / 256)
  {
    uint32_t cs = 256u;
    uint32_t cc = (ap->max_elements + cs - 1u) / cs;
    ap->soa_chunk_count = cc;
    ap->gpu_hot_versions =
        (uint32_t *)ap_alloc(ap, cc * sizeof(uint32_t), _Alignof(uint32_t));
    ap->gpu_appearance_versions =
        (uint32_t *)ap_alloc(ap, cc * sizeof(uint32_t), _Alignof(uint32_t));
    ap->gpu_effects_versions =
        (uint32_t *)ap_alloc(ap, cc * sizeof(uint32_t), _Alignof(uint32_t));
    ap->gpu_transform_versions =
        (uint32_t *)ap_alloc(ap, cc * sizeof(uint32_t), _Alignof(uint32_t));
    if (ap->gpu_hot_versions)
      memset(ap->gpu_hot_versions, 0, cc * sizeof(uint32_t));
    if (ap->gpu_appearance_versions)
      memset(ap->gpu_appearance_versions, 0, cc * sizeof(uint32_t));
    if (ap->gpu_effects_versions)
      memset(ap->gpu_effects_versions, 0, cc * sizeof(uint32_t));
    if (ap->gpu_transform_versions)
      memset(ap->gpu_transform_versions, 0, cc * sizeof(uint32_t));
  }

  ap->submit_hot = (StygianSoAHot *)ap_alloc(
      ap, (size_t)ap->max_elements * sizeof(StygianSoAHot),
      _Alignof(StygianSoAHot));
  if (!ap->submit_hot) {
    printf("[Stygian AP] Failed to allocate submit hot buffer\n");
    stygian_ap_destroy(ap);
    return NULL;
  }

  // Create VAO/VBO for quad vertices [-1, +1] range (shader uses aPos * 0.5 +
  // 0.5)
  float quad[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
  glGenVertexArrays(1, &ap->vao);
  glBindVertexArray(ap->vao);

  glGenBuffers(1, &ap->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, ap->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

  ap->initialized = true;
  return ap;
}

void stygian_ap_destroy(StygianAP *ap) {
  if (!ap)
    return;

  stygian_ap_capture_release(ap);
  ap->capture_supported = false;
  stygian_ap_raw_target_release(ap);

  if (ap->clip_ssbo)
    glDeleteBuffers(1, &ap->clip_ssbo);
  if (ap->soa_ssbo_hot)
    glDeleteBuffers(1, &ap->soa_ssbo_hot);
  if (ap->soa_ssbo_appearance)
    glDeleteBuffers(1, &ap->soa_ssbo_appearance);
  if (ap->soa_ssbo_effects)
    glDeleteBuffers(1, &ap->soa_ssbo_effects);
  if (ap->soa_ssbo_transform)
    glDeleteBuffers(1, &ap->soa_ssbo_transform);
  if (ap->gpu_query_initialized) {
    glDeleteQueries(STYGIAN_GL_GPU_QUERY_SLOTS, ap->gpu_queries);
    memset(ap->gpu_queries, 0, sizeof(ap->gpu_queries));
    ap->gpu_query_initialized = false;
    ap->gpu_query_in_flight = false;
    memset(ap->gpu_query_pending, 0, sizeof(ap->gpu_query_pending));
  }
  if (ap->vbo)
    glDeleteBuffers(1, &ap->vbo);
  if (ap->program)
    glDeleteProgram(ap->program);
  ap_free(ap, ap->gpu_hot_versions);
  ap_free(ap, ap->gpu_appearance_versions);
  ap_free(ap, ap->gpu_effects_versions);
  ap_free(ap, ap->gpu_transform_versions);
  ap_free(ap, ap->submit_hot);
  ap->gpu_hot_versions = NULL;
  ap->gpu_appearance_versions = NULL;
  ap->gpu_effects_versions = NULL;
  ap->gpu_transform_versions = NULL;
  ap->submit_hot = NULL;

  if (ap->gl_context) {
    stygian_window_gl_destroy_context(ap->gl_context);
    ap->gl_context = NULL;
  }

  // Free AP struct via its own allocator
  StygianAllocator *allocator = ap->allocator;
  cfg_free(allocator, ap);
}

StygianAPAdapterClass stygian_ap_get_adapter_class(const StygianAP *ap) {
  if (!ap)
    return STYGIAN_AP_ADAPTER_UNKNOWN;
  return ap->adapter_class;
}

uint32_t stygian_ap_get_last_upload_bytes(const StygianAP *ap) {
  if (!ap)
    return 0u;
  return ap->last_upload_bytes;
}

uint32_t stygian_ap_get_last_upload_ranges(const StygianAP *ap) {
  if (!ap)
    return 0u;
  return ap->last_upload_ranges;
}

float stygian_ap_get_last_gpu_ms(const StygianAP *ap) {
  if (!ap)
    return 0.0f;
  return ap->last_gpu_ms;
}

void stygian_ap_gpu_timer_begin(StygianAP *ap) {
  if (!ap || !ap->gpu_query_initialized)
    return;
  if (ap->gpu_query_in_flight)
    return;
  glBeginQuery(GL_TIME_ELAPSED, ap->gpu_queries[ap->gpu_query_index]);
  ap->gpu_query_in_flight = true;
  ap->gpu_query_pending[ap->gpu_query_index] = true;
}

void stygian_ap_gpu_timer_end(StygianAP *ap) {
  uint8_t poll_index;
  GLint available = 0;
  uint64_t ns = 0;
  if (!ap || !ap->gpu_query_initialized)
    return;
  if (!ap->gpu_query_in_flight)
    return;
  glEndQuery(GL_TIME_ELAPSED);
  ap->gpu_query_in_flight = false;

  // Rotate and poll the previous-frame query to avoid stalling on the query
  // we just ended.
  ap->gpu_query_index =
      (uint8_t)((ap->gpu_query_index + 1u) % STYGIAN_GL_GPU_QUERY_SLOTS);
  poll_index = ap->gpu_query_index;

  if (!ap->gpu_query_pending[poll_index])
    return;

  glGetQueryObjectiv(ap->gpu_queries[poll_index], GL_QUERY_RESULT_AVAILABLE,
                     &available);
  if (available) {
    glGetQueryObjectui64v(ap->gpu_queries[poll_index], GL_QUERY_RESULT, &ns);
    ap->last_gpu_ms = (float)((double)ns / 1000000.0);
    ap->gpu_query_pending[poll_index] = false;
  }
}

// ============================================================================
// Shader Hot Reload
// ============================================================================

bool stygian_ap_reload_shaders(StygianAP *ap) {
  if (!ap)
    return false;

  // Compile new program FIRST (do not touch ap->program yet)
  GLint new_loc_screen_size, new_loc_font_tex, new_loc_image_tex,
      new_loc_atlas_size, new_loc_px_range, new_loc_output_transform_enabled,
      new_loc_output_matrix, new_loc_output_src_srgb, new_loc_output_src_gamma,
      new_loc_output_dst_srgb, new_loc_output_dst_gamma;
  GLuint new_program = compile_program_internal(
      ap, &new_loc_screen_size, &new_loc_font_tex, &new_loc_image_tex,
      &new_loc_atlas_size, &new_loc_px_range, &new_loc_output_transform_enabled,
      &new_loc_output_matrix, &new_loc_output_src_srgb,
      &new_loc_output_src_gamma, &new_loc_output_dst_srgb,
      &new_loc_output_dst_gamma);

  if (!new_program) {
    // Compilation failed - keep old shader, no black screen!
    printf("[Stygian AP] Shader reload FAILED - keeping previous shader\n");
    return false;
  }

  // Success! Now safe to delete old program
  if (ap->program) {
    glDeleteProgram(ap->program);
  }

  // Swap in new program and uniform locations
  ap->program = new_program;
  ap->loc_screen_size = new_loc_screen_size;
  ap->loc_font_tex = new_loc_font_tex;
  ap->loc_image_tex = new_loc_image_tex;
  ap->loc_atlas_size = new_loc_atlas_size;
  ap->loc_px_range = new_loc_px_range;
  ap->loc_output_transform_enabled = new_loc_output_transform_enabled;
  ap->loc_output_matrix = new_loc_output_matrix;
  ap->loc_output_src_srgb = new_loc_output_src_srgb;
  ap->loc_output_src_gamma = new_loc_output_src_gamma;
  ap->loc_output_dst_srgb = new_loc_output_dst_srgb;
  ap->loc_output_dst_gamma = new_loc_output_dst_gamma;
  ap->current_program = 0u;
  ap->cached_screen_w = -1;
  ap->cached_screen_h = -1;
  ap->sampler_uniforms_dirty = true;
  ap->output_uniforms_dirty = true;

  // Update load timestamp for hot-reload tracking
  ap->shader_load_time = get_shader_newest_mod_time(ap->shader_dir);
  stygian_ap_bind_program_if_needed(ap);
  if (ap->output_uniforms_dirty) {
    upload_output_color_transform_uniforms(ap);
    ap->output_uniforms_dirty = false;
  }

  printf("[Stygian AP] Shaders reloaded successfully\n");
  return true;
}

// Check if shader files have been modified since last load
bool stygian_ap_shaders_need_reload(StygianAP *ap) {
  if (!ap || !ap->shader_dir[0])
    return false;

  uint64_t newest = get_shader_newest_mod_time(ap->shader_dir);
  return newest > ap->shader_load_time;
}

// ============================================================================
// Frame Management
// ============================================================================

void stygian_ap_begin_frame(StygianAP *ap, int width, int height) {
  if (!ap)
    return;

  // Ensure the correct GL context is current for this frame.
  stygian_ap_make_current(ap);
  stygian_ap_bind_render_target(ap, width, height);

  glViewport(0, 0, width, height);
  glClearColor(0.235f, 0.259f, 0.294f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  stygian_ap_sync_common_uniforms(ap, width, height);
  stygian_ap_bind_vao_if_needed(ap);
  stygian_ap_bind_scene_buffers(ap);
}

void stygian_ap_submit(StygianAP *ap, const StygianSoAHot *soa_hot,
                       uint32_t count) {
  if (!ap || !soa_hot || !ap->submit_hot || count == 0)
    return;

  if (count > ap->max_elements) {
    count = ap->max_elements;
  }

  ap->element_count = count;
  ap->submit_hot_src = soa_hot;

  // Remap sparse texture handles to dense sampler slots per submit.
  // Texture unit routing:
  //   unit 1: font atlas
  //   units 2..(2+N-1): image textures (STYGIAN_TEXTURE)
  uint32_t mapped_handles[STYGIAN_GL_IMAGE_SAMPLERS];
  uint32_t mapped_count = 0;
  bool copied_submit_hot = false;

  for (uint32_t i = 0; i < count; ++i) {
    // Most frames do not need texture slot remapping at all. Borrow the hot
    // stream directly until the first texture element shows up; only then do we
    // pay for a copied submit stream.
    uint32_t type = soa_hot[i].type & 0xFFFF;
    uint32_t tex_id = soa_hot[i].texture_id;

    if (type == STYGIAN_TEXTURE && tex_id != 0) {
      if (!copied_submit_hot) {
        memcpy(ap->submit_hot, soa_hot, sizeof(StygianSoAHot) * count);
        ap->submit_hot_src = ap->submit_hot;
        copied_submit_hot = true;
      }
      uint32_t slot = UINT32_MAX;
      for (uint32_t j = 0; j < mapped_count; ++j) {
        if (mapped_handles[j] == tex_id) {
          slot = j;
          break;
        }
      }

      if (slot == UINT32_MAX) {
        if (mapped_count < STYGIAN_GL_IMAGE_SAMPLERS) {
          slot = mapped_count;
          mapped_handles[mapped_count++] = tex_id;
        } else {
          slot = STYGIAN_GL_IMAGE_SAMPLERS;
        }
      }
      // Keep source SoA immutable; only the submit stream gets remapped IDs.
      ap->submit_hot[i].texture_id = slot;
    }
  }

  // Bind image textures to configured image units.
  for (uint32_t i = 0; i < mapped_count; ++i) {
    if (ap->bound_image_textures[i] == (GLuint)mapped_handles[i])
      continue;
    glActiveTexture(GL_TEXTURE0 + STYGIAN_GL_IMAGE_UNIT_BASE + i);
    glBindTexture(GL_TEXTURE_2D, (GLuint)mapped_handles[i]);
    ap->bound_image_textures[i] = (GLuint)mapped_handles[i];
  }
}

// ============================================================================
// SoA Versioned Chunk Upload
// ============================================================================

void stygian_ap_submit_soa(StygianAP *ap, const StygianSoAHot *hot,
                           const StygianSoAAppearance *appearance,
                           const StygianSoAEffects *effects,
                           const StygianSoATransform *transform,
                           uint32_t element_count,
                           const StygianBufferChunk *chunks,
                           uint32_t chunk_count, uint32_t chunk_size) {
  if (!ap || !hot || !appearance || !effects || !transform || !chunks ||
      element_count == 0)
    return;
  const StygianSoAHot *hot_src = ap->submit_hot_src ? ap->submit_hot_src : hot;

  ap->last_upload_bytes = 0u;
  ap->last_upload_ranges = 0u;

  // Clamp to tracked chunk metadata; submit must never walk past AP mirrors.
  if (chunk_count > ap->soa_chunk_count) {
    chunk_count = ap->soa_chunk_count;
  }

  GLuint bound_upload_buffer = 0u;

  for (uint32_t ci = 0; ci < chunk_count; ci++) {
    const StygianBufferChunk *c = &chunks[ci];
    uint32_t base = ci * chunk_size;

    if (ap->gpu_hot_versions && c->hot_version != ap->gpu_hot_versions[ci]) {
      uint32_t dmin = c->hot_dirty_min;
      uint32_t dmax = c->hot_dirty_max;
      if (dmin <= dmax) {
        uint32_t abs_min = base + dmin;
        uint32_t abs_max = base + dmax;
        if (abs_max >= element_count)
          abs_max = element_count - 1;
        if (abs_min < element_count) {
          uint32_t range_count = abs_max - abs_min + 1;
          stygian_ap_upload_range(ap, &bound_upload_buffer, ap->soa_ssbo_hot,
                                  abs_min, range_count, sizeof(StygianSoAHot),
                                  &hot_src[abs_min]);
        }
      }
      ap->gpu_hot_versions[ci] = c->hot_version;
    }
  }

  for (uint32_t ci = 0; ci < chunk_count; ci++) {
    const StygianBufferChunk *c = &chunks[ci];
    uint32_t base = ci * chunk_size;
    if (ap->gpu_appearance_versions &&
        c->appearance_version != ap->gpu_appearance_versions[ci]) {
      uint32_t dmin = c->appearance_dirty_min;
      uint32_t dmax = c->appearance_dirty_max;
      if (dmin <= dmax) {
        uint32_t abs_min = base + dmin;
        uint32_t abs_max = base + dmax;
        if (abs_max >= element_count)
          abs_max = element_count - 1;
        if (abs_min < element_count) {
          uint32_t range_count = abs_max - abs_min + 1;
          stygian_ap_upload_range(ap, &bound_upload_buffer,
                                  ap->soa_ssbo_appearance, abs_min, range_count,
                                  sizeof(StygianSoAAppearance),
                                  &appearance[abs_min]);
        }
      }
      ap->gpu_appearance_versions[ci] = c->appearance_version;
    }
  }

  for (uint32_t ci = 0; ci < chunk_count; ci++) {
    const StygianBufferChunk *c = &chunks[ci];
    uint32_t base = ci * chunk_size;
    if (ap->gpu_effects_versions &&
        c->effects_version != ap->gpu_effects_versions[ci]) {
      uint32_t dmin = c->effects_dirty_min;
      uint32_t dmax = c->effects_dirty_max;
      if (dmin <= dmax) {
        uint32_t abs_min = base + dmin;
        uint32_t abs_max = base + dmax;
        if (abs_max >= element_count)
          abs_max = element_count - 1;
        if (abs_min < element_count) {
          uint32_t range_count = abs_max - abs_min + 1;
          stygian_ap_upload_range(ap, &bound_upload_buffer,
                                  ap->soa_ssbo_effects, abs_min, range_count,
                                  sizeof(StygianSoAEffects), &effects[abs_min]);
        }
      }
      ap->gpu_effects_versions[ci] = c->effects_version;
    }
  }

  for (uint32_t ci = 0; ci < chunk_count; ci++) {
    const StygianBufferChunk *c = &chunks[ci];
    uint32_t base = ci * chunk_size;
    if (ap->gpu_transform_versions &&
        c->transform_version != ap->gpu_transform_versions[ci]) {
      uint32_t dmin = c->transform_dirty_min;
      uint32_t dmax = c->transform_dirty_max;
      if (dmin <= dmax) {
        uint32_t abs_min = base + dmin;
        uint32_t abs_max = base + dmax;
        if (abs_max >= element_count)
          abs_max = element_count - 1;
        if (abs_min < element_count) {
          uint32_t range_count = abs_max - abs_min + 1;
          stygian_ap_upload_range(ap, &bound_upload_buffer,
                                  ap->soa_ssbo_transform, abs_min, range_count,
                                  sizeof(StygianSoATransform),
                                  &transform[abs_min]);
        }
      }
      ap->gpu_transform_versions[ci] = c->transform_version;
    }
  }
}

void stygian_ap_draw(StygianAP *ap) {
  if (!ap || ap->element_count == 0)
    return;
  stygian_ap_draw_range(ap, 0u, ap->element_count);
}

void stygian_ap_draw_range(StygianAP *ap, uint32_t first_instance,
                           uint32_t instance_count) {
  if (!ap || instance_count == 0)
    return;
  if (glDrawArraysInstancedBaseInstance) {
    glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, instance_count,
                                      first_instance);
    return;
  }
  if (first_instance != 0u) {
    // This should be unavailable only on very old GL drivers.
    // Fall back to full draw to preserve visibility over perfect layering.
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, ap->element_count);
    return;
  }
  glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instance_count);
}

void stygian_ap_end_frame(StygianAP *ap) {
  if (!ap)
    return;
}

void stygian_ap_set_clips(StygianAP *ap, const float *clips, uint32_t count) {
  if (!ap || !ap->clip_ssbo || !clips || count == 0)
    return;
  if (count > STYGIAN_MAX_CLIPS)
    count = STYGIAN_MAX_CLIPS;

  if (ap->cached_clip_count == count &&
      memcmp(ap->cached_clips, clips, count * sizeof(float) * 4u) == 0) {
    return;
  }

  stygian_ap_bind_shader_storage_buffer_if_needed(ap, ap->clip_ssbo);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(float) * 4,
                  clips);
  memcpy(ap->cached_clips, clips, count * sizeof(float) * 4u);
  ap->cached_clip_count = count;
}

void stygian_ap_swap(StygianAP *ap) {
  if (!ap)
    return;
  stygian_window_gl_swap_buffers(ap->window);
}

void stygian_ap_set_present_enabled(StygianAP *ap, bool enable) {
  if (!ap)
    return;
  ap->present_enabled = enable;
  if (enable && glBindFramebuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // The raw target only exists to keep no-present frames off the window
    // backbuffer. Once present comes back, hanging onto a fullscreen texture is
    // just dead weight.
    stygian_ap_raw_target_release(ap);
  }
}

// ============================================================================
// Textures
// ============================================================================

StygianAPTexture stygian_ap_texture_create(StygianAP *ap, int w, int h,
                                           const void *rgba) {
  if (!ap)
    return 0;

  GLuint tex;
  // Keep font sampler binding (unit 1) intact by creating textures on unit 0.
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               rgba);

  return (StygianAPTexture)tex;
}

bool stygian_ap_texture_update(StygianAP *ap, StygianAPTexture tex, int x,
                               int y, int w, int h, const void *rgba) {
  if (!ap || !tex || !rgba || w <= 0 || h <= 0)
    return false;

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, (GLuint)tex);
  glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE,
                  rgba);
  return true;
}

void stygian_ap_texture_destroy(StygianAP *ap, StygianAPTexture tex) {
  if (!ap || !tex)
    return;
  GLuint id = (GLuint)tex;
  glDeleteTextures(1, &id);
}

void stygian_ap_texture_bind(StygianAP *ap, StygianAPTexture tex,
                             uint32_t slot) {
  if (!ap)
    return;
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, (GLuint)tex);
}

// ============================================================================
// Uniforms
// ============================================================================

void stygian_ap_set_font_texture(StygianAP *ap, StygianAPTexture tex,
                                 int atlas_w, int atlas_h, float px_range) {
  if (!ap)
    return;

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, (GLuint)tex);

  stygian_ap_bind_program_if_needed(ap);
  glUniform2f(ap->loc_atlas_size, (float)atlas_w, (float)atlas_h);
  glUniform1f(ap->loc_px_range, px_range);
}

void stygian_ap_set_output_color_transform(
    StygianAP *ap, bool enabled, const float *rgb3x3, bool src_srgb_transfer,
    float src_gamma, bool dst_srgb_transfer, float dst_gamma) {
  static const float identity[9] = {
      1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  };
  if (!ap)
    return;
  ap->output_color_transform_enabled = enabled;
  memcpy(ap->output_color_matrix, rgb3x3 ? rgb3x3 : identity,
         sizeof(ap->output_color_matrix));
  ap->output_src_srgb_transfer = src_srgb_transfer;
  ap->output_dst_srgb_transfer = dst_srgb_transfer;
  ap->output_src_gamma = (src_gamma > 0.0f) ? src_gamma : 2.2f;
  ap->output_dst_gamma = (dst_gamma > 0.0f) ? dst_gamma : 2.2f;
  ap->output_uniforms_dirty = true;
  if (!ap->program)
    return;
  stygian_ap_bind_program_if_needed(ap);
  upload_output_color_transform_uniforms(ap);
  ap->output_uniforms_dirty = false;
}

// ============================================================================
// Multi-Surface Support (Floating Windows)
// ============================================================================

struct StygianAPSurface {
  StygianWindow *window;
  int width;
  int height;
};

StygianAPSurface *stygian_ap_surface_create(StygianAP *ap,
                                            StygianWindow *window) {
  if (!ap || !window)
    return NULL;

  StygianAPSurface *surf = (StygianAPSurface *)ap_alloc(
      ap, sizeof(StygianAPSurface), _Alignof(StygianAPSurface));
  if (!surf)
    return NULL;
  memset(surf, 0, sizeof(StygianAPSurface));

  surf->window = window;

  if (!stygian_window_gl_set_pixel_format(window)) {
    printf("[Stygian AP GL] Failed to set pixel format for surface\n");
    ap_free(ap, surf);
    return NULL;
  }

  printf("[Stygian AP GL] Surface created\n");
  return surf;
}

void stygian_ap_surface_destroy(StygianAP *ap, StygianAPSurface *surface) {
  if (!ap || !surface)
    return;

  // Window layer owns native DC lifetime; surface teardown only frees wrapper.

  ap_free(ap, surface);
}

void stygian_ap_surface_begin(StygianAP *ap, StygianAPSurface *surface,
                              int width, int height) {
  if (!ap || !surface)
    return;

  surface->width = width;
  surface->height = height;

  if (!stygian_window_gl_make_current(surface->window, ap->gl_context)) {
    printf("[Stygian AP GL] Failed to make surface current\n");
    return;
  }

  glViewport(0, 0, width, height);
  glClearColor(0.235f, 0.259f, 0.294f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Projection uses logical size to match UI layout coordinates across DPI.
  int log_w, log_h;
  if (surface->window) {
    stygian_window_get_size(surface->window, &log_w, &log_h);
  } else {
    log_w = width;
    log_h = height;
  }

  stygian_ap_sync_common_uniforms(ap, log_w, log_h);
  stygian_ap_bind_vao_if_needed(ap);
  stygian_ap_bind_scene_buffers(ap);
}

void stygian_ap_surface_submit(StygianAP *ap, StygianAPSurface *surface,
                               const StygianSoAHot *soa_hot, uint32_t count) {
  // Shared context path: reuse main submit/draw to preserve AP parity.
  stygian_ap_submit(ap, soa_hot, count);
  stygian_ap_draw(ap);
  stygian_ap_end_frame(ap);
}

void stygian_ap_surface_end(StygianAP *ap, StygianAPSurface *surface) {
  // No additional GL work; submit path already flushed commands.
  (void)ap;
  (void)surface;
}

void stygian_ap_surface_swap(StygianAP *ap, StygianAPSurface *surface) {
  if (!ap || !surface)
    return;

  stygian_window_gl_swap_buffers(surface->window);

  // Main context restoration is handled by the next surface/main begin call.
}

void stygian_ap_make_current(StygianAP *ap) {
  if (!ap)
    return;

  if (!stygian_window_gl_make_current(ap->window, ap->gl_context)) {
    printf("[Stygian AP GL] Failed to restore main context\n");
  }
}

void stygian_ap_set_viewport(StygianAP *ap, int width, int height) {
  if (!ap)
    return;
  glViewport(0, 0, width, height);

  // Restore logical projection after multi-surface switches.
  if (ap->window) {
    int log_w, log_h;
    stygian_window_get_size(ap->window, &log_w, &log_h);
    stygian_ap_sync_common_uniforms(ap, log_w, log_h);
  } else {
    stygian_ap_sync_common_uniforms(ap, width, height);
  }
}

bool stygian_ap_capture_is_supported(const StygianAP *ap) {
  if (!ap)
    return false;
  return ap->capture_supported;
}

bool stygian_ap_capture_begin(StygianAP *ap) {
  int width = 0;
  int height = 0;
  if (!ap || !ap->capture_supported)
    return false;
  stygian_window_get_framebuffer_size(ap->window, &width, &height);
  if (width <= 0 || height <= 0)
    return false;
  if (!stygian_ap_capture_configure(ap, width, height))
    return false;
  ap->capture_active = true;
  return true;
}

void stygian_ap_capture_end(StygianAP *ap) {
  if (!ap)
    return;
  stygian_ap_capture_release(ap);
}

bool stygian_ap_capture_request_readback(StygianAP *ap) {
  uint8_t slot = 0;
  int width = 0;
  int height = 0;
  if (!ap || !ap->capture_supported || !ap->capture_active)
    return false;

  stygian_window_get_framebuffer_size(ap->window, &width, &height);
  if (width <= 0 || height <= 0)
    return false;
  if (width != ap->capture_width || height != ap->capture_height) {
    if (!stygian_ap_capture_configure(ap, width, height))
      return false;
  }

  slot = ap->capture_write_slot;
  if (ap->capture_in_flight[slot]) {
    GLenum wait = glClientWaitSync(ap->capture_fences[slot], 0, 0);
    if (wait != GL_ALREADY_SIGNALED && wait != GL_CONDITION_SATISFIED) {
      return false;
    }
    glDeleteSync(ap->capture_fences[slot]);
    ap->capture_fences[slot] = NULL;
    ap->capture_in_flight[slot] = false;
  }

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, ap->capture_pbos[slot]);
  glReadBuffer((!ap->present_enabled && ap->raw_fbo) ? GL_COLOR_ATTACHMENT0
                                                     : GL_BACK);
  glReadPixels(0, 0, ap->capture_width, ap->capture_height, GL_RGBA,
               GL_UNSIGNED_BYTE, 0);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

  ap->capture_fences[slot] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  if (!ap->capture_fences[slot]) {
    return false;
  }
  ap->capture_frame_counter++;
  ap->capture_slot_frame_index[slot] = ap->capture_frame_counter;
  ap->capture_in_flight[slot] = true;
  ap->capture_write_slot = (uint8_t)((slot + 1u) % STYGIAN_GL_CAPTURE_PBO_SLOTS);
  return true;
}

bool stygian_ap_capture_poll_readback(StygianAP *ap, uint8_t *dst_rgba,
                                      uint32_t dst_bytes,
                                      StygianAPCaptureFrameInfo *out_info) {
  uint8_t slot = 0;
  GLenum wait = GL_WAIT_FAILED;
  void *mapped = NULL;
  uint32_t bytes_to_copy = 0u;
  if (!ap || !ap->capture_supported || !ap->capture_active || !dst_rgba ||
      dst_bytes == 0u)
    return false;
  slot = ap->capture_read_slot;
  if (!ap->capture_in_flight[slot] || !ap->capture_fences[slot])
    return false;

  wait = glClientWaitSync(ap->capture_fences[slot], 0, 0);
  if (wait != GL_ALREADY_SIGNALED && wait != GL_CONDITION_SATISFIED)
    return false;

  if (out_info) {
    memset(out_info, 0, sizeof(*out_info));
    out_info->width = ap->capture_width;
    out_info->height = ap->capture_height;
    out_info->stride_bytes = ap->capture_stride_bytes;
    out_info->frame_index = ap->capture_slot_frame_index[slot];
    out_info->format = STYGIAN_AP_CAPTURE_FORMAT_RGBA8;
  }
  if (dst_bytes < ap->capture_frame_bytes) {
    return false;
  }

  glBindBuffer(GL_PIXEL_PACK_BUFFER, ap->capture_pbos[slot]);
  mapped = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0,
                            (GLsizeiptr)ap->capture_frame_bytes,
                            GL_MAP_READ_BIT);
  if (!mapped) {
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    return false;
  }
  bytes_to_copy = ap->capture_frame_bytes;
  if (dst_bytes < bytes_to_copy)
    bytes_to_copy = dst_bytes;
  memcpy(dst_rgba, mapped, bytes_to_copy);
  glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

  glDeleteSync(ap->capture_fences[slot]);
  ap->capture_fences[slot] = NULL;
  ap->capture_in_flight[slot] = false;
  ap->capture_slot_frame_index[slot] = 0u;
  ap->capture_read_slot = (uint8_t)((slot + 1u) % STYGIAN_GL_CAPTURE_PBO_SLOTS);

  if (out_info) {
    out_info->format = STYGIAN_AP_CAPTURE_FORMAT_RGBA8;
  }
  return true;
}

bool stygian_ap_capture_snapshot(StygianAP *ap, uint8_t *dst_rgba,
                                 uint32_t dst_bytes,
                                 StygianAPCaptureFrameInfo *out_info) {
  int width = 0;
  int height = 0;
  uint32_t stride = 0u;
  uint32_t bytes = 0u;
  if (!ap || !ap->capture_supported || !dst_rgba || dst_bytes == 0u)
    return false;

  stygian_window_get_framebuffer_size(ap->window, &width, &height);
  if (width <= 0 || height <= 0)
    return false;
  stride = (uint32_t)width * 4u;
  bytes = stride * (uint32_t)height;
  if (bytes == 0u || dst_bytes < bytes)
    return false;

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  glReadBuffer((!ap->present_enabled && ap->raw_fbo) ? GL_COLOR_ATTACHMENT0
                                                     : GL_BACK);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dst_rgba);

  if (out_info) {
    memset(out_info, 0, sizeof(*out_info));
    out_info->width = width;
    out_info->height = height;
    out_info->stride_bytes = stride;
    out_info->frame_index = 0u;
    out_info->format = STYGIAN_AP_CAPTURE_FORMAT_RGBA8;
  }
  return true;
}
