/**
 * @file ui_lobby.h
 */

#ifndef __UI_LOBBY_H__
#define __UI_LOBBY_H__

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

void ui_lobby_init_stage1(void);
void ui_lobby_init_stage2(void);
void ui_lobby_run(void);
void ui_lobby_esc_behave(void);
void ui_lobby_navigate(void);
void ui_lobby_flush(void);

#endif // __ifndef __UI_LOBBY_H__
