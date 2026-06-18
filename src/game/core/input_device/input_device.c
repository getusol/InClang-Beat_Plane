/**
 * @file input_device.c
 * @brief 输入设备抽象层实现 — 将 LOCAL/REMOTE 函数指针组装为虚函数表
 */

/*********************
 *      INCLUDES
 *********************/
#include "input_device.h"
#include "lkey.h"
#include "rkey.h"
#include "controller.h"
#include "joystick.h"
#include <stddef.h>

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static input_device_t local_device;
static input_device_t remote_device;
static input_device_t controller_device;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化输入设备 — 将函数指针"插"到两个设备单例上
 * @note 必须在 key_init / joystick_init / rkey_init 之后调用
 */
void input_device_init(void)
{
    /* ---- LOCAL: 直接指向本地硬件函数 ---- */
    local_device.pressed = key_pressed;
    local_device.released = key_released;
    local_device.down = key_down;
    local_device.long_press = key_long_press;
    local_device.x = joystick_get_x;
    local_device.y = joystick_get_y;
    local_device.type = INPUT_DEVICE_LOCAL;

    /* ---- REMOTE: rkey_xxx / rjoystick_xxx (MCU stub) ---- */
    remote_device.pressed = rkey_pressed;
    remote_device.released = rkey_released;
    remote_device.down = rkey_down;
    remote_device.long_press = rkey_long_press;
    remote_device.x = rjoystick_get_x;
    remote_device.y = rjoystick_get_y;
    remote_device.type = INPUT_DEVICE_REMOTE;

    /* ---- CONTROLLER: XInput 手柄 (PC) / stub (MCU) ---- */
    controller_device.pressed = ckey_pressed;
    controller_device.released = ckey_released;
    controller_device.down = ckey_down;
    controller_device.long_press = ckey_long_press;
    controller_device.x = cjoystick_get_x;
    controller_device.y = cjoystick_get_y;
    controller_device.type = INPUT_DEVICE_CONTROLLER;
}

/**
 * @brief 获取指定类型的输入设备指针
 * @param type INPUT_DEVICE_LOCAL 或 INPUT_DEVICE_REMOTE
 * @return 设备指针, type 无效时返回 NULL
 */
input_device_t *input_device_get(input_device_type_t type)
{
    static input_device_t *devices[INPUT_DEVICE_COUNT] = {
        &local_device,
        &remote_device,
        &controller_device,
    };
    return (type < INPUT_DEVICE_COUNT) ? devices[type] : NULL;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
