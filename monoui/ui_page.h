#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "ui_widget.h"
#include "ui_canvas.h"
#include "ui_anim.h"

/* ═══════════════════════════════════════════════════════════════════════════
 * Page manager
 *
 * Manages a navigation stack of pages (screens). Each page owns a root
 * widget tree and optional enter/exit lifecycle callbacks.
 *
 * Transitions are driven by the animation engine and require the caller
 * to provide a second canvas for off-screen rendering of the outgoing page.
 *
 * Typical usage:
 *   ui_page_t home, settings;
 *   ui_page_init(&home, &home_root_widget, home_on_appear, home_on_disappear);
 *   ui_page_init(&settings, &settings_root, NULL, NULL);
 *
 *   ui_page_manager_t pm;
 *   ui_page_manager_init(&pm, &main_canvas, &transition_canvas);
 *
 *   ui_page_push(&pm, &home, UI_TRANS_NONE, 0);   // initial page
 *   // later:
 *   ui_page_push(&pm, &settings, UI_TRANS_SLIDE_LEFT, 300);
 *   ui_page_pop (&pm, UI_TRANS_SLIDE_RIGHT, 300);
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    UI_TRANS_NONE = 0,
    UI_TRANS_FADE,
    UI_TRANS_SLIDE_LEFT,    /* push: new page slides in from right */
    UI_TRANS_SLIDE_RIGHT,   /* pop:  old page slides out to right  */
} ui_trans_type_t;

/* ─── Page descriptor ─────────────────────────────────────────────────────── */

typedef struct ui_page_s ui_page_t;

typedef void (*ui_page_cb_t)(ui_page_t *page);

struct ui_page_s {
    ui_widget_t   *root;          /* root widget of this page's UI tree        */
    ui_page_cb_t   on_appear;     /* called after transition in completes       */
    ui_page_cb_t   on_disappear;  /* called after transition out completes      */
    void          *user_data;
};

/* ─── Page manager ────────────────────────────────────────────────────────── */

typedef enum {
    UI_PM_IDLE = 0,
    UI_PM_TRANSITIONING,
} ui_pm_state_t;

typedef struct {
    /* Navigation stack */
    ui_page_t    *stack[UI_PAGE_STACK_MAX];
    int           stack_top;          /* index of current page; -1 = empty     */

    /* Render canvases (caller provides, never freed by manager) */
    ui_canvas_t  *main_canvas;        /* rendered to every frame               */
    ui_canvas_t  *trans_canvas;       /* off-screen target for outgoing page   */

    /* Transition state */
    ui_pm_state_t state;
    ui_page_t    *incoming;
    ui_page_t    *outgoing;
    ui_trans_type_t trans_type;
    ui_anim_id_t    trans_anim;
    float           trans_progress;   /* 0.0 → 1.0, driven by tween           */
    bool            is_push;          /* true=push (grow stack), false=pop      */

} ui_page_manager_t;

/* ─── API ─────────────────────────────────────────────────────────────────── */

void ui_page_init(ui_page_t *page, ui_widget_t *root,
                  ui_page_cb_t on_appear, ui_page_cb_t on_disappear);

void ui_page_manager_init(ui_page_manager_t *pm,
                           ui_canvas_t *main_canvas,
                           ui_canvas_t *trans_canvas);

/*  Push a new page onto the stack (navigate forward).                          */
void ui_page_push(ui_page_manager_t *pm, ui_page_t *page,
                  ui_trans_type_t trans, uint32_t duration_ms);

/*  Pop the top page (navigate back).                                           */
void ui_page_pop(ui_page_manager_t *pm,
                 ui_trans_type_t trans, uint32_t duration_ms);

/*  Replace the top page without altering stack depth.                          */
void ui_page_replace(ui_page_manager_t *pm, ui_page_t *page,
                     ui_trans_type_t trans, uint32_t duration_ms);

/*  Returns the currently visible page (or NULL if stack empty).                */
ui_page_t *ui_page_current(const ui_page_manager_t *pm);

/*  True while a transition animation is in progress.                           */
bool ui_page_is_transitioning(const ui_page_manager_t *pm);

/*  Called by ui_core_tick() every frame to composite the current frame.
 *  Renders into the canvases provided at init time.                            */
void ui_page_manager_render(ui_page_manager_t *pm);

/*  Dispatch an input event to the current page's widget tree.                  */
void ui_page_dispatch_event(ui_page_manager_t *pm, const ui_event_t *evt);
