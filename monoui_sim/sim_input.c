#include "sim_input.h"
#include <SDL2/SDL.h>
#include <string.h>

/* ─── Internal state ──────────────────────────────────────────────────────── */

static sim_input_state_t s_state;
static ui_event_t        s_pending_event;

/* ─── Public ──────────────────────────────────────────────────────────────── */

void sim_input_init(void) {
    memset(&s_state, 0, sizeof(s_state));
}

void sim_input_process(const SDL_Event *e) {
    s_state.event_pending = false;

    switch (e->type) {
        /* ── Keyboard ─────────────────────────────────────────────────────── */
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            bool down    = (e->type == SDL_KEYDOWN);
            bool repeat  = (e->key.repeat != 0);

            switch (e->key.keysym.sym) {
                /* Encoder CW: Right arrow or Up arrow */
                case SDLK_RIGHT:
                case SDLK_UP:
                    s_state.key_enc_cw = down;
                    if (down) {                         /* fire on every key-repeat too */
                        s_pending_event.type  = UI_EVT_ENCODER_CW;
                        s_pending_event.value = 1;
                        s_state.event_pending = true;
                    }
                    break;

                /* Encoder CCW: Left arrow or Down arrow */
                case SDLK_LEFT:
                case SDLK_DOWN:
                    s_state.key_enc_ccw = down;
                    if (down) {
                        s_pending_event.type  = UI_EVT_ENCODER_CCW;
                        s_pending_event.value = -1;
                        s_state.event_pending = true;
                    }
                    break;

                /* OK / confirm: Space or Enter */
                case SDLK_SPACE:
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    s_state.key_ok = down;
                    if (!repeat) {
                        s_pending_event.type  = down ? UI_EVT_BTN_PRESS
                                                      : UI_EVT_BTN_RELEASE;
                        s_pending_event.value = 0;
                        s_state.event_pending = true;
                    }
                    break;

                /* Back / cancel: Escape */
                case SDLK_ESCAPE:
                    s_state.key_back = down;
                    if (down && !repeat) {
                        s_pending_event.type  = UI_EVT_BTN_PRESS;
                        s_pending_event.value = 1;          /* btn id 1 = back */
                        s_state.event_pending = true;
                    }
                    if (!down) {
                        s_pending_event.type  = UI_EVT_BTN_RELEASE;
                        s_pending_event.value = 1;
                        s_state.event_pending = true;
                    }
                    break;

                default: break;
            }
            break;
        }

        /* ── Mouse wheel → encoder ────────────────────────────────────────── */
        case SDL_MOUSEWHEEL: {
            int dy = e->wheel.y;
#if SDL_VERSION_ATLEAST(2, 0, 18)
            if (e->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) dy = -dy;
#endif
            if (dy > 0) {
                s_state.key_enc_cw    = true;
                s_pending_event.type  = UI_EVT_ENCODER_CW;
                s_pending_event.value = dy;
                s_state.event_pending = true;
            } else if (dy < 0) {
                s_state.key_enc_ccw   = true;
                s_pending_event.type  = UI_EVT_ENCODER_CCW;
                s_pending_event.value = -dy;
                s_state.event_pending = true;
            }
            break;
        }

        /* Clear wheel "held" state on frame end (wheels don't have key-up) */
        case SDL_USEREVENT:
            s_state.key_enc_cw  = false;
            s_state.key_enc_ccw = false;
            break;

        default: break;
    }
}

bool sim_input_poll(ui_event_t *out) {
    if (!s_state.event_pending) return false;
    *out = s_pending_event;
    s_state.event_pending = false;
    return true;
}

const sim_input_state_t *sim_input_state(void) {
    return &s_state;
}
