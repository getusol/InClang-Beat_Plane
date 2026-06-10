cmake_minimum_required(VERSION 3.15)
project(LVGL_Beat_Plane LANGUAGES C)

# 设置 C 标准为 11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 生成compile_commands.json供编辑器使用(可选)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 供用户指定路径
set(LVGL_DIR "" CACHE PATH "Local LVGL path")
set(LV_DRIVERS_DIR "" CACHE PATH "Local LVGL Drivers path")
set(SDL2_PATH "" CACHE PATH "Local SDL2 path(contain SDL2.h and SDL2.lib)")
set(LVGL_CONF_PATH "" CACHE PATH "Local lvgl_conf.h path")
set(LV_DRV_CONF_PATH "" CACHE PATH "Local lv_drv_conf.h path")

# 全局配置
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "Disable LVGL examples" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "Disable LVGL demos" FORCE)


# ==========================================
# 核心：将当前目录（存放 lv_conf.h 和 lv_drv_conf.h 的目录）加入全局头文件搜索路径
# 这样 lvgl 和 lv_drivers 子项目在编译时，都能直接通过 #include "lv_conf.h" 找到配置文件
# ==========================================
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


# ==========================================
# 1. 配置SDL2路径
# ==========================================

if (SDL2_PATH)  # 如果指定了 SDL2_PATH
    list(APPEND CMAKE_PREFIX_PATH ${SDL2_PATH}/include)
endif()

find_package(SDL2 CONFIG QUIET) # 先尝试使用 pkg-config 查找 SDL2
if (NOT SDL2_FOUND)
    find_package(SDL2 QUIET)
endif()

if (NOT SDL2_FOUND) # 如果 pkg-config 和 QUIET 都没有找到 SDL2 尝试手动查找
    message(STATUS "SDL2 Not Found via pkg-config and QUIET,try manual search")

    set(SDL2_SEARCH_HINTS "")
    if (SDL2_PATH)
        set(SDL2_SEARCH_HINTS
            "${SDL2_PATH}"
            "${SDL2_PATH}/include"
            "${SDL2_PATH}/lib"
        )
    endif()

    # 查找 SDL2 头文件
    find_path(SDL2_INCLUDE_DIR
        NAMES SDL.h SDL2/SDL.h
        HINTS ${SDL2_SEARCH_HINTS}
        PATH_SUFFIXES include include/SDL2 SDL2
    )

    # 查找 SDL2 库 (修正 NAME 为 NAMES)
    find_library(SDL2_LIBRARY
        NAMES SDL2
        HINTS ${SDL2_SEARCH_HINTS}
        PATH_SUFFIXES lib lib/x64 lib/x86
    )
    
    # WIN 平台 libSDL2main.lib (修正 NAME 为 NAMES)
    if (WIN32)
        find_library(SDL2MAIN_LIBRARY
            NAMES SDL2main
            HINTS ${SDL2_SEARCH_HINTS}
            PATH_SUFFIXES lib lib/x64 lib/x86
        )
    endif()

    # 检查 SDL2 是否找到
    if(SDL2_INCLUDE_DIR AND SDL2_LIBRARY)
        set(SDL2_FOUND TRUE)
        message(STATUS "Found SDL2 manually:")
        message(STATUS "  - Headers: ${SDL2_INCLUDE_DIR}")
        message(STATUS "  - Library: ${SDL2_LIBRARY}")

        # 手动创建与 find_package 行为一致的现代 CMake 目标：SDL2::SDL2
        if(NOT TARGET SDL2::SDL2)
            add_library(SDL2::SDL2 UNKNOWN IMPORTED)
            set_target_properties(SDL2::SDL2 PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
                IMPORTED_LOCATION "${SDL2_LIBRARY}"
            )
            
            # 如果在 Windows 平台且找到了 SDL2main，将其作为依赖追加
            if(WIN32 AND SDL2MAIN_LIBRARY)
                set_property(TARGET SDL2::SDL2 APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES "${SDL2MAIN_LIBRARY}"
                )
                message(STATUS "  - Main Lib: ${SDL2MAIN_LIBRARY}")
            endif()
        endif()
    else()
        message(FATAL_ERROR 
            "Could not find SDL2.\n"
            "Please install SDL2 on your system, or set 'SDL2_PATH' to the path "
            "containing SDL2 headers and libraries."
        )
    endif()

else()
    message(STATUS "Found SDL2 via find_package: ${SDL2_DIR}")
    # 某些旧版 find_package 寻找完后可能没有创建 SDL2::SDL2 别名，我们这里做一下兼容
    if(NOT TARGET SDL2::SDL2)
        if(TARGET SDL2)
            add_library(SDL2::SDL2 ALIAS SDL2)
        elseif(SDL2_LIBRARIES AND SDL2_INCLUDE_DIRS)
            add_library(SDL2::SDL2 UNKNOWN IMPORTED)
            set_target_properties(SDL2::SDL2 PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
                IMPORTED_LOCATION "${SDL2_LIBRARIES}"
            )
        endif()
    endif()
endif()


# ==========================================
# 2. 设置 lv_conf.h 路径
# ==========================================

if (NOT LVGL_CONF_PATH) # 如果没有指定 LVGL_CONF_PATH
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h)
        set(LVGL_CONF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h)
    endif()
endif()

if (LVGL_CONF_PATH)
    if (NOT EXISTS "${LVGL_CONF_PATH}") # 校验 LVGL_CONF_PATH 是否存在
        message(FATAL_ERROR "LVGL_CONF_PATH does not exist: ${LVGL_CONF_PATH}")
    endif()

    # 映射给 LVGL 官方使用的内部 CMake 变量，使其能提取配置目录
    set(LV_CONF_PATH "${LVGL_CONF_PATH}" CACHE STRING "Official lvgl_conf.h path" FORCE)
 
    message(STATUS "Using LVGL config: ${LVGL_CONF_PATH}")

else()
    # 如果找不到配置文件，LVGL 根本无法编译，直接报错阻断
    message(FATAL_ERROR 
        "Could NOT find 'lv_conf.h' in your project root, and 'LVGL_CONF_PATH' was not specified.\n"
        "LVGL requires this configuration file to compile. Please place 'lv_conf.h' in the root directory."
    )
endif()


# ==========================================
# 3. 设置 lv_drv_conf.h 路径
# ==========================================

if (NOT LV_DRV_CONF_PATH) # 如果没有指定 LV_DRV_CONF_PATH
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lv_drv_conf.h)
        set(LV_DRV_CONF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lv_drv_conf.h)
    endif()
endif()

if (LV_DRV_CONF_PATH)
    if (NOT EXISTS "${LV_DRV_CONF_PATH}")
        message(FATAL_ERROR "LV_DRV_CONF_PATH is set to '${LV_DRV_CONF_PATH}', but this file does not exist!")
    endif()
    
    # 删除之前带双引号的 LV_DRV_CONF_PATH 宏定义，利用全局头文件搜索和简单包含
    message(STATUS "Using LVGL Driver config: ${LV_DRV_CONF_PATH}")
else()
    message(FATAL_ERROR "Could not find 'lv_drv_conf.h' in your project root, and 'LV_DRV_CONF_PATH' was not specified.\n"
        "LVGL requires this configuration file to compile. Please place 'lv_drv_conf.h' in the root directory."
    )
endif()


# ==========================================
# 4. 配置 LVGL 路径并引入
# ==========================================

if (NOT LVGL_DIR) # 如果没有指定 LVGL_DIR
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lvgl)
        set(LVGL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lvgl)
    endif()
endif()

if (LVGL_DIR AND EXISTS "${LVGL_DIR}/CMakeLists.txt")
    message(STATUS "Found LVGL source directory: ${LVGL_DIR}")
    # 禁用 LVGL 自带的样例和演示程序编译
    add_subdirectory(${LVGL_DIR} lvgl)
else()
    message(FATAL_ERROR 
        "LVGL source directory not found!\n"
        "Please place 'lvgl' folder in the project root, or specify 'LVGL_DIR' manually."
    )
endif()


# ==========================================
# 5. 配置 LVGL Drivers 路径并引入
# ==========================================

if (NOT LV_DRIVERS_DIR) # 如果没有指定 LV_DRIVERS_DIR
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lv_drivers)
        set(LV_DRIVERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lv_drivers)
    endif()
endif()

if (LV_DRIVERS_DIR AND EXISTS "${LV_DRIVERS_DIR}/CMakeLists.txt")
    message(STATUS "Found LVGL Drivers source directory: ${LV_DRIVERS_DIR}")
    set(LV_DRIVERS_USE_SDL ON CACHE BOOL "Use SDL2 driver" FORCE)
    add_subdirectory(${LV_DRIVERS_DIR} lv_drivers)
    target_link_libraries(lv_drivers PRIVATE SDL2::SDL2)
else()
    message(FATAL_ERROR 
        "LVGL Drivers source directory not found!\n"
        "Please place 'lv_drivers' folder in the project root, or specify 'LV_DRIVERS_DIR' manually."
    )
endif()


# ==========================================
# 6. 收集项目源文件并生成可执行目标
# ==========================================

# 递归收集 src 下的所有 .c 文件
file (GLOB_RECURSE MY_PROJECT_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# 头文件包含路径
file (GLOB_RECURSE MY_PROJECT_INCS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)
set(MY_PROJECT_INC_DIRS 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
foreach(HEADER_FILE ${MY_PROJECT_INCS})
    get_filename_component(HEADER_DIR ${HEADER_FILE} DIRECTORY)
    list(APPEND MY_PROJECT_INC_DIRS ${HEADER_DIR})
endforeach()
list(REMOVE_DUPLICATES MY_PROJECT_INC_DIRS)

add_executable(${PROJECT_NAME} ${MY_PROJECT_SRCS})

# 设置项目自身的头文件包含路径
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${MY_PROJECT_INC_DIRS}
)

# 链接依赖（LVGL 核心库、驱动库、SDL2）
target_link_libraries(${PROJECT_NAME} PRIVATE 
    lvgl
    lv_drivers
    SDL2::SDL2
)

target_compile_definitions(${PROJECT_NAME} PRIVATE SIMULATOR=1) # 模拟器模式
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)
target_compile_definitions(lv_drivers PUBLIC LV_CONF_INCLUDE_SIMPLE)
