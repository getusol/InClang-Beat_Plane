#include "cg.h"
#include "lvgl.h"
#include "bgm.h"           // 引入音频控制
#include "ui_templates.h" 
#include "lvgl_utils.h"    // 引入 img_create_from_dsc 声明
#include <stdio.h>
#include <stdlib.h>        // 引入 malloc / free

#define CG_IMG1_NAME "cg_img1.bin"
#define CG_IMG2_NAME "cg_img2.bin"

#ifdef SIMULATOR
static uint8_t * cg1_buf = NULL;
static lv_img_dsc_t cg1_struct;
static uint8_t * cg2_buf = NULL;
static lv_img_dsc_t cg2_struct;
#endif

static void anim_fade_in_cb(void * obj, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)value, 0);
}

static void anim_fade_out_cb(void * obj, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)value, 0);
}


//必须先强行删除所有正在跑的渐变动画，防止图层被删后动画回调引发崩溃。

static void cg_skip(lv_obj_t * cg_layer)
{
    // 1. 立即清除所有相关的渐变动画（通过回调函数名精确清除）
    lv_anim_del(NULL, anim_fade_in_cb);
    lv_anim_del(NULL, anim_fade_out_cb);
    
    // 2. 销毁图层并释放内存（复用原有的清理逻辑）
    if (cg_layer) {
        lv_obj_del(cg_layer); // 删掉父图层，所有子图片和文字会自动被连带删除
        
#ifdef SIMULATOR
        if(cg1_buf) { free(cg1_buf); cg1_buf = NULL; }
        if(cg2_buf) { free(cg2_buf); cg2_buf = NULL; }
#endif
        music_bgm_load(); // 立即切回游戏主菜单的背景音乐
    }
}

//按键与点击的事件过滤器

static void cg_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * cg_layer = lv_event_get_target(e);

    // LV_EVENT_CLICKED 拦截：屏幕触摸、鼠标点击
    // LV_EVENT_KEY     拦截：键盘任意键、游戏手柄/实体按键
    if (code == LV_EVENT_CLICKED || code == LV_EVENT_KEY) {
        cg_skip(cg_layer);
    }
}

// 动画正常播放彻底结束后的回调（保持不变）
static void cg_anim_ready_cb(lv_anim_t * a)
{
    lv_obj_t * cg_layer = (lv_obj_t *)a->var;
    if (cg_layer) {
        lv_obj_del(cg_layer);
        
#ifdef SIMULATOR
        if(cg1_buf) { free(cg1_buf); cg1_buf = NULL; }
        if(cg2_buf) { free(cg2_buf); cg2_buf = NULL; }
#endif
        music_bgm_load(); 
    }
}

//cg_play 函数支持传入输入组（group）

void cg_play(lv_obj_t * parent, lv_group_t * group)
{
    audio_switch_track("CG.pcm", 1798144);

#ifdef SIMULATOR
    if (cg1_buf == NULL) {
        cg1_buf = (uint8_t *)malloc(1024 * 600 * 4);
    }
    if (cg2_buf == NULL) {
        cg2_buf = (uint8_t *)malloc(1024 * 600 * 4);
    }
#endif

    // 1. 创建全黑的基础父容器
    lv_obj_t * cg_layer = lv_obj_create(parent);
    lv_obj_set_size(cg_layer, 1024, 600);
    lv_obj_center(cg_layer);
    lv_obj_set_style_bg_color(cg_layer, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(cg_layer, LV_OPA_COVER, 0); 
    lv_obj_set_style_border_width(cg_layer, 0, 0);
    lv_obj_set_style_radius(cg_layer, 0, 0);
    lv_obj_clear_flag(cg_layer, LV_OBJ_FLAG_SCROLLABLE);

    
    lv_obj_add_flag(cg_layer, LV_OBJ_FLAG_CLICKABLE);              // 允许被点击
    lv_obj_set_style_outline_width(cg_layer, 0, LV_STATE_FOCUSED); // 隐藏聚焦时难看的橙色/蓝色外边框
    lv_obj_add_event_cb(cg_layer, cg_event_cb, LV_EVENT_ALL, NULL); // 绑定刚刚写的跳过事件

    if (group) {
        lv_group_add_obj(group, cg_layer); // 把 CG 图层塞进输入组
        lv_group_focus_obj(cg_layer);     // 强行聚焦到 CG 图层，这样按键才会优先发给它
    }
    // ----------------------------------------

    lv_obj_invalidate(cg_layer);

    char img_path_buf[64];

    // 2. 创建第一幕图片
    lv_obj_t * img1;
#ifdef SIMULATOR
    img1 = img_create_from_dsc(cg_layer, img_path(CG_IMG1_NAME, img_path_buf, 64), 1024, 600, cg1_buf, &cg1_struct, false);
#else
    img1 = lv_img_create(cg_layer);
    lv_img_set_src(img1, img_path(CG_IMG1_NAME, img_path_buf, 64));
#endif
    lv_obj_center(img1);
    lv_obj_set_style_opa(img1, LV_OPA_TRANSP, 0); 

    // 3. 创建第二幕图片
    lv_obj_t * img2;
#ifdef SIMULATOR
    img2 = img_create_from_dsc(cg_layer, img_path(CG_IMG2_NAME, img_path_buf, 64), 1024, 600, cg2_buf, &cg2_struct, false);
#else
    img2 = lv_img_create(cg_layer);
    lv_img_set_src(img2, img_path(CG_IMG2_NAME, img_path_buf, 64));
#endif
    lv_obj_center(img2);
    lv_obj_set_style_opa(img2, LV_OPA_TRANSP, 0); 

    // 4. 创建第一幕文字
    lv_obj_t * label1 = lv_label_create(cg_layer);
    lv_label_set_text(label1, "Once we thrived in unbroken peace.\nThen they came.");
    lv_obj_set_style_text_color(label1, lv_color_white(), 0);
    lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label1, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_set_style_opa(label1, LV_OPA_COVER, 0); 

    // 5. 创建第二幕文字
    lv_obj_t * label2 = lv_label_create(cg_layer);
    lv_obj_set_width(label2, 850); 
    lv_label_set_long_mode(label2, LV_LABEL_LONG_WRAP); 
    lv_label_set_text_static(label2, "Though the path be broken and uncertain,\n" "claim your place as the King of PlaneWar, and rebuild what we have lost.");
    lv_obj_set_style_text_color(label2, lv_color_white(), 0);
    lv_obj_set_style_text_align(label2, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label2, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_set_style_opa(label2, LV_OPA_TRANSP, 0); 

    
    // 6. 级联动画配置
    
    lv_anim_t a;

    // ---- 【5秒点】：文字1隐去 (用时300ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, label1);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 300);
    lv_anim_set_delay(&a, 5000);
    lv_anim_set_exec_cb(&a, anim_fade_out_cb); 
    lv_anim_start(&a);

    // ---- 【5秒点】：图片1显现 (用时800ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, img1);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, 800);
    lv_anim_set_delay(&a, 5000);
    lv_anim_set_exec_cb(&a, anim_fade_in_cb);  
    lv_anim_start(&a);

    // ---- 【10秒点】：图片1隐去切回黑屏 (用时400ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, img1);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 400);
    lv_anim_set_delay(&a, 10000);
    lv_anim_set_exec_cb(&a, anim_fade_out_cb); 
    lv_anim_start(&a);

    // ---- 【11秒点】：图片2显现 (用时800ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, img2);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, 800); 
    lv_anim_set_delay(&a, 11000);
    lv_anim_set_exec_cb(&a, anim_fade_in_cb);  
    lv_anim_start(&a);

    // ---- 【11秒点】：文字2显现 (用时800ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, label2);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, 800);
    lv_anim_set_delay(&a, 11000);
    lv_anim_set_exec_cb(&a, anim_fade_in_cb);  
    lv_anim_start(&a);

    // ---- 【16秒点】：图片2隐去 (用时500ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, img2);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 500);
    lv_anim_set_delay(&a, 16000);
    lv_anim_set_exec_cb(&a, anim_fade_out_cb); 
    lv_anim_start(&a);

    // ---- 【16秒点】：文字2隐去 (用时500ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, label2);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 500);
    lv_anim_set_delay(&a, 16000);
    lv_anim_set_exec_cb(&a, anim_fade_out_cb); 
    lv_anim_start(&a);

    // ---- 【16.5秒点】：全黑背景淡出，露出主菜单 ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, cg_layer);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 1500); 
    lv_anim_set_delay(&a, 16500);
    lv_anim_set_exec_cb(&a, anim_fade_out_cb);
    lv_anim_set_ready_cb(&a, cg_anim_ready_cb); 
    lv_anim_start(&a);

    lv_obj_invalidate(img1);
    lv_obj_invalidate(img2);
    lv_obj_invalidate(cg_layer);
    
    lv_obj_set_style_opa(img1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_opa(img2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_opa(label2, LV_OPA_TRANSP, 0); 
}
