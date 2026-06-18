/**
 * @file lkey.h
 * @brief 本地按键接口 (local key)
 */

#ifndef __LKEY_H__
#define __LKEY_H__

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>
#include <stdbool.h>

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief 虚拟按钮枚举
 * @note 添加新虚拟按钮:需要在此处注册对应id号
 */
typedef enum {
    KEY_NONE = 0,
    KEY_A,
    KEY_B,
    KEY_X,
    KEY_Y,
    KEY_MAX,
} key_code_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void key_init(void);
void key_scan(const uint8_t *ptr);

bool key_pressed(key_code_t key);
bool key_released(key_code_t key);
bool key_down(key_code_t key);
bool key_long_press(key_code_t key);

/**********************
 *  STATIC VARIABLES
 **********************/

#endif /* __LKEY_H__ */
