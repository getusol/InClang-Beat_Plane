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

/* =============================================================
 * 【核心修复 1】分离淡入和淡出的回调函数！
 * 名字不同，LVGL 内部就不会把它们当成冲突动画去覆盖吞掉。
 * 统一改用全局 opa 属性，Label 和 图片 都能百分百完美隐藏。
 * ============================================================= */
static void anim_fade_in_cb(void * obj, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)value, 0);
}

static void anim_fade_out_cb(void * obj, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)value, 0);
}

// 动画彻底结束后的回调
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

void cg_play(lv_obj_t * parent)
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
    lv_obj_set_style_opa(img1, LV_OPA_TRANSP, 0); // 初始完全透明

    // 3. 创建第二幕图片
    lv_obj_t * img2;
#ifdef SIMULATOR
    img2 = img_create_from_dsc(cg_layer, img_path(CG_IMG2_NAME, img_path_buf, 64), 1024, 600, cg2_buf, &cg2_struct, false);
#else
    img2 = lv_img_create(cg_layer);
    lv_img_set_src(img2, img_path(CG_IMG2_NAME, img_path_buf, 64));
#endif
    lv_obj_center(img2);
    lv_obj_set_style_opa(img2, LV_OPA_TRANSP, 0); // 初始完全透明

    // 4. 创建第一幕文字
    lv_obj_t * label1 = lv_label_create(cg_layer);
    lv_label_set_text(label1, "Once we thrived in unbroken peace.\nThen they came.");
    lv_obj_set_style_text_color(label1, lv_color_white(), 0);
    lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label1, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_set_style_opa(label1, LV_OPA_COVER, 0); // 第一幕文字初始直接显示

    // 5. 创建第二幕文字
    lv_obj_t * label2 = lv_label_create(cg_layer);
    lv_obj_set_width(label2, 850); 
    lv_label_set_long_mode(label2, LV_LABEL_LONG_WRAP); 
    lv_label_set_text_static(label2, "Though the path be broken and uncertain,\n" "claim your place as the King of PlaneWar, and rebuild what we have lost.");
    lv_obj_set_style_text_color(label2, lv_color_white(), 0);
    lv_obj_set_style_text_align(label2, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label2, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_set_style_opa(label2, LV_OPA_TRANSP, 0); // 初始完全透明


    // =============================================================
    // 6. 级联动画配置
    // =============================================================
    lv_anim_t a;

    // ---- 【5秒点】：文字1隐去 (用时300ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, label1);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 300);
    lv_anim_set_delay(&a, 5000);
    lv_anim_set_exec_cb(&a, anim_fade_out_cb); // 使用淡出
    lv_anim_start(&a);

    // ---- 【5秒点】：图片1显现 (用时800ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, img1);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, 800);
    lv_anim_set_delay(&a, 5000);
    lv_anim_set_exec_cb(&a, anim_fade_in_cb);  // 使用淡入
    lv_anim_start(&a);

    // ---- 【10秒点】：图片1隐去切回黑屏 (用时400ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, img1);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 400);
    lv_anim_set_delay(&a, 10000);
    lv_anim_set_exec_cb(&a, anim_fade_out_cb); // 使用淡出，不会覆盖上面的淡入！
    lv_anim_start(&a);

    // ---- 【11秒点】：图片2显现 (用时800ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, img2);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, 800); // 修复：从原先错误的 100 恢复为 800ms 柔和渐变
    lv_anim_set_delay(&a, 11000);
    lv_anim_set_exec_cb(&a, anim_fade_in_cb);  // 使用淡入
    lv_anim_start(&a);

    // ---- 【11秒点】：文字2显现 (用时800ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, label2);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, 800);
    lv_anim_set_delay(&a, 11000);
    lv_anim_set_exec_cb(&a, anim_fade_in_cb);  // 使用淡入
    lv_anim_start(&a);

    // ---- 【16秒点】：图片2隐去 (用时500ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, img2);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 500);
    lv_anim_set_delay(&a, 16000);
    lv_anim_set_exec_cb(&a, anim_fade_out_cb); // 使用淡出
    lv_anim_start(&a);

    // ---- 【16秒点】：文字2隐去 (用时500ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, label2);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 500);
    lv_anim_set_delay(&a, 16000);
    lv_anim_set_exec_cb(&a, anim_fade_out_cb); // 使用淡出
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