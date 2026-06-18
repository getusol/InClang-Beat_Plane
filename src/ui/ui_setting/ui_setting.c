/**
 * @file ui_setting.c
 */

/*********************
 * INCLUDES
 *********************/

#include "ui_setting.h"
#include "lvgl.h"
#include "settings.h"
#include "fsm.h"
#include "config.h"
#include "tools.h"
#include "lvgl_utils.h"
#include "audio.h"
#include "user_data.h"

/**********************
 * MACROS
 **********************/

#define BACK_ARROW_IMG_NAME "back_arrow.bin"

/**********************
 *      TYPEDEFS
 **********************/

typedef struct
{
    lv_obj_t *label;
    lv_obj_t *ctrl; /* 开关或滑块 */
    const setting_t *setting;
} setting_ui_t;

/**********************
 * STATIC PROTOTYPES
 **********************/

static void fetch_tab_names(void);
static void create_tab_content(lv_obj_t *tab, const char *module);
static void refresh_all_ui(void);

// callbacks
static void back_btn_event_cb(lv_event_t *e);
static void setting_ui_event_cb(lv_event_t *e);
static void setting_ui_label_event_cb(lv_event_t *e);
static void danger_reset_settings_cb(lv_event_t *e);
static void danger_clear_userdata_cb(lv_event_t *e);

/**********************
 * STATIC VARIABLES
 **********************/

static const char *tab_names[SETTINGS_MAX] = {NULL};
static uint8_t tab_names_count = 0;

// UI 控件引用 (用于刷新)
static setting_ui_t *s_ui_controls[SETTINGS_MAX];
static int s_ui_count = 0;

// lvgl
static lv_obj_t *dp_setting = NULL;

#ifdef SIMULATOR
static lv_img_dsc_t back_arrow_img_struct;
#endif

// 记录进入设置前所在的状态，用于返回
static game_state_t setting_prev_state = GS_MENU;

/**********************
 * GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化设置界面 根容器等可能与其它模块共享的资源
 */
void ui_setting_init_stage1(void)
{
    // 创建设置根屏幕
    dp_setting = lv_obj_create(NULL);
    lv_obj_clear_flag(dp_setting, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(dp_setting, lv_color_hex(DP_PLAY_FILL_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dp_setting, LV_OPA_COVER, LV_PART_MAIN);
}

/**
 * @brief 初始化设置界面 全部元素
 */
void ui_setting_init_stage2(void)
{

    // dp_setting 已初始化检查
    if (dp_setting == NULL)
    {
        CONSOLE_ERROR("dp_setting is NULL,cannot init setting interface.");
        LOG_ERROR("dp_setting is NULL,cannot init setting interface.");
        return;
    }

    // 标题
    lv_obj_t *title = lv_label_create(dp_setting);
    lv_label_set_text(title, "SETTINGS");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_44, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    // 右上角返回按钮 (64x64 透明 + 箭头图标)
    lv_obj_t *back_btn = lv_btn_create(dp_setting);
    lv_obj_set_size(back_btn, 64, 64);
    lv_obj_align(back_btn, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(back_btn, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(back_btn, 0, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);

    char img_path_buf[64];
#ifdef SIMULATOR
    lv_obj_t *back_img = img_create_from_dsc(back_btn,
                                             img_path(BACK_ARROW_IMG_NAME, img_path_buf, sizeof(img_path_buf)),
                                             64, 64, NULL, &back_arrow_img_struct, true);
#else
    lv_obj_t *back_img = lv_img_create(back_btn);
    lv_img_set_src(back_img, img_path(BACK_ARROW_IMG_NAME, img_path_buf, sizeof(img_path_buf)));
#endif
    lv_obj_center(back_img);

    // TabView
    lv_obj_t *tabview = lv_tabview_create(dp_setting, LV_DIR_TOP, 60);
    lv_obj_align(tabview, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_size(tabview, 800, 450);
    fetch_tab_names(); // now tab_names contains all unique module names(not sorted)
    // create tabs and tab content
    for (int i = 0; i < tab_names_count; i++)
    {
        lv_obj_t *tab = lv_tabview_add_tab(tabview, tab_names[i]);
        lv_obj_add_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(tab, 4, LV_STATE_DEFAULT);
        create_tab_content(tab, tab_names[i]);
    }

    // 特殊选项卡 — Danger Zone
    {
        lv_obj_t *tab_danger = lv_tabview_add_tab(tabview, "Danger Zone");
        lv_obj_add_flag(tab_danger, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(tab_danger, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(tab_danger, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(tab_danger, 10, LV_STATE_DEFAULT);

        // Row 1: Reset all settings
        {
            lv_obj_t *row = lv_obj_create(tab_danger);
            lv_obj_set_size(row, LV_PCT(90), 50);
            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(row, 0, 0);
            lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *lbl = lv_label_create(row);
            lv_label_set_text(lbl, "Reset all settings");
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_22, 0);

            lv_obj_t *btn = lv_btn_create(row);
            lv_obj_set_size(btn, 120, 40);
            lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_RED), 0);
            lv_obj_add_event_cb(btn, danger_reset_settings_cb, LV_EVENT_CLICKED, NULL);
            lv_obj_t *bl = lv_label_create(btn);
            lv_label_set_text(bl, "RESET");
            lv_obj_center(bl);
        }

        // Row 2: Clear user data
        {
            lv_obj_t *row = lv_obj_create(tab_danger);
            lv_obj_set_size(row, LV_PCT(90), 50);
            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(row, 0, 0);
            lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *lbl = lv_label_create(row);
            lv_label_set_text(lbl, "Clear user datas");
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_22, 0);

            lv_obj_t *btn = lv_btn_create(row);
            lv_obj_set_size(btn, 120, 40);
            lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_RED), 0);
            lv_obj_add_event_cb(btn, danger_clear_userdata_cb, LV_EVENT_CLICKED, NULL);
            lv_obj_t *bl = lv_label_create(btn);
            lv_label_set_text(bl, "CLEAR");
            lv_obj_center(bl);
        }
    }

    // tabview样式
    lv_obj_t *tab_cont = lv_tabview_get_content(tabview);
    lv_obj_set_style_bg_opa(tab_cont, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tab_cont, lv_color_hex(0x808080), LV_PART_MAIN);
    lv_obj_set_style_text_font(lv_tabview_get_tab_btns(tabview), &lv_font_montserrat_22, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lv_tabview_get_tab_btns(tabview), lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(lv_tabview_get_tab_btns(tabview), lv_color_hex(0x100F0F), LV_PART_MAIN);

    CONSOLE_INFO("Settings screen initialized.");
}

void ui_setting_run(void)
{
    if (dp_setting == NULL)
        return;
    lv_scr_load(dp_setting);
    set_group(NULL);
}

void ui_setting_set_prev_state(game_state_t s)
{
    setting_prev_state = s;
}

game_state_t ui_setting_get_prev_state(void)
{
    return setting_prev_state;
}

/**********************
 * STATIC FUNCTIONS
 **********************/

/**
 * @brief 从settings module中获取所有选项卡名称
 */
static void fetch_tab_names(void)
{
    const setting_t *setting = NULL;
    bool found = false;
    for (int i = 0; i < settings_get_setting_count(); i++)
    {
        found = false;
        setting = settings_get_setting(i);
        if (setting == NULL)
            continue;
        for (int j = 0; j < tab_names_count; j++)
        {
            if (strcmp(setting->module, tab_names[j]) == 0)
            {
                found = true;
                break;
            }
        }
        if (found)
            continue;
        tab_names[tab_names_count++] = setting->module;
    }
}

/**
 * @brief 创建选项卡内容
 */
static void create_tab_content(lv_obj_t *tab, const char *module)
{
    const setting_t *setting = NULL;
    for (int i = 0; i < settings_get_setting_count(); i++)
    {
        setting = settings_get_setting(i);
        if (setting == NULL || strcmp(setting->module, module) != 0)
            continue;

        // 创建工具结构体 malloc 不释放(!)(确保不会多次进入)
        setting_ui_t *setting_ui = ram_malloc(sizeof(setting_ui_t));
        setting_ui->setting = NULL;
        setting_ui->label = NULL;

        switch (setting->type)
        {
        case ST_BOOL:
        {
            // 创建布尔选项
            // 水平容器 左侧标签右侧开关
            lv_obj_t *cont_bool = lv_obj_create(tab);
            lv_obj_clear_flag(cont_bool, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_flex_flow(cont_bool, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(cont_bool, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_size(cont_bool, 800 - 40, 80);
            lv_obj_set_style_pad_all(cont_bool, 20, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_column(cont_bool, 40, LV_STATE_DEFAULT);

            lv_obj_t *label_bool = lv_label_create(cont_bool);
            lv_label_set_text_fmt(label_bool, "%s: %s", setting->name, *(bool *)setting->data ? "ON" : "OFF");
            lv_obj_set_style_text_font(label_bool, &lv_font_montserrat_22, LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(label_bool, LV_TEXT_ALIGN_RIGHT, LV_STATE_DEFAULT);
            lv_obj_set_width(label_bool, 250); // 防止宽度变化导致抖动

            lv_obj_t *sswitch_bool = lv_switch_create(cont_bool);
            if (*(bool *)setting->data)
                lv_obj_add_state(sswitch_bool, LV_STATE_CHECKED);
            else
                lv_obj_clear_state(sswitch_bool, LV_STATE_CHECKED);
            lv_obj_set_size(sswitch_bool, 70, 35);

            setting_ui->setting = setting;
            setting_ui->label = label_bool;
            setting_ui->ctrl = sswitch_bool;

            lv_obj_add_event_cb(sswitch_bool, setting_ui_event_cb, LV_EVENT_VALUE_CHANGED, (void *)setting);
            lv_obj_add_event_cb(sswitch_bool, setting_ui_label_event_cb, LV_EVENT_VALUE_CHANGED, (void *)setting_ui);

            break;
        }
        case ST_INT:
        {
            // 创建整数选项
            // 水平容器 左侧标签右侧滑块
            lv_obj_t *cont_int = lv_obj_create(tab);
            lv_obj_clear_flag(cont_int, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_flex_flow(cont_int, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(cont_int, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_size(cont_int, 800 - 40, 80);
            lv_obj_set_style_pad_all(cont_int, 20, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_column(cont_int, 40, LV_STATE_DEFAULT);

            lv_obj_t *label_int = lv_label_create(cont_int);
            lv_label_set_text_fmt(label_int, "%s: %d", setting->name, *(int *)setting->data);
            lv_obj_set_style_text_font(label_int, &lv_font_montserrat_22, LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(label_int, LV_TEXT_ALIGN_RIGHT, LV_STATE_DEFAULT);
            lv_obj_set_width(label_int, 250); // 防止宽度变化导致抖动

            lv_obj_t *slider_int = lv_slider_create(cont_int);
            lv_slider_set_range(slider_int, setting->int_data.min, setting->int_data.max);
            lv_slider_set_value(slider_int, *(int *)setting->data, LV_ANIM_OFF);
            lv_obj_set_size(slider_int, 300, 20);

            setting_ui->setting = setting;
            setting_ui->label = label_int;
            setting_ui->ctrl = slider_int;

            lv_obj_add_event_cb(slider_int, setting_ui_event_cb, LV_EVENT_VALUE_CHANGED, (void *)setting);
            lv_obj_add_event_cb(slider_int, setting_ui_label_event_cb, LV_EVENT_VALUE_CHANGED, (void *)setting_ui);

            break;
        }
        default:
        {
            CONSOLE_WARNING("Unknown setting type: %d,cannot create option.", setting->type);
            LOG_WARNING("Unknown setting type: %d,cannot create option.", setting->type);
            break;
        }
        }
        if (setting_ui->setting && s_ui_count < SETTINGS_MAX)
            s_ui_controls[s_ui_count++] = setting_ui;
    }
}

/**
 * @brief 右上角返回按钮点击事件处理函数
 */
static void back_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    settings_save();
    audio_load(AUDIO_MOUSECLOSE, AUDIO_CHAN_SFX1, false);
    fsm_switch_state(setting_prev_state);
}

/**
 * @brief 全部settings控件事件处理函数
 * @param e->target 控件对象
 * @param e->user_data 配置项指针
 */
static void setting_ui_event_cb(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    const setting_t *setting = (const setting_t *)lv_event_get_user_data(e);
    if (target == NULL || setting == NULL)
        return;

    // 更新配置项值
    switch (setting->type)
    {
    case ST_BOOL:
    {
        bool state = lv_obj_has_state(target, LV_STATE_CHECKED);
        settings_set(setting->module, setting->name, state);
        break;
    }
    case ST_INT:
    {
        int value = lv_slider_get_value(target);
        settings_set(setting->module, setting->name, value);
        break;
    }
    default:
        break;
    }

    // 根据不同配置项 使用不同效果 Based on name
    if (strcmp(setting->name, "BGM Volume") == 0)
        audio_load(AUDIO_TROPICAL, AUDIO_CHAN_BGM, false);
    else if (strcmp(setting->name, "SFX Volume") == 0)
        audio_load(AUDIO_BOSSATTACK, AUDIO_CHAN_SFX1, false);
    else if (strcmp(setting->name, "Amplifier") == 0)
        audio_load(AUDIO_FAH, AUDIO_CHAN_SFX1, false);
    else if (strcmp(setting->name, "Mute") == 0)
        audio_stop_all();
}

/**
 * @brief 更新标签文本
 * @param e->target 控件对象
 * @param e->user_data setting_ui_t * 指向工具结构体的指针, 包含标签对象与配置项指针
 */
static void setting_ui_label_event_cb(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    setting_ui_t *ui = (setting_ui_t *)lv_event_get_user_data(e);
    if (target == NULL || ui == NULL)
        return;
    lv_obj_t *label = ui->label;
    const setting_t *setting = ui->setting;
    if (label == NULL || setting == NULL)
        return;
    if (lv_obj_get_class(target) == &lv_slider_class)
    {
        int value = 0;
        value = lv_slider_get_value(target);
        lv_label_set_text_fmt(label, "%s: %d", setting->name, value);
    }
    else if (lv_obj_get_class(target) == &lv_switch_class)
    {
        bool state = false;
        state = lv_obj_has_state(target, LV_STATE_CHECKED);
        lv_label_set_text_fmt(label, "%s: %s", setting->name, state ? "ON" : "OFF");
    }
    else
    {
        CONSOLE_WARNING("Unknown setting type: %p,cannot update label.", lv_obj_get_class(target));
        LOG_WARNING("Unknown setting type: %p,cannot update label.", lv_obj_get_class(target));
    }
}

static void refresh_all_ui(void)
{
    for (int i = 0; i < s_ui_count; i++)
    {
        setting_ui_t *ui = s_ui_controls[i];
        if (!ui || !ui->setting)
            continue;
        switch (ui->setting->type)
        {
        case ST_BOOL:
            if (*(bool *)ui->setting->data)
                lv_obj_add_state(ui->ctrl, LV_STATE_CHECKED);
            else
                lv_obj_clear_state(ui->ctrl, LV_STATE_CHECKED);
            lv_label_set_text_fmt(ui->label, "%s: %s",
                                  ui->setting->name, *(bool *)ui->setting->data ? "ON" : "OFF");
            break;
        case ST_INT:
            lv_slider_set_value(ui->ctrl, *(int *)ui->setting->data, LV_ANIM_OFF);
            lv_label_set_text_fmt(ui->label, "%s: %d",
                                  ui->setting->name, *(int *)ui->setting->data);
            break;
        }
    }
}

static void danger_reset_settings_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    settings_reset();
    settings_load();
    refresh_all_ui();
    CONSOLE_INFO("Settings reset and UI refreshed.");
}

static void danger_clear_userdata_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    user_data_reset();
    user_data_load();
    CONSOLE_INFO("User data cleared.");
}
