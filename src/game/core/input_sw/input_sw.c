/**
 * @file input_sw.h
 */

/*********************
 *      INCLUDES
 *********************/
#include "input_hw.h"
#include "input_sw.h"
#include "input_device.h"
#include "rkey.h"

#include "tools.h"
#include "lvgl.h"
#include <string.h>

/**********************
 *      MACROS
 **********************/

#define KEY_EVENT_MAX 2 // 最大单个按键事件数量

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void input_sw_dispatch();
#ifdef SIMULATOR
static key_code_t event_to_key_code(key_event_t event);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

// 按键事件回调函数数组，支持多个按键事件的回调注册 每个事件最多支持 `KEY_EVENT_MAX` 个回调函数
static key_event_callback_t press_callbacks[KEY_EVENT_COUNT][KEY_EVENT_MAX] = {NULL};

// 长按定时器
static non_blocking_timer_t long_press_timers[KEY_EVENT_COUNT][KEY_EVENT_MAX] = {{0}};

// 按下定时器
static non_blocking_timer_t key_down_timers[KEY_EVENT_COUNT][KEY_EVENT_MAX] = {{0}};

// 长按回调函数数组，支持多个按键事件的长按回调注册 每个事件最多支持 `KEY_EVENT_MAX` 个回调函数
static key_event_callback_t long_press_callbacks[KEY_EVENT_COUNT][KEY_EVENT_MAX] = {NULL};

// 按下回调函数数组 支持多个按键事件的按下回调注册 每个事件最多支持 `KEY_EVENT_MAX` 个回调函数
static key_event_callback_t key_down_callbacks[KEY_EVENT_COUNT][KEY_EVENT_MAX] = {NULL};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 输入分发函数，包含按键扫描和按键分发
 */
void input_dispatch()
{
    input_hw_scan();
    input_sw_dispatch();
}

/**
 * @brief 输入系统初始化函数，负责初始化输入相关的资源和状态
 */
void input_init()
{
    input_hw_init();
    input_device_init();
}

/**
 * @brief 注册按键事件短按回调函数
 * @param event 按键事件类型
 * @param callback 回调函数指针
 * @note 单个按键事件最大支持 `KEY_EVENT_MAX` 个按键行为的回调注册
 *       用不了callback中的参数 void *
 * @return void
 */
void input_sw_register_press_callback(key_event_t event, key_event_callback_t callback)
{
    if (event >= KEY_EVENT_COUNT)
    {
        CONSOLE_WARNING("Invalid event type: %d", event);
        LOG_WARNING("Invalid event type: %d", event);
        return;
    }
    for (int i = 0; i < KEY_EVENT_MAX; i++)
    {
        if (press_callbacks[event][i] == NULL)
        {
            press_callbacks[event][i] = callback;
            CONSOLE_INFO("Callback registered for event %d at index %d", event, i);
            return;
        }
    }
    CONSOLE_WARNING("No available slot to register callback for event: %d", event);
    LOG_WARNING("No available slot to register callback for event: %d", event);
}

/**
 * @brief 注册按键事件长按回调函数
 * @param event 按键事件类型
 * @param callback 回调函数指针
 * @param cycle_delay_ms 长按回调函数的调用周期，单位毫秒
 * @note 单个按键事件最大支持 `KEY_EVENT_MAX` 个按键行为的长按回调注册
 *       长按一段时间后 才会开始循环触发
 *       不会修改 v,v常驻
 */
void input_sw_register_long_press_callback(key_event_t event, key_event_callback_t callback, uint32_t cycle_delay_ms)
{
    if (event >= KEY_EVENT_COUNT)
    {
        CONSOLE_WARNING("Invalid event type: %d", event);
        LOG_WARNING("Invalid event type: %d", event);
        return;
    }
    for (int i = 0; i < KEY_EVENT_MAX; i++)
    {
        if (long_press_callbacks[event][i] == NULL)
        {
            long_press_callbacks[event][i] = callback;
            long_press_timers[event][i].func = callback;
            long_press_timers[event][i].tick_get = play_tick_get;
            long_press_timers[event][i].delay_ms = cycle_delay_ms;
            // 设置 last_tick 使得首次长按立即触发
            long_press_timers[event][i].last_tick = play_tick_get() - cycle_delay_ms - 1;
            console_out("[input_sw_register_long_press_callback] Long press callback registered for event %d at index %d with cycle delay %d ms\n", event, i, cycle_delay_ms);
            return;
        }
    }
    CONSOLE_WARNING("No available slot to register long press callback for event: %d", event);
    LOG_WARNING("No available slot to register long press callback for event: %d", event);
}

/**
 * @brief 注册按键事件按下回调函数
 * @param event 按键事件类型
 * @param callback 回调函数指针
 * @param cycle_delay_ms 长按回调函数的调用周期，单位毫秒
 * @note 单个按键事件最大支持 `KEY_EVENT_MAX` 个按键行为的长按回调注册
 *       直接开始循环触发
 *       不会修改 v,v常驻
 */
void input_sw_register_key_down_callback(key_event_t event, key_event_callback_t callback, uint32_t cycle_delay_ms)
{
    if (event >= KEY_EVENT_COUNT)
    {
        CONSOLE_WARNING("Invalid event type: %d", event);
        LOG_WARNING("Invalid event type: %d", event);
        return;
    }
    for (int i = 0; i < KEY_EVENT_MAX; i++)
    {
        if (key_down_callbacks[event][i] == NULL)
        {
            key_down_callbacks[event][i] = callback;
            key_down_timers[event][i].func = callback;
            key_down_timers[event][i].tick_get = play_tick_get;
            key_down_timers[event][i].delay_ms = cycle_delay_ms;
            // 设置 last_tick 使得首次按下立即触发
            key_down_timers[event][i].last_tick = play_tick_get() - cycle_delay_ms - 1;
            CONSOLE_INFO("key down callback registered for event %d at index %d with cycle delay %d ms", event, i, cycle_delay_ms);
            return;
        }
    }
    CONSOLE_WARNING("No available slot to register long press callback for event: %d", event);
    LOG_WARNING("No available slot to register long press callback for event: %d", event);
}

/**
 * @brief 注销按键按下回调函数
 */
void input_sw_unregister_key_down_callback(key_event_t event, key_event_callback_t callback)
{
    if (event >= KEY_EVENT_COUNT)
        return;
    for (int i = 0; i < KEY_EVENT_MAX; i++)
    {
        if (key_down_callbacks[event][i] == callback)
        {
            key_down_callbacks[event][i] = NULL;
            memset(&key_down_timers[event][i], 0, sizeof(non_blocking_timer_t));
        }
    }
}

/**
 * @brief 注销按键长按回调函数
 */
void input_sw_unregister_long_press_callback(key_event_t event, key_event_callback_t callback)
{
    if (event >= KEY_EVENT_COUNT)
        return;
    for (int i = 0; i < KEY_EVENT_MAX; i++)
    {
        if (long_press_callbacks[event][i] == callback)
        {
            long_press_callbacks[event][i] = NULL;
            memset(&long_press_timers[event][i], 0, sizeof(non_blocking_timer_t));
        }
    }
}

/**
 * @brief 注销按键短按回调函数
 */
void input_sw_unregister_press_callback(key_event_t event, key_event_callback_t callback)
{
    if (event >= KEY_EVENT_COUNT)
        return;
    for (int i = 0; i < KEY_EVENT_MAX; i++)
    {
        if (press_callbacks[event][i] == callback)
        {
            press_callbacks[event][i] = NULL;
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 将 key_event_t 映射为 key_code_t (统一映射到 KEY_A..KEY_Y)
 * @note RKEY 事件映射到同样的 KEY_A..KEY_Y, 由调用方选择 key_* 或 rkey_*
 */
static key_code_t event_to_key_code(key_event_t event)
{
#ifdef SIMULATOR
    if (event >= KEY_EVENT_RKEY_A)
        return (key_code_t)(KEY_A + (event - KEY_EVENT_RKEY_A));
#endif
    return (key_code_t)(event + 1);
}

#ifdef SIMULATOR
static bool is_remote_event(key_event_t event)
{
    return event >= KEY_EVENT_RKEY_A && event <= KEY_EVENT_RKEY_Y;
}

static bool event_key_pressed(key_event_t event)
{
    key_code_t k = event_to_key_code(event);
    return is_remote_event(event) ? rkey_pressed(k) : key_pressed(k);
}

static bool event_key_long_press(key_event_t event)
{
    key_code_t k = event_to_key_code(event);
    return is_remote_event(event) ? rkey_long_press(k) : key_long_press(k);
}

static bool event_key_down(key_event_t event)
{
    key_code_t k = event_to_key_code(event);
    return is_remote_event(event) ? rkey_down(k) : key_down(k);
}
#else
#define event_key_pressed(e) key_pressed(event_to_key_code(e))
#define event_key_long_press(e) key_long_press(event_to_key_code(e))
#define event_key_down(e) key_down(event_to_key_code(e))
#endif

/**
 * @brief 按键分发函数，负责将扫描到的按键事件分发给相应的处理函数
 */
static void input_sw_dispatch()
{

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

    for (int i = 0; i < KEY_EVENT_COUNT; i++)
    {
        if (!event_key_pressed(i))
            continue;
        for (int j = 0; j < KEY_EVENT_MAX; j++)
        {
            if (press_callbacks[i][j] != NULL)
            {
                press_callbacks[i][j]();
            }
        }
    }

    // 处理长按事件

    for (int i = 0; i < KEY_EVENT_COUNT; i++)
    {
        if (!event_key_long_press(i))
            continue;
        for (int j = 0; j < KEY_EVENT_MAX; j++)
        {
            if (long_press_callbacks[i][j] != NULL)
            {
                non_blocking_delay(&long_press_timers[i][j]);
            }
        }
    }

    // 处理按下事件
    // X键诊断
    static int x_key_check_count = 0;
    if (x_key_check_count < 5 && event_key_down(KEY_EVENT_X))
    {
        CONSOLE_INFO("key_down(KEY_X)=true, callbacks[0]=%p callbacks[1]=%p",
                     (void *)key_down_callbacks[KEY_EVENT_X][0],
                     (void *)key_down_callbacks[KEY_EVENT_X][1]);
        x_key_check_count++;
    }

    for (int i = 0; i < KEY_EVENT_COUNT; i++)
    {
        if (!event_key_down(i))
            continue;
        for (int j = 0; j < KEY_EVENT_MAX; j++)
        {
            if (key_down_callbacks[i][j] != NULL)
            {
                non_blocking_delay(&key_down_timers[i][j]);
            }
            else if (i == KEY_EVENT_X)
            {
                // 仅X键: 检测到按下但无回调注册
                static int x_no_cb_log = 0;
                if (x_no_cb_log < 3)
                {

                    x_no_cb_log++;
                }
            }
        }
    }
}
