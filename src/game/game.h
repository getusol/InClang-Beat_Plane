/**
 * @file game.h
 */

#ifndef __GAME_H__
#define __GAME_H__

/*********************
 *      INCLUDES
 *********************/

#include <stdbool.h>
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

void game_init();
int game_register_obj(game_obj_t * obj);
void game_update(void);


#endif // #ifndef __GAME_H__
