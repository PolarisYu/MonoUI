# monoui_sim — PC Simulator

基于 **SDL2** 的桌面预览器，让你在不接硬件的情况下运行和调试完整的 monoui 页面与动画。

---

## 窗口预览

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │  Bezel                                                              │   │
│   │  ┌───────────────────────────────────────────────────────────────┐  │   │
│   │  │                                                               │  │   │
│   │  │   256×64 OLED 以 4× 放大  (1024×256 px)                      │  │   │
│   │  │   带像素网格叠加层，真实还原单像素边界                          │  │   │
│   │  │                                                               │  │   │
│   │  └───────────────────────────────────────────────────────────────┘  │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│   [←CCW] [CW→]                            [  OK  ] [ Back ]                │
│   (按下时高亮)                              (绿色)    (红色)                 │
└─────────────────────────────────────────────────────────────────────────────┘
 标题栏: monoui Simulator | SSD1322 256×64 @ 62 fps | ← → Encoder  Space OK  Esc Back
```

---

## 目录结构

```
monoui_sim/
├── CMakeLists.txt    构建系统 (不需要改)
├── sim_main.c        SDL2 窗口 + 渲染主循环 (不需要改)
├── sim_hal.c/.h      4bpp→ARGB 转换 + SDL2 纹理更新 (不需要改)
├── sim_input.c/.h    键盘/鼠标→ ui_event_t (不需要改)
└── app/
    ├── sim_app.h     接口声明 (不需要改)
    └── sim_app.c     ← 你只改这一个文件
```

---

## 依赖安装

**macOS**
```bash
brew install sdl2 cmake
```

**Ubuntu / Debian**
```bash
sudo apt install libsdl2-dev cmake build-essential
```

**Windows**
- 从 [libsdl.org](https://libsdl.org) 下载 SDL2-devel-x.x.x-VC.zip
- 解压到任意路径，CMake 配置时指定 `-DSDL2_DIR=<path>`

---

## 编译运行

假设目录布局为：
```
dev/
├── monoui/          ← 框架源码
└── monoui_sim/      ← 本目录
```

```bash
cd monoui_sim
mkdir build && cd build
cmake ..
cmake --build .
./monoui_sim          # Linux/macOS
monoui_sim.exe        # Windows
```

默认情况下，CMake 会从 `../monoui` 和 `../app_ui` 取源码。
如果目录不在默认位置，可以覆盖这两个缓存变量：

```bash
cmake .. \
  -DMONOUI_DIR=/absolute/path/to/monoui \
  -DAPP_UI_DIR=/absolute/path/to/app_ui
```

`MONOUI_DIR` 必须包含 `monoui.h`，`APP_UI_DIR` 必须包含 `app_ui.h`；
路径无效时，配置阶段会直接报错。

---

## 接入你自己的应用代码

只需修改 `app/sim_app.c`，把你在 STM32 上写的页面初始化代码粘贴进去：

```c
void sim_app_init(void) {
    ui_page_manager_init(&pm,
        ui_core_get_main_canvas(),
        ui_core_get_trans_canvas());
    ui_core_set_page_manager(&pm);

    // 你的 home_build(), settings_build() 等...
    ui_page_push(&pm, &my_first_page, UI_TRANS_NONE, 0);
}

void sim_app_tick(uint32_t delta_ms) {
    // 导航逻辑 + ui_core_tick(delta_ms)
    ui_core_tick(delta_ms);
}
```

**STM32 和 PC 的代码完全共用，一行不差。** 唯一的差别是：
- STM32: `ui_core_init(my_ssd1322_flush, &holed)`
- PC Sim: `ui_core_init(sim_hal_flush, NULL)`  ← sim_main.c 已经帮你做了

---

## 按键映射

| 按键 | 对应硬件 | 生成的 ui_event_t |
|------|----------|-------------------|
| `→` / `↑` / 鼠标滚轮上 | 编码器顺时针 | `UI_EVT_ENCODER_CW` |
| `←` / `↓` / 鼠标滚轮下 | 编码器逆时针 | `UI_EVT_ENCODER_CCW` |
| `Space` / `Enter` | 确认按钮 (id=0) | `UI_EVT_BTN_PRESS` |
| `Esc` | 返回按钮 (id=1) | `UI_EVT_BTN_PRESS` |
| `Ctrl+Q` | — | 退出模拟器 |

---

## 自定义 OLED 磷光颜色

在 `sim_main.c` 的 `sim_hal_init()` 之后调用：

```c
sim_hal_set_phosphor(255, 240, 160);  // 暖黄色 OLED
sim_hal_set_phosphor(180, 255, 200);  // 绿色 OLED
sim_hal_set_phosphor(200, 225, 255);  // 冷白色 (默认)
```

---

## 工作原理

```
SDL 事件 ──→ sim_input_process() ──→ ui_core_push_event()
                                             │
                                     ui_core_tick(Δms)
                                             │
                           ┌────────────────→│←────────────────────┐
                      ui_anim_tick()   page_render()        sim_hal_flush()
                      更新 float*        控件树绘制         4bpp→ARGB
                      属性              到 canvas           SDL_UpdateTexture
                                             │
                                     SDL_RenderCopy (4×缩放)
                                     draw_bezel + pixel_grid + key indicators
                                     SDL_RenderPresent
```

monoui 框架源码**零修改**在 PC 上编译运行，唯一的接触点是 HAL flush 回调。
