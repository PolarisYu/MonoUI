#pragma once
#include <stdint.h>
#include "monoui.h"

/* 全局字体（在 app_ui.c 中定义，所有页面共用） */
extern const ui_font_t   g_font;

/* 全局页面管理器 */
extern ui_page_manager_t g_pm;

/* 初始化所有页面，推入第一页 */
void app_ui_init(ui_page_manager_t *pm);

/* 每帧调用 */
void app_ui_tick(uint32_t delta_ms);