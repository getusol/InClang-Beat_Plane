/**
 * @file ui.h
 * @brief ui的统一接口
 */

#ifndef __UI_H__
#define __UI_H__

/*********************
 *      INCLUDES
 *********************/

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

void ui_init_stage2();
void ui_init_stage1();
void ui_run();    // 负责当游戏状态改变时 切换ui状态
void ui_update(); // 负责更新ui元素

#endif // #ifndef __UI_H__
