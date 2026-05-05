/*
 * example_rtos_task.c
 *
 * 当前架构下的 RTOS 任务接入模板。
 *
 * 适用场景：
 *   - FreeRTOS / CMSIS-RTOS 等任务模型
 *   - 输入事件由 ISR 或其他任务写入队列
 *   - UI 任务统一取出事件并驱动 app_ui_tick()
 *
 * 推荐原则：
 *   - ISR 不直接调用页面逻辑
 *   - ISR / 驱动只投递 ui_event_t 到队列
 *   - UI task 统一调用 ui_core_push_event() 和 app_ui_dispatch()
 */

#include <stdbool.h>
#include <stdint.h>

#include "monoui.h"
#include "app_ui.h"
#include "ssd1322.h"

extern SSD1322_HandleTypeDef holed;

/*
 * 由你的 RTOS 封装层提供。
 *
 * timeout_ms:
 *   0    表示立即返回
 *   >0   表示最多等待指定毫秒
 */
extern bool board_ui_queue_receive(ui_event_t *out, uint32_t timeout_ms);

/*
 * 由你的 RTOS 时基提供。
 * FreeRTOS 可直接换成 pdMS_TO_TICKS() 配合 xTaskGetTickCount()。
 */
extern uint32_t board_ui_now_ms(void);

/*
 * 由你的 RTOS 适配层提供。
 * FreeRTOS 对应 vTaskDelay(pdMS_TO_TICKS(ms))。
 */
extern void board_ui_sleep_ms(uint32_t ms);

static void oled_flush(const uint8_t *buf,
                       uint16_t x1, uint16_t y1,
                       uint16_t x2, uint16_t y2,
                       void *ctx) {
    SSD1322_HandleTypeDef *dev = (SSD1322_HandleTypeDef *)ctx;
    SSD1322_FlushArea(dev, x1, y1, x2, y2, buf);
}

void example_rtos_ui_init(void) {
    ui_core_init(oled_flush, &holed);
    app_ui_init();
}

void example_rtos_handle_event(const ui_event_t *evt) {
    if (!evt) {
        return;
    }

    ui_core_push_event(evt);
    app_ui_dispatch(evt);
}

void example_rtos_tick(uint32_t delta_ms) {
    app_ui_tick(delta_ms);
}

/*
 * 可作为独立 UI 任务主体。
 *
 * 典型做法：
 *   - 任务周期 5~16ms
 *   - 每轮先尽量清空输入队列
 *   - 再调用一次 app_ui_tick(delta_ms)
 */
void example_rtos_ui_task(void *argument) {
    uint32_t last_ms;
    (void)argument;

    example_rtos_ui_init();
    last_ms = board_ui_now_ms();

    for (;;) {
        ui_event_t evt;
        uint32_t now_ms = board_ui_now_ms();
        uint32_t delta_ms = now_ms - last_ms;

        last_ms = now_ms;

        while (board_ui_queue_receive(&evt, 0)) {
            example_rtos_handle_event(&evt);
        }

        example_rtos_tick(delta_ms);
        board_ui_sleep_ms(UI_FRAME_MS);
    }
}
