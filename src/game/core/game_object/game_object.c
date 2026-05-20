/**
 * @file game_object.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "game_object.h"
#include "tools.h"

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

 /**
  * @brief 获取游戏对象位置
  * @param obj 游戏对象指针
  * @return lv_point_t 游戏对象的位置坐标
  */
lv_point_t game_obj_get_pos(const game_obj_t * obj)
{
    return obj ? (lv_point_t){obj->x, obj->y} : (lv_point_t){0, 0};
}

/**
 * @brief 获取游戏对象宽度
 * @param obj 游戏对象指针
 * @return uint16_t 游戏对象的宽度
 */
uint16_t game_obj_get_width(const game_obj_t * obj)
{
    return obj ? obj->w : 0;
}

/**
 * @brief 获取游戏对象高度
 * @param obj 游戏对象指针
 * @return uint16_t 游戏对象的高度
 */
uint16_t game_obj_get_height(const game_obj_t * obj)
{
    return obj ? obj->h : 0;
}

/**
 * @brief 获取游戏对象速度
 * @param obj 游戏对象指针
 * @return float 游戏对象的速度
 */
float game_obj_get_speed(const game_obj_t * obj)
{
    return obj ? obj->speed : 0.0f;
}

/**
 * @brief 判断游戏对象是否处于活跃状态
 * @param obj 游戏对象指针
 * @return bool true表示对象活跃，false表示对象不活跃或指针无效
 */
bool game_obj_is_active(const game_obj_t * obj)
{
    return obj ? obj->active : false;
}

#if SHOW_HITBOX

/**
 * @brief 初始化游戏对象的碰撞框
 * @param obj 游戏对象指针
 * @return lv_obj_t* 碰撞框对象
 * @note 此函数会自动填充 obj->hitbox_obj
 */
lv_obj_t * game_obj_hitbox_init(game_obj_t * obj)
{
    if (obj == NULL || obj->obj == NULL) {
        return NULL;
    }
    if (obj->hitbox_obj) {
        CONSOLE("[INFO] Hitbox object already exists. It will be deleted.");
        lv_obj_del(obj->hitbox_obj);
        obj->hitbox_obj = NULL;
        CONSOLE("[INFO] Hitbox object already exists. It has been deleted.");
    }

    lv_obj_t * hitbox = lv_obj_create(obj->obj);

    if (hitbox == NULL) {
        CONSOLE("[WARNING] Failed to create hitbox object.");
        return NULL;
    }

    // 移除默认样式，设置为纯线框
    lv_obj_remove_style_all(hitbox);
    lv_obj_set_style_border_width(hitbox, 1, 0);   // 红色边框
    lv_obj_set_style_border_color(hitbox, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_border_opa(hitbox, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(hitbox, LV_OPA_0, 0);  // 填充透明
    lv_obj_set_style_radius(hitbox, 0, 0);        // 无圆角
    lv_obj_clear_flag(hitbox, LV_OBJ_FLAG_CLICKABLE); // 不可交互

    // 设置相对位置和大小（相对于图片）
    lv_obj_set_pos(hitbox, obj->hitbox_x, obj->hitbox_y);
    lv_obj_set_size(hitbox, obj->hitbox_w, obj->hitbox_h);

    obj->hitbox_obj = hitbox;

    CONSOLE("[INFO] Hitbox object created.");

    return hitbox;
}

/**
 * @brief 更新游戏对象的碰撞框
 * @param obj 游戏对象指针
 */
void game_obj_hitbox_update(game_obj_t * obj)
{
    if (obj == NULL || obj->hitbox_obj == NULL) {
        return ;
    }
    if (obj->active) {
        lv_obj_clear_flag(obj->hitbox_obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(obj->hitbox_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

#endif

/**
 * @brief 设置对象行为
 * @param obj 游戏对象指针
 * @param func 行为函数指针
 * @param usr_data 用户数据指针
 * @return bool 设置成功返回true，失败返回false
 */
bool game_obj_set_behave(game_obj_t * obj, behave_func_t func, void * usr_data)
{
    if (obj == NULL) {
        CONSOLE("[WARNING] Game object is NULL. Cannot set behavior.");
        return false;
    }
    obj->behave.f = func;
    obj->behave.usr_data = usr_data;
    return true;
}
