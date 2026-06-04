/**
 * @file coin.h
 */

#ifndef __COIN_H__
#define __COIN_H__

/*********************
 *      INCLUDES
 *********************/

#include "lvgl.h"          // 提供 lv_obj_t 和 lv_coord_t 类型定义
#include "game_object.h"   // 提供 game_obj_t 基类定义
#include <stdint.h>

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void coin_init(lv_obj_t * parent);
game_obj_t * coin_spawn(lv_coord_t x, lv_coord_t y,
                        uint16_t value, uint8_t disappear_time_s);
int coin_get_num(void);
void coin_add_num(int delta);
void coin_set_num(int value);

#endif // #ifndef __COIN_H__
