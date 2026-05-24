/**
 * @file enemy_behaviors.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "enemy_behaviors.h"
#include "bullet.h"
#include "tools.h"
#include "timer.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

 /**********************
  *  STATIC PROTOTYPES
  **********************/

static void enemy_move_rand_timer(game_obj_t * g,void * v);
static void enemy_normal_shoot_timer(game_obj_t * g,void * v);

/**********************
 *  STATIC VARIABLES
 **********************/

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 普通敌人行为函数
 * 
 */
void enemy_behave_normal(game_obj_t * g,void * v)
{
   if (g == NULL || g->active == false) return ;
   if (!g->timered) {
    timer_create(g,500,TIMER_MODE_REPEAT,enemy_move_rand_timer,NULL);
    timer_create(g,900,TIMER_MODE_REPEAT,enemy_normal_shoot_timer,NULL);
    g->timered = true;
   }
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 敌人随机选取一个速度矢量运动500ms然后切换 速度大小固定为 5
 */
static void enemy_move_rand_timer(game_obj_t * g,void * v)
{
    (void) v;
    // 不需要做检测 timer 中有保险

    if (g->y < 40) {
        g->vx = 0;
        g->vy = 4;
        return ;
    }

    int16_t vx = lv_rand(-128,127);
    int16_t vy = lv_rand(-64,127);
    direction_to_velocity(vx,vy,5,&vx,&vy);
    g->vx = vx;
    g->vy = vy;
    return ;
}

/**
 * @brief 敌人射击计时器 向下发射一颗伤害为5，速度为 10~33 的子弹
 */
static void enemy_normal_shoot_timer(game_obj_t * g,void * v)
{
    if (g == NULL || g->active == false) return ;
    int8_t speed = lv_rand(10,33);
    bullet_create(g,g->x+g->w / 2 - 6,g->y+g->h,0,speed,5,NULL_BEHAVE);
}
