/**
 * @file shop.c
 * @brief 商店抽奖界面 (添加返回退出弹窗、优化中奖金币图标尺寸为 18*18)
 */

#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ui_shop.h"
#include "ui_base.h"

/* 引入基础工具与状态机头文件 */
#include "tools.h"
#include "event.h"
#include "fsm.h"
#include "lvgl_utils.h"
#include "ui_templates.h" // 用于 popup_create, popup_show, popup_hide
#include "coin.h"
#include "audio.h"

/**********************
 * MACROS
 **********************/
#define SHOP_BG_IMG "shop_bg.bin"
#define PLAYER_EMBER_IMG "player_ember.bin"
#define PLAYER_STREAM_IMG "player_stream.bin"
#define PLAYER_VERDANT_IMG "player_verdant.bin"
#define COIN_LEAVE_IMG "coin.bin"
#define COIN_IMG_NAME "coin_bar.bin"
#define BASE_BACK_ICON "back_arrow.bin"
#define DRAW_COST 160
#define TOTAL_SLOTS 8

// 网格单元大小
#define CELL_WIDTH 120
#define CELL_HEIGHT 120
#define CELL_PAD 10

/**********************
 * TYPEDEFS
 **********************/
typedef struct
{
    int id;
    const char *name;
    const char *img_src;
    int probability; // 万分比权重
} reward_info_t;

/**********************
 * STATIC VARIABLES
 **********************/
static lv_obj_t *dp_shop = NULL;
static lv_obj_t *highlight_cursor = NULL;
static lv_timer_t *roulette_timer = NULL;

/* 中奖弹窗特有UI组件 */
static lv_obj_t *reward_popup = NULL;
static lv_obj_t *reward_msg = NULL;
static lv_obj_t *reward_img = NULL;
/* 左下角蓝币显示UI */
static lv_obj_t *coin_label = NULL;
/* 新增：退出/暂停确认弹窗组件 */
static lv_obj_t *shop_exit_popup = NULL;

static lv_group_t *shop_group = NULL;

static int esc_cnt = 0; // 用来反复按esc键退出弹窗

static int current_slot = 0;
static int target_slot = 0;
static int steps_remaining = 0;
static int current_speed_ms = 40;
static int draw_count = 0;
// 奖池定义（按顺时针排列 0~7）
static reward_info_t rewards[TOTAL_SLOTS] = {
    {0, "Ember Plane", PLAYER_EMBER_IMG, 150},     // 0-左上
    {1, "Coin x5", COIN_LEAVE_IMG, 3000},          // 1-中上
    {2, "Stream Plane", PLAYER_STREAM_IMG, 150},   // 2-右上
    {3, "Coin x10", COIN_LEAVE_IMG, 2000},         // 3-右中
    {4, "Verdant Plane", PLAYER_VERDANT_IMG, 150}, // 4-右下
    {5, "Coin x20", COIN_LEAVE_IMG, 1000},         // 5-中下
    {6, "Better luck", NULL, 3000},                // 6-左下
    {7, "Coin x200", COIN_LEAVE_IMG, 550}          // 7-左中
};

// 顺时针 3x3 网格逻辑坐标映射表
static const int slot_coords[TOTAL_SLOTS][2] = {
    {0, 0}, {1, 0}, {2, 0}, {2, 1}, {2, 2}, {1, 2}, {0, 2}, {0, 1}};

#ifdef SIMULATOR
static lv_img_dsc_t shop_bg_dsc, ember_dsc, stream_dsc, verdant_dsc, coin_dsc, popup_reward_dsc, coin_img_dsc, back_arrow_dsc;
#endif

/**********************
 * STATIC PROTOTYPES
 **********************/
static void draw_btn_event_cb(lv_event_t *e);
static void roulette_timer_cb(lv_timer_t *timer);
static int get_random_reward_slot(void);
static void update_cursor_position(int slot);
static void give_reward(int slot);
static void reward_close_cb(lv_event_t *e);
static void show_reward_popup(int slot);

/* 新增：退出相关事件回调 */
static void shop_exit_btn_event_cb(lv_event_t *e);
static void shop_back_menu_btn_event_cb(lv_event_t *e);
static void shop_continue_draw_btn_event_cb(lv_event_t *e);

/**********************
 * GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化商店界面 根容器等可能与其它模块共享的资源
 */
void ui_shop_init_stage1(void)
{
    // 创建独立画布
    dp_shop = lv_obj_create(NULL);
    lv_obj_clear_flag(dp_shop, LV_OBJ_FLAG_SCROLLABLE);
}

/**
 * @brief 商店界面的全布局显式初始化
 * @note 建议在程序刚启动时（例如在 main.c 中与其他 UI 初始化函数一起）调用一次。
 */
void ui_shop_init_stage2(void)
{
    shop_group = lv_group_create();

    // 检查独立画布
    if (dp_shop == NULL)
    {
        CONSOLE_ERROR("dp_shop is NULL,cannot init shop interface.");
        LOG_ERROR("dp_shop is NULL,cannot init shop interface.");
        return;
    }

    static char bg_path_buf[64];
    static char btn_path_buf[64];
    static char cell_path_bufs[TOTAL_SLOTS][64];

    lv_obj_t *bg;
#ifdef SIMULATOR
    bg = img_create_from_dsc(dp_shop, img_path(SHOP_BG_IMG, bg_path_buf, 64), 1024, 600, NULL, &shop_bg_dsc, false);
#else
    bg = lv_img_create(dp_shop);
    lv_img_set_src(bg, img_path(SHOP_BG_IMG, bg_path_buf, 64));
#endif
    lv_obj_center(bg);

    // coin_img initialize
    static char img_path_buf[64];
#ifdef SIMULATOR
    lv_obj_t *coin_img = img_create_from_dsc(dp_shop, img_path(COIN_IMG_NAME, img_path_buf, 64), 166, 46, NULL, &coin_img_dsc, true);
    lv_obj_set_align(coin_img, LV_ALIGN_BOTTOM_LEFT);
#else
    lv_obj_t *coin_img = lv_img_create(dp_shop);
    lv_img_set_src(coin_img, img_path(COIN_IMG_NAME, img_path_buf, 64));
    lv_obj_set_align(coin_img, LV_ALIGN_BOTTOM_LEFT);
#endif

    lv_obj_t *exit_btn = lv_btn_create(dp_shop);
    lv_obj_set_size(exit_btn, 64, 64);
    lv_obj_set_align(exit_btn, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_pos(exit_btn, 0, 0); // 微调边缘间距
    lv_obj_add_event_cb(exit_btn, shop_exit_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_opa(exit_btn, 0, 0);

    lv_obj_t *back_icon;
#ifdef SIMULATOR
    back_icon = img_create_from_dsc(dp_shop, img_path(BASE_BACK_ICON, bg_path_buf, 64), 64, 64, NULL, &back_arrow_dsc, true);
    lv_obj_align(back_icon, LV_ALIGN_TOP_RIGHT, 0, 0);
#else
    back_icon = lv_img_create(dp_shop);
    lv_img_set_src(back_icon, img_path(BASE_BACK_ICON, bg_path_buf, 64));
    lv_obj_align(back_icon, LV_ALIGN_TOP_RIGHT, 0, 0);
#endif

    // 九宫格区域容器
    lv_obj_t *grid_cont = lv_obj_create(bg);
    lv_obj_clear_flag(grid_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(grid_cont, (CELL_WIDTH + CELL_PAD) * 3, (CELL_HEIGHT + CELL_PAD) * 3);
    lv_obj_set_style_bg_opa(grid_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid_cont, 0, 0);

    lv_obj_align(grid_cont, LV_ALIGN_CENTER, -27, -25);

    // 绘制外围 8 个抽奖格子
    for (int i = 0; i < TOTAL_SLOTS; i++)
    {
        lv_obj_t *cell = lv_obj_create(grid_cont);
        lv_obj_set_size(cell, CELL_WIDTH, CELL_HEIGHT);
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(cell, slot_coords[i][0] * (CELL_WIDTH + CELL_PAD), slot_coords[i][1] * (CELL_HEIGHT + CELL_PAD));

        lv_obj_t *label = lv_label_create(cell);
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_t *img = NULL;

        switch (i)
        {
        case 0: // 左上
#ifdef SIMULATOR
            img = img_create_from_dsc(cell, img_path(PLAYER_EMBER_IMG, cell_path_bufs[i], 64), 64, 64, NULL, &ember_dsc, false);
#else
            img = lv_img_create(cell);
            lv_img_set_src(img, img_path(PLAYER_EMBER_IMG, cell_path_bufs[i], 64));
#endif
            lv_label_set_text(label, "Ember");
            lv_obj_set_style_text_color(label, lv_color_hex(0xFF0000), LV_STATE_DEFAULT);
            break;
        case 2: // 右上
#ifdef SIMULATOR
            img = img_create_from_dsc(cell, img_path(PLAYER_STREAM_IMG, cell_path_bufs[i], 64), 64, 64, NULL, &stream_dsc, false);
#else
            img = lv_img_create(cell);
            lv_img_set_src(img, img_path(PLAYER_STREAM_IMG, cell_path_bufs[i], 64));
#endif
            lv_label_set_text(label, "Stream");
            lv_obj_set_style_text_color(label, lv_color_hex(0x0000FF), LV_STATE_DEFAULT);
            break;
        case 4: // 右下
#ifdef SIMULATOR
            img = img_create_from_dsc(cell, img_path(PLAYER_VERDANT_IMG, cell_path_bufs[i], 64), 64, 64, NULL, &verdant_dsc, false);
#else
            img = lv_img_create(cell);
            lv_img_set_src(img, img_path(PLAYER_VERDANT_IMG, cell_path_bufs[i], 64));
#endif
            lv_label_set_text(label, "Verdant");
            lv_obj_set_style_text_color(label, lv_color_hex(0x00FF00), LV_STATE_DEFAULT);
            break;
        case 6: // 左下: 谢谢参与
            lv_label_set_text(label, "Better luck\nnext time");
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
            break;
        default: // 周围四个金币框
#ifdef SIMULATOR
            img = img_create_from_dsc(cell, img_path(COIN_LEAVE_IMG, cell_path_bufs[i], 64), 18, 18, NULL, &coin_dsc, true);
#else
            img = lv_img_create(cell);
            lv_img_set_src(img, img_path(COIN_LEAVE_IMG, cell_path_bufs[i], 64));
#endif
            if (i == 1)
                lv_label_set_text(label, "x5");
            if (i == 3)
                lv_label_set_text(label, "x10");
            if (i == 5)
                lv_label_set_text(label, "x20");
            if (i == 7)
                lv_label_set_text(label, "x200");
            break;
        }
        if (img)
            lv_obj_align(img, LV_ALIGN_CENTER, 0, -10);
    }

    // 中心抽奖 Button (逻辑坐标 1,1)
    lv_obj_t *draw_btn = lv_btn_create(grid_cont);
    lv_obj_set_size(draw_btn, CELL_WIDTH, CELL_HEIGHT);
    lv_obj_set_pos(draw_btn, 1 * (CELL_WIDTH + CELL_PAD), 1 * (CELL_HEIGHT + CELL_PAD));
    lv_obj_add_event_cb(draw_btn, draw_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(shop_group, draw_btn);

    // 按钮上下垂直排布
    lv_obj_set_flex_flow(draw_btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(draw_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *btn_label_top = lv_label_create(draw_btn);
    lv_label_set_text_fmt(btn_label_top, "%d", DRAW_COST);

    lv_obj_t *btn_img = lv_img_create(draw_btn);
    lv_img_set_src(btn_img, img_path(COIN_LEAVE_IMG, btn_path_buf, 64));

    lv_obj_t *btn_label_bot = lv_label_create(draw_btn);
    lv_label_set_text(btn_label_bot, "per draw");

    // 抽奖跑马灯发光框
    highlight_cursor = lv_obj_create(grid_cont);
    lv_obj_set_size(highlight_cursor, CELL_WIDTH + 8, CELL_HEIGHT + 8);
    lv_obj_set_style_bg_opa(highlight_cursor, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(highlight_cursor, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_border_width(highlight_cursor, 4, 0);
    lv_obj_set_style_shadow_color(highlight_cursor, lv_palette_main(LV_PALETTE_CYAN), 0);
    lv_obj_set_style_shadow_width(highlight_cursor, 10, 0);

    update_cursor_position(current_slot);

    // 💡 建立并隐藏中奖弹窗面板
    reward_popup = popup_create(dp_shop);
    lv_obj_add_flag(reward_popup, LV_OBJ_FLAG_HIDDEN);

    reward_msg = lv_label_create(reward_popup);
    lv_obj_set_style_text_align(reward_msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(reward_msg, &lv_font_montserrat_20, 0);
    lv_obj_align(reward_msg, LV_ALIGN_TOP_MID, 0, 25);

#ifndef SIMULATOR
    reward_img = lv_img_create(reward_popup);
    lv_obj_align(reward_img, LV_ALIGN_CENTER, 0, -10);
#endif

    lv_obj_t *close_btn = lv_btn_create(reward_popup);
    lv_obj_set_size(close_btn, 160, 45);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -25);
    lv_obj_add_event_cb(close_btn, reward_close_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(shop_group, close_btn);

    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "CONTINUE");
    lv_obj_center(close_label);

    // ==========================================
    // 新增：向 play 界面看齐的 Exit/Pause 弹窗
    // ==========================================
    shop_exit_popup = popup_create(dp_shop);
    lv_obj_add_flag(shop_exit_popup, LV_OBJ_FLAG_HIDDEN);

    // 弹窗主标题
    lv_obj_t *pause_label = lv_label_create(shop_exit_popup);
    lv_obj_set_pos(pause_label, 10, 50);
    lv_label_set_text(pause_label, "LEAVE SHOP?");
    lv_obj_set_style_text_font(pause_label, &lv_font_montserrat_44, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(pause_label, lv_color_hex(0x13AEFB), LV_STATE_DEFAULT);

    // 继续抽奖按钮
    lv_obj_t *shop_continue_btn = lv_btn_create(shop_exit_popup);
    lv_obj_set_size(shop_continue_btn, 300, 60);
    lv_obj_set_pos(shop_continue_btn, 25, 240);
    lv_obj_add_event_cb(shop_continue_btn, shop_continue_draw_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(shop_group, shop_continue_btn);

    lv_obj_t *continue_btn_label = lv_label_create(shop_continue_btn);
    lv_obj_set_align(continue_btn_label, LV_ALIGN_CENTER);
    lv_label_set_text(continue_btn_label, "Continue");
    lv_obj_set_style_text_font(continue_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    // 返回菜单按钮
    lv_obj_t *shop_back_btn = lv_btn_create(shop_exit_popup);
    lv_obj_set_size(shop_back_btn, 300, 60);
    lv_obj_set_pos(shop_back_btn, 25, 320);
    lv_obj_add_event_cb(shop_back_btn, shop_back_menu_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(shop_group, shop_back_btn);

    lv_obj_t *back_btn_label = lv_label_create(shop_back_btn);
    lv_obj_set_align(back_btn_label, LV_ALIGN_CENTER);
    lv_label_set_text(back_btn_label, "Leave");
    lv_obj_set_style_text_font(back_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    coin_label = lv_label_create(coin_img);
    lv_obj_set_pos(coin_label, 120, 23);
    lv_label_set_text_fmt(coin_label, "%d", 0);
    lv_obj_set_style_text_font(coin_label, &lv_font_montserrat_16, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(coin_label, lv_color_white(), LV_STATE_DEFAULT);

    if (coin_label != NULL)
    {
        lv_label_set_text_fmt(coin_label, "%d", 0);
    }
}

/**
 * @brief 状态机绑定的核心商店运行/切入函数
 * @note 当状态机检测到状态切换（last_game_state != fsm_get_state()）
 * 并且当前状态为 GS_SHOP 时，进入 switch-case 触发此函数。
 */
void ui_shop_run(void)
{
    lv_scr_load(dp_shop);

    if (coin_label != NULL)
    {
        lv_label_set_text_fmt(coin_label, "%d", 0);
    }

    // 每次切回界面时重置隐藏状态
    popup_hide(shop_exit_popup);
    popup_hide(reward_popup);

    set_group(shop_group);
}

/**
 * @brief 安全的ui_shop esc交互退出逻辑
 */
void ui_shop_esc_behave(void)
{
    shop_exit_btn_event_cb(NULL);
    esc_cnt = 1 - esc_cnt;
    if (esc_cnt == 0)
    {
        popup_hide(shop_exit_popup);
    }
}

/**
 * @brief 获取当前抽奖次数
 */
int ui_shop_get_draw_cnt(void)
{
    return draw_count;
}

/**
 * @brief 安全设置抽奖次数
 */
void ui_shop_set_draw_cnt(int cnt)
{
    if (cnt < 0)
        return;
    draw_count = cnt;
}

/**********************
 * STATIC FUNCTIONS
 **********************/

static void reward_close_cb(lv_event_t *e)
{
    popup_hide(reward_popup);
}

static void shop_exit_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if (roulette_timer != NULL)
        return; // 正在滚动抽奖时不允许弹出退出
    audio_load(AUDIO_MOUSEOPEN, AUDIO_CHAN_AUTO, false);
    popup_show(shop_exit_popup);
}

static void shop_continue_draw_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    audio_load(AUDIO_MOUSECLOSE, AUDIO_CHAN_AUTO, false);
    popup_hide(shop_exit_popup);
}

static void shop_back_menu_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    popup_hide(shop_exit_popup);
    audio_load(AUDIO_MOUSECLOSE, AUDIO_CHAN_AUTO, false);
    fsm_switch_state(GS_MENU); // 切换状态回主菜单
    console_out("[shop][shop_back_btn] State has been switched to %d\n", fsm_get_state());
}

/**
 * @brief 抽奖按钮点击事件
 */
static void draw_btn_event_cb(lv_event_t *e)
{
    if (roulette_timer != NULL)
        return;
    if (!lv_obj_has_flag(reward_popup, LV_OBJ_FLAG_HIDDEN))
        return;
    if (!lv_obj_has_flag(shop_exit_popup, LV_OBJ_FLAG_HIDDEN))
        return; // 退出窗口显示时无法抽奖

    // 检查金币是否足够
    if (true) // coin_get_num() < DRAW_COST)
    {
        CONSOLE_INFO("Coins insufficient");
        return;
    }

    if (coin_label != NULL)
    {
        lv_label_set_text_fmt(coin_label, "%d", 0); // coin_get_num());
    }

    target_slot = get_random_reward_slot();

    int base_spins = 3;
    int steps_to_target = target_slot - current_slot;
    if (steps_to_target <= 0)
    {
        steps_to_target += TOTAL_SLOTS;
    }
    steps_remaining = (base_spins * TOTAL_SLOTS) + steps_to_target;
    current_speed_ms = 40;

    roulette_timer = lv_timer_create(roulette_timer_cb, current_speed_ms, NULL);
}

/**
 * @brief LVGL 定时器回调：处理顺时针步进与平滑阻尼降速
 */
static void roulette_timer_cb(lv_timer_t *timer)
{
    current_slot = (current_slot + 1) % TOTAL_SLOTS;
    update_cursor_position(current_slot);

    steps_remaining--;

    if (steps_remaining < 15)
    {
        current_speed_ms += 35;
    }
    else if (steps_remaining < 30)
    {
        current_speed_ms += 12;
    }
    else
    {
        current_speed_ms += 1;
    }

    lv_timer_set_period(timer, current_speed_ms);

    if (steps_remaining <= 0)
    {
        lv_timer_del(timer);
        roulette_timer = NULL;

        CONSOLE_INFO("Draw ended. Hit slot %d: %s", current_slot, rewards[current_slot].name);

        give_reward(current_slot);
        show_reward_popup(current_slot);
    }
}

/**
 * @brief 安全弹出中奖视窗并加载图片路径
 */
static void show_reward_popup(int slot)
{
    static char popup_path_buf[64];

    if (slot == 6)
    { // 谢谢参与
        lv_label_set_text(reward_msg, "SURPRISE!\n\nBetter luck next time!");
        lv_obj_set_style_text_color(reward_msg, lv_color_white(), LV_STATE_DEFAULT);
#ifdef SIMULATOR
        if (reward_img != NULL)
        {
            lv_obj_del(reward_img);
            reward_img = NULL;
        }
#else
        lv_obj_add_flag(reward_img, LV_OBJ_FLAG_HIDDEN);
#endif
    }
    else
    {
        lv_label_set_text_fmt(reward_msg, "SURPRISE!\n\nYOU WON:\n%s", rewards[slot].name);
        lv_obj_set_style_text_color(reward_msg, lv_color_white(), LV_STATE_DEFAULT);

        // 判断当前中奖的是否为金币格子（根据名称或者 slot 判定：1,3,5,7位金币）
        bool is_coin = (slot % 2 != 0);
        int img_w = is_coin ? 18 : 64; // 💡 满足要求：如果是金币图，强制指定 18*18；飞机则是 64*64
        int img_h = is_coin ? 18 : 64;

#ifdef SIMULATOR
        if (reward_img != NULL)
        {
            lv_obj_del(reward_img);
            reward_img = NULL;
        }
        // 传入动态识别出的 img_w 和 img_h
        reward_img = img_create_from_dsc(reward_popup, img_path(rewards[slot].img_src, popup_path_buf, 64), img_w, img_h, NULL, &popup_reward_dsc, is_coin);
        lv_obj_align(reward_img, LV_ALIGN_CENTER, 0, -10);
#else
        if (rewards[slot].img_src != NULL)
        {
            lv_img_set_src(reward_img, img_path(rewards[slot].img_src, popup_path_buf, 64));
            // 真机非等比缩放或调整（如果驱动或LVGL版本支持，可显式设置大小，此处大小主要取决于底层bin分辨率，或可用 lv_obj_set_size 限制）
            lv_obj_set_size(reward_img, img_w, img_h);
            lv_obj_clear_flag(reward_img, LV_OBJ_FLAG_HIDDEN);
        }
#endif
    }

    popup_show(reward_popup);
}

/**
 * @brief 实时更新跑马灯高亮光标的位置
 */
static void update_cursor_position(int slot)
{
    int x_pos = slot_coords[slot][0] * (CELL_WIDTH + CELL_PAD) - 4;
    int y_pos = slot_coords[slot][1] * (CELL_HEIGHT + CELL_PAD) - 4;
    lv_obj_set_pos(highlight_cursor, x_pos, y_pos);
}

/**
 * @brief 控制抽奖核心概率算法
 * @note 第 1/3/6 抽固定出 Ember/Verdant/Stream，其余随机
 */
static int get_random_reward_slot(void)
{
    draw_count++;

    // 固定抽奖结果
    switch (draw_count)
    {
    case 1:
        return 2; // stream
    case 6:
        return 4; // Verdant
    default:
        break;
    }

    int rand_val = rand() % 10000;
    int cumulative_prob = 0;

    for (int i = 0; i < TOTAL_SLOTS; i++)
    {
        cumulative_prob += rewards[i].probability;
        if (rand_val < cumulative_prob)
        {
            return i;
        }
    }
    return 6;
}

/**
 * @brief 结算逻辑
 */
static void give_reward(int slot)
{
    switch (slot)
    {
    case 1:
        // coin_add_num(5);
        break;
    case 3:
        // coin_add_num(10);
        break;
    case 5:
        // coin_add_num(20);
        break;
    case 7:
        // coin_add_num(200);
        break;
    case 0:
        ui_base_character_unlock(EMBER);
        CONSOLE_INFO("Unlocked Ember Plane!");
        break;
    case 2:
        ui_base_character_unlock(STREAM);
        CONSOLE_INFO("Unlocked Stream Plane!");
        break;
    case 4:
        ui_base_character_unlock(VERDANT);
        CONSOLE_INFO("Unlocked Verdant Plane!");
        break;
    }

    // 更新金币显示
    if (coin_label != NULL)
    {
        lv_label_set_text_fmt(coin_label, "%d", 0); // coin_get_num());
    }
}
