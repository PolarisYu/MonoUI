#include "page_settings.h"
#include "app_ui.h"

static ui_widget_t root;
static ui_label_t  title;

ui_page_t page_settings;
static app_page_binding_t s_page_binding;

static const ui_keymap_entry_t s_keymap[] = {
    { UI_EVT_DPAD_LEFT,   UI_ANY_VALUE, UI_ACTION_CANCEL },
    { UI_EVT_DPAD_CENTER, UI_ANY_VALUE, UI_ACTION_CANCEL },
    UI_KEYMAP_END
};

static void on_action(ui_action_t action, const ui_event_t *raw, void *ctx) {
    (void)raw; (void)ctx;
    if (action == UI_ACTION_CANCEL)
        app_ui_pop(UI_TRANS_SLIDE_RIGHT, UI_TRANS_DURATION_MS);
}

void page_settings_build(void) {
    ui_widget_init(&root, 0, 0, 256, 64, NULL);

    ui_label_init(&title, 8, 8, 200, 12,
                  "Settings", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &title.base);

    app_page_init(&page_settings, &s_page_binding, &root,
                  s_keymap, on_action, NULL, NULL, NULL);
}
