#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "ui_conf.h"

/* ═══════════════════════════════════════════════════════════════════════════
 * Canvas — a view onto a 4bpp packed pixel buffer.
 *
 * Pixel format (native SSD1322):
 *   byte[i] = (pixel[2i] << 4) | pixel[2i+1]
 *   even-column pixel → high nibble
 *   odd-column  pixel → low  nibble
 *
 * Gray values: 0x0 = black … 0xF = white (matches SSD1322 directly)
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef uint8_t ui_gray_t;   /* 0x0 – 0xF */

/* Semantic gray design tokens */
#define UI_GRAY_BLACK      ((ui_gray_t)0x0)
#define UI_GRAY_SHADOW     ((ui_gray_t)0x2)
#define UI_GRAY_DIM        ((ui_gray_t)0x4)
#define UI_GRAY_MID        ((ui_gray_t)0x7)
#define UI_GRAY_MUTED      ((ui_gray_t)0x9)
#define UI_GRAY_LIGHT      ((ui_gray_t)0xC)
#define UI_GRAY_BRIGHT     ((ui_gray_t)0xE)
#define UI_GRAY_WHITE      ((ui_gray_t)0xF)

typedef enum {
    UI_BLEND_NORMAL   = 0, /* standard alpha composite          */
    UI_BLEND_MULTIPLY,     /* darken: out = dst * src / 15      */
    UI_BLEND_SCREEN,       /* lighten: out = 15 – (15-dst)*(15-src)/15 */
    UI_BLEND_ADD,          /* additive: out = min(dst+src, 15)  */
} ui_blend_mode_t;

typedef struct {
    uint8_t  *buf;     /* pixel data: (stride * h) bytes           */
    uint16_t  w;       /* width  in pixels                          */
    uint16_t  h;       /* height in pixels                          */
    uint16_t  stride;  /* bytes per row (== w/2 for full-row canvas)*/
} ui_canvas_t;

/* ─── Lifecycle ───────────────────────────────────────────────────────────── */
void ui_canvas_init (ui_canvas_t *c, uint8_t *buf, uint16_t w, uint16_t h);
void ui_canvas_clear(ui_canvas_t *c, ui_gray_t gray);

/* ─── Pixel ───────────────────────────────────────────────────────────────── */
ui_gray_t ui_canvas_get_pixel  (const ui_canvas_t *c, int x, int y);
void      ui_canvas_set_pixel  (ui_canvas_t *c, int x, int y, ui_gray_t gray);
void      ui_canvas_blend_pixel(ui_canvas_t *c, int x, int y,
                                 ui_gray_t gray, uint8_t alpha4);

/* ─── Filled shapes ───────────────────────────────────────────────────────── */
void ui_canvas_fill_rect      (ui_canvas_t *c, int x, int y, int w, int h,
                                ui_gray_t gray);
void ui_canvas_fill_grad_h    (ui_canvas_t *c, int x, int y, int w, int h,
                                ui_gray_t g_left, ui_gray_t g_right);
void ui_canvas_fill_grad_v    (ui_canvas_t *c, int x, int y, int w, int h,
                                ui_gray_t g_top, ui_gray_t g_bottom);
void ui_canvas_fill_rounded   (ui_canvas_t *c, int x, int y, int w, int h,
                                int r, ui_gray_t gray);

/* ─── Stroked shapes ──────────────────────────────────────────────────────── */
void ui_canvas_draw_rect      (ui_canvas_t *c, int x, int y, int w, int h,
                                ui_gray_t gray);
void ui_canvas_draw_rounded   (ui_canvas_t *c, int x, int y, int w, int h,
                                int r, ui_gray_t gray);
void ui_canvas_draw_line      (ui_canvas_t *c, int x0, int y0, int x1, int y1,
                                ui_gray_t gray);
void ui_canvas_draw_h_line    (ui_canvas_t *c, int x, int y, int len, ui_gray_t gray);
void ui_canvas_draw_v_line    (ui_canvas_t *c, int x, int y, int len, ui_gray_t gray);
void ui_canvas_draw_circle    (ui_canvas_t *c, int cx, int cy, int r, ui_gray_t gray);
void ui_canvas_fill_circle    (ui_canvas_t *c, int cx, int cy, int r, ui_gray_t gray);

/* ─── Compositing ─────────────────────────────────────────────────────────── */

/*  Blit src region onto dst at (dx, dy).
 *  alpha4:   0=transparent, 15=opaque
 *  mode:     blend equation for src over dst                                  */
void ui_canvas_blit(ui_canvas_t *dst, int dx, int dy,
                    const ui_canvas_t *src, int sx, int sy, int sw, int sh,
                    uint8_t alpha4, ui_blend_mode_t mode);

/*  Fade between two full-screen canvases, result → dst.
 *  alpha4: 0=all a, 15=all b                                                  */
void ui_canvas_fade(ui_canvas_t *dst,
                    const ui_canvas_t *a, const ui_canvas_t *b,
                    uint8_t alpha4);

/*  Slide-left composite (push transition).
 *  split (0..256, even): columns [0,split) from old_pg, [split,255] from new_pg */
void ui_canvas_slide_left (ui_canvas_t *dst,
                            const ui_canvas_t *old_pg,
                            const ui_canvas_t *new_pg,
                            int split);

/*  Slide-right composite (pop transition). Reverse of slide_left.              */
void ui_canvas_slide_right(ui_canvas_t *dst,
                            const ui_canvas_t *old_pg,
                            const ui_canvas_t *new_pg,
                            int split);

/*  Multiply every pixel by factor/15 (global dim; factor=15 → no change).     */
void ui_canvas_apply_dim  (ui_canvas_t *c, uint8_t factor4);
