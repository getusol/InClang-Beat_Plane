/**
 * @file player.h
 */

#ifndef PLAYER_H
#define PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*PLAYER_H*/