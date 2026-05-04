#include "ui_action.h"
#include <stddef.h>

/* ── Router state ──────────────────────────────────────────────────────────── */
static ui_keymap_t    s_keymap = NULL;
static ui_action_cb_t s_cb     = NULL;
static void          *s_ctx    = NULL;

void ui_router_init(void) {
    s_keymap = NULL;
    s_cb     = NULL;
    s_ctx    = NULL;
}

void ui_router_set_keymap(ui_keymap_t keymap, ui_action_cb_t cb, void *ctx) {
    if (keymap) s_keymap = keymap;
    s_cb  = cb;
    s_ctx = ctx;
}

ui_action_t ui_router_translate(const ui_event_t *evt) {
    if (!s_keymap || !evt) return UI_ACTION_NONE;
    for (const ui_keymap_entry_t *e = s_keymap; e->evt != UI_EVT_NONE; e++) {
        if (e->evt == evt->type &&
            (e->value == UI_ANY_VALUE || e->value == evt->value)) {
            return e->action;
        }
    }
    return UI_ACTION_NONE;
}

ui_action_t ui_router_dispatch(const ui_event_t *evt) {
    ui_action_t action = ui_router_translate(evt);
    if (action != UI_ACTION_NONE && s_cb) {
        s_cb(action, evt, s_ctx);
    }
    return action;
}

/* ── Built-in keymap tables ─────────────────────────────────────────────── */

const ui_keymap_entry_t ui_keymap_menu[] = {
    { UI_EVT_DPAD_UP,        UI_ANY_VALUE, UI_ACTION_NAV_UP      },
    { UI_EVT_DPAD_DOWN,      UI_ANY_VALUE, UI_ACTION_NAV_DOWN    },
    { UI_EVT_ENCODER_CW,     UI_ANY_VALUE, UI_ACTION_SCROLL_DOWN },
    { UI_EVT_ENCODER_CCW,    UI_ANY_VALUE, UI_ACTION_SCROLL_UP   },
    { UI_EVT_DPAD_CENTER,    UI_ANY_VALUE, UI_ACTION_CONFIRM     },
    { UI_EVT_DPAD_LEFT,      UI_ANY_VALUE, UI_ACTION_CANCEL      },
    UI_KEYMAP_END
};

const ui_keymap_entry_t ui_keymap_param[] = {
    { UI_EVT_ENCODER_CW,     UI_ANY_VALUE, UI_ACTION_SCROLL_UP   },
    { UI_EVT_ENCODER_CCW,    UI_ANY_VALUE, UI_ACTION_SCROLL_DOWN },
    { UI_EVT_DPAD_UP,        UI_ANY_VALUE, UI_ACTION_SCROLL_UP   },
    { UI_EVT_DPAD_DOWN,      UI_ANY_VALUE, UI_ACTION_SCROLL_DOWN },
    { UI_EVT_DPAD_CENTER,    UI_ANY_VALUE, UI_ACTION_CONFIRM     },
    { UI_EVT_DPAD_LEFT,      UI_ANY_VALUE, UI_ACTION_CANCEL      },
    { UI_EVT_BTN_LONG_PRESS, UI_ANY_VALUE, UI_ACTION_MENU        },
    UI_KEYMAP_END
};

const ui_keymap_entry_t ui_keymap_encoder_only[] = {
    { UI_EVT_ENCODER_CW,  UI_ANY_VALUE, UI_ACTION_SCROLL_UP   },
    { UI_EVT_ENCODER_CCW, UI_ANY_VALUE, UI_ACTION_SCROLL_DOWN },
    { UI_EVT_DPAD_CENTER, UI_ANY_VALUE, UI_ACTION_CONFIRM     },
    UI_KEYMAP_END
};
