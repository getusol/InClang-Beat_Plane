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
 * @brief 外观模板结构体，定义游戏对象的视觉属性
 * @note 外观是共享的（多个对象可共用同一个apr），LVGL图像对象(lv_obj_t)存储在game_obj中
 */
typedef struct {
    uint16_t w, h;                  // 尺寸
    int16_t hitbox_x, hitbox_y;     // 碰撞框相对位置
    uint16_t hitbox_w, hitbox_h;    // 碰撞框大小
    const char *img_name;           // 图片文件名
    bool is_alpha;                  // 是否包含alpha通道
    lv_img_dsc_t img_dsc;           // 图片描述符(模拟器预加载，MCU无效)
} apr_t;

/**
 * @brief 游戏对象父结构体，包含位置、大小、速度、对应的lvgl对象指针以及一些方法指针
 */
typedef struct game_obj {
    lv_coord_t x, y;       // position
    int8_t speed;           // @deprecated movement speed (dx/dt)
    int16_t vx, vy;         // velocity
    const apr_t *apr;       // 外观指针（共享）
    lv_obj_t *obj;          // LVGL 图像控件（每个对象私有）
#if SHOW_HITBOX
    lv_obj_t *hitbox_obj;   // LVGL 碰撞框调试控件
#endif
    bool active;            // is the object active in the game
    game_obj_type_t type;
    behave_t behave;        // AI / 被动行为
    bool timered;           // 是否有定时器 用在behave中初始化一次
    // general methods for game objects
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
