#pragma once

#include "home_state.h"
#include "settings/settings_data.h"

typedef struct {
    const char *device_name;
    const char *power_text;
    const char *temp_text;
    const char *time_text;
} home_top_bar_data_t;

typedef struct {
    const char            *title;
    const char            *line1;
    const char            *line2;
    app_settings_section_t target_section;
} home_tile_data_t;

void home_data_top_bar(home_top_bar_data_t *out);
void home_data_tile(home_focus_t focus, home_tile_data_t *out);
app_settings_section_t home_data_target_section(home_focus_t focus);
