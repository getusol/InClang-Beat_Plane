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
#include "audio.h"

#include "ui_cg.h"
#include "ui_menu.h"
#include "ui_play.h"
#include "ui_shop.h"
#include "ui_base.h"

#include "ui_sys_halt.h"
#include "apr.h"
#include "ui_comm.h"

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
  // APR 外观模板必须在所有 UI 初始化之前加载（Base UI 需要预加载的图片描述符）
  apr_init();

  //各界面画图
  ui_cg_init();
  ui_menu_init();
  ui_play_init();
  ui_shop_init();
  ui_base_init();
  ui_comm_init();
  ui_sys_halt_init();
  //按键注册
  input_sw_register_press_callback(KEY_EVENT_B, ui_esc_pressed_handler);
  CONSOLE("[INFO] Ui initialization finished\n");
}
/**
 * @brief 根据当前游戏状态决定UI加载和音乐加载，
 *        不同加载函数位于相应的文件中 UI渲染已经在init创建完毕
 */
void ui_run()
{
    if (fsm_get_state() == last_game_state) return ;
    // 清理上一个状态的资源
    if (last_game_state == GS_CG) ui_cg_cleanup();
    switch (fsm_get_state()) {
        case GS_CG    :
          audio_load(AUDIO_CG,AUDIO_CHAN_BGM,false);
          ui_cg_run(); 
          break;
        case GS_MENU  : 
        CONSOLE("[INFO] Come to menu");
          audio_stop_all(); 
          ui_menu_run();
          break;
        case GS_PLAY  : 
          if (last_game_state == GS_MENU || last_game_state == GS_OVER) audio_load(AUDIO_BGM,AUDIO_CHAN_BGM,true);
          if (last_game_state == GS_PAUSE) audio_resume_all();
          ui_play_run(); 
          break;
        case GS_SHOP  : 
          ui_shop_run(); 
          break;
        case GS_BASE : 
          ui_base_run(); 
          break;
        case GS_PAUSE : 
          audio_pause_all();
          ui_play_run(); 
          break;
        case GS_OVER  :
          audio_stop_all();
          ui_play_run();
          break;
        case SYS_HALT : 
          audio_stop_all(); 
          ui_sys_halt_run(); 
          break;
        default : 
          CONSOLE("[Error] Unknow fsm state code: %d",fsm_get_state()); 
          LOG("[Error] Unknow fsm state code: %d",fsm_get_state()); 
          sys_halt();
          break;
    }
    last_game_state = fsm_get_state();
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
    case GS_CG:
        ui_cg_skip();
        fsm_switch_state(GS_MENU);
        CONSOLE("[INFO] CG skipped by user");
        break;
    case GS_PLAY:
        fsm_switch_state(GS_PAUSE);
        CONSOLE("[INFO] State has been changed by ESC to GS_PAUSE");
        break;
    case GS_PAUSE:
        fsm_switch_state(GS_PLAY);
        CONSOLE("[INFO] State has been changed by ESC to GS_PLAY");
        break;
    case GS_OVER:
        fsm_switch_state(GS_MENU);
        CONSOLE("[INFO] State has been changed by ESC to GS_MENU");
    default:
        CONSOLE("[INFO] No ESC action defined for current state %d", game_state);
        break;
  }
}
