#include "home_state.h"

app_home_state_t g_home_state = {
    .focus = HOME_FOCUS_INPUT,
};

void home_state_reset(void) {
    g_home_state.focus = HOME_FOCUS_INPUT;
}

home_focus_t home_state_focus(void) {
    return g_home_state.focus;
}

void home_state_set_focus(home_focus_t focus) {
    if (focus < 0 || focus >= HOME_FOCUS_COUNT) {
        g_home_state.focus = HOME_FOCUS_INPUT;
        return;
    }
    g_home_state.focus = focus;
}

void home_state_step_focus(int delta) {
    int next = (int)g_home_state.focus + delta;

    while (next < 0) {
        next += HOME_FOCUS_COUNT;
    }
    while (next >= HOME_FOCUS_COUNT) {
        next -= HOME_FOCUS_COUNT;
    }

    g_home_state.focus = (home_focus_t)next;
}
