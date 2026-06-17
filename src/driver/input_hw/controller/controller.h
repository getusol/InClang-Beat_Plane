/**
 * @file controller.h
 * @brief XInput 手柄输入接口 (PC) / stub (MCU)
 */

#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>
#include <stdbool.h>
#include "lkey.h"

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

void ckey_init(void);
void ckey_scan(void);

bool ckey_pressed(key_code_t key);
bool ckey_released(key_code_t key);
bool ckey_down(key_code_t key);
bool ckey_long_press(key_code_t key);

int16_t cjoystick_get_x(void);
int16_t cjoystick_get_y(void);

/**********************
 *  STATIC VARIABLES
 **********************/

#endif /* __CONTROLLER_H__ */
