/**
 * @file bullet_behaviors.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "bullet_behaviors.h"
#include "apr.h"
#include <stdint.h>
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
 static void bullet_track_timeout_cb(game_obj_t *g, void *v);

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
* @brief 这个函数会让子弹做圆周运动
*/
void bullet_behave_circle(game_obj_t *g, void *v)
{
  float theta = 0.12f; // radius determines how fast the bullet rotates
  // The formula is:
  // vx_next = vx * cos(theta) - vy * sin(theta)
  // vy_next = vx * sin(theta) + vy * cos(theta)
  float sin_theta = theta - theta * theta * theta / 6.0f;
  float cos_theta = 1.0f - theta * theta / 2.0f;
  int16_t c_vx = g->vx;
  int16_t c_vy = g->vy;
  int16_t n_vx = (int16_t)(c_vx * cos_theta - c_vy * sin_theta);
  int16_t n_vy = (int16_t)(c_vx * sin_theta + c_vy * cos_theta);
  direction_to_velocity(n_vx,n_vy,20,&g->vx,&g->vy);
  // CONSOLE_INFO("Bullet speed: (%d, %d)", bullet->vx, bullet->vy);
}

/**
* @brief 这个函数会让子弹做正弦运动
*/
void bullet_behave_sine(game_obj_t *g, void *v)
{
  float omega = 10.0f; // radius
  // The formula is:
  // a_x = - omega ^ 2 * (x - x0)
  float x = g->x / (float)SCREEN_WIDTH;
  float ax = - omega * omega * (x - 0.5f);
  g->vx += ax;
  // whereas g->vy stays the same
  // CONSOLE_INFO("ax = %f", ax);
  // CONSOLE_INFO("Bullet speed: (%d, %d)", g->vx, g->vy);
}

/**
 * @brief 追踪玩家子弹行为：子弹会追踪玩家一段时间（默认2秒），之后失去追踪能力沿当前方向直线飞行
 * @param g 子弹对象指针
 * @param v 玩家对象指针（通过 behave.usr_data 传入，调用时传入 player_get_base() ）
 * @note 追踪速度为 5，追踪持续时间为 2000ms，可根据需要调整
 */
void bullet_behave_track_player(game_obj_t *g, void *v)
{
    if (g == NULL || !g->active) return;
    if (v == NULL) return;

    game_obj_t *player = (game_obj_t *)v;

    // 首次调用时初始化定时器，到时间后停止追踪
    if (!g->timered) {
        timer_create(g, 2000, TIMER_MODE_ONCE, bullet_track_timeout_cb, NULL);
        g->timered = true;
    }

    // 计算指向玩家的方向，并设置速度
    int16_t dx = player->x + player->apr->w / 2 - g->x;
    int16_t dy = player->y + player->apr->h / 2 - g->y;

    int16_t vx, vy;
    direction_to_velocity(dx, dy, 8, &vx, &vy);
    g->vx = vx;
    g->vy = vy;

    // 旋转三角形尖端朝向玩家（参照开发板参考代码）
    int16_t angle_lvgl = cordic_atan2(dy, dx);
    lv_img_set_pivot(g->obj, game_obj_get_apr(g)->w / 2, game_obj_get_apr(g)->h / 2);
    lv_img_set_angle(g->obj, angle_lvgl);
}

/**
 * @brief 追踪超时回调，停止子弹追踪行为
 */
static void bullet_track_timeout_cb(game_obj_t *g, void *v)
{
    (void)v;
    g->behave = NULL_BEHAVE;
    g->timered = false;
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/
