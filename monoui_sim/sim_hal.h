#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Opaque SDL types — avoids pulling SDL2 headers into monoui code */
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture  SDL_Texture;

/* ─── Initialise the simulator HAL ───────────────────────────────────────── */

/*  Call once after SDL_CreateRenderer / SDL_CreateTexture.                    */
void sim_hal_init(SDL_Renderer *renderer, SDL_Texture *display_texture);

/* ─── HAL flush callback (register with ui_core_init) ────────────────────── */

/*  This IS the ui_hal_flush_fn implementation.  Pass it directly:
 *
 *     ui_core_init(sim_hal_flush, NULL);
 *
 *  It unpacks the 4bpp monoui framebuffer → ARGB8888, then uploads
 *  to the SDL2 texture for rendering.                                          */
void sim_hal_flush(const uint8_t *buf,
                    uint16_t x1, uint16_t y1,
                    uint16_t x2, uint16_t y2,
                    void *ctx);

/* ─── Optional: override OLED phosphor color ─────────────────────────────── */

/*  Default: (200, 225, 255) — cool-white, close to white SSD1322 OLEDs.
 *  Try (255, 240, 160) for yellow-green OLED warmth.
 *  Gray value 0→(0,0,0), gray value 15→(r,g,b) scaled linearly.              */
void sim_hal_set_phosphor(uint8_t r, uint8_t g, uint8_t b);

/*  True once init has been called with valid pointers.                         */
bool sim_hal_is_ready(void);
