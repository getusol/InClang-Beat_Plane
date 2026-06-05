/**
 * @file player.h
 */

#ifndef __PLAYER_H__
#define __PLAYER_H__

/*********************
 * INCLUDES
 *********************/
#include "lvgl.h"
#include "game_object.h"
#include <stdint.h>
#include <stdbool.h>

/**********************
 * TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void player_init(lv_obj_t * parent);
game_obj_t * player_get_base(void);
int16_t player_hp_modify(int16_t delta);
void player_apply_config(int plane_id);
int player_get_current_plane(void);
bool player_is_shield_active(void);
uint32_t player_get_skill_x_cd(void);
uint32_t player_get_skill_y_cd(void);
uint32_t player_get_skill_x_elapsed(void);
uint32_t player_get_skill_y_elapsed(void);

#endif // #ifndef __PLAYER_H__
