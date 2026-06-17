/**
 * @file controller.c
 * @brief SDL2 游戏手柄处理 (PC) / stub (MCU)
 *
 * PC 端通过 SDL2 GameController API 读取手柄 (Xbox/PS/DirectInput 全兼容),
 * 将 A/B/X/Y 映射到虚拟按键, 左摇杆映射到模拟轴。
 * MCU 端所有查询返回 false/0 — 未来可通过 comm 接收 PC 手柄数据。
 */

/*********************
 *      INCLUDES
 *********************/
#include "controller.h"
#include "key_core.h"
#include "config.h"
#include <string.h>
#ifdef SIMULATOR
#include "SDL.h"
#include "debug.h"
#endif

/**********************
 *      MACROS
 **********************/

/** 手柄按键数量 (A/B/X/Y) */
#define CKEY_COUNT 4

/**********************
 *      TYPEDEFS
 **********************/

/** 手柄按键条目 — 复用 key_core 状态机 */
typedef struct {
    key_state_t state;
    uint16_t press_tick;
    uint8_t pressed_reset_cnt;
    uint8_t released_reset_cnt;
} ckey_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/** 手柄对象指针 (PC: SDL_GameController*, MCU: 未使用) */
static void *s_controller = NULL;
/** 4 个手柄按键 (A/B/X/Y) 的状态机 */
static ckey_t ckeys[CKEY_COUNT];
/** 左摇杆当前值 (缩放至 ±JOY_MAX_VALUE) */
static int16_t cjoy_x;
static int16_t cjoy_y;
/** 手柄是否曾连接过 (用于首次检测日志) */
static bool s_was_connected = false;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化手柄 — 尝试打开第一个可用的 SDL 手柄
 */
void ckey_init(void)
{
    memset(ckeys, 0, sizeof(ckeys));
    cjoy_x = 0;
    cjoy_y = 0;
    s_was_connected = false;

#ifdef SIMULATOR
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    /* 打开第一个手柄 (index 0). SDL 自动处理 XInput / DirectInput / HID */
    s_controller = SDL_GameControllerOpen(0);
    if (s_controller) {
        const char *name = SDL_GameControllerName((SDL_GameController *)s_controller);
        CONSOLE_INFO("Controller opened: %s", name ? name : "Unknown");
        s_was_connected = true;
    }
#endif
}

/**
 * @brief 扫描手柄 — 读取按键和摇杆状态
 * @note PC: SDL_GameControllerGetButton / SDL_GameControllerGetAxis
 *       MCU: 不执行任何操作 (状态保持为 0)
 */
void ckey_scan(void)
{
    bool connected = false;
#ifdef SIMULATOR
    /* SDL_GameControllerGetButton 依赖事件泵更新内部状态,
       而项目中 SDL_PollEvent 未被调用, 需要手动 PumpEvents */
    SDL_PumpEvents();
    SDL_GameController *gc = (SDL_GameController *)s_controller;
    if (gc && SDL_GameControllerGetAttached(gc)) {
        connected = true;

        /* 读取按键: A→KEY_A, B→KEY_B, X→KEY_X, Y→KEY_Y */
        SDL_GameControllerButton map[] = {
            SDL_CONTROLLER_BUTTON_A,
            SDL_CONTROLLER_BUTTON_B,
            SDL_CONTROLLER_BUTTON_X,
            SDL_CONTROLLER_BUTTON_Y,
        };
        for (int i = 0; i < CKEY_COUNT; i++) {
            bool stable = SDL_GameControllerGetButton(gc, map[i]) != 0;
            key_state_machine(&ckeys[i].state, &ckeys[i].press_tick,
                              &ckeys[i].pressed_reset_cnt,
                              &ckeys[i].released_reset_cnt, stable);
        }

        /* 左摇杆: SDL 值 ±32768 → JOY_MAX_VALUE (±256) */
        cjoy_x = (int16_t)(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX)
                           * JOY_MAX_VALUE / 32767);
        cjoy_y = (int16_t)(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY)
                           * JOY_MAX_VALUE / 32767);

        /* 诊断: 检测首次按键 */
        static bool s_first_press_logged = false;
        if (!s_first_press_logged && ckeys[0].state.pressed) {
            CONSOLE_INFO("Controller A button pressed (SDL)");
            s_first_press_logged = true;
        }
    } else {
        if (s_was_connected) {
            CONSOLE_INFO("Controller disconnected");
            s_was_connected = false;
        }
    }
#endif
    if (!connected) {
        /* 未连接或 MCU: 所有按键喂 false → 状态机输出全 0 */
        for (int i = 0; i < CKEY_COUNT; i++) {
            key_state_machine(&ckeys[i].state, &ckeys[i].press_tick,
                              &ckeys[i].pressed_reset_cnt,
                              &ckeys[i].released_reset_cnt, false);
        }
        cjoy_x = 0;
        cjoy_y = 0;
    }
}

bool ckey_pressed(key_code_t key)
{
    if (key < KEY_A || key > KEY_Y) return false;
    int i = key - KEY_A;
    if (!ckeys[i].state.pressed) return false;
    ckeys[i].state.pressed = false; /* 单次触发, 读取后重置 */
    return true;
}

bool ckey_released(key_code_t key)
{
    if (key < KEY_A || key > KEY_Y) return false;
    int i = key - KEY_A;
    if (!ckeys[i].state.released) return false;
    ckeys[i].state.released = false;
    return true;
}

bool ckey_down(key_code_t key)
{
    if (key < KEY_A || key > KEY_Y) return false;
    return ckeys[key - KEY_A].state.stable;
}

bool ckey_long_press(key_code_t key)
{
    if (key < KEY_A || key > KEY_Y) return false;
    return ckeys[key - KEY_A].state.long_press;
}

int16_t cjoystick_get_x(void) { return cjoy_x; }
int16_t cjoystick_get_y(void) { return cjoy_y; }

/**********************
 *   STATIC FUNCTIONS
 **********************/
