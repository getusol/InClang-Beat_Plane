/**
 * @file game_object.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "game_object.h"
#include "apr.h"
#include "tools.h"
#include "settings.h"

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 获取游戏对象位置
 * @param obj 游戏对象指针
 * @return lv_point_t 游戏对象的位置坐标
 */
lv_point_t game_obj_get_pos(const game_obj_t *obj)
{
    return obj ? (lv_point_t){obj->x, obj->y} : (lv_point_t){0, 0};
}

/**
 * @brief 获取游戏对象宽度
 * @param obj 游戏对象指针
 * @return uint16_t 游戏对象的宽度
 */
uint16_t game_obj_get_width(const game_obj_t *obj)
{
    return (obj && obj->apr) ? obj->apr->w : 0;
}

/**
 * @brief 获取游戏对象高度
 * @param obj 游戏对象指针
 * @return uint16_t 游戏对象的高度
 */
uint16_t game_obj_get_height(const game_obj_t *obj)
{
    return (obj && obj->apr) ? obj->apr->h : 0;
}

/**
 * @brief 获取游戏对象速度
 * @param obj 游戏对象指针
 * @return float 游戏对象的速度
 */
float game_obj_get_speed(const game_obj_t *obj)
{
    return obj ? obj->speed : 0;
}

/**
 * @brief 判断游戏对象是否处于活跃状态
 * @param obj 游戏对象指针
 * @return bool true表示对象活跃，false表示对象不活跃或指针无效
 */
bool game_obj_is_active(const game_obj_t *obj)
{
    return obj ? obj->active : false;
}

/**
 * @brief 设置对象行为
 * @param obj 游戏对象指针
 * @param func 行为函数指针
 * @param usr_data 用户数据指针
 * @return bool 设置成功返回true，失败返回false
 */
bool game_obj_set_behave(game_obj_t *obj, behave_func_t func, void *usr_data)
{
    if (obj == NULL)
    {
        CONSOLE_WARNING("Game object is NULL. Cannot set behavior.");
        return false;
    }
    obj->behave.f = func;
    obj->behave.usr_data = usr_data;
    return true;
}

/**
 * @brief 获取外观指针
 */
const apr_t *game_obj_get_apr(const game_obj_t *obj)
{
    if (obj == NULL || obj->apr == NULL)
        return NULL;
    return obj->apr;
}
