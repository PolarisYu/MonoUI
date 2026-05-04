#pragma once
#include <stdint.h>
#include "ui_widget.h"   /* ui_event_t */

void sim_app_init(void);
void sim_app_tick(uint32_t delta_ms);

/* sim_main.c 在 poll 到事件后调用此函数，转发给翻译层 */
void sim_app_on_event(const ui_event_t *evt);