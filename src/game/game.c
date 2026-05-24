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
#include "enemy.h"
#include "fsm.h"
#include "level.h"
#include "perf_monitor.h"
#include "game_object.h"
#include "timer.h"
#include "coin.h"

/**********************
 *      MACROS
 **********************/

#define MAX_GAME_OBJ_COUNT (MAX_BULLET_COUNT + 1 + MAX_ENEMY_COUNT + MAX_COIN_COUNT)


/**********************
 *      TYPEDEFS
 **********************/

/**********************
*  STATIC PROTOTYPES
**********************/

static bool rec_overlap(game_obj_t * obj1, game_obj_t * obj2);
static void check_collisions(void);
static void init_hitbox(game_obj_t * obj,void * usr_data);

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

    timer_init();

    level_init();

    // 程序启动时 初始化游戏对象
    player_init(play_display);
    bullet_init(play_display);
    enemy_init(play_display);
    coin_init(play_display);

    #if SHOW_HITBOX
    game_for_each_obj(init_hitbox,NULL);
    #endif


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
void game_update()
{
  uint32_t t_start = lv_tick_get();

  timer_update();

  for (int i = 0;i < free_idx;i++) {
    if (!game_obj_is_active(game_objs[i])) continue;
    if (game_objs[i]->update) game_objs[i]->update(game_objs[i]);
    if (game_objs[i]->behave.f) game_objs[i]->behave.f(game_objs[i],game_objs[i]->behave.usr_data);
    #if SHOW_HITBOX
    game_obj_hitbox_update(game_objs[i]);
    #endif
  }

  check_collisions();

  level_update();

  uint32_t t_end = lv_tick_get();

  perf_monitor_set_mspt(t_end - t_start);
}

/**
 * @brief 游戏对象遍历函数，负责遍历游戏对象管理器中的所有游戏对象 并执行相应操作
 * @param fuc 操作函数 接受 game_obj_t * 和 void *
 * @param usr_data 用户数据
 * @note 关于对象是否有效，是否活跃，需要操作函数负责
 */
void game_for_each_obj(void (*fuc)(game_obj_t * ,void *),void * usr_data)
{
  for (int i = 0;i < free_idx;i++) {
    fuc(game_objs[i],usr_data);
  }
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 检测两个游戏对象是否重叠
 */
static bool rec_overlap(game_obj_t * obj1, game_obj_t * obj2)
{
  // 获取 obj1 碰撞箱边界
    int16_t ax1 = obj1->x + obj1->hitbox_x;
    int16_t ay1 = obj1->y + obj1->hitbox_y;
    int16_t ax2 = ax1 + obj1->hitbox_w;
    int16_t ay2 = ay1 + obj1->hitbox_h;

    // 获取 obj2 碰撞箱边界
    int16_t bx1 = obj2->x + obj2->hitbox_x;
    int16_t by1 = obj2->y + obj2->hitbox_y;
    int16_t bx2 = bx1 + obj2->hitbox_w;
    int16_t by2 = by1 + obj2->hitbox_h;

    return (ax1 < bx2 && ax2 > bx1 && ay1 < by2 && ay2 > by1);
}

/**
 * @brief 全局碰撞检测 遍历已注册的对象 对活动对象进行碰撞检测 并派发相应的事件
 */
/**
 * @brief 全局碰撞检测 遍历已注册的对象 对活动对象进行碰撞检测 并派发相应的事件
 */
static void check_collisions(void)
{
    if (fsm_get_state() != GS_PLAY) return;
    
    for (int i = 0; i < free_idx; i++) {
        game_obj_t *a = game_objs[i];
        if (!a->active) continue;

        for (int j = i + 1; j < free_idx; j++) {
            game_obj_t *b = game_objs[j];
            if (!b->active) continue;

            // 只检测以下类型组合
            bool need_check = 
                (a->type == GAME_OBJ_TYPE_PLAYER && b->type == GAME_OBJ_TYPE_ENEMY) ||
                (a->type == GAME_OBJ_TYPE_ENEMY && b->type == GAME_OBJ_TYPE_PLAYER) ||
                (a->type == GAME_OBJ_TYPE_BULLET && b->type == GAME_OBJ_TYPE_ENEMY) ||
                (a->type == GAME_OBJ_TYPE_ENEMY && b->type == GAME_OBJ_TYPE_BULLET) ||
                (a->type == GAME_OBJ_TYPE_PLAYER && b->type == GAME_OBJ_TYPE_COIN)  ||
                (a->type == GAME_OBJ_TYPE_COIN && b->type == GAME_OBJ_TYPE_PLAYER);

            if (!need_check) continue;

            //  确保所有的事件派发都在 rec_overlap 成立的大括号内部！
            if (rec_overlap(a, b)) {
                
                // 1. 子弹 vs 敌人
                if (a->type == GAME_OBJ_TYPE_BULLET && b->type == GAME_OBJ_TYPE_ENEMY) {
                    if (bullet_get_source(a) == player_get_base()) {
                        event_dispatch(EVENT_BULLET_HIT_ENEMY, a, b);
                    }
                } 
                else if (a->type == GAME_OBJ_TYPE_ENEMY && b->type == GAME_OBJ_TYPE_BULLET) {
                    if (bullet_get_source(b) == player_get_base()) {
                        event_dispatch(EVENT_BULLET_HIT_ENEMY, b, a);
                    }
                }
                // 2. 玩家 vs 敌人
                else if (a->type == GAME_OBJ_TYPE_PLAYER && b->type == GAME_OBJ_TYPE_ENEMY) {
                    event_dispatch(EVENT_PLAYER_HIT_ENEMY, a, b);
                } 
                else if (a->type == GAME_OBJ_TYPE_ENEMY && b->type == GAME_OBJ_TYPE_PLAYER) {
                    event_dispatch(EVENT_PLAYER_HIT_ENEMY, b, a);
                }
                // 3. 玩家 vs 金币（现在正确嵌套在 if (rec_overlap) 内部了）
                else if (a->type == GAME_OBJ_TYPE_PLAYER && b->type == GAME_OBJ_TYPE_COIN) {
                    event_dispatch(EVENT_PLAYER_HIT_COIN, a, b);
                } 
                else if (a->type == GAME_OBJ_TYPE_COIN && b->type == GAME_OBJ_TYPE_PLAYER) {
                    event_dispatch(EVENT_PLAYER_HIT_COIN, b, a);
                }

                CONSOLE("[INFO] Collision detected between %d and %d", a->type, b->type);
            } // 用这个右括号正确闭合 if (rec_overlap(a, b))
        } // 闭合 for j
    } // 闭合 for i
} // 闭合 check_collisions 函数


/**
 * @brief 操作函数 初始化每个对象的碰撞箱
 */
static void init_hitbox(game_obj_t * obj,void * usr_data)
{
  LV_UNUSED(usr_data);
  #if SHOW_HITBOX
  game_obj_hitbox_init(obj);
  #endif
}

