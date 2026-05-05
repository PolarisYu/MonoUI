#pragma once
#include <stdint.h>
#include "app_pages.h"

extern ui_page_t page_boot;

void page_boot_build(void);
void page_boot_tick(uint32_t delta_ms);
