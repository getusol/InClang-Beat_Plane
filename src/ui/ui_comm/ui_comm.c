/**
 * @file ui_comm.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "ui_comm.h"
#include "lvgl.h"
#include "comm_status.h"
#include "comm.h"
#include "debug.h"
#include "multiplayer.h"
#include "fsm.h"
#include "save.h"
#include "lvgl_utils.h"
#include "ui_templates.h"

/**********************
 *      MACROS
 **********************/

#define MULTI_ICON "2DMultiIcon.bin"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void comm_label_update(lv_timer_t * t);
static void on_label_click(lv_event_t * e);
static void on_mp_event_cb(mp_event_t event,void * v);
static void on_mp_btm_click(lv_event_t * e);
static void on_mp_popup_yes_btn_click(lv_event_t * e);
static void on_mp_popup_no_btn_click(lv_event_t * e);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static lv_obj_t *comm_status_label = NULL;
static lv_obj_t * dp_comm = NULL;
static lv_img_dsc_t multi_icon_dsc = {0};
static lv_obj_t * mp_popup = NULL;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 目前只是一个小标签 后面可以拓展到单片机单独界面（即单片机从机禁用游戏）
 */
void ui_comm_init(void)
{
    // 在 overlay 层创建标签（始终显示在所有屏幕上方）
    lv_obj_t *overlay = lv_layer_top();
    comm_status_label = lv_label_create(overlay);

    lv_label_set_text(comm_status_label, "Comm: Disconnected");
    lv_obj_set_style_text_font(comm_status_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(comm_status_label,LV_OPA_COVER,0);
    lv_obj_set_style_text_color(comm_status_label,lv_color_hex(0xE8E8E8),0);
    lv_obj_set_style_bg_color(comm_status_label,lv_color_hex(0x232225),0);
    lv_obj_align(comm_status_label, LV_ALIGN_BOTTOM_RIGHT, -0, -20);

    lv_timer_create(comm_label_update, 1000, comm_status_label);
    lv_obj_add_flag(comm_status_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(comm_status_label, on_label_click, LV_EVENT_CLICKED, NULL);

    // UI_COMM 界面
    dp_comm = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(dp_comm,lv_color_hex(0x252532),0);

    // 文字提示
    lv_obj_t * mp_label = lv_label_create(dp_comm);
    lv_label_set_text(mp_label, "MCU now on multiplaying mode");
    lv_obj_set_style_text_color(mp_label,lv_color_hex(0x16C559),0);
    lv_obj_set_style_text_font(mp_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(mp_label, LV_ALIGN_CENTER, 0, -60);
    
    char img_path_buf[128];

    // 图标提示
    lv_obj_t * mp_img = img_create_from_dsc(dp_comm,img_path(MULTI_ICON,img_path_buf,128),96,82,NULL,&multi_icon_dsc,true);
    lv_obj_align(mp_img, LV_ALIGN_CENTER, 0, 30);

    // 图片按钮
    lv_obj_t * mp_btm = lv_btn_create(dp_comm);
    lv_obj_align(mp_btm, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_size(mp_btm,96,82);
    lv_obj_set_style_opa(mp_btm,LV_OPA_0,0);
    lv_obj_add_event_cb(mp_btm, on_mp_btm_click, LV_EVENT_CLICKED, NULL);

    // 弹窗
    mp_popup = popup_create(dp_comm);
    lv_obj_align(mp_popup, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(mp_popup, 400, 200);
    lv_obj_add_flag(mp_popup, LV_OBJ_FLAG_HIDDEN);

    // 弹窗标签
    lv_obj_t * mp_popup_label = lv_label_create(mp_popup);
    lv_obj_align(mp_popup_label, LV_ALIGN_CENTER, 0, -30);
    lv_label_set_text(mp_popup_label, "Disconnect from multiplayer mode?");
    lv_obj_set_style_text_font(mp_popup_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(mp_popup_label,lv_color_white(),0);

    // 弹窗按钮
    lv_obj_t * mp_popup_yes_btn = lv_btn_create(mp_popup);
    lv_obj_align(mp_popup_yes_btn, LV_ALIGN_CENTER, -90, 40);
    lv_obj_set_size(mp_popup_yes_btn,150,40);
    lv_obj_add_event_cb(mp_popup_yes_btn, on_mp_popup_yes_btn_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * mp_popup_no_btn = lv_btn_create(mp_popup);
    lv_obj_align(mp_popup_no_btn, LV_ALIGN_CENTER, 90, 40);
    lv_obj_set_size(mp_popup_no_btn,150,40);
    lv_obj_add_event_cb(mp_popup_no_btn, on_mp_popup_no_btn_click, LV_EVENT_CLICKED, NULL);

    // 弹窗按钮标签
    lv_obj_t * mp_popup_yes_btn_label = lv_label_create(mp_popup_yes_btn);
    lv_obj_align(mp_popup_yes_btn_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(mp_popup_yes_btn_label, "Yes");
    lv_obj_set_style_text_font(mp_popup_yes_btn_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(mp_popup_yes_btn_label,lv_color_white(),0);

    lv_obj_t * mp_popup_no_btn_label = lv_label_create(mp_popup_no_btn);
    lv_obj_align(mp_popup_no_btn_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(mp_popup_no_btn_label, "No");
    lv_obj_set_style_text_font(mp_popup_no_btn_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(mp_popup_no_btn_label,lv_color_white(),0);


    // 事件注册
    mp_event_register(MP_EVENT_CONNECTED,on_mp_event_cb);
    mp_event_register(MP_EVENT_DISCONNECTED,on_mp_event_cb);

    CONSOLE_INFO("Communication status UI initialized on overlay");
}

/**
 * @brief 通信界面运行函数
 */
void ui_comm_run()
{
    lv_scr_load(dp_comm);
    set_group(NULL);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 更新通信状态标签
 */
static void comm_label_update(lv_timer_t * t)
{
    lv_obj_t * label = t->user_data;
    comm_status_t status = comm_get_status();
    static comm_status_t last_status = COMM_STATUS_DISCONNECTED;
    if (last_status == status) return ;
    last_status = status;
    const char * status_str = NULL;
    lv_color_t status_color = lv_color_hex(0xE8E8E8);
    switch (status) {
        case COMM_STATUS_DISCONNECTED:
            status_str = "Comm: Disconnected";
            status_color = lv_color_hex(0xE8E8E8);
            break ;
        case COMM_STATUS_CONNECTED:
            status_str = "Comm: Connected";
            status_color = lv_color_hex(0x1CE91F);
            break ;
        case COMM_STATUS_ERROR:
            status_str = "Comm: Error";
            status_color = lv_color_hex(0xE20808);
            break ;
        case COMM_STATUS_CONNECTING:
            status_str = "Comm: Connecting";
            status_color = lv_color_hex(0xE3A41C);
            break ;
        default:
            status_str = "Comm: Unknown";
            status_color = lv_color_hex(0x1030CB);
            break ;
    }
    lv_label_set_text(label, status_str);
    lv_obj_set_style_text_color(label,status_color,0);
    lv_obj_set_style_bg_color(label,lv_color_hex(0x232225),0);
}

/**
 * @brief 点击标签事件 用点击标签来连接或者断开
 */
static void on_label_click(lv_event_t * e)
{
#ifdef SIMULATOR
    comm_status_t status = comm_get_status();
    if (status == COMM_STATUS_DISCONNECTED) {
        comm_connect(NULL);
    } else {
        comm_disconnect();
    }
#else
    (void)e; // MCU端点击标签无效
#endif
}

/**
 * @brief 多玩家事件回调函数
 */
static void on_mp_event_cb(mp_event_t event,void * v)
{
#ifdef SIMULATOR
    LV_UNUSED(event);
    LV_UNUSED(v);
    // PC上不做事 暂时不做处理
#else
    LV_UNUSED(v);
    switch (event) {
        case MP_EVENT_CONNECTED:
            save_write();
            fsm_switch_state(GS_COMM);
            break;
        case MP_EVENT_DISCONNECTED:
            //持续同步状态
            // 然后 存档 回到菜单界面
            save_write();
            fsm_switch_state(GS_MENU);
            break;
        default:
            break;
    }
#endif
}

/**
 * @brief 多人图标按钮点击回调
 */
static void on_mp_btm_click(lv_event_t * e)
{
    LV_UNUSED(e);
    popup_show(mp_popup);
}

/**
 * @brief 多人弹窗确认按钮点击回调
 */
static void on_mp_popup_yes_btn_click(lv_event_t * e)
{
    popup_hide(mp_popup);
    mp_disconnect();
}

/**
 * @brief 多人弹窗取消按钮点击回调
 */
static void on_mp_popup_no_btn_click(lv_event_t * e)
{
    popup_hide(mp_popup);
}
