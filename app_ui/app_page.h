#pragma once

#include "monoui.h"
#include "ui_action.h"

typedef struct {
    ui_keymap_t     keymap;
    ui_action_cb_t  on_action;
    ui_page_cb_t    on_appear;
    ui_page_cb_t    on_disappear;
    void           *ctx;
} app_page_binding_t;

void app_page_init(ui_page_t *page,
                   app_page_binding_t *binding,
                   ui_widget_t *root,
                   ui_keymap_t keymap,
                   ui_action_cb_t on_action,
                   ui_page_cb_t on_appear,
                   ui_page_cb_t on_disappear,
                   void *ctx);

const app_page_binding_t *app_page_binding(const ui_page_t *page);
