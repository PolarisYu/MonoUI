# MonoUI

`MonoUI` 是一个面向 `256x64` 灰度 OLED 设备的 C99 UI 项目，当前主要服务于桌面型音频解码 / 耳放 / 功放终端这一类设备。

项目不追求通用手机 App 式的 UI，而是强调以下特征：

- 硬直角
- 平面化
- 强对齐
- 少装饰
- 小屏信息密度可控
- 像专业设备而不是消费类 App

当前仓库已经具备 Alpha 阶段可持续迭代的基础，既可以用 `monoui_sim` 做桌面预览，也可以按现有模板接入 MCU 工程。

## 项目目标

这个仓库当前的重点不是“做一个通用 GUI 库”，而是建立一套适合嵌入式音频设备的完整 UI 工作方式：

- 底层框架层负责渲染、事件、动画、页面栈
- 业务页面层负责页面结构、导航、设置项逻辑、状态读取
- 状态接口层负责把真实硬件、驱动、SPI、自检、存储等能力整理成 UI 可直接读取的数据

换句话说，`UI` 不应该直接知道寄存器、SPI 包格式、Flash 细节、ISR 细节，而应该通过中间接口层获取状态和提交动作。

## 总体架构

仓库当前分为三层：

- `monoui`
  底层灰度 UI 框架，负责画布、控件树、动画、页面转场、事件分发、页面管理
- `app_ui`
  业务页面层，负责页面注册、导航、弹窗、状态抽象、具体页面逻辑
- `monoui_sim`
  SDL2 桌面模拟器，用于 PC 上预览、调试、演示和联调 UI

整体关系如下：

```text
物理按键 / 编码器 / 模拟器输入
        -> ui_event_t
        -> ui_core_push_event()
        -> ui_core_tick(delta_ms)
              |- widget 树事件分发
              |- 页面渲染 / 转场
        -> app_ui_dispatch()
              -> keymap
              -> ui_action_t
              -> 当前页面 on_action
        -> app_ui_tick(delta_ms)
              |- 页面逻辑 tick
              |- Boot / Home 等页面的持续更新
```

分层职责建议严格遵守：

- `monoui` 不直接关心设备业务
- `app_ui` 不直接操作硬件总线
- 硬件 / 驱动层不要直接改 UI 控件，而是更新中间状态层

## 目录结构

```text
MonoUI/
├── app_ui/
│   ├── boot/       Boot 状态接口层
│   ├── device/     设备状态接口层
│   ├── home/       首页状态/数据模型
│   ├── pages/      页面实现
│   │   ├── boot/
│   │   ├── home/
│   │   ├── nav/
│   │   └── settings/
│   └── settings/   Settings 状态与描述数据
├── monoui/         底层 UI 框架与 MCU 接入示例
└── monoui_sim/     SDL2 桌面模拟器
```

几个关键文件：

- [app_ui.h](app_ui/app_ui.h)
  `app_ui` 总入口，统一管理页面初始化、tick、页面跳转和弹窗
- [app_pages.h](app_ui/app_pages.h)
  页面 ID 和注册表定义
- [page_home.h](app_ui/pages/home/page_home.h)
  首页页面入口
- [page_settings.h](app_ui/pages/settings/page_settings.h)
  Settings 页面入口与 section 跳转接口
- [device_status.h](app_ui/device/device_status.h)
  设备运行态统一读取接口
- [boot_status.h](app_ui/boot/boot_status.h)
  启动自检状态统一读取接口
- [example_main.c](monoui/example_main.c)
  MCU 接入说明入口

## 页面体系

当前页面注册表中包含以下页面：

- `APP_PAGE_BOOT`
  开机初始化 / 自检页
- `APP_PAGE_HOME`
  设备主页 / 总览页
- `APP_PAGE_NAV`
  导航页，用于跳转 Home、Settings 和后续功能页
- `APP_PAGE_SETTINGS`
  一级 Settings 根页
- `APP_PAGE_SETTINGS_SECTION`
  二级 Settings 内容页

页面构建由 `app_pages` 统一注册，`app_ui_init()` 初始化后会统一 build，并默认推入开机页。

## UI 工作方式

### 1. 页面不是直接散落的

每个页面都应通过 `app_page` 这层统一绑定，而不是手写底层 page manager 接线。

统一绑定的好处：

- 页面生命周期统一
- keymap 统一
- action 路由统一
- 后续更容易做配置化页面注册

### 2. 输入不是直接写死在页面里

输入推荐统一变成 `ui_event_t`，再经过：

- `ui_core_push_event()` 进入底层事件系统
- `app_ui_dispatch()` 转成 `ui_action_t`
- 当前页面根据 action 响应

这种做法的好处是：

- 模拟器、按键、编码器、遥控器都可以统一接入
- 页面只关心“左、右、确认、取消、长按”等动作，不关心底层输入来源

### 3. 页面持续逻辑用 tick，不要塞进中断

例如：

- Boot 启动状态刷新
- Home 空闲隐藏高亮
- 将来设备状态轮询后的页面刷新

都应该在 `app_ui_tick(delta_ms)` 这一轮里发生，而不是在 ISR 里直接改页面控件。

## 快速开始

### 运行桌面模拟器

项目当前最直接的运行入口是 `monoui_sim`。

在 `monoui_sim/` 下执行：

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

生成程序后运行：

- Windows：`monoui_sim.exe`
- Linux / macOS：`./monoui_sim`

更完整的模拟器说明见 [monoui_sim/README.md](monoui_sim/README.md)。

### 接入 MCU 工程

`monoui/` 已提供两份和当前架构一致的模板：

- [example_baremetal_main.c](monoui/example_baremetal_main.c)
  裸机 / super-loop 模板
- [example_rtos_task.c](monoui/example_rtos_task.c)
  RTOS 任务模型模板

说明入口见 [example_main.c](monoui/example_main.c)。

最小接线原则如下：

```c
ui_core_init(...);
app_ui_init();

for (;;) {
    uint32_t delta_ms = platform_tick_delta();

    /* 把物理输入整理成 ui_event_t 后喂进去 */
    ui_core_push_event(&evt);
    app_ui_dispatch(&evt);

    /* 每轮驱动 UI */
    app_ui_tick(delta_ms);
}
```

几个原则：

- 先 `ui_core_init()`，再 `app_ui_init()`
- ISR 中不要直接操作页面逻辑
- 物理输入先整理成 `ui_event_t`
- 页面跳转统一通过 `app_ui_push_id()` / `app_ui_replace_id()` / `app_ui_pop()`

## app_ui 层的职责

`app_ui` 是整个业务 UI 的实际入口，当前负责：

- 页面注册和 build
- 页面 ID 到页面实例的统一映射
- 统一导航接口
- 全局 Popup 覆盖层
- 页面级 tick 分发
- 页面和业务状态层之间的连接

常用接口见 [app_ui.h](app_ui/app_ui.h)：

- `app_ui_init()`
- `app_ui_tick()`
- `app_ui_push_id()`
- `app_ui_replace_id()`
- `app_ui_pop()`
- `app_ui_current_page_id()`
- `app_ui_popup_show()`
- `app_ui_popup_hide()`
- `app_ui_dispatch()`

### 页面跳转建议

- 开机完成后跳 Home：
  `app_ui_replace_id(APP_PAGE_HOME, UI_TRANS_FADE, 240);`
- 从 Home 进入导航页：
  `app_ui_push_id(APP_PAGE_NAV, UI_TRANS_SLIDE_LEFT, 220);`
- 从任意页返回上一页：
  `app_ui_pop(UI_TRANS_SLIDE_RIGHT, 220);`

## 各页面如何使用

### Boot 页面

Boot 页是开机初始化 / 自检页，不是普通的 loading 动画页。

当前结构：

- 左侧纵向阶段块
- 右侧当前步骤标题
- 右侧状态说明
- 横向进度条

用途：

- 让用户知道设备正在启动，而不是卡死
- 显示当前启动阶段和进度
- 为将来真实硬件自检、等待状态和错误态预留显示入口

当前页面文件：

- [page_boot.c](app_ui/pages/boot/page_boot.c)

### Home 页面

Home 页是设备主状态总览页，不是设置页。

当前信息结构：

- 顶栏显示设备名、电源模式、温度、时间
- 主区域显示 `INPUT -> ASRC -> OUTPUT`
- 左侧提供 `NAV` 竖条入口
- 焦点可进入输入、ASRC、输出和导航入口

这个页面应优先展示“设备当前状态”，而不是让用户在这里做复杂编辑。

当前页面文件：

- [page_home.c](app_ui/pages/home/page_home.c)

### Nav 页面

Nav 页是统一导航入口，用于从首页或其他入口跳到几个核心页面。

建议用途：

- Home
- Settings
- 将来的播放页、诊断页、信息页

当前页面文件：

- [page_nav.c](app_ui/pages/nav/page_nav.c)

### Settings 页面

Settings 是当前项目最复杂、也最重要的页面系统。

当前结构不是传统的整页跳来跳去，而是分为三层：

- 一级根分类页
- 二级设置列表
- 三级右侧局部编辑器

已经支持的设置项类型：

- 开关
- 多选一
- 范围调节 / 步进值
- 动作项
- 只读项

当前设计特点：

- 一级列表和二级列表都尽量填满右侧，不浪费屏幕
- 二级打开三级编辑器时，编辑器从右侧推入
- 二级列表右侧同步缩进
- 左侧纵向标题栏表现层级
- Popup 可覆盖所有页面，作为最高优先级提示层

当前页面入口：

- [page_settings.c](app_ui/pages/settings/page_settings.c)
- [page_settings.h](app_ui/pages/settings/page_settings.h)

## 页面与真实功能如何对接

这是本项目最重要的一条原则：

**页面不直接碰真实硬件接口，而是通过中间状态层或动作应用层对接。**

### Home 如何对接真实设备状态

Home 不应该自己去读：

- SPI
- I2C
- ADC
- RTC 寄存器
- 电源芯片状态

而应该统一通过 [device_status.h](app_ui/device/device_status.h) 提供的接口读取。

当前 `device_status_snapshot_t` 已包含：

- 设备名
- 系统摘要
- 输入格式
- 固件版本
- 电源模式
- PD 电压
- 温度
- 时间
- 输入锁定状态
- ASRC 锁定状态

推荐接线方式：

```c
device_status_snapshot_t snapshot;
device_status_get_snapshot(&snapshot);
```

然后由页面只负责显示。

以后真实硬件接入时，优先修改 `device_status.c` 的实现，把这些字段改成读取你的驱动或系统状态缓存，而不是去改页面文件。

### Settings 如何对接真实功能

Settings 当前已经拆分成两层核心数据：

- `settings_data`
  设置项描述表，负责定义每一项是什么、属于什么类型、如何显示
- `settings_state`
  设置值本体，负责实际状态存储与变更

这种分法的目标是：

- 增加设置项时尽量不改渲染逻辑
- 把“UI 怎么画”和“值怎么存”分开

推荐对接方式：

1. 页面操作某一项
2. 通过 `settings_state` 或其 apply 逻辑修改业务值
3. 业务层再去驱动实际硬件函数

例如：

- 开关项修改某个 `bool`
- 选择项修改某个枚举值
- 进度项修改某个数值
- 动作项调用某个 apply 回调

后续接真实功能时，不建议在页面里直接写：

- `spi_write_xxx()`
- `dac_set_xxx()`
- `amp_enable_xxx()`

更推荐写成：

```c
static void apply_output_gain(int value) {
    g_settings_state.output.volume = value;
    audio_service_set_output_gain(value);
}
```

也就是：

- 页面只触发“变更”
- `settings_state` 负责值
- 具体功能服务层负责真正调用硬件函数

### Boot 如何对接真实自检流程

Boot 页对应的不是普通业务状态，而是启动自检状态。

当前已经新增了专门的接口层：

- [boot_status.h](app_ui/boot/boot_status.h)
- [boot_status.c](app_ui/boot/boot_status.c)

`boot_status_snapshot_t` 目前包含：

- `state`
- `stage`
- `progress`
- `stage_label`
- `title`
- `detail`
- `error_code`
- `stage_ok[]`

这层的设计目的就是为了让 Boot 页不直接依赖真实硬件总线。

#### 如果你的自检状态来自 SPI

推荐做法：

1. 底层驱动或启动管理器主动轮询 SPI
2. 解析当前自检阶段、错误码、进度和状态文案
3. 更新 `boot_status_snapshot_t`
4. Boot 页只调用 `boot_status_get_snapshot()`

当前已预留：

- `boot_status_set_snapshot()`
- `boot_status_set_simulation_enabled(false)`

一个典型对接方式可以是：

```c
boot_status_snapshot_t snapshot = {0};

snapshot.state = BOOT_STATUS_RUNNING;
snapshot.stage = BOOT_STATUS_STAGE_AUDIO;
snapshot.progress = 62;
snapshot.stage_label = "AUDIO";
snapshot.title = "ASRC INIT";
snapshot.detail = "WAIT INPUT PLL LOCK";
snapshot.error_code = 0;
snapshot.stage_ok[0] = true;
snapshot.stage_ok[1] = true;
snapshot.stage_ok[2] = false;
snapshot.stage_ok[3] = false;
snapshot.stage_ok[4] = false;

boot_status_set_simulation_enabled(false);
boot_status_set_snapshot(&snapshot);
```

推荐注意：

- UI 页面不要直接访问 SPI
- SPI 轮询和协议解析应放在驱动层或 `boot_manager`
- `boot_status` 作为 UI 读取层存在

后续建议继续补一个 `boot_manager` 层，职责为：

- 轮询启动自检项
- 维护启动状态机
- 统一更新 `boot_status`

## 如何新增一个页面

推荐步骤：

1. 在 `app_ui/pages/<feature>/` 下新增 `page_xxx.c/.h`
2. 在页面内 build 自己的 widget 树
3. 用 `app_page` 绑定页面、keymap、on_action、生命周期
4. 在 `app_pages.h` 中增加页面 ID
5. 在 `app_pages.c` 中注册页面
6. 在需要的地方通过 `app_ui_push_id()` / `replace_id()` 跳转
7. 如果页面依赖真实状态，优先先补状态层，不要在页面里直接写硬件读写

## 如何新增一个设备状态字段

以“新增耳机插入状态”为例，建议步骤如下：

1. 在 `device_status_snapshot_t` 中增加字段
2. 在 `device_status.c` 中实现该字段的来源
3. 让页面读取 `device_status_get_snapshot()`
4. 在页面中只处理显示方式，不直接处理硬件读取

这样做好处是：

- 页面逻辑更干净
- 硬件实现替换成本低
- 模拟器可以先给占位实现

## 如何新增一个设置项

推荐流程：

1. 在 `settings_state` 里新增对应值
2. 在 `settings_data` 中新增描述项
3. 如果需要真实功能联动，新增 apply 函数
4. 页面会按现有渲染逻辑自动呈现对应类型

这样做可以尽量避免每新增一项都去改 `page_settings.c` 的渲染结构。

## 当前页面与状态层关系

```text
Boot page
    -> boot_status
    -> 未来可由 boot_manager / SPI 自检服务更新

Home page
    -> device_status
    -> 设备运行态摘要

Settings page
    -> settings_data
    -> settings_state
    -> 必要时调用业务 apply/service
```

这个关系建议长期保持，不要让页面层重新回到“直接碰硬件”的写法。

## 当前 UI 风格约定

后续新增页面时尽量继续遵循现有风格：

- 硬直角
- 平面化
- 尽量使用纯块面和线性分区
- 减少圆角和装饰
- 避免底部大段按键提示占空间
- 小屏优先保证信息节奏和可读性

## 当前状态

当前仓库已完成这些关键工作：

- 事件队列已从单槽改为 FIFO 环形队列
- 页面注册、统一导航和 action 路由已建立
- Home 已演进为设备总览页
- Settings 已演进为三层复杂配置页
- Popup 覆盖层已接入
- 页面实现已整理到 `app_ui/pages/...`
- 设备状态统一入口 `device_status` 已建立
- 启动状态统一入口 `boot_status` 已建立
- Boot 页已接入默认启动流程
- 裸机和 RTOS 示例模板已补齐

## 建议阅读顺序

如果你是第一次接手这个项目，推荐按下面顺序看：

1. 本 README
2. [app_ui.h](app_ui/app_ui.h)
3. [app_pages.h](app_ui/app_pages.h)
4. [device_status.h](app_ui/device/device_status.h)
5. [boot_status.h](app_ui/boot/boot_status.h)
6. [page_home.c](app_ui/pages/home/page_home.c)
7. [page_settings.c](app_ui/pages/settings/page_settings.c)
8. [example_main.c](monoui/example_main.c)

## 说明

- 当前仓库没有正式自动化测试体系，主要验证方式是桌面模拟器和静态检查
- `build/` 与 `REFERENCE/` 已忽略，不进入版本控制
- 如果你只关心底层框架，请继续阅读 [monoui/README.md](monoui/README.md)
- 如果你要做桌面模拟器输入映射或运行方式调整，请继续阅读 [monoui_sim/README.md](monoui_sim/README.md)
