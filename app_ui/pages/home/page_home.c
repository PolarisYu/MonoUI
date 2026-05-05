#include "page_home.h"
#include "app_ui.h"
#include "home/home_data.h"
#include "home/home_state.h"
#include "settings/page_settings.h"

typedef struct {
    ui_rect_t  *tile;
    ui_label_t *title;
    ui_label_t *line1;
    ui_label_t *line2;
} home_tile_view_t;

static ui_widget_t root;
static ui_rect_t   bg;
static ui_rect_t   top_bar;
static ui_label_t  device_text;
static ui_label_t  power_text;
static ui_label_t  temp_text;
static ui_label_t  time_text;

static ui_rect_t   input_tile;
static ui_label_t  input_title;
static ui_label_t  input_value;
static ui_label_t  input_sub;

static ui_label_t  chain_arrow_1;
static ui_label_t  chain_arrow_2;

static ui_rect_t   asrc_tile;
static ui_label_t  asrc_title;
static ui_label_t  asrc_in_text;
static ui_label_t  asrc_out_text;

static ui_rect_t   output_tile;
static ui_label_t  output_title;
static ui_label_t  output_value;
static ui_label_t  output_sub;

static ui_rect_t   system_tile;
static ui_label_t  system_title;
static ui_label_t  system_value;

ui_page_t page_home;
static app_page_binding_t s_page_binding;

static const ui_keymap_entry_t s_keymap[] = {
    { UI_EVT_ENCODER_CW,     UI_ANY_VALUE, UI_ACTION_NAV_RIGHT },
    { UI_EVT_ENCODER_CCW,    UI_ANY_VALUE, UI_ACTION_NAV_LEFT  },
    { UI_EVT_DPAD_UP,        UI_ANY_VALUE, UI_ACTION_NAV_LEFT  },
    { UI_EVT_DPAD_DOWN,      UI_ANY_VALUE, UI_ACTION_NAV_RIGHT },
    { UI_EVT_DPAD_LEFT,      UI_ANY_VALUE, UI_ACTION_NAV_LEFT  },
    { UI_EVT_DPAD_RIGHT,     UI_ANY_VALUE, UI_ACTION_NAV_RIGHT },
    { UI_EVT_DPAD_CENTER,    UI_ANY_VALUE, UI_ACTION_CONFIRM   },
    { UI_EVT_BTN_LONG_PRESS, UI_ANY_VALUE, UI_ACTION_MENU      },
    UI_KEYMAP_END
};

static void set_tile_style(home_tile_view_t *tile_view, bool selected) {
    ui_gray_t fg = selected ? UI_GRAY_BLACK : UI_GRAY_WHITE;
    ui_gray_t sub_fg = selected ? UI_GRAY_SHADOW : UI_GRAY_LIGHT;

    tile_view->tile->fill_gray = selected ? UI_GRAY_WHITE : UI_GRAY_SHADOW;
    tile_view->title->fg_gray = fg;
    tile_view->line1->fg_gray = fg;
    if (tile_view->line2) {
        tile_view->line2->fg_gray = sub_fg;
    }
}

static void animate_focus(home_tile_view_t *tile_view) {
    tile_view->tile->base.offset_y = 3.f;
    ui_widget_animate(&tile_view->tile->base.offset_y, 3.f, 0.f, 140,
                      ui_ease_out_cubic, NULL, NULL);
}

static void sync_home_view(bool animate_focus_tile) {
    home_tile_view_t input_view  = { &input_tile,  &input_title,  &input_value,  &input_sub  };
    home_tile_view_t asrc_view   = { &asrc_tile,   &asrc_title,   &asrc_in_text, &asrc_out_text };
    home_tile_view_t output_view = { &output_tile, &output_title, &output_value, &output_sub };
    home_tile_view_t system_view = { &system_tile, &system_title, &system_value, NULL };
    home_top_bar_data_t top_bar_data;
    home_tile_data_t input_data;
    home_tile_data_t asrc_data;
    home_tile_data_t output_data;
    home_tile_data_t system_data;
    home_focus_t focus = home_state_focus();

    home_data_top_bar(&top_bar_data);
    home_data_tile(HOME_FOCUS_INPUT, &input_data);
    home_data_tile(HOME_FOCUS_ASRC, &asrc_data);
    home_data_tile(HOME_FOCUS_OUTPUT, &output_data);
    home_data_tile(HOME_FOCUS_SYSTEM, &system_data);

    ui_label_set_text(&device_text, top_bar_data.device_name);
    ui_label_set_text(&power_text, top_bar_data.power_text);
    ui_label_set_text(&temp_text, top_bar_data.temp_text);
    ui_label_set_text(&time_text, top_bar_data.time_text);

    ui_label_set_text(&input_title, input_data.title);
    ui_label_set_text(&input_value, input_data.line1);
    ui_label_set_text(&input_sub, input_data.line2 ? input_data.line2 : "");
    ui_label_set_text(&asrc_title, asrc_data.title);
    ui_label_set_text(&asrc_in_text, asrc_data.line1);
    ui_label_set_text(&asrc_out_text, asrc_data.line2 ? asrc_data.line2 : "");
    ui_label_set_text(&output_title, output_data.title);
    ui_label_set_text(&output_value, output_data.line1);
    ui_label_set_text(&output_sub, output_data.line2 ? output_data.line2 : "");
    ui_label_set_text(&system_title, system_data.title);
    ui_label_set_text(&system_value, system_data.line1);

    set_tile_style(&input_view, focus == HOME_FOCUS_INPUT);
    set_tile_style(&asrc_view, focus == HOME_FOCUS_ASRC);
    set_tile_style(&output_view, focus == HOME_FOCUS_OUTPUT);
    set_tile_style(&system_view, focus == HOME_FOCUS_SYSTEM);

    if (!animate_focus_tile) {
        return;
    }

    switch (focus) {
        case HOME_FOCUS_INPUT:  animate_focus(&input_view); break;
        case HOME_FOCUS_ASRC:   animate_focus(&asrc_view); break;
        case HOME_FOCUS_OUTPUT: animate_focus(&output_view); break;
        case HOME_FOCUS_SYSTEM: animate_focus(&system_view); break;
        default: break;
    }
}

static void open_focused_section(void) {
    page_settings_open_section(home_data_target_section(home_state_focus()),
                               UI_TRANS_SLIDE_LEFT, UI_TRANS_DURATION_MS);
}

static void on_action(ui_action_t action, const ui_event_t *raw, void *ctx) {
    (void)raw;
    (void)ctx;

    switch (action) {
        case UI_ACTION_NAV_LEFT:
            home_state_step_focus(-1);
            sync_home_view(true);
            break;
        case UI_ACTION_NAV_RIGHT:
            home_state_step_focus(1);
            sync_home_view(true);
            break;
        case UI_ACTION_CONFIRM:
            open_focused_section();
            break;
        case UI_ACTION_MENU:
            app_ui_push_id(APP_PAGE_SETTINGS, UI_TRANS_SLIDE_LEFT, UI_TRANS_DURATION_MS);
            break;
        default:
            break;
    }
}

static void on_appear(ui_page_t *p) {
    (void)p;

    home_state_reset();
    top_bar.base.offset_y = -4.f;
    input_tile.base.offset_y = 6.f;
    asrc_tile.base.offset_y = 6.f;
    output_tile.base.offset_y = 6.f;
    system_tile.base.offset_y = 4.f;

    device_text.base.alpha = 0.f;
    power_text.base.alpha = 0.f;
    temp_text.base.alpha = 0.f;
    time_text.base.alpha = 0.f;

    input_title.base.alpha = 0.f;
    input_value.base.alpha = 0.f;
    input_sub.base.alpha = 0.f;
    asrc_title.base.alpha = 0.f;
    asrc_in_text.base.alpha = 0.f;
    asrc_out_text.base.alpha = 0.f;
    output_title.base.alpha = 0.f;
    output_value.base.alpha = 0.f;
    output_sub.base.alpha = 0.f;
    system_title.base.alpha = 0.f;
    system_value.base.alpha = 0.f;

    sync_home_view(false);

    ui_widget_animate(&top_bar.base.offset_y, -4.f, 0.f, 200,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&input_tile.base.offset_y, 6.f, 0.f, 220,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&asrc_tile.base.offset_y, 6.f, 0.f, 260,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&output_tile.base.offset_y, 6.f, 0.f, 300,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&system_tile.base.offset_y, 4.f, 0.f, 340,
                      ui_ease_out_cubic, NULL, NULL);

    ui_widget_animate(&device_text.base.alpha, 0.f, 1.f, 180,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&power_text.base.alpha, 0.f, 1.f, 220,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&temp_text.base.alpha, 0.f, 1.f, 260,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&time_text.base.alpha, 0.f, 1.f, 300,
                      ui_ease_out_cubic, NULL, NULL);

    ui_widget_animate(&input_title.base.alpha, 0.f, 1.f, 220,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&input_value.base.alpha, 0.f, 1.f, 260,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&input_sub.base.alpha, 0.f, 1.f, 300,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&asrc_title.base.alpha, 0.f, 1.f, 260,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&asrc_in_text.base.alpha, 0.f, 1.f, 300,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&asrc_out_text.base.alpha, 0.f, 1.f, 340,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&output_title.base.alpha, 0.f, 1.f, 300,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&output_value.base.alpha, 0.f, 1.f, 340,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&output_sub.base.alpha, 0.f, 1.f, 380,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&system_title.base.alpha, 0.f, 1.f, 360,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&system_value.base.alpha, 0.f, 1.f, 400,
                      ui_ease_out_cubic, NULL, NULL);
}

void page_home_build(void) {
    ui_widget_init(&root, 0, 0, 256, 64, NULL);

    ui_rect_init(&bg, 0, 0, 256, 64, UI_GRAY_BLACK);
    bg.corner_r = 0;
    bg.gradient = false;
    ui_widget_add_child(&root, &bg.base);

    ui_rect_init(&top_bar, 0, 0, 256, 14, UI_GRAY_DIM);
    top_bar.corner_r = 0;
    top_bar.gradient = false;
    ui_widget_add_child(&root, &top_bar.base);

    ui_label_init(&device_text, 6, 3, 74, 8,
                  "", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &device_text.base);

    ui_label_init(&power_text, 86, 3, 50, 8,
                  "", &g_font,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    power_text.align = UI_ALIGN_CENTER;
    ui_widget_add_child(&root, &power_text.base);

    ui_label_init(&temp_text, 146, 3, 28, 8,
                  "", &g_font,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    temp_text.align = UI_ALIGN_CENTER;
    ui_widget_add_child(&root, &temp_text.base);

    ui_label_init(&time_text, 190, 3, 58, 8,
                  "", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    time_text.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &time_text.base);

    ui_rect_init(&input_tile, 8, 18, 50, 28, UI_GRAY_WHITE);
    input_tile.corner_r = 0;
    input_tile.gradient = false;
    ui_widget_add_child(&root, &input_tile.base);

    ui_label_init(&input_title, 14, 22, 38, 8,
                  "INPUT", &g_font,
                  UI_GRAY_BLACK, UI_GRAY_WHITE, true);
    ui_widget_add_child(&root, &input_title.base);

    ui_label_init(&input_value, 14, 32, 38, 8,
                  "", &g_font,
                  UI_GRAY_BLACK, UI_GRAY_WHITE, true);
    ui_widget_add_child(&root, &input_value.base);

    ui_label_init(&input_sub, 14, 40, 38, 8,
                  "SRC", &g_font,
                  UI_GRAY_SHADOW, UI_GRAY_WHITE, true);
    ui_widget_add_child(&root, &input_sub.base);

    ui_label_init(&chain_arrow_1, 60, 28, 8, 8,
                  ">", &g_font,
                  UI_GRAY_DIM, UI_GRAY_BLACK, true);
    chain_arrow_1.align = UI_ALIGN_CENTER;
    ui_widget_add_child(&root, &chain_arrow_1.base);

    ui_rect_init(&asrc_tile, 68, 18, 120, 28, UI_GRAY_SHADOW);
    asrc_tile.corner_r = 0;
    asrc_tile.gradient = false;
    ui_widget_add_child(&root, &asrc_tile.base);

    ui_label_init(&asrc_title, 74, 22, 30, 8,
                  "ASRC", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &asrc_title.base);

    ui_label_init(&asrc_in_text, 108, 22, 72, 8,
                  "", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    asrc_in_text.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &asrc_in_text.base);

    ui_label_init(&asrc_out_text, 74, 32, 106, 8,
                  "", &g_font,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    asrc_out_text.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &asrc_out_text.base);

    ui_label_init(&chain_arrow_2, 190, 28, 8, 8,
                  ">", &g_font,
                  UI_GRAY_DIM, UI_GRAY_BLACK, true);
    chain_arrow_2.align = UI_ALIGN_CENTER;
    ui_widget_add_child(&root, &chain_arrow_2.base);

    ui_rect_init(&output_tile, 198, 18, 50, 28, UI_GRAY_SHADOW);
    output_tile.corner_r = 0;
    output_tile.gradient = false;
    ui_widget_add_child(&root, &output_tile.base);

    ui_label_init(&output_title, 204, 22, 38, 8,
                  "OUT", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &output_title.base);

    ui_label_init(&output_value, 204, 32, 38, 8,
                  "", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &output_value.base);

    ui_label_init(&output_sub, 204, 40, 38, 8,
                  "AMP", &g_font,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &output_sub.base);

    ui_rect_init(&system_tile, 8, 50, 240, 10, UI_GRAY_SHADOW);
    system_tile.corner_r = 0;
    system_tile.gradient = false;
    ui_widget_add_child(&root, &system_tile.base);

    ui_label_init(&system_title, 14, 51, 48, 8,
                  "SYSTEM", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &system_title.base);

    ui_label_init(&system_value, 96, 51, 146, 8,
                  "", &g_font,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    system_value.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &system_value.base);

    sync_home_view(false);

    app_page_init(&page_home, &s_page_binding, &root,
                  s_keymap, on_action, on_appear, NULL, NULL);
}
