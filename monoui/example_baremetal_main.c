/*
 * example_baremetal_main.c
 *
 * 当前架构下的裸机 / super-loop 接入模板。
 *
 * 适用场景：
 *   - while(1) 主循环
 *   - SysTick / HAL_GetTick() 提供毫秒时基
 *   - 按键 / 编码器由轮询或中断写队列后在主循环中取出
 *
 * 事件流：
 *   board_ui_poll_event() -> example_baremetal_handle_event()
 *                         -> ui_core_push_event()
 *                         -> app_ui_dispatch()
 */

#include <stdbool.h>
#include <stdint.h>

#include "monoui.h"
#include "app_ui.h"
#include "ssd1322.h"

extern SSD1322_HandleTypeDef holed;

extern bool board_ui_poll_event(ui_event_t *out);
extern uint32_t board_ui_now_ms(void);

static void oled_flush(const uint8_t *buf,
                       uint16_t x1, uint16_t y1,
                       uint16_t x2, uint16_t y2,
                       void *ctx) {
    SSD1322_HandleTypeDef *dev = (SSD1322_HandleTypeDef *)ctx;
    SSD1322_FlushArea(dev, x1, y1, x2, y2, buf);
}

void example_baremetal_ui_init(void) {
    ui_core_init(oled_flush, &holed);
    app_ui_init();
}

void example_baremetal_handle_event(const ui_event_t *evt) {
    if (!evt) {
        return;
    }

    ui_core_push_event(evt);
    app_ui_dispatch(evt);
}

void example_baremetal_tick(uint32_t delta_ms) {
    app_ui_tick(delta_ms);
}

void example_baremetal_run_forever(void) {
    uint32_t last_ms;

    example_baremetal_ui_init();
    last_ms = board_ui_now_ms();

    for (;;) {
        ui_event_t evt;
        uint32_t now_ms = board_ui_now_ms();
        uint32_t delta_ms = now_ms - last_ms;

        last_ms = now_ms;

        while (board_ui_poll_event(&evt)) {
            example_baremetal_handle_event(&evt);
        }

        example_baremetal_tick(delta_ms);
    }
}

void example_baremetal_push_event(ui_evt_type_t type, int32_t value) {
    ui_event_t evt;

    evt.type = type;
    evt.value = value;
    example_baremetal_handle_event(&evt);
}
