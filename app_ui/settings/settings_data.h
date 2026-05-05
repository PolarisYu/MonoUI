#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    APP_SETTINGS_SECTION_INPUT = 0,
    APP_SETTINGS_SECTION_ASRC,
    APP_SETTINGS_SECTION_OUTPUT,
    APP_SETTINGS_SECTION_POWER,
    APP_SETTINGS_SECTION_PROTECT,
    APP_SETTINGS_SECTION_SYSTEM,
    APP_SETTINGS_SECTION_COUNT
} app_settings_section_t;

typedef enum {
    SETTINGS_KIND_TOGGLE = 0,
    SETTINGS_KIND_CHOICE,
    SETTINGS_KIND_RANGE,
    SETTINGS_KIND_ACTION,
    SETTINGS_KIND_READONLY
} settings_item_kind_t;

typedef void (*settings_text_fn_t)(char *buf, size_t buf_size);
typedef void (*settings_action_apply_fn_t)(void);

typedef struct {
    const char           *title;
    settings_item_kind_t  kind;
    union {
        struct {
            bool *value;
        } toggle;
        struct {
            int                *value;
            const char *const  *options;
            int                 option_count;
        } choice;
        struct {
            int         *value;
            int          min_value;
            int          max_value;
            int          step;
            const char  *unit;
        } range;
        struct {
            const char                *title;
            const char                *message;
            settings_action_apply_fn_t apply;
        } action;
        struct {
            settings_text_fn_t format;
        } readonly;
    } data;
} settings_item_desc_t;

typedef struct {
    app_settings_section_t       section;
    const char                  *title;
    const settings_item_desc_t  *items;
    int                          item_count;
    settings_text_fn_t           format_summary;
} settings_section_desc_t;

const settings_section_desc_t *settings_data_sections(size_t *count);
const settings_section_desc_t *settings_data_section(app_settings_section_t section);

void settings_data_format_item_value(const settings_item_desc_t *item,
                                     char *buf,
                                     size_t buf_size);
void settings_data_format_section_summary(app_settings_section_t section,
                                          char *buf,
                                          size_t buf_size);

const char *settings_data_home_input_text(void);
const char *settings_data_home_asrc_in_text(void);
const char *settings_data_home_asrc_out_text(void);
const char *settings_data_home_output_text(void);
const char *settings_data_home_power_text(void);
const char *settings_data_home_temp_text(void);
const char *settings_data_home_time_text(void);
