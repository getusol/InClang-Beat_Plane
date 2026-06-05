/**
 * @file enemy_behaviors.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "enemy_behaviors.h"
#include "bullet.h"
#include "bullet_behaviors.h"
#include "tools.h"
#include "timer.h"
#include "player.h"
#include "apr.h"
#include "coin.h"

/**********************
 *      MACROS
 **********************/

#define BOSS_TICK_MS         300    // Boss 主时钟间隔
#define BOSS_TICKS_PER_PHASE 8      // 每阶段 8 tick = 2.4s

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void enemy_move_rand_timer(game_obj_t * g, void * v);
static void enemy_normal_shoot_timer(game_obj_t * g, void * v);

static void boss_master_timer_cb(game_obj_t * g, void * v);
static void boss_fire_barrage(game_obj_t * g);
static void boss_fire_tracking(game_obj_t * g);

/**********************
 *  STATIC VARIABLES
 **********************/

static int boss_tick = 0;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void enemy_behave_normal(game_obj_t * g, void * v)
{
   if (v == BEHAVE_ON_DEATH) {
       lv_coord_t cx = g->x + (g->apr->w - 18) / 2;
       lv_coord_t cy = g->y + (g->apr->h - 18) / 2;
       coin_spawn(cx, cy, 50, 7, APR_COIN_DEFAULT);
       return;
   }
   if (g == NULL || g->active == false) return ;
   if (!g->timered) {
    timer_create(g, 500, TIMER_MODE_REPEAT, enemy_move_rand_timer, NULL);
    timer_create(g, 1500, TIMER_MODE_REPEAT, enemy_normal_shoot_timer, NULL);
    g->timered = true;
   }
}

/**
 * @brief Boss 两阶段循环攻击
 *
 * 阶段 0 (2.4s): 270° 对称弹幕，每 600ms 一发
 * 阶段 1 (2.4s): 1 颗追踪弹，每 900ms
 */
void enemy_behave_boss(game_obj_t * g, void * v)
{
    if (v == BEHAVE_ON_DEATH) {
        lv_coord_t cx = g->x + (g->apr->w - 18) / 2;
        lv_coord_t cy = g->y + (g->apr->h - 18) / 2;
        for (int i = 0; i < 8; i++) {
            coin_spawn(cx + lv_rand(-30, 30), cy + lv_rand(-30, 30), 60, 0, APR_COIN_DEFAULT);
        }
        return;
    }
    if (g == NULL || !g->active) return;

    if (!g->timered) {
        g->vx = 0;
        g->vy = 0;
        g->x = SCREEN_WIDTH / 2 - g->apr->w / 2;
        g->y = 50;
        lv_obj_set_pos(g->obj, g->x, g->y);

        boss_tick = 0;

        timer_create(g, BOSS_TICK_MS, TIMER_MODE_REPEAT, boss_master_timer_cb, NULL);

        g->timered = true;
        CONSOLE_INFO("Boss activated at (%d, %d)", g->x, g->y);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void enemy_move_rand_timer(game_obj_t * g, void * v)
{
    (void) v;

    if (g->y < 40) {
        g->vx = 0;
        g->vy = 4;
        return ;
    }

    int16_t vx = lv_rand(-128, 127);
    int16_t vy = lv_rand(-64, 127);
    direction_to_velocity(vx, vy, 5, &vx, &vy);
    g->vx = vx;
    g->vy = vy;
}

static void enemy_normal_shoot_timer(game_obj_t * g, void * v)
{
    if (g == NULL || g->active == false) return ;
    int8_t speed = lv_rand(10, 33);
    bullet_create(g, g->x + g->apr->w / 2 - 6, g->y + g->apr->h, 0, speed, 5, NULL_BEHAVE, APR_BULLET_CIRCLE);
}

// ==================== Boss 行为实现 ====================

static void boss_master_timer_cb(game_obj_t * g, void * v)
{
    if (g == NULL || !g->active) return;

    int phase = (boss_tick / BOSS_TICKS_PER_PHASE) % 2;
    int phase_tick = boss_tick % BOSS_TICKS_PER_PHASE;

    switch (phase) {
        case 0:
            // 270° 对称弹幕 —— 每 2 tick (600ms)
            if (phase_tick % 2 == 0) {
                boss_fire_barrage(g);
            }
            break;

        case 1:
            // 追踪弹 —— 每 3 tick (900ms)
            if (phase_tick % 3 == 0) {
                boss_fire_tracking(g);
            }
            break;
    }

    if (phase_tick == 0) {
        //CONSOLE_INFO(" Boss entering phase %d", phase);
    }

    boss_tick++;
}

/**
 * @brief 270° 对称弹幕（参照 360°圆形弹幕，裁掉头顶 90°）
 *        8 颗 circle.bin，均匀覆盖玩家方向，兼顾 MCU 渲染性能
 */
static void boss_fire_barrage(game_obj_t * g)
{
    lv_coord_t cx = g->x + g->apr->w / 2;
    lv_coord_t cy = g->y + g->apr->h / 2;

    // 270° 以正下方(90°)为中心: 从 -135° 到 +135°，步长 270°/7
    // 跳过正上方 90°（Boss 头顶方向，不会打到玩家）
    #define BARRAGE_COUNT 8
    float angle_start = -135.0f * 3.14159f / 180.0f;  // rad
    float angle_step  = (270.0f * 3.14159f / 180.0f) / (BARRAGE_COUNT - 1);

    for (int i = 0; i < BARRAGE_COUNT; i++) {
        float theta = angle_start + (float)i * angle_step;

        // Taylor 级数 sin/cos（避免 math.h，适配 MCU）
        float sin_t = theta - theta * theta * theta / 6.0f;
        float cos_t = 1.0f - theta * theta / 2.0f;

        int16_t vx, vy;
        direction_to_velocity((int16_t)(cos_t * 100), (int16_t)(sin_t * 100), 3, &vx, &vy);
        bullet_create(g, cx, cy, vx, vy, 8, NULL_BEHAVE, APR_BULLET_CIRCLE);
    }
}

/**
 * @brief 1 颗追踪弹 —— triangle.bin，追踪玩家 2 秒
 */
static void boss_fire_tracking(game_obj_t * g)
{
    game_obj_t *player = player_get_base();
    if (player == NULL || !player->active) return;

    lv_coord_t cx = g->x + g->apr->w / 2;
    lv_coord_t cy = g->y + g->apr->h / 2;

    behave_t track_behave = {
        .f = bullet_behave_track_player,
        .usr_data = (void *)player,
    };

    bullet_create(g, cx, cy, 0, 2, 20, track_behave, APR_BULLET_TRIANGLE);
}
