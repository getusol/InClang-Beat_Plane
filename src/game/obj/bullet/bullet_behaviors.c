/**
 * @file bullet_behaviors.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "bullet_behaviors.h"
#include <stdint.h>
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
  // CONSOLE("[INFO] Bullet speed: (%d, %d)", bullet->vx, bullet->vy);
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
  // CONSOLE("[INFO] ax = %f", ax);
  // CONSOLE("[INFO] Bullet speed: (%d, %d)", g->vx, g->vy);
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/
