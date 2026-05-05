/*
 * example_main.c
 *
 * 示例入口说明文件。
 *
 * 当前推荐的接入模板已拆为两份：
 *
 *   1. example_baremetal_main.c
 *      裸机 / super-loop 版本
 *      适合 HAL_GetTick() + while(1) 主循环项目
 *
 *   2. example_rtos_task.c
 *      RTOS 任务版本
 *      适合 FreeRTOS / CMSIS-RTOS 等任务模型
 *
 * 两份示例都遵循当前项目的统一架构：
 *
 *   物理事件 ui_event_t
 *      ├─ ui_core_push_event()   -> widget 树
 *      └─ app_ui_dispatch()      -> keymap -> ui_action_t -> 页面 on_action
 *
 *   每帧 / 每轮任务
 *      └─ app_ui_tick(delta_ms)
 *
 * 接入时优先原则：
 *   - 先 ui_core_init()
 *   - 再 app_ui_init()
 *   - 输入层统一整理成 ui_event_t
 *   - 不在 ISR 里直接调用页面逻辑
 *
 * 如果你的工程已经使用 app_ui 业务层，请优先参考这两份新模板，
 * 不要再按旧版 demo 的 page_manager 手动接法实现。
 */
