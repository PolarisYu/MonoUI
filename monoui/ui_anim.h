#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "ui_conf.h"

/* ═══════════════════════════════════════════════════════════════════════════
 * Animation Engine
 *
 * Drives non-linear interpolation of float-typed widget properties.
 * Uses a static pool of UI_ANIM_POOL_SIZE slots — no heap allocation.
 *
 * Usage:
 *   ui_anim_id_t id = ui_anim_create(&widget->alpha, 0.f, 1.f, 300, 0,
 *                                     ui_ease_out_cubic, NULL, NULL);
 *   ui_anim_start(id);
 *   // In main loop: ui_anim_tick(delta_ms);
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef float (*ui_ease_fn_t)(float t);  /* t ∈ [0, 1] → [0, 1] */
typedef void  (*ui_anim_cb_t)(void *ctx);

typedef int ui_anim_id_t;
#define UI_ANIM_INVALID  (-1)

/* ─── Easing functions ────────────────────────────────────────────────────── */

float ui_ease_linear       (float t);
float ui_ease_in_quad      (float t);
float ui_ease_out_quad     (float t);
float ui_ease_in_out_quad  (float t);
float ui_ease_in_cubic     (float t);
float ui_ease_out_cubic    (float t);
float ui_ease_in_out_cubic (float t);
float ui_ease_in_quart     (float t);
float ui_ease_out_quart    (float t);
float ui_ease_in_out_quart (float t);
float ui_ease_out_elastic  (float t);
float ui_ease_in_back      (float t);
float ui_ease_out_back     (float t);
float ui_ease_in_out_back  (float t);
float ui_ease_out_bounce   (float t);
float ui_ease_in_bounce    (float t);

/* ─── Tween API ───────────────────────────────────────────────────────────── */

/*  Allocate a tween from the pool.
 *  target:       pointer to the float to animate (e.g. &w->alpha, &w->offset_x)
 *  from/to:      value range
 *  duration_ms:  total playback time
 *  delay_ms:     start delay before animation begins
 *  ease:         easing function (NULL → linear)
 *  on_complete:  callback fired once when the tween finishes (NULL OK)
 *  ctx:          user data for callback
 *
 *  Returns UI_ANIM_INVALID if the pool is exhausted.                           */
ui_anim_id_t ui_anim_create(float        *target,
                              float         from,
                              float         to,
                              uint32_t      duration_ms,
                              uint32_t      delay_ms,
                              ui_ease_fn_t  ease,
                              ui_anim_cb_t  on_complete,
                              void         *ctx);

/*  Start / restart the tween (resets elapsed to zero).                         */
void ui_anim_start   (ui_anim_id_t id);

/*  Immediately stop the tween and free the pool slot.                          */
void ui_anim_stop    (ui_anim_id_t id);

/*  When yoyo=true the tween reverses from→to→from repeatedly.                 */
void ui_anim_set_yoyo(ui_anim_id_t id, bool yoyo);

/*  When loop=true the tween restarts from the beginning after each cycle.      */
void ui_anim_set_loop(ui_anim_id_t id, bool loop);

/*  Returns true while the tween is running (including delay phase).            */
bool ui_anim_is_active(ui_anim_id_t id);

/* ─── Engine tick ─────────────────────────────────────────────────────────── */

/*  Advance all active tweens. Call once per frame with the elapsed ms.         */
void ui_anim_tick(uint32_t delta_ms);

/*  Reset the entire pool (stop all animations).                                */
void ui_anim_reset_all(void);
