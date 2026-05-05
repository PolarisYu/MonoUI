#pragma once
#include <stdint.h>
#include "monoui.h"
#include "app_page.h"
#include "app_pages.h"
#include "ui_action.h"

/* 全局字体 */
extern const ui_font_t   g_font;

/* 全局页面管理器 */
extern ui_page_manager_t g_pm;

void app_ui_init(void);
void app_ui_tick(uint32_t delta_ms);

const app_page_entry_t *app_ui_page_entry(app_page_id_t id);
ui_page_t *app_ui_page(app_page_id_t id);
void app_ui_push(ui_page_t *page, ui_trans_type_t trans, uint32_t duration_ms);
void app_ui_pop(ui_trans_type_t trans, uint32_t duration_ms);
void app_ui_replace(ui_page_t *page, ui_trans_type_t trans, uint32_t duration_ms);
void app_ui_push_id(app_page_id_t id, ui_trans_type_t trans, uint32_t duration_ms);
void app_ui_replace_id(app_page_id_t id, ui_trans_type_t trans, uint32_t duration_ms);
ui_page_t *app_ui_current_page(void);
app_page_id_t app_ui_current_page_id(void);
bool app_ui_is_transitioning(void);

typedef void (*app_ui_popup_cb_t)(bool confirmed, void *ctx);

void app_ui_popup_show(const char *title,
                       const char *message,
                       app_ui_popup_cb_t cb,
                       void *ctx);
void app_ui_popup_hide(void);
bool app_ui_popup_visible(void);

/* 供页面调用：喂入物理事件，经翻译层后分发给当前页面的 on_action */
void app_ui_dispatch(const ui_event_t *evt);
