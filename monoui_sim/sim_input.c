#include "sim_input.h"
#include <SDL2/SDL.h>
#include <string.h>

/*
 * 键盘映射（与实际硬件对应）：
 *
 *   实际硬件                 键盘按键            鼠标
 *   ─────────────────────   ─────────────────   ──────────────
 *   编码器 顺时针 (ENC_R)    E  /  =  / 滚轮↑   滚轮向上
 *   编码器 逆时针 (ENC_L)    Q  /  -  / 滚轮↓   滚轮向下
 *   摇杆 上拨  (SW_A)        ↑  /  W             —
 *   摇杆 下拨  (SW_C)        ↓  /  S             —
 *   摇杆 左拨  (SW_B)        ←  /  A             —
 *   摇杆 右拨  (SW_D)        →  /  D             —
 *   摇杆 下压  (SW_CENTER)   Space / Enter        —
 *   （保留：长按模拟）        长按 Space ≥500ms   —
 */

#define LONG_PRESS_MS  500u

static sim_input_state_t s_state;
static ui_event_t        s_pending;

/* 长按计时 */
static bool     s_center_held    = false;
static uint32_t s_center_down_ms = 0;
static bool     s_long_fired     = false;

/* ── 内部：推送事件 ─────────────────────────────────────────────────────── */
static void _push(ui_evt_type_t type, int32_t value) {
    s_pending.type    = type;
    s_pending.value   = value;
    s_state.event_pending = true;
}

/* ── 公开 API ────────────────────────────────────────────────────────────── */
void sim_input_init(void) {
    memset(&s_state, 0, sizeof(s_state));
    s_center_held  = false;
    s_long_fired   = false;
}

void sim_input_process(const SDL_Event *e) {
    s_state.event_pending = false;
    s_state.enc_cw  = false;
    s_state.enc_ccw = false;

    switch (e->type) {

    /* ── 键盘按下 ─────────────────────────────────────────────────────────── */
    case SDL_KEYDOWN: {
        if (e->key.repeat) break;   /* 忽略系统自动重复，摇杆不需要连发 */

        switch (e->key.keysym.sym) {
            /* 编码器 顺时针 */
            case SDLK_e: case SDLK_EQUALS: case SDLK_KP_PLUS:
                s_state.enc_cw = true;
                _push(UI_EVT_ENCODER_CW, 1);
                break;

            /* 编码器 逆时针 */
            case SDLK_q: case SDLK_MINUS: case SDLK_KP_MINUS:
                s_state.enc_ccw = true;
                _push(UI_EVT_ENCODER_CCW, 1);
                break;

            /* 摇杆 上 */
            case SDLK_UP: case SDLK_w:
                s_state.dpad_up = true;
                _push(UI_EVT_DPAD_UP, 0);
                break;

            /* 摇杆 下 */
            case SDLK_DOWN: case SDLK_s:
                s_state.dpad_down = true;
                _push(UI_EVT_DPAD_DOWN, 0);
                break;

            /* 摇杆 左 */
            case SDLK_LEFT: case SDLK_a:
                s_state.dpad_left = true;
                _push(UI_EVT_DPAD_LEFT, 0);
                break;

            /* 摇杆 右 */
            case SDLK_RIGHT: case SDLK_d:
                s_state.dpad_right = true;
                _push(UI_EVT_DPAD_RIGHT, 0);
                break;

            /* 摇杆 下压（中键）—— 开始计时，松开时才触发或判断长按 */
            case SDLK_SPACE: case SDLK_RETURN: case SDLK_KP_ENTER:
                s_state.dpad_center = true;
                s_center_held       = true;
                s_center_down_ms    = SDL_GetTicks();
                s_long_fired        = false;
                break;

            default: break;
        }
        break;
    }

    /* ── 键盘松开 ─────────────────────────────────────────────────────────── */
    case SDL_KEYUP: {
        switch (e->key.keysym.sym) {
            case SDLK_UP:    case SDLK_w: s_state.dpad_up    = false; break;
            case SDLK_DOWN:  case SDLK_s: s_state.dpad_down  = false; break;
            case SDLK_LEFT:  case SDLK_a: s_state.dpad_left  = false; break;
            case SDLK_RIGHT: case SDLK_d: s_state.dpad_right = false; break;

            case SDLK_SPACE: case SDLK_RETURN: case SDLK_KP_ENTER:
                s_state.dpad_center = false;
                if (s_center_held && !s_long_fired) {
                    /* 短按：松开时触发 CENTER */
                    _push(UI_EVT_DPAD_CENTER, 0);
                }
                s_center_held = false;
                break;

            default: break;
        }
        break;
    }

    /* ── 鼠标滚轮 → 编码器 ───────────────────────────────────────────────── */
    case SDL_MOUSEWHEEL: {
        int dy = e->wheel.y;
#if SDL_VERSION_ATLEAST(2, 0, 18)
        if (e->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) dy = -dy;
#endif
        if (dy > 0) {
            s_state.enc_cw = true;
            _push(UI_EVT_ENCODER_CW, dy);
        } else if (dy < 0) {
            s_state.enc_ccw = true;
            _push(UI_EVT_ENCODER_CCW, -dy);
        }
        break;
    }

    default: break;
    }

    /* ── 长按检测（每次 process 都检查，不依赖独立定时器）────────────────── */
    if (s_center_held && !s_long_fired) {
        if (SDL_GetTicks() - s_center_down_ms >= LONG_PRESS_MS) {
            s_long_fired = true;
            _push(UI_EVT_BTN_LONG_PRESS, 0);
        }
    }
}

bool sim_input_poll(ui_event_t *out) {
    if (!s_state.event_pending) return false;
    *out = s_pending;
    s_state.event_pending = false;
    return true;
}

const sim_input_state_t *sim_input_state(void) {
    return &s_state;
}