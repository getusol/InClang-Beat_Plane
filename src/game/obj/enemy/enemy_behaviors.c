/**
 * @file enemy_behaviors.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "enemy_behaviors.h"
#include "enemy.h"
#include <stdlib.h>
#include "bullet.h"
#include "bullet_behaviors.h"
#include "tools.h"
#include "timer.h"
#include "player.h"
#include "apr.h"
#include "coin.h"
#include "audio.h"

/**********************
 *      MACROS
 **********************/

#define BOSS_TICK_MS         300    // Boss 主时钟间隔
#define BOSS_TICKS_PER_PHASE 8      // 每阶段 8 tick = 2.4s

#define ENDBOSS_TICK_MS         300    // 最终Boss主时钟间隔
#define ENDBOSS_TICKS_PER_PHASE 8      // 每阶段 8 tick = 2.4s

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void enemy_move_rand_timer(game_obj_t * g, void * v);
static void enemy_normal_shoot_timer(game_obj_t * g, void * v);
static void enemy4_shoot_timer(game_obj_t * g, void * v);

static void boss_master_timer_cb(game_obj_t * g, void * v);
static void boss_fire_barrage(game_obj_t * g);
static void boss_fire_tracking(game_obj_t * g);
static void boss_fire_fan_burst(game_obj_t * g);
static void boss_fire_rotating_circle(game_obj_t * g);
static void boss_fire_wave(game_obj_t * g);
static void boss_enter_cb(game_obj_t * g,void * v);

static void endboss_master_timer_cb(game_obj_t * g, void * v);
static void endboss_enter_cb(game_obj_t * g, void * v);
static void endboss_fire_barrage_360(game_obj_t * g);
static void endboss_fire_converge(game_obj_t * g);
static void endboss_fire_converge_split(game_obj_t * g);
static void endboss_fire_bounce(game_obj_t * g);
static void endboss_fire_tracking_rhythm(game_obj_t * g);

/**********************
 *  STATIC VARIABLES
 **********************/

static int boss_tick = 0;
static int endboss_tick = 0;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void enemy_behave_normal(game_obj_t * g, void * v)
{
   if (v == BEHAVE_ON_DEATH) {

        // 音效
        audio_load(AUDIO_ENEMYDIE,AUDIO_CHAN_AUTO,false);

       lv_coord_t cx = g->x + (g->apr->w - 18) / 2;
       lv_coord_t cy = g->y + (g->apr->h - 18) / 2;
       coin_spawn(cx, cy, 50, 7, APR_COIN_DEFAULT);
       return;
   }
   if (g == NULL || g->active == false) return ;
   if (enemy_is_frozen(g)) return; // 冻结时不能行动
   if (!g->timered) {
    timer_create(g, 500, TIMER_MODE_REPEAT, enemy_move_rand_timer, NULL);
    timer_create(g, 1500, TIMER_MODE_REPEAT, enemy_normal_shoot_timer, NULL);
    g->timered = true;
   }
}

/**
 * @brief Boss 四阶段循环攻击
 *
 * 阶段 0 (2.4s): 270° 对称弹幕，每 300ms 8 发旋转扫射
 * 阶段 1 (2.4s): 追踪弹，每 1200ms 1 颗追踪玩家
 * 阶段 2 (2.4s): 追踪扇形弹幕，每 600ms 5 发扇形瞄准玩家
 * 阶段 3 (2.4s): 旋转双速齿轮弹幕，每 900ms 12 发快慢交替旋转
 */
void enemy_behave_boss(game_obj_t * g, void * v)
{
    if (v == BEHAVE_ON_DEATH) {

        audio_load(AUDIO_BOSSDIE,AUDIO_CHAN_AUTO,false);

        lv_coord_t cx = g->x + (g->apr->w - 18) / 2;
        lv_coord_t cy = g->y + (g->apr->h - 18) / 2;
        for (int i = 0; i < 8; i++) {
            coin_spawn(cx + lv_rand(-30, 30), cy + lv_rand(-30, 30), 60, 0, APR_COIN_DEFAULT);
        }
        return;
    }
    if (g == NULL || !g->active) return;
    if (enemy_is_frozen(g)) return; // 冻结时不能行动

    if (!g->timered) {
        g->vx = 0;
        g->vy = 5;
        g->x = SCREEN_WIDTH / 2 - g->apr->w / 2;
        g->y = 50;
        lv_obj_set_pos(g->obj, g->x, g->y);

        boss_tick = 0;

        timer_create(g, BOSS_TICK_MS, TIMER_MODE_REPEAT, boss_master_timer_cb, NULL);
        timer_create(g,500,TIMER_MODE_ONCE,boss_enter_cb,NULL);

        g->timered = true;
        CONSOLE_INFO("Boss activated at (%d, %d)", g->x, g->y);
    }
}

/**
 * @brief 第3关起新敌人行为（enemy4）—— 发射圆形分裂弹
 *        子弹有节奏地分裂：1→2→4→8，每颗伤害 50
 */
void enemy_behave_enemy4(game_obj_t * g, void * v)
{
    if (v == BEHAVE_ON_DEATH) {
        // 音效
        audio_load(AUDIO_ENEMYDIE, AUDIO_CHAN_AUTO, false);

        lv_coord_t cx = g->x + (g->apr->w - 18) / 2;
        lv_coord_t cy = g->y + (g->apr->h - 18) / 2;
        coin_spawn(cx, cy, 50, 7, APR_COIN_DEFAULT);
        return;
    }
    if (g == NULL || g->active == false) return;
    if (enemy_is_frozen(g)) return; // 冻结时不能行动
    if (!g->timered) {
        timer_create(g, 500, TIMER_MODE_REPEAT, enemy_move_rand_timer, NULL);
        timer_create(g, 2200, TIMER_MODE_REPEAT, enemy4_shoot_timer, NULL);
        g->timered = true;
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void enemy_move_rand_timer(game_obj_t * g, void * v)
{
    (void) v;
    if (enemy_is_frozen(g)) return; // 冻结时不能行动

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
    if (enemy_is_frozen(g)) return; // 冻结时不能射击
    int8_t speed = lv_rand(10, 33);
    audio_load(AUDIO_ENEMYATTACK,AUDIO_CHAN_AUTO,false);
    bullet_create(g, g->x + g->apr->w / 2 - 6, g->y + g->apr->h, 0, speed, 8, NULL_BEHAVE, APR_BULLET_CIRCLE);
}

/**
 * @brief enemy4 射击定时器：发射一颗会分裂的圆形子弹
 *        子弹初始向下慢速飞行（vy=6），400ms后首次分裂为2颗，
 *        再400ms后每颗分裂为2颗（共4颗），再400ms后分裂为8颗，之后直线飞行
 */
static void enemy4_shoot_timer(game_obj_t * g, void * v)
{
    (void)v;
    if (g == NULL || g->active == false) return;
    if (enemy_is_frozen(g)) return; // 冻结时不能射击

    audio_load(AUDIO_ENEMYATTACK, AUDIO_CHAN_AUTO, false);

    lv_coord_t cx = g->x + g->apr->w / 2;
    lv_coord_t cy = g->y + g->apr->h;

    // 初代分裂子弹（gen=0），慢速向下
    behave_t split_behave = {
        .f = bullet_behave_split,
        .usr_data = (void *)(uintptr_t)0,
    };

    bullet_create(g, cx, cy, 0, 6, 50, split_behave, APR_BULLET_CIRCLE);
}

// ==================== Boss 行为实现 ====================

static void boss_master_timer_cb(game_obj_t * g, void * v)
{
    if (g == NULL || !g->active) return;
    if (enemy_is_frozen(g)) return; // 冻结时不能攻击

    int phase = (boss_tick / BOSS_TICKS_PER_PHASE) % 5;
    int phase_tick = boss_tick % BOSS_TICKS_PER_PHASE;

    switch (phase) {
        case 0:
            // 270° 对称弹幕 —— 每 2 tick (600ms)
            if (phase_tick % 2 == 0) {
                boss_fire_barrage(g);
                audio_load(AUDIO_BOSSATTACK, AUDIO_CHAN_AUTO, false);
            }
            break;

        case 1:
            // 追踪弹 —— 每 3 tick (900ms)
            if (phase_tick % 3 == 0) {
                boss_fire_tracking(g);
            }
            break;

        case 2:
            // 追踪扇形弹幕 —— 每 2 tick (600ms)
            if (phase_tick % 2 == 0) {
                boss_fire_fan_burst(g);
            }
            break;

        case 3:
            // 旋转双速齿轮弹幕 —— 每 3 tick (900ms)
            if (phase_tick % 3 == 0) {
                boss_fire_rotating_circle(g);
            }
            break;

        case 4:
            // 波浪弹幕 —— 每 2 tick (600ms)
            if (phase_tick % 2 == 0) {
                boss_fire_wave(g);
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

    static int16_t offset_ag = 0;
    offset_ag = (offset_ag + 10) % 38;
    int16_t delta = offset_ag << 2;

    // 预先定义好的8个方向
    static const int16_t base_dx[8] = {  71,  99,  85,  33, -33, -85, -99, -71 };
    static const int16_t base_dy[8] = { -71, -11,  53,  94,  94,  53, -11, -71 };

    for (int i = 0; i < 8; i++) {
        int16_t vx, vy;
        int16_t dx0 = base_dx[i];
        int16_t dy0 = base_dy[i];
        int16_t dx = dx0 - ((dy0 * delta) >> 8);
        int16_t dy = dy0 + ((dx0 * delta) >> 8);
        direction_to_velocity(dx, dy, 13, &vx, &vy);
        bullet_create(g, cx, cy, vx, vy, 14, NULL_BEHAVE, APR_BULLET_CIRCLE);
    }
}

/**
 * @brief 1 颗追踪弹 —— triangle.bin，追踪玩家 2 秒
 */
static void boss_fire_tracking(game_obj_t * g)
{
    game_obj_t *p1 = player_get_base();
#ifdef SIMULATOR
    game_obj_t *p2 = player_get_p2_base();
    // 选择最近的活跃玩家作为追踪目标
    game_obj_t *player = p1;
    if (p2 && p2->active) {
        if (!p1 || !p1->active) player = p2;
        else {
            int d1 = abs(g->x - p1->x) + abs(g->y - p1->y);
            int d2 = abs(g->x - p2->x) + abs(g->y - p2->y);
            player = (d2 < d1) ? p2 : p1;
        }
    }
#else
    game_obj_t *player = p1;
#endif
    if (player == NULL || !player->active) return;

    lv_coord_t cx = g->x + g->apr->w / 2;
    lv_coord_t cy = g->y + g->apr->h / 2;

    behave_t track_behave = {
        .f = bullet_behave_track_player,
        .usr_data = (void *)player,
    };

    bullet_create(g, cx, cy, 0, 2, 25, track_behave, APR_BULLET_TRIANGLE);
}

/**
 * @brief 追踪扇形弹幕 —— Boss 瞄准玩家发射 5 发扇形弹
 *        扇形展开约 80°（±40°），中心指向玩家
 *        子弹间隔 ~20°，留出侧向闪避空间，考验玩家身法
 */
static void boss_fire_fan_burst(game_obj_t * g)
{
    // 选择最近的活跃玩家作为瞄准目标（复用原有双人逻辑）
    game_obj_t *p1 = player_get_base();
#ifdef SIMULATOR
    game_obj_t *p2 = player_get_p2_base();
    game_obj_t *player = p1;
    if (p2 && p2->active) {
        if (!p1 || !p1->active) player = p2;
        else {
            int d1 = abs(g->x - p1->x) + abs(g->y - p1->y);
            int d2 = abs(g->x - p2->x) + abs(g->y - p2->y);
            player = (d2 < d1) ? p2 : p1;
        }
    }
#else
    game_obj_t *player = p1;
#endif
    if (player == NULL || !player->active) return;

    lv_coord_t cx = g->x + g->apr->w / 2;
    lv_coord_t cy = g->y + g->apr->h / 2;

    // boss → 玩家 方向向量
    int16_t dx = player->x + player->apr->w / 2 - cx;
    int16_t dy = player->y + player->apr->h / 2 - cy;

    // 5 发扇形：角度 ±40°, ±20°, 0°（中心瞄准玩家）
    // 子弹间有 ~20° 空隙，玩家侧向移动即可躲避
    static const float fan_angles[5] = {
        -0.698f, -0.349f, 0.0f, 0.349f, 0.698f
    };

    for (int i = 0; i < 5; i++) {
        float theta = fan_angles[i];
        // Taylor 近似 sin/cos
        float sin_t = theta - theta * theta * theta / 6.0f;
        float cos_t = 1.0f - theta * theta / 2.0f;

        // 旋转方向向量
        int16_t rdx = (int16_t)(dx * cos_t - dy * sin_t);
        int16_t rdy = (int16_t)(dx * sin_t + dy * cos_t);

        int16_t vx, vy;
        direction_to_velocity(rdx, rdy, 10, &vx, &vy);
        bullet_create(g, cx, cy, vx, vy, 12, NULL_BEHAVE, APR_BULLET_CIRCLE);
    }

    audio_load(AUDIO_BOSSATTACK, AUDIO_CHAN_AUTO, false);
}

/**
 * @brief 旋转双速齿轮弹幕 —— 12 发全圆弹幕，快慢交替，缓慢旋转
 *        快弹 (速度14) 和慢弹 (速度6) 交替排列，快弹超越慢弹形成动态空隙
 *        每轮旋转约 2.7°，安全走廊持续移动，玩家需要跟节奏走位
 */
static void boss_fire_rotating_circle(game_obj_t * g)
{
    lv_coord_t cx = g->x + g->apr->w / 2;
    lv_coord_t cy = g->y + g->apr->h / 2;

    // 缓慢旋转：每轮增量 3（vs 旧弹幕的 10），约 2.7°/burst
    static int16_t offset_ag = 0;
    offset_ag = (offset_ag + 3) % 90;
    int16_t delta = offset_ag << 2;

    // 12 个均匀分布的方向（每 30°），半径 100 保证精度
    static const int16_t base_dx[12] = {
        100,  86,  50,   0, -50, -86,
       -100, -86, -50,   0,  50,  86
    };
    static const int16_t base_dy[12] = {
          0,  50,  86, 100,  86,  50,
          0, -50, -86, -100, -86, -50
    };

    for (int i = 0; i < 12; i++) {
        int16_t vx, vy;
        int16_t dx0 = base_dx[i];
        int16_t dy0 = base_dy[i];
        // 小角度旋转
        int16_t dx = dx0 - ((dy0 * delta) >> 8);
        int16_t dy = dy0 + ((dx0 * delta) >> 8);

        // 快慢交替：偶数索引快弹，奇数索引慢弹 → 齿轮效果
        int8_t speed = (i % 2 == 0) ? 14 : 6;
        direction_to_velocity(dx, dy, speed, &vx, &vy);
        bullet_create(g, cx, cy, vx, vy, 10, NULL_BEHAVE, APR_BULLET_CIRCLE);
    }

    audio_load(AUDIO_BOSSATTACK, AUDIO_CHAN_AUTO, false);
}

/**
 * @brief 波浪弹幕 —— 7 发正弦波圆形弹，从 Boss 下方水平排开
 *        每颗子弹垂直下落 + 弹簧简谐振动产生正弦曲线
 *        不同初始 x 位置 → 不同的振荡相位和幅度 → 交织波浪纹路
 *        考验玩家在波纹间穿梭的身法
 */
static void boss_fire_wave(game_obj_t * g)
{
    lv_coord_t cx = g->x + g->apr->w / 2;
    lv_coord_t cy = g->y + g->apr->h;

    // 7 发子弹水平等距排开（间距 30px，覆盖 ±90px）
    // 每颗不同 x 起点 → 弹簧振动相位/幅度不同 → 自然形成波浪
    static const int16_t x_offsets[7] = { -90, -60, -30, 0, 30, 60, 90 };

    behave_t sine_behave = {
        .f = bullet_behave_sine,
        .usr_data = NULL,
    };

    for (int i = 0; i < 7; i++) {
        lv_coord_t bx = cx + x_offsets[i];
        // vx=0, vy=6: 垂直下落，靠弹簧振动产生横向运动
        bullet_create(g, bx, cy, 0, 6, 10, sine_behave, APR_BULLET_CIRCLE);
    }

    audio_load(AUDIO_BOSSATTACK, AUDIO_CHAN_AUTO, false);
}

/**
 * @brief boss进场走一段
 */
static void boss_enter_cb(game_obj_t * g,void * v)
{
    g->vy = 0;
}

// ==================== THE END 最终Boss 行为实现 ====================

/**
 * @brief THE END 最终Boss 五阶段循环攻击
 *
 * 阶段 0 (2.4s): 360° 全面弹幕，2倍速度 旋转扫射
 * 阶段 1 (2.4s): 快速子弹从左右两边向中间收缩
 * 阶段 2 (2.4s): 左右收缩分裂弹（快速分裂版）
 * 阶段 3 (2.4s): 8波全周反弹弹，每波10颗
 * 阶段 4 (2.4s): 追踪弹有节奏地发射（5tick × 2发）
 */
void enemy_behave_endboss(game_obj_t * g, void * v)
{
    if (v == BEHAVE_ON_DEATH) {
        audio_load(AUDIO_BOSSDIE, AUDIO_CHAN_AUTO, false);

        lv_coord_t cx = g->x + (g->apr->w - 18) / 2;
        lv_coord_t cy = g->y + (g->apr->h - 18) / 2;
        for (int i = 0; i < 12; i++) {
            coin_spawn(cx + lv_rand(-30, 30), cy + lv_rand(-30, 30), 60, 0, APR_COIN_DEFAULT);
        }
        return;
    }
    if (g == NULL || !g->active) return;
    if (enemy_is_frozen(g)) return;

    if (!g->timered) {
        g->vx = 0;
        g->vy = 5;
        g->x = SCREEN_WIDTH / 2 - g->apr->w / 2;
        g->y = 50;
        lv_obj_set_pos(g->obj, g->x, g->y);

        endboss_tick = 0;

        timer_create(g, ENDBOSS_TICK_MS, TIMER_MODE_REPEAT, endboss_master_timer_cb, NULL);
        timer_create(g, 500, TIMER_MODE_ONCE, endboss_enter_cb, NULL);

        g->timered = true;
        CONSOLE_INFO("THE END Boss activated at (%d, %d)", g->x, g->y);
    }
}

// ==================== THE END Boss 内部函数 ====================

static void endboss_enter_cb(game_obj_t * g, void * v)
{
    (void)v;
    g->vy = 0;
}

/**
 * @brief 最终Boss主时钟回调 —— 5阶段循环
 */
static void endboss_master_timer_cb(game_obj_t * g, void * v)
{
    (void)v;
    if (g == NULL || !g->active) return;
    if (enemy_is_frozen(g)) return;

    int phase = (endboss_tick / ENDBOSS_TICKS_PER_PHASE) % 5;
    int phase_tick = endboss_tick % ENDBOSS_TICKS_PER_PHASE;

    switch (phase) {
        case 0:
            // 360° 全面弹幕 2倍速 —— 每 tick 发射 12 发
            if (phase_tick % 1 == 0) {
                endboss_fire_barrage_360(g);
                audio_load(AUDIO_BOSSATTACK, AUDIO_CHAN_AUTO, false);
            }
            break;

        case 1:
            // 左右夹击快速子弹 —— 每 tick 发射
            if (phase_tick % 1 == 0) {
                endboss_fire_converge(g);
                audio_load(AUDIO_BOSSATTACK, AUDIO_CHAN_AUTO, false);
            }
            break;

        case 2:
            // 左右夹击分裂弹（快速分裂）—— 每 tick 发射
            if (phase_tick % 1 == 0) {
                endboss_fire_converge_split(g);
                audio_load(AUDIO_BOSSATTACK, AUDIO_CHAN_AUTO, false);
            }
            break;

        case 3:
            // 反弹弹 8波 每波10颗 —— 每 tick 发射一波
            if (phase_tick % 1 == 0) {
                endboss_fire_bounce(g);
            }
            break;

        case 4:
            // 追踪弹有节奏发射 —— 前5 tick 每 tick 2发 = 10发
            if (phase_tick < 5) {
                endboss_fire_tracking_rhythm(g);
            }
            break;
    }

    endboss_tick++;
}

/**
 * @brief 360° 全面弹幕 —— 12 发全周圆形弹，2倍速度(~26)，2倍旋转速度
 *        比常规 Boss 弹幕更快更密，考验极限闪避
 */
static void endboss_fire_barrage_360(game_obj_t * g)
{
    lv_coord_t cx = g->x + g->apr->w / 2;
    lv_coord_t cy = g->y + g->apr->h / 2;

    static int16_t offset_ag = 0;
    offset_ag = (offset_ag + 20) % 84;   // 2倍旋转速度
    int16_t delta = offset_ag << 2;

    // 12 个均匀分布的方向（每 30°），覆盖 360°
    static const int16_t base_dx[12] = {
        100,  86,  50,   0, -50, -86,
       -100, -86, -50,   0,  50,  86
    };
    static const int16_t base_dy[12] = {
          0,  50,  86, 100,  86,  50,
          0, -50, -86, -100, -86, -50
    };

    for (int i = 0; i < 12; i++) {
        int16_t vx, vy;
        int16_t dx0 = base_dx[i];
        int16_t dy0 = base_dy[i];
        int16_t dx = dx0 - ((dy0 * delta) >> 8);
        int16_t dy = dy0 + ((dx0 * delta) >> 8);
        direction_to_velocity(dx, dy, 26, &vx, &vy);   // 2倍速度
        bullet_create(g, cx, cy, vx, vy, 50, NULL_BEHAVE, APR_BULLET_CIRCLE);
    }
}

/**
 * @brief 左右夹击快速子弹 —— 从屏幕左右边缘向中间收缩
 *        左侧子弹向右下，右侧子弹向左下，夹击玩家
 */
static void endboss_fire_converge(game_obj_t * g)
{
    lv_coord_t cy = g->y + g->apr->h / 2;

    // 左侧：3 颗高速子弹向右飞行
    for (int i = 0; i < 3; i++) {
        lv_coord_t y = cy + lv_rand(-50, 50);
        int16_t vx, vy;
        direction_to_velocity(80, 40, 16, &vx, &vy);
        bullet_create(g, 0, y, vx, vy, 50, NULL_BEHAVE, APR_BULLET_CIRCLE);
    }

    // 右侧：3 颗高速子弹向左飞行
    for (int i = 0; i < 3; i++) {
        lv_coord_t y = cy + lv_rand(-50, 50);
        int16_t vx, vy;
        direction_to_velocity(-80, 40, 16, &vx, &vy);
        bullet_create(g, SCREEN_WIDTH, y, vx, vy, 50, NULL_BEHAVE, APR_BULLET_CIRCLE);
    }
}

/**
 * @brief 左右夹击快速分裂弹 —— 与上同+每颗子弹继承分裂逻辑（快速版 200ms）
 */
static void endboss_fire_converge_split(game_obj_t * g)
{
    lv_coord_t cy = g->y + g->apr->h / 2;

    behave_t split_behave = {
        .f = bullet_behave_split_fast,
        .usr_data = (void *)(uintptr_t)0,
    };

    // 左侧：3 颗高速分裂弹
    for (int i = 0; i < 3; i++) {
        lv_coord_t y = cy + lv_rand(-50, 50);
        int16_t vx, vy;
        direction_to_velocity(80, 40, 16, &vx, &vy);
        bullet_create(g, 0, y, vx, vy, 50, split_behave, APR_BULLET_CIRCLE);
    }

    // 右侧：3 颗高速分裂弹
    for (int i = 0; i < 3; i++) {
        lv_coord_t y = cy + lv_rand(-50, 50);
        int16_t vx, vy;
        direction_to_velocity(-80, 40, 16, &vx, &vy);
        bullet_create(g, SCREEN_WIDTH, y, vx, vy, 50, split_behave, APR_BULLET_CIRCLE);
    }
}

/**
 * @brief 全周反弹弹幕 —— 8 波，每波 10 颗向 360° 均匀发射，遇边界反弹
 *        子弹持续在场内弹跳，玩家必须在弹雨中寻找缝隙
 */
static void endboss_fire_bounce(game_obj_t * g)
{
    lv_coord_t cx = g->x + g->apr->w / 2;
    lv_coord_t cy = g->y + g->apr->h / 2;

    behave_t bounce_behave = {
        .f = bullet_behave_bounce,
        .usr_data = NULL,
    };

    // 10 颗子弹均匀覆盖 360°
    for (int i = 0; i < 10; i++) {
        float angle = (float)i * 2.0f * 3.14159f / 10.0f;
        // Taylor 近似 sin/cos
        float sin_a = angle - angle * angle * angle / 6.0f;
        float cos_a = 1.0f - angle * angle / 2.0f;

        int16_t vx, vy;
        direction_to_velocity((int16_t)(cos_a * 100.0f), (int16_t)(sin_a * 100.0f),
                              10, &vx, &vy);
        bullet_create(g, cx, cy, vx, vy, 50, bounce_behave, APR_BULLET_CIRCLE);
    }

    audio_load(AUDIO_BOSSATTACK, AUDIO_CHAN_AUTO, false);
}

/**
 * @brief 有节奏追踪弹 —— 每 tick 发射 2 发追踪弹
 *        前 5 tick × 2 发 = 共 10 发，形成追踪弹连射节奏
 */
static void endboss_fire_tracking_rhythm(game_obj_t * g)
{
    game_obj_t *p1 = player_get_base();
#ifdef SIMULATOR
    game_obj_t *p2 = player_get_p2_base();
    game_obj_t *player = p1;
    if (p2 && p2->active) {
        if (!p1 || !p1->active) player = p2;
        else {
            int d1 = abs(g->x - p1->x) + abs(g->y - p1->y);
            int d2 = abs(g->x - p2->x) + abs(g->y - p2->y);
            player = (d2 < d1) ? p2 : p1;
        }
    }
#else
    game_obj_t *player = p1;
#endif
    if (player == NULL || !player->active) return;

    lv_coord_t cx = g->x + g->apr->w / 2;
    lv_coord_t cy = g->y + g->apr->h / 2;

    behave_t track_behave = {
        .f = bullet_behave_track_player,
        .usr_data = (void *)player,
    };

    // 每 tick 发射 2 发追踪弹，有节奏连射
    for (int i = 0; i < 2; i++) {
        int16_t dx = player->x + player->apr->w / 2 - cx;
        int16_t dy = player->y + player->apr->h / 2 - cy;

        // 两发之间略有角度偏移，形成微小扇面
        float offset = (i == 0) ? -0.12f : 0.12f;
        float sin_o = offset - offset * offset * offset / 6.0f;
        float cos_o = 1.0f - offset * offset / 2.0f;

        int16_t rdx = (int16_t)(dx * cos_o - dy * sin_o);
        int16_t rdy = (int16_t)(dx * sin_o + dy * cos_o);

        int16_t vx, vy;
        direction_to_velocity(rdx, rdy, 4, &vx, &vy);
        bullet_create(g, cx, cy, vx, vy, 50, track_behave, APR_BULLET_TRIANGLE);
    }

    audio_load(AUDIO_BOSSATTACK, AUDIO_CHAN_AUTO, false);
}
