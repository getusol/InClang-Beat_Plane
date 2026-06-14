/**
 * @file hitbox.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "hitbox.h"
#include "apr.h"
#include "settings.h"
#include "debug.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

// 配置项
static bool show_hitbox = false; // 是否显示碰撞箱

static setting_t settings[] = {
    {.module = "Game", .name = "Show Hitbox", .type = ST_BOOL, .data = &show_hitbox, .bool_data = {.def = false}},
};

static uint8_t settings_count = sizeof(settings) / sizeof(settings[0]);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 为一个游戏对象创建一个碰撞框调试控件
 * @param obj 游戏对象指针
 * @param v 未使用参数
 * @return lv_obj_t* 碰撞框调试控件指针
 */
void hitbox_create(game_obj_t *obj, void *v)
{
    LV_UNUSED(v);
    if (obj == NULL || obj->obj == NULL || obj->apr == NULL)
    {
        return;
    }

    if (obj->hitbox_obj)
    {
        CONSOLE_INFO("Hitbox object already exists. It will be deleted.");
        lv_obj_del(obj->hitbox_obj);
        obj->hitbox_obj = NULL;
        CONSOLE_INFO("Hitbox object already exists. It has been deleted.");
    }

    lv_obj_t *hitbox = lv_obj_create(obj->obj);

    if (hitbox == NULL)
    {
        CONSOLE_WARNING("Failed to create hitbox object.");
        return;
    }

    // 移除默认样式，设置为纯线框
    lv_obj_remove_style_all(hitbox);
    lv_obj_set_style_border_width(hitbox, 1, 0);
    lv_obj_set_style_border_color(hitbox, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_border_opa(hitbox, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(hitbox, LV_OPA_0, 0);
    lv_obj_set_style_radius(hitbox, 0, 0);
    lv_obj_clear_flag(hitbox, LV_OBJ_FLAG_CLICKABLE);

    // 设置相对位置和大小（相对于图片）
    lv_obj_set_pos(hitbox, obj->apr->hitbox_x, obj->apr->hitbox_y);
    lv_obj_set_size(hitbox, obj->apr->hitbox_w, obj->apr->hitbox_h);

    obj->hitbox_obj = hitbox;

    // CONSOLE_INFO("Hitbox object created.");
}

/**
 * @brief 更新一个游戏对象的碰撞框调试控件的可见性 位置和大小由外部设置
 * @param obj 游戏对象指针
 */
void hitbox_update(game_obj_t *obj)
{
    if (obj == NULL || obj->hitbox_obj == NULL)
    {
        return;
    }
    // 可见性
    if (!show_hitbox)
    {
        lv_obj_add_flag(obj->hitbox_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else if (obj->active)
    {
        lv_obj_clear_flag(obj->hitbox_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * @brief 初始化碰撞框调试控件
 */
void hitbox_init(void)
{
    // 注册配置项
    for (int i = 0; i < settings_count; i++)
    {
        settings_register(&settings[i]);
    }
}

/**
 * @brief 重新调整一个游戏对象的碰撞框调试控件的位置和大小
 * @param obj 游戏对象指针
 */
void hitbox_resize(game_obj_t *obj)
{
    if (obj == NULL || obj->hitbox_obj == NULL)
    {
        return;
    }
    // 位置
    lv_obj_set_pos(obj->hitbox_obj, obj->apr->hitbox_x, obj->apr->hitbox_y);
    // 大小
    lv_obj_set_size(obj->hitbox_obj, obj->apr->hitbox_w, obj->apr->hitbox_h);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
