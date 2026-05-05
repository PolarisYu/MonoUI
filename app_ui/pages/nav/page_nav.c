#include "page_nav.h"
#include "app_ui.h"
#include "pages/settings/page_settings.h"

typedef enum {
    NAV_TARGET_HOME = 0,
    NAV_TARGET_SETTINGS,
    NAV_TARGET_INPUT,
    NAV_TARGET_ASRC,
    NAV_TARGET_OUTPUT,
    NAV_TARGET_SYSTEM,
    NAV_TARGET_COUNT
} nav_target_t;

typedef struct {
    const char *title;
    const char *subtitle;
    app_settings_section_t section;
    bool opens_section;
} nav_item_t;

typedef struct {
    ui_rect_t  *tile;
    ui_label_t *title;
    ui_label_t *subtitle;
} nav_tile_view_t;

static const nav_item_t s_nav_items[NAV_TARGET_COUNT] = {
    { "HOME",     "OVERVIEW", APP_SETTINGS_SECTION_INPUT,   false },
    { "SET",      "ALL",      APP_SETTINGS_SECTION_INPUT,   false },
    { "INPUT",    "SOURCE",   APP_SETTINGS_SECTION_INPUT,   true  },
    { "ASRC",     "FORMAT",   APP_SETTINGS_SECTION_ASRC,    true  },
    { "OUTPUT",   "AMP",      APP_SETTINGS_SECTION_OUTPUT,  true  },
    { "SYSTEM",   "POWER",    APP_SETTINGS_SECTION_SYSTEM,  true  },
};

static ui_widget_t root;
static ui_rect_t   bg;
static ui_rect_t   title_bar;
static ui_label_t  title_text;
static ui_label_t  subtitle_text;

static ui_rect_t   tile_home;
static ui_label_t  tile_home_title;
static ui_label_t  tile_home_subtitle;
static ui_rect_t   tile_settings;
static ui_label_t  tile_settings_title;
static ui_label_t  tile_settings_subtitle;
static ui_rect_t   tile_input;
static ui_label_t  tile_input_title;
static ui_label_t  tile_input_subtitle;
static ui_rect_t   tile_asrc;
static ui_label_t  tile_asrc_title;
static ui_label_t  tile_asrc_subtitle;
static ui_rect_t   tile_output;
static ui_label_t  tile_output_title;
static ui_label_t  tile_output_subtitle;
static ui_rect_t   tile_system;
static ui_label_t  tile_system_title;
static ui_label_t  tile_system_subtitle;

ui_page_t page_nav;
static app_page_binding_t s_page_binding;
static int s_selected = 1;

static const ui_keymap_entry_t s_keymap[] = {
    { UI_EVT_ENCODER_CW,     UI_ANY_VALUE, UI_ACTION_NAV_DOWN  },
    { UI_EVT_ENCODER_CCW,    UI_ANY_VALUE, UI_ACTION_NAV_UP    },
    { UI_EVT_DPAD_UP,        UI_ANY_VALUE, UI_ACTION_NAV_UP    },
    { UI_EVT_DPAD_DOWN,      UI_ANY_VALUE, UI_ACTION_NAV_DOWN  },
    { UI_EVT_DPAD_LEFT,      UI_ANY_VALUE, UI_ACTION_NAV_LEFT  },
    { UI_EVT_DPAD_RIGHT,     UI_ANY_VALUE, UI_ACTION_NAV_RIGHT },
    { UI_EVT_DPAD_CENTER,    UI_ANY_VALUE, UI_ACTION_CONFIRM   },
    { UI_EVT_BTN_LONG_PRESS, UI_ANY_VALUE, UI_ACTION_CANCEL    },
    UI_KEYMAP_END
};

static nav_tile_view_t tile_view(int index) {
    switch (index) {
        case NAV_TARGET_HOME:     return (nav_tile_view_t){ &tile_home,     &tile_home_title,     &tile_home_subtitle };
        case NAV_TARGET_SETTINGS: return (nav_tile_view_t){ &tile_settings, &tile_settings_title, &tile_settings_subtitle };
        case NAV_TARGET_INPUT:    return (nav_tile_view_t){ &tile_input,    &tile_input_title,    &tile_input_subtitle };
        case NAV_TARGET_ASRC:     return (nav_tile_view_t){ &tile_asrc,     &tile_asrc_title,     &tile_asrc_subtitle };
        case NAV_TARGET_OUTPUT:   return (nav_tile_view_t){ &tile_output,   &tile_output_title,   &tile_output_subtitle };
        case NAV_TARGET_SYSTEM:   return (nav_tile_view_t){ &tile_system,   &tile_system_title,   &tile_system_subtitle };
        default:                  return (nav_tile_view_t){ NULL, NULL, NULL };
    }
}

static void set_tile_style(nav_tile_view_t *view, bool selected) {
    if (!view->tile || !view->title || !view->subtitle) {
        return;
    }

    view->tile->fill_gray = selected ? UI_GRAY_WHITE : UI_GRAY_SHADOW;
    view->tile->border_w = 0;
    view->title->fg_gray = selected ? UI_GRAY_BLACK : UI_GRAY_WHITE;
    view->subtitle->fg_gray = selected ? UI_GRAY_SHADOW : UI_GRAY_MUTED;
}

static void animate_tile(nav_tile_view_t *view) {
    if (!view->tile) {
        return;
    }

    view->tile->base.offset_y = 2.f;
    ui_widget_animate(&view->tile->base.offset_y, 2.f, 0.f, 120,
                      ui_ease_out_cubic, NULL, NULL);
}

static void sync_view(bool animate) {
    int i;

    for (i = 0; i < NAV_TARGET_COUNT; ++i) {
        nav_tile_view_t view = tile_view(i);
        const nav_item_t *item = &s_nav_items[i];

        ui_label_set_text(view.title, item->title);
        ui_label_set_text(view.subtitle, item->subtitle);
        set_tile_style(&view, i == s_selected);
    }

    if (animate) {
        nav_tile_view_t view = tile_view(s_selected);
        animate_tile(&view);
    }
}

static void move_selection(int dx, int dy) {
    int col = s_selected % 2;
    int row = s_selected / 2;

    col += dx;
    row += dy;

    while (col < 0) { col += 2; }
    while (col >= 2) { col -= 2; }
    while (row < 0) { row += 3; }
    while (row >= 3) { row -= 3; }

    s_selected = row * 2 + col;
}

static void open_selected(void) {
    const nav_item_t *item = &s_nav_items[s_selected];

    if (!item->opens_section) {
        if (s_selected == NAV_TARGET_HOME) {
            app_ui_replace_id(APP_PAGE_HOME, UI_TRANS_SLIDE_RIGHT, UI_TRANS_DURATION_MS);
        } else {
            app_ui_replace_id(APP_PAGE_SETTINGS, UI_TRANS_SLIDE_LEFT, UI_TRANS_DURATION_MS);
        }
        return;
    }

    page_settings_replace_section(item->section, UI_TRANS_SLIDE_LEFT, UI_TRANS_DURATION_MS);
}

static void on_action(ui_action_t action, const ui_event_t *raw, void *ctx) {
    (void)raw;
    (void)ctx;

    switch (action) {
        case UI_ACTION_NAV_UP:
            move_selection(0, -1);
            sync_view(true);
            break;
        case UI_ACTION_NAV_DOWN:
            move_selection(0, 1);
            sync_view(true);
            break;
        case UI_ACTION_NAV_LEFT:
            move_selection(-1, 0);
            sync_view(true);
            break;
        case UI_ACTION_NAV_RIGHT:
            move_selection(1, 0);
            sync_view(true);
            break;
        case UI_ACTION_CONFIRM:
            open_selected();
            break;
        case UI_ACTION_CANCEL:
            app_ui_pop(UI_TRANS_SLIDE_RIGHT, UI_TRANS_DURATION_MS);
            break;
        default:
            break;
    }
}

static void on_appear(ui_page_t *p) {
    (void)p;

    s_selected = 1;
    sync_view(false);
}

void page_nav_build(void) {
    ui_widget_init(&root, 0, 0, 256, 64, NULL);

    ui_rect_init(&bg, 0, 0, 256, 64, UI_GRAY_BLACK);
    bg.corner_r = 0;
    bg.gradient = false;
    ui_widget_add_child(&root, &bg.base);

    ui_rect_init(&title_bar, 0, 0, 256, 12, UI_GRAY_DIM);
    title_bar.corner_r = 0;
    title_bar.gradient = false;
    ui_widget_add_child(&root, &title_bar.base);

    ui_label_init(&title_text, 8, 2, 70, 8,
                  "NAV", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &title_text.base);
    ui_label_init(&subtitle_text, 162, 3, 86, 5,
                  "PAGES", &g_font_mini,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    subtitle_text.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &subtitle_text.base);

    ui_rect_init(&tile_home, 8, 16, 116, 13, UI_GRAY_SHADOW);
    ui_widget_add_child(&root, &tile_home.base);
    ui_label_init(&tile_home_title, 14, 19, 54, 8,
                  "HOME", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &tile_home_title.base);
    ui_label_init(&tile_home_subtitle, 84, 19, 34, 5,
                  "VIEW", &g_font_mini,
                  UI_GRAY_MUTED, UI_GRAY_BLACK, true);
    tile_home_subtitle.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &tile_home_subtitle.base);

    ui_rect_init(&tile_settings, 132, 16, 116, 13, UI_GRAY_WHITE);
    ui_widget_add_child(&root, &tile_settings.base);
    ui_label_init(&tile_settings_title, 138, 19, 54, 8,
                  "SET", &g_font,
                  UI_GRAY_BLACK, UI_GRAY_WHITE, true);
    ui_widget_add_child(&root, &tile_settings_title.base);
    ui_label_init(&tile_settings_subtitle, 208, 19, 34, 5,
                  "ALL", &g_font_mini,
                  UI_GRAY_SHADOW, UI_GRAY_WHITE, true);
    tile_settings_subtitle.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &tile_settings_subtitle.base);

    ui_rect_init(&tile_input, 8, 33, 116, 13, UI_GRAY_SHADOW);
    ui_widget_add_child(&root, &tile_input.base);
    ui_label_init(&tile_input_title, 14, 36, 54, 8,
                  "INPUT", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &tile_input_title.base);
    ui_label_init(&tile_input_subtitle, 84, 36, 34, 5,
                  "SRC", &g_font_mini,
                  UI_GRAY_MUTED, UI_GRAY_BLACK, true);
    tile_input_subtitle.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &tile_input_subtitle.base);

    ui_rect_init(&tile_asrc, 132, 33, 116, 13, UI_GRAY_SHADOW);
    ui_widget_add_child(&root, &tile_asrc.base);
    ui_label_init(&tile_asrc_title, 138, 36, 54, 8,
                  "ASRC", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &tile_asrc_title.base);
    ui_label_init(&tile_asrc_subtitle, 208, 36, 34, 5,
                  "FMT", &g_font_mini,
                  UI_GRAY_MUTED, UI_GRAY_BLACK, true);
    tile_asrc_subtitle.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &tile_asrc_subtitle.base);

    ui_rect_init(&tile_output, 8, 50, 116, 13, UI_GRAY_SHADOW);
    ui_widget_add_child(&root, &tile_output.base);
    ui_label_init(&tile_output_title, 14, 53, 54, 8,
                  "OUTPUT", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &tile_output_title.base);
    ui_label_init(&tile_output_subtitle, 84, 53, 34, 5,
                  "AMP", &g_font_mini,
                  UI_GRAY_MUTED, UI_GRAY_BLACK, true);
    tile_output_subtitle.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &tile_output_subtitle.base);

    ui_rect_init(&tile_system, 132, 50, 116, 13, UI_GRAY_SHADOW);
    ui_widget_add_child(&root, &tile_system.base);
    ui_label_init(&tile_system_title, 138, 53, 54, 8,
                  "SYSTEM", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &tile_system_title.base);
    ui_label_init(&tile_system_subtitle, 208, 53, 34, 5,
                  "PWR", &g_font_mini,
                  UI_GRAY_MUTED, UI_GRAY_BLACK, true);
    tile_system_subtitle.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &tile_system_subtitle.base);

    sync_view(false);

    app_page_init(&page_nav, &s_page_binding, &root,
                  s_keymap, on_action, on_appear, NULL, NULL);
}
