/**
 * @file tools.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "tools.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "fsm.h"

#include "lvgl.h" // only for lv_tick_get()



#ifdef SIMULATOR
#include <SDL.h>

#else
#include "sdram_malloc.h"
#include "drivers.h"
#endif // #ifdef SIMULATOR

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 内存分配 MCU sdram，PC malloc
 */
void * ram_malloc(size_t xWantedSize)
{
    #ifdef SIMULATOR
    return malloc(xWantedSize);
    #else
    return sdram_malloc(xWantedSize);
    #endif
}

/**
 * @brief 内存释放 MCU sdram PC malloc
 */
void ram_free(void * mem)
{
    #ifdef SIMULATOR
    free(mem);
    #else
    sdram_free(mem);
    #endif
}

/**
 * @brief 毫秒级延时函数，PC使用SDL_Delay，MCU使用循环调用delay_us实现
 * @param ms 延时的毫秒数
 */
void delay_ms(uint32_t ms)
{
    #ifdef SIMULATOR
    SDL_Delay(ms);
    #else
    while (ms--)
    {
        delay_us(1000);
    }
    #endif //#ifdef SIMULATOR
}

/**
 * @brief 非阻塞式延时函数，定时调用某个函数
 * @param timer 非阻塞延时回调函数与对应计时器结构体
 */
void non_blocking_delay(non_blocking_timer_t *timer)
{
    uint32_t now_tick = timer->tick_get();
    if (now_tick - timer->last_tick > timer->delay_ms)
    {
        timer->last_tick = now_tick;
        timer->func();
    }
    return ;
}

/**
 * @brief 返回两个值间的最大值
 * @param a 第一个值 int32_t
 * @param b 第二个值 int32_t
 * @return 两个值中的较大者 int32_t
 */
int32_t max(int32_t a, int32_t b)
{
    return (a > b) ? a : b;
}

/**
 * @brief 工具统一初始化函数入口
 */
void tools_init()
{
    debug_init();
    console_out("[tools] tools_init called\n");
}

/**
 * @brief 将方向向量转换为速度向量,速度向量模长为speed
 * @param dx 方向向量x分量
 * @param dy 方向向量y分量
 * @param speed 速度向量模长
 * @param vx 速度向量x分量
 * @param vy 速度向量y分量
 */
void direction_to_velocity(int16_t dx, int16_t dy,int8_t speed,int16_t *vx, int16_t *vy)
{
    // 绝对值
    uint16_t ax = (dx < 0) ? -dx : dx;
    uint16_t ay = (dy < 0) ? -dy : dy;
    
    // 近似长度 L ≈ max + min/4
    uint16_t max_val = (ax > ay) ? ax : ay;
    uint16_t min_val = (ax < ay) ? ax : ay;
    uint32_t L_approx = max_val + (min_val >> 2);
    if (L_approx == 0) L_approx = 1;   // 防止除零
    
    // 计算缩放因子 (Q16.16 格式)
    // scale = speed * 65536 / L_approx
    uint32_t scale = ((uint32_t)speed << 16) / L_approx;
    
    // 计算速度分量
    *vx = (int32_t)dx * scale >> 16;
    *vy = (int32_t)dy * scale >> 16;
}

/**
 * @brief 计算向量长度
 * @return 向量长度
 * @note 使用欧几里得近似公式
 */
uint16_t vec_length(int16_t x, int16_t y)
{
    if (x == 0 && y == 0) return 0;
    if (x == 0) return abs(y); 
    if (y == 0) return abs(x);
    uint16_t ax = (x < 0) ? -x : x;
    uint16_t ay = (y < 0) ? -y : y;
    uint16_t max_val = (ax > ay) ? ax : ay;
    uint16_t min_val = (ax < ay) ? ax : ay;
    uint16_t L_approx = max_val + (min_val >> 2);
    return L_approx;
}

/**
 * @brief This is a timer only add up when in play state
 */
uint32_t play_tick_get()
{
    static uint32_t last_sys_tick = 0;
    static uint32_t accumulated = 0;
    uint32_t now = lv_tick_get();
    if (fsm_get_state() == GS_PLAY) {
        accumulated += now - last_sys_tick;
    }
    last_sys_tick = now;
    return accumulated;
}
