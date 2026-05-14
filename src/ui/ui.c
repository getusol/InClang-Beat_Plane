/**
 * @file ui.h
 */

/*********************
 *      INCLUDES
 *********************/
#include "ui.h"
#include "fsm.h"
#include "tools.h"
#include "input_sw.h"

#include "ui_menu.h"
#include "ui_play.h"
#include "ui_sys_halt.h"

#include <stdio.h>
#include <stdlib.h>

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

 /**********************
  *  STATIC PROTOTYPES
  **********************/

static void ui_esc_pressed_handler();

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static game_state_t last_game_state = GS_MAX;

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief ui初始化函数 需要在main.c调用一次 初始化所有界面的基础渲染
 */
 void ui_init()
{

  //各界面画图
    ui_menu_init();
    ui_play_init();
    ui_sys_halt_init();
    //按键注册
    input_sw_register_press_callback(KEY_EVENT_B, ui_esc_pressed_handler);
    console_out("[ui] Ui initialization finished\n");
}
/**
 * @brief 根据当前游戏状态决定UI渲染，不同渲染函数位于相应的文件中
 */
void ui_run()
{
    if (fsm_get_state() == last_game_state) return ;
    last_game_state = fsm_get_state();
    switch (fsm_get_state()) {
        case GS_MENU  : 
          ui_menu_run(); 
          break;
        case GS_EXIT  : 
          ui_menu_run(); 
          break;
        case GS_PLAY  : 
          ui_play_run(); 
          break;
        case GS_PAUSE : 
          ui_play_run(); 
          break;
        case GS_OVER  : 
          ui_play_run(); 
          break;
        case SYS_HALT : 
          ui_sys_halt_run(); 
          break;
        default : 
          console_out("[Error][ui] Unknow fsm state code: %d\n",fsm_get_state()); 
          log_out("[Error][ui] Unknow fsm state code: %d",fsm_get_state()); 
          sys_halt();
          break;
    }
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief ESC键在UI切换中的作用控制函数
 */
static void ui_esc_pressed_handler()
{
    game_state_t game_state = fsm_get_state();
    switch (game_state) {
    case GS_MENU:
        fsm_switch_state(GS_EXIT);
        console_out("[ui] State has been changed by ESC to %d\n",GS_EXIT);
        break;
    case GS_EXIT:
        fsm_switch_state(GS_MENU);
        console_out("[ui] State has been changed by ESC to %d\n",GS_MENU);
        break;
    case GS_PLAY:
        fsm_switch_state(GS_PAUSE);
        console_out("[ui] State has been changed by ESC to %d\n",GS_PAUSE);
        break;
    case GS_PAUSE:
        fsm_switch_state(GS_PLAY);
        console_out("[ui] State has been changed by ESC to %d\n",GS_PLAY);
        break;
    case GS_OVER:
        fsm_switch_state(GS_MENU);
        console_out("[ui] State has been changed by ESC to %d\n",GS_MENU);
    default:
        console_out("[ui] In this state,ESC has noting to do\n");
        break;
  }
}
