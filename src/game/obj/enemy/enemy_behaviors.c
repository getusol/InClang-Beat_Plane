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

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 普通敌人行为函数 入场后随机左右移动 发射直线子弹
 * 
 */
void enemy_behave_normal(game_obj_t * g,void * v)
{
    lv_coord_t x = g->x;
    lv_coord_t y = g->y;

    // 入场
    if (y < 100) {
        g->vy = 10;
        g->vx = 0;
        return ;
    }

    
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/
