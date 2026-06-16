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

void player_init(lv_obj_t *parent);
game_obj_t *player_spawn();

game_obj_t *player_get_base(void);
int16_t player_hp_modify(int16_t delta);
void player_apply_config(int plane_id);
int player_get_current_plane(void);
bool player_is_shield_active(void);

void player_set_shield_active(game_obj_t *obj, bool active);

bool player_is_shield_active_for(game_obj_t *obj);
uint32_t player_get_skill_x_cd(void);
uint32_t player_get_skill_y_cd(void);
uint32_t player_get_skill_x_elapsed(void);
uint32_t player_get_skill_y_elapsed(void);

#ifdef SIMULATOR
game_obj_t *player_get_p2_base(void);
void player_apply_p2_config(int plane_id);
uint32_t player_get_p2_skill_x_cd(void);
uint32_t player_get_p2_skill_y_cd(void);
uint32_t player_get_p2_skill_x_elapsed(void);
uint32_t player_get_p2_skill_y_elapsed(void);
#endif

#endif // #ifndef __PLAYER_H__
