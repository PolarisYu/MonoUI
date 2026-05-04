#include "page_home.h"
#include "page_settings.h"
#include "app_ui.h"

/* ── 控件 ─────────────────────────────────────────────────────────────────── */
static ui_widget_t   root;
static ui_rect_t     bg;
static ui_label_t    title;
static ui_label_t    subtitle;
static ui_progress_t bar;
static ui_label_t    hint;

ui_page_t page_home;
static app_page_binding_t s_page_binding;

/* ── 本页面的按键映射：编码器调进度条，中键复位，右键进入设置 ─────────────── */
static const ui_keymap_entry_t s_keymap[] = {
    { UI_EVT_ENCODER_CW,     UI_ANY_VALUE, UI_ACTION_SCROLL_UP   },
    { UI_EVT_ENCODER_CCW,    UI_ANY_VALUE, UI_ACTION_SCROLL_DOWN },
    { UI_EVT_DPAD_UP,        UI_ANY_VALUE, UI_ACTION_SCROLL_UP   },
    { UI_EVT_DPAD_DOWN,      UI_ANY_VALUE, UI_ACTION_SCROLL_DOWN },
    { UI_EVT_DPAD_RIGHT,     UI_ANY_VALUE, UI_ACTION_NAV_RIGHT   },  /* → 进入设置 */
    { UI_EVT_DPAD_CENTER,    UI_ANY_VALUE, UI_ACTION_CONFIRM     },  /* 复位动画   */
    { UI_EVT_BTN_LONG_PRESS, UI_ANY_VALUE, UI_ACTION_MENU        },
    UI_KEYMAP_END
};

/* ── 动作处理 ─────────────────────────────────────────────────────────────── */
static void on_action(ui_action_t action, const ui_event_t *raw, void *ctx) {
    (void)raw; (void)ctx;
    switch (action) {
        case UI_ACTION_SCROLL_UP:
            bar.value += 0.05f;
            if (bar.value > 1.0f) bar.value = 1.0f;
            break;
        case UI_ACTION_SCROLL_DOWN:
            bar.value -= 0.05f;
            if (bar.value < 0.0f) bar.value = 0.0f;
            break;
        case UI_ACTION_CONFIRM:
            /* 复位：重新播放进入动画 */
            ui_widget_animate(&bar.value, 0.f, 0.75f, 900,
                              ui_ease_out_bounce, NULL, NULL);
            break;
        case UI_ACTION_NAV_RIGHT:
            app_ui_push_id(APP_PAGE_SETTINGS, UI_TRANS_SLIDE_LEFT, UI_TRANS_DURATION_MS);
            break;
        default: break;
    }
}

/* ── 生命周期 ─────────────────────────────────────────────────────────────── */
static void on_appear(ui_page_t *p) {
    (void)p;
    /* 进入动画 */
    ui_widget_animate(&bar.value,        0.f, 0.75f, 900, ui_ease_out_bounce, NULL, NULL);
    ui_widget_animate(&title.base.alpha, 0.f, 1.f,   350, ui_ease_out_cubic,  NULL, NULL);
}

void page_home_build(void) {
    ui_widget_init(&root, 0, 0, 256, 64, NULL);

    ui_rect_init(&bg, 0, 0, 256, 64, UI_GRAY_BLACK);
    bg.gradient   = true;
    bg.fill_gray2 = UI_GRAY_SHADOW;
    ui_widget_add_child(&root, &bg.base);

    ui_label_init(&title, 6, 5, 200, 10,
                  "MonoUI Demo", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &title.base);

    ui_label_init(&subtitle, 6, 16, 240, 9,
                  "SSD1322  256x64  4bpp Gray", &g_font,
                  UI_GRAY_MUTED, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &subtitle.base);

    ui_progress_init(&bar, 6, 30, 204, 10, UI_GRAY_DIM, UI_GRAY_LIGHT);
    bar.corner_r   = 2;
    bar.gradient   = true;
    bar.fill_gray2 = UI_GRAY_WHITE;
    ui_widget_add_child(&root, &bar.base);

    ui_label_init(&hint, 6, 50, 244, 9,
                  "Enc/UD:bar  Spc:reset  ->:settings", &g_font,
                  UI_GRAY_DIM, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &hint.base);

    app_page_init(&page_home, &s_page_binding, &root,
                  s_keymap, on_action, on_appear, NULL, NULL);
}
