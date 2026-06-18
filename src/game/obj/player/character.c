/**
 * @file character.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "character.h"
#include "skill.h"

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

static const character_config_t character_configs[CHARACTER_ID_MAX] = {
    [PLAYER] = {
        .id = PLAYER,
        .name = "Player",
        .apr_id = APR_PLAYER_DEFAULT,
        .hp_max = 200,
        .fire_cd = 160,
        .fire = skill_player_fire,
        .skill_x_cd = 3000,
        .skill_x = skill_triple_shot,
        .skill_y_cd = 5000,
        .skill_y = skill_shield,
        .skill_desc = "Triple: 3-way burst / Shield: 1s invincible",
    },
    [EMBER] = {
        .id = EMBER,
        .name = "Ember",
        .apr_id = APR_PLAYER_EMBER,
        .hp_max = 180,
        .fire_cd = 100,
        .fire = skill_ember_fire,
        .skill_x_cd = 500,
        .skill_x = skill_fire_bullet,
        .skill_y_cd = 1500,
        .skill_y = skill_flame_wall,
        .skill_desc = "Burn: DOT bullet / Flame Wall: clearing wave",
    },
    [STREAM] = {
        .id = STREAM,
        .name = "Stream",
        .apr_id = APR_PLAYER_STREAM,
        .hp_max = 210,
        .fire_cd = 150,
        .fire = skill_stream_fire,
        .skill_x_cd = 500,
        .skill_x = skill_ice_bullet,
        .skill_y_cd = 5000,
        .skill_y = skill_freeze,
        .skill_desc = "Freeze: immobilize / Slow: enemy bullets",
    },
    [VERDANT] = {
        .id = VERDANT,
        .name = "Verdant",
        .apr_id = APR_PLAYER_VERDANT,
        .hp_max = 260,
        .fire_cd = 200,
        .fire = skill_verdant_fire,
        .skill_x_cd = 1000,
        .skill_x = skill_accelerate,
        .skill_y_cd = 5000,
        .skill_y = skill_heal,
        .skill_desc = "Speed: double speed / Heal: +50 HP",
    },
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 通过id获取角色配置指针 只读
 * @param id 角色id
 * @return const character_config_t* 角色配置指针
 */
const character_config_t *character_get_config(character_id_t id)
{
    if (id >= CHARACTER_ID_MAX)
        return NULL;
    return &character_configs[id];
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
