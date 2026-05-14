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
uint16_t bullet_create(game_obj_t *source, float speed, lv_coord_t x, lv_coord_t y, uint8_t damage); // 目前方向固定向上

#endif // #ifndef __BULLET_H__
