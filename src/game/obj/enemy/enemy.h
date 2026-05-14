/**
 * @file enemy.h
 */

#ifndef __ENEMY_H__
#define __ENEMY_H__

/*********************
 *      INCLUDES
 *********************/

#include "game_object.h"
#include "lvgl.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void enemy_init(lv_obj_t * parent);
game_obj_t * enemy_spawn(lv_coord_t x, lv_coord_t y);

#endif // #ifndef __ENEMY_H__
