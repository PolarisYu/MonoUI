#include "ui_widget.h"
#include <string.h>

/* ─── Base lifecycle ──────────────────────────────────────────────────────── */

void ui_widget_init(ui_widget_t *w, int16_t x, int16_t y,
                    uint16_t width, uint16_t height,
                    const ui_widget_vtbl_t *vtbl) {
    memset(w, 0, sizeof(*w));
    w->x        = x;
    w->y        = y;
    w->w        = width;
    w->h        = height;
    w->alpha    = 1.0f;
    w->dim      = 1.0f;
    w->visible  = true;
    w->dirty    = true;
    w->vtbl     = vtbl;
}

void ui_widget_add_child(ui_widget_t *parent, ui_widget_t *child) {
    child->parent   = parent;
    child->next_sib = NULL;
    if (!parent->child_head) {
        parent->child_head = child;
        parent->child_tail = child;
    } else {
        parent->child_tail->next_sib = child;
        parent->child_tail           = child;
    }
    ui_widget_mark_dirty(parent);
}

void ui_widget_remove_child(ui_widget_t *parent, ui_widget_t *child) {
    ui_widget_t **cur = &parent->child_head;
    while (*cur) {
        if (*cur == child) {
            *cur = child->next_sib;
            if (parent->child_tail == child)
                parent->child_tail = NULL;
            child->parent   = NULL;
            child->next_sib = NULL;
            ui_widget_mark_dirty(parent);
            return;
        }
        cur = &(*cur)->next_sib;
    }
}

void ui_widget_mark_dirty(ui_widget_t *w) {
    w->dirty = true;
    if (w->parent) ui_widget_mark_dirty(w->parent);
}

void ui_widget_set_visible(ui_widget_t *w, bool visible) {
    if (w->visible != visible) {
        w->visible = visible;
        ui_widget_mark_dirty(w);
    }
}

/* ─── Render traversal ────────────────────────────────────────────────────── */

/*  Convert widget alpha+dim to a 0-15 canvas alpha.
 *  alpha: 0.0-1.0 → 0-15
 *  dim:   0.0-1.0 (multiplied)                                                */
static uint8_t _widget_alpha4(const ui_widget_t *w) {
    float a = w->alpha * w->dim;
    if (a <= 0.f) return 0;
    if (a >= 1.f) return 15;
    return (uint8_t)(a * 15.f + 0.5f);
}

void ui_widget_render(ui_widget_t *w, ui_canvas_t *canvas,
                      int parent_abs_x, int parent_abs_y) {
    if (!w || !w->visible) return;

    /* Accumulate absolute position (include animation translation) */
    int abs_x = parent_abs_x + w->x + (int)w->offset_x;
    int abs_y = parent_abs_y + w->y + (int)w->offset_y;

    /* Draw self */
    if (w->vtbl && w->vtbl->draw) {
        w->vtbl->draw(w, canvas, abs_x, abs_y);
    }

    /* Draw children */
    for (ui_widget_t *c = w->child_head; c; c = c->next_sib) {
        ui_widget_render(c, canvas, abs_x, abs_y);
    }

    w->dirty = false;
}

/* ─── Event dispatch ──────────────────────────────────────────────────────── */

/*  Simple depth-first search: dispatch to focused widget, then propagate up.  */
void ui_widget_dispatch(ui_widget_t *root, const ui_event_t *evt) {
    if (!root) return;
    /* Find first focused leaf */
    ui_widget_t *node = root;
    while (node) {
        if (node->focused && node->vtbl && node->vtbl->on_event) {
            node->vtbl->on_event(node, evt);
            return;
        }
        for (ui_widget_t *c = node->child_head; c; c = c->next_sib) {
            if (c->focused) { node = c; break; }
        }
        break;
    }
    /* Fallback: dispatch to root */
    if (root->vtbl && root->vtbl->on_event)
        root->vtbl->on_event(root, evt);
}

/* ─── Convenience tween wrapper ───────────────────────────────────────────── */

ui_anim_id_t ui_widget_animate(float *prop, float from, float to,
                                uint32_t dur_ms, ui_ease_fn_t ease,
                                ui_anim_cb_t cb, void *ctx) {
    ui_anim_id_t id = ui_anim_create(prop, from, to, dur_ms, 0,
                                      ease ? ease : ui_ease_out_cubic,
                                      cb, ctx);
    if (id == UI_ANIM_INVALID) {
        /* Fail soft when the pool is exhausted: keep UI state correct even if
           the transition has to snap instead of animate. */
        if (prop) *prop = to;
        if (cb) cb(ctx);
        return id;
    }
    ui_anim_start(id);
    return id;
}

/* ════════════════════════════════════════════════════════════════════════════
 * Built-in widget implementations
 * ════════════════════════════════════════════════════════════════════════════ */

/* ─── Rect ────────────────────────────────────────────────────────────────── */

static void rect_draw(ui_widget_t *self, ui_canvas_t *canvas, int ax, int ay) {
    ui_rect_t *r = (ui_rect_t *)self;
    uint8_t a4 = _widget_alpha4(self);
    if (a4 == 0) return;

    int x = ax, y = ay, w = (int)self->w, h = (int)self->h;

    if (r->filled) {
        if (a4 == 15) {
            /* Opaque: direct fill — no blend overhead */
            if (r->gradient) {
                if (r->corner_r > 0)
                    ui_canvas_fill_rounded(canvas, x, y, w, h, r->corner_r, r->fill_gray);
                ui_canvas_fill_grad_v(canvas, x, y, w, h, r->fill_gray, r->fill_gray2);
            } else {
                if (r->corner_r > 0)
                    ui_canvas_fill_rounded(canvas, x, y, w, h, r->corner_r, r->fill_gray);
                else
                    ui_canvas_fill_rect(canvas, x, y, w, h, r->fill_gray);
            }
        } else {
            /* Semi-transparent: blit with alpha */
            /* For simplicity, render opaque then blit. A more memory-efficient
               path would blend per-pixel directly here.                       */
            ui_canvas_fill_rect(canvas, x, y, w, h, r->fill_gray);
            /* TODO: proper alpha path — blit from off-screen in a future pass */
        }
    }

    if (r->border_w > 0) {
        for (uint8_t i = 0; i < r->border_w; i++) {
            if (r->corner_r > 0)
                ui_canvas_draw_rounded(canvas, x+i, y+i, w-2*i, h-2*i, r->corner_r-i, r->border_gray);
            else
                ui_canvas_draw_rect(canvas, x+i, y+i, w-2*i, h-2*i, r->border_gray);
        }
    }
}

static const ui_widget_vtbl_t s_rect_vtbl = { rect_draw, NULL, NULL };

void ui_rect_init(ui_rect_t *r, int16_t x, int16_t y,
                  uint16_t w, uint16_t h, ui_gray_t fill) {
    ui_widget_init(&r->base, x, y, w, h, &s_rect_vtbl);
    r->fill_gray   = fill;
    r->fill_gray2  = fill;
    r->border_gray = UI_GRAY_WHITE;
    r->border_w    = 0;
    r->corner_r    = 0;
    r->filled      = true;
    r->gradient    = false;
}

/* ─── Font glyph renderers ────────────────────────────────────────────────── */

static void _draw_glyph_1bpp(ui_canvas_t *c, int cx, int cy,
                              const ui_font_t *font, uint8_t ci,
                              ui_gray_t fg, uint8_t alpha4) {
    int bpr = (font->glyph_w + 7) / 8;
    const uint8_t *glyph = font->data + (size_t)ci * (size_t)bpr * font->glyph_h;
    for (int gy = 0; gy < (int)font->glyph_h; gy++) {
        for (int gx = 0; gx < (int)font->glyph_w; gx++) {
            uint8_t byte = glyph[gy * bpr + gx / 8];
            if (byte & (0x80u >> (gx & 7))) {
                if (alpha4 >= 15)
                    ui_canvas_set_pixel(c, cx + gx, cy + gy, fg);
                else
                    ui_canvas_blend_pixel(c, cx + gx, cy + gy, fg, alpha4);
            }
        }
    }
}

static void _draw_glyph_4bpp(ui_canvas_t *c, int cx, int cy,
                              const ui_font_t *font, uint8_t ci,
                              ui_gray_t fg, uint8_t alpha4) {
    int bpr = (font->glyph_w + 1) / 2;
    const uint8_t *glyph = font->data + (size_t)ci * (size_t)bpr * font->glyph_h;
    for (int gy = 0; gy < (int)font->glyph_h; gy++) {
        for (int gx = 0; gx < (int)font->glyph_w; gx++) {
            uint8_t byte     = glyph[gy * bpr + gx / 2];
            uint8_t coverage = (gx & 1) ? (byte & 0x0Fu) : (byte >> 4);
            if (coverage == 0) continue;
            /* Final alpha = coverage * widget_alpha (multiply) */
            uint8_t eff = (uint8_t)(((uint16_t)coverage * alpha4) >> 4);
            ui_canvas_blend_pixel(c, cx + gx, cy + gy, fg, eff);
        }
    }
}

/* ─── Label ───────────────────────────────────────────────────────────────── */

static void label_draw(ui_widget_t *self, ui_canvas_t *canvas, int ax, int ay) {
    ui_label_t *lbl = (ui_label_t *)self;
    uint8_t a4 = _widget_alpha4(self);
    if (a4 == 0 || !lbl->text || !lbl->font) return;

    if (!lbl->transparent_bg)
        ui_canvas_fill_rect(canvas, ax, ay, self->w, self->h, lbl->bg_gray);

    /* Compute text width for alignment */
    int text_w = 0;
    for (const char *p = lbl->text; *p; p++)
        text_w += lbl->font->glyph_w + lbl->font->glyph_gap;
    if (text_w > 0) text_w -= lbl->font->glyph_gap;

    int start_x = ax;
    if (lbl->align == UI_ALIGN_CENTER)
        start_x = ax + ((int)self->w - text_w) / 2;
    else if (lbl->align == UI_ALIGN_RIGHT)
        start_x = ax + (int)self->w - text_w;

    int draw_y = ay + ((int)self->h - (int)lbl->font->glyph_h) / 2;

    for (const char *p = lbl->text; *p; p++) {
        uint8_t ch = (uint8_t)*p;
        if (ch < lbl->font->first_char ||
            ch >= lbl->font->first_char + lbl->font->num_chars) {
            start_x += lbl->font->glyph_w + lbl->font->glyph_gap;
            continue;
        }
        uint8_t ci = ch - lbl->font->first_char;
        if (lbl->font->bpp == 4)
            _draw_glyph_4bpp(canvas, start_x, draw_y, lbl->font, ci, lbl->fg_gray, a4);
        else
            _draw_glyph_1bpp(canvas, start_x, draw_y, lbl->font, ci, lbl->fg_gray, a4);
        start_x += lbl->font->glyph_w + lbl->font->glyph_gap;
    }
}

static const ui_widget_vtbl_t s_label_vtbl = { label_draw, NULL, NULL };

void ui_label_init(ui_label_t *lbl, int16_t x, int16_t y,
                   uint16_t w, uint16_t h,
                   const char *text, const ui_font_t *font,
                   ui_gray_t fg, ui_gray_t bg, bool transparent_bg) {
    ui_widget_init(&lbl->base, x, y, w, h, &s_label_vtbl);
    lbl->text           = text;
    lbl->font           = font;
    lbl->fg_gray        = fg;
    lbl->bg_gray        = bg;
    lbl->transparent_bg = transparent_bg;
    lbl->align          = UI_ALIGN_LEFT;
}

void ui_label_set_text(ui_label_t *lbl, const char *text) {
    lbl->text = text;
    ui_widget_mark_dirty(&lbl->base);
}

/* ─── Progress bar ────────────────────────────────────────────────────────── */

static void progress_draw(ui_widget_t *self, ui_canvas_t *canvas, int ax, int ay) {
    ui_progress_t *p  = (ui_progress_t *)self;
    uint8_t        a4 = _widget_alpha4(self);
    if (a4 == 0) return;

    int x = ax, y = ay, w = (int)self->w, h = (int)self->h;
    int fill_w = (int)(p->value * (float)w + 0.5f);
    if (fill_w < 0) fill_w = 0;
    if (fill_w > w) fill_w = w;

    /* Track */
    if (p->corner_r > 0)
        ui_canvas_fill_rounded(canvas, x, y, w, h, p->corner_r, p->track_gray);
    else
        ui_canvas_fill_rect(canvas, x, y, w, h, p->track_gray);

    /* Fill */
    if (fill_w > 0) {
        if (p->gradient && fill_w > 1)
            ui_canvas_fill_grad_h(canvas, x, y, fill_w, h, p->fill_gray, p->fill_gray2);
        else
            ui_canvas_fill_rect(canvas, x, y, fill_w, h, p->fill_gray);
    }
}

static const ui_widget_vtbl_t s_progress_vtbl = { progress_draw, NULL, NULL };

void ui_progress_init(ui_progress_t *p, int16_t x, int16_t y,
                      uint16_t w, uint16_t h,
                      ui_gray_t track, ui_gray_t fill) {
    ui_widget_init(&p->base, x, y, w, h, &s_progress_vtbl);
    p->value      = 0.f;
    p->track_gray = track;
    p->fill_gray  = fill;
    p->fill_gray2 = fill;
    p->corner_r   = 0;
    p->gradient   = false;
}

/* ─── Image ───────────────────────────────────────────────────────────────── */

static void image_draw(ui_widget_t *self, ui_canvas_t *canvas, int ax, int ay) {
    ui_image_t *img = (ui_image_t *)self;
    if (!img->data) return;

    /* Create a temporary read-only canvas view of the image data */
    ui_canvas_t src = {
        .buf    = (uint8_t *)img->data,  /* const cast for struct, read-only */
        .w      = img->img_w,
        .h      = img->img_h,
        .stride = (uint16_t)(img->img_w / 2u),
    };

    uint8_t a4 = (uint8_t)(((uint16_t)img->alpha4 * _widget_alpha4(self)) >> 4);
    ui_canvas_blit(canvas, ax, ay, &src, 0, 0, img->img_w, img->img_h,
                   a4, img->blend);
}

static const ui_widget_vtbl_t s_image_vtbl = { image_draw, NULL, NULL };

void ui_image_init(ui_image_t *img, int16_t x, int16_t y,
                   const uint8_t *data, uint16_t img_w, uint16_t img_h) {
    ui_widget_init(&img->base, x, y, img_w, img_h, &s_image_vtbl);
    img->data   = data;
    img->img_w  = img_w;
    img->img_h  = img_h;
    img->alpha4 = 15;
    img->blend  = UI_BLEND_NORMAL;
}
