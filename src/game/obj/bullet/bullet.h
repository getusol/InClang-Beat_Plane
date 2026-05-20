/**
 * @file bullet.h
 */

#ifndef __BULLET_H__
#define __BULLET_H__

/*********************
 *      INCLUDES
 *********************/
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

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void bullet_init(lv_obj_t *parent);
game_obj_t * bullet_create(game_obj_t *source,
                         lv_coord_t x, lv_coord_t y, 
                         int16_t vx, int16_t vy,
                         uint8_t damage,
                         behave_t behave);
uint8_t bullet_get_damage(game_obj_t * bullet);
void bullet_set_timer(game_obj_t * g,uint32_t delay_ms,void (*on_timer)(game_obj_t * obj));

#endif // #ifndef __BULLET_H__
