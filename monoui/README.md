# monoui — 单色灰度UI框架

针对 **SSD1322 256×64 4bpp 灰度OLED** 设计，完全解耦于底层驱动。

---

## 架构总览

```
┌──────────────────────────────────────────────────────────────────────┐
│                        Application Layer                              │
│               (页面定义 / 控件树 / 业务逻辑)                          │
├────────────────────────┬─────────────────────────────────────────────┤
│    ui_page.c           │    ui_anim.c                                 │
│    页面栈 / 转场合成   │    Tween引擎 / 缓动函数库                     │
├────────────────────────┴─────────────────────────────────────────────┤
│    ui_widget.c                                                        │
│    控件树渲染 / 事件分发 / 内置控件 (rect / label / progress / image) │
├──────────────────────────────────────────────────────────────────────┤
│    ui_canvas.c                                                        │
│    4bpp原生画布 / 所有绘制基元 / 混合模式 / 转场合成                  │
├──────────────────────────────────────────────────────────────────────┤
│    ui_core.c  (HAL接口)                                               │
│    flush_fn 回调 ← 唯一与驱动的接触点                                  │
├──────────────────────────────────────────────────────────────────────┤
│    用户实现的 HAL flush                                                │
│    SSD1322_FlushArea(...)                                             │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 文件清单

| 文件 | 职责 |
|------|------|
| `ui_conf.h` | 编译期配置（屏幕尺寸、池大小、帧率） |
| `ui_canvas.h/.c` | 4bpp画布，绘制基元，混合模式，转场合成 |
| `ui_anim.h/.c` | 静态池Tween引擎，16种缓动函数 |
| `ui_widget.h/.c` | 控件基类、渲染树遍历、Rect/Label/Progress/Image |
| `ui_page.h/.c` | 页面导航栈、FADE/SLIDE_LEFT/SLIDE_RIGHT转场 |
| `ui_core.h/.c` | 拥有两个帧缓冲，驱动主Tick循环 |
| `monoui.h` | 总头文件（单include入口） |
| `example_main.c` | 示例入口说明 |
| `example_baremetal_main.c` | 裸机 / super-loop 接入模板 |
| `example_rtos_task.c` | RTOS 任务接入模板 |

---

## 接入步骤

### 1. 实现 HAL flush

```c
static void oled_flush(const uint8_t *buf,
                        uint16_t x1, uint16_t y1,
                        uint16_t x2, uint16_t y2,
                        void *ctx) {
    SSD1322_HandleTypeDef *dev = (SSD1322_HandleTypeDef *)ctx;
    SSD1322_FlushArea(dev, x1, y1, x2, y2, buf);
    // 如需等待DMA完成，可在此调用 SSD1322_WaitForDMA(dev)
}
```

### 2. 初始化

```c
ui_core_init(oled_flush, &holed);           // 注册HAL
ui_page_manager_init(&pm,
    ui_core_get_main_canvas(),
    ui_core_get_trans_canvas());
ui_core_set_page_manager(&pm);
ui_page_push(&pm, &my_first_page, UI_TRANS_NONE, 0);
```

### 3. 主循环 / RTOS任务

```c
// 每16ms调用一次
ui_core_tick(16);
```

---

## 内存占用

| 区域 | 大小 |
|------|------|
| 主帧缓冲 (main_canvas) | 8192 B |
| 转场缓冲 (trans_canvas) | 8192 B |
| Tween池 (16 slots) | ~512 B |
| 合计（不含控件/页面结构体） | **≈16.9 KB** |

STM32G4 通常具备 128 KB SRAM，SSD1322驱动本身也使用 2×8 KB，
总帧缓冲占用 **32 KB**，仍有充裕空间。

---

## 像素格式（与SSD1322完全匹配）

```
byte[i] = (pixel[2i] << 4) | pixel[2i+1]
          ↑ 偶列像素(高4bit)  ↑ 奇列像素(低4bit)
```

灰度值 `0x0`=黑, `0xF`=白，与SSD1322灰度寄存器直接对应，**flush时零转换开销**。

---

## 灰度作为设计语言

`ui_canvas.h` 中定义了语义灰度 token：

```c
UI_GRAY_BLACK   = 0x0   // 最深背景
UI_GRAY_SHADOW  = 0x2   // 阴影层
UI_GRAY_DIM     = 0x4   // 次要背景
UI_GRAY_MID     = 0x7   // 分隔线
UI_GRAY_MUTED   = 0x9   // 次要文字
UI_GRAY_LIGHT   = 0xC   // 辅助文字
UI_GRAY_BRIGHT  = 0xE   // 标题
UI_GRAY_WHITE   = 0xF   // 强调/选中
```

配合 `ui_widget_t.alpha`（动画淡入淡出）和 `ui_widget_t.dim`（整体压暗，如弹窗出现时压暗背景），
可以通过动画引擎直接驱动任意灰度属性。

---

## 动画引擎

```c
// 让进度条的 value 从 0 → 0.75，300ms，弹性回弹缓动
ui_widget_animate(&bar.value, 0.f, 0.75f, 300, ui_ease_out_bounce, NULL, NULL);

// 让整个页面淡入
ui_widget_animate(&page_root.alpha, 0.f, 1.f, 250, ui_ease_out_cubic, NULL, NULL);

// 呼吸灯效果（yoyo循环）
ui_anim_id_t id = ui_anim_create(&indicator.base.alpha, 0.3f, 1.f, 600, 0,
                                   ui_ease_in_out_quad, NULL, NULL);
ui_anim_set_yoyo(id, true);
ui_anim_start(id);
```

内置缓动函数（共16种）：
`linear` / `in/out_quad` / `in/out_cubic` / `in/out_quart` /
`out_elastic` / `in/out_back` / `out_bounce` / `in_bounce`

---

## 扩展方向（v2）

- **脏区追踪**：只重绘并只flush变化区域（可节省70%+ SPI带宽）
- **离屏图层**（Layer）：独立canvas的显式alpha合成，用于模态框遮罩
- **自定义绘制控件**：实现 `ui_widget_vtbl_t.draw` 即可，无需修改框架
- **4bpp抗锯齿字体**：使用对应字体生成工具产出4bpp字模数据
- **图像资源系统**：支持RLE压缩的4bpp图像

---

## 移植到其他屏幕

只需替换 HAL flush 函数。如果目标屏幕像素格式不同（如1bpp或8bpp），
在 flush 函数内进行格式转换即可，框架内部始终以 SSD1322 原生4bpp格式工作。
