#include "page_settings.h"

static ui_widget_t root;
static ui_label_t  title;

ui_page_t page_settings;

extern ui_page_manager_t g_pm;
extern const ui_font_t   g_font;

void page_settings_build(void) {
    ui_widget_init(&root, 0, 0, 256, 64, NULL);

    ui_label_init(&title, 8, 8, 200, 12,
                  "Settings", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &title.base);

    ui_page_init(&page_settings, &root, NULL, NULL);
}