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
#include "ui_lobby.h"

#include "tools.h"
#include "lvgl.h"
#include "fsm.h"
#include <string.h>

/**********************
 *      MACROS
 **********************/

#define KEY_EVENT_MAX 2 // 最大单个按键事件数量

/**********************
 *      TYPEDEFS
 **********************/

typedef struct
{
    input_event_callback_t callback;
    void *usr_data;
} input_event_struct_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void input_sw_dispatch();

/**********************
 *  STATIC VARIABLES
 **********************/

// 按键事件回调函数数组，支持多个按键事件的回调注册 每个事件最多支持 `KEY_EVENT_MAX` 个回调函数
static input_event_struct_t press_callbacks[INPUT_DEVICE_COUNT][KEY_EVENT_COUNT][KEY_EVENT_MAX] = {NULL};

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
 * @param dev_type 输入设备类型
 * @param event 按键事件类型
 * @param callback 回调函数指针
 * @note 单个按键事件最大支持 `KEY_EVENT_MAX` 个按键行为的回调注册
 *       用不了callback中的参数 void *   只给UI负责
 * @return void
 */
void input_sw_register_press_callback(input_device_type_t dev_type, key_event_t event, input_event_callback_t callback, void *usr_data)
{
    if (callback == NULL)
        return;
    if (event >= KEY_EVENT_COUNT)
    {
        CONSOLE_WARNING("Invalid event type: %d", event);
        LOG_WARNING("Invalid event type: %d", event);
        return;
    }
    if (dev_type >= INPUT_DEVICE_COUNT && dev_type != INPUT_DEVICE_ANY)
    {
        CONSOLE_WARNING("Invalid device type: %d", dev_type);
        LOG_WARNING("Invalid device type: %d", dev_type);
        return;
    }
    input_event_struct_t new_callback = {callback, usr_data};

    if (dev_type == INPUT_DEVICE_ANY)
    {
        bool found = false;
        for (int j = 0; j < INPUT_DEVICE_COUNT; j++)
        {
            found = false;
            for (int i = 0; i < KEY_EVENT_MAX; i++)
            {
                if (press_callbacks[j][event][i].callback == callback && press_callbacks[j][event][i].usr_data == usr_data)
                    found = true;
            }
            if (found)
                continue;
            for (int i = 0; i < KEY_EVENT_MAX; i++)
            {
                if (press_callbacks[j][event][i].callback == NULL)
                {
                    press_callbacks[j][event][i] = new_callback;
                    CONSOLE_INFO("Callback registered for event %d at index %d,dev %d", event, i, j);
                    break; /* 继续注册下一个设备 */
                }
            }
        }
        return;
    }

    for (int i = 0; i < KEY_EVENT_MAX; i++)
    {
        if (press_callbacks[dev_type][event][i].callback == callback && press_callbacks[dev_type][event][i].usr_data == usr_data)
            return; // 已注册
    }
    for (int i = 0; i < KEY_EVENT_MAX; i++)
    {
        if (press_callbacks[dev_type][event][i].callback == NULL)
        {
            press_callbacks[dev_type][event][i] = new_callback;
            CONSOLE_INFO("Callback registered for event %d at index %d,dev %d", event, i, dev_type);
            return;
        }
    }
    CONSOLE_WARNING("No available slot to register callback for event: %d", event);
    LOG_WARNING("No available slot to register callback for event: %d", event);
}

/**
 * @brief 注销按键短按回调函数
 */
void input_sw_unregister_press_callback(input_device_type_t dev_type, key_event_t event, input_event_callback_t callback)
{
    if (callback == NULL)
        return;
    if (event >= KEY_EVENT_COUNT)
        return;
    if (dev_type >= INPUT_DEVICE_COUNT && dev_type != INPUT_DEVICE_ANY)
        return;

    if (dev_type == INPUT_DEVICE_ANY)
    {
        for (int j = 0; j < INPUT_DEVICE_COUNT; j++)
        {
            for (int i = 0; i < KEY_EVENT_MAX; i++)
            {
                if (press_callbacks[j][event][i].callback == callback)
                {
                    press_callbacks[j][event][i] = (input_event_struct_t){NULL, NULL};
                }
            }
        }
        return;
    }

    for (int i = 0; i < KEY_EVENT_MAX; i++)
    {
        if (press_callbacks[dev_type][event][i].callback == callback)
        {
            press_callbacks[dev_type][event][i] = (input_event_struct_t){NULL, NULL};
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 按键分发函数，负责将扫描到的按键事件分发给相应的处理函数
 */
static void input_sw_dispatch()
{
    game_state_t state = fsm_get_state();

    input_event_t event = {0};

    for (int i = 0; i < INPUT_DEVICE_COUNT; i++)
    {
        for (int j = 0; j < KEY_EVENT_COUNT; j++)
        {
            // GS_PLAY: 只处理 KEY_B (暂停), 其余按键留给 player_control
            if (state == GS_PLAY && j != KEY_EVENT_B)
                continue;

            if (!(input_device_get(i)->pressed(j + 1)))
                continue;
            for (int k = 0; k < KEY_EVENT_MAX; k++)
            {
                if (press_callbacks[i][j][k].callback != NULL)
                {
                    event.dev_type = i;
                    event.key_code = j + 1;
                    event.usr_data = press_callbacks[i][j][k].usr_data;
                    press_callbacks[i][j][k].callback(&event);
                }
            }
        }
    }

    if (state == GS_LOBBY)
        ui_lobby_navigate();
}
