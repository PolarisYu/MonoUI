#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "ui_widget.h"   /* ui_evt_type_t */

/* ═══════════════════════════════════════════════════════════════════════════
 * Input Translation Layer  (Hardware Event → Logical Action)
 *
 * 架构位置：
 *
 *   硬件 / 模拟器
 *       │  ui_event_t  (物理事件：DPAD_UP / ENCODER_CW / ...)
 *       ▼
 *   ui_action_router              ← 本层
 *       │  ui_action_t  (逻辑动作：NAV_UP / CONFIRM / SCROLL / ...)
 *       ▼
 *   页面 on_action 回调
 *
 * 每个页面注册一张 ui_keymap_t，激活页面切换时路由器自动换图。
 * 同一个物理按键在不同页面可以映射到完全不同的逻辑动作。
 * ═══════════════════════════════════════════════════════════════════════════ */


/* ─── 逻辑动作枚举 ────────────────────────────────────────────────────────── */
/*
 * 动作按语义分组，页面只处理自己关心的那些，其余忽略即可。
 *
 * 命名约定：
 *   NAV_*    导航类（移动光标/焦点）
 *   SCROLL_* 滚动类（列表、参数调节）
 *   CMD_*    命令类（确认、取消、菜单）
 *   CUSTOM_* 页面自定义扩展（最多 16 个）
 */
typedef enum {
    UI_ACTION_NONE = 0,

    /* ── 导航 ─────────────────────────────────────────────────────────────── */
    UI_ACTION_NAV_UP,        /* 光标/焦点向上移动一步                         */
    UI_ACTION_NAV_DOWN,      /* 光标/焦点向下移动一步                         */
    UI_ACTION_NAV_LEFT,      /* 光标/焦点向左移动 / 返回上一级                */
    UI_ACTION_NAV_RIGHT,     /* 光标/焦点向右移动 / 进入下一级                */

    /* ── 滚动 / 调节 ──────────────────────────────────────────────────────── */
    UI_ACTION_SCROLL_UP,     /* 列表向上滚动 / 数值增大                       */
    UI_ACTION_SCROLL_DOWN,   /* 列表向下滚动 / 数值减小                       */

    /* ── 命令 ─────────────────────────────────────────────────────────────── */
    UI_ACTION_CONFIRM,       /* 确认 / 进入 / 选择                            */
    UI_ACTION_CANCEL,        /* 取消 / 返回                                   */
    UI_ACTION_MENU,          /* 呼出菜单 / 长按扩展操作                       */

    /* ── 页面自定义（0–15）─────────────────────────────────────────────────── */
    UI_ACTION_CUSTOM_0  = 0x20,
    UI_ACTION_CUSTOM_1,
    UI_ACTION_CUSTOM_2,
    UI_ACTION_CUSTOM_3,
    UI_ACTION_CUSTOM_4,
    UI_ACTION_CUSTOM_5,
    UI_ACTION_CUSTOM_6,
    UI_ACTION_CUSTOM_7,
    UI_ACTION_CUSTOM_8,
    UI_ACTION_CUSTOM_9,
    UI_ACTION_CUSTOM_10,
    UI_ACTION_CUSTOM_11,
    UI_ACTION_CUSTOM_12,
    UI_ACTION_CUSTOM_13,
    UI_ACTION_CUSTOM_14,
    UI_ACTION_CUSTOM_15,

    UI_ACTION__COUNT
} ui_action_t;

/* ─── 按键映射表 ──────────────────────────────────────────────────────────── */

/*
 * 一条映射规则：某个物理事件 → 某个逻辑动作。
 *
 * evt   : 要匹配的物理事件类型（ui_evt_type_t）
 * value : 事件附加值过滤，-1 表示匹配任意 value
 * action: 映射到的逻辑动作
 */
typedef struct {
    ui_evt_type_t evt;
    int32_t       value;      /* -1 = 通配                                    */
    ui_action_t   action;
} ui_keymap_entry_t;

#define UI_KEYMAP_END  { UI_EVT_NONE, 0, UI_ACTION_NONE }   /* 终止哨兵      */
#define UI_ANY_VALUE   (-1)

/*
 * 按键映射表：由若干 ui_keymap_entry_t 组成，最后一条必须是 UI_KEYMAP_END。
 *
 * 示例（主菜单页面）：
 *
 *   static const ui_keymap_entry_t keymap_home[] = {
 *       { UI_EVT_DPAD_UP,      UI_ANY_VALUE, UI_ACTION_NAV_UP      },
 *       { UI_EVT_DPAD_DOWN,    UI_ANY_VALUE, UI_ACTION_NAV_DOWN    },
 *       { UI_EVT_ENCODER_CW,   UI_ANY_VALUE, UI_ACTION_SCROLL_DOWN },
 *       { UI_EVT_ENCODER_CCW,  UI_ANY_VALUE, UI_ACTION_SCROLL_UP   },
 *       { UI_EVT_DPAD_CENTER,  UI_ANY_VALUE, UI_ACTION_CONFIRM     },
 *       { UI_EVT_DPAD_LEFT,    UI_ANY_VALUE, UI_ACTION_CANCEL      },
 *       UI_KEYMAP_END
 *   };
 *
 * 示例（参数调节页面，编码器换成调节数值）：
 *
 *   static const ui_keymap_entry_t keymap_param[] = {
 *       { UI_EVT_ENCODER_CW,   UI_ANY_VALUE, UI_ACTION_SCROLL_UP   },
 *       { UI_EVT_ENCODER_CCW,  UI_ANY_VALUE, UI_ACTION_SCROLL_DOWN },
 *       { UI_EVT_DPAD_CENTER,  UI_ANY_VALUE, UI_ACTION_CONFIRM     },
 *       { UI_EVT_DPAD_LEFT,    UI_ANY_VALUE, UI_ACTION_CANCEL      },
 *       { UI_EVT_BTN_LONG_PRESS, UI_ANY_VALUE, UI_ACTION_MENU      },
 *       UI_KEYMAP_END
 *   };
 */
typedef const ui_keymap_entry_t * ui_keymap_t;

/* ─── 动作回调 ────────────────────────────────────────────────────────────── */

/*
 * 页面注册此回调接收翻译后的逻辑动作。
 * action : 翻译结果
 * raw    : 原始物理事件（需要 value 等细节时使用，可忽略）
 */
typedef void (*ui_action_cb_t)(ui_action_t action,
                                const ui_event_t *raw,
                                void *ctx);

/* ─── 路由器 API ──────────────────────────────────────────────────────────── */

/*  初始化（程序启动时调用一次）                                               */
void ui_router_init(void);

/*  切换当前激活的按键映射表和回调（通常在 page.on_appear 里调用）
 *
 *  keymap  : 新映射表，NULL 表示沿用上次的映射
 *  cb      : 动作回调，NULL 表示只翻译不分发
 *  ctx     : 传给回调的用户指针                                              */
void ui_router_set_keymap(ui_keymap_t keymap,
                           ui_action_cb_t cb,
                           void *ctx);

/*  喂入一个物理事件 → 查表翻译 → 调用当前回调
 *  通常由 app_ui_tick() 在 ui_core_tick() 之前调用。
 *  返回翻译后的 action（UI_ACTION_NONE 表示未命中）                          */
ui_action_t ui_router_dispatch(const ui_event_t *evt);

/*  直接查询：不调用回调，只返回翻译结果（用于轮询场景）                       */
ui_action_t ui_router_translate(const ui_event_t *evt);

/* ─── 内置默认映射表（可直接使用，也可作为编写自定义表的参考）──────────────── */

/*  通用菜单：摇杆上下导航，编码器滚动，中键确认，左键/Esc取消               */
extern const ui_keymap_entry_t ui_keymap_menu[];

/*  参数调节：编码器调值，摇杆上下也调值，中键确认，左键取消                  */
extern const ui_keymap_entry_t ui_keymap_param[];

/*  纯编码器：只响应编码器和中键（适合播放控制、亮度调节等）                   */
extern const ui_keymap_entry_t ui_keymap_encoder_only[];