/**
 * @file fsm.h
 * @brief 有限状态机
 */

/*********************
 *      INCLUDES
 *********************/
#include "fsm.h"
#include "debug.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static game_state_t current_state;

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 游戏状态初始化
 * @note 需要在main.c中调用一次
 */
void fsm_init(void)
{
  current_state = GS_MENU;
}

/**
 * @brief 获取当前游戏状态
 */
game_state_t fsm_get_state(void)
{
  return current_state;
}

/**
 * @brief 切换游戏状态
 * @param new_state 目标状态/新游戏状态
 */
void fsm_switch_state(game_state_t new_state)
{
  if (new_state >= GS_MAX) {
    console_out("[Error][fsm] Unknown state!\n");
    log_out("[Error][fsm] Unknown state!");
    sys_halt();
    return ;
  }
  current_state = new_state;
}
