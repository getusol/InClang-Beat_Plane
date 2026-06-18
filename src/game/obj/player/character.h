/**
 * @file character.h
 * @brief 定义了角色枚举和静态属性
 */

#ifndef __CHARACTER_H__
#define __CHARACTER_H__

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>
#include "game_object.h"
#include "apr.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief 角色ID枚举 作为索引 不仅和角色名称相关 还和角色属性相关 以及影响到UI_BASE中的选择
 */
typedef enum
{
    PLAYER = 0,
    EMBER,
    STREAM,
    VERDANT,

    CHARACTER_ID_MAX,
} character_id_t;

/**
 * @brief 角色属性、技能指针结构体 用于存储角色的属性和技能指针 一般只读
 */
typedef struct
{
    character_id_t id;
    const char *name;
    apr_id_t apr_id;
    int16_t hp_max;
    int16_t fire_cd;
    void (*fire)(game_obj_t *player);
    int16_t skill_x_cd;
    void (*skill_x)(game_obj_t *player);
    int16_t skill_y_cd;
    void (*skill_y)(game_obj_t *player);
    const char *skill_desc;
} character_config_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

const character_config_t *character_get_config(character_id_t id);

#endif
