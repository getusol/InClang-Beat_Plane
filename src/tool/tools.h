/**
 * @file tools.h
 */
#ifndef __TOOLS_H__
#define __TOOLS_H__

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "debug.h"

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief 非阻塞延时回调函数与对应计时器结构体
 */
typedef struct {
    void (*func)(void * v);         // 回调函数指针
    uint32_t (*tick_get)(); // 获取系统时间戳的函数指针，单位毫秒
    uint32_t delay_ms;      // 延时毫秒数
    uint32_t last_tick;     // 上次调用的系统时间戳
    void * usr_data;        // 用户数据
} non_blocking_timer_t;

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void *ram_malloc(size_t xWantedSize);
void ram_free(void *mem);
void delay_ms(uint32_t ms);
void non_blocking_delay(non_blocking_timer_t *timer,void * v,bool force);
int32_t max(int32_t a, int32_t b); 
void tools_init();
void direction_to_velocity(int16_t dx, int16_t dy,int8_t speed,int16_t *vx, int16_t *vy);
uint16_t vec_length(int16_t x, int16_t y);
uint32_t play_tick_get();

#endif // #ifndef __TOOLS_H__

