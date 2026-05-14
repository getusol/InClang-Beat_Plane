/**
 * @file ui_sys_halt.h
 */

/*********************
 *      INCLUDES
 *********************/
#include "ui_sys_halt.h"
#include "tools.h"

#include "lvgl.h"
#include "debug.h"

#ifdef SIMULATOR
#include <stdlib.h>
#else
#include "gd32h7xx.h"
#endif // #ifdef SIMULATOR

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

 /**********************
  *  STATIC PROTOTYPES
  **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t * sys_halt_display;

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief sys_halt的ui界面
 */
void ui_sys_halt_init()
{
    //Parent
    sys_halt_display = lv_obj_create(NULL);
    lv_obj_clear_flag(sys_halt_display,LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * title_label = lv_label_create(sys_halt_display);
    lv_obj_set_align(title_label,LV_ALIGN_TOP_MID);
    lv_label_set_text(title_label,"SYSTEM HALT");
    lv_obj_set_style_text_font(title_label,&lv_font_montserrat_36,LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title_label,lv_palette_main(LV_PALETTE_RED),LV_STATE_DEFAULT);
}

/**
 * @brief sys_halt界面的行为管理函数
 */
void ui_sys_halt_run()
{
    lv_scr_load(sys_halt_display);

    const uint16_t log_start_y = 90;    // 日志起始Y坐标
    const uint16_t log_line_height = 30;// 每行高度
    for (uint8_t i = 0; i < HALT_LOG_CNT; i++)
    {
        // 获取单条日志（调用你debug的只读接口）
        const char *log_str = debug_get_halt_log(i);
        
        // 日志为空则跳过
        if (log_str == NULL || log_str[0] == '\0') {
            continue;
        }

        // 为每一条日志创建独立 Label
        lv_obj_t *label_log = lv_label_create(lv_scr_act());
        lv_label_set_text(label_log, log_str);
        // 左对齐，依次向下排列
        lv_obj_align(label_log, LV_ALIGN_TOP_LEFT, 10, log_start_y + i * log_line_height);
        // 小字16号
        lv_obj_set_style_text_font(label_log, &lv_font_montserrat_16, 0);
        // 带[Error]为红色 带[Warning]为橙色 二者皆无为绿色
        if (strstr(log_str,"[Error]"))
            lv_obj_set_style_text_color(label_log, lv_color_hex(0xD21414), 0);
        else if (strstr(log_str,"[Warning]"))
            lv_obj_set_style_text_color(label_log, lv_color_hex(0xEB6A00), 0);
        else
            lv_obj_set_style_text_color(label_log, lv_color_hex(0x04EB00), 0);
    }
    lv_timer_handler();   //立即刷新显示
    #ifndef SIMULATOR
        __disable_irq(); // 禁止中断，防止在停机界面时发生不可预料的行为
    while (1) {
        //停机界面不允许退出 卡住其它进程不运行 防止反复触发 sys_halt()
        delay_ms(100);
    }
    #endif // #ifndef SIMULATOR
    delay_ms(5000);
    exit(-1);  // 模拟器环境下停机界面显示5秒后退出程序
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/
