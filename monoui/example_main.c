/*  example_main.c
 *
 *  Demonstrates how to wire monoui to the SSD1322 driver:
 *
 *   - Two pages: "Home" and "Settings"
 *   - Home page: animated progress bar + label that fades in on appear
 *   - Settings page: static labels
 *   - Encoder CW → push Settings with SLIDE_LEFT
 *   - Encoder CCW (or btn) → pop Settings with SLIDE_RIGHT
 *
 *  Intended to run on STM32G4 with FreeRTOS or a bare-metal super-loop.
 */

#include "monoui.h"
#include "ssd1322.h"

/* ─── Display driver handle (defined/configured in ssd1322_bsp.c) ─────────── */
extern SSD1322_HandleTypeDef holed;

/* ─── HAL flush callback ──────────────────────────────────────────────────── */

static void oled_flush(const uint8_t *buf,
                        uint16_t x1, uint16_t y1,
                        uint16_t x2, uint16_t y2,
                        void *ctx) {
    SSD1322_HandleTypeDef *dev = (SSD1322_HandleTypeDef *)ctx;
    /* SSD1322_FlushArea expects packed 4bpp, which is exactly what monoui provides.
       It calls WaitForDMA internally, so this call is safe to make every frame. */
    SSD1322_FlushArea(dev, x1, y1, x2, y2, buf);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Home page
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Font data must be provided by the application — see your font generator.
   A placeholder is declared here to satisfy compilation.                       */
extern const ui_font_t font_8x12;

static ui_widget_t   home_root;
static ui_rect_t     home_bg;
static ui_label_t    home_title;
static ui_progress_t home_bar;
static ui_label_t    home_hint;
static ui_page_t     page_home;

static void home_on_appear(ui_page_t *page) {
    (void)page;
    /* Animate the progress bar value 0 → 0.75 with bounce on appear */
    ui_widget_animate(&home_bar.value, 0.f, 0.75f, 800, ui_ease_out_bounce, NULL, NULL);
    /* Fade the title in */
    ui_widget_animate(&home_title.base.alpha, 0.f, 1.f, 400, ui_ease_out_cubic, NULL, NULL);
}

static void home_build(void) {
    /* Root (invisible container covering full screen) */
    ui_widget_init(&home_root, 0, 0, UI_SCREEN_W, UI_SCREEN_H, NULL);

    /* Background: dark gradient */
    ui_rect_init(&home_bg, 0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_GRAY_BLACK);
    home_bg.gradient   = true;
    home_bg.fill_gray2 = UI_GRAY_SHADOW;
    ui_widget_add_child(&home_root, &home_bg.base);

    /* Title label */
    ui_label_init(&home_title, 8, 4, 200, 16,
                  "MonoUI Demo", &font_8x12,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    home_title.align = UI_ALIGN_LEFT;
    ui_widget_add_child(&home_root, &home_title.base);

    /* Progress bar with gradient fill */
    ui_progress_init(&home_bar, 8, 28, 200, 10, UI_GRAY_DIM, UI_GRAY_LIGHT);
    home_bar.corner_r   = 2;
    home_bar.gradient   = true;
    home_bar.fill_gray2 = UI_GRAY_WHITE;
    ui_widget_add_child(&home_root, &home_bar.base);

    /* Hint text */
    ui_label_init(&home_hint, 8, 48, 230, 12,
                  "CW: Settings", &font_8x12,
                  UI_GRAY_MUTED, UI_GRAY_BLACK, true);
    ui_widget_add_child(&home_root, &home_hint.base);

    ui_page_init(&page_home, &home_root, home_on_appear, NULL);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Settings page
 * ═══════════════════════════════════════════════════════════════════════════ */

static ui_widget_t settings_root;
static ui_rect_t   settings_bg;
static ui_label_t  settings_title;
static ui_label_t  settings_back_hint;
static ui_page_t   page_settings;

static void settings_build(void) {
    ui_widget_init(&settings_root, 0, 0, UI_SCREEN_W, UI_SCREEN_H, NULL);

    ui_rect_init(&settings_bg, 0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_GRAY_SHADOW);
    ui_widget_add_child(&settings_root, &settings_bg.base);

    ui_label_init(&settings_title, 8, 8, 200, 16,
                  "Settings", &font_8x12,
                  UI_GRAY_WHITE, UI_GRAY_SHADOW, true);
    settings_title.align = UI_ALIGN_LEFT;
    ui_widget_add_child(&settings_root, &settings_title.base);

    ui_label_init(&settings_back_hint, 8, 48, 230, 12,
                  "CCW: Back", &font_8x12,
                  UI_GRAY_MUTED, UI_GRAY_SHADOW, true);
    ui_widget_add_child(&settings_root, &settings_back_hint.base);

    ui_page_init(&page_settings, &settings_root, NULL, NULL);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Application entry
 * ═══════════════════════════════════════════════════════════════════════════ */

static ui_page_manager_t pm;

void app_ui_init(void) {
    /* 1. Initialise monoui core, register display flush */
    ui_core_init(oled_flush, &holed);

    /* 2. Initialise page manager with core-owned canvases */
    ui_page_manager_init(&pm,
                          ui_core_get_main_canvas(),
                          ui_core_get_trans_canvas());
    ui_core_set_page_manager(&pm);

    /* 3. Build page widget trees */
    home_build();
    settings_build();

    /* 4. Push the initial page (no transition) */
    ui_page_push(&pm, &page_home, UI_TRANS_NONE, 0);
}

/*  Call from your FreeRTOS task or SysTick handler at ~16 ms intervals.        */
void app_ui_tick(uint32_t delta_ms) {
    ui_core_tick(delta_ms);
}

/*  Call from your encoder / button ISR or debounce task.                       */
void app_ui_on_encoder_cw(void) {
    /* Navigate to Settings if not already there */
    if (!ui_page_is_transitioning(&pm) && ui_page_current(&pm) == &page_home) {
        ui_page_push(&pm, &page_settings, UI_TRANS_SLIDE_LEFT, UI_TRANS_DURATION_MS);
    }
}

void app_ui_on_encoder_ccw(void) {
    /* Go back */
    if (!ui_page_is_transitioning(&pm) && ui_page_current(&pm) == &page_settings) {
        ui_page_pop(&pm, UI_TRANS_SLIDE_RIGHT, UI_TRANS_DURATION_MS);
    }
}

/* ─── Alternative: event-driven input ────────────────────────────────────── */

void app_ui_on_button(uint8_t btn_id, bool pressed) {
    ui_event_t evt = {
        .type  = pressed ? UI_EVT_BTN_PRESS : UI_EVT_BTN_RELEASE,
        .value = btn_id,
    };
    ui_core_push_event(&evt);
}
