/**
 * @file player_behaviors.c
 * @brief 玩家控制行为实现
 */

/*********************
 *      INCLUDES
 *********************/
#include "player_behaviors.h"
#include "player.h"
#include "character.h"
#include "input_device.h"
#include "config.h"
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
 * @brief 玩家受控行为 — 每帧由 game_update() → behave.f() 调用
 */
void player_control(game_obj_t *g, void *v)
{
    if (g == NULL || !g->active || v == NULL) return;

    input_device_t *dev = (input_device_t *)v;
    const character_config_t *cfg = player_character_get(g);
    if (cfg == NULL) return;

    /* ---- 1. 移动: 摇杆 → 速度 ---- */
    g->vx = (int16_t)((int32_t)dev->x() * g->speed / JOY_MAX_VALUE);
    g->vy = (int16_t)((int32_t)dev->y() * g->speed / JOY_MAX_VALUE);

    /* ---- 2. 普攻: 按住 KEY_A 连发, CD 控制 ---- */
    if (dev->down(KEY_A) && cfg->fire != NULL) {
        uint32_t now = play_tick_get();
        if (now - player_fire_last_tick_get(g) >= (uint32_t)cfg->fire_cd) {
            player_fire_last_tick_set(g, now);
            cfg->fire(g);
        }
    }

    /* ---- 3. 技能 X: KEY_X 单次按下 ---- */
    if (dev->pressed(KEY_X) && cfg->skill_x != NULL) {
        uint32_t now = play_tick_get();
        if (now - player_skill_x_last_use_get(g) >= (uint32_t)cfg->skill_x_cd) {
            player_skill_x_last_use_set(g, now);
            cfg->skill_x(g);
        }
    }

    /* ---- 4. 技能 Y: KEY_Y 单次按下 ---- */
    if (dev->pressed(KEY_Y) && cfg->skill_y != NULL) {
        uint32_t now = play_tick_get();
        if (now - player_skill_y_last_use_get(g) >= (uint32_t)cfg->skill_y_cd) {
            player_skill_y_last_use_set(g, now);
            cfg->skill_y(g);
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
