/**
 * @file apr.h
 * @brief 统一的游戏对象外观模块 (Appearance)
 *        所有对象的外观模板集中管理，通过枚举+查表获取
 */

#ifndef __APR_H__
#define __APR_H__

/*********************
 *      INCLUDES
 *********************/

#include "game_object.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief 外观模板枚举，涵盖所有游戏对象类型
 */
typedef enum {
    // 玩家飞机
    APR_PLAYER_DEFAULT = 0,
    APR_PLAYER_EMBER,
    APR_PLAYER_STREAM,
    APR_PLAYER_VERDANT,
    // 子弹
    APR_BULLET_DEFAULT,
    APR_BULLET_EMBER,
    APR_BULLET_STREAM,
    APR_BULLET_VERDANT,
    APR_BULLET_CIRCLE,
    APR_BULLET_TRIANGLE,
    // 敌人
    APR_ENEMY_DEFAULT,
    APR_ENEMY_BOSS,
    // 金币
    APR_COIN_DEFAULT,

    APR_MAX
} apr_id_t;

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**
 * @brief 初始化所有外观模板，加载图片资源
 * @param parent 父LVGL对象，用于创建图片控件
 */
void apr_init(lv_obj_t *parent);

/**
 * @brief 根据枚举值获取外观模板指针
 * @param id 外观模板ID
 * @return apr_t* 外观模板指针，无效ID返回默认玩家外观
 */
apr_t *apr_get(apr_id_t id);

/**
 * @brief 将指定外观应用到游戏对象（更新图片、碰撞箱等）
 * @param obj 游戏对象指针
 * @param id 外观模板ID
 */
void apr_apply(game_obj_t *obj, apr_id_t id);

#endif // #ifndef __APR_H__
