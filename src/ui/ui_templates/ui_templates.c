/**
 * @file ui_templates.h
 */

/*********************
 *      INCLUDES
 *********************/
#include "ui_templates.h"
#include "tools.h"
#include "lvgl.h"
#include <stdio.h>

/**********************
 *      MACROS
 **********************/

//popup default params:
#define POPUP_WIDTH 400
#define POPUP_HIGHT 500
#define POPUP_ALIGN LV_ALIGN_CENTER
#define POPUP_RADIUS 20
#define POPUP_BG_OPA LV_OPA_70
#define POPUP_BG_COLOR lv_color_hex(0x121212)
#define POPUP_BORDEN_COLOR lv_color_hex(0x4FC3F7)
#define POPUP_BORDEN_WIDTH 3


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

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/
/**
 * @brief 创建lvgl 弹窗组件
 * @param parent 该对象的父对象
 * @note 默认样式查阅本文件宏定义，支持修改（全局）和初始化后修改（单独）
 */
 lv_obj_t * popup_create(lv_obj_t * parent)
{
    lv_obj_t * popup = lv_obj_create(parent);
    lv_obj_set_size(popup,POPUP_WIDTH,POPUP_HIGHT);
    lv_obj_set_align(popup,POPUP_ALIGN);
    lv_obj_set_style_bg_color(popup,POPUP_BG_COLOR,LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(popup,POPUP_BG_OPA,LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(popup,POPUP_BORDEN_COLOR,LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(popup,POPUP_BORDEN_WIDTH,LV_STATE_DEFAULT);
    lv_obj_set_style_radius(popup,POPUP_RADIUS,LV_STATE_DEFAULT);
    lv_obj_move_foreground(popup);
    lv_obj_clear_flag(popup,LV_OBJ_FLAG_SCROLLABLE);
    return popup;
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/
