# MonoUI

面向 `256x64` 灰度 OLED 设备界面的 C99 UI 项目，当前包含三层：

- `monoui`：底层灰度 UI 框架，负责画布、控件、动画、页面转场与事件分发
- `app_ui`：业务页面层，当前已实现设备首页、三层 `Settings`、全局弹窗与 action 路由
- `monoui_sim`：基于 SDL2 的桌面模拟器，用于在 PC 上预览和调试界面

当前仓库已可作为一个 `Alpha` 版本继续迭代，重点方向是桌面型音频解码 / 耳放 / 功放终端的设备 UI。

## 当前特性

- 统一的硬直角 Metro 风格界面
- 首页设备总览页：顶栏状态 + `INPUT -> ASRC -> OUTPUT -> SYSTEM`
- 三层 `Settings` 架构：分类页、设置列表、右侧编辑窗
- 通用 action 路由：物理事件 -> `ui_action_t` -> 页面逻辑
- 全局弹窗覆盖层：确认、提示、告警
- 设备状态预留层：后续可直接替换为真实硬件状态源
- PC 模拟器输入与长按支持，便于本地预览页面行为

## 项目结构

```text
MonoUI/
├── app_ui/       业务页面层
│   ├── device/   设备状态预留层
│   ├── home/     首页状态/数据模型
│   ├── pages/    页面实现入口
│   └── settings/ Settings 状态/描述数据
├── monoui/       底层 UI 框架与 MCU 接入示例
└── monoui_sim/   SDL2 桌面模拟器
```

## 架构关系

```text
硬件/模拟器输入
    -> ui_event_t
    -> ui_core_push_event()   -> widget 树事件分发
    -> app_ui_dispatch()      -> keymap -> ui_action_t -> 页面 on_action
    -> app_ui_tick(delta_ms)
```

其中：

- `monoui` 负责底层能力，不直接关心业务页面
- `app_ui` 负责页面注册、页面状态、业务数据和导航逻辑
- `monoui_sim` 负责在 PC 上提供渲染窗口、输入映射和调试入口

## 快速开始

### 1. 运行桌面模拟器

项目当前最直接的运行入口是 `monoui_sim`。

默认目录结构下，进入 `monoui_sim/` 后执行：

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

生成程序后运行：

- Windows：`monoui_sim.exe`
- Linux / macOS：`./monoui_sim`

更完整的模拟器说明见 [monoui_sim/README.md](file:///d:/Projects/MonoUI/monoui_sim/README.md)。

### 2. 接入 MCU 工程

`monoui/` 目录下已经提供两份更贴近当前架构的模板：

- [example_baremetal_main.c](file:///d:/Projects/MonoUI/monoui/example_baremetal_main.c)：裸机 / super-loop 接入模板
- [example_rtos_task.c](file:///d:/Projects/MonoUI/monoui/example_rtos_task.c)：RTOS 任务接入模板

说明入口见 [example_main.c](file:///d:/Projects/MonoUI/monoui/example_main.c)。

## 关键目录说明

### `monoui/`

- 4bpp 灰度画布
- 控件树与事件分发
- 页面栈与转场
- 动画系统
- 单头入口 `monoui.h`

底层框架说明见 [monoui/README.md](file:///d:/Projects/MonoUI/monoui/README.md)。

### `app_ui/`

- `app_page` / `app_pages`：页面绑定与统一注册表
- `pages/home` / `pages/settings`：页面入口实现
- `home` / `settings`：页面状态与描述数据
- `device`：设备状态接口预留层

### `monoui_sim/`

- SDL2 窗口渲染
- 键盘 / 鼠标到 `ui_event_t` 的输入映射
- 桌面侧页面预览与交互验证

## 当前状态

- 已完成 Alpha 阶段核心分层整理
- 已建立页面注册表与统一导航入口
- 已将首页与设置页从硬编码占位值抽到设备状态层
- 已补齐裸机与 RTOS 的 MCU 接入示例
- 当前更适合继续做真实设备状态接入、持久化、硬件驱动联调与业务页面细化

## 说明

- 目前仓库内没有正式自动化测试体系，主要验证方式是桌面模拟器
- `build/` 与 `REFERENCE/` 已按仓库约定忽略，不进入版本控制
- 若你主要关注某个子模块，建议优先阅读对应子目录 README

