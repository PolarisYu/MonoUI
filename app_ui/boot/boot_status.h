#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BOOT_STATUS_STAGE_POWER = 0,
    BOOT_STATUS_STAGE_CORE,
    BOOT_STATUS_STAGE_AUDIO,
    BOOT_STATUS_STAGE_STORE,
    BOOT_STATUS_STAGE_READY,
    BOOT_STATUS_STAGE_COUNT,
    BOOT_STATUS_STAGE_ERROR
} boot_status_stage_t;

typedef enum {
    BOOT_STATUS_IDLE = 0,
    BOOT_STATUS_RUNNING,
    BOOT_STATUS_DONE,
    BOOT_STATUS_ERROR
} boot_status_state_t;

typedef struct {
    boot_status_state_t state;
    boot_status_stage_t stage;
    uint8_t             progress;
    const char         *stage_label;
    const char         *title;
    const char         *detail;
    int                 error_code;
    bool                stage_ok[BOOT_STATUS_STAGE_COUNT];
} boot_status_snapshot_t;

void boot_status_reset(void);
void boot_status_tick(uint32_t delta_ms);
void boot_status_get_snapshot(boot_status_snapshot_t *out);
const char *boot_status_stage_label(boot_status_stage_t stage);

/* 预留给真实硬件接入：由底层管理器把 SPI 自检结果写入这里。 */
void boot_status_set_snapshot(const boot_status_snapshot_t *snapshot);
void boot_status_set_simulation_enabled(bool enabled);
