#include "device_status.h"

#include <stdio.h>
#include <string.h>

#include "settings/settings_state.h"

typedef struct {
    const char *device_name;
    const char *system_summary;
    const char *input_format;
    const char *firmware_version;
    int         temperature_c;
    int         time_hour;
    int         time_minute;
} device_status_static_t;

static const device_status_static_t k_device_status_static = {
    "MONO AMP",
    "POWER RTC",
    "44K PCM24",
    "A1.0",
    42,
    21,
    48,
};

static void fill_snapshot(device_status_snapshot_t *out) {
    if (!out) {
        return;
    }

    memset(out, 0, sizeof(*out));
    out->device_name = k_device_status_static.device_name;
    out->system_summary = k_device_status_static.system_summary;
    out->input_format = k_device_status_static.input_format;
    out->firmware_version = k_device_status_static.firmware_version;
    out->temperature_c = k_device_status_static.temperature_c;
    out->time_hour = k_device_status_static.time_hour;
    out->time_minute = k_device_status_static.time_minute;
    out->input_lock = g_settings_state.input.lock;
    out->asrc_lock = g_settings_state.asrc.lock;
    out->pd_voltage = g_settings_state.power.pd_voltage;

    switch (g_settings_state.power.source) {
        case 1:
            out->power_mode = DEVICE_STATUS_POWER_DC;
            break;
        case 2:
            out->power_mode = DEVICE_STATUS_POWER_PD;
            break;
        default:
            out->power_mode = DEVICE_STATUS_POWER_AUTO;
            break;
    }
}

void device_status_get_snapshot(device_status_snapshot_t *out) {
    fill_snapshot(out);
}

const char *device_status_device_name(void) {
    return k_device_status_static.device_name;
}

const char *device_status_system_summary(void) {
    return k_device_status_static.system_summary;
}

const char *device_status_input_format(void) {
    return k_device_status_static.input_format;
}

const char *device_status_power_text(void) {
    static char buf[16];
    device_status_snapshot_t snapshot;

    fill_snapshot(&snapshot);

    switch (snapshot.power_mode) {
        case DEVICE_STATUS_POWER_DC:
            snprintf(buf, sizeof(buf), "DC");
            break;
        case DEVICE_STATUS_POWER_PD:
            snprintf(buf, sizeof(buf), "PD%uV", (unsigned)snapshot.pd_voltage);
            break;
        default:
            snprintf(buf, sizeof(buf), "AUTO");
            break;
    }
    return buf;
}

const char *device_status_temp_text(void) {
    static char buf[16];
    device_status_snapshot_t snapshot;

    fill_snapshot(&snapshot);
    snprintf(buf, sizeof(buf), "%dC", snapshot.temperature_c);
    return buf;
}

const char *device_status_time_text(void) {
    static char buf[16];
    device_status_snapshot_t snapshot;

    fill_snapshot(&snapshot);
    snprintf(buf, sizeof(buf), "%02d:%02d", snapshot.time_hour, snapshot.time_minute);
    return buf;
}

const char *device_status_version_text(void) {
    return k_device_status_static.firmware_version;
}

bool device_status_input_lock(void) {
    return g_settings_state.input.lock;
}

bool device_status_asrc_lock(void) {
    return g_settings_state.asrc.lock;
}
