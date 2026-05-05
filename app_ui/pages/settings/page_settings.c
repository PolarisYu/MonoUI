#include "page_settings.h"
#include "app_ui.h"
#include <stdbool.h>

#define SETTINGS_ROOT_COUNT         APP_SETTINGS_SECTION_COUNT
#define SETTINGS_MAX_ITEMS          6
#define SETTINGS_VISIBLE_ROWS       3
#define SETTINGS_EDITOR_CHOICES     3
#define SETTINGS_RAIL_CHAR_MAX      5
#define SETTINGS_RAIL_SLOT_H        32
#define SETTINGS_RAIL_CHAR_STEP     6
#define SETTINGS_ROW_PITCH          12
#define SETTINGS_LEFT_W             14
#define SETTINGS_LIST_X             20
#define SETTINGS_LIST_Y             8
#define SETTINGS_LIST_W             168
#define SETTINGS_SECTION_LIST_W_COLLAPSED SETTINGS_LIST_W
#define SETTINGS_SECTION_LIST_W_EXPANDED  234
#define SETTINGS_ROOT_LIST_W              SETTINGS_SECTION_LIST_W_EXPANDED
#define SETTINGS_EDITOR_X           188
#define SETTINGS_EDITOR_W           68
#define SETTINGS_ROW_W              160
#define SETTINGS_SECTION_ROW_W_COLLAPSED  SETTINGS_ROW_W
#define SETTINGS_SECTION_ROW_W_EXPANDED   226
#define SETTINGS_ROOT_ROW_W               SETTINGS_SECTION_ROW_W_EXPANDED
#define SETTINGS_SWITCH_TRACK_W     20
#define SETTINGS_SWITCH_THUMB_W     8
#define SETTINGS_SWITCH_TRACK_X     132
#define SETTINGS_SWITCH_THUMB_OFF_X 134
#define SETTINGS_SWITCH_THUMB_ON_X  142
#define SETTINGS_SECTION_VALUE_W_COLLAPSED 50
#define SETTINGS_SECTION_VALUE_W_EXPANDED  68
#define SETTINGS_ROOT_VALUE_W              SETTINGS_SECTION_VALUE_W_EXPANDED
#define SETTINGS_SECTION_OVERLAY_OFFSET_X  (UI_SCREEN_W - SETTINGS_LEFT_W)

typedef struct {
    ui_rect_t  bg;
    ui_label_t title;
    ui_label_t value;
    ui_rect_t  switch_track;
    ui_rect_t  switch_thumb;
} settings_row_widgets_t;

typedef struct {
    ui_rect_t  bg;
    ui_label_t chars[SETTINGS_RAIL_CHAR_MAX];
    char       text[SETTINGS_RAIL_CHAR_MAX][2];
} settings_rail_title_t;

static int  s_root_selected = 0;
static app_settings_section_t s_active_section = APP_SETTINGS_SECTION_INPUT;
static int  s_section_selected = 0;
static bool s_editor_open = false;
static bool s_editor_visible = false;
static bool s_editor_opening = false;
static bool s_section_enter_from_root = false;
static bool s_section_overlay_animating = false;
static bool s_pages_built = false;

static void sync_section_rows(bool animate_scroll);
static void sync_editor_panel(void);
static void close_editor_immediate(void);

static char s_root_value_text[SETTINGS_ROOT_COUNT][20];
static char s_row_value_text[SETTINGS_MAX_ITEMS][20];
static char s_editor_value_text[20];
static ui_widget_t  s_root_widget;
static ui_rect_t    s_root_bg;
static settings_rail_title_t s_root_parent_title;
static settings_rail_title_t s_root_child_title;
static ui_widget_t  s_root_content;
static settings_row_widgets_t s_root_rows[SETTINGS_ROOT_COUNT];

static ui_widget_t  s_section_widget;
static ui_rect_t    s_section_bg;
static settings_rail_title_t s_section_parent_title;
static ui_widget_t  s_section_overlay;
static settings_rail_title_t s_section_active_title;
static ui_widget_t  s_section_content;
static settings_row_widgets_t s_section_rows[SETTINGS_MAX_ITEMS];
static ui_widget_t  s_editor_widget;
static ui_rect_t    s_editor_panel;
static ui_rect_t    s_editor_head;
static ui_label_t   s_editor_title;
static ui_label_t   s_editor_mode;
static ui_rect_t    s_editor_choice_bg[SETTINGS_EDITOR_CHOICES];
static ui_label_t   s_editor_choice_label[SETTINGS_EDITOR_CHOICES];
static ui_rect_t    s_editor_switch_track;
static ui_rect_t    s_editor_switch_thumb;
static ui_label_t   s_editor_state_label;
static ui_rect_t    s_editor_bar_track;
static ui_rect_t    s_editor_bar_fill;
static ui_label_t   s_editor_value_label;

ui_page_t page_settings;
ui_page_t page_settings_section;

static app_page_binding_t s_root_binding;
static app_page_binding_t s_section_binding;

static const ui_keymap_entry_t s_root_keymap[] = {
    { UI_EVT_ENCODER_CW,     UI_ANY_VALUE, UI_ACTION_SCROLL_DOWN },
    { UI_EVT_ENCODER_CCW,    UI_ANY_VALUE, UI_ACTION_SCROLL_UP   },
    { UI_EVT_DPAD_UP,        UI_ANY_VALUE, UI_ACTION_SCROLL_UP   },
    { UI_EVT_DPAD_DOWN,      UI_ANY_VALUE, UI_ACTION_SCROLL_DOWN },
    { UI_EVT_DPAD_RIGHT,     UI_ANY_VALUE, UI_ACTION_CONFIRM     },
    { UI_EVT_DPAD_CENTER,    UI_ANY_VALUE, UI_ACTION_CONFIRM     },
    { UI_EVT_DPAD_LEFT,      UI_ANY_VALUE, UI_ACTION_CANCEL      },
    UI_KEYMAP_END
};

static const ui_keymap_entry_t s_section_keymap[] = {
    { UI_EVT_ENCODER_CW,     UI_ANY_VALUE, UI_ACTION_SCROLL_DOWN },
    { UI_EVT_ENCODER_CCW,    UI_ANY_VALUE, UI_ACTION_SCROLL_UP   },
    { UI_EVT_DPAD_UP,        UI_ANY_VALUE, UI_ACTION_SCROLL_UP   },
    { UI_EVT_DPAD_DOWN,      UI_ANY_VALUE, UI_ACTION_SCROLL_DOWN },
    { UI_EVT_DPAD_RIGHT,     UI_ANY_VALUE, UI_ACTION_CONFIRM     },
    { UI_EVT_DPAD_CENTER,    UI_ANY_VALUE, UI_ACTION_CONFIRM     },
    { UI_EVT_DPAD_LEFT,      UI_ANY_VALUE, UI_ACTION_CANCEL      },
    UI_KEYMAP_END
};

static int clamp_int(int value, int min_value, int max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static const settings_section_desc_t *section_def(app_settings_section_t section) {
    int index = clamp_int((int)section, 0, SETTINGS_ROOT_COUNT - 1);
    return settings_data_section((app_settings_section_t)index);
}

static const settings_item_desc_t *section_item_def(app_settings_section_t section, int index) {
    const settings_section_desc_t *section_desc = section_def(section);

    if (!section_desc || section_desc->item_count <= 0) {
        return NULL;
    }
    index = clamp_int(index, 0, section_desc->item_count - 1);
    return &section_desc->items[index];
}

static const settings_item_desc_t *current_item_def(void) {
    return section_item_def(s_active_section, s_section_selected);
}

static int current_item_count(void) {
    const settings_section_desc_t *section_desc = section_def(s_active_section);
    return section_desc ? section_desc->item_count : 0;
}

static bool item_toggle_value(const settings_item_desc_t *item) {
    return item && item->kind == SETTINGS_KIND_TOGGLE && item->data.toggle.value
           ? *item->data.toggle.value
           : false;
}

static void set_item_toggle_value(const settings_item_desc_t *item, bool value) {
    if (item && item->kind == SETTINGS_KIND_TOGGLE && item->data.toggle.value) {
        *item->data.toggle.value = value;
    }
}

static int item_choice_value(const settings_item_desc_t *item) {
    return item && item->kind == SETTINGS_KIND_CHOICE && item->data.choice.value
           ? *item->data.choice.value
           : 0;
}

static void set_item_choice_value(const settings_item_desc_t *item, int value) {
    if (item && item->kind == SETTINGS_KIND_CHOICE && item->data.choice.value) {
        *item->data.choice.value = value;
    }
}

static int item_range_value(const settings_item_desc_t *item) {
    return item && item->kind == SETTINGS_KIND_RANGE && item->data.range.value
           ? *item->data.range.value
           : 0;
}

static void set_item_range_value(const settings_item_desc_t *item, int value) {
    if (item && item->kind == SETTINGS_KIND_RANGE && item->data.range.value) {
        *item->data.range.value = value;
    }
}

static void format_item_value(const settings_item_desc_t *item, char *buf, size_t buf_size) {
    settings_data_format_item_value(item, buf, buf_size);
}

static void format_root_value(app_settings_section_t section, char *buf, size_t buf_size) {
    settings_data_format_section_summary(section, buf, buf_size);
}

static const char *section_rail_title(app_settings_section_t section) {
    switch (section) {
        case APP_SETTINGS_SECTION_INPUT:   return "INPUT";
        case APP_SETTINGS_SECTION_ASRC:    return "ASRC";
        case APP_SETTINGS_SECTION_OUTPUT:  return "OUT";
        case APP_SETTINGS_SECTION_POWER:   return "POWER";
        case APP_SETTINGS_SECTION_PROTECT: return "SAFE";
        case APP_SETTINGS_SECTION_SYSTEM:  return "SYS";
        default:                           return "--";
    }
}

static void init_rail_title(settings_rail_title_t *title, ui_widget_t *parent, int y) {
    int i;

    ui_rect_init(&title->bg, 0, y, SETTINGS_LEFT_W, SETTINGS_RAIL_SLOT_H, UI_GRAY_BLACK);
    title->bg.corner_r = 0;
    title->bg.gradient = false;
    ui_widget_add_child(parent, &title->bg.base);

    for (i = 0; i < SETTINGS_RAIL_CHAR_MAX; ++i) {
        title->text[i][0] = '\0';
        title->text[i][1] = '\0';
        ui_label_init(&title->chars[i], 0, y, SETTINGS_LEFT_W, 5,
                      "", &g_font_mini,
                      UI_GRAY_WHITE, UI_GRAY_BLACK, true);
        title->chars[i].align = UI_ALIGN_CENTER;
        ui_widget_add_child(parent, &title->chars[i].base);
        ui_widget_set_visible(&title->chars[i].base, false);
    }
}

static void set_rail_title(settings_rail_title_t *title,
                           const char *text,
                           ui_gray_t bg_gray,
                           ui_gray_t fg_gray) {
    int count = 0;
    int i;
    int start_y;

    title->bg.fill_gray = bg_gray;

    while (text && text[count] && count < SETTINGS_RAIL_CHAR_MAX) {
        ++count;
    }

    start_y = title->bg.base.y + (SETTINGS_RAIL_SLOT_H - (count * SETTINGS_RAIL_CHAR_STEP - 1)) / 2;

    for (i = 0; i < SETTINGS_RAIL_CHAR_MAX; ++i) {
        bool visible = (i < count);
        ui_widget_set_visible(&title->chars[i].base, visible);
        if (!visible) {
            continue;
        }
        title->text[i][0] = text[i];
        title->text[i][1] = '\0';
        ui_label_set_text(&title->chars[i], title->text[i]);
        title->chars[i].fg_gray = fg_gray;
        title->chars[i].base.y = (int16_t)(start_y + i * SETTINGS_RAIL_CHAR_STEP);
    }
}

static float list_target_offset(int selected, int item_count) {
    int top_max = item_count - SETTINGS_VISIBLE_ROWS;
    int top = clamp_int(selected - 1, 0, top_max);
    return -(float)(top * SETTINGS_ROW_PITCH);
}

static void apply_root_layout(void) {
    int i;
    int value_x = SETTINGS_ROOT_ROW_W - SETTINGS_ROOT_VALUE_W - 12;

    s_root_content.w = (uint16_t)SETTINGS_ROOT_LIST_W;

    for (i = 0; i < SETTINGS_ROOT_COUNT; ++i) {
        s_root_rows[i].bg.base.w = (uint16_t)SETTINGS_ROOT_ROW_W;
        s_root_rows[i].title.base.w = (uint16_t)(value_x - 10);
        s_root_rows[i].value.base.x = (int16_t)value_x;
        s_root_rows[i].value.base.w = (uint16_t)SETTINGS_ROOT_VALUE_W;
    }
}

static int section_list_width(void) {
    return s_editor_open ? SETTINGS_SECTION_LIST_W_COLLAPSED
                         : SETTINGS_SECTION_LIST_W_EXPANDED;
}

static int section_row_width(void) {
    return s_editor_open ? SETTINGS_SECTION_ROW_W_COLLAPSED
                         : SETTINGS_SECTION_ROW_W_EXPANDED;
}

static int section_value_width(void) {
    return s_editor_open ? SETTINGS_SECTION_VALUE_W_COLLAPSED
                         : SETTINGS_SECTION_VALUE_W_EXPANDED;
}

static int section_switch_track_x(void) {
    return section_row_width() - (SETTINGS_SWITCH_TRACK_W + 10);
}

static int section_switch_track_w(void) {
    return SETTINGS_SWITCH_TRACK_W;
}

static int section_switch_thumb_off_x(void) {
    return section_switch_track_x() + 2;
}

static int section_switch_thumb_on_x(void) {
    return section_switch_track_x() + section_switch_track_w() - SETTINGS_SWITCH_THUMB_W - 2;
}

static void apply_section_layout(void) {
    int i;
    int row_w = section_row_width();
    int value_w = section_value_width();
    int value_x = row_w - value_w - 12;
    int track_x = section_switch_track_x();

    s_section_content.w = (uint16_t)section_list_width();

    for (i = 0; i < SETTINGS_MAX_ITEMS; ++i) {
        s_section_rows[i].bg.base.w = (uint16_t)row_w;
        s_section_rows[i].title.base.w = (uint16_t)(value_x - 10);
        s_section_rows[i].value.base.x = (int16_t)value_x;
        s_section_rows[i].value.base.w = (uint16_t)value_w;
        s_section_rows[i].switch_track.base.x = (int16_t)track_x;
        s_section_rows[i].switch_track.base.w = (uint16_t)section_switch_track_w();
    }
}

static void animate_section_editor_open_layout(void) {
    int i;
    int old_value_x[SETTINGS_MAX_ITEMS];
    int old_track_x[SETTINGS_MAX_ITEMS];
    int old_thumb_x[SETTINGS_MAX_ITEMS];

    for (i = 0; i < SETTINGS_MAX_ITEMS; ++i) {
        old_value_x[i] = s_section_rows[i].value.base.x;
        old_track_x[i] = s_section_rows[i].switch_track.base.x;
        old_thumb_x[i] = s_section_rows[i].switch_thumb.base.x;
    }

    s_editor_open = true;
    s_editor_visible = true;
    s_editor_opening = true;
    apply_section_layout();
    sync_section_rows(false);
    sync_editor_panel();
    s_editor_opening = false;

    for (i = 0; i < SETTINGS_MAX_ITEMS; ++i) {
        float value_from = (float)(old_value_x[i] - s_section_rows[i].value.base.x);
        float track_from = (float)(old_track_x[i] - s_section_rows[i].switch_track.base.x);
        float thumb_from = (float)(old_thumb_x[i] - s_section_rows[i].switch_thumb.base.x);

        s_section_rows[i].value.base.offset_x = value_from;
        s_section_rows[i].switch_track.base.offset_x = track_from;
        s_section_rows[i].switch_thumb.base.offset_x = thumb_from;

        ui_widget_animate(&s_section_rows[i].value.base.offset_x, value_from, 0.f, 180,
                          ui_ease_out_cubic, NULL, NULL);
        ui_widget_animate(&s_section_rows[i].switch_track.base.offset_x, track_from, 0.f, 180,
                          ui_ease_out_cubic, NULL, NULL);
        ui_widget_animate(&s_section_rows[i].switch_thumb.base.offset_x, thumb_from, 0.f, 180,
                          ui_ease_out_cubic, NULL, NULL);
    }

    s_editor_widget.offset_x = (float)SETTINGS_EDITOR_W;
    ui_widget_animate(&s_editor_widget.offset_x, (float)SETTINGS_EDITOR_W, 0.f, 180,
                      ui_ease_out_cubic, NULL, NULL);
}

static void on_editor_close_done(void *ctx) {
    (void)ctx;
    s_editor_visible = false;
    s_editor_widget.offset_x = 0.f;
    sync_editor_panel();
}

static void animate_section_editor_close_layout(void) {
    int i;
    int old_value_x[SETTINGS_MAX_ITEMS];
    int old_track_x[SETTINGS_MAX_ITEMS];
    int old_thumb_x[SETTINGS_MAX_ITEMS];

    for (i = 0; i < SETTINGS_MAX_ITEMS; ++i) {
        old_value_x[i] = s_section_rows[i].value.base.x;
        old_track_x[i] = s_section_rows[i].switch_track.base.x;
        old_thumb_x[i] = s_section_rows[i].switch_thumb.base.x;
    }

    s_editor_open = false;
    apply_section_layout();
    sync_section_rows(false);

    for (i = 0; i < SETTINGS_MAX_ITEMS; ++i) {
        float value_from = (float)(old_value_x[i] - s_section_rows[i].value.base.x);
        float track_from = (float)(old_track_x[i] - s_section_rows[i].switch_track.base.x);
        float thumb_from = (float)(old_thumb_x[i] - s_section_rows[i].switch_thumb.base.x);

        s_section_rows[i].value.base.offset_x = value_from;
        s_section_rows[i].switch_track.base.offset_x = track_from;
        s_section_rows[i].switch_thumb.base.offset_x = thumb_from;

        ui_widget_animate(&s_section_rows[i].value.base.offset_x, value_from, 0.f, 180,
                          ui_ease_out_cubic, NULL, NULL);
        ui_widget_animate(&s_section_rows[i].switch_track.base.offset_x, track_from, 0.f, 180,
                          ui_ease_out_cubic, NULL, NULL);
        ui_widget_animate(&s_section_rows[i].switch_thumb.base.offset_x, thumb_from, 0.f, 180,
                          ui_ease_out_cubic, NULL, NULL);
    }

    ui_widget_animate(&s_editor_widget.offset_x, 0.f, (float)SETTINGS_EDITOR_W, 180,
                      ui_ease_out_cubic, on_editor_close_done, NULL);
}

static void set_row_style(settings_row_widgets_t *row, bool selected, bool faded, bool toggle_on) {
    if (selected) {
        row->bg.fill_gray = UI_GRAY_WHITE;
        row->title.fg_gray = UI_GRAY_BLACK;
        row->value.fg_gray = UI_GRAY_BLACK;
    } else if (faded) {
        row->bg.fill_gray = UI_GRAY_DIM;
        row->title.fg_gray = UI_GRAY_MUTED;
        row->value.fg_gray = UI_GRAY_MUTED;
    } else {
        row->bg.fill_gray = UI_GRAY_SHADOW;
        row->title.fg_gray = UI_GRAY_WHITE;
        row->value.fg_gray = UI_GRAY_LIGHT;
    }

    row->switch_track.fill_gray = toggle_on ? (selected ? UI_GRAY_BLACK : UI_GRAY_LIGHT)
                                            : (selected ? UI_GRAY_SHADOW : UI_GRAY_DIM);
    row->switch_track.border_w = 1;
    row->switch_track.border_gray = selected ? UI_GRAY_BLACK : UI_GRAY_LIGHT;
    row->switch_thumb.fill_gray = selected ? UI_GRAY_WHITE : UI_GRAY_BRIGHT;
    row->switch_thumb.border_w = 1;
    row->switch_thumb.border_gray = selected ? UI_GRAY_BLACK : UI_GRAY_WHITE;
    row->switch_thumb.base.x = toggle_on ? section_switch_thumb_on_x()
                                         : section_switch_thumb_off_x();
}

static void animate_row_focus(settings_row_widgets_t *row) {
    row->bg.base.offset_x = 3.f;
    row->title.base.offset_x = 3.f;
    row->value.base.offset_x = 3.f;
    row->switch_track.base.offset_x = 3.f;
    row->switch_thumb.base.offset_x = 3.f;

    ui_widget_animate(&row->bg.base.offset_x, 3.f, 0.f, 120,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&row->title.base.offset_x, 3.f, 0.f, 120,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&row->value.base.offset_x, 3.f, 0.f, 120,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&row->switch_track.base.offset_x, 3.f, 0.f, 120,
                      ui_ease_out_cubic, NULL, NULL);
    ui_widget_animate(&row->switch_thumb.base.offset_x, 3.f, 0.f, 120,
                      ui_ease_out_cubic, NULL, NULL);
}

static void animate_inline_toggle(settings_row_widgets_t *row, bool now_on) {
    float from = now_on ? -4.f : 4.f;

    row->switch_thumb.base.offset_x = from;
    ui_widget_animate(&row->switch_thumb.base.offset_x, from, 0.f, 140,
                      ui_ease_out_cubic, NULL, NULL);
}

static void on_section_overlay_enter_done(void *ctx) {
    (void)ctx;
    s_section_overlay_animating = false;
}

static void on_section_overlay_close_done(void *ctx) {
    (void)ctx;
    s_section_overlay_animating = false;
    app_ui_pop(UI_TRANS_NONE, 0);
}

static void animate_section_overlay_enter(void) {
    if (!s_section_enter_from_root) {
        s_section_overlay.offset_x = 0.f;
        s_section_overlay_animating = false;
        return;
    }

    s_section_overlay_animating = true;
    s_section_overlay.offset_x = (float)SETTINGS_SECTION_OVERLAY_OFFSET_X;
    ui_widget_animate(&s_section_overlay.offset_x,
                      (float)SETTINGS_SECTION_OVERLAY_OFFSET_X, 0.f, 180,
                      ui_ease_out_cubic, on_section_overlay_enter_done, NULL);
}

static void animate_section_overlay_close(void) {
    s_section_overlay_animating = true;
    close_editor_immediate();
    ui_widget_animate(&s_section_overlay.offset_x,
                      s_section_overlay.offset_x, (float)SETTINGS_SECTION_OVERLAY_OFFSET_X, 180,
                      ui_ease_out_cubic, on_section_overlay_close_done, NULL);
}

static void sync_root_page(bool animate_scroll) {
    int i;
    float target = list_target_offset(s_root_selected, SETTINGS_ROOT_COUNT);

    apply_root_layout();
    set_rail_title(&s_root_parent_title, "SET", UI_GRAY_WHITE, UI_GRAY_BLACK);
    set_rail_title(&s_root_child_title, "", UI_GRAY_BLACK, UI_GRAY_BLACK);

    for (i = 0; i < SETTINGS_ROOT_COUNT; ++i) {
        const settings_section_desc_t *section_desc = section_def((app_settings_section_t)i);
        format_root_value((app_settings_section_t)i, s_root_value_text[i], sizeof(s_root_value_text[i]));
        ui_label_set_text(&s_root_rows[i].title, section_desc ? section_desc->title : "--");
        ui_label_set_text(&s_root_rows[i].value, s_root_value_text[i]);
        set_row_style(&s_root_rows[i], i == s_root_selected, false, false);
        ui_widget_set_visible(&s_root_rows[i].switch_track.base, false);
        ui_widget_set_visible(&s_root_rows[i].switch_thumb.base, false);
        ui_widget_set_visible(&s_root_rows[i].value.base, true);
    }

    animate_row_focus(&s_root_rows[s_root_selected]);

    if (animate_scroll) {
        ui_widget_animate(&s_root_content.offset_y, s_root_content.offset_y, target, 180,
                          ui_ease_out_cubic, NULL, NULL);
    } else {
        s_root_content.offset_y = target;
    }
}

static void sync_section_rows(bool animate_scroll) {
    const settings_section_desc_t *section_desc = section_def(s_active_section);
    int i;
    float target = list_target_offset(s_section_selected, section_desc->item_count);

    set_rail_title(&s_section_parent_title, "SET", UI_GRAY_DIM, UI_GRAY_MUTED);
    set_rail_title(&s_section_active_title, section_rail_title(s_active_section),
                   UI_GRAY_WHITE, UI_GRAY_BLACK);
    apply_section_layout();

    for (i = 0; i < SETTINGS_MAX_ITEMS; ++i) {
        bool row_visible = (i < section_desc->item_count);
        bool is_selected = (i == s_section_selected);
        bool faded = s_editor_open;

        ui_widget_set_visible(&s_section_rows[i].bg.base, row_visible);
        ui_widget_set_visible(&s_section_rows[i].title.base, row_visible);
        ui_widget_set_visible(&s_section_rows[i].value.base, row_visible);

        if (!row_visible) {
            ui_widget_set_visible(&s_section_rows[i].switch_track.base, false);
            ui_widget_set_visible(&s_section_rows[i].switch_thumb.base, false);
            continue;
        }

        {
            const settings_item_desc_t *item = &section_desc->items[i];

            ui_label_set_text(&s_section_rows[i].title, item->title);
            format_item_value(item, s_row_value_text[i], sizeof(s_row_value_text[i]));
            ui_label_set_text(&s_section_rows[i].value, s_row_value_text[i]);

            if (item->kind == SETTINGS_KIND_TOGGLE) {
                ui_widget_set_visible(&s_section_rows[i].switch_track.base, true);
                ui_widget_set_visible(&s_section_rows[i].switch_thumb.base, true);
                ui_widget_set_visible(&s_section_rows[i].value.base, false);
            } else {
                ui_widget_set_visible(&s_section_rows[i].switch_track.base, false);
                ui_widget_set_visible(&s_section_rows[i].switch_thumb.base, false);
                ui_widget_set_visible(&s_section_rows[i].value.base, true);
            }

            set_row_style(&s_section_rows[i], is_selected, faded, item_toggle_value(item));
        }
    }

    animate_row_focus(&s_section_rows[s_section_selected]);

    if (animate_scroll) {
        ui_widget_animate(&s_section_content.offset_y, s_section_content.offset_y, target, 180,
                          ui_ease_out_cubic, NULL, NULL);
    } else {
        s_section_content.offset_y = target;
    }
}

static void sync_editor_panel(void) {
    int i;
    const settings_item_desc_t *item = current_item_def();
    int choice_index;
    int fill_w;
    int value;

    ui_widget_set_visible(&s_editor_widget, s_editor_visible);
    ui_widget_set_visible(&s_editor_panel.base, s_editor_visible);
    ui_widget_set_visible(&s_editor_head.base, s_editor_visible);
    ui_widget_set_visible(&s_editor_mode.base, s_editor_visible);

    for (i = 0; i < SETTINGS_EDITOR_CHOICES; ++i) {
        ui_widget_set_visible(&s_editor_choice_bg[i].base, false);
        ui_widget_set_visible(&s_editor_choice_label[i].base, false);
    }
    ui_widget_set_visible(&s_editor_switch_track.base, false);
    ui_widget_set_visible(&s_editor_switch_thumb.base, false);
    ui_widget_set_visible(&s_editor_state_label.base, false);
    ui_widget_set_visible(&s_editor_bar_track.base, false);
    ui_widget_set_visible(&s_editor_bar_fill.base, false);
    ui_widget_set_visible(&s_editor_value_label.base, false);

    if (!s_editor_visible) {
        return;
    }

    switch (item->kind) {
        case SETTINGS_KIND_CHOICE:
            ui_label_set_text(&s_editor_mode, "SEL");
            choice_index = item_choice_value(item);
            for (i = 0; i < item->data.choice.option_count && i < SETTINGS_EDITOR_CHOICES; ++i) {
                ui_widget_set_visible(&s_editor_choice_bg[i].base, true);
                ui_widget_set_visible(&s_editor_choice_label[i].base, true);
                s_editor_choice_bg[i].fill_gray = (i == choice_index) ? UI_GRAY_WHITE : UI_GRAY_SHADOW;
                s_editor_choice_label[i].fg_gray = (i == choice_index) ? UI_GRAY_BLACK : UI_GRAY_WHITE;
                if (!s_editor_opening) {
                    s_editor_choice_bg[i].base.offset_x = (i == choice_index) ? 4.f : 0.f;
                    s_editor_choice_label[i].base.offset_x = (i == choice_index) ? 4.f : 0.f;
                }
                ui_label_set_text(&s_editor_choice_label[i], item->data.choice.options[i]);
                if (i == choice_index && !s_editor_opening) {
                    ui_widget_animate(&s_editor_choice_bg[i].base.offset_x, 4.f, 0.f, 120,
                                      ui_ease_out_cubic, NULL, NULL);
                    ui_widget_animate(&s_editor_choice_label[i].base.offset_x, 4.f, 0.f, 120,
                                      ui_ease_out_cubic, NULL, NULL);
                }
            }
            break;
        case SETTINGS_KIND_RANGE:
            ui_label_set_text(&s_editor_mode, "RNG");
            ui_widget_set_visible(&s_editor_bar_track.base, true);
            ui_widget_set_visible(&s_editor_bar_fill.base, true);
            ui_widget_set_visible(&s_editor_value_label.base, true);
            value = item_range_value(item);
            fill_w = (value - item->data.range.min_value) * 56
                   / (item->data.range.max_value - item->data.range.min_value);
            fill_w = clamp_int(fill_w, 1, 56);
            s_editor_bar_fill.base.w = (uint16_t)fill_w;
            format_item_value(item, s_editor_value_text, sizeof(s_editor_value_text));
            ui_label_set_text(&s_editor_value_label, s_editor_value_text);
            break;
        case SETTINGS_KIND_ACTION:
            ui_label_set_text(&s_editor_mode, "ACT");
            ui_widget_set_visible(&s_editor_value_label.base, true);
            ui_label_set_text(&s_editor_value_label, "READY");
            break;
        case SETTINGS_KIND_READONLY:
        default:
            ui_label_set_text(&s_editor_mode, "INFO");
            ui_widget_set_visible(&s_editor_value_label.base, true);
            format_item_value(item, s_editor_value_text, sizeof(s_editor_value_text));
            ui_label_set_text(&s_editor_value_label, s_editor_value_text);
            break;
    }
}

static void close_editor_immediate(void) {
    s_editor_open = false;
    s_editor_visible = false;
    s_editor_widget.offset_x = 0.f;
    apply_section_layout();
    sync_section_rows(false);
    sync_editor_panel();
}

static void open_editor(void) {
    animate_section_editor_open_layout();
}

static void close_editor(void) {
    animate_section_editor_close_layout();
}

static void on_popup_result(bool confirmed, void *ctx) {
    const settings_item_desc_t *item = (const settings_item_desc_t *)ctx;

    if (confirmed && item && item->kind == SETTINGS_KIND_ACTION && item->data.action.apply) {
        item->data.action.apply();
    }

    sync_root_page(false);
    sync_section_rows(false);
    sync_editor_panel();
}

static void on_root_appear(ui_page_t *p) {
    (void)p;
    s_root_content.offset_y = list_target_offset(s_root_selected, SETTINGS_ROOT_COUNT);
    sync_root_page(false);
}

static void on_section_appear(ui_page_t *p) {
    (void)p;
    close_editor_immediate();
    s_section_content.offset_y = list_target_offset(s_section_selected, current_item_count());
    apply_section_layout();
    sync_section_rows(false);
    animate_section_overlay_enter();
}

static void on_root_action(ui_action_t action, const ui_event_t *raw, void *ctx) {
    (void)raw;
    (void)ctx;

    switch (action) {
        case UI_ACTION_SCROLL_UP:
            s_root_selected = (s_root_selected + SETTINGS_ROOT_COUNT - 1) % SETTINGS_ROOT_COUNT;
            sync_root_page(true);
            break;
        case UI_ACTION_SCROLL_DOWN:
            s_root_selected = (s_root_selected + 1) % SETTINGS_ROOT_COUNT;
            sync_root_page(true);
            break;
        case UI_ACTION_CONFIRM:
            s_active_section = (app_settings_section_t)s_root_selected;
            s_section_selected = 0;
            close_editor_immediate();
            s_section_enter_from_root = true;
            app_ui_push_id(APP_PAGE_SETTINGS_SECTION, UI_TRANS_NONE, 0);
            break;
        case UI_ACTION_CANCEL:
            app_ui_pop(UI_TRANS_SLIDE_RIGHT, UI_TRANS_DURATION_MS);
            break;
        default:
            break;
    }
}

static void adjust_current_item(int direction) {
    const settings_item_desc_t *item = current_item_def();
    int choice;
    int value;

    switch (item->kind) {
        case SETTINGS_KIND_TOGGLE:
            set_item_toggle_value(item, direction > 0);
            break;
        case SETTINGS_KIND_CHOICE:
            choice = item_choice_value(item);
            choice += direction;
            if (choice < 0) choice = item->data.choice.option_count - 1;
            if (choice >= item->data.choice.option_count) choice = 0;
            set_item_choice_value(item, choice);
            break;
        case SETTINGS_KIND_RANGE:
            value = item_range_value(item);
            value += direction * item->data.range.step;
            value = clamp_int(value, item->data.range.min_value, item->data.range.max_value);
            set_item_range_value(item, value);
            break;
        default:
            break;
    }

    sync_root_page(false);
    sync_section_rows(false);
    sync_editor_panel();
}

static void on_section_action(ui_action_t action, const ui_event_t *raw, void *ctx) {
    const settings_item_desc_t *item;
    (void)raw;
    (void)ctx;

    if (s_section_overlay_animating) {
        return;
    }

    item = current_item_def();

    if (s_editor_open) {
        switch (action) {
            case UI_ACTION_SCROLL_UP:
                adjust_current_item(1);
                break;
            case UI_ACTION_SCROLL_DOWN:
                adjust_current_item(-1);
                break;
            case UI_ACTION_CONFIRM:
                if (item->kind == SETTINGS_KIND_TOGGLE) {
                    set_item_toggle_value(item, !item_toggle_value(item));
                    sync_root_page(false);
                    sync_section_rows(false);
                    sync_editor_panel();
                } else if (item->kind == SETTINGS_KIND_ACTION) {
                    app_ui_popup_show(item->data.action.title, item->data.action.message,
                                      on_popup_result, (void *)item);
                } else {
                    close_editor();
                }
                break;
            case UI_ACTION_CANCEL:
                close_editor();
                break;
            default:
                break;
        }
        return;
    }

    switch (action) {
        case UI_ACTION_SCROLL_UP:
            s_section_selected = (s_section_selected + current_item_count() - 1) % current_item_count();
            sync_section_rows(true);
            break;
        case UI_ACTION_SCROLL_DOWN:
            s_section_selected = (s_section_selected + 1) % current_item_count();
            sync_section_rows(true);
            break;
        case UI_ACTION_CONFIRM:
            if (item->kind == SETTINGS_KIND_TOGGLE) {
                bool now_on = !item_toggle_value(item);
                set_item_toggle_value(item, now_on);
                sync_root_page(false);
                sync_section_rows(false);
                animate_inline_toggle(&s_section_rows[s_section_selected], now_on);
            } else if (item->kind == SETTINGS_KIND_ACTION) {
                app_ui_popup_show(item->data.action.title, item->data.action.message,
                                  on_popup_result, (void *)item);
            } else {
                open_editor();
            }
            break;
        case UI_ACTION_CANCEL:
            if (s_section_enter_from_root) {
                animate_section_overlay_close();
            } else {
                app_ui_pop(UI_TRANS_SLIDE_RIGHT, UI_TRANS_DURATION_MS);
            }
            break;
        default:
            break;
    }
}

static void init_row_widgets(settings_row_widgets_t *row, ui_widget_t *parent, int y) {
    ui_rect_init(&row->bg, 2, y, SETTINGS_ROW_W, 10, UI_GRAY_SHADOW);
    row->bg.corner_r = 0;
    row->bg.gradient = false;
    ui_widget_add_child(parent, &row->bg.base);

    ui_label_init(&row->title, 8, y + 1, 88, 8,
                  "", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(parent, &row->title.base);

    ui_label_init(&row->value, 92, y + 1, SETTINGS_SECTION_VALUE_W_COLLAPSED, 8,
                  "", &g_font,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    row->value.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(parent, &row->value.base);

    ui_rect_init(&row->switch_track, SETTINGS_SWITCH_TRACK_X, y + 2,
                 SETTINGS_SWITCH_TRACK_W, 6, UI_GRAY_DIM);
    row->switch_track.corner_r = 0;
    row->switch_track.gradient = false;
    row->switch_track.border_w = 1;
    row->switch_track.border_gray = UI_GRAY_LIGHT;
    ui_widget_add_child(parent, &row->switch_track.base);

    ui_rect_init(&row->switch_thumb, SETTINGS_SWITCH_THUMB_OFF_X, y + 1,
                 SETTINGS_SWITCH_THUMB_W, 8, UI_GRAY_WHITE);
    row->switch_thumb.corner_r = 0;
    row->switch_thumb.gradient = false;
    row->switch_thumb.border_w = 1;
    row->switch_thumb.border_gray = UI_GRAY_WHITE;
    ui_widget_add_child(parent, &row->switch_thumb.base);
}

static void build_root_page(void) {
    int i;

    ui_widget_init(&s_root_widget, 0, 0, 256, 64, NULL);

    ui_rect_init(&s_root_bg, 0, 0, 256, 64, UI_GRAY_BLACK);
    ui_widget_add_child(&s_root_widget, &s_root_bg.base);
    init_rail_title(&s_root_parent_title, &s_root_widget, 0);
    init_rail_title(&s_root_child_title, &s_root_widget, SETTINGS_RAIL_SLOT_H);

    ui_widget_init(&s_root_content, SETTINGS_LIST_X, SETTINGS_LIST_Y, SETTINGS_ROOT_LIST_W, 48, NULL);
    ui_widget_add_child(&s_root_widget, &s_root_content);

    for (i = 0; i < SETTINGS_ROOT_COUNT; ++i) {
        init_row_widgets(&s_root_rows[i], &s_root_content, i * SETTINGS_ROW_PITCH);
    }

    apply_root_layout();

    app_page_init(&page_settings, &s_root_binding, &s_root_widget,
                  s_root_keymap, on_root_action, on_root_appear, NULL, NULL);
    sync_root_page(false);
}

static void build_section_page(void) {
    int i;

    ui_widget_init(&s_section_widget, 0, 0, 256, 64, NULL);

    ui_rect_init(&s_section_bg, 0, 0, 256, 64, UI_GRAY_BLACK);
    ui_widget_add_child(&s_section_widget, &s_section_bg.base);
    init_rail_title(&s_section_parent_title, &s_section_widget, 0);

    ui_widget_init(&s_section_overlay, 0, 0, 256, 64, NULL);
    ui_widget_add_child(&s_section_widget, &s_section_overlay);
    init_rail_title(&s_section_active_title, &s_section_overlay, SETTINGS_RAIL_SLOT_H);

    ui_widget_init(&s_section_content, SETTINGS_LIST_X, SETTINGS_LIST_Y, SETTINGS_LIST_W, 48, NULL);
    ui_widget_add_child(&s_section_overlay, &s_section_content);

    for (i = 0; i < SETTINGS_MAX_ITEMS; ++i) {
        init_row_widgets(&s_section_rows[i], &s_section_content, i * SETTINGS_ROW_PITCH);
    }

    ui_widget_init(&s_editor_widget, SETTINGS_EDITOR_X, 0, SETTINGS_EDITOR_W, 64, NULL);
    ui_widget_add_child(&s_section_overlay, &s_editor_widget);

    ui_rect_init(&s_editor_panel, 0, 0, SETTINGS_EDITOR_W, 64, UI_GRAY_BLACK);
    s_editor_panel.border_w = 1;
    s_editor_panel.border_gray = UI_GRAY_WHITE;
    ui_widget_add_child(&s_editor_widget, &s_editor_panel.base);

    ui_rect_init(&s_editor_head, 0, 0, SETTINGS_EDITOR_W, 10, UI_GRAY_WHITE);
    ui_widget_add_child(&s_editor_widget, &s_editor_head.base);

    ui_label_init(&s_editor_title, 4, 1, 40, 8, "", &g_font,
                  UI_GRAY_BLACK, UI_GRAY_WHITE, true);
    ui_widget_add_child(&s_editor_widget, &s_editor_title.base);
    ui_widget_set_visible(&s_editor_title.base, false);

    ui_label_init(&s_editor_mode, 4, 1, SETTINGS_EDITOR_W - 8, 8, "", &g_font,
                  UI_GRAY_BLACK, UI_GRAY_WHITE, true);
    s_editor_mode.align = UI_ALIGN_CENTER;
    ui_widget_add_child(&s_editor_widget, &s_editor_mode.base);

    for (i = 0; i < SETTINGS_EDITOR_CHOICES; ++i) {
        ui_rect_init(&s_editor_choice_bg[i], 6, 14 + i * 12, 56, 10, UI_GRAY_SHADOW);
        ui_widget_add_child(&s_editor_widget, &s_editor_choice_bg[i].base);

        ui_label_init(&s_editor_choice_label[i], 8, 15 + i * 12, 52, 8, "", &g_font,
                      UI_GRAY_WHITE, UI_GRAY_BLACK, true);
        s_editor_choice_label[i].align = UI_ALIGN_CENTER;
        ui_widget_add_child(&s_editor_widget, &s_editor_choice_label[i].base);
    }

    ui_rect_init(&s_editor_switch_track, 16, 20, 36, 10, UI_GRAY_DIM);
    ui_widget_add_child(&s_editor_widget, &s_editor_switch_track.base);

    ui_rect_init(&s_editor_switch_thumb, 18, 21, 14, 8, UI_GRAY_WHITE);
    ui_widget_add_child(&s_editor_widget, &s_editor_switch_thumb.base);

    ui_label_init(&s_editor_state_label, 8, 38, 52, 8, "", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    s_editor_state_label.align = UI_ALIGN_CENTER;
    ui_widget_add_child(&s_editor_widget, &s_editor_state_label.base);

    ui_rect_init(&s_editor_bar_track, 6, 20, 56, 10, UI_GRAY_DIM);
    ui_widget_add_child(&s_editor_widget, &s_editor_bar_track.base);

    ui_rect_init(&s_editor_bar_fill, 6, 20, 24, 10, UI_GRAY_WHITE);
    ui_widget_add_child(&s_editor_widget, &s_editor_bar_fill.base);

    ui_label_init(&s_editor_value_label, 4, 36, 60, 8, "", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    s_editor_value_label.align = UI_ALIGN_CENTER;
    ui_widget_add_child(&s_editor_widget, &s_editor_value_label.base);

    app_page_init(&page_settings_section, &s_section_binding, &s_section_widget,
                  s_section_keymap, on_section_action, on_section_appear, NULL, NULL);

    close_editor_immediate();
    sync_section_rows(false);
}

static void build_all_pages(void) {
    if (s_pages_built) {
        return;
    }
    s_pages_built = true;
    build_root_page();
    build_section_page();
}

void page_settings_build(void) {
    build_all_pages();
}

void page_settings_section_build(void) {
    build_all_pages();
}

static void open_section_common(app_settings_section_t section,
                                ui_trans_type_t trans,
                                uint32_t duration_ms,
                                bool replace_page) {
    build_all_pages();
    s_active_section = section;
    s_root_selected = (int)section;
    s_section_selected = 0;
    s_section_enter_from_root = false;
    close_editor_immediate();
    if (replace_page) {
        app_ui_replace_id(APP_PAGE_SETTINGS_SECTION, trans, duration_ms);
    } else {
        app_ui_push_id(APP_PAGE_SETTINGS_SECTION, trans, duration_ms);
    }
}

void page_settings_open_section(app_settings_section_t section,
                                ui_trans_type_t trans,
                                uint32_t duration_ms) {
    open_section_common(section, trans, duration_ms, false);
}

void page_settings_replace_section(app_settings_section_t section,
                                   ui_trans_type_t trans,
                                   uint32_t duration_ms) {
    open_section_common(section, trans, duration_ms, true);
}

const char *page_settings_home_input_text(void) {
    return settings_data_home_input_text();
}

const char *page_settings_home_asrc_in_text(void) {
    return settings_data_home_asrc_in_text();
}

const char *page_settings_home_asrc_out_text(void) {
    return settings_data_home_asrc_out_text();
}

const char *page_settings_home_output_text(void) {
    return settings_data_home_output_text();
}

const char *page_settings_home_power_text(void) {
    return settings_data_home_power_text();
}

const char *page_settings_home_temp_text(void) {
    return settings_data_home_temp_text();
}

const char *page_settings_home_time_text(void) {
    return settings_data_home_time_text();
}
