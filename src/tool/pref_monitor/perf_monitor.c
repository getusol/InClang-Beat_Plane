/**
 * @file pref_monitor.h
 */

/*********************
 *      INCLUDES
 *********************/

#include "perf_monitor.h"
#include "lvgl.h"
#include "config.h"
#include <stdio.h>
 
static lv_obj_t * s_label = NULL;
static uint32_t s_mspt = 0;
static uint32_t s_mspf = 0;

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

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化pref_monitor
 *        白色 14 号 字体 右下角 黑色半透明背景
 */
void perf_monitor_init(lv_obj_t * parent) 
{
#if PERF_MONITOR
    s_label = lv_label_create(parent);
    lv_obj_set_align(s_label, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_style_text_color(s_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_color(s_label, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_label, LV_OPA_50, 0);
#endif
}

/**
 * @brief 设置每逻辑帧所用时间
 * @param mspt 毫秒/tick
 */
void perf_monitor_set_mspt(uint32_t mspt)
{
#if PERF_MONITOR
    s_mspt = mspt;
#endif
}

/**
 * @brief 设置每渲染帧所用时间
 * @param mspf 毫秒/frame
 */
void perf_monitor_set_mspf(uint32_t mspf)
{
#if PERF_MONITOR
    s_mspf = mspf;
#endif
}

void perf_monitor_update(void)
{
#if PERF_MONITOR    
    if (s_label == NULL) {
        return ;
    }
    char buffer[32];
    snprintf(buffer,sizeof(buffer), "MSPT: %lu ms MSPF: %lu ms", s_mspt, s_mspf);
    lv_label_set_text(s_label, buffer);
#endif
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/
