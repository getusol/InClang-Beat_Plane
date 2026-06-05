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
#include "save.h"
/*********************
 * MACROS
 *********************/
#define BASE_BG_IMG     "base_bg.bin"
#define BASE_BACK_ICON "back_arrow.bin"

#define PLANE_WIDTH     160
#define PLANE_HEIGHT    160
#define PLANE_GAP       60
#define START_X         102  // (1024 - (160*4 + 60*3)) / 2 保持完美居中

/**********************
 * TYPEDEFS
 **********************/
typedef struct {
    int id;
    const char* name;
    const char* img_src;
    int max_hp;
    int damage;
    const char* skill_desc;
} plane_info_t;

/**********************
 * STATIC VARIABLES
 **********************/
static bool g_plane_unlocked[PLANE_ID_MAX] = {true, false, false, false};
static plane_id_t g_selected_plane_id = PLANE_ID_DEFAULT;

static lv_obj_t * dp_base = NULL;

static lv_obj_t * plane_objs[PLANE_ID_MAX] = {NULL};
static lv_obj_t * lock_masks[PLANE_ID_MAX] = {NULL};

static lv_obj_t * choosed_indicator = NULL; // 头顶的 "CHOOSED" 变换标签
static lv_obj_t * detail_panel = NULL;      // 动态移动的属性面板
static lv_obj_t * choose_btn = NULL;        // 选择按钮
static lv_obj_t * choose_lbl = NULL;        // 选择按钮文本
static lv_obj_t * hp_label = NULL;
static lv_obj_t * dmg_label = NULL;
static lv_obj_t * skill_label = NULL;

static lv_obj_t * base_exit_popup = NULL;   // 退出确认弹窗
static int current_viewing_idx = 0;         // 当前正在点击浏览的飞机索引

// 飞机固有属性配置表
static const plane_info_t plane_templates[PLANE_ID_MAX] = {
    {0, "Player",  "player.bin",          200, 34, "Burst: 3-way shot"},
    {1, "Ember",   "player_ember.bin",    200, 20, "Flame Circle"},
    {2, "Stream",  "player_stream.bin",   200, 10, "Shield"},
    {3, "Verdant", "player_verdant.bin",  250, 15, "HP Reclaim"}
};

// 静态化的长周期路径缓冲区，防止 LVGL 异步渲染时读取到脏数据导致乱码
static char plane_path_buf[PLANE_ID_MAX][64];

#ifdef SIMULATOR
static lv_img_dsc_t base_bg_dsc;
static lv_img_dsc_t plane_base_dsc;
static lv_img_dsc_t plane_dscs[PLANE_ID_MAX];
static lv_img_dsc_t back_arrow_dsc;
#endif

/**********************
 * STATIC PROTOTYPES
 **********************/
static void plane_click_cb(lv_event_t * e);
static void choose_btn_cb(lv_event_t * e);
static void base_exit_btn_cb(lv_event_t * e);
static void base_continue_btn_cb(lv_event_t * e);
static void base_back_menu_btn_cb(lv_event_t * e);
static void update_detail_panel(int idx);

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
    if (plane_id >= PLANE_ID_MAX) return false;
    return g_plane_unlocked[plane_id];
}

/**
 * @brief 解锁指定飞机（从商店抽奖获得后调用）
 * @param plane_id 飞机 ID（PLANE_ID_DEFAULT /_EMBER / STREAM / VERDANT）
 */
void ui_base_plane_unlock(plane_id_t plane_id)
{
    if (plane_id >= PLANE_ID_MAX) return;
    g_plane_unlocked[plane_id] = true;
    CONSOLE_INFO("Plane %d unlocked.", plane_id);
}

void ui_base_set_selected_plane_id(plane_id_t id)
{
    if (id >= PLANE_ID_MAX) return;
    g_selected_plane_id = id;
}

int ui_base_get_unlocked_mask(void)
{
    int mask = 0;
    for (int i = 0; i < PLANE_ID_MAX; i++) {
        if (g_plane_unlocked[i]) mask |= (1 << i);
    }
    return mask;
}

void ui_base_set_unlocked_mask(int mask)
{
    for (int i = 0; i < PLANE_ID_MAX; i++) {
        g_plane_unlocked[i] = (mask & (1 << i)) != 0;
    }
    // 确保 Default 飞机始终解锁
    g_plane_unlocked[PLANE_ID_DEFAULT] = true;
}

/**
 * @brief 基地UI界面初始化
 */
void ui_base_init(void)
{
    dp_base = lv_obj_create(NULL);
    lv_obj_clear_flag(dp_base, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(dp_base, lv_color_hex(0x546373), 0);

    char bg_path_buf[64];
    lv_obj_t * bg;
#ifdef SIMULATOR
    bg = img_create_from_dsc(dp_base, img_path(BASE_BG_IMG, bg_path_buf, 64), 1024, 600, NULL, &base_bg_dsc, false);
#else  
    bg = lv_img_create(dp_base);
    lv_img_set_src(bg, img_path(BASE_BG_IMG, bg_path_buf, 64));
#endif
    lv_obj_center(bg);

   

    // 1. 右上角返回按钮
    lv_obj_t * exit_btn = lv_btn_create(dp_base);
    lv_obj_set_size(exit_btn, 64, 64);
    lv_obj_set_pos(exit_btn, 0, 0);
    lv_obj_set_align(exit_btn, LV_ALIGN_TOP_RIGHT);
    lv_obj_add_event_cb(exit_btn, base_exit_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_opa(exit_btn, LV_OPA_0, 0);
    
    lv_obj_t * back_icon;
#ifdef SIMULATOR
    back_icon = img_create_from_dsc(dp_base, img_path(BASE_BACK_ICON, bg_path_buf, 64), 64, 64, NULL, &back_arrow_dsc, true);
    lv_obj_align(back_icon, LV_ALIGN_TOP_RIGHT, 0, 0);
#else  
    back_icon = lv_img_create(dp_base);
    lv_img_set_src(back_icon, img_path(BASE_BACK_ICON, bg_path_buf, 64));
    lv_obj_align(back_icon, LV_ALIGN_TOP_RIGHT, 0, 0);
#endif
    
    lv_obj_t * exit_btn_label = lv_label_create(exit_btn);
    lv_obj_center(exit_btn_label);
    lv_label_set_text(exit_btn_label, "Back");
    lv_obj_set_style_text_font(exit_btn_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    // 2. 头顶选中态变换标签 "CHOOSED"
    choosed_indicator = lv_label_create(dp_base);
    lv_label_set_text(choosed_indicator, "SELECTED");
    lv_obj_set_style_text_color(choosed_indicator, lv_color_hex(0x00FF00), 0); // 鲜艳绿
    lv_obj_set_style_text_font(choosed_indicator, &lv_font_montserrat_16, 0);

    // 3. 循环创建横向排列的四个飞机组件
    for (int i = 0; i < PLANE_ID_MAX; i++) {
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
        lv_obj_t * plane_img;
#ifdef SIMULATOR
        plane_img = lv_img_create(plane_objs[i]);
        lv_img_set_src(plane_img, &plane_apr->img_dsc);
#else
        plane_img = lv_img_create(plane_objs[i]);
        lv_img_set_src(plane_img, img_path(plane_apr->img_name, plane_path_buf[i], 64));
#endif
        lv_obj_center(plane_img);
    

        // 飞机名字标签
        lv_obj_t * name_lbl = lv_label_create(plane_objs[i]);
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
        lv_obj_t * lock_lbl = lv_label_create(lock_masks[i]);
        lv_label_set_text(lock_lbl, "LOCKED");
        lv_obj_set_style_text_color(lock_lbl, lv_color_hex(0x999999), 0);
        lv_obj_center(lock_lbl);

        // 💡 注意：为了让未解锁的飞机也能响应点击事件来查看属性，我们需要关闭锁遮罩层的冒泡或允许点击穿透
        // 这里直接让 lock_masks 不接收点击事件，点击就会穿透到下层的 plane_objs
        lv_obj_clear_flag(lock_masks[i], LV_OBJ_FLAG_CLICKABLE);

        // 绑定飞机点击事件
        lv_obj_add_event_cb(plane_objs[i], plane_click_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
    }

    // 4. 创建可跟随移动的飞机详情面板
    detail_panel = lv_obj_create(dp_base);
    lv_obj_set_size(detail_panel, PLANE_WIDTH, 200);
    lv_obj_clear_flag(detail_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(detail_panel, LV_OPA_TRANSP, 0); 
    lv_obj_set_style_border_width(detail_panel, 0, 0);
    lv_obj_set_style_pad_all(detail_panel, 0, 0);

    // 等宽 Choose 按钮
    choose_btn = lv_btn_create(detail_panel);
    lv_obj_set_size(choose_btn, LV_PCT(100), 40);
    lv_obj_align(choose_btn, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_event_cb(choose_btn, choose_btn_cb, LV_EVENT_CLICKED, NULL);

    choose_lbl = lv_label_create(choose_btn);
    lv_label_set_text(choose_lbl, "CHOOSE");
    lv_obj_center(choose_lbl);

    // 属性详情展示（单列垂直排布布局）
    lv_obj_t * info_cont = lv_obj_create(detail_panel);
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
    
    // 💡 修正：显式将全黑背景下的文字颜色全部指定为白色，使其清晰可见
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

    lv_obj_t * pause_label = lv_label_create(base_exit_popup);
    lv_obj_set_pos(pause_label, 10, 50);
    lv_label_set_text(pause_label, "LEAVE BASE?");
    lv_obj_set_style_text_font(pause_label, &lv_font_montserrat_44, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(pause_label, lv_color_hex(0x13AEFB), LV_STATE_DEFAULT);

    lv_obj_t * continue_btn = lv_btn_create(base_exit_popup);
    lv_obj_set_size(continue_btn, 300, 60);
    lv_obj_set_pos(continue_btn, 25, 240);
    lv_obj_add_event_cb(continue_btn, base_continue_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * continue_lbl = lv_label_create(continue_btn);
    lv_obj_center(continue_lbl);
    lv_label_set_text(continue_lbl, "Stay");
    lv_obj_set_style_text_font(continue_lbl, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    lv_obj_t * back_menu_btn = lv_btn_create(base_exit_popup);
    lv_obj_set_size(back_menu_btn, 300, 60);
    lv_obj_set_pos(back_menu_btn, 25, 320);
    lv_obj_add_event_cb(back_menu_btn, base_back_menu_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * back_menu_lbl = lv_label_create(back_menu_btn);
    lv_obj_center(back_menu_lbl);
    lv_label_set_text(back_menu_lbl, "Leave");
    lv_obj_set_style_text_font(back_menu_lbl, &lv_font_montserrat_22, LV_STATE_DEFAULT);
}

/**
 * @brief 核心业务状态机激活运行函数
 */
void ui_base_run(void)
{
    if (dp_base == NULL) {
        ui_base_init();
    }
    lv_scr_load(dp_base);
    
    popup_hide(base_exit_popup);

    // 动态同步最新的解锁状态幕层 (保留遮罩展示，但点击已能穿透)
    for (int i = 0; i < PLANE_ID_MAX; i++) {
        if (g_plane_unlocked[i]) {
            lv_obj_add_flag(lock_masks[i], LV_OBJ_FLAG_HIDDEN); 
        } else {
            lv_obj_clear_flag(lock_masks[i], LV_OBJ_FLAG_HIDDEN); 
        }
    }

    // 默认展示并对齐当前已经选择的飞机面板数据
    current_viewing_idx = g_selected_plane_id;
    update_detail_panel(current_viewing_idx);
    
    // 对齐 SELECTED 头顶变换标识
    lv_obj_align_to(choosed_indicator, plane_objs[g_selected_plane_id], LV_ALIGN_OUT_TOP_MID, 0, -8);

}

/**********************
 * STATIC FUNCTIONS
 **********************/

static void plane_click_cb(lv_event_t * e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    
    // 💡 修改：删除了 "!g_plane_unlocked[idx]" 的拦截返回。
    // 无论飞机是否解锁，现在都会更新当前浏览的 ID 并刷新面板。
    current_viewing_idx = idx;
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

    // 💡 视觉优化：如果当前看的是未解锁的飞机，让 Choose 按钮显示为 "LOCKED" 并变灰
    if (!g_plane_unlocked[idx]) {
        lv_label_set_text(choose_lbl, "LOCKED");
        lv_obj_set_style_bg_color(choose_btn, lv_color_hex(0x555555), 0);
    } else {
        lv_label_set_text(choose_lbl, "CHOOSE");
        lv_obj_set_style_bg_color(choose_btn, lv_color_hex(0x2196F3), 0); // 恢复原本的按钮高亮色（根据你的主题自行调整颜色值）
    }
}

static void choose_btn_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    // 💡 修改：在这里进行解锁校验，未解锁则拒绝选择
    if (!g_plane_unlocked[current_viewing_idx]) {
        CONSOLE_INFO("Cannot choose: Plane %s is locked. Spin the shop roulette to unlock!", plane_templates[current_viewing_idx].name);
        return;
    }

    g_selected_plane_id = current_viewing_idx;
    lv_obj_align_to(choosed_indicator, plane_objs[g_selected_plane_id], LV_ALIGN_OUT_TOP_MID, 0, -8);

    CONSOLE_INFO("Successfully switched to aircraft: %s", plane_templates[g_selected_plane_id].name);
}

static void base_exit_btn_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    popup_show(base_exit_popup);
}

static void base_continue_btn_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    popup_hide(base_exit_popup);
}

static void base_back_menu_btn_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    popup_hide(base_exit_popup);
    save_write();
    fsm_switch_state(GS_MENU);
    CONSOLE_INFO("Returning back to main menu.");
}
