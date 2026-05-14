/**
 * @file input_sw.h
 * @brief 这个文件管按键分发 同时集成了按键扫描
 */

#ifndef __INPUT_SW_H__
#define __INPUT_SW_H__

/*********************
 *      INCLUDES
 *********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief 按键事件枚举
 */
typedef enum {
    KEY_EVENT_A,
    KEY_EVENT_B,
    KEY_EVENT_X,
    KEY_EVENT_Y,
    // 可以根据需要添加更多按键事件
    KEY_EVENT_COUNT        //最大按键个数
} key_event_t;

/**
 * @brief 按键回调函数类型
 */
typedef void (*key_event_callback_t)(void);

/**********************
 *  STATIC PROTOTYPES
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void input_dispatch();
void input_init();

void input_sw_register_press_callback(key_event_t event, key_event_callback_t callback);
void input_sw_register_long_press_callback(key_event_t event, key_event_callback_t callback,uint32_t cycle_delay_ms);

#endif // #ifndef __INPUT_SW_H__
