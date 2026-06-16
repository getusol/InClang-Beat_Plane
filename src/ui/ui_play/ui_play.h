/**
 * @file ui_play.h
 * @brief 渲染游戏进行时的ui界面，包括它的几个子界面:PAUSE,OVER
 */

#ifndef __UI_PLAY_H__
#define __UI_PLAY_H__

/*********************
 *      INCLUDES
 *********************/

#include "lvgl.h"

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

void ui_play_init_stage1();
void ui_play_init_stage2();
void ui_play_run();
lv_obj_t *ui_play_get_display(void);
lv_obj_t *ui_play_get_hud_layer(void);
void ui_play_level_enter_anim(const char *level_name);
void ui_play_set_freeze_overlay(bool show);

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif // #ifndef __UI_PLAY_H__
