#include "stygian_icc.h"
#include "stygian_internal.h" // stygian_cpystr

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ICCTagRecord {
  uint32_t sig;
  uint32_t offset;
  uint32_t size;
} ICCTagRecord;

typedef enum ICCTransferKind {
  ICC_TRANSFER_UNKNOWN = 0,
  ICC_TRANSFER_LINEAR = 1,
  ICC_TRANSFER_GAMMA = 2,
  ICC_TRANSFER_SRGB = 3,
} ICCTransferKind;

typedef struct ICCTransfer {
  ICCTransferKind kind;
  float gamma;
  bool valid;
} ICCTransfer;

static uint16_t be16(const uint8_t *p) {
  return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static uint32_t be32(const uint8_t *p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static int32_t be32s(const uint8_t *p) { return (int32_t)be32(p); }

static float s15fixed16_to_float(int32_t v) { return (float)v / 65536.0f; }

static uint32_t fourcc(const char a, const char b, const char c, const char d) {
  return ((uint32_t)(uint8_t)a << 24) | ((uint32_t)(uint8_t)b << 16) |
         ((uint32_t)(uint8_t)c << 8) | (uint32_t)(uint8_t)d;
}

static float clamp01(float v) {
  if (v < 0.0f)
    return 0.0f;
  if (v > 1.0f)
    return 1.0f;
  return v;
}

static float srgb_to_linear(float c) {
  c = clamp01(c);
  if (c <= 0.04045f)
    return c / 12.92f;
  return powf((c + 0.055f) / 1.055f, 2.4f);
}

static bool read_file(const char *path, uint8_t **out_data, size_t *out_size) {
  FILE *f;
  long n;
  uint8_t *buf;
  if (!path || !out_data || !out_size)
    return false;
  f = fopen(path, "rb");
  if (!f)
    return false;
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return false;
  }
  n = ftell(f);
  if (n <= 0) {
    fclose(f);
    return false;
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return false;
  }
  buf = (uint8_t *)malloc((size_t)n);
  if (!buf) {
    fclose(f);
    return false;
  }
  if (fread(buf, 1, (size_t)n, f) != (size_t)n) {
    free(buf);
    fclose(f);
    return false;
  }
  fclose(f);
  *out_data = buf;
  *out_size = (size_t)n;
  return true;
}

static const ICCTagRecord *find_tag(const ICCTagRecord *tags, uint32_t count,
                                    uint32_t sig) {
  uint32_t i;
  for (i = 0; i < count; i++) {
    if (tags[i].sig == sig)
      return &tags[i];
  }
  return NULL;
}

static bool parse_xyz_tag(const uint8_t *buf, size_t len, const ICCTagRecord *t,
                          float out_xyz[3]) {
  const uint8_t *p;
  if (!buf || !t || !out_xyz)
    return false;
  if ((size_t)t->offset + (size_t)t->size > len || t->size < 20u)
    return false;
  p = buf + t->offset;
  if (be32(p) != fourcc('X', 'Y', 'Z', ' '))
    return false;
  p += 8; // type + reserved
  out_xyz[0] = (float)be32s(p + 0) / 65536.0f;
  out_xyz[1] = (float)be32s(p + 4) / 65536.0f;
  out_xyz[2] = (float)be32s(p + 8) / 65536.0f;
  return true;
}

static bool parse_desc_tag(const uint8_t *buf, size_t len,
                           const ICCTagRecord *t, char *out, size_t out_size) {
  const uint8_t *p;
  uint32_t text_len;
  if (!buf || !t || !out || out_size == 0u)
    return false;
  if ((size_t)t->offset + (size_t)t->size > len || t->size < 16u)
    return false;
  p = buf + t->offset;
  if (be32(p) != fourcc('d', 'e', 's', 'c'))
    return false;
  text_len = be32(p + 8);
  if (text_len == 0u)
    return false;
  if (12u + text_len > t->size)
    return false;
  if (text_len >= out_size)
    text_len = (uint32_t)out_size - 1u;
  memcpy(out, p + 12, text_len);
  out[text_len] = '\0';
  return true;
}

static bool parse_mluc_tag(const uint8_t *buf, size_t len,
                           const ICCTagRecord *t, char *out, size_t out_size) {
  const uint8_t *p;
  uint32_t record_count;
  uint32_t record_size;
  uint32_t str_len;
  uint32_t str_off;
  uint32_t i;
  uint32_t copied = 0u;
  if (!buf || !t || !out || out_size == 0u)
    return false;
  if ((size_t)t->offset + (size_t)t->size > len || t->size < 28u)
    return false;
  p = buf + t->offset;
  if (be32(p) != fourcc('m', 'l', 'u', 'c'))
    return false;
  record_count = be32(p + 8);
  record_size = be32(p + 12);
  if (record_count == 0u || record_size < 12u)
    return false;
  if (16u + record_size > t->size)
    return false;
  p += 16; // first record
  str_len = be32(p + 4);
  str_off = be32(p + 8);
  if (str_len < 2u || (str_len % 2u) != 0u)
    return false;
  if ((size_t)str_off + (size_t)str_len > t->size)
    return false;
  p = buf + t->offset + str_off;
  for (i = 0u; i + 1u < str_len && copied + 1u < out_size; i += 2u) {
    uint16_t codepoint = be16(p + i);
    if (codepoint >= 32u && codepoint < 127u) {
      out[copied++] = (char)codepoint;
    } else if (codepoint == 0u) {
      break;
    } else {
      out[copied++] = '?';
    }
  }
  out[copied] = '\0';
  return copied > 0u;
}

static void icc_transfer_from_gamma(ICCTransfer *transfer, float gamma) {
  if (!transfer)
    return;
  transfer->valid = true;
  if (gamma < 0.01f)
    gamma = 2.2f;
  transfer->gamma = gamma;
  if (fabsf(gamma - 1.0f) < 0.03f) {
    transfer->kind = ICC_TRANSFER_LINEAR;
    transfer->gamma = 1.0f;
  } else {
    transfer->kind = ICC_TRANSFER_GAMMA;
  }
}

static bool icc_curve_matches_srgb(const uint8_t *p, uint32_t count) {
  uint32_t i;
  float max_err = 0.0f;
  float err_sum = 0.0f;
  if (!p || count < 16u)
    return false;
  for (i = 0u; i < count; i++) {
    float x = (count > 1u) ? ((float)i / (float)(count - 1u)) : 0.0f;
    float y = (float)be16(p + 12u + i * 2u) / 65535.0f;
    float y_ref = srgb_to_linear(x);
    float err = fabsf(y - y_ref);
    if (err > max_err)
      max_err = err;
    err_sum += err;
  }
  return (max_err <= 0.03f) && ((err_sum / (float)count) <= 0.01f);
}

static float icc_curve_estimate_gamma(const uint8_t *p, uint32_t count) {
  uint32_t i;
  float sum = 0.0f;
  uint32_t n = 0u;
  if (!p || count < 2u)
    return 2.2f;
  for (i = 1u; i + 1u < count; i++) {
    float x = (float)i / (float)(count - 1u);
    float y = (float)be16(p + 12u + i * 2u) / 65535.0f;
    float g;
    if (x <= 0.05f || x >= 0.95f || y <= 0.0001f || y >= 0.9999f)
      continue;
    g = logf(y) / logf(x);
    if (!isfinite(g))
      continue;
    if (g < 0.1f || g > 10.0f)
      continue;
    sum += g;
    n++;
  }
  if (n == 0u)
    return 2.2f;
  return sum / (float)n;
}

static bool parse_curve_trc(const uint8_t *buf, size_t len,
                            const ICCTagRecord *t, ICCTransfer *out_transfer) {
  const uint8_t *p;
  uint32_t count;
  if (!buf || !t || !out_transfer)
    return false;
  if ((size_t)t->offset + (size_t)t->size > len || t->size < 12u)
    return false;
  p = buf + t->offset;
  if (be32(p) != fourcc('c', 'u', 'r', 'v'))
    return false;
  count = be32(p + 8);
  if (count == 0u) {
    out_transfer->valid = true;
    out_transfer->kind = ICC_TRANSFER_LINEAR;
    out_transfer->gamma = 1.0f;
    return true;
  }
  if (count == 1u) {
    float gamma = (float)be16(p + 12) / 256.0f;
    icc_transfer_from_gamma(out_transfer, gamma);
    return true;
  }
  if (12u + (size_t)count * 2u > t->size)
    return false;
  if (icc_curve_matches_srgb(p, count)) {
    out_transfer->valid = true;
    out_transfer->kind = ICC_TRANSFER_SRGB;
    out_transfer->gamma = 2.4f;
    return true;
  }
  icc_transfer_from_gamma(out_transfer, icc_curve_estimate_gamma(p, count));
  return true;
}

static bool parse_parametric_trc(const uint8_t *buf, size_t len,
                                 const ICCTagRecord *t,
                                 ICCTransfer *out_transfer) {
  const uint8_t *p;
  uint16_t func_type;
  uint32_t param_count = 0u;
  float params[7] = {0};
  uint32_t i;
  if (!buf || !t || !out_transfer)
    return false;
  if ((size_t)t->offset + (size_t)t->size > len || t->size < 16u)
    return false;
  p = buf + t->offset;
  if (be32(p) != fourcc('p', 'a', 'r', 'a'))
    return false;
  func_type = be16(p + 8);
  switch (func_type) {
  case 0:
    param_count = 1u;
    break;
  case 1:
    param_count = 3u;
    break;
  case 2:
    param_count = 4u;
    break;
  case 3:
    param_count = 5u;
    break;
  case 4:
    param_count = 7u;
    break;
  default:
    return false;
  }
  if (12u + (size_t)param_count * 4u > t->size)
    return false;
  for (i = 0u; i < param_count; i++) {
    params[i] = s15fixed16_to_float(be32s(p + 12u + i * 4u));
  }
  if (func_type == 3u) {
    // IEC 61966-2-1 style parametric sRGB decode curve.
    const float g = params[0];
    const float a = params[1];
    const float b = params[2];
    const float c = params[3];
    const float d = params[4];
    if (fabsf(g - 2.4f) <= 0.05f && fabsf(a - (1.0f / 1.055f)) <= 0.02f &&
        fabsf(b - (0.055f / 1.055f)) <= 0.02f &&
        fabsf(c - (1.0f / 12.92f)) <= 0.02f && fabsf(d - 0.04045f) <= 0.02f) {
      out_transfer->valid = true;
      out_transfer->kind = ICC_TRANSFER_SRGB;
      out_transfer->gamma = 2.4f;
      return true;
    }
  }
  icc_transfer_from_gamma(out_transfer, params[0]);
  return true;
}

static bool parse_trc_tag(const uint8_t *buf, size_t len, const ICCTagRecord *t,
                          ICCTransfer *out_transfer) {
  const uint8_t *p;
  if (!buf || !t || !out_transfer)
    return false;
  memset(out_transfer, 0, sizeof(*out_transfer));
  if ((size_t)t->offset + (size_t)t->size > len || t->size < 12u)
    return false;
  p = buf + t->offset;
  switch (be32(p)) {
  case 0x63757276u: // 'curv'
    return parse_curve_trc(buf, len, t, out_transfer);
  case 0x70617261u: // 'para'
    return parse_parametric_trc(buf, len, t, out_transfer);
  default:
    return false;
  }
}

static void pick_transfer_from_trcs(const ICCTransfer trcs[3], bool *out_srgb,
                                    float *out_gamma) {
  uint32_t i;
  uint32_t valid_count = 0u;
  uint32_t srgb_count = 0u;
  float gamma_sum = 0.0f;
  uint32_t gamma_count = 0u;
  if (!out_srgb || !out_gamma)
    return;
  *out_srgb = true;
  *out_gamma = 2.4f;
  for (i = 0u; i < 3u; i++) {
    if (!trcs[i].valid)
      continue;
    valid_count++;
    if (trcs[i].kind == ICC_TRANSFER_SRGB) {
      srgb_count++;
    } else if (trcs[i].kind == ICC_TRANSFER_LINEAR ||
               trcs[i].kind == ICC_TRANSFER_GAMMA) {
      gamma_sum += trcs[i].gamma;
      gamma_count++;
    }
  }
  if (valid_count < 3u)
    return;
  if (srgb_count == 3u) {
    *out_srgb = true;
    *out_gamma = 2.4f;
    return;
  }
  if (gamma_count > 0u) {
    *out_srgb = false;
    *out_gamma = gamma_sum / (float)gamma_count;
    if (*out_gamma < 0.01f)
      *out_gamma = 2.2f;
  }
}

bool stygian_icc_load_profile(const char *path,
                              StygianColorProfile *out_profile,
                              StygianICCInfo *out_info) {
  uint8_t *buf = NULL;
  size_t len = 0;
  uint32_t tag_count;
  ICCTagRecord *tags = NULL;
  uint32_t i;
  uint32_t color_space_sig;
  float r_xyz[3], g_xyz[3], b_xyz[3];
  bool has_r = false, has_g = false, has_b = false;
  bool output_srgb = true;
  float output_gamma = 2.4f;
  ICCTransfer trc[3];
  bool has_all_trc = false;

  if (!path || !out_profile)
    return false;

  stygian_color_profile_init_builtin(out_profile, STYGIAN_COLOR_SPACE_SRGB);
  if (out_info) {
    memset(out_info, 0, sizeof(*out_info));
    stygian_cpystr(out_info->path, sizeof(out_info->path), path);
  }

  memset(trc, 0, sizeof(trc));

  if (!read_file(path, &buf, &len))
    return false;
  if (len < 132u)
    goto fail;
  if (be32(buf + 36) != fourcc('a', 'c', 's', 'p'))
    goto fail;

  color_space_sig = be32(buf + 16);
  if (color_space_sig != fourcc('R', 'G', 'B', ' '))
    goto fail;

  tag_count = be32(buf + 128);
  if (tag_count > 4096u)
    goto fail;
  if (128u + 4u + (size_t)tag_count * 12u > len)
    goto fail;

  tags =
      (ICCTagRecord *)calloc(tag_count ? tag_count : 1u, sizeof(ICCTagRecord));
  if (!tags)
    goto fail;
  for (i = 0; i < tag_count; i++) {
    const uint8_t *p = buf + 128u + 4u + (size_t)i * 12u;
    tags[i].sig = be32(p + 0);
    tags[i].offset = be32(p + 4);
    tags[i].size = be32(p + 8);
  }

  {
    const ICCTagRecord *d = find_tag(tags, tag_count, fourcc('d', 'e', 's', 'c'));
    const ICCTagRecord *m = find_tag(tags, tag_count, fourcc('m', 'l', 'u', 'c'));
    bool desc_ok = false;
    if (d && out_info) {
      desc_ok = parse_desc_tag(buf, len, d, out_info->description,
                               sizeof(out_info->description));
    }
    if (!desc_ok && m && out_info) {
      parse_mluc_tag(buf, len, m, out_info->description,
                     sizeof(out_info->description));
    }
  }

  {
    const ICCTagRecord *t;
    t = find_tag(tags, tag_count, fourcc('r', 'X', 'Y', 'Z'));
    if (t)
      has_r = parse_xyz_tag(buf, len, t, r_xyz);
    t = find_tag(tags, tag_count, fourcc('g', 'X', 'Y', 'Z'));
    if (t)
      has_g = parse_xyz_tag(buf, len, t, g_xyz);
    t = find_tag(tags, tag_count, fourcc('b', 'X', 'Y', 'Z'));
    if (t)
      has_b = parse_xyz_tag(buf, len, t, b_xyz);
  }

  {
    const ICCTagRecord *rt = find_tag(tags, tag_count, fourcc('r', 'T', 'R', 'C'));
    const ICCTagRecord *gt = find_tag(tags, tag_count, fourcc('g', 'T', 'R', 'C'));
    const ICCTagRecord *bt = find_tag(tags, tag_count, fourcc('b', 'T', 'R', 'C'));
    bool ok_r = false, ok_g = false, ok_b = false;
    if (rt)
      ok_r = parse_trc_tag(buf, len, rt, &trc[0]);
    if (gt)
      ok_g = parse_trc_tag(buf, len, gt, &trc[1]);
    if (bt)
      ok_b = parse_trc_tag(buf, len, bt, &trc[2]);
    has_all_trc = ok_r && ok_g && ok_b;
    if (has_all_trc) {
      pick_transfer_from_trcs(trc, &output_srgb, &output_gamma);
    }
  }

  if (has_r && has_g && has_b) {
    float m[9];
    char profile_name[64];
    m[0] = r_xyz[0];
    m[1] = g_xyz[0];
    m[2] = b_xyz[0];
    m[3] = r_xyz[1];
    m[4] = g_xyz[1];
    m[5] = b_xyz[1];
    m[6] = r_xyz[2];
    m[7] = g_xyz[2];
    m[8] = b_xyz[2];
    memset(profile_name, 0, sizeof(profile_name));
    stygian_cpystr(profile_name, sizeof(profile_name), "ICC RGB");
    if (out_info && out_info->description[0]) {
      stygian_cpystr(profile_name, sizeof(profile_name), "ICC ");
      {
        size_t base_len = strlen(profile_name);
        size_t room = (base_len < sizeof(profile_name))
                          ? (sizeof(profile_name) - base_len - 1u)
                          : 0u;
        if (room > 0u) {
          strncat(profile_name, out_info->description, room);
        }
      }
    }
    if (!has_all_trc) {
      output_srgb = true;
      output_gamma = 2.4f;
    }
    if (!stygian_color_profile_init_custom(out_profile, profile_name, m,
                                           output_srgb, output_gamma)) {
      stygian_color_profile_init_builtin(out_profile, STYGIAN_COLOR_SPACE_SRGB);
    }
  } else {
    // Heuristic fallback by description/path
    const char *probe = NULL;
    if (out_info && out_info->description[0])
      probe = out_info->description;
    else
      probe = path;
    if (probe && (strstr(probe, "P3") || strstr(probe, "Display P3") ||
                  strstr(probe, "display p3"))) {
      stygian_color_profile_init_builtin(out_profile,
                                         STYGIAN_COLOR_SPACE_DISPLAY_P3);
    } else if (probe && (strstr(probe, "2020") || strstr(probe, "BT.2020") ||
                         strstr(probe, "Rec.2020"))) {
      stygian_color_profile_init_builtin(out_profile,
                                         STYGIAN_COLOR_SPACE_BT2020);
    } else {
      stygian_color_profile_init_builtin(out_profile, STYGIAN_COLOR_SPACE_SRGB);
    }
  }

  if (out_info)
    out_info->loaded = true;
  free(tags);
  free(buf);
  return true;

fail:
  free(tags);
  free(buf);
  return false;
}
