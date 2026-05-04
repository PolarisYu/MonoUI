#include "app_page.h"

static void app_page_on_appear(ui_page_t *page) {
    app_page_binding_t *binding = (app_page_binding_t *)page->user_data;
    if (!binding) return;

    ui_router_set_keymap(binding->keymap,
                         binding->on_action,
                         binding->ctx);

    if (binding->on_appear) {
        binding->on_appear(page);
    }
}

static void app_page_on_disappear(ui_page_t *page) {
    app_page_binding_t *binding = (app_page_binding_t *)page->user_data;
    if (!binding) return;

    if (binding->on_disappear) {
        binding->on_disappear(page);
    }
}

void app_page_init(ui_page_t *page,
                   app_page_binding_t *binding,
                   ui_widget_t *root,
                   ui_keymap_t keymap,
                   ui_action_cb_t on_action,
                   ui_page_cb_t on_appear,
                   ui_page_cb_t on_disappear,
                   void *ctx) {
    binding->keymap = keymap;
    binding->on_action = on_action;
    binding->on_appear = on_appear;
    binding->on_disappear = on_disappear;
    binding->ctx = ctx;

    ui_page_init(page, root, app_page_on_appear, app_page_on_disappear);
    page->user_data = binding;
}

const app_page_binding_t *app_page_binding(const ui_page_t *page) {
    if (!page || !page->user_data) return NULL;
    return (const app_page_binding_t *)page->user_data;
}
