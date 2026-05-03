#include "page_home.h"
#include "app_ui.h"

static ui_widget_t   root;
static ui_rect_t     bg;
static ui_label_t    title;
static ui_label_t    subtitle;
static ui_progress_t bar;
static ui_label_t    hint;

ui_page_t page_home;

static void on_appear(ui_page_t *p) {
    (void)p;
    ui_widget_animate(&bar.value,        0.f, 0.75f, 900, ui_ease_out_bounce, NULL, NULL);
    ui_widget_animate(&title.base.alpha, 0.f, 1.f,   350, ui_ease_out_cubic,  NULL, NULL);
}

void page_home_build(void) {
    ui_widget_init(&root, 0, 0, 256, 64, NULL);

    /* 渐变背景 */
    ui_rect_init(&bg, 0, 0, 256, 64, UI_GRAY_BLACK);
    bg.gradient   = true;
    bg.fill_gray2 = UI_GRAY_SHADOW;
    ui_widget_add_child(&root, &bg.base);

    /* 标题 */
    ui_label_init(&title, 6, 5, 200, 10,
                  "MonoUI Demo", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &title.base);

    /* 副标题 */
    ui_label_init(&subtitle, 6, 16, 240, 9,
                  "SSD1322  256x64  4bpp Gray", &g_font,
                  UI_GRAY_MUTED, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &subtitle.base);

    /* 进度条 */
    ui_progress_init(&bar, 6, 30, 204, 10, UI_GRAY_DIM, UI_GRAY_LIGHT);
    bar.corner_r   = 2;
    bar.gradient   = true;
    bar.fill_gray2 = UI_GRAY_WHITE;
    ui_widget_add_child(&root, &bar.base);

    /* 提示 */
    ui_label_init(&hint, 6, 50, 244, 9,
                  "-> Next   Space: replay", &g_font,
                  UI_GRAY_DIM, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &hint.base);

    ui_page_init(&page_home, &root, on_appear, NULL);
}