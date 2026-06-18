/**
 * @file enemy.h
 */

#ifndef __ENEMY_H__
#define __ENEMY_H__

/*********************
 *      INCLUDES
 *********************/

#include "game_object.h"
#include "apr.h"
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
game_obj_t * enemy_spawn(lv_coord_t x, lv_coord_t y,
                         int16_t vx, int16_t vy,
                         uint16_t health, int16_t hit_damage,
                         behave_t behave,
                         apr_id_t apr_id);
int16_t enemy_get_damage(game_obj_t * g);

// 状态效果
void enemy_apply_burn(game_obj_t *g);
void enemy_apply_freeze(game_obj_t *g);
bool enemy_is_frozen(game_obj_t *g);
void enemy_apply_damage(game_obj_t *g, int16_t damage);
uint16_t enemy_count_active(void);

#endif // #ifndef __ENEMY_H__
