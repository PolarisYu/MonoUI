#include "settings_data.h"

#include <stdio.h>
#include "device/device_status.h"
#include "settings_state.h"

#define ARRAY_SIZE(a) ((int)(sizeof(a) / sizeof((a)[0])))

static void format_readonly_input_lock(char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "%s", device_status_input_lock() ? "LOCK" : "WAIT");
}

static void format_readonly_asrc_lock(char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "%s", device_status_asrc_lock() ? "SYNC" : "UNLK");
}

static void format_readonly_system_time(char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "%s", device_status_time_text());
}

static void format_readonly_system_version(char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "%s", device_status_version_text());
}

static void format_summary_input(char *buf, size_t buf_size);
static void format_summary_asrc(char *buf, size_t buf_size);
static void format_summary_output(char *buf, size_t buf_size);
static void format_summary_power(char *buf, size_t buf_size);
static void format_summary_protect(char *buf, size_t buf_size);
static void format_summary_system(char *buf, size_t buf_size);

static const char *const s_input_source_options[] = { "USB", "I2S", "OPT" };
static const char *const s_input_priority_options[] = { "AUTO", "USB", "I2S" };
static const char *const s_rate_options[] = { "48K", "96K", "192K" };
static const char *const s_filter_options[] = { "FAST", "BAL", "NOS" };
static const char *const s_phase_options[] = { "MIN", "LIN", "HYB" };
static const char *const s_output_target_options[] = { "HP", "SPK", "AUTO" };
static const char *const s_output_mode_options[] = { "A", "D", "MIX" };
static const char *const s_power_source_options[] = { "AUTO", "DC", "PD" };
static const char *const s_sleep_options[] = { "OFF", "5M", "30M" };
static const char *const s_power_loss_options[] = { "HOLD", "MUTE", "OFF" };
static const char *const s_storage_slot_options[] = { "A", "B", "C" };

static const settings_item_desc_t s_input_items[] = {
    { "SOURCE",   SETTINGS_KIND_CHOICE,   .data.choice = { &g_settings_state.input.source,      s_input_source_options,   ARRAY_SIZE(s_input_source_options) } },
    { "AUTO DET", SETTINGS_KIND_TOGGLE,   .data.toggle = { &g_settings_state.input.auto_detect } },
    { "PRIORITY", SETTINGS_KIND_CHOICE,   .data.choice = { &g_settings_state.input.priority,    s_input_priority_options, ARRAY_SIZE(s_input_priority_options) } },
    { "LOCK",     SETTINGS_KIND_READONLY, .data.readonly = { format_readonly_input_lock } },
    { "GAIN",     SETTINGS_KIND_RANGE,    .data.range = { &g_settings_state.input.gain, 0, 24, 2, "dB" } },
    { "RESCAN",   SETTINGS_KIND_ACTION,   .data.action = { "INPUT", "RESCAN INPUT?", settings_state_apply_input_rescan } },
};

static const settings_item_desc_t s_asrc_items[] = {
    { "ENABLE",   SETTINGS_KIND_TOGGLE,   .data.toggle = { &g_settings_state.asrc.enable } },
    { "IN LOCK",  SETTINGS_KIND_READONLY, .data.readonly = { format_readonly_asrc_lock } },
    { "OUT RATE", SETTINGS_KIND_CHOICE,   .data.choice = { &g_settings_state.asrc.rate, s_rate_options, ARRAY_SIZE(s_rate_options) } },
    { "FILTER",   SETTINGS_KIND_CHOICE,   .data.choice = { &g_settings_state.asrc.filter, s_filter_options, ARRAY_SIZE(s_filter_options) } },
    { "JITTER",   SETTINGS_KIND_RANGE,    .data.range = { &g_settings_state.asrc.jitter, 0, 100, 5, "%" } },
    { "PHASE",    SETTINGS_KIND_CHOICE,   .data.choice = { &g_settings_state.asrc.phase, s_phase_options, ARRAY_SIZE(s_phase_options) } },
};

static const settings_item_desc_t s_output_items[] = {
    { "TARGET", SETTINGS_KIND_CHOICE, .data.choice = { &g_settings_state.output.target, s_output_target_options, ARRAY_SIZE(s_output_target_options) } },
    { "MODE",   SETTINGS_KIND_CHOICE, .data.choice = { &g_settings_state.output.mode, s_output_mode_options, ARRAY_SIZE(s_output_mode_options) } },
    { "VOLUME", SETTINGS_KIND_RANGE,  .data.range = { &g_settings_state.output.volume, 0, 100, 1, "%" } },
    { "GAIN",   SETTINGS_KIND_RANGE,  .data.range = { &g_settings_state.output.gain, 0, 24, 2, "dB" } },
    { "MUTE",   SETTINGS_KIND_TOGGLE, .data.toggle = { &g_settings_state.output.mute } },
    { "SAFE",   SETTINGS_KIND_ACTION, .data.action = { "OUTPUT", "ENABLE SAFE?", settings_state_apply_output_safe } },
};

static const settings_item_desc_t s_power_items[] = {
    { "SOURCE",    SETTINGS_KIND_CHOICE, .data.choice = { &g_settings_state.power.source, s_power_source_options, ARRAY_SIZE(s_power_source_options) } },
    { "PD VOLT",   SETTINGS_KIND_RANGE,  .data.range = { &g_settings_state.power.pd_voltage, 5, 20, 1, "V" } },
    { "SLEEP",     SETTINGS_KIND_CHOICE, .data.choice = { &g_settings_state.power.sleep, s_sleep_options, ARRAY_SIZE(s_sleep_options) } },
    { "RTC SYNC",  SETTINGS_KIND_TOGGLE, .data.toggle = { &g_settings_state.power.rtc_sync } },
    { "STBY SAVE", SETTINGS_KIND_TOGGLE, .data.toggle = { &g_settings_state.power.store } },
    { "OFF NOW",   SETTINGS_KIND_ACTION, .data.action = { "POWER", "SHUTDOWN NOW?", settings_state_apply_power_shutdown } },
};

static const settings_item_desc_t s_protect_items[] = {
    { "ENABLE",   SETTINGS_KIND_TOGGLE, .data.toggle = { &g_settings_state.protect.enable } },
    { "TEMP LIM", SETTINGS_KIND_RANGE,  .data.range = { &g_settings_state.protect.temp_limit, 40, 90, 5, "C" } },
    { "DC PROT",  SETTINGS_KIND_TOGGLE, .data.toggle = { &g_settings_state.protect.dc } },
    { "CLIP",     SETTINGS_KIND_TOGGLE, .data.toggle = { &g_settings_state.protect.clip } },
    { "LOSS",     SETTINGS_KIND_CHOICE, .data.choice = { &g_settings_state.protect.power_loss, s_power_loss_options, ARRAY_SIZE(s_power_loss_options) } },
    { "CLEAR",    SETTINGS_KIND_ACTION, .data.action = { "PROTECT", "CLEAR LATCH?", settings_state_apply_protect_clear } },
};

static const settings_item_desc_t s_system_items[] = {
    { "TIME",      SETTINGS_KIND_READONLY, .data.readonly = { format_readonly_system_time } },
    { "RTC SET",   SETTINGS_KIND_ACTION,   .data.action = { "RTC", "SYNC RTC NOW?", settings_state_apply_system_rtc_set } },
    { "SLOT",      SETTINGS_KIND_CHOICE,   .data.choice = { &g_settings_state.system.slot, s_storage_slot_options, ARRAY_SIZE(s_storage_slot_options) } },
    { "AUTO SAVE", SETTINGS_KIND_TOGGLE,   .data.toggle = { &g_settings_state.system.auto_save } },
    { "VERSION",   SETTINGS_KIND_READONLY, .data.readonly = { format_readonly_system_version } },
    { "RESET ALL", SETTINGS_KIND_ACTION,   .data.action = { "SYSTEM", "RESET PROFILE?", settings_state_apply_system_reset_all } },
};

static const settings_section_desc_t s_sections[] = {
    { APP_SETTINGS_SECTION_INPUT,   "INPUT",   s_input_items,   ARRAY_SIZE(s_input_items),   format_summary_input },
    { APP_SETTINGS_SECTION_ASRC,    "ASRC",    s_asrc_items,    ARRAY_SIZE(s_asrc_items),    format_summary_asrc },
    { APP_SETTINGS_SECTION_OUTPUT,  "OUTPUT",  s_output_items,  ARRAY_SIZE(s_output_items),  format_summary_output },
    { APP_SETTINGS_SECTION_POWER,   "POWER",   s_power_items,   ARRAY_SIZE(s_power_items),   format_summary_power },
    { APP_SETTINGS_SECTION_PROTECT, "PROTECT", s_protect_items, ARRAY_SIZE(s_protect_items), format_summary_protect },
    { APP_SETTINGS_SECTION_SYSTEM,  "SYSTEM",  s_system_items,  ARRAY_SIZE(s_system_items),  format_summary_system },
};

const settings_section_desc_t *settings_data_sections(size_t *count) {
    if (count) {
        *count = ARRAY_SIZE(s_sections);
    }
    return s_sections;
}

const settings_section_desc_t *settings_data_section(app_settings_section_t section) {
    int index = (int)section;
    if (index < 0 || index >= ARRAY_SIZE(s_sections)) {
        return NULL;
    }
    return &s_sections[index];
}

void settings_data_format_item_value(const settings_item_desc_t *item,
                                     char *buf,
                                     size_t buf_size) {
    int index;

    if (!item) {
        snprintf(buf, buf_size, "--");
        return;
    }

    switch (item->kind) {
        case SETTINGS_KIND_TOGGLE:
            snprintf(buf, buf_size, "%s",
                     (item->data.toggle.value && *item->data.toggle.value) ? "ON" : "OFF");
            break;
        case SETTINGS_KIND_CHOICE:
            index = item->data.choice.value ? *item->data.choice.value : 0;
            if (index < 0) index = 0;
            if (index >= item->data.choice.option_count) index = item->data.choice.option_count - 1;
            snprintf(buf, buf_size, "%s", item->data.choice.options[index]);
            break;
        case SETTINGS_KIND_RANGE:
            if (item->data.range.unit) {
                snprintf(buf, buf_size, "%d%s",
                         item->data.range.value ? *item->data.range.value : 0,
                         item->data.range.unit);
            } else {
                snprintf(buf, buf_size, "%d", item->data.range.value ? *item->data.range.value : 0);
            }
            break;
        case SETTINGS_KIND_ACTION:
            snprintf(buf, buf_size, "OPEN");
            break;
        case SETTINGS_KIND_READONLY:
            if (item->data.readonly.format) {
                item->data.readonly.format(buf, buf_size);
            } else {
                snprintf(buf, buf_size, "--");
            }
            break;
    }
}

void settings_data_format_section_summary(app_settings_section_t section,
                                          char *buf,
                                          size_t buf_size) {
    const settings_section_desc_t *section_desc = settings_data_section(section);

    if (section_desc && section_desc->format_summary) {
        section_desc->format_summary(buf, buf_size);
    } else {
        snprintf(buf, buf_size, "--");
    }
}

static void format_summary_input(char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "%s", s_input_source_options[g_settings_state.input.source]);
}

static void format_summary_asrc(char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "%s", s_rate_options[g_settings_state.asrc.rate]);
}

static void format_summary_output(char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "%s", s_output_target_options[g_settings_state.output.target]);
}

static void format_summary_power(char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "%s", s_power_source_options[g_settings_state.power.source]);
}

static void format_summary_protect(char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "%s", g_settings_state.protect.enable ? "ON" : "OFF");
}

static void format_summary_system(char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "RTC");
}

const char *settings_data_home_input_text(void) {
    return s_input_source_options[g_settings_state.input.source];
}

const char *settings_data_home_asrc_in_text(void) {
    return device_status_input_format();
}

const char *settings_data_home_asrc_out_text(void) {
    static char buf[16];
    snprintf(buf, sizeof(buf), "%s PCM24", s_rate_options[g_settings_state.asrc.rate]);
    return buf;
}

const char *settings_data_home_output_text(void) {
    static char buf[16];
    snprintf(buf, sizeof(buf), "%s %s",
             s_output_target_options[g_settings_state.output.target],
             s_output_mode_options[g_settings_state.output.mode]);
    return buf;
}

const char *settings_data_home_power_text(void) {
    return device_status_power_text();
}

const char *settings_data_home_temp_text(void) {
    return device_status_temp_text();
}

const char *settings_data_home_time_text(void) {
    return device_status_time_text();
}
