#include "app_pages.h"
#include "pages/home/page_home.h"
#include "pages/settings/page_settings.h"

static const app_page_entry_t s_pages[APP_PAGE_COUNT] = {
    { APP_PAGE_HOME,             "home",              &page_home,             page_home_build            },
    { APP_PAGE_SETTINGS,         "settings",          &page_settings,         page_settings_build        },
    { APP_PAGE_SETTINGS_SECTION, "settings_section",  &page_settings_section, page_settings_section_build },
};

const app_page_entry_t *app_pages_all(size_t *count) {
    if (count) {
        *count = APP_PAGE_COUNT;
    }
    return s_pages;
}

const app_page_entry_t *app_page_entry(app_page_id_t id) {
    if (id < 0 || id >= APP_PAGE_COUNT) {
        return NULL;
    }
    return &s_pages[id];
}

app_page_id_t app_page_id_from_page(const ui_page_t *page) {
    size_t i;

    if (!page) {
        return APP_PAGE_COUNT;
    }

    for (i = 0; i < APP_PAGE_COUNT; ++i) {
        if (s_pages[i].page == page) {
            return s_pages[i].id;
        }
    }

    return APP_PAGE_COUNT;
}
