/**
 * @file hitbox.h
 * @brief 提供调试用lvgl碰撞箱代码
 */

#ifndef __HITBOX_H__
#define __HITBOX_H__

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

/**********************
 *  STATIC PROTOTYPES
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void hitbox_create(game_obj_t *obj, void *v);
void hitbox_update(game_obj_t *obj);
void hitbox_init(void);
void hitbox_resize(game_obj_t *obj);

#endif // #ifndef __HITBOX_H__
