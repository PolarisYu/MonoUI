#pragma once

#include <stdbool.h>

typedef enum {
    DEVICE_STATUS_POWER_AUTO = 0,
    DEVICE_STATUS_POWER_DC,
    DEVICE_STATUS_POWER_PD
} device_status_power_mode_t;

typedef struct {
    const char                *device_name;
    const char                *system_summary;
    const char                *input_format;
    const char                *firmware_version;
    device_status_power_mode_t power_mode;
    int                        pd_voltage;
    int                        temperature_c;
    int                        time_hour;
    int                        time_minute;
    bool                       input_lock;
    bool                       asrc_lock;
} device_status_snapshot_t;

void device_status_get_snapshot(device_status_snapshot_t *out);

const char *device_status_device_name(void);
const char *device_status_system_summary(void);
const char *device_status_input_format(void);
const char *device_status_power_text(void);
const char *device_status_temp_text(void);
const char *device_status_time_text(void);
const char *device_status_version_text(void);

bool device_status_input_lock(void);
bool device_status_asrc_lock(void);
