/**
 * @file ui_menu.h
 * @brief menu菜单所有的ui在这里
 */

/*********************
 *      INCLUDES
 *********************/
#include "ui_menu.h"

#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>

#include "fsm.h"
#include "tools.h"
#include "lvgl_utils.h"
#include "ui_templates.h"
#include "game.h"
#include "event.h"

#ifndef SIMULATOR
#include "drivers.h"
#endif


/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

 /**********************
  *  STATIC PROTOTYPES
  **********************/
static void start_btn_event_cb(lv_event_t * e);
static void exit_btn_event_cb(lv_event_t * e);
static void yes_exit_btn_event_cb(lv_event_t * e);
static void no_exit_btn_event_cb(lv_event_t * e);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/
//display
static lv_obj_t * menu_display;
//groups
static lv_group_t * menu_group;
static lv_group_t * exit_popup_group;
//popups
static lv_obj_t * exit_popup;
//imgs
//bg:
static lv_img_dsc_t menu_bg_struct;

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief menu的ui渲染初始化,所有menu相关lvgl控件都写在这里
 */
void ui_menu_init(void)
{
    //group initialize
    menu_group = lv_group_create();
    exit_popup_group = lv_group_create();
    //Parent object initialize
    menu_display = lv_obj_create(NULL);
    lv_obj_clear_flag(menu_display,LV_OBJ_FLAG_SCROLLABLE);
    //popup initialization
    exit_popup = popup_create(menu_display);
    lv_obj_set_size(exit_popup,380,150);
    lv_obj_set_style_bg_opa(exit_popup,LV_OPA_90,LV_STATE_DEFAULT);
    popup_hide(exit_popup);
    //background img
    char menu_bg_path[100];
    lv_obj_t * menu_bg_img = img_create_from_array(menu_display,img_path("menu_bg.bin",menu_bg_path,100),1024,600,NULL,&menu_bg_struct,false);
    //start button
    lv_obj_t * start_btn = lv_btn_create(menu_display);
    lv_obj_set_size(start_btn,250,50);
    lv_obj_set_pos(start_btn,120,300);
    lv_obj_add_event_cb(start_btn,start_btn_event_cb,LV_EVENT_PRESSED,NULL);
    lv_group_add_obj(menu_group,start_btn);
    //label for start button
    lv_obj_t * start_label = lv_label_create(start_btn);
    lv_obj_set_align(start_label,LV_ALIGN_CENTER);
    lv_label_set_text(start_label,"Start");
    //exit button
    lv_obj_t * exit_btn = lv_btn_create(menu_display);
    lv_obj_set_size(exit_btn,250,50);
    lv_obj_set_pos(exit_btn,120,380);
    lv_obj_add_event_cb(exit_btn,exit_btn_event_cb,LV_EVENT_PRESSED,NULL);
    lv_group_add_obj(menu_group,exit_btn);
    //yes btn for exit popup
    lv_obj_t * yes_exit_btn = lv_btn_create(exit_popup);
    lv_obj_set_size(yes_exit_btn,140,40);
    lv_obj_set_pos(yes_exit_btn,10,60);
    lv_obj_add_event_cb(yes_exit_btn,yes_exit_btn_event_cb,LV_EVENT_PRESSED,NULL);
    lv_group_add_obj(exit_popup_group,yes_exit_btn);
    //no btn for exit popup
    lv_obj_t * no_exit_btn = lv_btn_create(exit_popup);
    lv_obj_set_size(no_exit_btn,140,40);
    lv_obj_set_pos(no_exit_btn,185,60);
    lv_obj_add_event_cb(no_exit_btn,no_exit_btn_event_cb,LV_EVENT_PRESSED,NULL);
    lv_group_add_obj(exit_popup_group,no_exit_btn);
    //label for exit button
    lv_obj_t * exit_label = lv_label_create(exit_btn);
    lv_obj_set_align(exit_label,LV_ALIGN_CENTER);
    lv_label_set_text(exit_label,"Exit");
    //label for yes exit btn
    lv_obj_t * yes_exit_btn_label = lv_label_create(yes_exit_btn);
    lv_obj_set_align(yes_exit_btn_label,LV_ALIGN_CENTER);
    lv_label_set_text(yes_exit_btn_label,"YES");
    //label for no exit btn
    lv_obj_t * no_exit_btn_label = lv_label_create(no_exit_btn);
    lv_obj_set_align(no_exit_btn_label,LV_ALIGN_CENTER);
    lv_label_set_text(no_exit_btn_label,"NO");
    //label for exit popup
    lv_obj_t * exit_popup_label = lv_label_create(exit_popup);
    lv_obj_set_pos(exit_popup_label,120,0);
    lv_label_set_text(exit_popup_label,"Quit?");
    lv_obj_set_style_text_font(exit_popup_label,&lv_font_montserrat_36,LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(exit_popup_label,lv_color_hex(0xFFFFFF),LV_STATE_DEFAULT);
}

/**
 * @brief 加载menu界面
 */
void ui_menu_run(void)
{
    lv_scr_load(menu_display);

    if (fsm_get_state() == GS_EXIT) {
        popup_show(exit_popup);
        set_group(exit_popup_group);
    }
    else {
        popup_hide(exit_popup);
    }    
    if (fsm_get_state() == GS_MENU) {
        set_group(menu_group);
    }
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/
/**
 * @brief start_btn的事件回调函数，按下按钮游戏状态改为GS_PLAY 并初始化游戏
 */
static void start_btn_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    fsm_switch_state(GS_PLAY);
    event_dispatch(EVENT_GAME_START,NULL,NULL);
    CONSOLE("[INFO] Fsm state was changed to %d",GS_PLAY);
}

/**
 * @brief exit_btn的事件回调函数，按下按钮弹窗
 */
static void exit_btn_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    fsm_switch_state(GS_EXIT);
    CONSOLE("[INFO] Fsm state was changed to %d",GS_EXIT);
}

/**
 * @brief yes btn for exit popup 按下确认退出
 */
static void yes_exit_btn_event_cb(lv_event_t * e)
{
    console_out("[menu][yes_exit_btn] Exiting game by user command\n");
    log_out("[menu][yes_exit_btn] Exiting game by user command");
    LV_UNUSED(e);
    #ifdef SIMULATOR
    exit(0);
    #else
    NVIC_SystemReset();
    #endif //#ifdef SIMULATOR
}

/**
 * @brief no btn for exit popup 按下取消退出
 */
static void no_exit_btn_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    fsm_switch_state(GS_MENU);
    console_out("[menu][no_exit_btn] Cancelled exit, returning to menu\n");
}
