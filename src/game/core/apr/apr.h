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

/**
 * @brief 外观模板结构体，定义游戏对象的视觉属性
 * @note 外观是共享的（多个对象可共用同一个apr），LVGL图像对象(lv_obj_t)存储在game_obj中
 */
typedef struct apr_s {
    uint16_t w, h;                  // 尺寸
    int16_t hitbox_x, hitbox_y;     // 碰撞框相对位置
    uint16_t hitbox_w, hitbox_h;    // 碰撞框大小
    lv_img_dsc_t img_dsc;           // 图片描述符(模拟器预加载，MCU无效)
    const char * img_name;          // 图片名称 方便创建
    bool is_alpha;                  // 是否透明
} apr_t;

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void apr_init();
apr_t *apr_get(apr_id_t id);
void apr_apply(game_obj_t *obj, apr_id_t id);

#endif // #ifndef __APR_H__
