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

void ui_play_init();
void ui_play_run();
lv_obj_t * ui_play_get_display(void);

 /**********************
 *   STATIC FUNCTIONS
 **********************/

#endif //#ifndef __UI_PLAY_H__
