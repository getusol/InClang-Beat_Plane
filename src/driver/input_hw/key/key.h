/**
 * @file key.h
 */

#ifndef __KEY_H__
#define __KEY_H__
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
 * @brief 虚拟按钮枚举注册处
 * @note 添加新虚拟按钮:需要在此处注册对应id号
 */
typedef enum
{
    KEY_NONE = 0,
    KEY_A,
    KEY_B,
    KEY_X,
    KEY_Y,
} key_code_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void key_init();
void key_scan(const uint8_t * ptr);

bool key_pressed(key_code_t key);
bool key_released(key_code_t key);
bool key_down(key_code_t key);
bool key_long_press(key_code_t key);

/**********************
 *  STATIC VARIABLES
 **********************/

#endif
