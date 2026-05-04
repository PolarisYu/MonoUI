#pragma once

#include <stdbool.h>
#include "ui_widget.h"

typedef union SDL_Event SDL_Event;

/* ─── 按键状态快照（用于渲染指示器和边沿检测）────────────────────────────── */
typedef struct {
    /* 五向摇杆 */
    bool dpad_up;
    bool dpad_down;
    bool dpad_left;
    bool dpad_right;
    bool dpad_center;

    /* 旋转编码器（仅在滚轮触发的帧内为 true）*/
    bool enc_cw;
    bool enc_ccw;

    bool event_pending;
} sim_input_state_t;

void sim_input_init(void);
void sim_input_process(const SDL_Event *e);
bool sim_input_poll(ui_event_t *out);
const sim_input_state_t *sim_input_state(void);