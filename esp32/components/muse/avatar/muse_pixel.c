// Custom Muse avatar: the user's cream plush in a red cape.
//
// The art (muse_avatar_art.h) is one high-resolution still with the eyes and
// mouth painted out. This renderer keeps all of the stock avatar's life -- it
// breathes, bobs, leans, pops up on boot, hops when petted -- by warping that
// still each frame, and draws the eyes, mouth and every state effect on top at
// the canvas resolution, so nothing is chunky. The procedural fur is gone; the
// motion and the reactions are the same.
//
// Interface is unchanged (muse_pixel.h): render() computes one frame into an
// RGB565 buffer, scale() hands LVGL any rectangle of it.

#include "muse_pixel.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#define BIG_ALLOC(n) heap_caps_malloc((n), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define BIG_FREE(p) heap_caps_free((p))
#else
#include <stdlib.h>
#define BIG_ALLOC(n) malloc((n))
#define BIG_FREE(p) free((p))
#endif

#include "muse_avatar_art.h"

#define TAU 6.2831853f
#define PI 3.14159265f

/* The character is laid out in a 64-unit square, like the stock avatar, then
 * scaled to the canvas. The still fills that square. */
#define GRID 64.0f
#define PIVX 32.0f /* feet stay near here as the body squashes and bobs */
#define PIVY 53.0f

/* Per-mode glow ramp (bright -> deep) and accent, copied from the stock
 * avatar so the surrounding UI keeps its colours. */
typedef struct {
    uint32_t f[4];
    uint32_t acc;
} scheme_t;

static const scheme_t SCHEMES[MUSE_MODE_COUNT] = {
    [MUSE_MODE_BOOT] = {{0xffffff, 0xcfe0ff, 0x8fa8ff, 0x5a5fe0}, 0xa9c0ff},
    [MUSE_MODE_IDLE] = {{0xf4e8ff, 0xc7a4ff, 0x9a6bff, 0x5b3fd9}, 0xa77dff},
    [MUSE_MODE_LISTENING] = {{0xe8faff, 0x8fdcff, 0x3fa2ff, 0x2a5bd7}, 0x5cb8ff},
    [MUSE_MODE_THINKING] = {{0xffe6ff, 0xff9cf0, 0xd35bff, 0x7a2bd9}, 0xe07bff},
    [MUSE_MODE_SPEAKING] = {{0xeafff4, 0x9ff5cf, 0x3fd9a0, 0x1f9a7a}, 0x6ff0bf},
    [MUSE_MODE_ERROR] = {{0xffd6d6, 0xff6b6b, 0xc7304a, 0x6b1a3a}, 0xff5c5c},
    [MUSE_MODE_OFF] = {{0xd8d4ff, 0x8f86d9, 0x5a4fb0, 0x2e2870}, 0x7c72d0},
};

typedef struct {
    float r, g, b;
} rgb_t;

/* ----- tiny maths ----- */
static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
static inline float mixf(float a, float b, float t) { return a + (b - a) * t; }
static inline rgb_t hex_rgb(uint32_t c)
{
    return (rgb_t){((c >> 16) & 255) / 255.0f, ((c >> 8) & 255) / 255.0f, (c & 255) / 255.0f};
}
static inline rgb_t mix_rgb(rgb_t a, rgb_t b, float t)
{
    return (rgb_t){mixf(a.r, b.r, t), mixf(a.g, b.g, t), mixf(a.b, b.b, t)};
}

static uint32_t s_rng = 0x9e3779b9u;
static float frand(void)
{
    s_rng = s_rng * 1664525u + 1013904223u;
    return (s_rng >> 8) / 16777216.0f;
}

/* ----- the frame buffer ----- */
static uint16_t *s_buf;
static int s_size;
static int s_cap;

static inline void unpack(uint16_t c, rgb_t *o)
{
    o->r = ((c >> 11) & 31) / 31.0f;
    o->g = ((c >> 5) & 63) / 63.0f;
    o->b = (c & 31) / 31.0f;
}
static inline uint16_t pack(float r, float g, float b)
{
    int R = (int)(clampf(r, 0, 1) * 31 + 0.5f);
    int G = (int)(clampf(g, 0, 1) * 63 + 0.5f);
    int B = (int)(clampf(b, 0, 1) * 31 + 0.5f);
    return (uint16_t)((R << 11) | (G << 5) | B);
}

static inline void blend_px(int x, int y, rgb_t c, float a)
{
    if (a <= 0 || x < 0 || y < 0 || x >= s_size || y >= s_size) {
        return;
    }
    if (a > 1) {
        a = 1;
    }
    uint16_t *p = &s_buf[y * s_size + x];
    rgb_t b;
    unpack(*p, &b);
    *p = pack(mixf(b.r, c.r, a), mixf(b.g, c.g, a), mixf(b.b, c.b, a));
}
static inline void add_px(int x, int y, rgb_t c, float a)
{
    if (a <= 0 || x < 0 || y < 0 || x >= s_size || y >= s_size) {
        return;
    }
    uint16_t *p = &s_buf[y * s_size + x];
    rgb_t b;
    unpack(*p, &b);
    *p = pack(b.r + c.r * a, b.g + c.g * a, b.b + c.b * a);
}

/* ----- primitives, in canvas pixels ----- */
static void soft_disc(float cx, float cy, float rad, rgb_t c, float inten, bool additive)
{
    if (rad < 0.5f) {
        rad = 0.5f;
    }
    int x0 = (int)(cx - rad - 1), x1 = (int)(cx + rad + 1);
    int y0 = (int)(cy - rad - 1), y1 = (int)(cy + rad + 1);
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            float d = hypotf(x - cx, y - cy);
            float cov = clampf(rad - d + 0.5f, 0, 1) * inten;
            if (additive) {
                add_px(x, y, c, cov);
            } else {
                blend_px(x, y, c, cov);
            }
        }
    }
}

/* Soft radial glow with a smooth falloff, additive. */
static void glow(float cx, float cy, float rad, rgb_t c, float inten)
{
    int x0 = (int)(cx - rad - 1), x1 = (int)(cx + rad + 1);
    int y0 = (int)(cy - rad - 1), y1 = (int)(cy + rad + 1);
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            float d = hypotf(x - cx, y - cy) / rad;
            if (d >= 1) {
                continue;
            }
            float f = (1 - d) * (1 - d);
            add_px(x, y, c, f * inten);
        }
    }
}

static void ring(float cx, float cy, float rad, float thick, rgb_t c, float inten)
{
    int x0 = (int)(cx - rad - thick - 1), x1 = (int)(cx + rad + thick + 1);
    int y0 = (int)(cy - rad - thick - 1), y1 = (int)(cy + rad + thick + 1);
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            float d = fabsf(hypotf(x - cx, y - cy) - rad);
            float cov = clampf(thick - d + 0.5f, 0, 1) * inten;
            add_px(x, y, c, cov);
        }
    }
}

static float seg_dist(float px, float py, float ax, float ay, float bx, float by)
{
    float dx = bx - ax, dy = by - ay;
    float len2 = dx * dx + dy * dy;
    float t = len2 > 0 ? clampf(((px - ax) * dx + (py - ay) * dy) / len2, 0, 1) : 0;
    return hypotf(px - (ax + dx * t), py - (ay + dy * t));
}

static void stroke(float ax, float ay, float bx, float by, float thick, rgb_t c, float a, bool additive)
{
    int x0 = (int)(fminf(ax, bx) - thick - 1), x1 = (int)(fmaxf(ax, bx) + thick + 1);
    int y0 = (int)(fminf(ay, by) - thick - 1), y1 = (int)(fmaxf(ay, by) + thick + 1);
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            float cov = clampf(thick - seg_dist(x, y, ax, ay, bx, by) + 0.5f, 0, 1) * a;
            if (additive) {
                add_px(x, y, c, cov);
            } else {
                blend_px(x, y, c, cov);
            }
        }
    }
}

/* Filled ellipse (blended). */
static void ellipse(float cx, float cy, float rx, float ry, rgb_t c, float a)
{
    if (rx < 0.4f) {
        rx = 0.4f;
    }
    if (ry < 0.4f) {
        ry = 0.4f;
    }
    int x0 = (int)(cx - rx - 1), x1 = (int)(cx + rx + 1);
    int y0 = (int)(cy - ry - 1), y1 = (int)(cy + ry + 1);
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            float nx = (x - cx) / rx, ny = (y - cy) / ry;
            float d = sqrtf(nx * nx + ny * ny);
            blend_px(x, y, c, clampf((1 - d) * fmaxf(rx, ry), 0, 1) * a);
        }
    }
}

/* A smile/frown arc: a quadratic through (cx-hw,y) .. (cx,y+sag) .. (cx+hw,y). */
static void arc_mouth(float cx, float y, float hw, float sag, float thick, rgb_t c, float a)
{
    float px = cx - hw, py = y;
    int steps = 14;
    for (int i = 1; i <= steps; i++) {
        float t = (float)i / steps;
        float xx = cx - hw + 2 * hw * t;
        float yy = y + sag * (1 - (2 * t - 1) * (2 * t - 1));
        stroke(px, py, xx, yy, thick, c, a, false);
        px = xx;
        py = yy;
    }
}

static void sparkle(float cx, float cy, float r, rgb_t c, float inten)
{
    int x0 = (int)(cx - r - 1), x1 = (int)(cx + r + 1);
    int y0 = (int)(cy - r - 1), y1 = (int)(cy + r + 1);
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            float dx = fabsf(x - cx), dy = fabsf(y - cy);
            float diamond = clampf(1 - (dx + dy) / r, 0, 1);
            add_px(x, y, c, diamond * diamond * inten);
        }
    }
}

static void heart(float cx, float cy, float s, rgb_t c, float a)
{
    soft_disc(cx - s * 0.45f, cy - s * 0.3f, s * 0.55f, c, a, false);
    soft_disc(cx + s * 0.45f, cy - s * 0.3f, s * 0.55f, c, a, false);
    for (int i = 0; i <= 10; i++) {
        float t = i / 10.0f;
        float w = s * (1.0f - t);
        stroke(cx - w, cy - s * 0.1f + t * s * 0.9f, cx + w, cy - s * 0.1f + t * s * 0.9f, 0.6f, c, a, false);
    }
}

/* ----- eyes: blink and gaze, ported from the stock avatar ----- */
static struct {
    float last_t;
    float blink_at;
    float next_blink;
    float gx, gy, tgx, tgy;
    float next_gaze;
    bool init;
} s_eyes;

static float eyes_update(const muse_pose_t *p, float dt)
{
    if (!s_eyes.init) {
        s_eyes.next_blink = p->t + 1.5f;
        s_eyes.next_gaze = p->t + 1.0f;
        s_eyes.blink_at = -10;
        s_eyes.init = true;
    }
    if (p->t > s_eyes.next_blink) {
        s_eyes.blink_at = p->t;
        s_eyes.next_blink = p->t + 2.0f + frand() * 3.5f;
    }
    float since = p->t - s_eyes.blink_at;
    float blink = 0;
    if (since >= 0 && since < 0.16f) {
        blink = since < 0.08f ? since / 0.08f : (0.16f - since) / 0.08f;
    }
    if (p->t > s_eyes.next_gaze) {
        s_eyes.tgx = (frand() * 2 - 1) * 1.4f;
        s_eyes.tgy = (frand() * 2 - 1) * 0.9f;
        s_eyes.next_gaze = p->t + 1.0f + frand() * 2.5f;
    }
    float k = clampf(dt * 7, 0, 1);
    s_eyes.gx += (s_eyes.tgx - s_eyes.gx) * k;
    s_eyes.gy += (s_eyes.tgy - s_eyes.gy) * k;
    return clampf(blink, 0, 1);
}

/* ----- per-frame transform (64-unit space) ----- */
static float W_sx = 1, W_sy = 1, W_shear = 0, W_tx = 0, W_ty = 0;

static inline void fwd(float ax, float ay, float *sx, float *sy)
{
    float rx = (ax - PIVX) * W_sx;
    float ry = (ay - PIVY) * W_sy;
    rx += W_shear * ry;
    *sx = PIVX + rx + W_tx;
    *sy = PIVY + ry + W_ty;
}

/* Premultiplied bilinear sample of one pose, art pixel coords. */
static void sample_art(int pose, float u, float v, rgb_t *col, float *alpha)
{
    if (u < 0 || v < 0 || u >= ART_PX - 1 || v >= ART_PX - 1) {
        *alpha = 0;
        col->r = col->g = col->b = 0;
        return;
    }
    int x = (int)u, y = (int)v;
    float fx = u - x, fy = v - y;
    float w[4] = {(1 - fx) * (1 - fy), fx * (1 - fy), (1 - fx) * fy, fx * fy};
    int idx[4] = {y * ART_PX + x, y * ART_PX + x + 1, (y + 1) * ART_PX + x, (y + 1) * ART_PX + x + 1};
    float sa = 0;
    rgb_t sc = {0, 0, 0};
    for (int i = 0; i < 4; i++) {
        float a = ART_A8[pose][idx[i]] / 255.0f;
        rgb_t c;
        unpack(ART_RGB565[pose][idx[i]], &c);
        sa += w[i] * a;
        sc.r += w[i] * c.r * a;
        sc.g += w[i] * c.g * a;
        sc.b += w[i] * c.b * a;
    }
    *alpha = sa;
    if (sa > 1e-3f) {
        col->r = sc.r / sa;
        col->g = sc.g / sa;
        col->b = sc.b / sa;
    } else {
        col->r = col->g = col->b = 0;
    }
}

uint32_t muse_pixel_accent(muse_mode_t mode)
{
    if (mode < 0 || mode >= MUSE_MODE_COUNT) {
        mode = MUSE_MODE_IDLE;
    }
    return SCHEMES[mode].acc;
}

void muse_pixel_set_size(int px)
{
    if (px < 1) {
        px = 1;
    }
    if (px > 512) {
        px = 512;
    }
    if (px > s_cap) {
        uint16_t *nb = BIG_ALLOC((size_t)px * px * sizeof(uint16_t));
        if (nb) {
            if (s_buf) {
                BIG_FREE(s_buf);
            }
            s_buf = nb;
            s_cap = px;
        }
    }
    s_size = px <= s_cap ? px : s_cap;
}

void muse_pixel_scale(uint16_t *dst, int stride_px, int x0, int x1, int y0, int y1)
{
    for (int y = y0; y <= y1; y++, dst += stride_px) {
        const uint16_t *row = (s_buf && y >= 0 && y < s_size) ? &s_buf[y * s_size] : NULL;
        for (int x = x0; x <= x1; x++) {
            dst[x - x0] = (row && x >= 0 && x < s_size) ? row[x] : 0;
        }
    }
}

void muse_pixel_render(const muse_pose_t *p)
{
    if (!s_buf || s_size <= 0) {
        return;
    }
    float dt = s_eyes.last_t > 0 ? clampf(p->t - s_eyes.last_t, 0, 0.2f) : 0.04f;
    s_eyes.last_t = p->t;

    muse_mode_t mode = p->mode;
    if (mode < 0 || mode >= MUSE_MODE_COUNT) {
        mode = MUSE_MODE_IDLE;
    }
    float t = p->t, level = clampf(p->level, 0, 1), happy = clampf(p->happy, 0, 1);
    float fade = mode == MUSE_MODE_OFF ? clampf(1.0f - p->mode_t / 1.3f, 0, 1) : 1.0f;
    float boot = mode == MUSE_MODE_BOOT ? clampf(p->mode_t / 1.4f, 0, 1) : 1.0f;
    float pop = mode == MUSE_MODE_BOOT ? clampf(p->mode_t / 0.6f, 0, 1) : 1.0f;
    float squash = 1.0f - (1.0f - pop) * 0.35f + sinf(pop * PI) * 0.06f;

    /* blended scheme colours for the effects */
    static rgb_t sch[4];
    static bool sch_init;
    const scheme_t *S = &SCHEMES[mode];
    for (int i = 0; i < 4; i++) {
        rgb_t tgt = hex_rgb(S->f[i]);
        sch[i] = sch_init ? mix_rgb(sch[i], tgt, clampf(dt * 6, 0, 1)) : tgt;
    }
    sch_init = true;
    rgb_t acc = sch[1];

    /* ----- body motion ----- */
    float bob, lean = 0;
    switch (mode) {
    case MUSE_MODE_LISTENING:
        bob = sinf(t * 3.0f) * 0.6f;
        break;
    case MUSE_MODE_THINKING:
        bob = sinf(t * 2.4f) * 0.8f;
        lean = sinf(t * 1.3f) * 1.2f;
        break;
    case MUSE_MODE_SPEAKING:
        bob = sinf(t * 5.0f) * 0.6f - level * 1.5f;
        break;
    case MUSE_MODE_ERROR:
        bob = 1.0f;
        lean = sinf(t * 18.0f) * (p->mode_t < 0.6f ? 1.0f : 0.0f);
        break;
    default:
        bob = sinf(t * 1.8f) * 1.0f;
        break;
    }
    float hop = happy > 0 ? fabsf(sinf(t * 9.0f)) * 3.0f * happy : 0;
    float breathe = sinf(t * 2.0f + 1.0f) * 0.03f;

    W_sx = (1.0f + breathe) * (2.0f - squash) * (1.0f + level * 0.04f);
    W_sy = (1.0f - breathe) * squash;
    W_shear = lean * 0.05f;
    W_tx = lean;
    W_ty = bob * 0.5f - hop;

    float U = s_size / GRID;

    /* ----- background ----- */
    memset(s_buf, 0, (size_t)s_size * s_size * sizeof(uint16_t));

    float cxp = PIVX * U + W_tx * U;
    float midp = (PIVY - 16) * U + W_ty * U; /* roughly the body centre */
    float aura_s = (0.7f * boot + level * 0.45f + happy * 0.3f) * fade;
    if (aura_s > 0.01f) {
        glow(cxp, midp, 30.0f * U, mix_rgb(sch[0], sch[1], 0.5f), 0.35f * aura_s);
    }

    /* ground ripples while listening or speaking */
    if (mode == MUSE_MODE_LISTENING || mode == MUSE_MODE_SPEAKING) {
        float footy = (PIVY + 2) * U + W_ty * U;
        for (int i = 0; i < 2; i++) {
            float ph = fmodf(t * (mode == MUSE_MODE_LISTENING ? 0.9f : 1.3f) + i * 0.5f, 1.0f);
            float rad = (6 + ph * 20) * U;
            ring(cxp, footy, rad, 1.2f * U, acc, (1 - ph) * (0.3f + level * 0.5f));
        }
    }

    /* Which arm pose to crossfade toward. A partial blend ghosts an arm, so
     * these are full poses, eased in over a short moment. */
    int pose = ART_POSE_REST;
    float pose_k = 0;
    if (happy > 0.15f && mode != MUSE_MODE_ERROR) {
        pose = ART_POSE_HAPPY;
        pose_k = happy;
    } else if (mode == MUSE_MODE_LISTENING) {
        pose = ART_POSE_LISTEN;
        pose_k = clampf(p->mode_t / 0.22f, 0, 1);
    } else if (mode == MUSE_MODE_THINKING) {
        pose = ART_POSE_THINK;
        pose_k = clampf(p->mode_t / 0.22f, 0, 1);
    } else if (mode == MUSE_MODE_OFF) {
        pose = ART_POSE_WAVE;
        pose_k = clampf(1.0f - p->mode_t / 0.9f, 0, 1);
    }

    /* ----- body: warp the still over the background ----- */
    rgb_t off_tint = hex_rgb(0x9a8fd0);
    for (int iy = 0; iy < s_size; iy++) {
        for (int ix = 0; ix < s_size; ix++) {
            float fx = ix / U, fy = iy / U;
            /* inverse of fwd() */
            float dx = fx - PIVX - W_tx, dy = fy - PIVY - W_ty;
            float ry = dy, rx = dx - W_shear * ry;
            float ax = PIVX + rx / W_sx, ay = PIVY + ry / W_sy;
            float u = ax / GRID * ART_PX, v = ay / GRID * ART_PX;
            rgb_t c;
            float a;
            sample_art(ART_POSE_REST, u, v, &c, &a);
            if (pose_k > 0.004f) {
                rgb_t c2;
                float a2;
                sample_art(pose, u, v, &c2, &a2);
                float w0 = (1.0f - pose_k) * a, w1 = pose_k * a2;
                float ws = w0 + w1;
                a = ws;
                if (ws > 1e-3f) {
                    c.r = (c.r * w0 + c2.r * w1) / ws;
                    c.g = (c.g * w0 + c2.g * w1) / ws;
                    c.b = (c.b * w0 + c2.b * w1) / ws;
                }
            }
            if (a <= 0.003f) {
                continue;
            }
            if (fade < 1.0f) {
                float d = 0.35f + 0.65f * fade;
                c = mix_rgb((rgb_t){c.r * d, c.g * d, c.b * d}, off_tint, (1 - fade) * 0.25f);
            }
            blend_px(ix, iy, c, a);
        }
    }

    /* ----- face anchors, warped with the body ----- */
    float elx, ely, erx, ery, mx, my;
    fwd(ART_EYE_L_CX / ART_PX * GRID, ART_EYE_L_CY / ART_PX * GRID, &elx, &ely);
    fwd(ART_EYE_R_CX / ART_PX * GRID, ART_EYE_R_CY / ART_PX * GRID, &erx, &ery);
    fwd(ART_MOUTH_CX / ART_PX * GRID, ART_MOUTH_CY / ART_PX * GRID, &mx, &my);
    float eye_r = (ART_EYE_L_W / ART_PX * GRID) * 0.62f; /* bead radius, 64-space */

    float blink = eyes_update(p, dt);
    float open = 1.0f - blink;
    int style = 0;    /* 0 normal, 1 wide, 2 happy, 3 x */
    int mouth = 0;    /* 0 smile, 1 O, 2 hmm, 3 talk, 4 flat, 5 grin */
    float mouth_open = 0;

    switch (mode) {
    case MUSE_MODE_BOOT:
        open = p->mode_t < 0.9f ? 0.0f : clampf((p->mode_t - 0.9f) / 0.3f, 0, 1);
        break;
    case MUSE_MODE_LISTENING:
        style = 1;
        mouth = 1;
        break;
    case MUSE_MODE_THINKING:
        open *= 0.85f;
        mouth = 2;
        break;
    case MUSE_MODE_SPEAKING:
        mouth = 3;
        mouth_open = level * 1.3f + 0.1f * (0.5f + 0.5f * sinf(t * 22.0f));
        break;
    case MUSE_MODE_ERROR:
        style = 3;
        mouth = 4;
        break;
    case MUSE_MODE_OFF:
        open = clampf((1.0f - p->mode_t / 1.0f) * 1.5f, 0, 1);
        break;
    default:
        break;
    }
    if (happy > 0.2f && mode != MUSE_MODE_ERROR) {
        style = 2;
        mouth = 5;
    }

    rgb_t black = {0.04f, 0.03f, 0.03f};
    rgb_t mouthc = hex_rgb(0x3a1f1a);
    rgb_t white = {1, 1, 1};
    float gx = s_eyes.gx * U * 0.5f, gy = s_eyes.gy * U * 0.5f;

    for (int e = 0; e < 2; e++) {
        float ex = (e ? erx : elx) * U, ey = (e ? ery : ely) * U;
        float rr = eye_r * U * (style == 1 ? 1.15f : 1.0f);
        if (style == 2) { /* happy ^^ */
            stroke(ex - rr, ey + rr * 0.3f, ex, ey - rr * 0.5f, rr * 0.42f, black, 1, false);
            stroke(ex, ey - rr * 0.5f, ex + rr, ey + rr * 0.3f, rr * 0.42f, black, 1, false);
        } else if (style == 3) { /* error X */
            stroke(ex - rr, ey - rr, ex + rr, ey + rr, rr * 0.4f, black, 1, false);
            stroke(ex - rr, ey + rr, ex + rr, ey - rr, rr * 0.4f, black, 1, false);
        } else {
            float ry2 = rr * clampf(open, 0.06f, 1.2f);
            ellipse(ex + gx, ey + gy, rr, ry2, black, 1);
            if (open > 0.4f) {
                soft_disc(ex + gx - rr * 0.3f, ey + gy - ry2 * 0.35f, rr * 0.32f, white, 0.9f, false);
            }
        }
        /* brows */
        if (mode == MUSE_MODE_THINKING) {
            float by = ey - rr * 2.2f;
            stroke(ex - rr, by - (e ? 0.6f : 0) * U, ex + rr, by + (e ? 0 : 0.6f) * U, 0.5f * U, mouthc, 0.8f, false);
        } else if (mode == MUSE_MODE_LISTENING) {
            float by = ey - rr * 2.6f;
            stroke(ex - rr, by, ex + rr, by, 0.5f * U, mouthc, 0.7f, false);
        }
    }

    /* mouth */
    float mw = (ART_MOUTH_W / ART_PX * GRID) * U;
    if (mw < 2.2f * U) {
        mw = 2.2f * U;
    }
    switch (mouth) {
    case 1: /* O */
        ellipse(mx * U, my * U, mw * 0.5f, mw * 0.6f, mouthc, 1);
        break;
    case 2: /* hmm, small flat offset */
        stroke(mx * U - mw * 0.4f, my * U, mx * U + mw * 0.2f, my * U - 0.3f * U, 0.6f * U, mouthc, 1, false);
        break;
    case 3: /* talk */
        ellipse(mx * U, my * U, mw * 0.5f, clampf(mouth_open, 0.15f, 1.4f) * U * 1.6f, mouthc, 1);
        break;
    case 4: /* flat */
        stroke(mx * U - mw * 0.5f, my * U, mx * U + mw * 0.5f, my * U, 0.6f * U, mouthc, 1, false);
        break;
    case 5: /* grin */
        arc_mouth(mx * U, my * U - 0.3f * U, mw * 0.75f, 1.3f * U, 0.7f * U, mouthc, 1);
        break;
    default: /* smile */
        arc_mouth(mx * U, my * U, mw * 0.6f, 0.9f * U, 0.6f * U, mouthc, 1);
        break;
    }

    /* extra rosy cheeks when happy or speaking (the still already has soft blush) */
    float extra = happy * 0.35f + (mode == MUSE_MODE_SPEAKING ? 0.12f : 0);
    if (extra > 0.02f) {
        rgb_t blush = hex_rgb(0xf4968c);
        float cyf = (ely + 1.4f) * U;
        soft_disc((elx - 3.0f) * U, cyf, 2.2f * U, blush, extra * 0.5f, false);
        soft_disc((erx + 3.0f) * U, cyf, 2.2f * U, blush, extra * 0.5f, false);
    }

    /* ----- foreground effects ----- */
    float top = (PIVY - 30) * U + W_ty * U;
    int spk = mode == MUSE_MODE_BOOT ? (int)(boot * 6) : (int)(6 * fade);
    for (int i = 0; i < spk; i++) {
        float ang = i * (TAU / 6) + t * 0.6f;
        float rad = (24 + 3 * sinf(t * 1.3f + i)) * U;
        float tw = 0.5f + 0.5f * sinf(t * 3.0f + i * 1.7f);
        sparkle(cxp + cosf(ang) * rad, midp + sinf(ang) * rad * 0.9f, (1.1f + tw) * U, mix_rgb(sch[0], acc, 0.3f),
                (0.3f + 0.5f * tw) * fade);
    }
    if (mode == MUSE_MODE_THINKING) {
        for (int i = 0; i < 3; i++) {
            float ph = fmodf(t * 1.5f - i * 0.5f, 3.0f);
            if (ph > 1.2f) {
                continue;
            }
            soft_disc((PIVX + 13 + i * 2.5f) * U + W_tx * U, top + (2 - i) * 2.0f * U, (0.9f + i * 0.3f) * U, acc,
                      clampf(ph, 0, 1), false);
        }
    }
    if (happy > 0) {
        for (int i = 0; i < 3; i++) {
            float ph = fmodf(t * 1.2f + i * 0.4f, 1.0f);
            heart(cxp + (i - 1) * 6 * U, top - ph * 10 * U, (1.6f + 0.5f * i) * U, hex_rgb(0xff4f8b),
                  (1 - ph) * happy);
        }
    }
    if (mode == MUSE_MODE_ERROR) {
        float ex = (PIVX + 18) * U + W_tx * U, ey = top;
        rgb_t red = hex_rgb(0xff5c5c);
        stroke(ex, ey, ex, ey + 4 * U, 1.0f * U, red, 1, true);
        soft_disc(ex, ey + 6 * U, 1.0f * U, red, 1, true);
    }
}
