/**
 * @file key_core.h
 * @brief 公共按键状态机, 被 lkey.c 和 rkey.c 共用
 */

#ifndef __KEY_CORE_H__
#define __KEY_CORE_H__

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief 按键状态结构体，纪录按钮是按下还是长按状态
 */
typedef struct {
    bool pressed;
    bool released;
    bool long_press;
    bool stable;
    bool last;
} key_state_t;

/**********************
 *  STATIC FUNCTIONS (inline)
 **********************/

/**
 * @brief 公共按键状态机：边沿检测 + 边沿保持 + 长按检测
 * @param state       按键状态 (in/out)
 * @param press_tick  长按计时器 (in/out)
 * @param pressed_cnt 按下边沿保持计数器 (in/out)
 * @param released_cnt 释放边沿保持计数器 (in/out)
 * @param cur_stable  当前消抖/读取后的稳定电平
 * @note 消除 lkey.c 和 rkey.c 之间的重复逻辑
 */
static inline void key_state_machine(key_state_t *state, uint16_t *press_tick,
                                     uint8_t *pressed_cnt, uint8_t *released_cnt,
                                     bool cur_stable)
{
    /* ---- 1. 边沿检测 ---- */
    bool raw_pressed  = !state->last && cur_stable;
    bool raw_released = state->last && !cur_stable;

    if (raw_pressed)  { state->pressed  = true; *pressed_cnt  = 0; }
    if (raw_released) { state->released = true; *released_cnt = 0; }

    /* ---- 2. 边沿保持 (让 30Hz 的 game tick 能捕获到) ---- */
    if (state->pressed) {
        *pressed_cnt += SCAN_RATE_MS;
        if (*pressed_cnt >= PRESSED_TICKS_THRESHOLD) state->pressed = false;
    }
    if (state->released) {
        *released_cnt += SCAN_RATE_MS;
        if (*released_cnt >= RELEASED_TICKS_THRESHOLD) state->released = false;
    }

    state->last = cur_stable;

    /* ---- 3. 长按检测 ---- */
    if (cur_stable) {
        *press_tick += SCAN_RATE_MS;
        if (*press_tick >= LONG_PRESS_MS) state->long_press = true;
    } else {
        *press_tick = 0;
        state->long_press = false;
    }

    state->stable = cur_stable;
}

#endif /* __KEY_CORE_H__ */
