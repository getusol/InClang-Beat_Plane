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
