/**
 * @file enemy_behaviors.h
 */

#ifndef __ENEMY_BEHAVIORS_H__
#define __ENEMY_BEHAVIORS_H__

/*********************
 *      INCLUDES
 *********************/

#include "game_object.h"

/**********************
 *      MACROS
 **********************/

/** 作为 behave.f 的 void*v 传入时触发死亡逻辑（掉落金币等） */
#define BEHAVE_ON_DEATH ((void *)0xDEAD)

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void enemy_behave_normal(game_obj_t * g,void * v);
void enemy_behave_boss(game_obj_t * g,void * v);
void enemy_behave_enemy4(game_obj_t * g,void * v);
void enemy_behave_endboss(game_obj_t * g,void * v);

#endif // #ifndef __ENEMY_BEHAVIORS_H__
