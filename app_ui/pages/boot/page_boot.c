#include "page_boot.h"
#include "app_ui.h"
#include "boot/boot_status.h"
#include "device/device_status.h"

#include <stdio.h>

#define BOOT_STAGE_X            8
#define BOOT_STAGE_Y            9
#define BOOT_STAGE_W            40
#define BOOT_STAGE_H            8
#define BOOT_STAGE_PITCH        9
#define BOOT_INFO_X             56
#define BOOT_INFO_Y             8
#define BOOT_INFO_W             192
#define BOOT_INFO_H             50
#define BOOT_PROGRESS_W         132
#define BOOT_PROGRESS_H         8
#define BOOT_COMPLETE_HOLD_MS   380u

static ui_widget_t root;
static ui_rect_t   bg;
static ui_rect_t   stage_rects[BOOT_STATUS_STAGE_COUNT];
static ui_label_t  stage_labels[BOOT_STATUS_STAGE_COUNT];
static ui_rect_t   info_panel;
static ui_label_t  step_text;
static ui_label_t  title_text;
static ui_label_t  detail_text;
static ui_rect_t   progress_bg;
static ui_rect_t   progress_fill;
static ui_label_t  percent_text;
static ui_label_t  footer_text;

static app_page_binding_t s_page_binding;
ui_page_t page_boot;

static uint32_t s_boot_done_hold_ms = 0;
static bool     s_boot_exit_started = false;
static char     s_percent_buf[8];
static char     s_step_buf[16];
static char     s_footer_buf[24];

static const ui_keymap_entry_t s_keymap[] = {
    UI_KEYMAP_END
};

static void sync_boot_view(void) {
    device_status_snapshot_t snapshot;
    boot_status_snapshot_t boot_snapshot;
    int stage_index;
    int i;

    device_status_get_snapshot(&snapshot);
    boot_status_get_snapshot(&boot_snapshot);
    stage_index = (boot_snapshot.stage >= BOOT_STATUS_STAGE_COUNT)
                ? (BOOT_STATUS_STAGE_COUNT - 1)
                : (int)boot_snapshot.stage;

    snprintf(s_step_buf, sizeof(s_step_buf), "STEP %d/%d", stage_index + 1, BOOT_STATUS_STAGE_COUNT);
    snprintf(s_percent_buf, sizeof(s_percent_buf), "%u%%", (unsigned)boot_snapshot.progress);
    snprintf(s_footer_buf, sizeof(s_footer_buf), "%s  SELF TEST",
             snapshot.firmware_version ? snapshot.firmware_version : "A1.0");

    ui_label_set_text(&step_text, s_step_buf);
    ui_label_set_text(&title_text, boot_snapshot.title ? boot_snapshot.title : "BOOT");
    ui_label_set_text(&detail_text, boot_snapshot.detail ? boot_snapshot.detail : "");
    ui_label_set_text(&percent_text, s_percent_buf);
    ui_label_set_text(&footer_text, s_footer_buf);

    progress_fill.base.w = (uint16_t)((BOOT_PROGRESS_W * boot_snapshot.progress) / 100);
    if (boot_snapshot.progress > 0 && progress_fill.base.w < 2) {
        progress_fill.base.w = 2;
    }
    if (boot_snapshot.progress >= 100) {
        progress_fill.base.w = BOOT_PROGRESS_W;
    }

    for (i = 0; i < BOOT_STATUS_STAGE_COUNT; ++i) {
        ui_label_set_text(&stage_labels[i], boot_status_stage_label((boot_status_stage_t)i));

        if (boot_snapshot.stage_ok[i]) {
            stage_rects[i].fill_gray = UI_GRAY_WHITE;
            stage_rects[i].border_w = 0;
            stage_labels[i].fg_gray = UI_GRAY_BLACK;
        } else if (i == stage_index) {
            stage_rects[i].fill_gray = UI_GRAY_LIGHT;
            stage_rects[i].border_w = 1;
            stage_rects[i].border_gray = UI_GRAY_WHITE;
            stage_labels[i].fg_gray = UI_GRAY_BLACK;
        } else {
            stage_rects[i].fill_gray = UI_GRAY_SHADOW;
            stage_rects[i].border_w = 0;
            stage_labels[i].fg_gray = UI_GRAY_MUTED;
        }
    }

    if (boot_snapshot.state == BOOT_STATUS_DONE) {
        footer_text.fg_gray = UI_GRAY_LIGHT;
        detail_text.fg_gray = UI_GRAY_WHITE;
    } else if (boot_snapshot.state == BOOT_STATUS_ERROR) {
        footer_text.fg_gray = UI_GRAY_WHITE;
        detail_text.fg_gray = UI_GRAY_WHITE;
        progress_fill.fill_gray = UI_GRAY_WHITE;
    } else {
        footer_text.fg_gray = UI_GRAY_MUTED;
        detail_text.fg_gray = UI_GRAY_LIGHT;
        progress_fill.fill_gray = UI_GRAY_WHITE;
    }
}

static void on_appear(ui_page_t *p) {
    (void)p;
    boot_status_reset();
    s_boot_done_hold_ms = 0;
    s_boot_exit_started = false;
    sync_boot_view();
}

void page_boot_build(void) {
    int i;

    ui_widget_init(&root, 0, 0, 256, 64, NULL);

    ui_rect_init(&bg, 0, 0, 256, 64, UI_GRAY_BLACK);
    bg.corner_r = 0;
    bg.gradient = false;
    ui_widget_add_child(&root, &bg.base);

    for (i = 0; i < BOOT_STATUS_STAGE_COUNT; ++i) {
        ui_rect_init(&stage_rects[i], BOOT_STAGE_X, BOOT_STAGE_Y + i * BOOT_STAGE_PITCH,
                     BOOT_STAGE_W, BOOT_STAGE_H, UI_GRAY_SHADOW);
        stage_rects[i].corner_r = 0;
        stage_rects[i].gradient = false;
        ui_widget_add_child(&root, &stage_rects[i].base);

        ui_label_init(&stage_labels[i], BOOT_STAGE_X + 2, BOOT_STAGE_Y + 2 + i * BOOT_STAGE_PITCH,
                      BOOT_STAGE_W - 4, 5, "", &g_font_mini,
                      UI_GRAY_MUTED, UI_GRAY_BLACK, true);
        stage_labels[i].align = UI_ALIGN_CENTER;
        ui_widget_add_child(&root, &stage_labels[i].base);
    }

    ui_rect_init(&info_panel, BOOT_INFO_X, BOOT_INFO_Y, BOOT_INFO_W, BOOT_INFO_H, UI_GRAY_SHADOW);
    info_panel.corner_r = 0;
    info_panel.gradient = false;
    ui_widget_add_child(&root, &info_panel.base);

    ui_label_init(&step_text, BOOT_INFO_X + 8, BOOT_INFO_Y + 4, 54, 5, "", &g_font_mini,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &step_text.base);

    ui_label_init(&title_text, BOOT_INFO_X + 8, BOOT_INFO_Y + 14, 118, 8, "", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &title_text.base);

    ui_label_init(&detail_text, BOOT_INFO_X + 8, BOOT_INFO_Y + 26, 136, 5, "", &g_font_mini,
                  UI_GRAY_LIGHT, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &detail_text.base);

    ui_rect_init(&progress_bg, BOOT_INFO_X + 8, BOOT_INFO_Y + 34,
                 BOOT_PROGRESS_W, BOOT_PROGRESS_H, UI_GRAY_DIM);
    progress_bg.corner_r = 0;
    progress_bg.gradient = false;
    progress_bg.border_w = 1;
    progress_bg.border_gray = UI_GRAY_LIGHT;
    ui_widget_add_child(&root, &progress_bg.base);

    ui_rect_init(&progress_fill, BOOT_INFO_X + 8, BOOT_INFO_Y + 34,
                 2, BOOT_PROGRESS_H, UI_GRAY_WHITE);
    progress_fill.corner_r = 0;
    progress_fill.gradient = false;
    ui_widget_add_child(&root, &progress_fill.base);

    ui_label_init(&percent_text, BOOT_INFO_X + 146, BOOT_INFO_Y + 34, 38, 8, "", &g_font,
                  UI_GRAY_WHITE, UI_GRAY_BLACK, true);
    percent_text.align = UI_ALIGN_RIGHT;
    ui_widget_add_child(&root, &percent_text.base);

    ui_label_init(&footer_text, BOOT_INFO_X + 8, BOOT_INFO_Y + 44, 172, 5, "", &g_font_mini,
                  UI_GRAY_MUTED, UI_GRAY_BLACK, true);
    ui_widget_add_child(&root, &footer_text.base);

    app_page_init(&page_boot, &s_page_binding, &root,
                  s_keymap, NULL, on_appear, NULL, NULL);
    sync_boot_view();
}

void page_boot_tick(uint32_t delta_ms) {
    boot_status_snapshot_t snapshot;

    if (s_boot_exit_started) {
        return;
    }

    boot_status_tick(delta_ms);
    boot_status_get_snapshot(&snapshot);
    sync_boot_view();

    if (snapshot.state == BOOT_STATUS_DONE) {
        s_boot_done_hold_ms += delta_ms;
    } else {
        s_boot_done_hold_ms = 0;
    }

    if (snapshot.state == BOOT_STATUS_DONE && s_boot_done_hold_ms >= BOOT_COMPLETE_HOLD_MS) {
        s_boot_exit_started = true;
        app_ui_replace_id(APP_PAGE_HOME, UI_TRANS_FADE, 240);
    }
}
