/**
 * @file ui_play.c
 */

/*********************
 * INCLUDES
 *********************/
#include "ui_play.h"

#include "lvgl.h"
#include <stdio.h>

#include "tools.h"
#include "fsm.h"
#include "ui_templates.h"
#include "lvgl_utils.h"
#include "game_object.h"

#include "coin.h"
#include "event.h"
#include "perf_monitor.h"

/**********************
 * MACROS
 **********************/


#define HUD_IMG_NAME "hUd.bin"
#define HUD_IMG_NAME "play_hud.bin"
#define COIN_BAR_IMG_NAME "coin_bar.bin"

/**********************
 * TYPEDEFS
 **********************/

 /**********************
  * STATIC PROTOTYPES
  **********************/

static void pause_exit_btn_event_cb(lv_event_t * e);
static void pause_continue_btn_event_cb(lv_event_t * e);
static void pause_btn_event_cb(lv_event_t * e);
static void over_exit_btn_event_cb(lv_event_t * e);

static void opa_anim_cb(void * obj, int32_t opa);
static void y_anim_cb(void * obj, int32_t y);
static void level_anim_finish(lv_anim_t * anim);

static void ui_play_event_game_start_cb(game_obj_t * a,game_obj_t * b);

static void hurt_flash_timer_cb(lv_timer_t * timer);
static void ui_play_event_player_hurt_cb(game_obj_t * a, game_obj_t * b);
static void ui_play_event_hit_coin_cb(game_obj_t * src, game_obj_t * trg);

/***********************
 * GLOBAL PROTOTYPES
 ***********************/

/**********************
 * STATIC VARIABLES
 **********************/

static lv_group_t * pause_group;
static lv_group_t * over_group;

static lv_obj_t * dp_play;

static lv_obj_t * pause_popup;
static lv_obj_t * over_popup;
static lv_obj_t * coin_label;

#ifdef SIMULATOR
static lv_img_dsc_t coin_img_dsc;
static lv_img_dsc_t hurt_img_dsc;
static lv_img_dsc_t hud_img_dsc;
#endif
static lv_obj_t * hurt_img = NULL;
static lv_timer_t * hurt_timer = NULL;

 /**********************
 * GLOBAL FUNCTIONS
 **********************/

/**
 * @brief play的ui初始化，完成所有元素绘制和回调函数绑定
 */
void ui_play_init()
{
    CONSOLE("[DEBUG-PLAY] Entering ui_play_init...");
    
    //group initialize
    pause_group = lv_group_create();
    
    //Parent object initialize
    dp_play = lv_obj_create(NULL);
    if (dp_play == NULL) {
        CONSOLE("[FATAL-PLAY] dp_play creation failed!");
        return;
    }
    lv_obj_clear_flag(dp_play, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(dp_play, lv_color_hex(DP_PLAY_FILL_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dp_play, LV_OPA_COVER, LV_PART_MAIN);

    //coin_img initialize
    static char img_path_buf[64];
#ifdef SIMULATOR
    CONSOLE("[DEBUG-PLAY] Creating hud_img from dsc...");
    lv_obj_t * hud_img = img_create_from_dsc(dp_play, img_path(HUD_IMG_NAME, img_path_buf, 64), 200, 52, NULL, &hud_img_dsc, false);
    if (hud_img != NULL) lv_obj_set_align(hud_img, LV_ALIGN_TOP_LEFT);
#else
    CONSOLE("[DEBUG-PLAY] Creating standard hud_img...");
    lv_obj_t * hud_img = lv_img_create(dp_play);
    if (hud_img != NULL) {
        lv_img_set_src(hud_img, img_path(HUD_IMG_NAME, img_path_buf, 64));
        lv_obj_set_align(hud_img, LV_ALIGN_TOP_LEFT);
    }
    //coin_img initialize
    #ifdef SIMULATOR
    lv_obj_t * coin_img = img_create_from_dsc(dp_play,img_path(COIN_BAR_IMG_NAME,img_path_buf,64),166,46,NULL,&coin_img_dsc,true);
    lv_obj_set_align(coin_img,LV_ALIGN_BOTTOM_LEFT);
    #else
    lv_obj_t * coin_img = lv_img_create(dp_play);
    lv_img_set_src(coin_img,img_path(COIN_BAR_IMG_NAME,img_path_buf,64));
    lv_obj_set_align(coin_img,LV_ALIGN_BOTTOM_RIGHT);
    #endif

    if (coin_img == NULL) {
        CONSOLE("[ERROR-PLAY] coin_img is NULL! coin_label might bound to invalid parent.");
    }

    //Popups initialization
    pause_popup = popup_create(dp_play);
    lv_obj_add_flag(pause_popup, LV_OBJ_FLAG_HIDDEN);
    over_popup = popup_create(dp_play);
    lv_obj_add_flag(over_popup, LV_OBJ_FLAG_HIDDEN);
    
    //continue btn for pause popup
    lv_obj_t * pause_continue_btn = lv_btn_create(pause_popup);
    lv_obj_set_size(pause_continue_btn, 300, 60);
    lv_obj_set_pos(pause_continue_btn, 25, 240);
    lv_obj_add_event_cb(pause_continue_btn, pause_continue_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(pause_group, pause_continue_btn);
    
    //exit btn for pause popup
    lv_obj_t * pause_exit_btn = lv_btn_create(pause_popup);
    lv_obj_set_size(pause_exit_btn, 300, 60);
    lv_obj_set_pos(pause_exit_btn, 25, 320);
    lv_obj_add_event_cb(pause_exit_btn, pause_exit_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(pause_group, pause_exit_btn);
    
    //btn for pause
    lv_obj_t * pause_btn = lv_btn_create(dp_play);
    lv_obj_set_size(pause_btn, 30, 30);
    lv_obj_set_align(pause_btn, LV_ALIGN_TOP_RIGHT);
    lv_obj_add_event_cb(pause_btn, pause_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    //label for exit btn
    lv_obj_t * exit_btn_label = lv_label_create(pause_exit_btn);
    lv_obj_set_align(exit_btn_label, LV_ALIGN_CENTER);
    lv_label_set_text(exit_btn_label, "Back to menu");
    lv_obj_set_style_text_font(exit_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);
    
    //label for continue btn
    lv_obj_t * continue_btn_label = lv_label_create(pause_continue_btn);
    lv_obj_set_align(continue_btn_label, LV_ALIGN_CENTER);
    lv_label_set_text(continue_btn_label, "Continue");
    lv_obj_set_style_text_font(continue_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);
    
    //label for pause btn
    lv_obj_t * pause_btn_label = lv_label_create(pause_btn);
    lv_obj_set_align(pause_btn_label, LV_ALIGN_CENTER);
    lv_label_set_text(pause_btn_label, "ll");
    lv_obj_set_style_text_font(pause_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);
    
    //label for pause popup
    lv_obj_t * pause_label = lv_label_create(pause_popup);
    lv_obj_set_pos(pause_label, 10, 50);
    lv_label_set_text(pause_label, "GAME PAUSED");
    lv_obj_set_style_text_font(pause_label, &lv_font_montserrat_44, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(pause_label, lv_color_hex(0x13AEFB), LV_STATE_DEFAULT);
    
    //label for over popup
    lv_obj_t * over_label = lv_label_create(over_popup);
    lv_obj_set_pos(over_label, 40, 50);
    lv_label_set_text(over_label, "GAME OVER");
    lv_obj_set_style_text_font(over_label, &lv_font_montserrat_44, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(over_label, lv_palette_main(LV_PALETTE_RED), LV_STATE_DEFAULT);
    
    //back to menu btn for over popup
    lv_obj_t * over_exit_btn = lv_btn_create(over_popup);
    lv_obj_set_size(over_exit_btn, 300, 60);
    lv_obj_set_pos(over_exit_btn, 25, 240);
    lv_obj_add_event_cb(over_exit_btn, over_exit_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    //label for over_exit_btn
    lv_obj_t * over_exit_btn_label = lv_label_create(over_exit_btn);
    lv_obj_set_align(over_exit_btn_label, LV_ALIGN_CENTER);
    lv_label_set_text(over_exit_btn_label, "Back to menu");
    lv_obj_set_style_text_font(over_exit_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);
    
    //label for coin
    coin_label = lv_label_create(coin_img);
    lv_obj_set_pos(coin_label,120,23);
    lv_label_set_text_fmt(coin_label, "%d", coin_get_num());
    lv_obj_set_style_text_font(coin_label,&lv_font_montserrat_16,LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(coin_label,lv_color_white(),LV_STATE_DEFAULT);

    // 性能检测UI初始化
    perf_monitor_init(dp_play);

    // 受伤闪烁遮罩 (1024×600, 较高透明度, 不阻挡按键)
    {
        static char hurt_path[64];
#ifdef SIMULATOR
        hurt_img = img_create_from_dsc(dp_play, img_path("hurted.bin", hurt_path, 64),
                                        1024, 600, NULL, &hurt_img_dsc, false);
#else
        hurt_img = lv_img_create(dp_play);
        lv_img_set_src(hurt_img, img_path("hurted.bin", hurt_path, 64));
#endif
        lv_obj_set_size(hurt_img, 1024, 600);
        lv_obj_set_style_opa(hurt_img, LV_OPA_40, 0); // 60%透明
        lv_obj_add_flag(hurt_img, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(hurt_img, LV_OBJ_FLAG_CLICKABLE); // 不挡按键
        lv_obj_move_foreground(hurt_img);
    }

    // 事件注册
    event_register(EVENT_GAME_START,ui_play_event_game_start_cb);
    event_register(EVENT_PLAYER_HIT_COIN, ui_play_event_hit_coin_cb);
}

/**
 * @brief 加载play界面 && 负责控制弹窗是否显示 && 播放BGM
 */
void ui_play_run()
{
    CONSOLE("[DEBUG-PLAY] START ui_play_run");
    
    if (dp_play == NULL) {
        CONSOLE("[FATAL-PLAY] ui_play_run: dp_play is NULL!");
        return;
    }

    lv_scr_load(dp_play);
    CONSOLE("[DEBUG-PLAY] ui_play_run: Screen loaded");

    int state = fsm_get_state();
    CONSOLE("[DEBUG-PLAY] ui_play_run: Current FSM state = %d", state);

    if (state == GS_PAUSE) {
        CONSOLE("[DEBUG-PLAY] ui_play_run: Processing GS_PAUSE");
        popup_show(pause_popup);
        set_group(pause_group);
    }
    else {
        popup_hide(pause_popup);
    }
    
    if (state == GS_OVER) {
        CONSOLE("[DEBUG-PLAY] ui_play_run: Processing GS_OVER");
        popup_show(over_popup);
        // 防御性检查：确保 over_group 已创建，防止传入野指针
        if (over_group != NULL) {
            set_group(over_group);
        } else {
            CONSOLE("[WARNING-PLAY] over_group is NULL during GS_OVER!");
        }
    }
    else {
        popup_hide(over_popup);
    }
    
    if (state == GS_PLAY) {
        CONSOLE("[DEBUG-PLAY] ui_play_run: Processing GS_PLAY, setting group to NULL");
        set_group(NULL);
    }
    
    CONSOLE("[DEBUG-PLAY] END ui_play_run successfully");
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
    CONSOLE("[DEBUG-PLAY] ui_play_level_enter_anim: Start with name '%s'", level_name);
    
    lv_obj_t * label_level = lv_label_create(dp_play);
    if (label_level == NULL) {
        CONSOLE("[FATAL-PLAY] label_level creation failed!");
        return;
    }
    
    lv_label_set_text(label_level, level_name);
    lv_obj_align(label_level, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_set_style_text_color(label_level, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_level, &lv_font_montserrat_44, 0);
    
    CONSOLE("[DEBUG-PLAY] ui_play_level_enter_anim: Configuring animation layers...");
    lv_obj_move_foreground(label_level);
    lv_obj_set_y(label_level, -90);
    lv_obj_set_style_opa(label_level, LV_OPA_TRANSP, 0);

    static lv_anim_t anim_move, anim_fade;
    
    CONSOLE("[DEBUG-PLAY] ui_play_level_enter_anim: Launching move animation...");
    lv_anim_init(&anim_move);
    lv_anim_set_var(&anim_move, label_level);
    lv_anim_set_exec_cb(&anim_move, y_anim_cb);
    lv_anim_set_values(&anim_move, -90, 30);
    lv_anim_set_time(&anim_move, 600);
    lv_anim_set_ready_cb(&anim_move, level_anim_finish);
    lv_anim_start(&anim_move);

    CONSOLE("[DEBUG-PLAY] ui_play_level_enter_anim: Launching fade animation...");
    lv_anim_init(&anim_fade);
    lv_anim_set_var(&anim_fade, label_level);
    lv_anim_set_exec_cb(&anim_fade, opa_anim_cb);
    lv_anim_set_values(&anim_fade, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&anim_fade, 600);
    lv_anim_start(&anim_fade);
    
    CONSOLE("[DEBUG-PLAY] ui_play_level_enter_anim: Complete");
}

 /**********************
 * STATIC FUNCTIONS
 **********************/
/**
 * @brief 玩家拾取金币事件回调：实时更新金币标签显示
 */
static void ui_play_event_hit_coin_cb(game_obj_t * src, game_obj_t * trg)
{
    (void)src;
    (void)trg;
    if (coin_label != NULL) {
        lv_label_set_text_fmt(coin_label, "%d", coin_get_num());
    }
}

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
    if (obj != NULL) {
        lv_obj_set_style_opa((lv_obj_t*)obj, opa, 0); 
    }
}

/**
 * @brief y坐标动画回调 对进场level动画使用
 */
static void y_anim_cb(void * obj, int32_t y) 
{ 
    if (obj != NULL) {
        lv_obj_set_y((lv_obj_t*)obj, y); 
    }
}

/**
 * @brief Level 进场动画结束
 */
static void level_anim_finish(lv_anim_t * anim)
{
    CONSOLE("[DEBUG-PLAY] level_anim_finish: Animation finish ready_cb entered");
    if (anim == NULL || anim->var == NULL) {
        CONSOLE("[ERROR-PLAY] level_anim_finish: anim or anim->var is NULL!");
        return;
    }
    
    lv_obj_t * label = anim->var;
    lv_anim_t fade_out;
    lv_anim_init(&fade_out);
    lv_anim_set_var(&fade_out, label);
    lv_anim_set_exec_cb(&fade_out, opa_anim_cb);
    lv_anim_set_values(&fade_out, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&fade_out, 500);
    lv_anim_set_delay(&fade_out, 1000);
    lv_anim_start(&fade_out);
    CONSOLE("[DEBUG-PLAY] level_anim_finish: Fade out animation scheduled");
}

static void ui_play_event_game_start_cb(game_obj_t * a, game_obj_t * b)
{
    CONSOLE("[DEBUG-PLAY] START ui_play_event_game_start_cb");
    ui_play_level_enter_anim("Level 1");
    CONSOLE("[DEBUG-PLAY] END ui_play_event_game_start_cb");
}

/**
 * @brief 受伤闪烁计时器 —— 1 秒后隐藏红色遮罩
 */
static void hurt_flash_timer_cb(lv_timer_t * timer)
{
    if (hurt_img != NULL) {
        lv_obj_add_flag(hurt_img, LV_OBJ_FLAG_HIDDEN);
    }
    hurt_timer = NULL;
    lv_timer_del(timer);
}

/**
 * @brief 玩家受伤事件回调 —— 显示半透明红色遮罩闪烁
 */
static void ui_play_event_player_hurt_cb(game_obj_t * a, game_obj_t * b)
{
    if (hurt_img == NULL) return;

    // 先删旧定时器，重新计时（连续受伤保持闪烁）
    if (hurt_timer != NULL) {
        lv_timer_del(hurt_timer);
        hurt_timer = NULL;
    }

    lv_obj_clear_flag(hurt_img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(hurt_img);
    hurt_timer = lv_timer_create(hurt_flash_timer_cb, 1000, NULL);
}
