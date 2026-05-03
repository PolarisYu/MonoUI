#include "ui_canvas.h"
#include <string.h>
#include <stdlib.h>

/* ─── Internal pixel helpers ─────────────────────────────────────────────── */

static inline uint8_t _px_get(const uint8_t *buf, uint16_t stride, int x, int y) {
    uint32_t idx = (uint32_t)y * stride + (uint32_t)(x >> 1);
    return (x & 1) ? (buf[idx] & 0x0F) : (buf[idx] >> 4);
}

static inline void _px_set(uint8_t *buf, uint16_t stride, int x, int y, uint8_t g) {
    uint32_t idx = (uint32_t)y * stride + (uint32_t)(x >> 1);
    if (x & 1)
        buf[idx] = (buf[idx] & 0xF0) | (g & 0x0F);
    else
        buf[idx] = (buf[idx] & 0x0F) | ((g & 0x0F) << 4);
}

/*  4-bit alpha blend.  alpha4: 0=dst unchanged, 15=dst replaced by src.
 *  Uses >>4 (divides by 16) as a fast approximation of /15.
 *  Max absolute error: 1 gray level (negligible on a 16-level display).       */
static inline uint8_t _blend4(uint8_t dst, uint8_t src, uint8_t a) {
    return (uint8_t)((src * a + dst * (15u - a)) >> 4);
}

static inline uint8_t _clamp4(int v) {
    return (uint8_t)(v < 0 ? 0 : v > 15 ? 15 : v);
}

/* Blend equation dispatch */
static inline uint8_t _apply_blend(uint8_t dst, uint8_t src,
                                    uint8_t alpha4, ui_blend_mode_t mode) {
    uint8_t blended;
    switch (mode) {
        case UI_BLEND_MULTIPLY: blended = (uint8_t)((dst * src + 7) / 15); break;
        case UI_BLEND_SCREEN:   blended = (uint8_t)(15 - (15-dst)*(15-src)/15); break;
        case UI_BLEND_ADD:      blended = _clamp4(dst + src); break;
        default:                blended = src; break;                /* NORMAL */
    }
    return _blend4(dst, blended, alpha4);
}

/* ─── Clipping helpers ───────────────────────────────────────────────────── */

/* Returns false if the rectangle is fully outside the canvas. */
static bool _clip_rect(const ui_canvas_t *c,
                        int *x, int *y, int *w, int *h) {
    if (*x < 0) { *w += *x; *x = 0; }
    if (*y < 0) { *h += *y; *y = 0; }
    if (*x + *w > (int)c->w) *w = (int)c->w - *x;
    if (*y + *h > (int)c->h) *h = (int)c->h - *y;
    return (*w > 0 && *h > 0);
}

/* ─── Lifecycle ───────────────────────────────────────────────────────────── */

void ui_canvas_init(ui_canvas_t *c, uint8_t *buf, uint16_t w, uint16_t h) {
    c->buf    = buf;
    c->w      = w;
    c->h      = h;
    c->stride = (uint16_t)(w / 2u);
}

void ui_canvas_clear(ui_canvas_t *c, ui_gray_t gray) {
    uint8_t g = gray & 0x0Fu;
    memset(c->buf, (g << 4) | g, (size_t)c->stride * c->h);
}

/* ─── Pixel ───────────────────────────────────────────────────────────────── */

ui_gray_t ui_canvas_get_pixel(const ui_canvas_t *c, int x, int y) {
    if ((unsigned)x >= c->w || (unsigned)y >= c->h) return 0;
    return _px_get(c->buf, c->stride, x, y);
}

void ui_canvas_set_pixel(ui_canvas_t *c, int x, int y, ui_gray_t gray) {
    if ((unsigned)x >= c->w || (unsigned)y >= c->h) return;
    _px_set(c->buf, c->stride, x, y, gray & 0x0Fu);
}

void ui_canvas_blend_pixel(ui_canvas_t *c, int x, int y,
                            ui_gray_t gray, uint8_t alpha4) {
    if ((unsigned)x >= c->w || (unsigned)y >= c->h) return;
    uint8_t dst = _px_get(c->buf, c->stride, x, y);
    _px_set(c->buf, c->stride, x, y, _blend4(dst, gray & 0x0Fu, alpha4));
}

/* ─── Fill rect (fast byte-level inner loop) ─────────────────────────────── */

void ui_canvas_fill_rect(ui_canvas_t *c, int x, int y, int w, int h,
                          ui_gray_t gray) {
    if (!_clip_rect(c, &x, &y, &w, &h)) return;
    uint8_t g   = gray & 0x0Fu;
    uint8_t fb  = (uint8_t)((g << 4) | g);   /* fill byte: two identical pixels */

    for (int row = 0; row < h; row++) {
        uint8_t *row_ptr = c->buf + (size_t)(y + row) * c->stride;
        int      cx      = x;
        int      rem     = w;

        /* Handle odd start pixel */
        if (cx & 1) {
            row_ptr[cx >> 1] = (row_ptr[cx >> 1] & 0xF0u) | g;
            cx++; rem--;
        }

        /* Fill aligned pairs */
        int full = rem >> 1;
        if (full > 0) {
            memset(row_ptr + (cx >> 1), fb, (size_t)full);
            cx  += full << 1;
            rem -= full << 1;
        }

        /* Handle odd end pixel */
        if (rem > 0) {
            row_ptr[cx >> 1] = (g << 4) | (row_ptr[cx >> 1] & 0x0Fu);
        }
    }
}

/* ─── Gradients ──────────────────────────────────────────────────────────── */

void ui_canvas_fill_grad_h(ui_canvas_t *c, int x, int y, int w, int h,
                            ui_gray_t g_left, ui_gray_t g_right) {
    if (!_clip_rect(c, &x, &y, &w, &h)) return;
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            uint8_t g = (uint8_t)((g_left * (w - 1 - col) + g_right * col) / (w - 1 < 1 ? 1 : w - 1));
            _px_set(c->buf, c->stride, x + col, y + row, g);
        }
    }
}

void ui_canvas_fill_grad_v(ui_canvas_t *c, int x, int y, int w, int h,
                            ui_gray_t g_top, ui_gray_t g_bottom) {
    if (!_clip_rect(c, &x, &y, &w, &h)) return;
    for (int row = 0; row < h; row++) {
        uint8_t g = (uint8_t)((g_top * (h - 1 - row) + g_bottom * row) / (h - 1 < 1 ? 1 : h - 1));
        /* Fill full row at this gray */
        uint8_t fb = (uint8_t)((g << 4) | g);
        uint8_t *rp = c->buf + (size_t)(y + row) * c->stride;
        int cx = x, rem = w;
        if (cx & 1) { rp[cx >> 1] = (rp[cx >> 1] & 0xF0u) | g; cx++; rem--; }
        int full = rem >> 1;
        if (full > 0) { memset(rp + (cx >> 1), fb, (size_t)full); cx += full << 1; rem -= full << 1; }
        if (rem > 0)  { rp[cx >> 1] = (g << 4) | (rp[cx >> 1] & 0x0Fu); }
    }
}

/* ─── Rounded rectangle helper (Bresenham-style quarter circle) ──────────── */

static void _fill_h_span(ui_canvas_t *c, int x1, int x2, int y, uint8_t g) {
    if (y < 0 || y >= (int)c->h) return;
    if (x1 > x2) { int tmp = x1; x1 = x2; x2 = tmp; }
    x1 = (x1 < 0) ? 0 : x1;
    x2 = (x2 >= (int)c->w) ? (int)c->w - 1 : x2;
    if (x1 > x2) return;
    uint8_t fb = (uint8_t)((g << 4) | g);
    uint8_t *rp = c->buf + (size_t)y * c->stride;
    int cx = x1, rem = x2 - x1 + 1;
    if (cx & 1) { rp[cx >> 1] = (rp[cx >> 1] & 0xF0u) | g; cx++; rem--; }
    int full = rem >> 1;
    if (full > 0) { memset(rp + (cx >> 1), fb, (size_t)full); cx += full << 1; rem -= full << 1; }
    if (rem > 0)  { rp[cx >> 1] = (g << 4) | (rp[cx >> 1] & 0x0Fu); }
}

void ui_canvas_fill_rounded(ui_canvas_t *c, int x, int y, int w, int h,
                             int r, ui_gray_t gray) {
    uint8_t g = gray & 0x0Fu;
    if (r <= 0 || (r * 2 > w) || (r * 2 > h)) {
        ui_canvas_fill_rect(c, x, y, w, h, gray);
        return;
    }
    /* Fill the non-rounded rectangular strips */
    ui_canvas_fill_rect(c, x + r, y,     w - 2*r, h,     gray);  /* centre column */
    ui_canvas_fill_rect(c, x,     y + r, r,        h - 2*r, gray);  /* left strip */
    ui_canvas_fill_rect(c, x+w-r, y + r, r,        h - 2*r, gray);  /* right strip */

    /* Bresenham quarter circles for four corners */
    int cx = r - 1, cy = 0, err = 0;
    while (cx >= cy) {
        /* top-left */
        _fill_h_span(c, x + r - cx, x + r - 1,           y + r - cy - 1, g);
        _fill_h_span(c, x + r - cy, x + r - 1,           y + r - cx - 1, g);
        /* top-right */
        _fill_h_span(c, x + w - r, x + w - r + cx,      y + r - cy - 1, g);
        _fill_h_span(c, x + w - r, x + w - r + cy,      y + r - cx - 1, g);
        /* bottom-left */
        _fill_h_span(c, x + r - cx, x + r - 1,           y + h - r + cy, g);
        _fill_h_span(c, x + r - cy, x + r - 1,           y + h - r + cx, g);
        /* bottom-right */
        _fill_h_span(c, x + w - r, x + w - r + cx,      y + h - r + cy, g);
        _fill_h_span(c, x + w - r, x + w - r + cy,      y + h - r + cx, g);
        cy++;
        err += 2*cy - 1;
        if (err > 2*cx) { err -= 2*cx - 1; cx--; }
    }
}

/* ─── Stroked rect ───────────────────────────────────────────────────────── */

void ui_canvas_draw_rect(ui_canvas_t *c, int x, int y, int w, int h,
                          ui_gray_t gray) {
    ui_canvas_draw_h_line(c, x,         y,       w,   gray);  /* top    */
    ui_canvas_draw_h_line(c, x,         y + h-1, w,   gray);  /* bottom */
    ui_canvas_draw_v_line(c, x,         y,       h,   gray);  /* left   */
    ui_canvas_draw_v_line(c, x + w - 1, y,       h,   gray);  /* right  */
}

void ui_canvas_draw_rounded(ui_canvas_t *c, int x, int y, int w, int h,
                             int r, ui_gray_t gray) {
    /* Straight edges */
    ui_canvas_draw_h_line(c, x + r, y,       w - 2*r, gray);
    ui_canvas_draw_h_line(c, x + r, y + h-1, w - 2*r, gray);
    ui_canvas_draw_v_line(c, x,     y + r,   h - 2*r, gray);
    ui_canvas_draw_v_line(c, x+w-1, y + r,   h - 2*r, gray);

    /* Bresenham corners */
    int cx = r - 1, cy = 0, err = 0;
    uint8_t g = gray & 0x0Fu;
    while (cx >= cy) {
        _px_set(c->buf, c->stride, x + w - 1 - cx, y + r - cy - 1,     g);
        _px_set(c->buf, c->stride, x + r - 1 - cy, y + r - cx - 1,     g);
        _px_set(c->buf, c->stride, x + r - 1 - cx, y + r - cy - 1,     g);
        _px_set(c->buf, c->stride, x + r - 1 - cy, y + r - cx - 1,     g);
        _px_set(c->buf, c->stride, x + w - 1 - cx, y + h - r + cy,     g);
        _px_set(c->buf, c->stride, x + r - 1 - cy, y + h - r + cx,     g);
        _px_set(c->buf, c->stride, x + r - 1 - cx, y + h - r + cy,     g);
        _px_set(c->buf, c->stride, x + r - 1 - cy, y + h - r + cx - 1, g);
        cy++;
        err += 2*cy - 1;
        if (err > 2*cx) { err -= 2*cx - 1; cx--; }
    }
}

/* ─── Lines ───────────────────────────────────────────────────────────────── */

void ui_canvas_draw_h_line(ui_canvas_t *c, int x, int y, int len, ui_gray_t gray) {
    ui_canvas_fill_rect(c, x, y, len, 1, gray);
}

void ui_canvas_draw_v_line(ui_canvas_t *c, int x, int y, int len, ui_gray_t gray) {
    int w = 1, h = len;
    if (!_clip_rect(c, &x, &y, &w, &h)) return;
    uint8_t g = gray & 0x0Fu;
    for (int row = 0; row < h; row++)
        _px_set(c->buf, c->stride, x, y + row, g);
}

void ui_canvas_draw_line(ui_canvas_t *c, int x0, int y0, int x1, int y1,
                          ui_gray_t gray) {
    uint8_t g  = gray & 0x0Fu;
    int     dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int     dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int     err = dx + dy;

    while (1) {
        ui_canvas_set_pixel(c, x0, y0, g);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* ─── Circles ─────────────────────────────────────────────────────────────── */

void ui_canvas_draw_circle(ui_canvas_t *c, int cx, int cy, int r, ui_gray_t gray) {
    uint8_t g = gray & 0x0Fu;
    int x = 0, y = r, d = 3 - 2 * r;
    while (x <= y) {
        ui_canvas_set_pixel(c, cx+x, cy+y, g); ui_canvas_set_pixel(c, cx-x, cy+y, g);
        ui_canvas_set_pixel(c, cx+x, cy-y, g); ui_canvas_set_pixel(c, cx-x, cy-y, g);
        ui_canvas_set_pixel(c, cx+y, cy+x, g); ui_canvas_set_pixel(c, cx-y, cy+x, g);
        ui_canvas_set_pixel(c, cx+y, cy-x, g); ui_canvas_set_pixel(c, cx-y, cy-x, g);
        if (d < 0) d += 4*x + 6; else { d += 4*(x-y) + 10; y--; }
        x++;
    }
}

void ui_canvas_fill_circle(ui_canvas_t *c, int cx, int cy, int r, ui_gray_t gray) {
    uint8_t g = gray & 0x0Fu;
    int x = 0, y = r, d = 3 - 2 * r;
    while (x <= y) {
        _fill_h_span(c, cx - x, cx + x, cy + y, g);
        _fill_h_span(c, cx - x, cx + x, cy - y, g);
        _fill_h_span(c, cx - y, cx + y, cy + x, g);
        _fill_h_span(c, cx - y, cx + y, cy - x, g);
        if (d < 0) d += 4*x + 6; else { d += 4*(x-y) + 10; y--; }
        x++;
    }
}

/* ─── Blit (region copy with blend) ──────────────────────────────────────── */

void ui_canvas_blit(ui_canvas_t *dst, int dx, int dy,
                    const ui_canvas_t *src, int sx, int sy, int sw, int sh,
                    uint8_t alpha4, ui_blend_mode_t mode) {
    /* Clip source to src canvas */
    if (sx < 0) { sw += sx; dx -= sx; sx = 0; }
    if (sy < 0) { sh += sy; dy -= sy; sy = 0; }
    if (sx + sw > (int)src->w) sw = (int)src->w - sx;
    if (sy + sh > (int)src->h) sh = (int)src->h - sy;
    /* Clip dest to dst canvas */
    if (dx < 0) { sw += dx; sx -= dx; dx = 0; }
    if (dy < 0) { sh += dy; sy -= dy; dy = 0; }
    if (dx + sw > (int)dst->w) sw = (int)dst->w - dx;
    if (dy + sh > (int)dst->h) sh = (int)dst->h - dy;
    if (sw <= 0 || sh <= 0) return;

    for (int row = 0; row < sh; row++) {
        for (int col = 0; col < sw; col++) {
            uint8_t s  = _px_get(src->buf, src->stride, sx + col, sy + row);
            uint8_t d  = _px_get(dst->buf, dst->stride, dx + col, dy + row);
            uint8_t out = _apply_blend(d, s, alpha4, mode);
            _px_set(dst->buf, dst->stride, dx + col, dy + row, out);
        }
    }
}

/* ─── Fade composite (full-screen, byte-level) ───────────────────────────── */

void ui_canvas_fade(ui_canvas_t *dst,
                    const ui_canvas_t *a, const ui_canvas_t *b,
                    uint8_t alpha4) {
    /* Process two pixels per iteration (one byte) */
    uint32_t n = (uint32_t)UI_FB_SIZE;
    for (uint32_t i = 0; i < n; i++) {
        uint8_t pa = a->buf[i], pb = b->buf[i];
        uint8_t hi = _blend4(pa >> 4,   pb >> 4,   alpha4);
        uint8_t lo = _blend4(pa & 0x0F, pb & 0x0F, alpha4);
        dst->buf[i] = (uint8_t)((hi << 4) | lo);
    }
}

/* ─── Slide composites ────────────────────────────────────────────────────── */

/*  split (0..256, even): old page occupies x=[0, split), new page x=[split,255]
 *  For slide-left (push): split goes 256 → 0 as progress 0 → 1.               */
void ui_canvas_slide_left(ui_canvas_t *dst,
                           const ui_canvas_t *old_pg,
                           const ui_canvas_t *new_pg,
                           int split) {
    /* Force even for clean byte alignment */
    split = split & ~1;
    if (split < 0) split = 0;
    if (split > (int)UI_SCREEN_W) split = (int)UI_SCREEN_W;

    int split_bytes = split >> 1;

    for (int row = 0; row < (int)UI_SCREEN_H; row++) {
        uint8_t       *d  = dst->buf    + (size_t)row * dst->stride;
        const uint8_t *op = old_pg->buf + (size_t)row * old_pg->stride;
        const uint8_t *np = new_pg->buf + (size_t)row * new_pg->stride;

        if (split_bytes > 0)
            memcpy(d, op, (size_t)split_bytes);
        if (split_bytes < (int)UI_FB_STRIDE)
            memcpy(d + split_bytes, np, (size_t)(UI_FB_STRIDE - split_bytes));
    }
}

/*  For slide-right (pop): new page reveals from left, split goes 0 → 256.
 *  new_pg shows its RIGHT portion on the LEFT of screen.                        */
void ui_canvas_slide_right(ui_canvas_t *dst,
                            const ui_canvas_t *old_pg,
                            const ui_canvas_t *new_pg,
                            int split) {
    split = split & ~1;
    if (split < 0) split = 0;
    if (split > (int)UI_SCREEN_W) split = (int)UI_SCREEN_W;

    int split_bytes      = split >> 1;
    int new_start_bytes  = (int)UI_FB_STRIDE - split_bytes; /* byte offset in new_pg row */

    for (int row = 0; row < (int)UI_SCREEN_H; row++) {
        uint8_t       *d  = dst->buf    + (size_t)row * dst->stride;
        const uint8_t *op = old_pg->buf + (size_t)row * old_pg->stride;
        const uint8_t *np = new_pg->buf + (size_t)row * new_pg->stride;

        /* Left side: new page's right portion */
        if (split_bytes > 0)
            memcpy(d, np + new_start_bytes, (size_t)split_bytes);
        /* Right side: old page's left portion */
        if (split_bytes < (int)UI_FB_STRIDE)
            memcpy(d + split_bytes, op + split_bytes, (size_t)(UI_FB_STRIDE - split_bytes));
    }
}

/* ─── Global dim ──────────────────────────────────────────────────────────── */

void ui_canvas_apply_dim(ui_canvas_t *c, uint8_t factor4) {
    if (factor4 >= 15u) return;  /* no-op */
    uint32_t n = (uint32_t)c->stride * c->h;
    for (uint32_t i = 0; i < n; i++) {
        uint8_t p  = c->buf[i];
        uint8_t hi = (uint8_t)((p >> 4)   * factor4 >> 4);
        uint8_t lo = (uint8_t)((p & 0x0F) * factor4 >> 4);
        c->buf[i]  = (uint8_t)((hi << 4) | lo);
    }
}
