#include "sim_app.h"
#include "app_ui.h"
#include "sim_input.h"

/*
 * sim_app.c 的职责极简：
 *   1. init: 调 app_ui_init，页面自己在 on_appear 里注册按键映射
 *   2. tick: 把 sim_input 采集到的物理事件同时送给两条通路：
 *            ① ui_core_push_event → widget 树事件分发（框架内部）
 *            ② app_ui_dispatch    → 翻译层 → 当前页面 on_action 回调
 *
 * sim_main.c 的事件循环已经把 poll 到的事件 push 给了 ui_core，
 * 所以这里只需要再走一次翻译层分发即可，不会重复。
 *
 * 等价的 STM32 main.c 伪代码：
 *   ui_event_t evt = hw_indev_scan();   // 硬件扫描
 *   ui_core_push_event(&evt);           // → widget 树
 *   app_ui_dispatch(&evt);              // → 翻译层 → 页面 on_action
 *   app_ui_tick(delta);
 */

static ui_event_t        s_last_evt;
static bool              s_has_evt;

/* sim_main.c 在 poll 之后调用此函数，把事件同步给翻译层 */
void sim_app_on_event(const ui_event_t *evt) {
    s_last_evt = *evt;
    s_has_evt  = true;
}

void sim_app_init(void) {
    app_ui_init();
    s_has_evt = false;
}

void sim_app_tick(uint32_t delta_ms) {
    /* 翻译层分发（widget 树分发由 ui_core_tick 内部处理）*/
    if (s_has_evt) {
        app_ui_dispatch(&s_last_evt);
        s_has_evt = false;
    }
    app_ui_tick(delta_ms);
}
