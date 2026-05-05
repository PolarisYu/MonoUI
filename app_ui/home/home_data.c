#include "home_data.h"

#include "device/device_status.h"

typedef struct {
    const char            *title;
    const char            *line2;
    app_settings_section_t target_section;
} home_tile_meta_t;

static const home_tile_meta_t s_tile_meta[HOME_FOCUS_COUNT] = {
    { "INPUT",  "SRC",       APP_SETTINGS_SECTION_INPUT  },
    { "ASRC",   NULL,         APP_SETTINGS_SECTION_ASRC   },
    { "OUT",    "AMP",       APP_SETTINGS_SECTION_OUTPUT },
    { "SYSTEM", NULL,         APP_SETTINGS_SECTION_SYSTEM },
};

void home_data_top_bar(home_top_bar_data_t *out) {
    if (!out) {
        return;
    }

    out->device_name = device_status_device_name();
    out->power_text = device_status_power_text();
    out->temp_text = device_status_temp_text();
    out->time_text = device_status_time_text();
}

void home_data_tile(home_focus_t focus, home_tile_data_t *out) {
    if (!out || focus < 0 || focus >= HOME_FOCUS_COUNT) {
        return;
    }

    out->title = s_tile_meta[focus].title;
    out->line2 = s_tile_meta[focus].line2;
    out->target_section = s_tile_meta[focus].target_section;

    switch (focus) {
        case HOME_FOCUS_INPUT:
            out->line1 = settings_data_home_input_text();
            break;
        case HOME_FOCUS_ASRC:
            out->line1 = settings_data_home_asrc_in_text();
            out->line2 = settings_data_home_asrc_out_text();
            break;
        case HOME_FOCUS_OUTPUT:
            out->line1 = settings_data_home_output_text();
            break;
        case HOME_FOCUS_SYSTEM:
        default:
            out->line1 = device_status_system_summary();
            break;
    }
}

app_settings_section_t home_data_target_section(home_focus_t focus) {
    if (focus < 0 || focus >= HOME_FOCUS_COUNT) {
        return APP_SETTINGS_SECTION_SYSTEM;
    }
    return s_tile_meta[focus].target_section;
}
