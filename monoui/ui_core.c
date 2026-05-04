#include "ui_core.h"
#include "ui_anim.h"
#include <string.h>

/* ─── Core-owned framebuffers ─────────────────────────────────────────────── */

/*  Two 8192-byte buffers:
 *    main_buf   → what gets rendered to and flushed to the display each frame
 *    trans_buf  → off-screen target for the outgoing page during transitions  */
static uint8_t s_main_buf [UI_FB_SIZE] __attribute__((aligned(4)));
static uint8_t s_trans_buf[UI_FB_SIZE] __attribute__((aligned(4)));

static ui_canvas_t s_main_canvas;
static ui_canvas_t s_trans_canvas;

/* ─── Core state ──────────────────────────────────────────────────────────── */

static struct {
    ui_hal_flush_fn    flush_fn;
    void              *user_ctx;
    ui_page_manager_t *page_manager;
    bool               force_redraw;

    /* FIFO queue for bursty input (e.g. encoder scroll) */
    ui_event_t  event_queue[UI_EVENT_QUEUE_SIZE];
    uint8_t     event_head;
    uint8_t     event_tail;
    uint8_t     event_count;
} s_core;

/* ─── Init ────────────────────────────────────────────────────────────────── */

void ui_core_init(ui_hal_flush_fn flush_fn, void *user_ctx) {
    memset(&s_core, 0, sizeof(s_core));
    s_core.flush_fn  = flush_fn;
    s_core.user_ctx  = user_ctx;
    s_core.force_redraw = true;

    ui_canvas_init(&s_main_canvas,  s_main_buf,  UI_SCREEN_W, UI_SCREEN_H);
    ui_canvas_init(&s_trans_canvas, s_trans_buf, UI_SCREEN_W, UI_SCREEN_H);

    ui_canvas_clear(&s_main_canvas,  UI_GRAY_BLACK);
    ui_canvas_clear(&s_trans_canvas, UI_GRAY_BLACK);

    ui_anim_reset_all();
}

void ui_core_set_page_manager(ui_page_manager_t *pm) {
    s_core.page_manager = pm;
}

ui_canvas_t *ui_core_get_main_canvas(void)  { return &s_main_canvas;  }
ui_canvas_t *ui_core_get_trans_canvas(void) { return &s_trans_canvas; }

void ui_core_invalidate(void) { s_core.force_redraw = true; }

/* ─── Event queue ─────────────────────────────────────────────────────────── */

void ui_core_push_event(const ui_event_t *evt) {
    if (!evt) return;

    if (s_core.event_count >= UI_EVENT_QUEUE_SIZE) {
        /* Keep the queue live under sustained input by dropping the oldest event. */
        s_core.event_tail = (uint8_t)((s_core.event_tail + 1u) % UI_EVENT_QUEUE_SIZE);
        s_core.event_count--;
    }

    s_core.event_queue[s_core.event_head] = *evt;
    s_core.event_head = (uint8_t)((s_core.event_head + 1u) % UI_EVENT_QUEUE_SIZE);
    s_core.event_count++;
}

/* ─── Main tick ───────────────────────────────────────────────────────────── */

/*
 *  Frame pipeline (called at UI_FRAME_MS interval):
 *
 *  1.  Advance animation engine — updates all float property targets.
 *  2.  Dispatch pending input events to the page manager.
 *  3.  Render: page manager composites its canvases.
 *  4.  Flush: push the main canvas to the display via HAL.
 */
void ui_core_tick(uint32_t delta_ms) {
    /* ── 1. Animation engine ──────────────────────────────────────────────── */
    ui_anim_tick(delta_ms);

    /* ── 2. Events ────────────────────────────────────────────────────────── */
    if (s_core.page_manager) {
        while (s_core.event_count > 0u) {
            const ui_event_t *evt = &s_core.event_queue[s_core.event_tail];
            ui_page_dispatch_event(s_core.page_manager, evt);
            s_core.event_tail = (uint8_t)((s_core.event_tail + 1u) % UI_EVENT_QUEUE_SIZE);
            s_core.event_count--;
        }
    }

    /* ── 3. Render ────────────────────────────────────────────────────────── */
    if (s_core.page_manager) {
        ui_page_manager_render(s_core.page_manager);
    }

    /* ── 4. HAL flush ─────────────────────────────────────────────────────── */
    if (s_core.flush_fn) {
        s_core.flush_fn(s_main_canvas.buf,
                        0, 0,
                        (uint16_t)(UI_SCREEN_W - 1),
                        (uint16_t)(UI_SCREEN_H - 1),
                        s_core.user_ctx);
    }

    s_core.force_redraw = false;
}
