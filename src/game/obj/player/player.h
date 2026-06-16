/**
 * @file player.h
 */

#ifndef __PLAYER_H__
#define __PLAYER_H__

/*********************
 * INCLUDES
 *********************/

#include "lvgl.h"
#include <stdint.h>
#include "game_object.h"
#include "character.h"

/**********************
 * TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void player_init(lv_obj_t *parent);
game_obj_t *player_spawn(lv_coord_t x, lv_coord_t y,
                         character_id_t character_id, behave_t behave);
void player_character_set(game_obj_t *player, character_id_t id);
const character_config_t *player_character_get(game_obj_t *player);
void player_hp_set(game_obj_t *player, int hp);
void player_hp_modify(game_obj_t *player, int delta);
bool player_shield_is_active(game_obj_t *player);
void player_shield_set_active(game_obj_t *player, bool active);

uint32_t player_fire_last_tick_get(game_obj_t *player);
void     player_fire_last_tick_set(game_obj_t *player, uint32_t tick);
uint32_t player_skill_x_last_use_get(game_obj_t *player);
void     player_skill_x_last_use_set(game_obj_t *player, uint32_t tick);
uint32_t player_skill_y_last_use_get(game_obj_t *player);
void     player_skill_y_last_use_set(game_obj_t *player, uint32_t tick);

#endif // #ifndef __PLAYER_H__
