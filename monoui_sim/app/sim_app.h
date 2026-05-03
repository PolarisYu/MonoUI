#pragma once

#include <stdint.h>

/*  These two functions are called by sim_main.c.
 *  Implement them in sim_app.c with your actual application code.              */

/*  Called once at startup.  Build pages, call ui_page_push(), etc.            */
void sim_app_init(void);

/*  Called every frame.  Call ui_core_tick(delta_ms) here.                     */
void sim_app_tick(uint32_t delta_ms);
