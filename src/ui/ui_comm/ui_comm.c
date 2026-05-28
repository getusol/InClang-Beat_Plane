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

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void comm_label_update(lv_timer_t * t);
static void on_label_click(lv_event_t * e);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static lv_obj_t *comm_status_label = NULL;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

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

    CONSOLE("[INFO] Communication status UI initialized on overlay\n");
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
