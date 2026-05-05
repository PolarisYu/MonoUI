#include "page_home.h"
#include "app_ui.h"
#include "device/device_status.h"
#include "home/home_data.h"
#include "home/home_state.h"
#include "pages/settings/page_settings.h"
#include "settings/settings_state.h"
#include <stdio.h>

typedef struct {
    ui_rect_t  *tile;
    ui_rect_t  *mark;
    ui_label_t *title;
    ui_label_t *line1;
    ui_label_t *line2;
} home_tile_view_t;

static ui_widget_t root;
static ui_rect_t   bg;
static ui_rect_t   top_bar;
static ui_label_t  device_text;

static ui_rect_t   power_badge;
static ui_label_t  power_badge_text;
static ui_label_t  power_text;
static ui_rect_t   temp_badge;
static ui_label_t  temp_badge_text;
static ui_label_t  temp_text;
static ui_rect_t   time_badge;
static ui_label_t  time_badge_text;
static ui_label_t  time_text;

static ui_rect_t   nav_strip;
static ui_label_t  nav_n1;
static ui_label_t  nav_n2;
static ui_label_t  nav_n3;
static ui_rect_t   nav_panel;
static ui_label_t  nav_title;
static ui_label_t  nav_value;
static ui_label_t  nav_menu_text;

static ui_rect_t   input_tile;
static ui_rect_t   input_mark;
static ui_label_t  input_title;
static ui_label_t  input_value;
static ui_label_t  input_sub;

static ui_label_t  chain_arrow_1;
static ui_label_t  chain_arrow_2;

static ui_rect_t   asrc_tile;
static ui_rect_t   asrc_mark;
static ui_label_t  asrc_title;
static ui_label_t  asrc_lock_text;
static ui_label_t  asrc_mode_text;
static ui_label_t  asrc_in_label;
static ui_label_t  asrc_in_text;
static ui_label_t  asrc_out_label;
static ui_label_t  asrc_out_text;
static ui_rect_t   asrc_vol_bg;
static ui_rect_t   asrc_vol_fill;
static ui_label_t  asrc_vol_text;

static ui_rect_t   output_tile;
static ui_rect_t   output_mark;
static ui_label_t  output_title;
static ui_label_t  output_value;
static ui_label_t  output_sub;
static ui_rect_t   output_vol_bg;
static ui_rect_t   output_vol_fill;
static ui_label_t  output_vol_text;

ui_page_t page_home;
static app_page_binding_t s_page_binding;
static bool s_focus_visible = true;
static uint32_t s_idle_ms = 0;
static char s_asrc_vol_buf[16];
static char s_output_vol_buf[16];
static char s_output_info_buf[16];

#define HOME_IDLE_HIDE_MS 10000u

static const char *const s_home_filter_options[] = { "FAST", "BAL", "NOS" };

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

static void set_tile_style(home_tile_view_t *view, bool selected) {
    ui_gray_t fg = selected ? UI_GRAY_BLACK : UI_GRAY_WHITE;
    ui_gray_t sub_fg = selected ? UI_GRAY_SHADOW : UI_GRAY_MUTED;

    view->tile->fill_gray = selected ? UI_GRAY_WHITE : UI_GRAY_SHADOW;
    view->tile->border_w = 0;
    view->mark->fill_gray = selected ? UI_GRAY_BLACK : UI_GRAY_BRIGHT;
    view->title->fg_gray = selected ? UI_GRAY_SHADOW : UI_GRAY_LIGHT;
    view->line1->fg_gray = fg;
    if (view->line2) {
        view->line2->fg_gray = sub_fg;
    }
}

static void animate_focus(home_tile_view_t *view) {
    view->tile->base.offset_y = 3.f;
    ui_widget_animate(&view->tile->base.offset_y, 3.f, 0.f, 140,
                      ui_ease_out_cubic, NULL, NULL);
}

static void set_vol_bar(ui_rect_t *fill, ui_label_t *label, char *buf, size_t buf_size,
                        int value, int full_width, bool selected) {
    int clamped = value;
    int w;

    if (clamped < 0) clamped = 0;
    if (clamped > 100) clamped = 100;

    w = (full_width * clamped) / 100;
    if (clamped > 0 && w < 2) {
        w = 2;
    }

    fill->base.w = (uint16_t)w;
    fill->fill_gray = selected ? UI_GRAY_BLACK : UI_GRAY_WHITE;
    snprintf(buf, buf_size, "VOL %d", clamped);
    ui_label_set_text(label, buf);
    label->fg_gray = selected ? UI_GRAY_BLACK : UI_GRAY_LIGHT;
}

static void set_nav_style(bool selected) {
    nav_strip.fill_gray = selected ? UI_GRAY_WHITE : UI_GRAY_SHADOW;
    nav_n1.fg_gray = selected ? UI_GRAY_BLACK : UI_GRAY_WHITE;
    nav_n2.fg_gray = selected ? UI_GRAY_BLACK : UI_GRAY_WHITE;
    nav_n3.fg_gray = selected ? UI_GRAY_BLACK : UI_GRAY_WHITE;

    ui_widget_set_visible(&nav_panel.base, selected);
    ui_widget_set_visible(&nav_title.base, selected);
    ui_widget_set_visible(&nav_value.base, selected);
    ui_widget_set_visible(&nav_menu_text.base, selected);

    nav_panel.base.alpha = selected ? 1.f : 0.f;
    nav_title.fg_gray = selected ? UI_GRAY_SHADOW : UI_GRAY_LIGHT;
    nav_value.fg_gray = selected ? UI_GRAY_BLACK : UI_GRAY_LIGHT;
    nav_menu_text.fg_gray = selected ? UI_GRAY_BLACK : UI_GRAY_MUTED;
}

static void animate_nav_focus(void) {
    nav_panel.base.alpha = 0.f;
    nav_panel.base.offset_x = -10.f;
    nav_title.base.alpha = 0.f;
    nav_value.base.alpha = 0.f;
    nav_menu_text.base.alpha = 0.f;
    nav_title.base.offset_x = -6.f;
    nav_value.base.offset_x = -6.f;
    nav_menu_text.base.offset_x = -6.f;

    ui_widget_animate(&nav_panel.base.alpha, 0.f, 1.f, 140,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&nav_panel.base.offset_x, -10.f, 0.f, 140,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&nav_title.base.alpha, 0.f, 1.f, 120,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&nav_value.base.alpha, 0.f, 1.f, 140,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&nav_menu_text.base.alpha, 0.f, 1.f, 160,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&nav_title.base.offset_x, -6.f, 0.f, 140,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&nav_value.base.offset_x, -6.f, 0.f, 140,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&nav_menu_text.base.offset_x, -6.f, 0.f, 140,
                      ui_ease_out_cubic, NULL, NULL);
}

static void sync_home_view(bool animate_focus_tile) {
    home_tile_view_t input_view  = { &input_tile,  &input_mark,  &input_title,  &input_value,  &input_sub };
    home_tile_view_t asrc_view   = { &asrc_tile,   &asrc_mark,   &asrc_title,   &asrc_in_text, &asrc_out_text };
    home_tile_view_t output_view = { &output_tile, &output_mark, &output_title, &output_value, &output_sub };
    home_top_bar_data_t top_bar_data;
    home_tile_data_t input_data;
    home_tile_data_t asrc_data;
    home_tile_data_t output_data;
    device_status_snapshot_t snapshot;
    home_focus_t focus = home_state_focus();
    bool input_selected;
    bool asrc_selected;
    bool output_selected;
    bool nav_selected;

    home_data_top_bar(&top_bar_data);
    home_data_tile(HOME_FOCUS_INPUT, &input_data);
    home_data_tile(HOME_FOCUS_ASRC, &asrc_data);
    home_data_tile(HOME_FOCUS_OUTPUT, &output_data);
    device_status_get_snapshot(&snapshot);

    ui_label_set_text(&device_text, top_bar_data.device_name);
    ui_label_set_text(&power_text, top_bar_data.power_text);
    ui_label_set_text(&temp_text, top_bar_data.temp_text);
    ui_label_set_text(&time_text, top_bar_data.time_text);

    ui_label_set_text(&input_title, input_data.title);
    ui_label_set_text(&input_value, input_data.line1);
    ui_label_set_text(&input_sub, "SRC");
    ui_label_set_text(&asrc_title, asrc_data.title);
    ui_label_set_text(&asrc_lock_text, "LOCK");
    ui_label_set_text(&asrc_mode_text, g_settings_state.asrc.enable ? "SYNC" : "BYP");
    ui_label_set_text(&asrc_in_label, "IN");
    ui_label_set_text(&asrc_in_text, asrc_data.line1);
    ui_label_set_text(&asrc_out_label, "OUT");
    ui_label_set_text(&asrc_out_text, asrc_data.line2 ? asrc_data.line2 : "");
    ui_label_set_text(&output_title, output_data.title);
    snprintf(s_output_info_buf, sizeof(s_output_info_buf), "DAC %s",
             s_home_filter_options[g_settings_state.asrc.filter]);
    ui_label_set_text(&output_value, output_data.line1);
    ui_label_set_text(&output_sub, s_output_info_buf);
    ui_label_set_text(&nav_title, "NAV");
    ui_label_set_text(&nav_value, "PAGES");
    ui_label_set_text(&nav_menu_text, "OPEN");

    input_selected = s_focus_visible && (focus == HOME_FOCUS_INPUT);
    asrc_selected = s_focus_visible && (focus == HOME_FOCUS_ASRC);
    output_selected = s_focus_visible && (focus == HOME_FOCUS_OUTPUT);
    nav_selected = s_focus_visible && (focus == HOME_FOCUS_NAV);

    set_tile_style(&input_view, input_selected);
    set_tile_style(&asrc_view, asrc_selected);
    set_tile_style(&output_view, output_selected);
    set_nav_style(nav_selected);

    asrc_lock_text.fg_gray = snapshot.input_lock
                           ? (asrc_selected ? UI_GRAY_BLACK : UI_GRAY_WHITE)
                           : (asrc_selected ? UI_GRAY_SHADOW : UI_GRAY_MUTED);
    asrc_mode_text.fg_gray = g_settings_state.asrc.enable
                           ? (asrc_selected ? UI_GRAY_BLACK : UI_GRAY_WHITE)
                           : (asrc_selected ? UI_GRAY_SHADOW : UI_GRAY_MUTED);
    asrc_in_label.fg_gray = asrc_selected ? UI_GRAY_SHADOW : UI_GRAY_MUTED;
    asrc_out_label.fg_gray = asrc_selected ? UI_GRAY_SHADOW : UI_GRAY_MUTED;
    asrc_vol_bg.fill_gray = asrc_selected ? UI_GRAY_LIGHT : UI_GRAY_DIM;
    output_vol_bg.fill_gray = output_selected ? UI_GRAY_LIGHT : UI_GRAY_DIM;
    set_vol_bar(&asrc_vol_fill, &asrc_vol_text, s_asrc_vol_buf, sizeof(s_asrc_vol_buf),
                g_settings_state.output.volume, 91, asrc_selected);
    set_vol_bar(&output_vol_fill, &output_vol_text, s_output_vol_buf, sizeof(s_output_vol_buf),
                g_settings_state.output.volume, 38, output_selected);
    chain_arrow_1.fg_gray = asrc_selected ? UI_GRAY_LIGHT : UI_GRAY_DIM;
    chain_arrow_2.fg_gray = (asrc_selected || output_selected) ? UI_GRAY_LIGHT : UI_GRAY_DIM;

    if (!animate_focus_tile || !s_focus_visible) {
        return;
    }

    switch (focus) {
        case HOME_FOCUS_INPUT:  animate_focus(&input_view); break;
        case HOME_FOCUS_ASRC:   animate_focus(&asrc_view); break;
        case HOME_FOCUS_OUTPUT: animate_focus(&output_view); break;
        case HOME_FOCUS_NAV: animate_nav_focus(); break;
        default: break;
    }
}

static void open_focused_target(void) {
    if (home_state_focus() == HOME_FOCUS_NAV) {
        app_ui_push_id(APP_PAGE_NAV, UI_TRANS_SLIDE_LEFT, UI_TRANS_DURATION_MS);
        return;
    }

    page_settings_open_section(home_data_target_section(home_state_focus()),
                               UI_TRANS_SLIDE_LEFT, UI_TRANS_DURATION_MS);
}

static void on_action(ui_action_t action, const ui_event_t *raw, void *ctx) {
    (void)raw;
    (void)ctx;

    s_idle_ms = 0;
    if (!s_focus_visible) {
        s_focus_visible = true;
        sync_home_view(true);
        return;
    }

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
            open_focused_target();
            break;
        case UI_ACTION_MENU:
            app_ui_push_id(APP_PAGE_NAV, UI_TRANS_SLIDE_LEFT, UI_TRANS_DURATION_MS);
            break;
        default:
            break;
    }
}

static void on_appear(ui_page_t *p) {
    (void)p;

    s_focus_visible = true;
    s_idle_ms = 0;
    sync_home_view(false);
}

void page_home_build(void) {
    ui_widget_init(&root, 0, 0, 256, 64, NULL);

    ui_rect_init(&bg, 0, 0, 256, 64, UI_GRAY_BLACK);
    bg.corner_r = 0;
    bg.gradient = false;
    ui_widget_add_child(&root, &bg.base);

    ui_rect_init(&top_bar, 0, 0, 256, 12, UI_GRAY_DIM);
    top_bar.corner_r = 0;
    top_bar.gradient = false;
    ui_widget_add_child(&root, &top_bar.base);

    ui_label_init(&device_text, 6, 2, 82, 8,
                  "", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &device_text.base);

    ui_rect_init(&power_badge, 90, 3, 7, 5, UI_GRAY_WHITE);
    ui_widget_add_child(&root, &power_badge.base);
    ui_label_init(&power_badge_text, 92, 3, 3, 5,
                  "P", &g_font_mini,
                  UI_GRAY_BLACK, UI_GRAY_WHITE, true);
    ui_widget_add_child(&root, &power_badge_text.base);
    ui_label_init(&power_text, 100, 3, 28, 5,
                  "", &g_font_mini,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &power_text.base);

    ui_rect_init(&temp_badge, 132, 3, 7, 5, UI_GRAY_WHITE);
    ui_widget_add_child(&root, &temp_badge.base);
    ui_label_init(&temp_badge_text, 134, 3, 3, 5,
                  "T", &g_font_mini,
                  UI_GRAY_BLACK, UI_GRAY_WHITE, true);
    ui_widget_add_child(&root, &temp_badge_text.base);
    ui_label_init(&temp_text, 142, 3, 18, 5,
                  "", &g_font_mini,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &temp_text.base);

    ui_rect_init(&time_badge, 164, 3, 7, 5, UI_GRAY_WHITE);
    ui_widget_add_child(&root, &time_badge.base);
    ui_label_init(&time_badge_text, 166, 3, 3, 5,
                  "C", &g_font_mini,
                  UI_GRAY_BLACK, UI_GRAY_WHITE, true);
    ui_widget_add_child(&root, &time_badge_text.base);
    ui_label_init(&time_text, 174, 3, 32, 5,
                  "", &g_font_mini,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &time_text.base);

    ui_rect_init(&nav_panel, 8, 15, 56, 47, UI_GRAY_WHITE);
    nav_panel.corner_r = 0;
    nav_panel.gradient = false;
    ui_widget_add_child(&root, &nav_panel.base);
    ui_label_init(&nav_title, 14, 19, 44, 5,
                  "NAV", &g_font_mini,
                  UI_GRAY_SHADOW, UI_GRAY_WHITE, true);
    nav_title.align = UI_ALIGN_CENTER;
    ui_widget_add_child(&root, &nav_title.base);
    ui_label_init(&nav_value, 14, 29, 44, 5,
                  "PAGES", &g_font_mini,
                  UI_GRAY_BLACK, UI_GRAY_WHITE, true);
    nav_value.align = UI_ALIGN_CENTER;
    ui_widget_add_child(&root, &nav_value.base);
    ui_label_init(&nav_menu_text, 14, 42, 44, 8,
                  "OPEN", &g_font,
                  UI_GRAY_BLACK, UI_GRAY_WHITE, true);
    nav_menu_text.align = UI_ALIGN_CENTER;
    ui_widget_add_child(&root, &nav_menu_text.base);

    ui_rect_init(&nav_strip, 8, 15, 12, 47, UI_GRAY_SHADOW);
    nav_strip.corner_r = 0;
    nav_strip.gradient = false;
    ui_widget_add_child(&root, &nav_strip.base);
    ui_label_init(&nav_n1, 12, 21, 4, 5,
                  "N", &g_font_mini,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &nav_n1.base);
    ui_label_init(&nav_n2, 12, 34, 4, 5,
                  "A", &g_font_mini,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &nav_n2.base);
    ui_label_init(&nav_n3, 12, 47, 4, 5,
                  "V", &g_font_mini,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &nav_n3.base);

    ui_rect_init(&input_tile, 24, 15, 36, 47, UI_GRAY_WHITE);
    input_tile.corner_r = 0;
    input_tile.gradient = false;
    ui_widget_add_child(&root, &input_tile.base);
    ui_rect_init(&input_mark, 29, 19, 3, 5, UI_GRAY_BLACK);
    ui_widget_add_child(&root, &input_mark.base);
    ui_label_init(&input_title, 35, 19, 20, 5,
                  "INPUT", &g_font_mini,
                  UI_GRAY_SHADOW, UI_GRAY_WHITE, true);
    ui_widget_add_child(&root, &input_title.base);
    ui_label_init(&input_value, 27, 31, 30, 8,
                  "", &g_font,
                  UI_GRAY_BLACK, UI_GRAY_WHITE, true);
    input_value.align = UI_ALIGN_CENTER;
    ui_widget_add_child(&root, &input_value.base);
    ui_label_init(&input_sub, 27, 50, 30, 5,
                  "SRC", &g_font_mini,
                  UI_GRAY_SHADOW, UI_GRAY_WHITE, true);
    input_sub.align = UI_ALIGN_CENTER;
    ui_widget_add_child(&root, &input_sub.base);

    ui_label_init(&chain_arrow_1, 64, 34, 8, 8,
                  ">", &g_font,
                  UI_GRAY_DIM, UI_GRAY_BLACK, true);
    chain_arrow_1.align = UI_ALIGN_CENTER;
    ui_widget_add_child(&root, &chain_arrow_1.base);

    ui_rect_init(&asrc_tile, 74, 15, 112, 47, UI_GRAY_SHADOW);
    asrc_tile.corner_r = 0;
    asrc_tile.gradient = false;
    ui_widget_add_child(&root, &asrc_tile.base);
    ui_rect_init(&asrc_mark, 80, 19, 3, 5, UI_GRAY_BRIGHT);
    ui_widget_add_child(&root, &asrc_mark.base);
    ui_label_init(&asrc_title, 86, 19, 24, 5,
                  "ASRC", &g_font_mini,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &asrc_title.base);
    ui_label_init(&asrc_lock_text, 136, 19, 20, 5,
                  "LOCK", &g_font_mini,
                  UI_GRAY_MUTED, UI_GRAY_BLACK, true);
    asrc_lock_text.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &asrc_lock_text.base);
    ui_label_init(&asrc_mode_text, 160, 19, 18, 5,
                  "SYNC", &g_font_mini,
                  UI_GRAY_MUTED, UI_GRAY_BLACK, true);
    asrc_mode_text.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &asrc_mode_text.base);
    ui_label_init(&asrc_in_label, 86, 29, 12, 5,
                  "IN", &g_font_mini,
                  UI_GRAY_MUTED, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &asrc_in_label.base);
    ui_label_init(&asrc_in_text, 100, 28, 78, 8,
                  "", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    asrc_in_text.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &asrc_in_text.base);
    ui_label_init(&asrc_out_label, 86, 39, 12, 5,
                  "OUT", &g_font_mini,
                  UI_GRAY_MUTED, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &asrc_out_label.base);
    ui_label_init(&asrc_out_text, 100, 39, 78, 5,
                  "", &g_font_mini,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    asrc_out_text.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &asrc_out_text.base);
    ui_rect_init(&asrc_vol_bg, 86, 54, 91, 4, UI_GRAY_DIM);
    asrc_vol_bg.border_w = 0;
    ui_widget_add_child(&root, &asrc_vol_bg.base);
    ui_rect_init(&asrc_vol_fill, 86, 54, 40, 4, UI_GRAY_WHITE);
    asrc_vol_fill.border_w = 0;
    ui_widget_add_child(&root, &asrc_vol_fill.base);
    ui_label_init(&asrc_vol_text, 86, 48, 91, 5,
                  "VOL 0", &g_font_mini,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    asrc_vol_text.align = UI_ALIGN_LEFT;
    ui_widget_add_child(&root, &asrc_vol_text.base);

    ui_label_init(&chain_arrow_2, 188, 34, 8, 8,
                  ">", &g_font,
                  UI_GRAY_DIM, UI_GRAY_BLACK, true);
    chain_arrow_2.align = UI_ALIGN_CENTER;
    ui_widget_add_child(&root, &chain_arrow_2.base);

    ui_rect_init(&output_tile, 198, 15, 50, 47, UI_GRAY_SHADOW);
    output_tile.corner_r = 0;
    output_tile.gradient = false;
    ui_widget_add_child(&root, &output_tile.base);
    ui_rect_init(&output_mark, 203, 19, 3, 5, UI_GRAY_BRIGHT);
    ui_widget_add_child(&root, &output_mark.base);
    ui_label_init(&output_title, 209, 19, 22, 5,
                  "OUT", &g_font_mini,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &output_title.base);
    ui_label_init(&output_value, 204, 29, 38, 8,
                  "", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    output_value.align = UI_ALIGN_LEFT;
    ui_widget_add_child(&root, &output_value.base);
    ui_label_init(&output_sub, 204, 40, 38, 5,
                  "DAC BAL", &g_font_mini,
                  UI_GRAY_MUTED, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &output_sub.base);
    ui_rect_init(&output_vol_bg, 204, 54, 38, 4, UI_GRAY_DIM);
    output_vol_bg.border_w = 0;
    ui_widget_add_child(&root, &output_vol_bg.base);
    ui_rect_init(&output_vol_fill, 204, 54, 24, 4, UI_GRAY_WHITE);
    output_vol_fill.border_w = 0;
    ui_widget_add_child(&root, &output_vol_fill.base);
    ui_label_init(&output_vol_text, 204, 48, 38, 5,
                  "VOL 0", &g_font_mini,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    output_vol_text.align = UI_ALIGN_LEFT;
    ui_widget_add_child(&root, &output_vol_text.base);

    /* 让 NAV 展开面板作为覆盖层显示在主链路之上 */
    ui_widget_remove_child(&root, &nav_panel.base);
    ui_widget_remove_child(&root, &nav_title.base);
    ui_widget_remove_child(&root, &nav_value.base);
    ui_widget_remove_child(&root, &nav_menu_text.base);
    ui_widget_remove_child(&root, &nav_strip.base);
    ui_widget_remove_child(&root, &nav_n1.base);
    ui_widget_remove_child(&root, &nav_n2.base);
    ui_widget_remove_child(&root, &nav_n3.base);
    ui_widget_add_child(&root, &nav_panel.base);
    ui_widget_add_child(&root, &nav_title.base);
    ui_widget_add_child(&root, &nav_value.base);
    ui_widget_add_child(&root, &nav_menu_text.base);
    ui_widget_add_child(&root, &nav_strip.base);
    ui_widget_add_child(&root, &nav_n1.base);
    ui_widget_add_child(&root, &nav_n2.base);
    ui_widget_add_child(&root, &nav_n3.base);

    sync_home_view(false);

    app_page_init(&page_home, &s_page_binding, &root,
                  s_keymap, on_action, on_appear, NULL, NULL);
}

void page_home_tick(uint32_t delta_ms) {
    if (!s_focus_visible) {
        return;
    }

    if (s_idle_ms < HOME_IDLE_HIDE_MS) {
        s_idle_ms += delta_ms;
        if (s_idle_ms >= HOME_IDLE_HIDE_MS) {
            s_focus_visible = false;
            sync_home_view(false);
        }
    }
}
