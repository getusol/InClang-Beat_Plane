/**
 * @file skill.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "skill.h"
#include "game_object.h"
#include "player.h"
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
 * @brief 玩家普通攻击 频率较慢 伤害高
 * @note 频率只能由调用方保证
 */
void skill_player_fire(game_obj_t *obj)
{
    bullet_create(obj,
                  obj->x + obj->apr->w / 2 - obj->apr->w / 16,
                  obj->y - obj->apr->h / 4,
                  0, -20,
                  78,
                  NULL_BEHAVE,
                  APR_BULLET_DEFAULT);
}

/**
 * @brief Ember角色普通攻击 频率较快 伤害较低
 */
void skill_ember_fire(game_obj_t *obj)
{
    bullet_create(obj,
                  obj->x + obj->apr->w / 2 - obj->apr->w / 16,
                  obj->y - obj->apr->h / 4,
                  0, -22,
                  57,
                  NULL_BEHAVE,
                  APR_BULLET_EMBER);
}

/**
 * @brief Stream角色普通攻击 //TODO: 穿透伤害
 */
void skill_stream_fire(game_obj_t *obj)
{
    bullet_create(obj,
                  obj->x + obj->apr->w / 2 - obj->apr->w / 16,
                  obj->y - obj->apr->h / 4,
                  0, -25,
                  67,
                  NULL_BEHAVE,
                  APR_BULLET_STREAM);
}

/**
 * @brief Verdant角色普通攻击 //TODO: 吸血
 */
void skill_verdant_fire(game_obj_t *obj)
{
    bullet_create(obj,
                  obj->x + obj->apr->w / 2 - obj->apr->w / 16,
                  obj->y - obj->apr->h / 4,
                  0, -16,
                  40,
                  NULL_BEHAVE,
                  APR_BULLET_VERDANT);
}

/**
 * @brief 三连射技能 发射三颗神射手弹
 */
void skill_triple_shot(game_obj_t *obj)
{
    lv_coord_t cx = obj->x + obj->apr->w / 2 - obj->apr->w / 16;
    lv_coord_t cy = obj->y - obj->apr->h / 4;

    bullet_create(obj, cx, cy, 0, -20,
                  45, NULL_BEHAVE, APR_BULLET_MARKSMANROUND);
    bullet_create(obj, cx, cy, -1, -12,
                  55, NULL_BEHAVE, APR_BULLET_MARKSMANROUND);
    bullet_create(obj, cx, cy, 1, -12,
                  55, NULL_BEHAVE, APR_BULLET_MARKSMANROUND);
}

/**
 * @brief 屏盾技能 保护玩家免受伤害
 */
void skill_shield(game_obj_t *obj)
{
}

void skill_fire_bullet(game_obj_t *obj)
{
}

void skill_flame_wall(game_obj_t *obj)
{
}
void skill_ice_bullet(game_obj_t *obj)
{
}

void skill_freeze(game_obj_t *obj)
{
}
void skill_accelerate(game_obj_t *obj)
{
}

void skill_heal(game_obj_t *obj)
{
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
