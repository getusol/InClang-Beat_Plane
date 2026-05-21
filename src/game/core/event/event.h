/**
 * @file event.h
 */

#ifndef __EVENT_H__
#define __EVENT_H__

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
 * @brief 事件类型枚举
 */
typedef enum {
    EVENT_NONE = 0,         // 无事件

    EVENT_GAME_START,       // 游戏开始事件
    EVENT_GAME_OVER,        // 游戏结束事件(胜利)

    EVENT_BULLET_HIT_ENEMY, // 子弹击中敌人
    EVENT_PLAYER_HIT_ENEMY, // 敌人击中玩家
    EVENT_ENEMY_DESTROYED,  // 玩家击中敌人
    EVENT_BULLET_HIT_PLAYER,// 子弹击中玩家

    EVENT_PLAYER_DIE,       // 玩家死亡(失败)

    EVENT_COUNT             // 事件总数
} event_code_t;

/**
 * @brief 事件回调函数
 * @param source 事件发起者 可为NULL
 * @param target 事件目标对象 可为NULL
 */
typedef void (*event_callback_t)(game_obj_t * source,game_obj_t * target);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void event_init();
void event_dispatch(event_code_t code,game_obj_t * source,game_obj_t * target);
void event_register(event_code_t code,event_callback_t callback);

#endif // #ifndef __EVENT_H__
