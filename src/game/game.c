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

static void enemy_spawn_timer_func();
static bool rec_overlap(game_obj_t * obj1, game_obj_t * obj2);
static void check_collisions(void);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static game_obj_t * game_objs[MAX_GAME_OBJ_COUNT];
static uint8_t free_idx = 0;

static non_blocking_timer_t enemy_spawn_timer = {
  .func = enemy_spawn_timer_func,
  .tick_get = lv_tick_get,
  .delay_ms = 500, //间隔2ms
  .last_tick = 0
};

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

 /**
  * @brief 游戏初始化函数，负责初始化程序游戏相关内容
  */
void game_init()
{
    lv_obj_t * play_display = ui_play_get_display();
    // 程序启动时 初始化游戏对象
    player_init(play_display);
    bullet_init(play_display);
    enemy_init(play_display);

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

  check_collisions();

  non_blocking_delay(&enemy_spawn_timer);
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 敌人随机生成的定时器回调函数
 */
static void enemy_spawn_timer_func()
{
  if (fsm_get_state() != GS_PLAY) return ;
  lv_coord_t x = lv_rand(0,980);
  lv_coord_t y = -64;
  enemy_spawn(x,y);
}

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
static void check_collisions(void)
{
  for (int i = 0; i < free_idx; i++) {
    game_obj_t *a = game_objs[i];
    if (!a->active) continue;

    for (int j = i + 1; j < free_idx; j++) {
        game_obj_t *b = game_objs[j];
        if (!b->active) continue;

        // 只检测以下类型组合：
        // 1. 玩家  vs 敌人
        // 2. 子弹  vs 敌人
        bool need_check = 
            (a->type == GAME_OBJ_TYPE_PLAYER && b->type == GAME_OBJ_TYPE_ENEMY) ||
            (a->type == GAME_OBJ_TYPE_ENEMY && b->type == GAME_OBJ_TYPE_PLAYER) ||
            (a->type == GAME_OBJ_TYPE_BULLET && b->type == GAME_OBJ_TYPE_ENEMY) ||
            (a->type == GAME_OBJ_TYPE_ENEMY && b->type == GAME_OBJ_TYPE_BULLET);

        if (!need_check) continue;

        if (rec_overlap(a, b)) {
            // 根据组合派发事件
            if (a->type == GAME_OBJ_TYPE_BULLET && b->type == GAME_OBJ_TYPE_ENEMY) {
                event_dispatch(EVENT_BULLET_HIT_ENEMY, a, b);
            } else if (a->type == GAME_OBJ_TYPE_ENEMY && b->type == GAME_OBJ_TYPE_BULLET) {
                event_dispatch(EVENT_BULLET_HIT_ENEMY, b, a);
            } else if (a->type == GAME_OBJ_TYPE_PLAYER && b->type == GAME_OBJ_TYPE_ENEMY) {
                event_dispatch(EVENT_PLAYER_HIT_ENEMY, a, b);
            } else if (a->type == GAME_OBJ_TYPE_ENEMY && b->type == GAME_OBJ_TYPE_PLAYER) {
                event_dispatch(EVENT_PLAYER_HIT_ENEMY, b, a);
            }
            CONSOLE("[INFO] Collision detected between %d and %d", a->type, b->type);
        }
    }
  }
}
