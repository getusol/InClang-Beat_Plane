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
#include "ui_setting.h"
#include "multiplayer.h"

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
 * @brief ui内容绘制函数 第一阶段 初始化根容器等可能与其它模块共享的资源  先调用
 */
void ui_init_stage1()
{
  ui_cg_init_stage1();
  ui_menu_init_stage1();
  ui_play_init_stage1();
  ui_setting_init_stage1();
  ui_shop_init_stage1();
  ui_base_init_stage1();
  ui_comm_init_stage1();
  ui_sys_halt_init_stage1();
}

/**
 * @brief ui内容绘制函数 第二阶段 初始化所有界面的基础渲染 最后调用
 */
void ui_init_stage2()
{
  // 各界面画图
  ui_cg_init_stage2();
  ui_menu_init_stage2();
  ui_play_init_stage2();
  ui_setting_init_stage2();
  ui_shop_init_stage2();
  ui_base_init_stage2();
  ui_comm_init_stage2();
  ui_sys_halt_init_stage2();
  // 按键注册
  input_sw_register_press_callback(KEY_EVENT_B, ui_esc_pressed_handler);
  CONSOLE_INFO("Ui initialization finished");
}

/**
 * @brief 根据当前游戏状态决定UI加载和音乐加载，
 *        不同加载函数位于相应的文件中 UI渲染已经在init创建完毕
 */
void ui_run()
{
  if (fsm_get_state() == last_game_state)
    return;
  // 清理上一个状态的资源
  if (last_game_state == GS_CG)
    ui_cg_cleanup();
  switch (fsm_get_state())
  {
  case GS_CG:
    audio_load(AUDIO_CG, AUDIO_CHAN_BGM, false);
    ui_cg_run();
    break;
  case GS_MENU:
    // CONSOLE_INFO("Come to menu");
    mp_exit_game(); // 临时修复
    audio_stop(AUDIO_CHAN_BGM);
    ui_menu_run();
    break;
  case GS_SETTING:
    ui_setting_run();
    break;
  case GS_COMM:
    audio_stop(AUDIO_CHAN_BGM);
    ui_comm_run();
    break;
  case GS_PLAY:
    if (last_game_state == GS_MENU || last_game_state == GS_OVER)
      audio_load(AUDIO_BGM, AUDIO_CHAN_BGM, true);
    if (last_game_state == GS_PAUSE)
      audio_resume_all();
    ui_play_run();
    break;
  case GS_SHOP:
    audio_load(AUDIO_SHOPMUSIC, AUDIO_CHAN_BGM, true);
    ui_shop_run();
    break;
  case GS_BASE:
    audio_load(AUDIO_BASEMUSIC, AUDIO_CHAN_BGM, true);
    ui_base_run();
    break;
  case GS_PAUSE:
    if (last_game_state == GS_SETTING)
      audio_load(AUDIO_BGM, AUDIO_CHAN_BGM, true);
    audio_pause(AUDIO_CHAN_BGM);
    ui_play_run();
    break;
  case GS_OVER:
    audio_stop_all();
    ui_play_run();
    break;
  case SYS_HALT:
    audio_stop_all();
    ui_sys_halt_run();
    break;
  default:
    CONSOLE_ERROR("Unknow fsm state code: %d", fsm_get_state());
    LOG_ERROR("Unknow fsm state code: %d", fsm_get_state());
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
  switch (game_state)
  {
  case GS_CG:
    ui_cg_skip();
    fsm_switch_state(GS_MENU);
    CONSOLE_INFO("CG skipped by user");
    break;
  case GS_BASE:
    fsm_switch_state(GS_MENU);
    CONSOLE_INFO("Come to menu");
    break;
  case GS_SHOP:
    ui_shop_esc_behave();
    break;
  case GS_PLAY:
    fsm_switch_state(GS_PAUSE);
    CONSOLE_INFO("State has been changed by ESC to GS_PAUSE");
    break;
  case GS_PAUSE:
    fsm_switch_state(GS_PLAY);
    CONSOLE_INFO("State has been changed by ESC to GS_PLAY");
    break;
  case GS_SETTING:
    fsm_switch_state(ui_setting_get_prev_state());
    CONSOLE_INFO("State has been changed by ESC from GS_SETTING");
    break;
  case GS_OVER:
    fsm_switch_state(GS_MENU);
    CONSOLE_INFO("State has been changed by ESC to GS_MENU");
  default:
    CONSOLE_INFO("No ESC action defined for current state %d", game_state);
    break;
  }
}
