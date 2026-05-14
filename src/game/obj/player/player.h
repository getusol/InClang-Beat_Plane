/**
 * @file player.h
 */

#ifndef __PLAYER_H__
#define __PLAYER_H__

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

void player_init(lv_obj_t * parent);
game_obj_t * player_get_base();
int16_t player_hp_modify(int16_t delta);
lv_point_t player_move(lv_coord_t dx, lv_coord_t dy);

#endif // #ifndef __PLAYER_H__
