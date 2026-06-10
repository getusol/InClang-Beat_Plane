/**
 * @file config.h
 * @brief 全局配置文件，定义游戏的全局常量、宏和配置项 集中便于修改
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

/*=======================
 * DEBUG PARAMS
 *=======================*/

/*-------------
 * GAME INFO
 *-----------*/

#define PERF_MONITOR 1 // 1 开启性能检测 0 关闭

/*-------------
 * CONSOLE
 *-----------*/

// 缺点：修改这个会导致几乎所有文件的重编译
// CONSOLE_ENABLE = 0 时所有 CONSOLE_* 宏编译为空
// (compile-time level filtering)

#define CONSOLE_ENABLE            1   // 主开关
#define CONSOLE_DEBUG_ENABLE      0   // [DEBUG] 详细信息，默认关闭
#define CONSOLE_INFO_ENABLE       0   // [INFO]
#define CONSOLE_WARNING_ENABLE    1   // [WARNING]
#define CONSOLE_ERROR_ENABLE      1   // [ERROR]

/*-------------
 * LOG
 *-----------*/

// LOG_ENABLE = 0 时所有 LOG_* 宏编译为空
// 一般LOG只会在WARNING和ERROR级别打印 所以这里只保留总开关

#define LOG_ENABLE                1   // 主开关

/*=======================
 * HARDWARE PARAMS
 *=======================*/

/*-------------
 * SCREEN
 *-----------*/

#define SCREEN_WIDTH 1024 // 屏幕宽度
#define SCREEN_HEIGHT 600 // 屏幕高度

/*-------------
 * UART COMM
 *-----------*/

#define DEFAULT_BAUD_RATE 115200
#define DEFAULT_COM_PORT "COM11"

#if 1                     // 是否开启高级通信功能
    #define DO_MCU_SEND_CONSOLE 1  // 启用MCU控制台向电脑输出传输
    #define DO_MCU_SEND_INPUT   1  // 启用MCU按键摇杆状态向电脑输出传输
    #define DO_PC_PRINT_CONSOLE 1  // 启用PC控制台打印
#else
    #define DO_MCU_SEND_CONSOLE 0  // 关闭MCU控制台向电脑输出传输
    #define DO_MCU_SEND_INPUT   0  // 关闭MCU按键摇杆状态向电脑输出传输
    #define DO_PC_PRINT_CONSOLE 0  // 关闭PC控制台打印
#endif

/*-------------
 * JOYSTICK
 *-----------*/

/**
 * @brief JOY_MAX_VALUE 决定摇杆输出值的最大值
 * @note JOY_MAX_VALUE 不宜大于 16384
 *       最终输出范围 : -JOY_MAX_VALUE ~ JOY_MAX_VALUE
 */
#define JOY_MAX_VALUE 256

#ifdef SIMULATOR // ON PC

#define JS_DIR_KEY_COUNT 2 // 摇杆一个方向的按键数量，默认2 即 WASD 与 上下左右
#define JS_ACCEL 0.3f      // 决定摇杆的响应速度 越接近0越慢
#define JS_DECAY 0.8f      // 决定摇杆的回落速度 越接近1越慢

#else // ON MCU

#define JOY_DEADZONE 30 // 死区范围（消除微小跳动）
#define FILTER_FACTOR 2 // 滤波系数（2的幂次，便于移位运算）
                        // 越大越平滑，但响应越慢（推荐 4-16）

#endif // #ifdef SIMULATOR

#define JS_THRESHOLD 20 // 控制手柄输入上推阈值(UI交互)

/*-------------
 * KEYBOARD
 *-----------*/

#define LONG_PRESS_MS 150
#define PRESSED_TICKS_THRESHOLD 50
#define RELEASED_TICKS_THRESHOLD 50

#ifdef SIMULATOR // ON PC

#define MAX_BINDING_KEYS_COUNT 2

#else // ON MCU

#endif // #ifdef SIMULATOR

/*=======================
 * GAME PARAMS
 *=======================*/

/*-------------
 * CLOCKS
 *-----------*/

#define SCAN_RATE_MS 5 // 输入扫描频率，单位毫秒
#define GAME_TICK 30   // 游戏逻辑更新频率，单位Hz
#define MAX_FPS 30     // 最大帧率，单位Hz

/*-------------
 * TIMERS
 *-----------*/

#define MAX_TIMER_COUNT 30 // 最大定时器数量

/*-------------
 * AUDIOS
 *-----------*/

/*-------------
 * BULLET
 *-----------*/
#ifdef SIMULATOR // ON PC
        #define MAX_BULLET_COUNT 50 // 最大子弹数量
#else
        #define MAX_BULLET_COUNT 23 // 最大子弹数量
#endif
/*-------------
 * ENEMY
 *-----------*/
#ifdef SIMULATOR // ON PC
        #define MAX_ENEMY_COUNT 10 // 最大敌人数量
#else
        #define MAX_ENEMY_COUNT 6 // 最大敌人数量
#endif

/*-------------
 * COINS
 *-----------*/
#ifdef SIMULATOR // ON PC
        #define MAX_COIN_COUNT 13 // 最大金币数量
#else
        #define MAX_COIN_COUNT 10 // 最大金币数量
#endif

#define MAX_FLAME_WALL_COUNT 3 // 最大火墙数量

/*=======================
 * UI PARAMS
 *=======================*/

/*-------------
 * UI_PLAY
 *-----------*/

#define DP_PLAY_FILL_COLOR 0x252532

#endif // #ifndef __CONFIG_H__
