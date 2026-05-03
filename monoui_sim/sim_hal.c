#include "sim_hal.h"
#include "ui_conf.h"   /* UI_SCREEN_W, UI_SCREEN_H, UI_FB_STRIDE */

#include <SDL2/SDL.h>
#include <string.h>
#include <stddef.h>

/* ─── Internal state ──────────────────────────────────────────────────────── */

static SDL_Renderer *s_renderer = NULL;
static SDL_Texture  *s_texture  = NULL;

/* ARGB intermediate buffer (256×64 × 4 bytes = 65536 bytes) */
static uint32_t s_argb[UI_SCREEN_W * UI_SCREEN_H];

/* OLED phosphor color (applied as a multiplicative tint) */
static uint8_t s_pr = 200;   /* R */
static uint8_t s_pg = 225;   /* G */
static uint8_t s_pb = 255;   /* B */

/* ─── Public API ──────────────────────────────────────────────────────────── */

void sim_hal_init(SDL_Renderer *renderer, SDL_Texture *display_texture) {
    s_renderer = renderer;
    s_texture  = display_texture;
}

void sim_hal_set_phosphor(uint8_t r, uint8_t g, uint8_t b) {
    s_pr = r; s_pg = g; s_pb = b;
}

bool sim_hal_is_ready(void) {
    return (s_renderer != NULL && s_texture != NULL);
}

/* ─── Flush implementation ────────────────────────────────────────────────── */

/*  Unpack the 4bpp monoui buffer to ARGB8888 and upload to the SDL2 texture.
 *
 *  4bpp layout (identical to SSD1322 native format):
 *    byte[y*128 + x/2] = (pixel[x_even] << 4) | pixel[x_odd]
 *
 *  Gray mapping: gray4 ∈ [0,15] → gray8 = gray4 * 17  (so 0→0, 15→255)
 *  Color: gray8 scaled by phosphor (r,g,b) → tinted OLED appearance.          */
void sim_hal_flush(const uint8_t *buf,
                    uint16_t x1, uint16_t y1,
                    uint16_t x2, uint16_t y2,
                    void *ctx) {
    (void)x1; (void)y1; (void)x2; (void)y2; (void)ctx;

    if (!s_texture) return;

    const uint8_t *src = buf;

    for (int y = 0; y < (int)UI_SCREEN_H; y++) {
        uint32_t *dst_row = &s_argb[y * UI_SCREEN_W];
        const uint8_t *src_row = src + (size_t)y * UI_FB_STRIDE;

        for (int x = 0; x < (int)UI_SCREEN_W; x++) {
            uint8_t byte  = src_row[x >> 1];
            uint8_t g4    = (x & 1) ? (byte & 0x0Fu) : (byte >> 4);
            uint8_t g8    = (uint8_t)(g4 * 17u);  /* 0→0, 15→255, perfectly linear */

            /*  Phosphor tint: scale each channel proportionally.
             *  Fast integer path: (channel * g8) >> 8  ≈ channel * g8/255.
             *  For g8=255: out = channel (full brightness); g8=0: out = 0.     */
            uint8_t r = (uint8_t)(((uint16_t)s_pr * g8) >> 8);
            uint8_t g = (uint8_t)(((uint16_t)s_pg * g8) >> 8);
            uint8_t b = (uint8_t)(((uint16_t)s_pb * g8) >> 8);

            /* SDL2 ARGB8888 = 0xAARRGGBB */
            dst_row[x] = (0xFFu << 24) | ((uint32_t)r << 16)
                                        | ((uint32_t)g <<  8)
                                        |  (uint32_t)b;
        }
    }

    /* Upload to GPU texture (row pitch = width × 4 bytes) */
    SDL_UpdateTexture(s_texture, NULL, s_argb,
                      (int)(UI_SCREEN_W * sizeof(uint32_t)));
}
