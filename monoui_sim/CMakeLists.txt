cmake_minimum_required(VERSION 3.16)
project(monoui_sim C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS OFF)

# ── 路径配置 ──────────────────────────────────────────────────────────────────
# CMakeLists.txt 位于 monoui_sim/，所以默认项目根目录在 ..
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(MONOUI_DIR "${PROJECT_ROOT}/monoui" CACHE PATH
    "Path to the monoui framework source directory")
set(APP_UI_DIR "${PROJECT_ROOT}/app_ui" CACHE PATH
    "Path to the app_ui source directory")

if(NOT IS_ABSOLUTE "${MONOUI_DIR}")
    get_filename_component(MONOUI_DIR "${MONOUI_DIR}" ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(NOT IS_ABSOLUTE "${APP_UI_DIR}")
    get_filename_component(APP_UI_DIR "${APP_UI_DIR}" ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(NOT EXISTS "${MONOUI_DIR}/monoui.h")
    message(FATAL_ERROR
        "MONOUI_DIR is invalid: '${MONOUI_DIR}'. Expected a directory containing monoui.h")
endif()

if(NOT EXISTS "${APP_UI_DIR}/app_ui.h")
    message(FATAL_ERROR
        "APP_UI_DIR is invalid: '${APP_UI_DIR}'. Expected a directory containing app_ui.h")
endif()

# ── SDL2 ──────────────────────────────────────────────────────────────────────
# 优先使用现代 SDL2 CMake config，回退到传统 FindSDL2
find_package(SDL2 CONFIG QUIET)
if(NOT SDL2_FOUND)
    find_package(SDL2 REQUIRED)
endif()

# ── 源文件 ────────────────────────────────────────────────────────────────────
set(MONOUI_SRC
    "${MONOUI_DIR}/ui_canvas.c"
    "${MONOUI_DIR}/ui_anim.c"
    "${MONOUI_DIR}/ui_widget.c"
    "${MONOUI_DIR}/ui_page.c"
    "${MONOUI_DIR}/ui_core.c"
    "${MONOUI_DIR}/ui_action.c"
)

set(APP_UI_SRC
    "${APP_UI_DIR}/app_page.c"
    "${APP_UI_DIR}/app_pages.c"
    "${APP_UI_DIR}/app_ui.c"
    "${APP_UI_DIR}/device/device_status.c"
    "${APP_UI_DIR}/home/home_state.c"
    "${APP_UI_DIR}/home/home_data.c"
    "${APP_UI_DIR}/settings/settings_state.c"
    "${APP_UI_DIR}/settings/settings_data.c"
    "${APP_UI_DIR}/pages/home/page_home.c"
    # 新增页面在 pages/<feature>/ 下继续追加，例如：
    "${APP_UI_DIR}/pages/settings/page_settings.c"
)

set(SIM_SRC
    sim_main.c
    sim_hal.c
    sim_input.c
    app/sim_app.c
)

# ── 可执行文件 ────────────────────────────────────────────────────────────────
add_executable(monoui_sim
    ${SIM_SRC}
    ${APP_UI_SRC}
    ${MONOUI_SRC}
)

# ── 头文件搜索路径 ────────────────────────────────────────────────────────────
target_include_directories(monoui_sim PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"        # sim_hal.h, sim_input.h
    "${CMAKE_CURRENT_SOURCE_DIR}/app"    # sim_app.h
    "${MONOUI_DIR}"                      # monoui.h 及所有 ui_*.h
    "${APP_UI_DIR}"                      # app_ui.h, app_pages.h 等
    "${APP_UI_DIR}/device"               # device_status.h
    "${APP_UI_DIR}/pages"                # pages/<feature>/page_*.h
    "${APP_UI_DIR}/settings"             # settings_data.h, settings_state.h
)

# ── 宏定义 ────────────────────────────────────────────────────────────────────
target_compile_definitions(monoui_sim PRIVATE
    UI_SIMULATOR=1      # 可在业务代码里用 #ifdef UI_SIMULATOR 区分平台
)

# ── 链接 SDL2 ─────────────────────────────────────────────────────────────────
if(TARGET SDL2::SDL2)
    # 现代 CMake config 方式
    target_link_libraries(monoui_sim PRIVATE SDL2::SDL2)
else()
    # 传统 FindSDL2 方式
    target_include_directories(monoui_sim PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(monoui_sim PRIVATE ${SDL2_LIBRARIES})
endif()

# ── 平台差异处理 ──────────────────────────────────────────────────────────────

# Linux: 链接数学库（ui_anim.c 用到 powf / sinf）
if(UNIX AND NOT APPLE)
    target_link_libraries(monoui_sim PRIVATE m)
endif()

# Windows: 把 SDL2.dll 复制到可执行文件旁边
if(WIN32 AND TARGET SDL2::SDL2)
    target_link_options(monoui_sim PRIVATE -mconsole)
    # add_custom_command(TARGET monoui_sim POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #         $<TARGET_FILE:SDL2::SDL2>
    #         $<TARGET_FILE_DIR:monoui_sim>
    #     COMMENT "Copying SDL2.dll to output directory")
endif()

# ── 构建信息输出 ──────────────────────────────────────────────────────────────
message(STATUS "monoui dir : ${MONOUI_DIR}")
message(STATUS "app_ui dir : ${APP_UI_DIR}")
message(STATUS "Build dir  : ${CMAKE_BINARY_DIR}")
