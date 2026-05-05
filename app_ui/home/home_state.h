#pragma once

typedef enum {
    HOME_FOCUS_INPUT = 0,
    HOME_FOCUS_ASRC,
    HOME_FOCUS_OUTPUT,
    HOME_FOCUS_SYSTEM,
    HOME_FOCUS_COUNT
} home_focus_t;

typedef struct {
    home_focus_t focus;
} app_home_state_t;

extern app_home_state_t g_home_state;

void home_state_reset(void);
home_focus_t home_state_focus(void);
void home_state_set_focus(home_focus_t focus);
void home_state_step_focus(int delta);
