/**
 * @file flame_wall.h
 * @brief 火墙游戏对象 - Ember Y技能
 */

#ifndef __FLAME_WALL_H__
#define __FLAME_WALL_H__

/*********************
 *      INCLUDES
 *********************/

#include "lvgl.h"
#include "game_object.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void flame_wall_init(lv_obj_t *parent);
game_obj_t *flame_wall_create(lv_coord_t x, lv_coord_t y, int16_t vy);
int flame_wall_get_damage(game_obj_t *obj);

#endif // #ifndef __FLAME_WALL_H__
