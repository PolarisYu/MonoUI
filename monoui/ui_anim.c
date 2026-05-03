#include "ui_anim.h"
#include <math.h>
#include <string.h>

/* ─── Static pool ─────────────────────────────────────────────────────────── */

typedef struct {
    float        *target;
    float         from, to;
    float         cur_from;    /* supports yoyo direction reversal */
    float         cur_to;
    uint32_t      duration_ms;
    uint32_t      delay_ms;
    uint32_t      elapsed_ms;
    ui_ease_fn_t  ease;
    ui_anim_cb_t  on_complete;
    void         *ctx;
    bool          active;
    bool          yoyo;
    bool          loop;
    bool          in_delay;
} anim_slot_t;

static anim_slot_t s_pool[UI_ANIM_POOL_SIZE];

/* ═══════════════════════════════════════════════════════════════════════════
 * Easing functions
 * Reference: https://easings.net
 * All functions take t ∈ [0, 1] and return value ∈ [0, 1].
 * ═══════════════════════════════════════════════════════════════════════════ */

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

float ui_ease_linear(float t) { return t; }

float ui_ease_in_quad(float t)       { return t * t; }
float ui_ease_out_quad(float t)      { return t * (2.f - t); }
float ui_ease_in_out_quad(float t)   { return t < 0.5f ? 2*t*t : -1+(4-2*t)*t; }

float ui_ease_in_cubic(float t)      { return t * t * t; }
float ui_ease_out_cubic(float t)     { float s = t-1; return s*s*s + 1; }
float ui_ease_in_out_cubic(float t)  {
    return t < 0.5f ? 4*t*t*t : (t-1)*(2*t-2)*(2*t-2)+1;
}

float ui_ease_in_quart(float t)      { return t*t*t*t; }
float ui_ease_out_quart(float t)     { float s=t-1; return 1-s*s*s*s; }
float ui_ease_in_out_quart(float t)  {
    return t < 0.5f ? 8*t*t*t*t : 1-8*(t-1)*(t-1)*(t-1)*(t-1);
}

float ui_ease_out_elastic(float t) {
    if (t <= 0.f) return 0.f;
    if (t >= 1.f) return 1.f;
    const float c4 = (2.f * (float)M_PI) / 3.f;
    return powf(2.f, -10.f*t) * sinf((t*10.f - 0.75f) * c4) + 1.f;
}

float ui_ease_in_back(float t) {
    const float c1 = 1.70158f, c3 = c1 + 1.f;
    return c3*t*t*t - c1*t*t;
}
float ui_ease_out_back(float t) {
    const float c1 = 1.70158f, c3 = c1 + 1.f;
    float s = t - 1.f;
    return 1.f + c3*s*s*s + c1*s*s;
}
float ui_ease_in_out_back(float t) {
    const float c1 = 1.70158f, c2 = c1 * 1.525f;
    return t < 0.5f
        ? (powf(2.f*t, 2.f) * ((c2+1.f)*2.f*t - c2)) / 2.f
        : (powf(2.f*t-2.f, 2.f) * ((c2+1.f)*(t*2.f-2.f) + c2) + 2.f) / 2.f;
}

static float _bounce_out(float t) {
    const float n1 = 7.5625f, d1 = 2.75f;
    if (t < 1.f/d1)       return n1*t*t;
    else if (t < 2.f/d1)  { t -= 1.5f/d1;  return n1*t*t + 0.75f; }
    else if (t < 2.5f/d1) { t -= 2.25f/d1; return n1*t*t + 0.9375f; }
    else                   { t -= 2.625f/d1;return n1*t*t + 0.984375f; }
}
float ui_ease_out_bounce(float t) { return _bounce_out(t); }
float ui_ease_in_bounce (float t) { return 1.f - _bounce_out(1.f - t); }

/* ═══════════════════════════════════════════════════════════════════════════
 * Pool management
 * ═══════════════════════════════════════════════════════════════════════════ */

ui_anim_id_t ui_anim_create(float        *target,
                              float         from,
                              float         to,
                              uint32_t      duration_ms,
                              uint32_t      delay_ms,
                              ui_ease_fn_t  ease,
                              ui_anim_cb_t  on_complete,
                              void         *ctx) {
    for (int i = 0; i < (int)UI_ANIM_POOL_SIZE; i++) {
        if (!s_pool[i].active) {
            anim_slot_t *s = &s_pool[i];
            s->target      = target;
            s->from        = from;
            s->to          = to;
            s->cur_from    = from;
            s->cur_to      = to;
            s->duration_ms = duration_ms > 0 ? duration_ms : 1;
            s->delay_ms    = delay_ms;
            s->elapsed_ms  = 0;
            s->ease        = ease ? ease : ui_ease_linear;
            s->on_complete = on_complete;
            s->ctx         = ctx;
            s->active      = false;   /* start() must be called separately */
            s->yoyo        = false;
            s->loop        = false;
            s->in_delay    = (delay_ms > 0);
            return (ui_anim_id_t)i;
        }
    }
    return UI_ANIM_INVALID;
}

void ui_anim_start(ui_anim_id_t id) {
    if (id < 0 || id >= (int)UI_ANIM_POOL_SIZE) return;
    anim_slot_t *s = &s_pool[id];
    s->elapsed_ms = 0;
    s->cur_from   = s->from;
    s->cur_to     = s->to;
    s->in_delay   = (s->delay_ms > 0);
    s->active     = true;
    if (s->target) *s->target = s->from;
}

void ui_anim_stop(ui_anim_id_t id) {
    if (id < 0 || id >= (int)UI_ANIM_POOL_SIZE) return;
    s_pool[id].active = false;
}

void ui_anim_set_yoyo(ui_anim_id_t id, bool yoyo) {
    if (id < 0 || id >= (int)UI_ANIM_POOL_SIZE) return;
    s_pool[id].yoyo = yoyo;
}

void ui_anim_set_loop(ui_anim_id_t id, bool loop) {
    if (id < 0 || id >= (int)UI_ANIM_POOL_SIZE) return;
    s_pool[id].loop = loop;
}

bool ui_anim_is_active(ui_anim_id_t id) {
    if (id < 0 || id >= (int)UI_ANIM_POOL_SIZE) return false;
    return s_pool[id].active;
}

void ui_anim_reset_all(void) {
    memset(s_pool, 0, sizeof(s_pool));
}

/* ─── Main tick ───────────────────────────────────────────────────────────── */

void ui_anim_tick(uint32_t delta_ms) {
    for (int i = 0; i < (int)UI_ANIM_POOL_SIZE; i++) {
        anim_slot_t *s = &s_pool[i];
        if (!s->active) continue;

        s->elapsed_ms += delta_ms;

        /* Delay phase */
        if (s->in_delay) {
            if (s->elapsed_ms < s->delay_ms) continue;
            s->elapsed_ms -= s->delay_ms;
            s->in_delay = false;
        }

        /* Clamp time to [0, duration] */
        uint32_t t_ms = s->elapsed_ms;
        if (t_ms > s->duration_ms) t_ms = s->duration_ms;

        float t  = (float)t_ms / (float)s->duration_ms;
        float et = s->ease(t);
        float val = s->cur_from + et * (s->cur_to - s->cur_from);

        if (s->target) *s->target = val;

        /* Check completion */
        if (s->elapsed_ms >= s->duration_ms) {
            if (s->target) *s->target = s->cur_to;  /* snap to end value */

            if (s->yoyo) {
                /* Reverse direction */
                float tmp   = s->cur_from;
                s->cur_from = s->cur_to;
                s->cur_to   = tmp;
                s->elapsed_ms = 0;
                s->in_delay = false;
                /* Note: yoyo loops indefinitely unless loop=false was set before.
                   If you want a one-shot yoyo, check elapsed_ms across two cycles
                   or use a counter in on_complete. */
            } else if (s->loop) {
                s->cur_from   = s->from;
                s->cur_to     = s->to;
                s->elapsed_ms = 0;
                s->in_delay   = (s->delay_ms > 0);
            } else {
                s->active = false;
                if (s->on_complete) s->on_complete(s->ctx);
            }
        }
    }
}
