#include "boot_status.h"

#include <string.h>

typedef struct {
    const char *label;
    const char *title;
    const char *detail;
    uint16_t    duration_ms;
} boot_stage_desc_t;

static const boot_stage_desc_t s_stage_desc[BOOT_STATUS_STAGE_COUNT] = {
    { "POWER", "POWER CHECK", "DC PD INPUT STABLE", 520 },
    { "CORE",  "CORE INIT",   "CLOCK UI RTC ONLINE", 620 },
    { "AUDIO", "AUDIO PATH",  "INPUT ASRC OUTPUT SET", 780 },
    { "STORE", "STORE SYNC",  "LOAD LAST PROFILE", 420 },
    { "READY", "SYSTEM READY","ENTERING HOME PAGE", 360 },
};

static boot_status_snapshot_t s_snapshot = {
    .state = BOOT_STATUS_IDLE,
    .stage = BOOT_STATUS_STAGE_POWER,
    .progress = 0,
    .stage_label = "POWER",
    .title = "POWER CHECK",
    .detail = "DC PD INPUT STABLE",
    .error_code = 0,
    .stage_ok = { false, false, false, false, false },
};

static uint32_t s_elapsed_ms = 0;
static bool     s_simulation_enabled = true;

static uint32_t total_duration_ms(void) {
    uint32_t total = 0;
    int i;

    for (i = 0; i < BOOT_STATUS_STAGE_COUNT; ++i) {
        total += s_stage_desc[i].duration_ms;
    }
    return total;
}

static boot_status_stage_t stage_from_elapsed(uint32_t elapsed_ms) {
    uint32_t acc = 0;
    int i;

    for (i = 0; i < BOOT_STATUS_STAGE_COUNT; ++i) {
        acc += s_stage_desc[i].duration_ms;
        if (elapsed_ms < acc) {
            return (boot_status_stage_t)i;
        }
    }

    return BOOT_STATUS_STAGE_READY;
}

static uint8_t progress_from_elapsed(uint32_t elapsed_ms) {
    uint32_t total = total_duration_ms();

    if (elapsed_ms >= total) {
        return 100;
    }

    return (uint8_t)((elapsed_ms * 100u) / total);
}

static void sync_simulation_snapshot(void) {
    boot_status_stage_t stage = stage_from_elapsed(s_elapsed_ms);
    int i;

    s_snapshot.state = (s_elapsed_ms >= total_duration_ms()) ? BOOT_STATUS_DONE : BOOT_STATUS_RUNNING;
    s_snapshot.stage = stage;
    s_snapshot.progress = progress_from_elapsed(s_elapsed_ms);
    s_snapshot.stage_label = s_stage_desc[stage].label;
    s_snapshot.title = s_stage_desc[stage].title;
    s_snapshot.detail = s_stage_desc[stage].detail;
    s_snapshot.error_code = 0;

    for (i = 0; i < BOOT_STATUS_STAGE_COUNT; ++i) {
        s_snapshot.stage_ok[i] = (i < (int)stage)
                              || (s_snapshot.state == BOOT_STATUS_DONE && i == (int)stage);
    }
}

void boot_status_reset(void) {
    s_elapsed_ms = 0;
    s_snapshot.state = BOOT_STATUS_IDLE;
    s_snapshot.stage = BOOT_STATUS_STAGE_POWER;
    s_snapshot.progress = 0;
    s_snapshot.stage_label = s_stage_desc[0].label;
    s_snapshot.title = s_stage_desc[0].title;
    s_snapshot.detail = s_stage_desc[0].detail;
    s_snapshot.error_code = 0;
    memset(s_snapshot.stage_ok, 0, sizeof(s_snapshot.stage_ok));

    if (s_simulation_enabled) {
        sync_simulation_snapshot();
    }
}

void boot_status_tick(uint32_t delta_ms) {
    if (!s_simulation_enabled) {
        return;
    }

    if (s_snapshot.state == BOOT_STATUS_DONE || s_snapshot.state == BOOT_STATUS_ERROR) {
        return;
    }

    s_elapsed_ms += delta_ms;
    if (s_elapsed_ms > total_duration_ms()) {
        s_elapsed_ms = total_duration_ms();
    }

    sync_simulation_snapshot();
}

void boot_status_get_snapshot(boot_status_snapshot_t *out) {
    if (!out) {
        return;
    }
    *out = s_snapshot;
}

const char *boot_status_stage_label(boot_status_stage_t stage) {
    if (stage >= BOOT_STATUS_STAGE_COUNT) {
        return "BOOT";
    }
    return s_stage_desc[stage].label;
}

void boot_status_set_snapshot(const boot_status_snapshot_t *snapshot) {
    if (!snapshot) {
        return;
    }

    s_simulation_enabled = false;
    s_snapshot = *snapshot;
}

void boot_status_set_simulation_enabled(bool enabled) {
    s_simulation_enabled = enabled;
    if (enabled) {
        boot_status_reset();
    }
}
