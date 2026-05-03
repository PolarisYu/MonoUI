#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "ui_canvas.h"
#include "ui_anim.h"

/* ═══════════════════════════════════════════════════════════════════════════
 * Widget system
 *
 * C-style OOP with a vtable.  Concrete widgets (rect, label, image …) embed
 * ui_widget_t as their first member and cast freely between base and derived.
 *
 * Coordinate system: each widget's (x, y) is relative to its parent.
 * The render traversal accumulates absolute position before calling draw().
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct ui_widget_s ui_widget_t;

/* ─── Input events ────────────────────────────────────────────────────────── */

typedef enum {
    UI_EVT_NONE = 0,
    UI_EVT_BTN_PRESS,
    UI_EVT_BTN_RELEASE,
    UI_EVT_BTN_LONG_PRESS,
    UI_EVT_ENCODER_CW,      /* encoder clockwise step        */
    UI_EVT_ENCODER_CCW,     /* encoder counter-clockwise step */
    UI_EVT_FOCUS_IN,
    UI_EVT_FOCUS_OUT,
} ui_evt_type_t;

typedef struct {
    ui_evt_type_t type;
    int32_t       value;    /* button id, encoder delta, etc. */
} ui_event_t;

/* ─── Widget virtual table ────────────────────────────────────────────────── */

typedef struct {
    /* Called during render traversal.  abs_x/abs_y = accumulated position. */
    void (*draw)    (ui_widget_t *self, ui_canvas_t *canvas, int abs_x, int abs_y);
    /* Called when an event reaches this widget. */
    void (*on_event)(ui_widget_t *self, const ui_event_t *evt);
    /* Optional destructor (release dynamic resources). */
    void (*destroy) (ui_widget_t *self);
} ui_widget_vtbl_t;

/* ─── Base widget ─────────────────────────────────────────────────────────── */

struct ui_widget_s {
    int16_t   x, y;          /* position relative to parent (px)             */
    uint16_t  w, h;          /* size (px)                                     */

    /* Animatable properties — point ui_anim_create() targets at these:       */
    float     alpha;          /* 0.0 = fully transparent … 1.0 = opaque       */
    float     offset_x;      /* translation (added to x during render)        */
    float     offset_y;
    float     dim;            /* 0.0 = black … 1.0 = normal (gray multiplier) */

    bool      visible;
    bool      dirty;
    bool      focusable;
    bool      focused;

    ui_widget_t            *parent;
    ui_widget_t            *child_head;   /* first child (intrusive LL)        */
    ui_widget_t            *child_tail;
    ui_widget_t            *next_sib;     /* next sibling                      */

    const ui_widget_vtbl_t *vtbl;
};

/* ─── Font descriptor ─────────────────────────────────────────────────────── */

/*  Supports 1-bpp bitmap fonts and 4-bpp anti-aliased fonts.
 *
 *  1-bpp layout: ceil(glyph_w / 8) bytes/row × glyph_h rows per glyph.
 *      Bit order: MSB = leftmost pixel.
 *
 *  4-bpp layout: ceil(glyph_w / 2) bytes/row × glyph_h rows per glyph.
 *      High nibble = even-column pixel, low nibble = odd-column pixel.
 *      Coverage 0 = transparent, 15 = fully opaque.                           */

typedef struct {
    uint8_t        glyph_w;      /* glyph width  in pixels                    */
    uint8_t        glyph_h;      /* glyph height in pixels                    */
    uint8_t        glyph_gap;    /* horizontal spacing between glyphs          */
    uint8_t        first_char;   /* ASCII code of first glyph                  */
    uint8_t        num_chars;    /* number of glyphs                           */
    uint8_t        bpp;          /* 1 or 4                                     */
    const uint8_t *data;         /* glyph bitmap data                          */
} ui_font_t;

/* ─── Widget lifecycle ────────────────────────────────────────────────────── */

/*  Initialise base fields.  Call before using any widget.                      */
void ui_widget_init(ui_widget_t *w, int16_t x, int16_t y,
                    uint16_t width, uint16_t height,
                    const ui_widget_vtbl_t *vtbl);

void ui_widget_add_child   (ui_widget_t *parent, ui_widget_t *child);
void ui_widget_remove_child(ui_widget_t *parent, ui_widget_t *child);
void ui_widget_mark_dirty  (ui_widget_t *w);
void ui_widget_set_visible (ui_widget_t *w, bool visible);

/*  Render w and all its children onto canvas.
 *  parent_abs_x/y: accumulated absolute position from the widget's ancestors. */
void ui_widget_render(ui_widget_t *w, ui_canvas_t *canvas,
                      int parent_abs_x, int parent_abs_y);

/*  Dispatch an event down the focused path of the widget tree.                 */
void ui_widget_dispatch(ui_widget_t *root, const ui_event_t *evt);

/* ─── Helper: animate a widget property ──────────────────────────────────── */

/*  Convenience wrapper that creates AND starts a tween on a widget property.   */
ui_anim_id_t ui_widget_animate(float *prop, float from, float to,
                                uint32_t dur_ms, ui_ease_fn_t ease,
                                ui_anim_cb_t cb, void *ctx);

/* ════════════════════════════════════════════════════════════════════════════
 * Built-in concrete widgets
 * ════════════════════════════════════════════════════════════════════════════ */

/* ─── Rect ────────────────────────────────────────────────────────────────── */

typedef struct {
    ui_widget_t  base;
    ui_gray_t    fill_gray;
    ui_gray_t    border_gray;
    uint8_t      border_w;        /* 0 = no border */
    int          corner_r;        /* corner radius; 0 = square */
    bool         filled;
    bool         gradient;        /* fill with vertical gradient if true */
    ui_gray_t    fill_gray2;      /* gradient bottom color (if gradient=true) */
} ui_rect_t;

void ui_rect_init(ui_rect_t *r, int16_t x, int16_t y,
                  uint16_t w, uint16_t h, ui_gray_t fill);

/* ─── Label ───────────────────────────────────────────────────────────────── */

typedef enum {
    UI_ALIGN_LEFT   = 0,
    UI_ALIGN_CENTER,
    UI_ALIGN_RIGHT,
} ui_text_align_t;

typedef struct {
    ui_widget_t      base;
    const char      *text;
    const ui_font_t *font;
    ui_gray_t        fg_gray;
    ui_gray_t        bg_gray;
    bool             transparent_bg;
    ui_text_align_t  align;
} ui_label_t;

void ui_label_init(ui_label_t *lbl, int16_t x, int16_t y,
                   uint16_t w, uint16_t h,
                   const char *text, const ui_font_t *font,
                   ui_gray_t fg, ui_gray_t bg, bool transparent_bg);

void ui_label_set_text(ui_label_t *lbl, const char *text);

/* ─── Progress bar ────────────────────────────────────────────────────────── */

typedef struct {
    ui_widget_t  base;
    float        value;          /* 0.0 – 1.0 (animatable)                    */
    ui_gray_t    track_gray;
    ui_gray_t    fill_gray;
    int          corner_r;
    bool         gradient;
    ui_gray_t    fill_gray2;
} ui_progress_t;

void ui_progress_init(ui_progress_t *p, int16_t x, int16_t y,
                      uint16_t w, uint16_t h,
                      ui_gray_t track, ui_gray_t fill);

/* ─── Image (4bpp bitmap from flash/ROM) ─────────────────────────────────── */

typedef struct {
    ui_widget_t    base;
    const uint8_t *data;         /* 4bpp packed bitmap, same format as canvas */
    uint16_t       img_w;
    uint16_t       img_h;
    uint8_t        alpha4;       /* global alpha override (0-15)              */
    ui_blend_mode_t blend;
} ui_image_t;

void ui_image_init(ui_image_t *img, int16_t x, int16_t y,
                   const uint8_t *data, uint16_t img_w, uint16_t img_h);
