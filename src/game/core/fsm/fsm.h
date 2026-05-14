/**
 * @file fsm.h
 */

#ifndef __FSM_H__
#define __FSM_H__

/*********************
 *      INCLUDES
 *********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/
//note:增加新的状态需要在ui中加入状态的ui_*_run()函数和esc在对应ui中的反应
typedef enum {
    GS_MENU,
    GS_PLAY,
        GS_PAUSE,   //GS_PLAY子状态
        GS_OVER,    //GS_PLAY子状态
    SYS_HALT,       //停机状态

    GS_MAX          //GS最大个数，不是有效的游戏状态
} game_state_t;

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void fsm_init(void);
game_state_t fsm_get_state(void);
void fsm_switch_state(game_state_t new_state);

#endif // #ifndef __FSM_H__
