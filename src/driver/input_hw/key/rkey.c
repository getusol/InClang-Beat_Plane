/**
 * @file rkey.c
 * @brief 远程按键处理 (remote key)
 *        PC: 从 comm_rx 读取对端按键掩码, 经过边沿检测/长按状态机
 *        MCU: stub — comm 尚未支持 PC→MCU 按键传输, 所有查询返回 false
 */

/*********************
 *      INCLUDES
 *********************/
#include "rkey.h"
#include "key_core.h"
#include "config.h"
#include <string.h>
#ifdef SIMULATOR
#include "comm_rx.h"
#endif

/**********************
 *      MACROS
 **********************/

#define RKEY_COUNT 4  // KEY_A .. KEY_Y

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief 远程按键条目 — 比 key_t 轻量 (无需 scancode / GPIO 绑定)
 */
typedef struct {
    key_state_t state;
    uint16_t press_tick;
    uint8_t pressed_reset_cnt;
    uint8_t released_reset_cnt;
} rkey_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static rkey_t rkeys[RKEY_COUNT];

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化远程按键状态
 */
void rkey_init(void)
{
    memset(rkeys, 0, sizeof(rkeys));
}

/**
 * @brief 扫描远程按键 — 从 comm_rx 读取掩码, 更新所有 rkey 状态机
 * @note PC: 读 comm_get_key_mask(), MCU: stable 恒为 false
 */
void rkey_scan(void)
{
    for (int i = 0; i < RKEY_COUNT; i++) {
        bool stable = false;
#ifdef SIMULATOR
        uint8_t mask = comm_get_key_mask();
        stable = (mask >> i) & 1;
#endif
        key_state_machine(&rkeys[i].state, &rkeys[i].press_tick,
                          &rkeys[i].pressed_reset_cnt, &rkeys[i].released_reset_cnt, stable);
    }
}

/**
 * @brief 检测远程按键是否按下（单次触发）
 */
bool rkey_pressed(key_code_t key)
{
    if (key < KEY_A || key > KEY_Y) return false;
    int i = (int)(key - KEY_A);
    if (!rkeys[i].state.pressed) return false;
    rkeys[i].state.pressed = false;
    return true;
}

/**
 * @brief 检测远程按键是否松开（单次触发）
 */
bool rkey_released(key_code_t key)
{
    if (key < KEY_A || key > KEY_Y) return false;
    int i = (int)(key - KEY_A);
    if (!rkeys[i].state.released) return false;
    rkeys[i].state.released = false;
    return true;
}

/**
 * @brief 检测远程按键是否按下（处于按下状态）
 */
bool rkey_down(key_code_t key)
{
    if (key < KEY_A || key > KEY_Y) return false;
    return rkeys[key - KEY_A].state.stable;
}

/**
 * @brief 检测远程按键是否长按（处于长按状态）
 */
bool rkey_long_press(key_code_t key)
{
    if (key < KEY_A || key > KEY_Y) return false;
    return rkeys[key - KEY_A].state.long_press;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
