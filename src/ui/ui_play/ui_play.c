/**
 * @file ui_play.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "ui_play.h"

#include "lvgl.h"
#include <stdio.h>

#include "tools.h"
#include "fsm.h"
#include "ui_templates.h"
#include "lvgl_utils.h"
#include "player.h"
#include "bullet.h"
#include "event.h"

/**********************
 *      MACROS
 **********************/

#define BG_IMG_NAME "play_bg.bin"
#define HUD_IMG_NAME "play_hud.bin"

/**********************
 *      TYPEDEFS
 **********************/

 /**********************
  *  STATIC PROTOTYPES
  **********************/

static void pause_exit_btn_event_cb(lv_event_t * e);
static void pause_continue_btn_event_cb(lv_event_t * e);
static void pause_btn_event_cb(lv_event_t * e);
static void over_exit_btn_event_cb(lv_event_t * e);

static void opa_anim_cb(void * obj, int32_t opa);
static void y_anim_cb(void * obj, int32_t y);
static void level_anim_finish(lv_anim_t * anim);

static void ui_play_event_game_start_cb(game_obj_t * a,game_obj_t * b);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static lv_group_t * pause_group;
static lv_group_t * over_group;

static lv_obj_t * dp_play;

static lv_obj_t * pause_popup;
static lv_obj_t * over_popup;

#ifdef SIMULATOR
static lv_img_dsc_t bg_img_dsc;
static lv_img_dsc_t hud_img_dsc;
#endif

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief play的ui初始化，完成所有元素绘制和回调函数绑定
 */
void ui_play_init()
{
    //group initialize
    pause_group = lv_group_create();
    //Parent object initialize
    dp_play = lv_obj_create(NULL);
    lv_obj_clear_flag(dp_play,LV_OBJ_FLAG_SCROLLABLE);
    //imgs initialize
    char img_path_buf[64];
    #ifdef SIMULATOR
    lv_obj_t * bg_img = img_create_from_array(dp_play,img_path(BG_IMG_NAME,img_path_buf,64),1024,600,NULL,&bg_img_dsc,false);
    lv_obj_t * hud_img = img_create_from_array(dp_play,img_path(HUD_IMG_NAME,img_path_buf,64),210,105,NULL,&hud_img_dsc,false);
    lv_obj_set_align(hud_img,LV_ALIGN_TOP_LEFT);
    #else
    lv_obj_t * bg_img = lv_img_create(dp_play);
    lv_img_set_src(bg_img,img_path(BG_IMG_NAME,img_path_buf,64));
    lv_obj_t * hud_img = lv_img_create(dp_play);
    lv_img_set_src(hud_img,img_path(HUD_IMG_NAME,img_path_buf,64));
    lv_obj_set_align(hud_img,LV_ALIGN_TOP_LEFT);
    #endif

    //Popups initialization
    pause_popup = popup_create(dp_play);
    lv_obj_add_flag(pause_popup,LV_OBJ_FLAG_HIDDEN);
    over_popup = popup_create(dp_play);
    lv_obj_add_flag(over_popup,LV_OBJ_FLAG_HIDDEN);
    //continue btn for pause popup
    lv_obj_t * pause_continue_btn = lv_btn_create(pause_popup);
    lv_obj_set_size(pause_continue_btn,300,60);
    lv_obj_set_pos(pause_continue_btn,25,240);
    lv_obj_add_event_cb(pause_continue_btn,pause_continue_btn_event_cb,LV_EVENT_CLICKED,NULL);
    lv_group_add_obj(pause_group,pause_continue_btn);
    //exit btn for pause popup
    lv_obj_t * pause_exit_btn = lv_btn_create(pause_popup);
    lv_obj_set_size(pause_exit_btn,300,60);
    lv_obj_set_pos(pause_exit_btn,25,320);
    lv_obj_add_event_cb(pause_exit_btn,pause_exit_btn_event_cb,LV_EVENT_CLICKED,NULL);
    lv_group_add_obj(pause_group,pause_exit_btn);
    //btn for pause
    lv_obj_t * pause_btn = lv_btn_create(dp_play);
    lv_obj_set_size(pause_btn,30,30);
    lv_obj_set_align(pause_btn,LV_ALIGN_TOP_RIGHT);
    lv_obj_add_event_cb(pause_btn,pause_btn_event_cb,LV_EVENT_CLICKED,NULL);
    //label for exit btn
    lv_obj_t * exit_btn_label = lv_label_create(pause_exit_btn);
    lv_obj_set_align(exit_btn_label,LV_ALIGN_CENTER);
    lv_label_set_text(exit_btn_label,"Back to menu");
    lv_obj_set_style_text_font(exit_btn_label,&lv_font_montserrat_22,LV_STATE_DEFAULT);
    //label for continue btn
    lv_obj_t * continue_btn_label = lv_label_create(pause_continue_btn);
    lv_obj_set_align(continue_btn_label,LV_ALIGN_CENTER);
    lv_label_set_text(continue_btn_label,"Continue");
    lv_obj_set_style_text_font(continue_btn_label,&lv_font_montserrat_22,LV_STATE_DEFAULT);
    //label for pause btn
    lv_obj_t * pause_btn_label = lv_label_create(pause_btn);
    lv_obj_set_align(pause_btn_label,LV_ALIGN_CENTER);
    lv_label_set_text(pause_btn_label,"ll");
    lv_obj_set_style_text_font(pause_btn_label,&lv_font_montserrat_22,LV_STATE_DEFAULT);
    //label for pause popup
    lv_obj_t * pause_label = lv_label_create(pause_popup);
    lv_obj_set_pos(pause_label,10,50);
    lv_label_set_text(pause_label,"GAME PAUSED");
    lv_obj_set_style_text_font(pause_label,&lv_font_montserrat_44,LV_STATE_DEFAULT);
    //label for over popup
    lv_obj_t * over_label = lv_label_create(over_popup);
    lv_obj_set_pos(over_label,10,50);
    lv_label_set_text(over_label,"GAME OVER");
    lv_obj_set_style_text_font(over_label,&lv_font_montserrat_44,LV_STATE_DEFAULT);
    //back to menu btn for over popup
    lv_obj_t * over_exit_btn = lv_btn_create(over_popup);
    lv_obj_set_size(over_exit_btn,300,60);
    lv_obj_set_pos(over_exit_btn,25,240);
    lv_obj_add_event_cb(over_exit_btn,over_exit_btn_event_cb,LV_EVENT_CLICKED,NULL);
    //label for over_exit_btn
    lv_obj_t * over_exit_btn_label = lv_label_create(over_exit_btn);
    lv_obj_set_align(over_exit_btn_label,LV_ALIGN_CENTER);
    lv_label_set_text(over_exit_btn_label,"Back to menu");
    lv_obj_set_style_text_font(over_exit_btn_label,&lv_font_montserrat_22,LV_STATE_DEFAULT);

    event_register(EVENT_GAME_START,ui_play_event_game_start_cb);

}

/**
 * @brief 加载play界面 && 负责控制弹窗是否显示
 */
void ui_play_run()
{
    lv_scr_load(dp_play);
    if (fsm_get_state() == GS_PAUSE) {
        popup_show(pause_popup);
        set_group(pause_group);
    }
    else {
        popup_hide(pause_popup);
    }
    if (fsm_get_state() == GS_OVER) {
        popup_show(over_popup);
        set_group(over_group);
    }
    else {
        popup_hide(over_popup);
    }
    if (fsm_get_state() == GS_PLAY) {
        set_group(NULL);
    }
}

/**
 * @brief 获取play界面 只给game_init()调用
 * @return play界面
 */
lv_obj_t * ui_play_get_display(void)
{
    return dp_play;
}   

/**
 * @brief Level 进场动画
 * @param level_name 自定义名称
 */
void ui_play_level_enter_anim(const char * level_name)
{
    lv_obj_t * label_level = lv_label_create(dp_play);
    lv_label_set_text(label_level, level_name);
    lv_obj_align(label_level, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_set_style_text_color(label_level, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_level, &lv_font_montserrat_44, 0);
    
    lv_obj_move_foreground(label_level);
    lv_obj_set_y(label_level, -90);
    lv_obj_set_style_opa(label_level, LV_OPA_TRANSP, 0);

    static lv_anim_t anim_move, anim_fade;
    lv_anim_init(&anim_move);
    lv_anim_set_var(&anim_move, label_level);
    lv_anim_set_exec_cb(&anim_move, y_anim_cb);
    lv_anim_set_values(&anim_move, -90, 30);
    lv_anim_set_time(&anim_move, 600);
    lv_anim_set_ready_cb(&anim_move, level_anim_finish);
    lv_anim_start(&anim_move);

    lv_anim_init(&anim_fade);
    lv_anim_set_var(&anim_fade, label_level);
    lv_anim_set_exec_cb(&anim_fade, opa_anim_cb);
    lv_anim_set_values(&anim_fade, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&anim_fade, 600);
    lv_anim_start(&anim_fade);
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/
/**
 * @brief 按下回退到菜单
 */
static void pause_exit_btn_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    fsm_switch_state(GS_MENU);
    console_out("[play][pause_exit_btn] State has been switched to %d\n", fsm_get_state());
}

/**
 * @brief 按下继续游戏
 */
static void pause_continue_btn_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    fsm_switch_state(GS_PLAY);
    console_out("[play][pause_continue_btn] State has been switched to %d\n", fsm_get_state());
}

/**
 * @brief 按下暂停/继续
 */
static void pause_btn_event_cb(lv_event_t * e)
{
    switch (fsm_get_state())
    {
        case GS_PLAY : fsm_switch_state(GS_PAUSE); console_out("[play][pause_btn] State has been switched to %d\n", fsm_get_state()); break;
        case GS_PAUSE : fsm_switch_state(GS_PLAY); console_out("[play][pause_btn] State has been switched to %d\n", fsm_get_state()); break;
    }
}

/**
 * @brief 返回主菜单
 */
static void over_exit_btn_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    fsm_switch_state(GS_MENU);
    console_out("[play][over_exit_btn] State has been switched to %d\n", fsm_get_state());
}

/**
 * @brief 透明度动画回调 对进场level动画使用
 */
static void opa_anim_cb(void * obj, int32_t opa) 
{
    lv_obj_set_style_opa((lv_obj_t*)obj, opa, 0); 
}

/**
 * @brief y坐标动画回调 对进场level动画使用
 */
static void y_anim_cb(void * obj, int32_t y) 
{ 
    lv_obj_set_y((lv_obj_t*)obj, y); 
}

/**
 * @brief Level 进场动画结束
 */
static void level_anim_finish(lv_anim_t * anim)
{
    lv_obj_t * label = anim->var;
    lv_anim_t fade_out;
    lv_anim_init(&fade_out);
    lv_anim_set_var(&fade_out, label);
    lv_anim_set_exec_cb(&fade_out, opa_anim_cb);
    lv_anim_set_values(&fade_out, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&fade_out, 500);
    lv_anim_set_delay(&fade_out, 1000);
    lv_anim_start(&fade_out);
}

static void ui_play_event_game_start_cb(game_obj_t * a,game_obj_t * b)
{
    CONSOLE("[INFO] Level 1 Animation Start.");
    ui_play_level_enter_anim("Level 1");
}
