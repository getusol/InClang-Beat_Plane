/**
 * @file game_object.h
 */

#ifndef __GAME_OBJECT_H__
#define __GAME_OBJECT_H__

/*********************
 *      INCLUDES
 *********************/

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>
#include "config.h"

/**********************
 *      TYPEDEFS
 **********************/

struct game_obj;

typedef void (*behave_func_t)(struct game_obj * g,void * v);

/**
 * @brief behave 结构体 敌人 子弹的 AI 玩家的 被动行为
 */
typedef struct behave_t {
    behave_func_t f;
    void * usr_data;
} behave_t;

/**
 * @brief 游戏对象类别
 */
typedef enum {
    GAME_OBJ_TYPE_PLAYER = 0,
    GAME_OBJ_TYPE_BULLET,
    GAME_OBJ_TYPE_ENEMY,
    GAME_OBJ_TYPE_COUNT,
    GAME_OBJ_TYPE_COIN
} game_obj_type_t;

/**
 * @brief 游戏对象父结构体，包含位置、大小、速度、对应的lvgl对象指针以及一些方法指针
 */
typedef struct game_obj {
    lv_coord_t x, y; //position
    uint16_t w, h;   //size
    int16_t hitbox_x, hitbox_y; //hitbox relative position
    uint16_t hitbox_w, hitbox_h;
    int8_t speed;   //@deprecated movement speed （dx / dt)
    int16_t vx,vy;  //velocity
    lv_obj_t * obj;  //the lvgl object representing this game object
    #if SHOW_HITBOX
    lv_obj_t * hitbox_obj; //the lvgl object representing this game object's hitbox
    #endif
    bool active;     //is the object active in the game
    game_obj_type_t type;
    behave_t behave; // AI
    bool timered;   // 是否有定时器 用在behave中初始化一次
    // general methods for game objects, like update, can be added here
    void (*update)(struct game_obj *self);
    void (*show)(struct game_obj *self);
    void (*hide)(struct game_obj *self);
} game_obj_t;

/**********************
 *      MACROS
 **********************/

#define NULL_BEHAVE ((behave_t){(behave_func_t)NULL,NULL})

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

// getters
lv_point_t game_obj_get_pos(const game_obj_t * obj);
uint16_t game_obj_get_width(const game_obj_t * obj);
uint16_t game_obj_get_height(const game_obj_t * obj);
float game_obj_get_speed(const game_obj_t * obj);
bool game_obj_is_active(const game_obj_t * obj);

// setters
bool game_obj_set_behave(game_obj_t * obj, behave_func_t f, void * usr_data);

#if SHOW_HITBOX
lv_obj_t * game_obj_hitbox_init(game_obj_t * obj);
void game_obj_hitbox_update(game_obj_t * obj);
#endif

#endif //#ifndef __GAME_OBJECT_H__
