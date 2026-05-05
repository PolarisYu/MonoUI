#include "settings_state.h"

static const app_settings_state_t k_default_settings_state = {
    .input = {
        .source = 0,
        .auto_detect = true,
        .priority = 0,
        .lock = true,
        .gain = 12,
    },
    .asrc = {
        .enable = true,
        .lock = true,
        .rate = 1,
        .filter = 1,
        .jitter = 40,
        .phase = 0,
    },
    .output = {
        .target = 0,
        .mode = 0,
        .volume = 62,
        .gain = 10,
        .mute = false,
    },
    .power = {
        .source = 2,
        .pd_voltage = 15,
        .sleep = 1,
        .rtc_sync = true,
        .store = true,
    },
    .protect = {
        .enable = true,
        .temp_limit = 75,
        .dc = true,
        .clip = true,
        .power_loss = 1,
    },
    .system = {
        .slot = 0,
        .auto_save = true,
    },
};

app_settings_state_t g_settings_state = {
    .input = {
        .source = 0,
        .auto_detect = true,
        .priority = 0,
        .lock = true,
        .gain = 12,
    },
    .asrc = {
        .enable = true,
        .lock = true,
        .rate = 1,
        .filter = 1,
        .jitter = 40,
        .phase = 0,
    },
    .output = {
        .target = 0,
        .mode = 0,
        .volume = 62,
        .gain = 10,
        .mute = false,
    },
    .power = {
        .source = 2,
        .pd_voltage = 15,
        .sleep = 1,
        .rtc_sync = true,
        .store = true,
    },
    .protect = {
        .enable = true,
        .temp_limit = 75,
        .dc = true,
        .clip = true,
        .power_loss = 1,
    },
    .system = {
        .slot = 0,
        .auto_save = true,
    },
};

void settings_state_apply_input_rescan(void) {
    g_settings_state.input.lock = true;
}

void settings_state_apply_output_safe(void) {
    g_settings_state.output.mute = true;
}

void settings_state_apply_power_shutdown(void) {
    g_settings_state.power.store = true;
}

void settings_state_apply_protect_clear(void) {
    g_settings_state.protect.clip = false;
}

void settings_state_apply_system_rtc_set(void) {
}

void settings_state_apply_system_reset_all(void) {
    g_settings_state = k_default_settings_state;
}
