/**
 * @file controller.c
 * @brief SDL2 游戏手柄处理 (PC) / 接收 PC 手柄数据 (MCU)
 *
 * PC 端通过 SDL2 GameController API 读取手柄, 并通过 comm 发送给 MCU。
 * MCU 端通过 comm 接收 PC 发来的手柄数据。
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
#include "comm_tx.h"
#include "protocol.h"
#else
#include "comm_rx.h"
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
typedef struct
{
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

#ifdef SIMULATOR
/** 手柄对象指针 (PC: SDL_GameController*, MCU: 未使用) */
static void *s_controller = NULL;
/** 手柄是否曾连接过 (用于首次检测日志) */
static bool s_was_connected = false;
#else

#endif
/** 4 个手柄按键 (A/B/X/Y) 的状态机 */
static ckey_t ckeys[CKEY_COUNT];
/** 左摇杆当前值 (缩放至 ±JOY_MAX_VALUE) */
static int16_t cjoy_x;
static int16_t cjoy_y;

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

#ifdef SIMULATOR
    s_was_connected = false;
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    /* 打开第一个手柄 (index 0). SDL 自动处理 XInput / DirectInput / HID */
    s_controller = SDL_GameControllerOpen(0);
    if (s_controller)
    {
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
#ifdef SIMULATOR
    /* ---- PC: SDL 读取手柄 + 断线重连 ---- */
    static int s_reconnect_delay = 0; /* 重连冷却计数 (扫描周期) */
    SDL_PumpEvents();
    SDL_GameController *gc = (SDL_GameController *)s_controller;

    /* 检测断开: 关闭旧句柄 */
    if (gc && !SDL_GameControllerGetAttached(gc))
    {
        SDL_GameControllerClose(gc);
        s_controller = NULL;
        s_was_connected = false;
        s_reconnect_delay = 200; /* 约 1 秒后开始重试 (5ms × 200) */
        CONSOLE_INFO("Controller disconnected, will retry...");
    }

    /* 尝试重连 (带冷却, 避免高频 Open/Close) */
    if (!s_controller && s_reconnect_delay > 0)
    {
        s_reconnect_delay--;
    }
    else if (!s_controller)
    {
        s_controller = SDL_GameControllerOpen(0);
        if (s_controller)
        {
            const char *name = SDL_GameControllerName((SDL_GameController *)s_controller);
            CONSOLE_INFO("Controller reconnected: %s", name ? name : "Unknown");
            s_was_connected = true;
        }
        else
        {
            s_reconnect_delay = 200; /* 失败则重新冷却 */
        }
    }

    gc = (SDL_GameController *)s_controller;
    bool connected = (gc && SDL_GameControllerGetAttached(gc));

    if (connected)
    {
        SDL_GameControllerButton map[] = {
            SDL_CONTROLLER_BUTTON_A,
            SDL_CONTROLLER_BUTTON_B,
            SDL_CONTROLLER_BUTTON_X,
            SDL_CONTROLLER_BUTTON_Y,
        };
        for (int i = 0; i < CKEY_COUNT; i++)
        {
            bool stable = SDL_GameControllerGetButton(gc, map[i]) != 0;
            key_state_machine(&ckeys[i].state, &ckeys[i].press_tick,
                              &ckeys[i].pressed_reset_cnt,
                              &ckeys[i].released_reset_cnt, stable);
        }
        cjoy_x = (int16_t)(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX) * JOY_MAX_VALUE / 32767);
        cjoy_y = (int16_t)(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY) * JOY_MAX_VALUE / 32767);

        /* 发送手柄状态给 MCU */
        uint8_t key_mask = 0;
        if (ckeys[0].state.stable)
            key_mask |= COMM_KEY_A_MASK;
        if (ckeys[1].state.stable)
            key_mask |= COMM_KEY_B_MASK;
        if (ckeys[2].state.stable)
            key_mask |= COMM_KEY_X_MASK;
        if (ckeys[3].state.stable)
            key_mask |= COMM_KEY_Y_MASK;
        comm_pc_send_controller_key_state(key_mask);
        comm_pc_send_controller_joystick(cjoy_x, cjoy_y);

        static bool s_first_logged = false;
        if (!s_first_logged && ckeys[0].state.pressed)
        {
            CONSOLE_INFO("Controller A pressed (SDL → MCU)");
            s_first_logged = true;
        }
    }
    else
    {
        for (int i = 0; i < CKEY_COUNT; i++)
            key_state_machine(&ckeys[i].state, &ckeys[i].press_tick,
                              &ckeys[i].pressed_reset_cnt,
                              &ckeys[i].released_reset_cnt, false);
        cjoy_x = 0;
        cjoy_y = 0;
    }
#else
    /* ---- MCU: 接收 PC 发来的手柄数据 ---- */
    uint8_t key_mask = comm_get_controller_key_mask();
    for (int i = 0; i < CKEY_COUNT; i++)
    {
        bool stable = (key_mask >> i) & 1;
        key_state_machine(&ckeys[i].state, &ckeys[i].press_tick,
                          &ckeys[i].pressed_reset_cnt,
                          &ckeys[i].released_reset_cnt, stable);
    }
    cjoy_x = comm_get_controller_joystick_x();
    cjoy_y = comm_get_controller_joystick_y();
#endif
}

bool ckey_pressed(key_code_t key)
{
    if (key < KEY_A || key > KEY_Y)
        return false;
    int i = key - KEY_A;
    if (!ckeys[i].state.pressed)
        return false;
    ckeys[i].state.pressed = false; /* 单次触发, 读取后重置 */
    return true;
}

bool ckey_released(key_code_t key)
{
    if (key < KEY_A || key > KEY_Y)
        return false;
    int i = key - KEY_A;
    if (!ckeys[i].state.released)
        return false;
    ckeys[i].state.released = false;
    return true;
}

bool ckey_down(key_code_t key)
{
    if (key < KEY_A || key > KEY_Y)
        return false;
    return ckeys[key - KEY_A].state.stable;
}

bool ckey_long_press(key_code_t key)
{
    if (key < KEY_A || key > KEY_Y)
        return false;
    return ckeys[key - KEY_A].state.long_press;
}

int16_t cjoystick_get_x(void) { return cjoy_x; }
int16_t cjoystick_get_y(void) { return cjoy_y; }

/**********************
 *   STATIC FUNCTIONS
 **********************/
