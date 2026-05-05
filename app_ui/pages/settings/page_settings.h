#pragma once
#include "app_pages.h"
#include "settings/settings_data.h"

extern ui_page_t page_settings;
extern ui_page_t page_settings_section;

void page_settings_build(void);
void page_settings_section_build(void);

void page_settings_open_section(app_settings_section_t section,
                                ui_trans_type_t trans,
                                uint32_t duration_ms);
void page_settings_replace_section(app_settings_section_t section,
                                   ui_trans_type_t trans,
                                   uint32_t duration_ms);

const char *page_settings_home_input_text(void);
const char *page_settings_home_asrc_in_text(void);
const char *page_settings_home_asrc_out_text(void);
const char *page_settings_home_output_text(void);
const char *page_settings_home_power_text(void);
const char *page_settings_home_temp_text(void);
const char *page_settings_home_time_text(void);
