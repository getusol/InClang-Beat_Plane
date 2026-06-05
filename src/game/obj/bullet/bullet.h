/**
 * @file bullet.h
 */

#ifndef __BULLET_H__
#define __BULLET_H__

/*********************
 *      INCLUDES
 *********************/
#include "game_object.h"
#include "apr.h"

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
                         int16_t damage,
                         behave_t behave,
                         apr_id_t bullet_apr);
int16_t bullet_get_damage(game_obj_t * bullet);
game_obj_t * bullet_get_source(game_obj_t * g);

/* 子弹特殊效果标志 */
#define BULLET_FLAG_NONE      0x00
#define BULLET_FLAG_BURN      0x01
#define BULLET_FLAG_FREEZE    0x02
#define BULLET_FLAG_REFLECTED 0x04

void bullet_set_flags(game_obj_t *bullet, uint8_t flags);
uint8_t bullet_get_flags(game_obj_t *bullet);
void bullet_set_source(game_obj_t *bullet, game_obj_t *source);
void bullet_set_enemy_slow(bool enabled);
bool bullet_get_enemy_slow(void);

#endif // #ifndef __BULLET_H__
