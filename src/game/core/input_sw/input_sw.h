/**
 * @file input_sw.h
 * @brief 这个文件管按键分发 同时集成了按键扫描
 * @note 现在这里的按键注册只给UI负责，不负责游戏逻辑
 */

#ifndef __INPUT_SW_H__
#define __INPUT_SW_H__

/*********************
 *      INCLUDES
 *********************/

#include "input_device.h"

/**********************
 *      MACROS
 **********************/

#define INPUT_DEVICE_ANY INPUT_DEVICE_COUNT // 任意设备

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief 按键事件枚举
 */
typedef enum
{
    KEY_EVENT_A,
    KEY_EVENT_B,
    KEY_EVENT_X,
    KEY_EVENT_Y,
    KEY_EVENT_COUNT // 最大按键个数
} key_event_t;

/**
 * @brief 事件结构体
 */
typedef struct
{
    input_device_type_t dev_type;
    key_code_t key_code;
    void *usr_data;
} input_event_t;

/**
 * @brief 按键回调函数类型
 */
typedef void (*input_event_callback_t)(input_event_t *event);

/**********************
 *  STATIC PROTOTYPES
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void input_dispatch();
void input_init();

void input_sw_register_press_callback(input_device_type_t dev_type, key_event_t event, input_event_callback_t callback, void *usr_data);
void input_sw_unregister_press_callback(input_device_type_t dev_type, key_event_t event, input_event_callback_t callback);

#endif // #ifndef __INPUT_SW_H__
