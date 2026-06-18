/**
 * @file apr.c
 * @brief 统一的游戏对象外观模块实现
 */

/*********************
 *      INCLUDES
 *********************/

#include "apr.h"
#include "lvgl_utils.h"
#include "tools.h"
#include "debug.h"
#include <string.h>

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

/**
 * @brief 所有外观模板的静态数组，通过 apr_get() 查表获取
 */
static apr_t apr_list[APR_MAX];

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化所有外观模板，加载图片资源
 */
void apr_init()
{
    memset(apr_list, 0, sizeof(apr_list));
    char path[128];

    // ==================== 玩家飞机 ====================

    // APR_PLAYER_DEFAULT - 基础飞机
    apr_list[APR_PLAYER_DEFAULT] = (apr_t){
        .w = 64, .h = 64,
        .hitbox_x = 2, .hitbox_y = 22,
        .hitbox_w = 60, .hitbox_h = 20,
        .img_name = "player.bin",
        .is_alpha = true,
    };

    // APR_PLAYER_EMBER - 火焰飞机
    apr_list[APR_PLAYER_EMBER] = (apr_t){
        .w = 64, .h = 53,
        .hitbox_x = 2, .hitbox_y = 22,
        .hitbox_w = 60, .hitbox_h = 20,
        .img_name = "player_ember.bin",
        .is_alpha = false,
    };

    // APR_PLAYER_STREAM - 水流飞机
    apr_list[APR_PLAYER_STREAM] = (apr_t){
        .w = 64, .h = 53,
        .hitbox_x = 2, .hitbox_y = 22,
        .hitbox_w = 60, .hitbox_h = 20,
        .img_name = "player_stream.bin",
        .is_alpha = false,
    };

    // APR_PLAYER_VERDANT - 自然飞机
    apr_list[APR_PLAYER_VERDANT] = (apr_t){
        .w = 64, .h = 51,
        .hitbox_x = 2, .hitbox_y = 22,
        .hitbox_w = 60, .hitbox_h = 20,
        .img_name = "player_verdant.bin",
        .is_alpha = false,
    };

    // ==================== 子弹 ====================

    // APR_BULLET_DEFAULT - 默认子弹
    apr_list[APR_BULLET_DEFAULT] = (apr_t){
        .w = 6, .h = 16,
        .hitbox_x = 0, .hitbox_y = 0,
        .hitbox_w = 6, .hitbox_h = 16,
        .img_name = "bullet.bin",
        .is_alpha = false,
    };

    // APR_BULLET_EMBER - 火焰弹
    apr_list[APR_BULLET_EMBER] = (apr_t){
        .w = 6, .h = 16,
        .hitbox_x = 0, .hitbox_y = 0,
        .hitbox_w = 6, .hitbox_h = 16,
        .img_name = "bullet_ember.bin",
        .is_alpha = false,
    };

    // APR_BULLET_STREAM - 水弹
    apr_list[APR_BULLET_STREAM] = (apr_t){
        .w = 6, .h = 16,
        .hitbox_x = 0, .hitbox_y = 0,
        .hitbox_w = 6, .hitbox_h = 16,
        .img_name = "bullet_stream.bin",
        .is_alpha = false,
    };

    // APR_BULLET_VERDANT - 绿色弹
    apr_list[APR_BULLET_VERDANT] = (apr_t){
        .w = 6, .h = 16,
        .hitbox_x = 0, .hitbox_y = 0,
        .hitbox_w = 6, .hitbox_h = 16,
        .img_name = "bullet_verdant.bin",
        .is_alpha = false,
    };

    // APR_BULLET_CIRCLE - 圆形弹（碰撞框内缩，匹配实际圆形可见区域）
    apr_list[APR_BULLET_CIRCLE] = (apr_t){
        .w = 16, .h = 16,
        .hitbox_x = 2, .hitbox_y = 2,
        .hitbox_w = 12, .hitbox_h = 12,
        .img_name = "circle.bin",
        .is_alpha = true,
    };

    // APR_BULLET_TRIANGLE - 三角弹（碰撞框内缩，匹配实际三角形可见区域）
    apr_list[APR_BULLET_TRIANGLE] = (apr_t){
        .w = 15, .h = 16,
        .hitbox_x = 2, .hitbox_y = 2,
        .hitbox_w = 12, .hitbox_h = 12,
        .img_name = "triangle.bin",
        .is_alpha = true,
    };

    // ==================== 敌人 ====================

    // APR_ENEMY_DEFAULT
    apr_list[APR_ENEMY_DEFAULT] = (apr_t){
        .w = 64, .h = 50,
        .hitbox_x = 2, .hitbox_y = 22,
        .hitbox_w = 60, .hitbox_h = 20,
        .img_name = "enemy2.bin",
        .is_alpha = false,
    };
    // APR_ENEMY_BOSS
    apr_list[APR_ENEMY_BOSS] = (apr_t){
        .w = 64, .h = 48,
        .hitbox_x = 3, .hitbox_y = 33,
        .hitbox_w = 60, .hitbox_h = 20,
        .img_name = "boss.bin",
        .is_alpha = false,
    };
    // APR_ENEMY4 - 第3关起新敌人（圆形分裂弹）
    apr_list[APR_ENEMY4] = (apr_t){
        .w = 64, .h = 50,
        .hitbox_x = 2, .hitbox_y = 22,
        .hitbox_w = 60, .hitbox_h = 20,
        .img_name = "enemy4.bin",
        .is_alpha = false,
    };

    // APR_ENEMY_ENDBOSS - THE END 最终Boss
    apr_list[APR_ENEMY_ENDBOSS] = (apr_t){
        .w = 64, .h = 48,
        .hitbox_x = 3, .hitbox_y = 33,
        .hitbox_w = 60, .hitbox_h = 20,
        .img_name = "end.bin",
        .is_alpha = false,
    };

    // ==================== 金币 ====================

    // APR_COIN_DEFAULT
    apr_list[APR_COIN_DEFAULT] = (apr_t){
        .w = 18, .h = 18,
        .hitbox_x = 0, .hitbox_y = 0,
        .hitbox_w = 20, .hitbox_h = 20,
        .img_name = "coin.bin",
        .is_alpha = true,
    };

    
    // APR_FLAME_WALL - 火墙
    apr_list[APR_FLAME_WALL] = (apr_t){
        .w = 64, .h = 10,
        .hitbox_x = 0, .hitbox_y = 0,
        .hitbox_w = 64, .hitbox_h = 10,
        .img_name = "flame.bin",
        .is_alpha = false,
    };

    for (int i = 0; i < APR_MAX; i++) {
        if (apr_list[i].img_name == NULL) continue;
        load_img_dsc(img_path(apr_list[i].img_name, path, 128),
                     &apr_list[i].img_dsc,
                     apr_list[i].w, apr_list[i].h,
                     apr_list[i].is_alpha);
        CONSOLE_INFO("APR %d loaded: %s (%dx%d)", i, apr_list[i].img_name, apr_list[i].w, apr_list[i].h);
    }

}


/**
 * @brief 根据枚举值获取外观模板指针
 * @param id 外观模板ID
 * @return apr_t* 外观模板指针，无效ID返回默认玩家外观
 */
apr_t *apr_get(apr_id_t id)
{
    if (id >= APR_MAX) {
        CONSOLE_WARNING("Invalid APR id: %d, returning default", id);
        LOG_WARNING("Invalid APR id: %d, returning default", id);
        return &apr_list[APR_PLAYER_DEFAULT];
    }
    return &apr_list[id];
}

/**
 * @brief 将指定外观应用到游戏对象（更新图片、碰撞箱等）
 * @param obj 游戏对象指针
 * @param id 外观模板ID
 */
void apr_apply(game_obj_t *obj, apr_id_t id)
{
    if (obj == NULL) return;

    apr_t *apr = apr_get(id);
    obj->apr = (const apr_t *)apr;

    if (obj->obj == NULL) return;

    lv_img_set_src(obj->obj, &apr->img_dsc);

    // 更新碰撞框调试显示（尺寸和位置随外观变化）
    if (obj->hitbox_obj != NULL) {
        lv_obj_set_pos(obj->hitbox_obj, apr->hitbox_x, apr->hitbox_y);
        lv_obj_set_size(obj->hitbox_obj, apr->hitbox_w, apr->hitbox_h);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
