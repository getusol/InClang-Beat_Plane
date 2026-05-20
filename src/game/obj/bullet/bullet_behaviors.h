/**
 * @file bullet_behaviors.h
 * @brief A file containing all bullet behaviors
 */

#ifndef __BULLET_BEHAVIORS_H__
#define __BULLET_BEHAVIORS_H__

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

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void bullet_behave_circle(game_obj_t *g, void *v);
void bullet_behave_sine(game_obj_t *g, void *v);

#endif // #ifndef __BULLET_BEHAVIORS_H__
