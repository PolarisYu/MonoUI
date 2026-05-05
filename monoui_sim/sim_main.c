/*  sim_main.c — monoui PC Simulator
 *
 *  Renders the 256×64 monoui framebuffer to an SDL2 window with:
 *    - Integer-scaled display (default 4×) with optional pixel grid
 *    - Realistic OLED bezel chrome
 *    - Visual key indicators (encoder left/right, OK, Back)
 *    - Live FPS in window title
 *
 *  Key bindings:
 *    ← / ↓ / Scroll down  →  Encoder CCW
 *    → / ↑ / Scroll up    →  Encoder CW
 *    Space / Enter         →  OK button (id 0)
 *    Escape                →  Back button (id 1)
 */

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

#include "monoui.h"
#include "sim_hal.h"
#include "sim_input.h"
#include "sim_app.h"

/* ─── Layout knobs ────────────────────────────────────────────────────────── */

#define SIM_SCALE         4          /* integer zoom: 1 OLED px → N×N screen px */
#define SHOW_PIXEL_GRID   1          /* draw faint grid between pixels           */
#define PIXEL_GRID_ALPHA  35         /* 0=invisible, 255=solid                   */

/* Derived geometry */
#define DISP_W    (256 * SIM_SCALE)  /* 1024 px */
#define DISP_H    ( 64 * SIM_SCALE)  /*  256 px */
#define BEZEL_PAD 20                 /* space between display and bezel edge     */
#define MARGIN    28                 /* outer window margin                      */
#define CHROME_H  130                /* height of key-indicator strip below bezel */

#define WIN_W  (DISP_W + 2 * (BEZEL_PAD + MARGIN))
#define WIN_H  (DISP_H + 2 * (BEZEL_PAD + MARGIN) + CHROME_H)

/* Top-left corner of the OLED display area inside the window */
#define DISP_X  (MARGIN + BEZEL_PAD)
#define DISP_Y  (MARGIN + BEZEL_PAD)

/* ─── Helper draw utilities ───────────────────────────────────────────────── */

static SDL_Renderer *g_ren = NULL;

static inline void col(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    SDL_SetRenderDrawColor(g_ren, r, g, b, a);
}
static inline void fillr(int x, int y, int w, int h) {
    SDL_Rect rc = { x, y, w, h };
    SDL_RenderFillRect(g_ren, &rc);
}
static inline void liner(int x1, int y1, int x2, int y2) {
    SDL_RenderDrawLine(g_ren, x1, y1, x2, y2);
}

/* ─── Chrome rendering ────────────────────────────────────────────────────── */

static void draw_bezel(void) {
    int bx = MARGIN;
    int by = MARGIN;
    int bw = DISP_W + 2 * BEZEL_PAD;
    int bh = DISP_H + 2 * BEZEL_PAD;

    /* Shadow under bezel */
    col(0x00, 0x00, 0x00, 80);
    SDL_SetRenderDrawBlendMode(g_ren, SDL_BLENDMODE_BLEND);
    fillr(bx + 4, by + 4, bw, bh);
    SDL_SetRenderDrawBlendMode(g_ren, SDL_BLENDMODE_NONE);

    /* Bezel body */
    col(0x28, 0x28, 0x28, 0xFF);
    fillr(bx, by, bw, bh);

    /* Bezel top highlight edge */
    col(0x40, 0x40, 0x40, 0xFF);
    fillr(bx, by, bw, 1);
    col(0x38, 0x38, 0x38, 0xFF);
    fillr(bx, by + 1, bw, 1);

    /* Bezel bottom shadow edge */
    col(0x10, 0x10, 0x10, 0xFF);
    fillr(bx, by + bh - 2, bw, 2);

    /* OLED recess: thin dark border just inside bezel around display */
    col(0x08, 0x08, 0x08, 0xFF);
    fillr(DISP_X - 2, DISP_Y - 2, DISP_W + 4, DISP_H + 4);
}

static void draw_pixel_grid(void) {
#if SHOW_PIXEL_GRID
    SDL_SetRenderDrawBlendMode(g_ren, SDL_BLENDMODE_BLEND);
    col(0x00, 0x00, 0x00, PIXEL_GRID_ALPHA);

    /* Vertical grid lines (one per display pixel column) */
    for (int gx = 1; gx < 256; gx++) {
        int sx = DISP_X + gx * SIM_SCALE;
        liner(sx, DISP_Y, sx, DISP_Y + DISP_H - 1);
    }
    /* Horizontal grid lines (one per display pixel row) */
    for (int gy = 1; gy < 64; gy++) {
        int sy = DISP_Y + gy * SIM_SCALE;
        liner(DISP_X, sy, DISP_X + DISP_W - 1, sy);
    }

    SDL_SetRenderDrawBlendMode(g_ren, SDL_BLENDMODE_NONE);
#endif
}

/* ─── Key indicator block ─────────────────────────────────────────────────── */

typedef struct {
    uint8_t r, g, b;
} Color3;

static void draw_key(int x, int y, int w, int h,
                      bool pressed, Color3 on_col,
                      const char *label_hint) {
    (void)label_hint;  /* would need SDL_ttf to render — handled via window title */

    if (pressed) {
        col(on_col.r, on_col.g, on_col.b, 0xFF);
        fillr(x, y, w, h);
        /* Top shine */
        SDL_SetRenderDrawBlendMode(g_ren, SDL_BLENDMODE_BLEND);
        col(0xFF, 0xFF, 0xFF, 60);
        fillr(x + 2, y + 2, w - 4, h / 3);
        SDL_SetRenderDrawBlendMode(g_ren, SDL_BLENDMODE_NONE);
    } else {
        /* Unpressed: dark with subtle gradient illusion */
        col(0x30, 0x30, 0x30, 0xFF);
        fillr(x, y, w, h);
        col(0x3A, 0x3A, 0x3A, 0xFF);
        fillr(x, y, w, h / 2);
        col(0x25, 0x25, 0x25, 0xFF);
        fillr(x, y + h - 2, w, 2);
    }

    /* Border */
    col(0x18, 0x18, 0x18, 0xFF);
    SDL_Rect border = { x, y, w, h };
    SDL_RenderDrawRect(g_ren, &border);
}

/*
 * Chrome 布局（对应真实硬件）：
 *
 *   左侧：旋转编码器  [Q/-/CCW]  [E/+/CW]
 *   右侧：五向摇杆十字键
 *                          [ ↑ W ]
 *                  [ ← A ] [Spc ●] [ → D ]
 *                          [ ↓ S ]
 */
static void draw_chrome_keys(const sim_input_state_t *inp) {
    int base_y = DISP_Y + DISP_H + BEZEL_PAD + 12;

    /* ── 颜色 ─────────────────────────────────────────────────────────────── */
    Color3 enc_col    = { 0x55, 0xAA, 0xFF };  /* 蓝：编码器          */
    Color3 dpad_col   = { 0xDD, 0xAA, 0x33 };  /* 橙：摇杆方向        */
    Color3 center_col = { 0x44, 0xDD, 0x77 };  /* 绿：摇杆中心按下    */

    /* ── 左侧：编码器两个方向 ────────────────────────────────────────────── */
    int enc_w = 52, enc_h = 32, enc_gap = 6;
    int enc_x = MARGIN;
    int enc_y = base_y + 20;   /* 垂直居中对齐摇杆 */

    draw_key(enc_x,                 enc_y, enc_w, enc_h,
             inp->enc_ccw, enc_col, "Q/-");
    draw_key(enc_x + enc_w + enc_gap, enc_y, enc_w, enc_h,
             inp->enc_cw,  enc_col, "E/+");

    /* 编码器标识线 */
    col(0x40, 0x40, 0x40, 0xFF);
    fillr(enc_x, enc_y - 8, enc_w * 2 + enc_gap, 1);

    /* ── 右侧：五向摇杆十字 ──────────────────────────────────────────────── */
    int dw = 40, dh = 32, dgap = 4;
    /* 十字中心锚点（在 chrome 区域中偏右） */
    int cx = WIN_W - MARGIN - dw * 2 - dgap - dw / 2;
    int cy = base_y + dh + dgap / 2;   /* 中行 Y */

    /* 上 */
    draw_key(cx - dw / 2,           base_y,       dw, dh,
             inp->dpad_up,    dpad_col, "W");
    /* 左 */
    draw_key(cx - dw - dgap / 2 - dw/2, cy,       dw, dh,
             inp->dpad_left,  dpad_col, "A");
    /* 中（下压） */
    draw_key(cx - dw / 2,           cy,            dw, dh,
             inp->dpad_center, center_col, "Spc");
    /* 右 */
    draw_key(cx + dgap / 2 + dw/2,  cy,            dw, dh,
             inp->dpad_right, dpad_col, "D");
    /* 下 */
    draw_key(cx - dw / 2,           cy + dh + dgap, dw, dh,
             inp->dpad_down,  dpad_col, "S");

    /* ── 分隔线 ──────────────────────────────────────────────────────────── */
    col(0x30, 0x30, 0x30, 0xFF);
    int mid = (enc_x + enc_w * 2 + enc_gap + cx - dw) / 2;
    fillr(mid, base_y, 1, dh * 2 + dgap);
}

/* ─── FPS counter (window title) ──────────────────────────────────────────── */

static void update_title(SDL_Window *win, uint32_t fps) {
    char title[200];
    SDL_snprintf(title, sizeof(title),
        "monoui Sim | SSD1322 256x64 | %u fps | "
        "WASD/Arrows:Dpad  Spc:Center  Q/E or Scroll:Encoder",
        fps);
    SDL_SetWindowTitle(win, title);
}

/* ─── Entry point ─────────────────────────────────────────────────────────── */

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    SDL_SetMainReady();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "[sim] SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "monoui Simulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        fprintf(stderr, "[sim] SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit(); return 1;
    }

    /* Accelerated renderer with vsync */
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        /* Fallback to software renderer */
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer) {
            fprintf(stderr, "[sim] SDL_CreateRenderer: %s\n", SDL_GetError());
            SDL_DestroyWindow(window); SDL_Quit(); return 1;
        }
        fprintf(stderr, "[sim] Warning: using software renderer (no GPU?)\n");
    }
    g_ren = renderer;

    /* OLED display texture: 256×64 ARGB8888, pixel-perfect scaling */
    SDL_Texture *display_tex = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        256, 64);
    if (!display_tex) {
        fprintf(stderr, "[sim] SDL_CreateTexture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
        SDL_Quit(); return 1;
    }
    /* Nearest-neighbor scaling to keep pixel-perfect OLED look */
    SDL_SetTextureScaleMode(display_tex, SDL_ScaleModeNearest);

    /* ── Init subsystems ─────────────────────────────────────────────────── */
    sim_hal_init(renderer, display_tex);
    sim_input_init();

    /* ── Init monoui + application ────────────────────────────────────────── */
    ui_core_init(sim_hal_flush, NULL);
    sim_app_init();   /* <- User code: builds pages and pushes the initial page */

    /* ── Main loop ────────────────────────────────────────────────────────── */
    bool running = true;
    uint32_t last_ticks  = SDL_GetTicks();
    uint32_t fps_timer   = last_ticks;
    uint32_t frame_count = 0;
    uint32_t current_fps = 0;

    while (running) {
        /* ── 1. Event pump ──────────────────────────────────────────────── */
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT ||
               (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_q &&
               (e.key.keysym.mod & KMOD_CTRL))) {
                running = false;
                break;
            }
            sim_input_process(&e);

            /* Forward generated UI events:
               ① ui_core_push_event → widget 树
               ② sim_app_on_event   → 翻译层 → 页面 on_action */
            ui_event_t ui_evt;
            if (sim_input_poll(&ui_evt)) {
                ui_core_push_event(&ui_evt);
                sim_app_on_event(&ui_evt);
            }
        }

        /* ── 2. Tick UI (renders + calls sim_hal_flush internally) ───────── */
        uint32_t now   = SDL_GetTicks();
        uint32_t delta = now - last_ticks;
        last_ticks     = now;
        if (delta > 100) delta = 100;   /* clamp for debugger / sleep */

        /* 允许长按在“没有新的 SDL 事件”时也能按帧触发。 */
        sim_input_tick();
        {
            ui_event_t ui_evt;
            if (sim_input_poll(&ui_evt)) {
                ui_core_push_event(&ui_evt);
                sim_app_on_event(&ui_evt);
            }
        }

        sim_app_tick(delta);            /* <- calls ui_core_tick() */

        /* ── 3. Render window ────────────────────────────────────────────── */
        /* Window background */
        col(0x18, 0x18, 0x18, 0xFF);
        SDL_RenderClear(renderer);

        /* Bezel */
        draw_bezel();

        /* OLED display texture (scaled) */
        SDL_Rect dst_rect = { DISP_X, DISP_Y, DISP_W, DISP_H };
        SDL_RenderCopy(renderer, display_tex, NULL, &dst_rect);

        /* Pixel grid overlay */
        draw_pixel_grid();

        /* Key indicators */
        draw_chrome_keys(sim_input_state());

        /* Separator line above key indicators */
        col(0x22, 0x22, 0x22, 0xFF);
        fillr(MARGIN, DISP_Y + DISP_H + BEZEL_PAD + 8,
              WIN_W - 2 * MARGIN, 1);

        SDL_RenderPresent(renderer);

        /* ── 4. FPS counter ──────────────────────────────────────────────── */
        frame_count++;
        if (now - fps_timer >= 1000u) {
            current_fps = frame_count;
            frame_count = 0;
            fps_timer   = now;
            update_title(window, current_fps);
        }
    }

    /* ── Cleanup ──────────────────────────────────────────────────────────── */
    SDL_DestroyTexture(display_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
