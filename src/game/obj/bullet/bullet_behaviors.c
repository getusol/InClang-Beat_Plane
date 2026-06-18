/**
 * @file bullet_behaviors.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "bullet_behaviors.h"
#include "bullet.h"
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
 static void bullet_split_timer_cb(game_obj_t *g, void *v);
 static void bullet_split_fast_timer_cb(game_obj_t *g, void *v);

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
* @brief 这个函数会让子弹做正弦波浪运动
*        基于弹簧简谐振动: a_x = -omega² * (x - x0)
*        omega=2.8 → 周期约 2.3 秒，子弹在屏幕上形成 ~1.5 道完整波浪
* @note 子弹初始 vx 应为 0，x 位置决定振荡相位和幅度
*/
void bullet_behave_sine(game_obj_t *g, void *v)
{
  (void)v;
  // 弹簧简谐振动：x 越偏离中心，回复力越大，产生正弦波
  // x_norm ∈ [0, 1], center = 0.5(屏幕中央)
  float omega = 2.8f;
  float x_norm = g->x / (float)SCREEN_WIDTH;
  float ax = - omega * omega * (x_norm - 0.5f);
  g->vx += (int16_t)ax;
  // vy 保持不变，子弹一边下落一边左右摆动
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

/**
 * @brief 分裂子弹行为：子弹有节奏地分裂成2个、4个、8个（不再继续）
 *        gen 通过 behave.usr_data 传递（0=初代, 1=二代, 2=三代）
 *        每颗子弹经过 400ms 后分裂为 2 颗，gen+1 传递给子子弹
 *        gen >= 3 的子弹不再分裂，沿当前方向直线飞行
 * @param g 子弹对象指针
 * @param v 当前分裂代数 (通过 behave.usr_data 传入，0-2)
 */
void bullet_behave_split(game_obj_t *g, void *v)
{
    if (g == NULL || !g->active) return;

    uint8_t gen = (uint8_t)(uintptr_t)g->behave.usr_data;

    // gen >= 3 不再分裂，移除行为
    if (gen >= 3) {
        g->behave = NULL_BEHAVE;
        g->timered = false;
        return;
    }

    // 首次调用时启动分裂定时器
    if (!g->timered) {
        timer_create(g, 400, TIMER_MODE_ONCE, bullet_split_timer_cb, NULL);
        g->timered = true;
    }
}

/**
 * @brief 分裂定时器回调：创建2颗子子弹，销毁自身
 */
static void bullet_split_timer_cb(game_obj_t *g, void *v)
{
    (void)v;
    if (g == NULL || !g->active) return;

    uint8_t gen = (uint8_t)(uintptr_t)g->behave.usr_data;
    if (gen >= 3) return; // 不应到达这里

    // 获取发射源
    game_obj_t *source = bullet_get_source(g);

    // 根据代数决定扩散角度（代数越小，扩散角越大）
    float spread_angle;
    switch (gen) {
        case 0: spread_angle = 0.38f; break;  // ~22°
        case 1: spread_angle = 0.28f; break;  // ~16°
        case 2: spread_angle = 0.19f; break;  // ~11°
        default: return;
    }

    // 泰勒近似 sin/cos
    float sin_a = spread_angle - spread_angle * spread_angle * spread_angle / 6.0f;
    float cos_a = 1.0f - spread_angle * spread_angle / 2.0f;

    int16_t pvx = g->vx;
    int16_t pvy = g->vy;

    // 子子弹1：逆时针旋转 spread_angle
    int16_t c1_dx = (int16_t)(pvx * cos_a - pvy * sin_a);
    int16_t c1_dy = (int16_t)(pvx * sin_a + pvy * cos_a);

    // 子子弹2：顺时针旋转 spread_angle
    int16_t c2_dx = (int16_t)(pvx * cos_a + pvy * sin_a);
    int16_t c2_dy = (int16_t)(-pvx * sin_a + pvy * cos_a);

    // 归一化到固定速度 8
    int16_t c1_vx, c1_vy, c2_vx, c2_vy;
    direction_to_velocity(c1_dx, c1_dy, 8, &c1_vx, &c1_vy);
    direction_to_velocity(c2_dx, c2_dy, 8, &c2_vx, &c2_vy);

    // 子子弹行为：gen+1
    behave_t split_behave = {
        .f = bullet_behave_split,
        .usr_data = (void *)(uintptr_t)(gen + 1),
    };

    bullet_create(source, g->x, g->y, c1_vx, c1_vy, 50, split_behave, APR_BULLET_CIRCLE);
    bullet_create(source, g->x, g->y, c2_vx, c2_vy, 50, split_behave, APR_BULLET_CIRCLE);

    // 销毁自身
    g->hide(g);
}

/**
 * @brief 快速分裂子弹行为（200ms间隔，vs 普通 400ms）
 *        用于 THE END 最终Boss 的弹幕模式
 */
void bullet_behave_split_fast(game_obj_t *g, void *v)
{
    (void)v;
    if (g == NULL || !g->active) return;

    uint8_t gen = (uint8_t)(uintptr_t)g->behave.usr_data;

    if (gen >= 3) {
        g->behave = NULL_BEHAVE;
        g->timered = false;
        return;
    }

    if (!g->timered) {
        timer_create(g, 200, TIMER_MODE_ONCE, bullet_split_fast_timer_cb, NULL);
        g->timered = true;
    }
}

/**
 * @brief 边界反弹子弹行为：子弹碰到屏幕边界反弹
 *        预判下一帧位置，提前反转速度分量防止越界被销毁
 */
void bullet_behave_bounce(game_obj_t *g, void *v)
{
    (void)v;
    if (g == NULL || !g->active) return;

    int16_t next_x = g->x + g->vx;
    int16_t next_y = g->y + g->vy;

    if (next_x < 0 || next_x + (int16_t)g->apr->w > SCREEN_WIDTH) {
        g->vx = -g->vx;
    }
    if (next_y < 0 || next_y + (int16_t)g->apr->h > SCREEN_HEIGHT) {
        g->vy = -g->vy;
    }
}

/**
 * @brief 快速分裂定时器回调：创建2颗子子弹（快速版 200ms间隔），销毁自身
 */
static void bullet_split_fast_timer_cb(game_obj_t *g, void *v)
{
    (void)v;
    if (g == NULL || !g->active) return;

    uint8_t gen = (uint8_t)(uintptr_t)g->behave.usr_data;
    if (gen >= 3) return;

    game_obj_t *source = bullet_get_source(g);

    float spread_angle;
    switch (gen) {
        case 0: spread_angle = 0.38f; break;
        case 1: spread_angle = 0.28f; break;
        case 2: spread_angle = 0.19f; break;
        default: return;
    }

    float sin_a = spread_angle - spread_angle * spread_angle * spread_angle / 6.0f;
    float cos_a = 1.0f - spread_angle * spread_angle / 2.0f;

    int16_t pvx = g->vx;
    int16_t pvy = g->vy;

    int16_t c1_dx = (int16_t)(pvx * cos_a - pvy * sin_a);
    int16_t c1_dy = (int16_t)(pvx * sin_a + pvy * cos_a);
    int16_t c2_dx = (int16_t)(pvx * cos_a + pvy * sin_a);
    int16_t c2_dy = (int16_t)(-pvx * sin_a + pvy * cos_a);

    int16_t c1_vx, c1_vy, c2_vx, c2_vy;
    direction_to_velocity(c1_dx, c1_dy, 8, &c1_vx, &c1_vy);
    direction_to_velocity(c2_dx, c2_dy, 8, &c2_vx, &c2_vy);

    behave_t split_behave = {
        .f = bullet_behave_split_fast,
        .usr_data = (void *)(uintptr_t)(gen + 1),
    };

    bullet_create(source, g->x, g->y, c1_vx, c1_vy, 50, split_behave, APR_BULLET_CIRCLE);
    bullet_create(source, g->x, g->y, c2_vx, c2_vy, 50, split_behave, APR_BULLET_CIRCLE);

    g->hide(g);
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/
