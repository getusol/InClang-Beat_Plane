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

#include "event.h"
#include "perf_monitor.h"
#include "ui_setting.h"
#include "audio.h"
#include "player.h"
#include "character.h"
#include "multiplayer.h"
#include "settings.h"
#include "ui_shop.h"
#include "input_device.h"
#include "comm_tx.h"
#include "debug.h"

/**********************
 * MACROS
 **********************/

#define CD_BASE_Y 99 // black_x 基准 Y
#define CD_GAP 80    // X-Y 圆形间距 (3 玩家时最末 y≈499, Ready≈555 < 600)

/**********************
 * TYPEDEFS
 **********************/

/**********************
 * STATIC PROTOTYPES
 **********************/

static void pause_exit_btn_event_cb(lv_event_t *e);
static void pause_continue_btn_event_cb(lv_event_t *e);
static void pause_btn_event_cb(lv_event_t *e);
static void over_exit_btn_event_cb(lv_event_t *e);
static void over_restart_btn_event_cb(lv_event_t *e);

static void opa_anim_cb(void *obj, int32_t opa);
static void y_anim_cb(void *obj, int32_t y);
static void level_anim_finish(lv_anim_t *anim);

static void ui_play_event_game_start_cb(game_obj_t * a,game_obj_t * b);
static void ui_play_event_game_over_cb(game_obj_t * a,game_obj_t * b);
static void ui_play_event_game_win_cb(game_obj_t * a,game_obj_t * b);

static void hurt_flash_timer_cb(lv_timer_t *timer);
static void ui_play_event_player_hurt_cb(game_obj_t *a, game_obj_t *b);
static void ui_play_event_hit_coin_cb(game_obj_t *src, game_obj_t *trg);
static void pause_setting_btn_event_cb(lv_event_t *e);
static void cd_update_timer_cb(lv_timer_t *timer);
static void on_mp_disconnected(mp_event_t e, void *v);

/***********************
 * GLOBAL PROTOTYPES
 ***********************/

/**********************
 * STATIC VARIABLES
 **********************/

// 配置项
static bool show_hurt_overlay = false;
static setting_t settings[] = {
    {.module = "Game", .name = "Show HurtEffect", .type = ST_BOOL, .data = &show_hurt_overlay, .bool_data = {.def = false}},
};
static uint8_t settings_count = sizeof(settings) / sizeof(settings[0]);

static lv_group_t *pause_group;
static lv_group_t *over_group;

// dp_play 是 play的根容器
static lv_obj_t *dp_play;
// hud_layer 是 play的ui容器
static lv_obj_t *hud_layer;

static lv_obj_t *pause_popup;
static lv_obj_t *over_popup;
static lv_obj_t *over_score_label;
static lv_obj_t *over_label = NULL;              // GAME OVER / You Are The WINNER! 标签
static lv_obj_t *over_restart_btn_label = NULL;  // Restart / Play Again 按钮标签
static lv_obj_t *pause_icon_btn;
static bool is_win = false;                      // true = 胜利通关, false = 死亡失败

// ---- 多玩家 HUD 数组 ----
static lv_obj_t *hud_imgs[MAX_PLAYER_COUNT];
static lv_obj_t *cd_arc_x[MAX_PLAYER_COUNT];
static lv_obj_t *cd_arc_y[MAX_PLAYER_COUNT];
static lv_obj_t *cd_bg_x[MAX_PLAYER_COUNT];
static lv_obj_t *cd_bg_y[MAX_PLAYER_COUNT];
static lv_obj_t *cd_ready_x[MAX_PLAYER_COUNT];
static lv_obj_t *cd_ready_y[MAX_PLAYER_COUNT];
static lv_obj_t *coin_bars[MAX_PLAYER_COUNT];
static lv_obj_t *coin_labels[MAX_PLAYER_COUNT];

// HUD 背景图 — 按玩家索引手动指定, NULL 跳过
static const char *hud_img_names[MAX_PLAYER_COUNT] = {
    "hud_blue.bin",  // P1
    "hud_red.bin",   // P2
    "hud_green.bin", // P3
};
// CD 弧颜色: P1 蓝, P2 红, P3 绿
static const lv_color_t cd_colors[MAX_PLAYER_COUNT] = {
    LV_COLOR_MAKE(0x44, 0x88, 0xCC), // P1 蓝
    LV_COLOR_MAKE(0xCC, 0x44, 0x88), // P2 红
    LV_COLOR_MAKE(0x44, 0xCC, 0x44), // P3 绿
};
// 金币条背景图
static const char *coin_bar_names[MAX_PLAYER_COUNT] = {
    "coin_bar.bin",       // P1
    "coin_red_bar.bin",   // P2
    "coin_green_bar.bin", // P3
};

#ifdef SIMULATOR
static lv_img_dsc_t hud_img_dsc[MAX_PLAYER_COUNT];
static lv_img_dsc_t coin_img_dsc[MAX_PLAYER_COUNT];
static lv_img_dsc_t hurt_img_dsc;
static lv_img_dsc_t setting_icon_dsc;
#endif
static lv_obj_t *hurt_img = NULL;
static lv_timer_t *hurt_timer = NULL;
static lv_obj_t *freeze_overlay = NULL;

static lv_timer_t *cd_update_timer = NULL; // CD更新定时器

/**********************
 * GLOBAL FUNCTIONS
 **********************/

/**
 * @brief play的ui初始化，完成根屏幕的初始化 第一阶段
 */
void ui_play_init_stage1()
{
    dp_play = lv_obj_create(NULL);
    if (dp_play == NULL)
    {
        CONSOLE_ERROR("dp_play creation failed!");
        LOG_ERROR("dp_play creation failed!");
        return;
    }
    lv_obj_clear_flag(dp_play, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(dp_play, lv_color_hex(DP_PLAY_FILL_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dp_play, LV_OPA_COVER, LV_PART_MAIN);

    // 创建hud层
    hud_layer = lv_obj_create(dp_play);
    if (hud_layer == NULL)
    {
        CONSOLE_ERROR("hud_layer creation failed!");
        LOG_ERROR("hud_layer creation failed!");
        return;
    }
    lv_obj_clear_flag(hud_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(hud_layer, LV_PCT(100), LV_PCT(100)); // 全屏
    lv_obj_set_style_bg_opa(hud_layer, LV_OPA_TRANSP, 0); // 背景透明
    lv_obj_set_style_border_width(hud_layer, 0, 0);       // 无边框
    lv_obj_set_style_pad_all(hud_layer, 0, 0);            // 无内边距
    lv_obj_clear_flag(hud_layer, LV_OBJ_FLAG_CLICKABLE);

    // 注册所有项到settings系统
    for (int i = 0; i < settings_count; i++)
    {
        settings_register(&settings[i]);
    }
}

/**
 * @brief play的ui初始化，完成所有元素绘制和回调函数绑定 第二阶段
 */
void ui_play_init_stage2()
{
    CONSOLE_DEBUG("Entering ui_play_init_stage2...");

    // group initialize
    pause_group = lv_group_create();

    // Parent object initialize
    if (dp_play == NULL)
    {
        CONSOLE_ERROR("dp_play is NULL! ui_play_init_stage2() must be called after ui_play_init_stage1()");
        LOG_ERROR("dp_play is NULL! ui_play_init_stage2() must be called after ui_play_init_stage1()");
        return;
    }

    if (hud_layer == NULL)
    {
        CONSOLE_ERROR("hud_layer is NULL! ui_play_init_stage2() must be called after ui_play_init_stage1()");
        LOG_ERROR("hud_layer is NULL! ui_play_init_stage2() must be called after ui_play_init_stage1()");
        return;
    }

    static char img_path_buf[64];

    /* ---- HUD 背景装饰 (左上角) ---- */
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        if (hud_img_names[i] == NULL)
        {
            CONSOLE_WARNING("hud_img_names[%d] is NULL, skipping HUD bg", i);
            hud_imgs[i] = NULL;
            continue;
        }
#ifdef SIMULATOR
        hud_imgs[i] = img_create_from_dsc(hud_layer,
                                          img_path(hud_img_names[i], img_path_buf, 64),
                                          200, 42, NULL, &hud_img_dsc[i], true);
#else
        hud_imgs[i] = lv_img_create(hud_layer);
        lv_img_set_src(hud_imgs[i], img_path(hud_img_names[i], img_path_buf, 64));
#endif
        if (hud_imgs[i] != NULL)
        {
            lv_obj_set_pos(hud_imgs[i], 2, 5 + i * 79);
            lv_obj_set_align(hud_imgs[i], LV_ALIGN_TOP_LEFT);
            lv_obj_add_flag(hud_imgs[i], LV_OBJ_FLAG_HIDDEN);
            CONSOLE_DEBUG("hud img %d hidden", i);
        }
    }

    /* ---- 金币条 (左下角, 向上堆叠) ---- */
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        const char *bar_name = (i < MAX_PLAYER_COUNT) ? coin_bar_names[i] : NULL;
        if (bar_name == NULL)
        {
            CONSOLE_WARNING("coin_bar_names[%d] is NULL, skipping coin bar", i);
            coin_bars[i] = NULL;
            coin_labels[i] = NULL;
            continue;
        }
#ifdef SIMULATOR
        coin_bars[i] = img_create_from_dsc(hud_layer,
                                           img_path(bar_name, img_path_buf, 64),
                                           166, 46, NULL, &coin_img_dsc[i], true);
#else
        coin_bars[i] = lv_img_create(hud_layer);
        lv_img_set_src(coin_bars[i], img_path(bar_name, img_path_buf, 64));
#endif
        if (coin_bars[i] != NULL)
        {
            lv_obj_set_align(coin_bars[i], LV_ALIGN_BOTTOM_LEFT);
            lv_obj_set_pos(coin_bars[i], 0, -i * 46);
            lv_obj_add_flag(coin_bars[i], LV_OBJ_FLAG_HIDDEN);

            coin_labels[i] = lv_label_create(coin_bars[i]);
            lv_obj_set_pos(coin_labels[i], 120, 23);
            lv_label_set_text(coin_labels[i], "0");
            lv_obj_set_style_text_font(coin_labels[i], &lv_font_montserrat_16, LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(coin_labels[i], lv_color_white(), LV_STATE_DEFAULT);
        }
    }

    // Popups initialization
    pause_popup = popup_create(hud_layer);
    lv_obj_add_flag(pause_popup, LV_OBJ_FLAG_HIDDEN);
    over_popup = popup_create(hud_layer);
    lv_obj_add_flag(over_popup, LV_OBJ_FLAG_HIDDEN);

    // continue btn for pause popup
    lv_obj_t *pause_continue_btn = lv_btn_create(pause_popup);
    lv_obj_set_size(pause_continue_btn, 300, 60);
    lv_obj_set_pos(pause_continue_btn, 25, 200);
    lv_obj_add_event_cb(pause_continue_btn, pause_continue_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(pause_group, pause_continue_btn);
    // exit btn for pause popup
    lv_obj_t *pause_exit_btn = lv_btn_create(pause_popup);
    lv_obj_set_size(pause_exit_btn, 300, 60);
    lv_obj_set_pos(pause_exit_btn, 25, 360);
    lv_obj_add_event_cb(pause_exit_btn, pause_exit_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(pause_group, pause_exit_btn);
    // 右上角暂停/设置图标按钮 (64x64 透明 + setting_icon.bin)
    pause_icon_btn = lv_btn_create(hud_layer);
    lv_obj_set_size(pause_icon_btn, 64, 64);
    lv_obj_align(pause_icon_btn, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_opa(pause_icon_btn, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(pause_icon_btn, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(pause_icon_btn, 0, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(pause_icon_btn, pause_btn_event_cb, LV_EVENT_CLICKED, NULL);

#ifdef SIMULATOR
    lv_obj_t *pause_icon_img = img_create_from_dsc(pause_icon_btn,
                                                   img_path("setting_icon.bin", img_path_buf, sizeof(img_path_buf)),
                                                   64, 64, NULL, &setting_icon_dsc, true);
#else
    lv_obj_t *pause_icon_img = lv_img_create(pause_icon_btn);
    lv_img_set_src(pause_icon_img, img_path("setting_icon.bin", img_path_buf, sizeof(img_path_buf)));
#endif
    lv_obj_center(pause_icon_img);

    // Settings button in pause popup
    lv_obj_t *pause_setting_btn = lv_btn_create(pause_popup);
    lv_obj_set_size(pause_setting_btn, 300, 60);
    lv_obj_set_pos(pause_setting_btn, 25, 280);
    lv_obj_add_event_cb(pause_setting_btn, pause_setting_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(pause_group, pause_setting_btn);

    lv_obj_t *setting_btn_label = lv_label_create(pause_setting_btn);
    lv_obj_set_align(setting_btn_label, LV_ALIGN_CENTER);
    lv_label_set_text(setting_btn_label, "Settings");
    lv_obj_set_style_text_font(setting_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    // label for exit btn
    lv_obj_t *exit_btn_label = lv_label_create(pause_exit_btn);
    lv_obj_set_align(exit_btn_label, LV_ALIGN_CENTER);
    lv_label_set_text(exit_btn_label, "Back to menu");
    lv_obj_set_style_text_font(exit_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    // label for continue btn
    lv_obj_t *continue_btn_label = lv_label_create(pause_continue_btn);
    lv_obj_set_align(continue_btn_label, LV_ALIGN_CENTER);
    lv_label_set_text(continue_btn_label, "Continue");
    lv_obj_set_style_text_font(continue_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);
    // label for pause popup
    lv_obj_t *pause_label = lv_label_create(pause_popup);
    lv_obj_set_pos(pause_label, 10, 50);
    lv_label_set_text(pause_label, "GAME PAUSED");
    lv_obj_set_style_text_font(pause_label, &lv_font_montserrat_44, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(pause_label, lv_color_hex(0x13AEFB), LV_STATE_DEFAULT);
    
    //label for over/win popup
    over_label = lv_label_create(over_popup);
    lv_obj_set_pos(over_label, 40, 50);
    lv_label_set_text(over_label, "GAME OVER");
    lv_obj_set_style_text_font(over_label, &lv_font_montserrat_44, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(over_label, lv_palette_main(LV_PALETTE_RED), LV_STATE_DEFAULT);

    // score label for over popup
    over_score_label = lv_label_create(over_popup);
    lv_obj_set_align(over_score_label, LV_ALIGN_CENTER);
    lv_obj_set_pos(over_score_label, 0, -70);
    lv_obj_set_style_text_font(over_score_label, &lv_font_montserrat_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(over_score_label, lv_color_hex(0xFFD152), LV_STATE_DEFAULT);

    // restart btn for over popup (reused for "Play Again" on win)
    lv_obj_t *over_restart_btn = lv_btn_create(over_popup);
    lv_obj_set_size(over_restart_btn, 300, 60);
    lv_obj_set_pos(over_restart_btn, 25, 240);
    lv_obj_add_event_cb(over_restart_btn, over_restart_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(over_group, over_restart_btn);

    // label for over_restart_btn (reused for "Play Again" on win)
    over_restart_btn_label = lv_label_create(over_restart_btn);
    lv_obj_set_align(over_restart_btn_label, LV_ALIGN_CENTER);
    lv_label_set_text(over_restart_btn_label, "Restart");
    lv_obj_set_style_text_font(over_restart_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);
    // back to menu btn for over popup
    lv_obj_t *over_exit_btn = lv_btn_create(over_popup);
    lv_obj_set_size(over_exit_btn, 300, 60);
    lv_obj_set_pos(over_exit_btn, 25, 320);
    lv_obj_add_event_cb(over_exit_btn, over_exit_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(over_group, over_exit_btn);

    // label for over_exit_btn
    lv_obj_t *over_exit_btn_label = lv_label_create(over_exit_btn);
    lv_obj_set_align(over_exit_btn_label, LV_ALIGN_CENTER);
    lv_label_set_text(over_exit_btn_label, "Back to menu");
    lv_obj_set_style_text_font(over_exit_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    // 性能检测UI初始化
    perf_monitor_init(hud_layer);

    // 受伤闪烁遮罩 (1024×600, 较高透明度, 不阻挡按键)
    {
        static char hurt_path[64];
#ifdef SIMULATOR
        hurt_img = img_create_from_dsc(hud_layer, img_path("hurted.bin", hurt_path, 64),
                                       1024, 600, NULL, &hurt_img_dsc, false);
#else
        hurt_img = lv_img_create(hud_layer);
        lv_img_set_src(hurt_img, img_path("hurted.bin", hurt_path, 64));
#endif
        lv_obj_set_size(hurt_img, 1024, 600);
        lv_obj_set_style_opa(hurt_img, LV_OPA_40, 0); // 60%透明
        lv_obj_add_flag(hurt_img, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(hurt_img, LV_OBJ_FLAG_CLICKABLE); // 不挡按键
        lv_obj_move_foreground(hurt_img);
    }

    // 冰冻减速遮罩 (Stream Y技能) — 冰蓝色半透明矩形
    {
        freeze_overlay = lv_obj_create(hud_layer);
        lv_obj_set_size(freeze_overlay, 1024, 600);
        lv_obj_set_style_bg_color(freeze_overlay, lv_color_hex(0x4488CC), 0);
        lv_obj_set_style_bg_opa(freeze_overlay, LV_OPA_20, 0);
        lv_obj_set_style_border_width(freeze_overlay, 0, 0);
        lv_obj_add_flag(freeze_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(freeze_overlay, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_move_foreground(freeze_overlay);
    }

    /* ---- 技能 CD 圆形 (右侧, X/Y 每玩家两个) ---- */
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        int base_y = CD_BASE_Y + i * CD_GAP * 2; // 99, 315, ...

        // X 技能
        int y_x = base_y;
        cd_bg_x[i] = lv_obj_create(hud_layer);
        lv_obj_set_size(cd_bg_x[i], 38, 38);
        lv_obj_set_pos(cd_bg_x[i], 949, y_x);
        lv_obj_set_style_bg_color(cd_bg_x[i], lv_color_black(), 0);
        lv_obj_set_style_bg_opa(cd_bg_x[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(cd_bg_x[i], 0, 0);
        lv_obj_set_style_radius(cd_bg_x[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_clear_flag(cd_bg_x[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(cd_bg_x[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(cd_bg_x[i], LV_OBJ_FLAG_HIDDEN);

        lv_obj_t *lx = lv_label_create(cd_bg_x[i]);
        lv_label_set_text(lx, "X");
        lv_obj_set_style_text_font(lx, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(lx, lv_color_white(), 0);
        lv_obj_center(lx);

        cd_arc_x[i] = lv_arc_create(hud_layer);
        lv_obj_set_size(cd_arc_x[i], 56, 56);
        lv_obj_set_pos(cd_arc_x[i], 940, y_x - 9);
        lv_obj_clear_flag(cd_arc_x[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_arc_set_mode(cd_arc_x[i], LV_ARC_MODE_NORMAL);
        lv_arc_set_bg_angles(cd_arc_x[i], 0, 360);
        lv_arc_set_rotation(cd_arc_x[i], 270);
        lv_arc_set_range(cd_arc_x[i], 0, 100);
        lv_arc_set_value(cd_arc_x[i], 100);
        lv_obj_set_style_arc_color(cd_arc_x[i], lv_color_hex(0x333344), LV_PART_MAIN);
        lv_obj_set_style_arc_color(cd_arc_x[i], cd_colors[i], LV_PART_INDICATOR); /* 蓝/红/绿 */
        lv_obj_set_style_arc_width(cd_arc_x[i], 5, LV_PART_MAIN);
        lv_obj_set_style_arc_width(cd_arc_x[i], 5, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(cd_arc_x[i], LV_OPA_TRANSP, LV_PART_KNOB);
        lv_obj_set_style_pad_all(cd_arc_x[i], 0, LV_PART_KNOB);
        lv_obj_add_flag(cd_arc_x[i], LV_OBJ_FLAG_HIDDEN);

        cd_ready_x[i] = lv_label_create(hud_layer);
        lv_label_set_text(cd_ready_x[i], "Ready!");
        lv_obj_set_style_text_font(cd_ready_x[i], &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(cd_ready_x[i], lv_color_hex(0x44FF44), 0);
        lv_obj_align_to(cd_ready_x[i], cd_arc_x[i], LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
        lv_obj_add_flag(cd_ready_x[i], LV_OBJ_FLAG_HIDDEN);

        // Y 技能
        int y_y = base_y + CD_GAP;
        cd_bg_y[i] = lv_obj_create(hud_layer);
        lv_obj_set_size(cd_bg_y[i], 38, 38);
        lv_obj_set_pos(cd_bg_y[i], 949, y_y);
        lv_obj_set_style_bg_color(cd_bg_y[i], lv_color_black(), 0);
        lv_obj_set_style_bg_opa(cd_bg_y[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(cd_bg_y[i], 0, 0);
        lv_obj_set_style_radius(cd_bg_y[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_clear_flag(cd_bg_y[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(cd_bg_y[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(cd_bg_y[i], LV_OBJ_FLAG_HIDDEN);

        lv_obj_t *ly = lv_label_create(cd_bg_y[i]);
        lv_label_set_text(ly, "Y");
        lv_obj_set_style_text_font(ly, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(ly, lv_color_white(), 0);
        lv_obj_center(ly);

        cd_arc_y[i] = lv_arc_create(hud_layer);
        lv_obj_set_size(cd_arc_y[i], 56, 56);
        lv_obj_set_pos(cd_arc_y[i], 940, y_y - 9);
        lv_obj_clear_flag(cd_arc_y[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_arc_set_mode(cd_arc_y[i], LV_ARC_MODE_NORMAL);
        lv_arc_set_bg_angles(cd_arc_y[i], 0, 360);
        lv_arc_set_rotation(cd_arc_y[i], 270);
        lv_arc_set_range(cd_arc_y[i], 0, 100);
        lv_arc_set_value(cd_arc_y[i], 100);
        lv_obj_set_style_arc_color(cd_arc_y[i], lv_color_hex(0x333344), LV_PART_MAIN);
        lv_obj_set_style_arc_color(cd_arc_y[i], cd_colors[i], LV_PART_INDICATOR); /* 蓝/红/绿 */
        lv_obj_set_style_arc_width(cd_arc_y[i], 5, LV_PART_MAIN);
        lv_obj_set_style_arc_width(cd_arc_y[i], 5, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(cd_arc_y[i], LV_OPA_TRANSP, LV_PART_KNOB);
        lv_obj_set_style_pad_all(cd_arc_y[i], 0, LV_PART_KNOB);
        lv_obj_add_flag(cd_arc_y[i], LV_OBJ_FLAG_HIDDEN);

        cd_ready_y[i] = lv_label_create(hud_layer);
        lv_label_set_text(cd_ready_y[i], "Ready!");
        lv_obj_set_style_text_font(cd_ready_y[i], &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(cd_ready_y[i], lv_color_hex(0x44FF44), 0);
        lv_obj_align_to(cd_ready_y[i], cd_arc_y[i], LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
        lv_obj_add_flag(cd_ready_y[i], LV_OBJ_FLAG_HIDDEN);
    }

    // 遮挡关系
    lv_obj_move_foreground(hud_layer);
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        if (hud_imgs[i])
            lv_obj_move_background(hud_imgs[i]);
        if (coin_bars[i])
            lv_obj_move_background(coin_bars[i]);
    }

    // 事件注册
    event_register(EVENT_GAME_START,ui_play_event_game_start_cb);
    event_register(EVENT_GAME_OVER,ui_play_event_game_over_cb);
    event_register(EVENT_GAME_WIN, ui_play_event_game_win_cb);
    mp_event_register(MP_EVENT_DISCONNECTED,on_mp_disconnected);

    // 技能CD可视化更新定时器 (每100ms更新一次)
    cd_update_timer = lv_timer_create(cd_update_timer_cb, 100, NULL);
    if (cd_update_timer) {};
}

/**
 * @brief 在 coin 等数据源初始化之后注册事件，确保数据先更新再刷新 UI
 */
void ui_play_register_events(void)
{
    event_register(EVENT_PLAYER_HIT_COIN, ui_play_event_hit_coin_cb);
    event_register(EVENT_PLAYER_HIT_ENEMY, ui_play_event_player_hurt_cb);
    event_register(EVENT_BULLET_HIT_PLAYER, ui_play_event_player_hurt_cb);

    mp_event_register(MP_EVENT_DISCONNECTED, on_mp_disconnected);

    // 技能CD可视化更新定时器 (每100ms更新一次)
    cd_update_timer = lv_timer_create(cd_update_timer_cb, 100, NULL);
    if (cd_update_timer)
    {
    }; // 防止cd_update_timer未使用警告
}

/**
 * @brief 加载play界面 && 负责控制弹窗是否显示 && 播放BGM
 */
void ui_play_run()
{
    // CONSOLE_DEBUG("START ui_play_run");

    if (dp_play == NULL)
    {
        CONSOLE_ERROR("dp_play is NULL!");
        LOG_ERROR("dp_play is NULL!");
        return;
    }

    lv_scr_load(dp_play);
    // GS_OVER 时隐藏右上角按钮，防止误触暂停
    if (fsm_get_state() == GS_OVER)
    {
        if (pause_icon_btn)
            lv_obj_add_flag(pause_icon_btn, LV_OBJ_FLAG_HIDDEN);
        // 更新得分显示: 当局获得金币数 * 10 (per-player)
        // 更新得分显示: 当局获得金币数 * 10 (per-player)
        {
            char buf[128] = {0};
            char tmp[32];

            for (int i = 0; i < MAX_PLAYER_COUNT; i++)
            {
                game_obj_t *p = player_get(i);
                if (p && player_was_active_get(p))
                {
                    int score = player_coin_count_get(p) * 10;
                    if (score < 0)
                        score = 0;
                    snprintf(tmp, sizeof(tmp), "P%d: %d ", i + 1, score);
                    strncat(buf, tmp, sizeof(buf) - strlen(buf) - 1);
                }
            }

            // 去掉末尾多余的空格
            size_t len = strlen(buf);
            if (len > 0 && buf[len - 1] == ' ')
            {
                buf[len - 1] = '\0';
            }

            // 如果没有活跃玩家，显示默认文本
            if (strlen(buf) == 0)
            {
                lv_label_set_text(over_score_label, "Score: 0");
            }
            else
            {
                lv_label_set_text(over_score_label, buf);
            }
        }
        // 根据 is_win 切换弹窗文字
        if (is_win) {
            lv_label_set_text(over_label, "You Are The WINNER!");
            lv_obj_set_style_text_color(over_label, lv_color_hex(0xFFD700), LV_STATE_DEFAULT);
            if (over_restart_btn_label) lv_label_set_text(over_restart_btn_label, "Play Again");
        } else {
            lv_label_set_text(over_label, "GAME OVER");
            lv_obj_set_style_text_color(over_label, lv_palette_main(LV_PALETTE_RED), LV_STATE_DEFAULT);
            if (over_restart_btn_label) lv_label_set_text(over_restart_btn_label, "Restart");
        }
        popup_show(over_popup);
        set_group(over_group);
    }
    else
    {
        if (pause_icon_btn)
            lv_obj_clear_flag(pause_icon_btn, LV_OBJ_FLAG_HIDDEN);
        popup_hide(over_popup);
    }
    if (fsm_get_state() == GS_PAUSE)
    {
        popup_show(pause_popup);
        set_group(pause_group);
    }
    else
    {
        popup_hide(pause_popup);
    }

    if (fsm_get_state() == GS_PLAY)
    {
        CONSOLE_INFO("Processing GS_PLAY, setting group to NULL");
        set_group(NULL);
    }

    // CONSOLE_DEBUG("END ui_play_run successfully");
}

/**
 * @brief 获取play界面 只给game_init()调用
 * @return play界面
 */
lv_obj_t *ui_play_get_display(void)
{
    return dp_play;
}

/**
 * @brief Level 进场动画
 * @param level_name 自定义名称
 */
void ui_play_level_enter_anim(const char *level_name)
{
    // CONSOLE_DEBUG("Start with name '%s'", level_name);

    lv_obj_t *label_level = lv_label_create(hud_layer);
    if (label_level == NULL)
    {
        CONSOLE_WARNING("label_level creation failed!");
        LOG_WARNING("label_level creation failed!");
        return;
    }

    lv_label_set_text(label_level, level_name);
    lv_obj_align(label_level, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_set_style_text_color(label_level, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_level, &lv_font_montserrat_44, 0);
    lv_obj_move_foreground(label_level);
    lv_obj_set_y(label_level, -90);
    lv_obj_set_style_opa(label_level, LV_OPA_TRANSP, 0);

    static lv_anim_t anim_move, anim_fade;

    // CONSOLE_DEBUG("Launching move animation...");
    lv_anim_init(&anim_move);
    lv_anim_set_var(&anim_move, label_level);
    lv_anim_set_exec_cb(&anim_move, y_anim_cb);
    lv_anim_set_values(&anim_move, -90, 30);
    lv_anim_set_time(&anim_move, 600);
    lv_anim_set_ready_cb(&anim_move, level_anim_finish);
    lv_anim_start(&anim_move);

    // CONSOLE_DEBUG("Launching fade animation...");
    lv_anim_init(&anim_fade);
    lv_anim_set_var(&anim_fade, label_level);
    lv_anim_set_exec_cb(&anim_fade, opa_anim_cb);
    lv_anim_set_values(&anim_fade, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&anim_fade, 600);
    lv_anim_start(&anim_fade);

    // CONSOLE_INFO("Complete");
}

/**
 * @brief 控制冰冻减速遮罩的显示/隐藏 (Stream Y技能)
 * @param show true=显示遮罩, false=隐藏遮罩
 */
void ui_play_set_freeze_overlay(bool show)
{
    if (freeze_overlay == NULL)
        return;
    if (show)
    {
        lv_obj_clear_flag(freeze_overlay, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(freeze_overlay, LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * @brief 获取hud层
 * @return hud层
 */
lv_obj_t *ui_play_get_hud_layer(void)
{
    if (hud_layer == NULL)
    {
        CONSOLE_ERROR("hud_layer is NULL!");
        LOG_ERROR("hud_layer is NULL!");
        return NULL;
    }
    return hud_layer;
}

/**********************
 * STATIC FUNCTIONS
 **********************/
/**
 * @brief 玩家拾取金币事件回调 (金币标签由 cd_update_timer 轮询更新)
 */
static void ui_play_event_hit_coin_cb(game_obj_t *src, game_obj_t *trg)
{
    (void)src;
    (void)trg;
    // 金币标签由 cd_update_timer_cb 每 100ms 轮询更新
}

/**
 * @brief 按下回退到菜单
 */
static void pause_exit_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    audio_load(AUDIO_MOUSECLOSE, AUDIO_CHAN_AUTO, false);
    event_dispatch(EVENT_GAME_OVER, NULL, NULL);
    CONSOLE_DEBUG("Game over by exit button.");
    fsm_switch_state(GS_MENU);
    CONSOLE_INFO("State has been switched to %d", fsm_get_state());
}

/**
 * @brief 按下继续游戏
 */
static void pause_continue_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    audio_load(AUDIO_MOUSECLOSE, AUDIO_CHAN_AUTO, false);
    fsm_switch_state(GS_PLAY);
    CONSOLE_INFO("State has been switched to %d", fsm_get_state());
}

/**
 * @brief 按下暂停/继续
 */
static void pause_btn_event_cb(lv_event_t *e)
{
    switch (fsm_get_state())
    {
    case GS_PLAY:
        audio_load(AUDIO_MOUSEOPEN, AUDIO_CHAN_AUTO, false);
        fsm_switch_state(GS_PAUSE);
        CONSOLE_INFO("State has been switched to %d", fsm_get_state());
        break;
    case GS_PAUSE:
        audio_load(AUDIO_MOUSECLOSE, AUDIO_CHAN_AUTO, false);
        fsm_switch_state(GS_PLAY);
        CONSOLE_INFO("State has been switched to %d", fsm_get_state());
        break;
    default:
        CONSOLE_WARNING("Unknown state %d", fsm_get_state());
        LOG_WARNING("Unknown state %d", fsm_get_state());
        break;
    }
}

/**
 * @brief 暂停弹窗中的 Settings 按钮
 */
static void pause_setting_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    ui_setting_set_prev_state(GS_PAUSE);
    audio_load(AUDIO_MOUSEOPEN, AUDIO_CHAN_AUTO, false);
    fsm_switch_state(GS_SETTING);
    CONSOLE_INFO("Open settings from pause popup.");
}

/**
 * @brief 重新开始 — 保留金币，重新初始化关卡
 */
static void over_exit_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    /* EVENT_GAME_OVER 已由 player_on_death 派发, 这里只需切状态 */
    fsm_switch_state(GS_MENU);
}

/**
 * @brief 重新开始 / Play Again — 重新初始化关卡
 */
static void over_restart_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    fsm_switch_state(GS_PLAY);
    event_dispatch(EVENT_GAME_START, NULL, NULL);
}

/**
 * @brief 透明度动画回调 对进场level动画使用
 */
static void opa_anim_cb(void *obj, int32_t opa)
{
    if (obj != NULL)
    {
        lv_obj_set_style_opa((lv_obj_t *)obj, opa, 0);
    }
}

/**
 * @brief y坐标动画回调 对进场level动画使用
 */
static void y_anim_cb(void *obj, int32_t y)
{
    if (obj != NULL)
    {
        lv_obj_set_y((lv_obj_t *)obj, y);
    }
}

/**
 * @brief Level 进场动画结束
 */
static void level_anim_finish(lv_anim_t *anim)
{
    // CONSOLE_DEBUG("Animation finish ready_cb entered");
    if (anim == NULL || anim->var == NULL)
    {
        CONSOLE_WARNING("anim or anim->var is NULL!");
        LOG_WARNING("anim or anim->var is NULL!");
        return;
    }

    lv_obj_t *label = anim->var;
    lv_anim_t fade_out;
    lv_anim_init(&fade_out);
    lv_anim_set_var(&fade_out, label);
    lv_anim_set_exec_cb(&fade_out, opa_anim_cb);
    lv_anim_set_values(&fade_out, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&fade_out, 500);
    lv_anim_set_delay(&fade_out, 1000);
    lv_anim_start(&fade_out);
    // CONSOLE_INFO("Fade out animation scheduled");
}

/**
 * @brief 游戏开始事件回调 — 显示活跃玩家的 HUD
 * @note 必须在 player 模块之后注册事件, 保证 player_spawn 已完成。
 *       game_init() 先调用 player_init() 再调用 ui_play_init_stage2(),
 *       因此 player_on_start (EVENT_GAME_START) 先于此回调执行。
 */
/**
 * @brief 游戏开始事件回调 — 显示活跃玩家的 HUD, 初始化 was_active 快照
 * @note 必须在 player 模块之后注册事件, 保证 player_spawn 已完成。
 *       game_init() 先调用 player_init() 再调用 ui_play_init_stage2(),
 *       因此 player_on_start (EVENT_GAME_START) 先于此回调执行。
 */
static void ui_play_event_game_start_cb(game_obj_t *a, game_obj_t *b)
{
    (void)a, (void)b;

    is_win = false;  // 重置胜利标志

    /* 初始化 was_active 快照: 活跃玩家保持 true, 未生成的设为 false */
    for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
        game_obj_t *p = player_get(i);
        if (p == NULL || !p->active)
            player_was_active_set(p, false);
    }

    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        game_obj_t *p = player_get(i);
        bool active = (p != NULL && p->active);
        CONSOLE_DEBUG("player %d active: %d", i, active);

        if (hud_imgs[i])
        {
            if (active)
                lv_obj_clear_flag(hud_imgs[i], LV_OBJ_FLAG_HIDDEN);
            else
            {
                CONSOLE_DEBUG("hud img %d hidden", i);
                lv_obj_add_flag(hud_imgs[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (coin_bars[i])
        {
            if (active)
                lv_obj_clear_flag(coin_bars[i], LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(coin_bars[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (cd_bg_x[i])
        {
            if (active)
                lv_obj_clear_flag(cd_bg_x[i], LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(cd_bg_x[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (cd_arc_x[i])
        {
            if (active)
                lv_obj_clear_flag(cd_arc_x[i], LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(cd_arc_x[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (cd_bg_y[i])
        {
            if (active)
                lv_obj_clear_flag(cd_bg_y[i], LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(cd_bg_y[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (cd_arc_y[i])
        {
            if (active)
                lv_obj_clear_flag(cd_arc_y[i], LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(cd_arc_y[i], LV_OBJ_FLAG_HIDDEN);
        }
        // "Ready!" 标签由 cd_update_timer 根据 CD 百分比管理
    }

    CONSOLE_INFO("Level 1 Animation Start.");
    ui_play_level_enter_anim("Level 1");
}

/**
 * @brief 技能CD可视化 + 金币标签轮询定时器回调 (每100ms)
 */
static void cd_update_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    if (fsm_get_state() != GS_PLAY)
        return;

    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        game_obj_t *p = player_get(i);
        if (p == NULL || !p->active)
            continue; /* 死亡后保留 HUD, 不更新也不隐藏 */

        const character_config_t *cfg = player_character_get(p);
        uint32_t now = play_tick_get();

        // X 技能 CD
        if (cd_arc_x[i])
        {
            uint32_t elapsed_x = now - player_skill_x_last_use_get(p);
            int pct_x = (cfg->skill_x_cd > 0)
                            ? (int)(elapsed_x * 100 / cfg->skill_x_cd)
                            : 100;
            if (pct_x > 100)
                pct_x = 100;
            lv_arc_set_value(cd_arc_x[i], pct_x);
            if (cd_ready_x[i])
            {
                if (pct_x >= 100)
                    lv_obj_clear_flag(cd_ready_x[i], LV_OBJ_FLAG_HIDDEN);
                else
                    lv_obj_add_flag(cd_ready_x[i], LV_OBJ_FLAG_HIDDEN);
            }
        }

        // Y 技能 CD
        if (cd_arc_y[i])
        {
            uint32_t elapsed_y = now - player_skill_y_last_use_get(p);
            int pct_y = (cfg->skill_y_cd > 0)
                            ? (int)(elapsed_y * 100 / cfg->skill_y_cd)
                            : 100;
            if (pct_y > 100)
                pct_y = 100;
            lv_arc_set_value(cd_arc_y[i], pct_y);
            if (cd_ready_y[i])
            {
                if (pct_y >= 100)
                    lv_obj_clear_flag(cd_ready_y[i], LV_OBJ_FLAG_HIDDEN);
                else
                    lv_obj_add_flag(cd_ready_y[i], LV_OBJ_FLAG_HIDDEN);
            }
        }

        // 金币标签轮询
        if (coin_labels[i])
            lv_label_set_text_fmt(coin_labels[i], "%d", player_coin_count_get(p));

        // CONSOLE_DEBUG("Player %d,coin count: %d", i, player_coin_count_get(p));
    }
}

/**
 * @brief 受击闪烁定时器回调：隐藏受击遮罩
 */
static void hurt_flash_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    if (hurt_img != NULL)
    {
        lv_obj_add_flag(hurt_img, LV_OBJ_FLAG_HIDDEN);
    }
    hurt_timer = NULL;
}

/**
 * @brief 玩家受击事件回调：显示受击红色遮罩并启动闪烁定时器
 */
static void ui_play_event_player_hurt_cb(game_obj_t *a, game_obj_t *b)
{
    LV_UNUSED(a);
    LV_UNUSED(b);
    if (hurt_img == NULL)
        return;
    if (!show_hurt_overlay)
        return;
    lv_obj_clear_flag(hurt_img, LV_OBJ_FLAG_HIDDEN);
    if (hurt_timer == NULL)
    {
        hurt_timer = lv_timer_create(hurt_flash_timer_cb, 200, NULL);
        lv_timer_set_repeat_count(hurt_timer, 1);
    }
}

/**
 * @brief 游戏结束事件回调 — 结算金币 + 隐藏 HUD
 * @note LOCAL/CONTROLLER 玩家的金币加入本地钱包, REMOTE 玩家通过 comm 同步
 */
static void ui_play_event_game_over_cb(game_obj_t *a, game_obj_t *b)
{
    (void)a, (void)b;

    /* ---- 金币结算 ---- */
    for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
        game_obj_t *p = player_get(i);
        if (!p || !player_was_active_get(p)) continue;

        int coins = player_coin_count_get(p);
        input_device_t *dev = (input_device_t *)p->behave.usr_data;
        if (dev && dev->type == INPUT_DEVICE_REMOTE) {
            comm_send_coin_sync(coins);
        } else {
            ui_shop_coin_add(coins);
        }
    }
    /* #2: 通知从机游戏结束 */
    comm_send_lobby_state(0);

    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        if (hud_imgs[i])
        {
            CONSOLE_DEBUG("hud img %d hidden", i);
            lv_obj_add_flag(hud_imgs[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (coin_bars[i])
            lv_obj_add_flag(coin_bars[i], LV_OBJ_FLAG_HIDDEN);
        if (cd_bg_x[i])
            lv_obj_add_flag(cd_bg_x[i], LV_OBJ_FLAG_HIDDEN);
        if (cd_arc_x[i])
            lv_obj_add_flag(cd_arc_x[i], LV_OBJ_FLAG_HIDDEN);
        if (cd_bg_y[i])
            lv_obj_add_flag(cd_bg_y[i], LV_OBJ_FLAG_HIDDEN);
        if (cd_arc_y[i])
            lv_obj_add_flag(cd_arc_y[i], LV_OBJ_FLAG_HIDDEN);
        if (cd_ready_x[i])
            lv_obj_add_flag(cd_ready_x[i], LV_OBJ_FLAG_HIDDEN);
        if (cd_ready_y[i])
            lv_obj_add_flag(cd_ready_y[i], LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * @brief 通关胜利事件回调：设置胜利标志并切换到 GS_OVER 弹窗
 */
static void ui_play_event_game_win_cb(game_obj_t * a, game_obj_t * b)
{
    (void)a; (void)b;
    is_win = true;
    fsm_switch_state(GS_OVER);
    CONSOLE_INFO("Victory! Switching to win screen.");
}

static void on_mp_disconnected(mp_event_t e,void * v)
{
    (void)e, (void)v;
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        if (hud_imgs[i])
        {
            CONSOLE_DEBUG("hud img %d hidden", i);
            lv_obj_add_flag(hud_imgs[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (coin_bars[i])
            lv_obj_add_flag(coin_bars[i], LV_OBJ_FLAG_HIDDEN);
        if (cd_bg_x[i])
            lv_obj_add_flag(cd_bg_x[i], LV_OBJ_FLAG_HIDDEN);
        if (cd_arc_x[i])
            lv_obj_add_flag(cd_arc_x[i], LV_OBJ_FLAG_HIDDEN);
        if (cd_bg_y[i])
            lv_obj_add_flag(cd_bg_y[i], LV_OBJ_FLAG_HIDDEN);
        if (cd_arc_y[i])
            lv_obj_add_flag(cd_arc_y[i], LV_OBJ_FLAG_HIDDEN);
        if (cd_ready_x[i])
            lv_obj_add_flag(cd_ready_x[i], LV_OBJ_FLAG_HIDDEN);
        if (cd_ready_y[i])
            lv_obj_add_flag(cd_ready_y[i], LV_OBJ_FLAG_HIDDEN);
    }
}
