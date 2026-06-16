/**
 * @file ui_base.c
 * @brief 基地/机库飞机选择界面
 */

#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fsm.h"
#include "ui_templates.h"
#include "lvgl_utils.h"
#include "tools.h"
#include "ui_base.h"
#include "player.h"
#include "apr.h"
#include "multiplayer.h"
#include "audio.h"
#ifdef SIMULATOR
#include "joystick.h"
#include "lkey.h"
#endif
#include "config.h"

/*********************
 * MACROS
 *********************/
#define BASE_BG_IMG "base_bg.bin"
#define BASE_BACK_ICON "back_arrow.bin"
#define MULTI_INVITE_ICON "2DMultiIcon.bin"

#define PLANE_WIDTH 160
#define PLANE_HEIGHT 160
#define PLANE_GAP 60
#ifdef SIMULATOR
#define BOX_W 176
#define BOX_H 386
#define BOX_Y 152
#define NAV_DELAY_MS 200
#define NAV_THRESHOLD 80
#endif
#define START_X 102 // (1024 - (160*4 + 60*3)) / 2 保持完美居中

/**********************
 * TYPEDEFS
 **********************/
typedef struct
{
    int id;
    const char *name;
    const char *img_src;
    int max_hp;
    int damage;
    const char *skill_desc;
} plane_info_t;

/**********************
 * STATIC VARIABLES
 **********************/
static bool g_plane_unlocked[PLANE_ID_MAX] = {true, false, false, false};
static plane_id_t g_selected_plane_id = PLANE_ID_DEFAULT;

static lv_obj_t *dp_base = NULL;

static lv_obj_t *plane_objs[PLANE_ID_MAX] = {NULL};
static lv_obj_t *lock_masks[PLANE_ID_MAX] = {NULL};

static lv_obj_t *choosed_indicator = NULL; // 头顶的 "CHOOSED" 变换标签
static lv_obj_t *detail_panel = NULL;      // 动态移动的属性面板
static lv_obj_t *choose_btn = NULL;        // 选择按钮
static lv_obj_t *choose_lbl = NULL;        // 选择按钮文本
static lv_obj_t *hp_label = NULL;
static lv_obj_t *dmg_label = NULL;
static lv_obj_t *skill_label = NULL;

// 多人联机相关组件
static lv_obj_t *multi_invite_popup = NULL;
static lv_obj_t *multi_invite_popup_label = NULL;
static lv_obj_t *multi_invite_yes_btn = NULL;
static lv_obj_t *multi_invite_no_btn = NULL;
static lv_obj_t *multi_invite_cancel_btn = NULL;
static lv_obj_t *multi_invite_ok_btn = NULL;

static lv_obj_t *base_exit_popup = NULL; // 退出确认弹窗
static int current_viewing_idx = 0;      // 当前正在点击浏览的飞机索引

#ifdef SIMULATOR
// 双人摇杆选角
static plane_id_t g_p1_selected_plane_id = PLANE_ID_DEFAULT;
static plane_id_t g_p2_selected_plane_id = PLANE_ID_DEFAULT;
static int g_p1_current_idx = 0;
static int g_p2_current_idx = 0;

static lv_obj_t *p1_selection_box = NULL;
static lv_obj_t *p2_selection_box = NULL;
static lv_obj_t *p1_indicator = NULL;
static lv_obj_t *p2_indicator = NULL;

static lv_timer_t *selection_timer = NULL;

static bool p1_nav_dir = 0;
static bool p2_nav_dir = 0;

static lv_obj_t *detail_panel_p2 = NULL; // 动态移动的属性面板
static lv_obj_t *choose_btn_p2 = NULL;   // 选择按钮
static lv_obj_t *choose_lbl_p2 = NULL;   // 选择按钮文本

static lv_obj_t *hp_label_p2 = NULL;
static lv_obj_t *dmg_label_p2 = NULL;
static lv_obj_t *skill_label_p2 = NULL;

static non_blocking_timer_t p1_nav_timer = {
    .func = NULL,
    .tick_get = lv_tick_get,
    .delay_ms = NAV_DELAY_MS,
    .last_tick = 0,
};
static non_blocking_timer_t p2_nav_timer = {
    .func = NULL,
    .tick_get = lv_tick_get,
    .delay_ms = NAV_DELAY_MS,
    .last_tick = 0,
};
#endif

// 飞机固有属性配置表
static const plane_info_t plane_templates[PLANE_ID_MAX] = {
    {0, "Player", "player.bin", 200, 34, "E:3-Way Shot\nF:Shield 1s"},
    {1, "Ember", "player_ember.bin", 200, 20, "E:Burn Bullet\nF:Flame Wall"},
    {2, "Stream", "player_stream.bin", 200, 10, "E:Freeze Bullet\nF:Slow Enemy"},
    {3, "Verdant", "player_verdant.bin", 250, 15, "E:Speed Boost\nF:HP Reclaim"}};

// 静态化的长周期路径缓冲区，防止 LVGL 异步渲染时读取到脏数据导致乱码
static char plane_path_buf[PLANE_ID_MAX][64];

#ifdef SIMULATOR
static lv_img_dsc_t base_bg_dsc;
static lv_img_dsc_t plane_base_dsc;
static lv_img_dsc_t plane_dscs[PLANE_ID_MAX];
static lv_img_dsc_t back_arrow_dsc, multi_icon_dsc;
#endif

/**********************
 * STATIC PROTOTYPES
 **********************/
static void plane_click_cb(lv_event_t *e);
static void choose_btn_cb(lv_event_t *e);
static void base_exit_btn_cb(lv_event_t *e);
static void base_continue_btn_cb(lv_event_t *e);
static void base_back_menu_btn_cb(lv_event_t *e);
static void update_detail_panel(int idx);
static void base_invite_btn_cb(lv_event_t *e);
static void base_invite_yes_btn_cb(lv_event_t *e);
static void base_invite_no_btn_cb(lv_event_t *e);
static void base_invite_cancel_btn_cb(lv_event_t *e);
static void base_invite_ok_btn_cb(lv_event_t *e);
static void base_mp_event_cb(mp_event_t event, void *data);
#ifdef SIMULATOR
static void selection_timer_cb(lv_timer_t *timer);
static void p1_move_cb(void);
static void p2_move_cb(void);
static void update_selection_box_pos(lv_obj_t *box, int idx);
static void update_detail_panel_p2(int idx);
static void choose_btn_p2_cb(lv_event_t *e);
#endif

/**********************
 * GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 获取当前选中的飞机ID (新添加的 Get 函数)
 * @return int 当前飞机的 ID
 */
plane_id_t ui_base_get_selected_plane_id(void)
{
    return g_selected_plane_id;
}

/**
 * @brief 查询指定飞机是否已解锁
 * @param plane_id 飞机 ID（PLANE_ID_DEFAULT / EMBER / STREAM / VERDANT）
 * @return true 已解锁, false 未解锁或 ID 无效
 */
bool ui_base_plane_is_unlocked(plane_id_t plane_id)
{
    if (plane_id >= PLANE_ID_MAX)
        return false;
    return g_plane_unlocked[plane_id];
}

/**
 * @brief 解锁指定飞机（从商店抽奖获得后调用）
 * @param plane_id 飞机 ID（PLANE_ID_DEFAULT /_EMBER / STREAM / VERDANT）
 */
void ui_base_plane_unlock(plane_id_t plane_id)
{
    if (plane_id >= PLANE_ID_MAX)
        return;
    g_plane_unlocked[plane_id] = true;
    CONSOLE_INFO("Plane %d unlocked.", plane_id);
}

void ui_base_set_selected_plane_id(plane_id_t id)
{
    if (id >= PLANE_ID_MAX)
        return;
    g_selected_plane_id = id;
}

int ui_base_get_unlocked_mask(void)
{
    int mask = 0;
    for (int i = 0; i < PLANE_ID_MAX; i++)
    {
        if (g_plane_unlocked[i])
            mask |= (1 << i);
    }
    return mask;
}

void ui_base_set_unlocked_mask(int mask)
{
    for (int i = 0; i < PLANE_ID_MAX; i++)
    {
        g_plane_unlocked[i] = (mask & (1 << i)) != 0;
    }
    // 确保 Default 飞机始终解锁
    g_plane_unlocked[PLANE_ID_DEFAULT] = true;
}

plane_id_t ui_base_get_p1_selected_plane_id(void)
{
#ifdef SIMULATOR
    return g_p1_selected_plane_id;
#else
    return PLANE_ID_DEFAULT;
#endif
}

plane_id_t ui_base_get_p2_selected_plane_id(void)
{
#ifdef SIMULATOR
    return g_p2_selected_plane_id;
#else
    return PLANE_ID_DEFAULT;
#endif
}

void ui_base_set_p1_selected_plane_id(plane_id_t id)
{
#ifdef SIMULATOR
    if (id >= PLANE_ID_MAX)
        return;
    g_p1_selected_plane_id = id;
#else
    (void)id;
#endif
}

void ui_base_set_p2_selected_plane_id(plane_id_t id)
{
#ifdef SIMULATOR
    if (id >= PLANE_ID_MAX)
        return;
    g_p2_selected_plane_id = id;
#else
    (void)id;
#endif
}

/**
 * @brief 基地UI界面初始化 第一阶段 初始化根容器等可能与其它模块共享的资源  先调用
 */
void ui_base_init_stage1(void)
{
    dp_base = lv_obj_create(NULL);
    lv_obj_clear_flag(dp_base, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(dp_base, lv_color_hex(0x546373), 0);
}

/**
 * @brief 基地UI界面初始化 第二阶段 渲染lvgl界面
 */
void ui_base_init_stage2(void)
{
    /*
    dp_base = lv_obj_create(NULL);
    lv_obj_clear_flag(dp_base, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(dp_base, lv_color_hex(0x546373), 0);
    */
    // 被注释掉，因为dp_base已经在stage1中创建了

    if (dp_base == NULL)
    {
        CONSOLE_ERROR("dp_base is NULL,cannot render base UI.");
        LOG_ERROR("dp_base is NULL,cannot render base UI.");
        return;
    }

    char bg_path_buf[64];
    lv_obj_t *bg;
#ifdef SIMULATOR
    bg = img_create_from_dsc(dp_base, img_path(BASE_BG_IMG, bg_path_buf, 64), 1024, 600, NULL, &base_bg_dsc, false);
#else
    bg = lv_img_create(dp_base);
    lv_img_set_src(bg, img_path(BASE_BG_IMG, bg_path_buf, 64));
#endif
    lv_obj_center(bg);

    // 1. 右上角返回按钮
    lv_obj_t *exit_btn = lv_btn_create(dp_base);
    lv_obj_set_size(exit_btn, 64, 64);
    lv_obj_set_pos(exit_btn, 0, 0);
    lv_obj_set_align(exit_btn, LV_ALIGN_TOP_RIGHT);
    lv_obj_add_event_cb(exit_btn, base_exit_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_opa(exit_btn, LV_OPA_0, 0);

    lv_obj_t *back_icon;
#ifdef SIMULATOR
    back_icon = img_create_from_dsc(dp_base, img_path(BASE_BACK_ICON, bg_path_buf, 64), 64, 64, NULL, &back_arrow_dsc, true);
    lv_obj_align(back_icon, LV_ALIGN_TOP_RIGHT, 0, 0);
#else
    back_icon = lv_img_create(dp_base);
    lv_img_set_src(back_icon, img_path(BASE_BACK_ICON, bg_path_buf, 64));
    lv_obj_align(back_icon, LV_ALIGN_TOP_RIGHT, 0, 0);
#endif

    lv_obj_t *exit_btn_label = lv_label_create(exit_btn);
    lv_obj_center(exit_btn_label);
    lv_label_set_text(exit_btn_label, "Back");
    lv_obj_set_style_text_font(exit_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    // 左上角多人联机Icon按钮
    lv_obj_t *multi_invite_btn = lv_btn_create(dp_base);
    lv_obj_set_size(multi_invite_btn, 96, 82);
    lv_obj_set_pos(multi_invite_btn, 20, 20);
    lv_obj_set_align(multi_invite_btn, LV_ALIGN_TOP_LEFT);
    lv_obj_add_event_cb(multi_invite_btn, base_invite_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_opa(multi_invite_btn, LV_OPA_0, 0);

    // 联机Icon图片
#ifdef SIMULATOR
    lv_obj_t *multi_invite_img = img_create_from_dsc(dp_base, img_path(MULTI_INVITE_ICON, bg_path_buf, 64), 96, 82, NULL, &multi_icon_dsc, true);
#else
    lv_obj_t *multi_invite_img = lv_img_create(dp_base);
    lv_img_set_src(multi_invite_img, img_path(MULTI_INVITE_ICON, bg_path_buf, 64));
#endif
    lv_obj_set_size(multi_invite_img, 96, 82);
    lv_obj_align(multi_invite_img, LV_ALIGN_TOP_LEFT, 20, 20);

    // 联机弹窗和标签 按钮
    multi_invite_popup = popup_create(lv_layer_top());
    lv_obj_add_flag(multi_invite_popup, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(multi_invite_popup, LV_OPA_COVER, 0);
    lv_obj_set_size(multi_invite_popup, 400, 200);

    multi_invite_popup_label = lv_label_create(multi_invite_popup);
    lv_obj_set_align(multi_invite_popup_label, LV_ALIGN_CENTER);
    lv_obj_set_pos(multi_invite_popup_label, 0, -40);
    lv_obj_set_style_text_font(multi_invite_popup_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(multi_invite_popup_label, lv_color_white(), 0);

    multi_invite_yes_btn = lv_btn_create(multi_invite_popup);
    lv_obj_set_align(multi_invite_yes_btn, LV_ALIGN_CENTER);
    lv_obj_set_pos(multi_invite_yes_btn, -90, 40);
    lv_obj_set_size(multi_invite_yes_btn, 150, 45);
    lv_obj_add_event_cb(multi_invite_yes_btn, base_invite_yes_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *multi_invite_yes_btn_label = lv_label_create(multi_invite_yes_btn);
    lv_obj_center(multi_invite_yes_btn_label);
    lv_label_set_text(multi_invite_yes_btn_label, "Yes");
    lv_obj_set_style_text_font(multi_invite_yes_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    multi_invite_ok_btn = lv_btn_create(multi_invite_popup);
    lv_obj_set_align(multi_invite_ok_btn, LV_ALIGN_CENTER);
    lv_obj_set_pos(multi_invite_ok_btn, -90, 40);
    lv_obj_set_size(multi_invite_ok_btn, 150, 45);
    lv_obj_add_event_cb(multi_invite_ok_btn, base_invite_ok_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *multi_invite_ok_btn_label = lv_label_create(multi_invite_ok_btn);
    lv_obj_center(multi_invite_ok_btn_label);
    lv_label_set_text(multi_invite_ok_btn_label, "OK");
    lv_obj_set_style_text_font(multi_invite_ok_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    multi_invite_no_btn = lv_btn_create(multi_invite_popup);
    lv_obj_set_align(multi_invite_no_btn, LV_ALIGN_CENTER);
    lv_obj_set_pos(multi_invite_no_btn, 90, 40);
    lv_obj_set_size(multi_invite_no_btn, 150, 45);
    lv_obj_add_event_cb(multi_invite_no_btn, base_invite_no_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *multi_invite_no_btn_label = lv_label_create(multi_invite_no_btn);
    lv_obj_center(multi_invite_no_btn_label);
    lv_label_set_text(multi_invite_no_btn_label, "No");
    lv_obj_set_style_text_font(multi_invite_no_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    multi_invite_cancel_btn = lv_btn_create(multi_invite_popup);
    lv_obj_set_align(multi_invite_cancel_btn, LV_ALIGN_CENTER);
    lv_obj_set_pos(multi_invite_cancel_btn, 90, 40);
    lv_obj_set_size(multi_invite_cancel_btn, 150, 45);
    lv_obj_add_event_cb(multi_invite_cancel_btn, base_invite_cancel_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *multi_invite_cancel_btn_label = lv_label_create(multi_invite_cancel_btn);
    lv_obj_center(multi_invite_cancel_btn_label);
    lv_label_set_text(multi_invite_cancel_btn_label, "Cancel");
    lv_obj_set_style_text_font(multi_invite_cancel_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    // 2. 头顶选中态变换标签 "CHOOSED"
    choosed_indicator = lv_label_create(dp_base);
    lv_label_set_text(choosed_indicator, "SELECTED");
    lv_obj_set_style_text_color(choosed_indicator, lv_color_hex(0x00FF00), 0); // 鲜艳绿
    lv_obj_set_style_text_font(choosed_indicator, &lv_font_montserrat_16, 0);

    // 3. 循环创建横向排列的四个飞机组件
    for (int i = 0; i < PLANE_ID_MAX; i++)
    {
        // 飞机触控底座容器
        plane_objs[i] = lv_obj_create(dp_base);
        lv_obj_set_size(plane_objs[i], PLANE_WIDTH, PLANE_HEIGHT);
        int x_pos = START_X + i * (PLANE_WIDTH + PLANE_GAP);
        lv_obj_set_pos(plane_objs[i], x_pos, 160);
        lv_obj_clear_flag(plane_objs[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(plane_objs[i], lv_color_hex(0x1F2326), 0);
        lv_obj_set_style_border_color(plane_objs[i], lv_color_hex(0x3B4246), 0);
        lv_obj_set_style_border_width(plane_objs[i], 2, 0);

        // 飞机图片渲染（使用 APR 外观模板）
        apr_t *plane_apr = apr_get(APR_PLAYER_DEFAULT + i);
        lv_obj_t *plane_img;
#ifdef SIMULATOR
        plane_img = lv_img_create(plane_objs[i]);
        lv_img_set_src(plane_img, &plane_apr->img_dsc);
#else
        plane_img = lv_img_create(plane_objs[i]);
        lv_img_set_src(plane_img, img_path(plane_apr->img_name, plane_path_buf[i], 64));
#endif
        lv_obj_center(plane_img);

        // 飞机名字标签
        lv_obj_t *name_lbl = lv_label_create(plane_objs[i]);
        lv_label_set_text(name_lbl, plane_templates[i].name);
        lv_obj_set_style_text_color(name_lbl, lv_color_white(), 0); // 确保名字也可见
        lv_obj_align(name_lbl, LV_ALIGN_BOTTOM_MID, 0, -5);

        // 半透明锁遮罩幕层
        lock_masks[i] = lv_obj_create(plane_objs[i]);
        lv_obj_set_size(lock_masks[i], LV_PCT(100), LV_PCT(100));
        lv_obj_center(lock_masks[i]);
        lv_obj_clear_flag(lock_masks[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(lock_masks[i], lv_color_black(), 0);
        lv_obj_set_style_bg_opa(lock_masks[i], LV_OPA_60, 0); // 60% 半透明度
        lv_obj_set_style_border_width(lock_masks[i], 0, 0);

        // 锁图标或文本提示
        lv_obj_t *lock_lbl = lv_label_create(lock_masks[i]);
        lv_label_set_text(lock_lbl, "LOCKED");
        lv_obj_set_style_text_color(lock_lbl, lv_color_hex(0x999999), 0);
        lv_obj_center(lock_lbl);

        // 为了让未解锁的飞机也能响应点击事件来查看属性，我们需要关闭锁遮罩层的冒泡或允许点击穿透
        // 这里直接让 lock_masks 不接收点击事件，点击就会穿透到下层的 plane_objs
        lv_obj_clear_flag(lock_masks[i], LV_OBJ_FLAG_CLICKABLE);

        // 绑定飞机点击事件
        lv_obj_add_event_cb(plane_objs[i], plane_click_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }

#ifdef SIMULATOR
    // 3.5 双人摇杆选择框 + 指示标签
    p1_selection_box = lv_obj_create(dp_base);
    lv_obj_set_size(p1_selection_box, BOX_W, BOX_H);
    lv_obj_set_pos(p1_selection_box, START_X - 8, BOX_Y);
    lv_obj_clear_flag(p1_selection_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(p1_selection_box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(p1_selection_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(p1_selection_box, lv_color_hex(0x2080FF), 0);
    lv_obj_set_style_border_width(p1_selection_box, 3, 0);

    p2_selection_box = lv_obj_create(dp_base);
    lv_obj_set_size(p2_selection_box, BOX_W, BOX_H);
    lv_obj_set_pos(p2_selection_box, START_X - 8, BOX_Y);
    lv_obj_clear_flag(p2_selection_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(p2_selection_box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(p2_selection_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(p2_selection_box, lv_color_hex(0xFF2020), 0);
    lv_obj_set_style_border_width(p2_selection_box, 3, 0);
    lv_obj_add_flag(p2_selection_box, LV_OBJ_FLAG_HIDDEN);

    p1_indicator = lv_label_create(dp_base);
    lv_label_set_text(p1_indicator, "1P");
    lv_obj_set_style_text_color(p1_indicator, lv_color_hex(0x2080FF), 0);
    lv_obj_set_style_text_font(p1_indicator, &lv_font_montserrat_22, 0);
    lv_obj_add_flag(p1_indicator, LV_OBJ_FLAG_HIDDEN);

    p2_indicator = lv_label_create(dp_base);
    lv_label_set_text(p2_indicator, "2P");
    lv_obj_set_style_text_color(p2_indicator, lv_color_hex(0xFF2020), 0);
    lv_obj_set_style_text_font(p2_indicator, &lv_font_montserrat_22, 0);
    lv_obj_add_flag(p2_indicator, LV_OBJ_FLAG_HIDDEN);
#endif

    // 4. 创建可跟随移动的飞机详情面板
    detail_panel = lv_obj_create(dp_base);
    lv_obj_set_size(detail_panel, PLANE_WIDTH, 200);
    lv_obj_clear_flag(detail_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(detail_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(detail_panel, 0, 0);
    lv_obj_set_style_pad_all(detail_panel, 0, 0);

#ifdef SIMULATOR
    // p2
    detail_panel_p2 = lv_obj_create(dp_base);
    lv_obj_set_size(detail_panel_p2, PLANE_WIDTH, 200);
    lv_obj_clear_flag(detail_panel_p2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(detail_panel_p2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(detail_panel_p2, 0, 0);
    lv_obj_set_style_pad_all(detail_panel_p2, 0, 0);

    // 默认隐藏
    lv_obj_add_flag(detail_panel_p2, LV_OBJ_FLAG_HIDDEN);

    // 等宽 Choose_p2 按钮
    choose_btn_p2 = lv_btn_create(detail_panel_p2);
    lv_obj_set_size(choose_btn_p2, LV_PCT(100), 40);
    lv_obj_align(choose_btn_p2, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_event_cb(choose_btn_p2, choose_btn_p2_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(choose_btn_p2, lv_color_hex(0xCC0052), 0);

    choose_lbl_p2 = lv_label_create(choose_btn_p2);
    lv_label_set_text(choose_lbl_p2, "CHOOSE");
    lv_obj_center(choose_lbl_p2);

    // 属性详情展示（单列垂直排布布局）
    lv_obj_t *info_cont_p2 = lv_obj_create(detail_panel_p2);
    lv_obj_set_size(info_cont_p2, LV_PCT(100), 150);
    lv_obj_align(info_cont_p2, LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_set_flex_flow(info_cont_p2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_cont_p2, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_color(info_cont_p2, lv_color_hex(0x13171A), 0);
    lv_obj_set_style_border_color(info_cont_p2, lv_color_hex(0x2D3134), 0);
    lv_obj_set_style_pad_all(info_cont_p2, 8, 0);

    hp_label_p2 = lv_label_create(info_cont_p2);
    dmg_label_p2 = lv_label_create(info_cont_p2);
    skill_label_p2 = lv_label_create(info_cont_p2);

    lv_obj_set_style_text_color(hp_label_p2, lv_color_white(), 0);
    lv_obj_set_style_text_color(dmg_label_p2, lv_color_white(), 0);
    lv_obj_set_style_text_color(skill_label_p2, lv_color_white(), 0);

    // 设置文本字体以便阅读
    lv_obj_set_style_text_font(hp_label_p2, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_font(dmg_label_p2, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_font(skill_label_p2, &lv_font_montserrat_14, 0);

#endif

    // 等宽 Choose 按钮
    choose_btn = lv_btn_create(detail_panel);
    lv_obj_set_size(choose_btn, LV_PCT(100), 40);
    lv_obj_align(choose_btn, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_event_cb(choose_btn, choose_btn_cb, LV_EVENT_CLICKED, NULL);

    choose_lbl = lv_label_create(choose_btn);
    lv_label_set_text(choose_lbl, "CHOOSE");
    lv_obj_center(choose_lbl);

    // 属性详情展示（单列垂直排布布局）
    lv_obj_t *info_cont = lv_obj_create(detail_panel);
    lv_obj_set_size(info_cont, LV_PCT(100), 150);
    lv_obj_align(info_cont, LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_set_flex_flow(info_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_color(info_cont, lv_color_hex(0x13171A), 0);
    lv_obj_set_style_border_color(info_cont, lv_color_hex(0x2D3134), 0);
    lv_obj_set_style_pad_all(info_cont, 8, 0);

    hp_label = lv_label_create(info_cont);
    dmg_label = lv_label_create(info_cont);
    skill_label = lv_label_create(info_cont);

    lv_obj_set_style_text_color(hp_label, lv_color_white(), 0);
    lv_obj_set_style_text_color(dmg_label, lv_color_white(), 0);
    lv_obj_set_style_text_color(skill_label, lv_color_white(), 0);

    // 设置文本字体以便阅读
    lv_obj_set_style_text_font(hp_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_font(dmg_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_font(skill_label, &lv_font_montserrat_14, 0);

    // 5. 退出确认弹窗声明
    base_exit_popup = popup_create(dp_base);
    lv_obj_add_flag(base_exit_popup, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *pause_label = lv_label_create(base_exit_popup);
    lv_obj_set_pos(pause_label, 10, 50);
    lv_label_set_text(pause_label, "LEAVE BASE?");
    lv_obj_set_style_text_font(pause_label, &lv_font_montserrat_44, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(pause_label, lv_color_hex(0x13AEFB), LV_STATE_DEFAULT);

    lv_obj_t *continue_btn = lv_btn_create(base_exit_popup);
    lv_obj_set_size(continue_btn, 300, 60);
    lv_obj_set_pos(continue_btn, 25, 240);
    lv_obj_add_event_cb(continue_btn, base_continue_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *continue_lbl = lv_label_create(continue_btn);
    lv_obj_center(continue_lbl);
    lv_label_set_text(continue_lbl, "Stay");
    lv_obj_set_style_text_font(continue_lbl, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    lv_obj_t *back_menu_btn = lv_btn_create(base_exit_popup);
    lv_obj_set_size(back_menu_btn, 300, 60);
    lv_obj_set_pos(back_menu_btn, 25, 320);
    lv_obj_add_event_cb(back_menu_btn, base_back_menu_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_menu_lbl = lv_label_create(back_menu_btn);
    lv_obj_center(back_menu_lbl);
    lv_label_set_text(back_menu_lbl, "Leave");
    lv_obj_set_style_text_font(back_menu_lbl, &lv_font_montserrat_22, LV_STATE_DEFAULT);

#ifdef SIMULATOR
    // 双人选角定时器
    p1_nav_timer.func = p1_move_cb;
    p2_nav_timer.func = p2_move_cb;
    selection_timer = lv_timer_create(selection_timer_cb, 30, NULL);
#endif

    // mp 事件注册
    mp_event_register(MP_EVENT_DISCONNECTED, base_mp_event_cb);
    mp_event_register(MP_EVENT_INVITE_RECEIVED, base_mp_event_cb);
    mp_event_register(MP_EVENT_INVITE_ACCEPTED, base_mp_event_cb);
    mp_event_register(MP_EVENT_INVITE_REJECTED, base_mp_event_cb);
    mp_event_register(MP_EVENT_INVITE_TIMEOUT, base_mp_event_cb);
    mp_event_register(MP_EVENT_CONNECTED, base_mp_event_cb);
    mp_event_register(MP_EVENT_WAITING_TIMEOUT, base_mp_event_cb);
}

/**
 * @brief 核心业务状态机激活运行函数
 */
void ui_base_run(void)
{
    lv_scr_load(dp_base);

    /* 清空上一次屏幕可能残留的 LVGL 输入组，防止按键被拦截 */
    set_group(NULL);

    popup_hide(base_exit_popup);

    // 动态同步最新的解锁状态幕层 (保留遮罩展示，但点击已能穿透)
    for (int i = 0; i < PLANE_ID_MAX; i++)
    {
        if (g_plane_unlocked[i])
        {
            lv_obj_add_flag(lock_masks[i], LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_clear_flag(lock_masks[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    // 默认展示并对齐当前已经选择的飞机面板数据
    current_viewing_idx = g_selected_plane_id;
    update_detail_panel(current_viewing_idx);

    /* 单人模式 SELECTED 标签（双方共用） */
    lv_obj_align_to(choosed_indicator, plane_objs[g_selected_plane_id],
                    LV_ALIGN_OUT_TOP_MID, 0, -8);
    lv_obj_clear_flag(choosed_indicator, LV_OBJ_FLAG_HIDDEN);

#ifdef SIMULATOR
    /* 双人选角初始化（联机时覆盖单人标签） */
    g_p1_current_idx = g_p1_selected_plane_id;
    update_selection_box_pos(p1_selection_box, g_p1_current_idx);
    p1_nav_timer.last_tick = 0;

    if (mp_get_state() == MP_STATE_CONNECTED)
    {
        lv_obj_add_flag(choosed_indicator, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align_to(p1_indicator, plane_objs[g_p1_selected_plane_id],
                        LV_ALIGN_OUT_TOP_MID, -20, -8);
        lv_obj_clear_flag(p1_indicator, LV_OBJ_FLAG_HIDDEN);

        g_p2_current_idx = g_p2_selected_plane_id;
        update_selection_box_pos(p2_selection_box, g_p2_current_idx);
        lv_obj_clear_flag(detail_panel_p2, LV_OBJ_FLAG_HIDDEN);
        update_detail_panel_p2(g_p2_current_idx);
        p2_nav_timer.last_tick = 0;
        lv_obj_clear_flag(p2_selection_box, LV_OBJ_FLAG_HIDDEN);

        lv_obj_align_to(p2_indicator, plane_objs[g_p2_selected_plane_id],
                        LV_ALIGN_OUT_TOP_MID, 20, -8);
        lv_obj_clear_flag(p2_indicator, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(p1_indicator, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(p2_indicator, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(p2_selection_box, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(detail_panel_p2, LV_OBJ_FLAG_HIDDEN);
    }
#endif
}

/**********************
 * STATIC FUNCTIONS
 **********************/

static void plane_click_cb(lv_event_t *e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);

    current_viewing_idx = idx;
#ifdef SIMULATOR
    g_p1_current_idx = idx;
    update_selection_box_pos(p1_selection_box, idx);
#endif
    update_detail_panel(current_viewing_idx);
}

static void update_detail_panel(int idx)
{
    int x_pos = START_X + idx * (PLANE_WIDTH + PLANE_GAP);
    lv_obj_set_pos(detail_panel, x_pos, 330);

    // 刷新基础属性数据
    lv_label_set_text_fmt(hp_label, "HP: %d", plane_templates[idx].max_hp);
    lv_label_set_text_fmt(dmg_label, "DMG: %d", plane_templates[idx].damage);
    lv_label_set_text_fmt(skill_label, "SKILL:\n%s", plane_templates[idx].skill_desc);

    if (!g_plane_unlocked[idx])
    {
        lv_label_set_text(choose_lbl, "LOCKED");
        lv_obj_set_style_bg_color(choose_btn, lv_color_hex(0x555555), 0);
    }
    else
    {
        lv_label_set_text(choose_lbl, "CHOOSE");
        lv_obj_set_style_bg_color(choose_btn, lv_color_hex(0x2196F3), 0); // 恢复原本的按钮高亮色（根据你的主题自行调整颜色值）
    }
}

static void choose_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);

    if (!g_plane_unlocked[current_viewing_idx])
    {
        CONSOLE_INFO("Cannot choose: Plane %s is locked. Spin the shop roulette to unlock!", plane_templates[current_viewing_idx].name);
        return;
    }

    g_selected_plane_id = current_viewing_idx;
    lv_obj_align_to(choosed_indicator, plane_objs[g_selected_plane_id], LV_ALIGN_OUT_TOP_MID, 0, -8);
#ifdef SIMULATOR
    g_p1_selected_plane_id = (plane_id_t)current_viewing_idx;
    g_p1_current_idx = current_viewing_idx;
    lv_obj_align_to(p1_indicator, plane_objs[g_p1_selected_plane_id], LV_ALIGN_OUT_TOP_MID, -20, -8);
    update_selection_box_pos(p1_selection_box, current_viewing_idx);
#endif

    CONSOLE_INFO("Successfully switched to aircraft: %s", plane_templates[g_selected_plane_id].name);
}

static void base_exit_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    // 不能执行退出操作，因为当前正在处理邀请，需要先取消邀请
    if (!lv_obj_has_flag(multi_invite_popup, LV_OBJ_FLAG_HIDDEN))
    {
        CONSOLE_INFO("Cannot exit: Invite is in progress. Please cancel it first.");
        return;
    }
    audio_load(AUDIO_MOUSEOPEN, AUDIO_CHAN_AUTO, false);
    popup_show(base_exit_popup);
}

static void base_continue_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    audio_load(AUDIO_MOUSECLOSE, AUDIO_CHAN_AUTO, false);
    popup_hide(base_exit_popup);
}

static void base_back_menu_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    popup_hide(base_exit_popup);
    audio_load(AUDIO_MOUSECLOSE, AUDIO_CHAN_AUTO, false);
    fsm_switch_state(GS_MENU);
    CONSOLE_INFO("Returning back to main menu.");
}

static void base_invite_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    audio_load(AUDIO_MOUSEOPEN, AUDIO_CHAN_AUTO, false);

    if (!lv_obj_has_flag(multi_invite_popup, LV_OBJ_FLAG_HIDDEN))
        return;

    popup_show(multi_invite_popup);
    lv_obj_add_flag(multi_invite_yes_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(multi_invite_cancel_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(multi_invite_no_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(multi_invite_popup_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);

    mp_state_t mp_state = mp_get_state();
    switch (mp_state)
    {
    case MP_STATE_IDLE:
        if (mp_send_invite())
        {
            lv_label_set_text_fmt(multi_invite_popup_label, "Invite sent,waiting...");
            lv_obj_clear_flag(multi_invite_cancel_btn, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_label_set_text(multi_invite_popup_label, "Invite failed, please check your connection.");
            lv_obj_clear_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    case MP_STATE_CONNECTED:
        lv_label_set_text(multi_invite_popup_label, "Connected, disconnect?");
        lv_obj_clear_flag(multi_invite_yes_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(multi_invite_no_btn, LV_OBJ_FLAG_HIDDEN);
        break;

    default:
        lv_label_set_text(multi_invite_popup_label, "Unexpected state,please report to the developer.");
        lv_obj_clear_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);
        CONSOLE_WARNING("Unexpected state: %d on invite button click.", mp_state);
        LOG_WARNING("Unexpected state: %d on invite button click.", mp_state);
        break;
    }
}

static void base_invite_yes_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    audio_load(AUDIO_MOUSECLOSE, AUDIO_CHAN_AUTO, false);
    switch (mp_get_state())
    {
    case MP_STATE_CONNECTED:
        mp_disconnect();
        break;
    case MP_STATE_WAITING:
        popup_hide(multi_invite_popup);
        mp_accept_invite();
        break;
    default:
        break;
    }
}

static void base_invite_no_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    audio_load(AUDIO_MOUSECLOSE, AUDIO_CHAN_AUTO, false);
    switch (mp_get_state())
    {
    case MP_STATE_CONNECTED:
        popup_hide(multi_invite_popup);
        break;
    case MP_STATE_WAITING:
        popup_hide(multi_invite_popup);
        mp_reject_invite();
        break;
    default:
        break;
    }
}

static void base_invite_cancel_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    audio_load(AUDIO_MOUSECLOSE, AUDIO_CHAN_AUTO, false);
    if (mp_get_state() == MP_STATE_INVITING)
    {
        mp_cancel_invite();
    }
    popup_hide(multi_invite_popup);
}

/**
 * @brief 确认邀请按钮点击事件处理函数
 */
static void base_invite_ok_btn_cb(lv_event_t *e)
{
    // simply hide the popup
    LV_UNUSED(e);
    audio_load(AUDIO_MOUSECLOSE, AUDIO_CHAN_AUTO, false);
    popup_hide(multi_invite_popup);
}

static void base_mp_event_cb(mp_event_t event, void *data)
{
    switch (event)
    {
    case MP_EVENT_DISCONNECTED:
        CONSOLE_INFO("MP_EVENT_DISCONNECTED");
#ifdef SIMULATOR
        lv_obj_add_flag(p2_indicator, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(p2_selection_box, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(detail_panel_p2, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(p1_indicator, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(choosed_indicator, LV_OBJ_FLAG_HIDDEN);
#endif
        // 防止ui 重叠
        lv_obj_add_flag(multi_invite_yes_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_cancel_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_no_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);

        popup_show(multi_invite_popup);

        lv_label_set_text(multi_invite_popup_label, "Disconnected.");
        lv_obj_clear_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);
        break;
    case MP_EVENT_INVITE_RECEIVED:
        popup_show(multi_invite_popup);
        lv_label_set_text(multi_invite_popup_label, "Received invite! Accept?");
        lv_obj_clear_flag(multi_invite_yes_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(multi_invite_no_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_cancel_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);
        CONSOLE_INFO("MP_EVENT_INVITE_RECEIVED");
        break;
    case MP_EVENT_INVITE_ACCEPTED:
        popup_show(multi_invite_popup);
        lv_obj_add_flag(multi_invite_yes_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_cancel_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_no_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);

        lv_label_set_text(multi_invite_popup_label, "Invite accepted.Connected.");
        lv_obj_clear_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);
        CONSOLE_INFO("MP_EVENT_INVITE_ACCEPTED");
        break;
    case MP_EVENT_INVITE_REJECTED:
        popup_show(multi_invite_popup);
        lv_obj_add_flag(multi_invite_yes_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_cancel_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_no_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);

        lv_label_set_text(multi_invite_popup_label, "Invite rejected.");
        lv_obj_clear_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);
        CONSOLE_INFO("MP_EVENT_INVITE_REJECTED");
        break;
    case MP_EVENT_INVITE_TIMEOUT:
        popup_show(multi_invite_popup);
        lv_obj_add_flag(multi_invite_yes_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_cancel_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_no_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);

        lv_label_set_text(multi_invite_popup_label, "Invite timeout.");
        lv_obj_clear_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);
        CONSOLE_INFO("MP_EVENT_INVITE_TIMEOUT");
        break;
    case MP_EVENT_CONNECTED:
        // 连接成功 隐藏"SELECTED" 显示"1P" "2P"
#ifdef SIMULATOR
        /* 双人选角初始化（联机时覆盖单人标签） */
        g_p1_current_idx = g_p1_selected_plane_id;
        update_selection_box_pos(p1_selection_box, g_p1_current_idx);
        p1_nav_timer.last_tick = 0;
        lv_obj_add_flag(choosed_indicator, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align_to(p1_indicator, plane_objs[g_p1_selected_plane_id],
                        LV_ALIGN_OUT_TOP_MID, -20, -8);
        lv_obj_clear_flag(p1_indicator, LV_OBJ_FLAG_HIDDEN);

        g_p2_current_idx = g_p2_selected_plane_id;
        lv_obj_clear_flag(detail_panel_p2, LV_OBJ_FLAG_HIDDEN);
        update_selection_box_pos(p2_selection_box, g_p2_current_idx);
        p2_nav_timer.last_tick = 0;
        lv_obj_clear_flag(p2_selection_box, LV_OBJ_FLAG_HIDDEN);

        lv_obj_align_to(p2_indicator, plane_objs[g_p2_selected_plane_id],
                        LV_ALIGN_OUT_TOP_MID, 20, -8);
        lv_obj_clear_flag(p2_indicator, LV_OBJ_FLAG_HIDDEN);
#endif
        if (!lv_obj_has_flag(multi_invite_popup, LV_OBJ_FLAG_HIDDEN))
        {
            break;
        }
        lv_obj_add_flag(multi_invite_yes_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_cancel_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_no_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);
        popup_show(multi_invite_popup);
        lv_label_set_text(multi_invite_popup_label, "Connected.");
        lv_obj_clear_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);
        break;
    case MP_EVENT_WAITING_TIMEOUT:
        popup_show(multi_invite_popup);
        lv_obj_add_flag(multi_invite_yes_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_cancel_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_no_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);

        lv_label_set_text(multi_invite_popup_label, "Waiting timeout.");
        lv_obj_clear_flag(multi_invite_ok_btn, LV_OBJ_FLAG_HIDDEN);
        CONSOLE_INFO("MP_EVENT_WAITING_TIMEOUT");
        break;
    default:
        break;
    }
}

#ifdef SIMULATOR
/**
 * @brief 双人摇杆选角定时器回调（每 30ms 执行一次）
 */
static void selection_timer_cb(lv_timer_t *timer)
{
    if (fsm_get_state() != GS_BASE)
        return;

    /* --- P1: 本地摇杆导航 --- */
    int16_t joy = joystick_get_x();
    if (joy > NAV_THRESHOLD)
    {
        p1_nav_dir = 0;
        non_blocking_delay(&p1_nav_timer);
    }
    else if (joy < -NAV_THRESHOLD)
    {
        p1_nav_dir = 1;
        non_blocking_delay(&p1_nav_timer);
    }
    else
    {
        p1_nav_timer.last_tick = 0; /* 回中 → 下次即时响应 */
    }

    /* 移动红框并同步浏览 */
    if (current_viewing_idx != g_p1_current_idx)
    {
        current_viewing_idx = g_p1_current_idx;
        update_detail_panel(current_viewing_idx);
    }
    update_selection_box_pos(p1_selection_box, g_p1_current_idx);

    /* P1: KEY_A 确认选择 */
    if (key_down(KEY_A) && g_plane_unlocked[g_p1_current_idx])
    {
        g_p1_selected_plane_id = (plane_id_t)g_p1_current_idx;
        g_selected_plane_id = (plane_id_t)g_p1_current_idx; /* 同步单人选择 */
        if (mp_get_state() == MP_STATE_CONNECTED)
        {
            lv_obj_align_to(p1_indicator, plane_objs[g_p1_current_idx],
                            LV_ALIGN_OUT_TOP_MID, -20, -8);
            lv_obj_clear_flag(p1_indicator, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_align_to(choosed_indicator, plane_objs[g_selected_plane_id], LV_ALIGN_OUT_TOP_MID, 0, -8);
        }
    }

    /* --- P2: 远程摇杆导航（仅联机状态） --- */
    if (mp_get_state() == MP_STATE_CONNECTED)
    {
        lv_obj_clear_flag(p2_selection_box, LV_OBJ_FLAG_HIDDEN);

        int16_t rjoy = rjoystick_get_x();
        if (rjoy > NAV_THRESHOLD)
        {
            p2_nav_dir = 0;
            non_blocking_delay(&p2_nav_timer);
        }
        else if (rjoy < -NAV_THRESHOLD)
        {
            p2_nav_dir = 1;
            non_blocking_delay(&p2_nav_timer);
        }
        else
        {
            p2_nav_timer.last_tick = 0;
        }

        update_selection_box_pos(p2_selection_box, g_p2_current_idx);
        update_detail_panel_p2(g_p2_current_idx);

        /* P2: RKEY_A 确认选择 */
        if (key_down(RKEY_A) && g_plane_unlocked[g_p2_current_idx])
        {
            g_p2_selected_plane_id = (plane_id_t)g_p2_current_idx;
            lv_obj_align_to(p2_indicator, plane_objs[g_p2_current_idx],
                            LV_ALIGN_OUT_TOP_MID, 20, -8);
            lv_obj_clear_flag(p2_indicator, LV_OBJ_FLAG_HIDDEN);
        }

        /* 红蓝框重合 → 红框在上 */
        if (g_p1_current_idx == g_p2_current_idx)
        {
            lv_obj_move_foreground(p1_selection_box);
        }
    }
    else
    {
        lv_obj_add_flag(p2_selection_box, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(p2_indicator, LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * @brief P1 导航移动回调（由 non_blocking_delay 触发）
 */
static void p1_move_cb(void)
{
    if (p1_nav_dir == 0 && g_p1_current_idx < PLANE_ID_MAX - 1)
    {
        g_p1_current_idx++;
    }
    else if (p1_nav_dir == 1 && g_p1_current_idx > 0)
    {
        g_p1_current_idx--;
    }
}

/**
 * @brief P2 导航移动回调（由 non_blocking_delay 触发）
 */
static void p2_move_cb(void)
{
    if (p2_nav_dir == 0 && g_p2_current_idx < PLANE_ID_MAX - 1)
    {
        g_p2_current_idx++;
    }
    else if (p2_nav_dir == 1 && g_p2_current_idx > 0)
    {
        g_p2_current_idx--;
    }
}

/**
 * @brief 将选择框移动到指定飞机的位置
 */
static void update_selection_box_pos(lv_obj_t *box, int idx)
{
    int box_x = START_X + idx * (PLANE_WIDTH + PLANE_GAP) - 8;
    lv_obj_set_pos(box, box_x, BOX_Y);
}

/**
 * @brief 更新 P2 属性面板位置
 */
static void update_detail_panel_p2(int idx)
{
    int x_pos = START_X + idx * (PLANE_WIDTH + PLANE_GAP);
    lv_obj_set_pos(detail_panel_p2, x_pos, 330);

    // 刷新基础属性数据
    lv_label_set_text_fmt(hp_label_p2, "HP: %d", plane_templates[idx].max_hp);
    lv_label_set_text_fmt(dmg_label_p2, "DMG: %d", plane_templates[idx].damage);
    lv_label_set_text_fmt(skill_label_p2, "SKILL:\n%s", plane_templates[idx].skill_desc);

    if (!g_plane_unlocked[idx])
    {
        lv_label_set_text(choose_lbl_p2, "LOCKED");
        lv_obj_set_style_bg_color(choose_btn_p2, lv_color_hex(0x555555), 0);
    }
    else
    {
        lv_label_set_text(choose_lbl_p2, "CHOOSE");
        lv_obj_set_style_bg_color(choose_btn_p2, lv_color_hex(0xCC0052), 0); // 恢复原本的按钮高亮色（根据你的主题自行调整颜色值）
    }
}

static void choose_btn_p2_cb(lv_event_t *e)
{
    LV_UNUSED(e);

    if (!g_plane_unlocked[current_viewing_idx])
    {
        CONSOLE_INFO("Cannot choose: Plane %s is locked. Spin the shop roulette to unlock!", plane_templates[current_viewing_idx].name);
        return;
    }

    g_p2_selected_plane_id = g_p2_current_idx;
    lv_obj_align_to(p2_indicator, plane_objs[g_p2_selected_plane_id], LV_ALIGN_OUT_TOP_MID, 20, -8);
    update_selection_box_pos(p2_selection_box, g_p2_current_idx);

    CONSOLE_INFO("Successfully switched to aircraft: %s", plane_templates[g_p2_selected_plane_id].name);
}

#endif
