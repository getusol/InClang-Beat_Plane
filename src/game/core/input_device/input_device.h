/**
 * @file input_device.h
 * @brief 输入设备抽象层 — 纯虚函数表封装 LOCAL 和 REMOTE 输入源
 */

#ifndef __INPUT_DEVICE_H__
#define __INPUT_DEVICE_H__

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>
#include <stdbool.h>
#include "lkey.h"

/**********************
 *      TYPEDEFS
 **********************/

typedef enum
{
    INPUT_DEVICE_LOCAL,
    INPUT_DEVICE_REMOTE,
    INPUT_DEVICE_CONTROLLER,
    INPUT_DEVICE_COUNT,
} input_device_type_t;

typedef struct input_device input_device_t;

/**
 * @brief 输入设备抽象
 *
 * LOCAL:  本机硬件 (PC 键盘/SDL, MCU GPIO/ADC)
 * REMOTE: 对端设备 (PC 读 comm_rx, MCU stub)
 *
 * 所有成员都是函数指针, 无缓存状态, 每次调用实时读取硬件层。
 */
struct input_device
{
    input_device_type_t type;
    bool (*pressed)(key_code_t key);
    bool (*released)(key_code_t key);
    bool (*down)(key_code_t key);
    bool (*long_press)(key_code_t key);
    int16_t (*x)(void);
    int16_t (*y)(void);
};

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void input_device_init(void);
input_device_t *input_device_get(input_device_type_t type);

/**
 * @brief 便捷宏 — 获取 LOCAL / REMOTE 设备指针
 *
 * 用法:
 *   LOCAL->down(KEY_A)       // 本机 A 键是否按住
 *   REMOTE->x()              // 远程摇杆 X 值
 *   LOCAL->pressed(KEY_X)    // 本机 X 键是否刚按下
 */
#define LOCAL input_device_get(INPUT_DEVICE_LOCAL)
#define REMOTE input_device_get(INPUT_DEVICE_REMOTE)
#define CONTROLLER input_device_get(INPUT_DEVICE_CONTROLLER)

#endif /* __INPUT_DEVICE_H__ */
