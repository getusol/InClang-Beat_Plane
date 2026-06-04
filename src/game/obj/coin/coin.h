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

<<<<<<< HEAD
/**
 * @brief 在指定坐标位置生成（激活）一枚金币
 * 内部会从 pool 对象池中申请空闲槽位，设置坐标并展现。
 * * @param x 金币生成的初始 X 坐标
 * @param y 金币生成的初始 Y 坐标
 * @return game_obj_t* 返回金币的游戏对象基类指针；若对象池已满，则返回 NULL
 */
game_obj_t * coin_spawn(lv_coord_t x, lv_coord_t y);

#endif /* COIN_H */

=======
#endif // #ifndef __COIN_H__
>>>>>>> feature-optimizeMerge#3
