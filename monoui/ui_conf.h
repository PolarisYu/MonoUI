#pragma once

/* ─── Screen ──────────────────────────────────────────────────────────────── */
#define UI_SCREEN_W        256u
#define UI_SCREEN_H         64u
#define UI_FB_STRIDE       (UI_SCREEN_W / 2u)           /* 128 bytes per row  */
#define UI_FB_SIZE         (UI_FB_STRIDE * UI_SCREEN_H) /* 8192 bytes total   */

/* ─── Static pool limits (tune to your RAM budget) ───────────────────────── */
#define UI_ANIM_POOL_SIZE  32u   /* max concurrent tweens                     */
#define UI_PAGE_STACK_MAX   8u   /* max page navigation stack depth            */
#define UI_EVENT_QUEUE_SIZE 16u  /* queued input events before oldest is dropped */

/* ─── Transition ─────────────────────────────────────────────────────────── */
#define UI_TRANS_DURATION_MS  300u  /* default page-switch animation duration  */

/* ─── Frame timing ───────────────────────────────────────────────────────── */
#define UI_FRAME_MS         16u  /* call ui_core_tick() at this interval (~62 fps) */
