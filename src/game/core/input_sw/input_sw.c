/**
 * @file input_sw.h
 */

/*********************
 *      INCLUDES
 *********************/
#include "input_hw.h"
#include "input_sw.h"

#include "tools.h"
#include "lvgl.h"

/**********************
 *      MACROS
 **********************/

#define KEY_EVENT_MAX 2                    //最大单个按键事件数量

/**********************
 *      TYPEDEFS
 **********************/

 /**********************
  *  STATIC PROTOTYPES
  **********************/

static void input_sw_dispatch();

/**********************
 *  STATIC VARIABLES
 **********************/

// 按键事件回调函数数组，支持多个按键事件的回调注册 每个事件最多支持 `KEY_EVENT_MAX` 个回调函数
static key_event_callback_t press_callbacks[KEY_EVENT_COUNT][KEY_EVENT_MAX] = {NULL}; 

// 长按定时器
static non_blocking_timer_t long_press_timers[KEY_EVENT_COUNT][KEY_EVENT_MAX] = {{0}};

// 长按回调函数数组，支持多个按键事件的长按回调注册 每个事件最多支持 `KEY_EVENT_MAX` 个回调函数
static key_event_callback_t long_press_callbacks[KEY_EVENT_COUNT][KEY_EVENT_MAX] = {NULL}; 

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 输入分发函数，包含按键扫描和按键分发
 */
void input_dispatch() {
    input_hw_scan();
    input_sw_dispatch();
}

/**
 * @brief 输入系统初始化函数，负责初始化输入相关的资源和状态
 */
void input_init() {
    input_hw_init();
}

/**
 * @brief 注册按键事件短按回调函数
 * @param event 按键事件类型
 * @param callback 回调函数指针
 * @note 单个按键事件最大支持 `KEY_EVENT_MAX` 个按键行为的回调注册
 * @return void
 */
void input_sw_register_press_callback(key_event_t event, key_event_callback_t callback)
{
    if (event >= KEY_EVENT_COUNT) {
        console_out("[Warning][input_sw_register_press_callback] Invalid event type: %d\n", event);
        log_out("[Warning][input_sw_register_press_callback] Invalid event type: %d", event);
        return ;
    }
    for (int i = 0; i < KEY_EVENT_MAX; i++) {
        if (press_callbacks[event][i] == NULL) {
            press_callbacks[event][i] = callback;
            console_out("[input_sw_register_press_callback] Callback registered for event %d at index %d\n", event, i);
            return ;
        }
    }
    console_out("[Warning][input_sw_register_press_callback] No available slot to register callback for event: %d\n", event);
    log_out("[Warning][input_sw_register_press_callback] No available slot to register callback for event: %d", event);
}

/**
 * @brief 注册按键事件长按回调函数
 * @param event 按键事件类型
 * @param callback 回调函数指针
 * @param cycle_delay_ms 长按回调函数的调用周期，单位毫秒
 * @note 单个按键事件最大支持 `KEY_EVENT_MAX` 个按键行为的长按回调注册
 */
void input_sw_register_long_press_callback(key_event_t event, key_event_callback_t callback,uint32_t cycle_delay_ms)
{
    if (event >= KEY_EVENT_COUNT) {
        console_out("[Warning][input_sw_register_long_press_callback] Invalid event type: %d\n", event);
        log_out("[Warning][input_sw_register_long_press_callback] Invalid event type: %d", event);
        return ;
    }
    for (int i = 0; i < KEY_EVENT_MAX; i++) {
        if (long_press_callbacks[event][i] == NULL) {
            long_press_callbacks[event][i] = callback;
            long_press_timers[event][i].func = callback;
            long_press_timers[event][i].tick_get = play_tick_get;
            long_press_timers[event][i].delay_ms = cycle_delay_ms;
            long_press_timers[event][i].last_tick = 0;
            console_out("[input_sw_register_long_press_callback] Long press callback registered for event %d at index %d with cycle delay %d ms\n", event, i, cycle_delay_ms);
            return ;
        }
    }
    console_out("[Warning][input_sw_register_long_press_callback] No available slot to register long press callback for event: %d\n", event);
    log_out("[Warning][input_sw_register_long_press_callback] No available slot to register long press callback for event: %d", event);
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 按键分发函数，负责将扫描到的按键事件分发给相应的处理函数
 */
static void input_sw_dispatch() {

    /*NOTE:
    typedef enum                    typedef enum
    {                               {
        KEY_NONE = 0,                   
        KEY_A,                          KEY_EVENT_A,
        KEY_B,                          KEY_EVENT_B,
        KEY_X,                          KEY_EVENT_X,
        KEY_Y,                          KEY_EVENT_Y,
    } key_code_t;                    } key_event_t;
    
    这里的按键事件枚举是从1开始的，所以在调用key_pressed等函数时需要加1
    */

    // 处理短按事件

    for (int i = 0; i < KEY_EVENT_COUNT; i++) {
        if (!key_pressed(i + 1)) continue; // 如果当前按键没有被按下，跳过处理
        for (int j = 0; j < KEY_EVENT_MAX; j++) {
            if (press_callbacks[i][j] != NULL) {
                press_callbacks[i][j]();
            }
        }
    }

    // 处理长按事件

    for (int i = 0; i < KEY_EVENT_COUNT; i++) {
        if (!key_down(i + 1)) continue; // 如果当前按键没有被按下，跳过处理
        for (int j = 0; j < KEY_EVENT_MAX; j++) {
            if (long_press_callbacks[i][j] != NULL) {
                non_blocking_delay(&long_press_timers[i][j]);
            }
        }
    }
}
