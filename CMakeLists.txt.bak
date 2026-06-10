cmake_minimum_required(VERSION 3.10)
project(LVGL_Simulator LANGUAGES C)

set(CMAKE_C_STANDARD 11)

# ================= 配置路径 =================
# 请根据你的实际路径修改下面这两行！
# 注意：路径中使用正斜杠 / 或者 双反斜杠 \\
set(MINGW_PATH "E:/Apps/mingw64")
set(SDL2_PATH "E:/Apps/mingw64")  # 如果你把 SDL2 复制到了 mingw 目录，就填 mingw 路径

# LVGL 路径
set(LVGL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.environment/lvgl)
set(LV_DRIVERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.environment/lv_drivers)

# ================= 配置 LVGL =================
file(GLOB_RECURSE LVGL_SOURCES "${LVGL_DIR}/src/*.c")
add_library(lvgl STATIC ${LVGL_SOURCES})
target_include_directories(lvgl PUBLIC ${LVGL_DIR})
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) # 包含 lv_conf.h

# ================= 配置 SDL2 (手动指定，不依赖 pkg-config) =================
# 添加 SDL2 头文件路径 (通常包含 SDL2 文件夹)
target_include_directories(lvgl PUBLIC ${MINGW_PATH}/include)
target_include_directories(lvgl PUBLIC ${MINGW_PATH}/include/SDL2)

# ================= 配置 LV Drivers =================
file(GLOB_RECURSE LV_DRIVERS_SOURCES "${LV_DRIVERS_DIR}/sdl/*.c")
add_library(lv_drivers STATIC ${LV_DRIVERS_SOURCES})
target_include_directories(lv_drivers PUBLIC ${LV_DRIVERS_DIR})
target_include_directories(lv_drivers PUBLIC ${MINGW_PATH}/include/SDL2)
target_include_directories(lv_drivers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) # 包含 lv_conf.h

# ================= 创建可执行文件 =================
file(GLOB_RECURSE MY_SOURCE "src/*.c")
add_executable(simulator ${MY_SOURCE})


# ================= 配置多文件编程 ==================
# 1. 递归搜索 src 下所有子目录
file(GLOB_RECURSE ALL_SRC_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src/*)
# 2. 只保留目录路径（过滤文件）
foreach(DIR ${ALL_SRC_DIRS})
    get_filename_component(DIR_PATH ${DIR} DIRECTORY)
    list(APPEND INCLUDE_DIRS ${DIR_PATH})
endforeach()
# 3. 去重 + 添加根目录
list(REMOVE_DUPLICATES INCLUDE_DIRS)
target_include_directories(simulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${INCLUDE_DIRS}
)

# 链接库 (注意顺序：可执行文件 -> 驱动 -> lvgl -> SDL2 库文件)
target_link_libraries(simulator PRIVATE 
    lv_drivers 
    lvgl 
    ${MINGW_PATH}/lib/libSDL2.a 
    ${MINGW_PATH}/lib/libSDL2main.a
)

# 链接系统库 (Windows 下 SDL2 需要这些)
# 【核心修改：补充 setupapi/imm32/cfgmgr32/version 库】
target_link_libraries(simulator PRIVATE 
    mingw32
    m
    dinput8
    dxguid
    winmm
    setupapi   # 解决 SetupDiGetClassDevsA 等设备枚举函数
    imm32      # 解决 ImmGetContext 等输入法相关函数
    cfgmgr32   # 解决 CM_Locate_DevNodeA 等设备管理函数
    version    # 解决 GetFileVersionInfoA 等版本信息函数
)

# 编译定义
target_compile_definitions(simulator PRIVATE SIMULATOR=1)
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)
target_compile_definitions(lv_drivers PUBLIC LV_CONF_INCLUDE_SIMPLE)