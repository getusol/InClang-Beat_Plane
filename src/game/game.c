/**
 * @file game.h
 */

/*********************
 *      INCLUDES
 *********************/

#include "game.h"

#include "lvgl.h"
#include <stdint.h>

#include "tools.h"
#include "ui_play.h"
#include "config.h"
#include "event.h"
#include "player.h"
#include "bullet.h"

/**********************
 *      MACROS
 **********************/

#define MAX_GAME_OBJ_COUNT (MAX_BULLET_COUNT + 1 + MAX_ENEMY_COUNT)


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

static game_obj_t * game_objs[MAX_GAME_OBJ_COUNT];
static uint8_t free_idx = 0;

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

 /**
  * @brief 游戏初始化函数，负责初始化程序游戏相关内容
  */
void game_init()
{
    lv_obj_t * play_display = ui_play_get_display();
    //事件初始化
    event_init();
    // 程序启动时 初始化游戏对象
    player_init(play_display);
    bullet_init(play_display);

    CONSOLE("[INFO] Game objects initialization complete.");
    return ;
}

/**
 * @brief 游戏对象注册函数，负责将游戏对象添加到游戏对象管理器中
 * @return 注册编号 失败返回 -1
 */
int game_register_obj(game_obj_t * obj)
{
  if (obj == NULL) {
    return -1;
  }
  if (free_idx >= MAX_GAME_OBJ_COUNT) {
    CONSOLE("[WARNING] Game obj register failed,type: %d",obj->type);
    LOG("[WARNING] Game obj register failed,type: %d",obj->type);
    return -1;
  }
  game_objs[free_idx++] = obj;
  return free_idx - 1;
}

/**
 * @brief 游戏更新函数，负责更新游戏状态
 */
void game_update(void)
{
  for (int i = 0;i < free_idx;i++) {
    game_objs[i]->update(game_objs[i]);

    #if SHOW_HITBOX
    game_obj_hitbox_update(game_objs[i]);
    #endif
  }
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/
