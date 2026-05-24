/**
 * @file ui_menu.c
 * @brief 合并后的菜单界面：保留功能UI，统一逻辑框架
 */

/*********************
 * INCLUDES
 *********************/

#include "ui_menu.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fsm.h"
#include "ui_templates.h"
#include "lvgl_utils.h"
#include "event.h"         // 用于分发游戏开始事件
#include "tools.h"
#include "cg.h"
#ifndef SIMULATOR
#include "drivers.h"
#include "bgm.h"
#endif

/*********************
 *      MARCOS
 *********************/

#define BG_IMG_NAME "menu_bg.bin"

/**********************
 * STATIC PROTOTYPES
 **********************/

static void btn_level_event_cb(lv_event_t * e);
static void btn_shop_event_cb(lv_event_t * e);
static void btn_base_event_cb(lv_event_t * e);

/**********************
 * STATIC VARIABLES
 **********************/

static lv_obj_t * dp_menu;          // 主菜单屏幕对象
static lv_group_t * menu_group;     // 主菜单输入组

#ifdef SIMULATOR
static uint8_t * bg_img_buf = NULL;
static lv_img_dsc_t bg_img_struct;
#endif

/**********************
 * GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 菜单UI初始化
 */
void ui_menu_init(void)
{
    // 1. 初始化组
    menu_group = lv_group_create();

    // 2. 创建主屏幕对象
    dp_menu = lv_obj_create(NULL);
    lv_obj_clear_flag(dp_menu, LV_OBJ_FLAG_SCROLLABLE);

    // 3. 创建原有的功能按钮 (Level, Shop, Base)
    // Level 按钮
    lv_obj_t * btn_level = lv_btn_create(dp_menu);
    lv_obj_set_size(btn_level, 200, 90);
    lv_obj_set_pos(btn_level, 430, 271);
    lv_obj_add_event_cb(btn_level, btn_level_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(menu_group, btn_level);
    
    lv_obj_t * label_level = lv_label_create(btn_level);
    lv_label_set_text(label_level, "Level");
    lv_obj_center(label_level);

    // Shop 按钮
    lv_obj_t * btn_shop = lv_btn_create(dp_menu);
    lv_obj_set_size(btn_shop, 200, 90);
    lv_obj_set_pos(btn_shop, 430, 370);
    lv_obj_add_event_cb(btn_shop, btn_shop_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(menu_group, btn_shop);

    lv_obj_t * label_shop = lv_label_create(btn_shop);
    lv_label_set_text(label_shop, "Shop");
    lv_obj_center(label_shop);

    // Base 按钮
    lv_obj_t * btn_base = lv_btn_create(dp_menu);
    lv_obj_set_size(btn_base, 200, 90);
    lv_obj_set_pos(btn_base, 430, 469);
    lv_obj_add_event_cb(btn_base, btn_base_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(menu_group, btn_base);

    // 4. 创建背景图 PC 用 img_create_from_dsc MCU 节省空间 用直接读取
    #ifdef SIMULATOR
    char bg_img_path[64];
    lv_obj_t * bg_img = img_create_from_dsc(dp_menu, img_path(BG_IMG_NAME,bg_img_path,64), 1024, 600, bg_img_buf, &bg_img_struct, false);
    #else
    lv_obj_t * bg_img = lv_img_create(dp_menu);
    char bg_img_path[64];
    lv_img_set_src(bg_img,img_path(BG_IMG_NAME,bg_img_path,64));
    #endif

    lv_obj_t * label_base = lv_label_create(btn_base);
    lv_label_set_text(label_base, "Base");
    lv_obj_center(label_base);

    cg_play(dp_menu);
}

/**
 * @brief 加载并根据状态机运行菜单逻辑
 */
void ui_menu_run(void)
{
    lv_scr_load(dp_menu);
    set_group(menu_group);
}

/**********************
 * STATIC FUNCTIONS
 **********************/

/**
 * @brief 菜单中的 Level 按钮事件回调
 */
static void btn_level_event_cb(lv_event_t * e)
{
    fsm_switch_state(GS_PLAY);
    event_dispatch(EVENT_GAME_START, NULL, NULL); // 触发游戏开始逻辑
    CONSOLE("[INFO] Game start.");
}

/**
 * @brief 菜单中的 Shop 按钮事件回调
 */
static void btn_shop_event_cb(lv_event_t * e)
{
     CONSOLE("[INFO] Shop.");
}

static void btn_base_event_cb(lv_event_t * e)
{
    CONSOLE("[INFO] Base.");
}
