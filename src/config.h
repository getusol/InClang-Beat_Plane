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

#define SHOW_HITBOX 1  // 是否显示碰撞框
#define PERF_MONITOR 1 // 1 开启性能检测 0 关闭

/*=======================
 * HARDWARE PARAMS
 *=======================*/

/*-------------
 * SCREEN
 *-----------*/

#define SCREEN_WIDTH 1024 // 屏幕宽度
#define SCREEN_HEIGHT 600 // 屏幕高度

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
#define ACCEL 0.3f         // 决定摇杆的响应速度 越接近0越慢
#define DECAY 0.8f         // 决定摇杆的回落速度 越接近1越慢

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
 * BULLET
 *-----------*/

#define MAX_BULLET_COUNT 15 // 最大子弹数量

/*-------------
 * ENEMY
 *-----------*/

#define MAX_ENEMY_COUNT 12 // 最大敌人数量

#endif // #ifndef __CONFIG_H__
