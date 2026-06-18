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
#include "timer.h"
#include "flame_wall.h"
#include "ui_play.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void skill_shield_end_cb(game_obj_t *owner, void *usr_data);
static void skill_freeze_end_cb(game_obj_t *owner, void *usr_data);
static void skill_accelerate_end_cb(game_obj_t *owner, void *usr_data);

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
    bullet_create(obj, cx, cy, -4, -19,
                  55, NULL_BEHAVE, APR_BULLET_MARKSMANROUND);
    bullet_create(obj, cx, cy, 4, -19,
                  55, NULL_BEHAVE, APR_BULLET_MARKSMANROUND);
}

/**
 * @brief 屏盾技能 保护玩家免受伤害
 */
void skill_shield(game_obj_t *obj)
{
    if (player_shield_is_active(obj))
        return;
    player_shield_set_active(obj, true);
    lv_obj_t *shield_overlay = player_shield_get_obj(obj);
    lv_obj_clear_flag(shield_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(shield_overlay, obj->x, obj->y);
    timer_create(obj, 1000, TIMER_MODE_ONCE, skill_shield_end_cb, NULL);
}

void skill_fire_bullet(game_obj_t *obj)
{
    lv_coord_t cx = obj->x + obj->apr->w / 2 - obj->apr->w / 16;
    lv_coord_t cy = obj->y - obj->apr->h / 4;

    game_obj_t *bullet = bullet_create(obj, cx, cy, 0, -10,
                                       40, NULL_BEHAVE, APR_BULLET_EMBER);
    if (bullet)
    {
        bullet_set_flags(bullet, BULLET_FLAG_BURN);
    }
}

void skill_flame_wall(game_obj_t *obj)
{
    lv_coord_t cx = obj->x + obj->apr->w / 2 - 32;
    lv_coord_t cy = obj->y - obj->apr->h / 2;
    flame_wall_create(cx, cy, -8);
}

void skill_ice_bullet(game_obj_t *obj)
{
    lv_coord_t cx = obj->x + obj->apr->w / 2 - obj->apr->w / 16;
    lv_coord_t cy = obj->y - obj->apr->h / 4;

    game_obj_t *bullet = bullet_create(obj, cx, cy, 0, -10,
                                       30, NULL_BEHAVE, APR_BULLET_STREAM);
    if (bullet)
        bullet_set_flags(bullet, BULLET_FLAG_FREEZE);
}

void skill_freeze(game_obj_t *obj)
{
    bullet_set_enemy_slow(true);
    ui_play_set_freeze_overlay(true);
    timer_create(obj, 2000, TIMER_MODE_ONCE, skill_freeze_end_cb, NULL);
}

void skill_accelerate(game_obj_t *obj)
{
    obj->speed = 20;
    timer_create(obj, 800, TIMER_MODE_ONCE, skill_accelerate_end_cb, NULL);
}

void skill_heal(game_obj_t *obj)
{
    player_hp_modify(obj, 50);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 屏盾技能 结束回调
 * @param owner 屏盾所属玩家对象
 * @param usr_data 未使用
 */
static void skill_shield_end_cb(game_obj_t *owner, void *usr_data)
{
    (void)usr_data;
    if (owner == NULL)
        return;
    player_shield_set_active(owner, false);
    lv_obj_add_flag(player_shield_get_obj(owner), LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief 冰冻技能 结束回调
 */
static void skill_freeze_end_cb(game_obj_t *owner, void *usr_data)
{
    (void)owner;
    (void)usr_data;
    bullet_set_enemy_slow(false);
    ui_play_set_freeze_overlay(false);
}

/**
 * @brief 加速技能 结束回调
 */
static void skill_accelerate_end_cb(game_obj_t *owner, void *usr_data)
{
    (void)usr_data;
    owner->speed = 14;
}
