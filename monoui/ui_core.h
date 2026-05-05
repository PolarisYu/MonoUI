#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "ui_canvas.h"
#include "ui_page.h"

/* ═══════════════════════════════════════════════════════════════════════════
 * UI Core
 *
 * Top-level subsystem.  Owns the two framebuffers, ties the animation engine,
 * page manager, and display HAL together.  The application only needs:
 *
 *   1. Implement the HAL flush callback to push 4bpp data to the display.
 *   2. Call ui_core_init() once.
 *   3. Call ui_core_tick(delta_ms) at a regular interval (e.g. every 16 ms).
 *   4. Call ui_core_push_event() for encoder / button input.
 *
 * Example wiring to SSD1322 driver:
 *
 *   static void my_flush(const uint8_t *buf, uint16_t x1, uint16_t y1,
 *                         uint16_t x2, uint16_t y2, void *ctx) {
 *       SSD1322_FlushArea((SSD1322_HandleTypeDef *)ctx, x1, y1, x2, y2, buf);
 *   }
 *
 *   SSD1322_HandleTypeDef holed;
 *   // ... init holed ...
 *   ui_core_init(my_flush, &holed);
 *   ui_core_set_page_manager(&pm);
 *
 * ═══════════════════════════════════════════════════════════════════════════ */

/* ─── HAL flush callback ──────────────────────────────────────────────────── */

/*  The framework calls this when it wants to push a rendered frame.
 *  buf:   pointer to the 4bpp packed framebuffer (UI_FB_SIZE bytes).
 *  x1,y1: top-left  (currently always 0,0 for full-screen flush)
 *  x2,y2: bottom-right (currently always 255, 63)
 *  ctx:   the user_ctx passed to ui_core_init()                               */
typedef void (*ui_hal_flush_fn)(const uint8_t *buf,
                                 uint16_t x1, uint16_t y1,
                                 uint16_t x2, uint16_t y2,
                                 void *ctx);

typedef void (*ui_overlay_render_fn)(ui_canvas_t *canvas, void *ctx);

/* ─── Core API ────────────────────────────────────────────────────────────── */

/*  Initialise the core, register the HAL flush callback.
 *  After this, canvases are ready but no pages are loaded.                     */
void ui_core_init(ui_hal_flush_fn flush_fn, void *user_ctx);

/*  Register the page manager to use for rendering.
 *  The page manager must have been initialised with ui_page_manager_init()
 *  pointing at ui_core_get_main_canvas() and ui_core_get_trans_canvas().      */
void ui_core_set_page_manager(ui_page_manager_t *pm);
void ui_core_set_overlay_renderer(ui_overlay_render_fn render_fn, void *ctx);

/*  Access the core-owned canvases (use these when calling ui_page_manager_init). */
ui_canvas_t *ui_core_get_main_canvas (void);
ui_canvas_t *ui_core_get_trans_canvas(void);

/*  Main tick — call every frame (e.g. from a FreeRTOS task or timer callback).
 *  delta_ms: milliseconds elapsed since the last call.                         */
void ui_core_tick(uint32_t delta_ms);

/*  Queue an input event (from ISR or task). Events are delivered in FIFO order. */
void ui_core_push_event(const ui_event_t *evt);

/*  Force a full re-render even if nothing is dirty.                            */
void ui_core_invalidate(void);
