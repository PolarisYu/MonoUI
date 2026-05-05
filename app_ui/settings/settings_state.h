#pragma once

#include <stdbool.h>

typedef struct {
    int  source;
    bool auto_detect;
    int  priority;
    bool lock;
    int  gain;
} settings_input_state_t;

typedef struct {
    bool enable;
    bool lock;
    int  rate;
    int  filter;
    int  jitter;
    int  phase;
} settings_asrc_state_t;

typedef struct {
    int  target;
    int  mode;
    int  volume;
    int  gain;
    bool mute;
} settings_output_state_t;

typedef struct {
    int  source;
    int  pd_voltage;
    int  sleep;
    bool rtc_sync;
    bool store;
} settings_power_state_t;

typedef struct {
    bool enable;
    int  temp_limit;
    bool dc;
    bool clip;
    int  power_loss;
} settings_protect_state_t;

typedef struct {
    int  slot;
    bool auto_save;
} settings_system_state_t;

typedef struct {
    settings_input_state_t   input;
    settings_asrc_state_t    asrc;
    settings_output_state_t  output;
    settings_power_state_t   power;
    settings_protect_state_t protect;
    settings_system_state_t  system;
} app_settings_state_t;

extern app_settings_state_t g_settings_state;

void settings_state_apply_input_rescan(void);
void settings_state_apply_output_safe(void);
void settings_state_apply_power_shutdown(void);
void settings_state_apply_protect_clear(void);
void settings_state_apply_system_rtc_set(void);
void settings_state_apply_system_reset_all(void);
