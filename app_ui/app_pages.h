#pragma once

#include <stddef.h>
#include "monoui.h"

typedef enum {
    APP_PAGE_HOME = 0,
    APP_PAGE_SETTINGS,
    APP_PAGE_SETTINGS_SECTION,
    APP_PAGE_COUNT
} app_page_id_t;

typedef struct {
    app_page_id_t id;
    const char   *name;
    ui_page_t    *page;
    void        (*build)(void);
} app_page_entry_t;

const app_page_entry_t *app_pages_all(size_t *count);
const app_page_entry_t *app_page_entry(app_page_id_t id);
app_page_id_t app_page_id_from_page(const ui_page_t *page);
