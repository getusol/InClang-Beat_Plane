/**
 * @file enemy_behaviors.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "enemy_behaviors.h"
#include "bullet.h"
#include "tools.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

 /**********************
  *  STATIC PROTOTYPES
  **********************/

static void enemy_normal_shoot(void * v);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static non_blocking_timer_t enemy_normal_shoot_timer = {
    .delay_ms = 600,
    .func = enemy_normal_shoot,
    .last_tick = 0,
    .tick_get = play_tick_get,
    .usr_data = NULL
};

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 普通敌人行为函数 入场后随机左右移动 发射直线子弹
 * 
 */
void enemy_behave_normal(game_obj_t * g,void * v)
{
    lv_coord_t y = g->y;
    lv_coord_t x = g->x;
    if (y < 100) {
        g->vy = 10;
        g->vx = 0;
        return ;
    }

    g->vx = lv_rand(0,10) - 5;
    g->vy = lv_rand(0,6) - 3;

    if (x < 200) {
        g->vx = 10;
    } else if (x > 500) {
        g->vx = -10;
    }
    if (y > 400) {
        g->vy = -20;
    }

    non_blocking_delay(&enemy_normal_shoot_timer,g,false);

}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 发射直线向下子弹 速度 0 -19 伤害 15
 */
static void enemy_normal_shoot(void * v)
{
    game_obj_t * g = (game_obj_t *)v;
    bullet_create(g,g->x + g->w / 2 - 6,g->y + g->h,0,19,15,NULL_BEHAVE);
}
