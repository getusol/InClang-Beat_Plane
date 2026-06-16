/**
 * @file skill.h
 * @brief 玩家技能大全 指针式
 */

#ifndef __SKILL_H__
#define __SKILL_H__

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

void skill_player_fire(game_obj_t *obj);
void skill_ember_fire(game_obj_t *obj);
void skill_stream_fire(game_obj_t *obj);
void skill_verdant_fire(game_obj_t *obj);

void skill_triple_shot(game_obj_t *obj);
void skill_shield(game_obj_t *obj);
void skill_fire_bullet(game_obj_t *obj);
void skill_flame_wall(game_obj_t *obj);
void skill_ice_bullet(game_obj_t *obj);
void skill_freeze(game_obj_t *obj);
void skill_accelerate(game_obj_t *obj);
void skill_heal(game_obj_t *obj);

#endif
