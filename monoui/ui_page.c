#include "ui_page.h"
#include <string.h>

/* ─── Init ────────────────────────────────────────────────────────────────── */

void ui_page_init(ui_page_t *page, ui_widget_t *root,
                  ui_page_cb_t on_appear, ui_page_cb_t on_disappear) {
    page->root         = root;
    page->on_appear    = on_appear;
    page->on_disappear = on_disappear;
    page->user_data    = NULL;
}

void ui_page_manager_init(ui_page_manager_t *pm,
                           ui_canvas_t *main_canvas,
                           ui_canvas_t *trans_canvas) {
    memset(pm, 0, sizeof(*pm));
    pm->main_canvas   = main_canvas;
    pm->trans_canvas  = trans_canvas;
    pm->stack_top     = -1;
    pm->state         = UI_PM_IDLE;
    pm->trans_anim    = UI_ANIM_INVALID;
    pm->trans_progress = 0.f;
}

/* ─── Internal: begin transition ──────────────────────────────────────────── */

static void _begin_transition(ui_page_manager_t *pm,
                               ui_page_t *outgoing,
                               ui_page_t *incoming,
                               ui_trans_type_t trans,
                               uint32_t duration_ms,
                               bool is_push) {
    pm->outgoing       = outgoing;
    pm->incoming       = incoming;
    pm->trans_type     = trans;
    pm->is_push        = is_push;
    pm->trans_progress = 0.f;
    pm->state          = UI_PM_TRANSITIONING;

    if (trans == UI_TRANS_NONE || duration_ms == 0) {
        /* Instant switch — no animation needed */
        pm->trans_progress = 1.f;
        pm->state          = UI_PM_IDLE;
        if (outgoing && outgoing->on_disappear) outgoing->on_disappear(outgoing);
        if (incoming && incoming->on_appear)    incoming->on_appear(incoming);
        return;
    }

    /* Stop any in-flight transition */
    if (pm->trans_anim != UI_ANIM_INVALID) {
        ui_anim_stop(pm->trans_anim);
        pm->trans_anim = UI_ANIM_INVALID;
    }

    /* Progress goes 0 → 1 over duration_ms */
    pm->trans_anim = ui_anim_create(&pm->trans_progress, 0.f, 1.f,
                                     duration_ms, 0,
                                     ui_ease_in_out_cubic,
                                     NULL, NULL);
    ui_anim_start(pm->trans_anim);
}

static void _finish_transition(ui_page_manager_t *pm) {
    pm->state = UI_PM_IDLE;
    if (pm->outgoing && pm->outgoing->on_disappear)
        pm->outgoing->on_disappear(pm->outgoing);
    if (pm->incoming && pm->incoming->on_appear)
        pm->incoming->on_appear(pm->incoming);
    pm->trans_anim = UI_ANIM_INVALID;
}

/* ─── Navigation ──────────────────────────────────────────────────────────── */

void ui_page_push(ui_page_manager_t *pm, ui_page_t *page,
                  ui_trans_type_t trans, uint32_t duration_ms) {
    if (pm->state == UI_PM_TRANSITIONING) return;   /* block during animation */

    ui_page_t *outgoing = (pm->stack_top >= 0) ? pm->stack[pm->stack_top] : NULL;

    if (pm->stack_top + 1 >= (int)UI_PAGE_STACK_MAX) return;  /* stack full */
    pm->stack_top++;
    pm->stack[pm->stack_top] = page;

    _begin_transition(pm, outgoing, page, trans, duration_ms, true);
}

void ui_page_pop(ui_page_manager_t *pm,
                 ui_trans_type_t trans, uint32_t duration_ms) {
    if (pm->state == UI_PM_TRANSITIONING) return;
    if (pm->stack_top <= 0) return;   /* nothing to pop */

    ui_page_t *outgoing = pm->stack[pm->stack_top];
    pm->stack_top--;
    ui_page_t *incoming = pm->stack[pm->stack_top];

    _begin_transition(pm, outgoing, incoming, trans, duration_ms, false);
    /* Remove the popped page from the stack after transition completes.
       We defer the actual pop until finish so we can still render it.
       Re-increment temporarily so outgoing stays accessible. */
    pm->stack_top++;
}

void ui_page_replace(ui_page_manager_t *pm, ui_page_t *page,
                     ui_trans_type_t trans, uint32_t duration_ms) {
    if (pm->state == UI_PM_TRANSITIONING) return;

    ui_page_t *outgoing = (pm->stack_top >= 0) ? pm->stack[pm->stack_top] : NULL;
    if (pm->stack_top < 0) pm->stack_top = 0;
    pm->stack[pm->stack_top] = page;

    _begin_transition(pm, outgoing, page, trans, duration_ms, true);
}

ui_page_t *ui_page_current(const ui_page_manager_t *pm) {
    if (pm->stack_top < 0) return NULL;
    return pm->stack[pm->stack_top];
}

bool ui_page_is_transitioning(const ui_page_manager_t *pm) {
    return pm->state == UI_PM_TRANSITIONING;
}

/* ─── Render ──────────────────────────────────────────────────────────────── */

static void _render_page(ui_page_t *page, ui_canvas_t *canvas) {
    if (!page || !page->root) return;
    ui_canvas_clear(canvas, UI_GRAY_BLACK);
    ui_widget_render(page->root, canvas, 0, 0);
}

void ui_page_manager_render(ui_page_manager_t *pm) {
    if (pm->state == UI_PM_IDLE) {
        /* Normal render: just draw current page */
        ui_page_t *cur = ui_page_current(pm);
        _render_page(cur, pm->main_canvas);
        return;
    }

    /* Transition: check if animation is done */
    bool anim_done = (pm->trans_anim == UI_ANIM_INVALID ||
                      !ui_anim_is_active(pm->trans_anim));

    if (anim_done && pm->trans_progress >= 1.f) {
        /* Snap to final state: only incoming page visible */
        _render_page(pm->incoming, pm->main_canvas);

        /* Now truly finish the pop (remove outgoing from stack) */
        if (!pm->is_push && pm->stack_top > 0)
            pm->stack_top--;

        _finish_transition(pm);
        return;
    }

    float t = pm->trans_progress;

    switch (pm->trans_type) {
        case UI_TRANS_FADE: {
            /* Render both pages; blend them */
            _render_page(pm->outgoing, pm->trans_canvas);
            _render_page(pm->incoming, pm->main_canvas);
            /* alpha4: 0=all outgoing, 15=all incoming */
            uint8_t alpha4 = (uint8_t)(t * 15.f + 0.5f);
            ui_canvas_fade(pm->main_canvas,
                           pm->trans_canvas,
                           pm->main_canvas,
                           alpha4);
            break;
        }
        case UI_TRANS_SLIDE_LEFT: {
            /* Push: new page slides in from the right.
               split goes 256 → 0 as t goes 0 → 1.
               Left of split: outgoing page. Right of split: incoming page. */
            int split = (int)((1.f - t) * (float)UI_SCREEN_W) & ~1;
            _render_page(pm->outgoing, pm->trans_canvas);
            _render_page(pm->incoming, pm->main_canvas);
            /* Composite: outgoing occupies [0, split), incoming [split, 255] */
            /* Overwrite main_canvas by sliding outgoing over the left portion */
            for (int row = 0; row < (int)UI_SCREEN_H; row++) {
                uint8_t *dst = pm->main_canvas->buf + (size_t)row * pm->main_canvas->stride;
                const uint8_t *old_r = pm->trans_canvas->buf + (size_t)row * pm->trans_canvas->stride;
                int sb = split >> 1;
                if (sb > 0)
                    memcpy(dst, old_r, (size_t)sb);
                /* right side already has incoming page, no action needed */
            }
            break;
        }
        case UI_TRANS_SLIDE_RIGHT: {
            /* Pop: old page exits to the right, new page reveals from left.
               split goes 0 → 256 as t goes 0 → 1. */
            int split = (int)(t * (float)UI_SCREEN_W) & ~1;
            _render_page(pm->outgoing, pm->trans_canvas);   /* page being popped */
            _render_page(pm->incoming, pm->main_canvas);    /* page being revealed */
            ui_canvas_slide_right(pm->main_canvas,
                                   pm->trans_canvas,
                                   pm->main_canvas,
                                   split);
            break;
        }
        default: {
            /* Fallback: instant */
            _render_page(pm->incoming, pm->main_canvas);
            break;
        }
    }
}

/* ─── Event dispatch ──────────────────────────────────────────────────────── */

void ui_page_dispatch_event(ui_page_manager_t *pm, const ui_event_t *evt) {
    if (pm->state == UI_PM_TRANSITIONING) return;   /* eat events during transition */
    ui_page_t *cur = ui_page_current(pm);
    if (cur && cur->root)
        ui_widget_dispatch(cur->root, evt);
}
