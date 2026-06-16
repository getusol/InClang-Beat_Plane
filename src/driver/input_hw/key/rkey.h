/**
 * @file rkey.h
 * @brief 远程按键接口 (remote key)
 *        PC 端读 comm_rx 串口数据, MCU 端为 stub (返回 false)
 */

#ifndef __RKEY_H__
#define __RKEY_H__

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

void rkey_init(void);
void rkey_scan(void);

bool rkey_pressed(key_code_t key);
bool rkey_released(key_code_t key);
bool rkey_down(key_code_t key);
bool rkey_long_press(key_code_t key);

/**********************
 *  STATIC VARIABLES
 **********************/

#endif /* __RKEY_H__ */
