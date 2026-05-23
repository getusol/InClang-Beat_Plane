/**
 * @file timer.h
 */

#ifndef __TIMER_H__
#define __TIMER_H__

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>
#include <stdbool.h>
#include "game_object.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

// 回调函数类型
typedef void (*timer_callback_t)(game_obj_t * owner,void *usr_data);

// 计时器模式
typedef enum
{
    TIMER_MODE_ONCE = 0,   // 单次触发
    TIMER_MODE_REPEAT = 1, // 周期触发
} timer_mode_t;

// 计时器结构体
typedef struct timer_t timer_t;

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void timer_init(void);
timer_t *timer_create(game_obj_t * owner,uint32_t interval_ms, timer_mode_t mode,
                      timer_callback_t callback, void *usr_data);
void timer_update();

#endif // #ifndef __TIMER_H__
