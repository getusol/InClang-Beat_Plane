/**
 * @file ui_setting.h
 * @brief 设置界面的 UI
 */

#ifndef __UI_SETTING_H__
#define __UI_SETTING_H__

/*********************
 *      INCLUDES
 *********************/

#include "fsm.h"

/**********************
 *      MACROS
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void ui_setting_init(void);
void ui_setting_run(void);
void ui_setting_set_prev_state(game_state_t s);
game_state_t ui_setting_get_prev_state(void);

#endif // #ifndef __UI_SETTING_H__
