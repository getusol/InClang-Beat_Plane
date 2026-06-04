/**
 * @file ui_cg.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "ui_cg.h"
#include "lvgl.h"
#include "audio.h"
#include "lvgl_utils.h"
#include "fsm.h"
#include "debug.h"

/**********************
 *      MACROS
 **********************/

#define CG_IMG1_NAME "CG_img1.bin"
#define CG_IMG2_NAME "CG_img2.bin"

/**********************
 *  STATIC PROTOTYPES
 **********************/

// 全局 OPA 动画回调（还原原版 cg.c 经典写法）
static void anim_fade_in_cb(void * obj, int32_t value);
static void anim_fade_out_cb(void * obj, int32_t value);
static void cg_anim_ready_cb(lv_anim_t * a);

static void cg_clean_up_resources(void);
static void cg_layer_event_cb(lv_event_t * e);

static void switch_timer_cb(lv_timer_t * t);

/**********************
 *  STATIC VARIABLES
 **********************/

#ifdef SIMULATOR
static lv_img_dsc_t cg1_struct;
static lv_img_dsc_t cg2_struct;
#endif

static lv_obj_t * dp_cg = NULL;       // 常驻黑色底板屏幕
static lv_obj_t * cg_layer = NULL;    // 临时 CG 图层容器，作为所有播放组件的父容器
static lv_obj_t * img1 = NULL;
static lv_obj_t * img2 = NULL;
static lv_obj_t * label1 = NULL;
static lv_obj_t * label2 = NULL;

static lv_timer_t * switch_timer = NULL;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化cg需要用到的全部组件
 */
void ui_cg_init()
{
    //CONSOLE("[DEBUG] ui_cg_init() -> ENTERED");
    //LOG("[DEBUG] ui_cg_init() -> ENTERED");

    // 1. 如果已存在未销毁的临时图层，先执行清理
    if (cg_layer != NULL) {
        //CONSOLE("[DEBUG] cg_layer is not NULL (pointer: %p), forcing skip cleanup", cg_layer);
        ui_cg_skip();
    }

    // 2. 如果常驻底板屏幕还未创建，则创建一次。底板屏幕全黑且不轻易被删除
    if (dp_cg == NULL) {
        dp_cg = lv_obj_create(NULL);
        lv_obj_set_size(dp_cg, 1024, 600);
        lv_obj_center(dp_cg);
        lv_obj_set_style_bg_color(dp_cg, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(dp_cg, LV_OPA_COVER, 0); 
        lv_obj_set_style_border_width(dp_cg, 0, 0);
        lv_obj_set_style_radius(dp_cg, 0, 0);
        lv_obj_clear_flag(dp_cg, LV_OBJ_FLAG_SCROLLABLE);
        //CONSOLE("[DEBUG] Persistent black background screen (dp_cg) created: %p", dp_cg);
    }

    // 3. 在 dp_cg 屏幕上建立播放专用的黑色子图层
    cg_layer = lv_obj_create(dp_cg);
    //CONSOLE("[DEBUG] Child container cg_layer created on dp_cg: %p", cg_layer);

    // 将释放底层图片的逻辑，注册在该图层的销毁事件（DELETE）中
    lv_obj_add_event_cb(cg_layer, cg_layer_event_cb, LV_EVENT_DELETE, NULL);

    lv_obj_set_size(cg_layer, 1024, 600);
    lv_obj_center(cg_layer);
    lv_obj_set_style_bg_color(cg_layer, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(cg_layer, LV_OPA_COVER, 0); 
    lv_obj_set_style_border_width(cg_layer, 0, 0);
    lv_obj_set_style_radius(cg_layer, 0, 0);
    lv_obj_clear_flag(cg_layer, LV_OBJ_FLAG_SCROLLABLE);

    char img_path_buf[64];

    // 4. 创建子组件（父容器全部从 dp_cg 变更为子图层 cg_layer）
#ifdef SIMULATOR
    img1 = img_create_from_dsc(cg_layer, img_path(CG_IMG1_NAME, img_path_buf, sizeof(img_path_buf)), 1024, 600, NULL, &cg1_struct, false);
#else
    img1 = lv_img_create(cg_layer);
    lv_img_set_src(img1, img_path(CG_IMG1_NAME, img_path_buf, sizeof(img_path_buf)));
#endif
    lv_obj_center(img1);
    lv_obj_set_style_opa(img1, LV_OPA_TRANSP, 0);

#ifdef SIMULATOR
    img2 = img_create_from_dsc(cg_layer, img_path(CG_IMG2_NAME, img_path_buf, sizeof(img_path_buf)), 1024, 600, NULL, &cg2_struct, false);
#else
    img2 = lv_img_create(cg_layer);
    lv_img_set_src(img2, img_path(CG_IMG2_NAME, img_path_buf, sizeof(img_path_buf)));
#endif
    lv_obj_center(img2);
    lv_obj_set_style_opa(img2, LV_OPA_TRANSP, 0);

    // 文字
    label1 = lv_label_create(cg_layer);
    lv_label_set_text(label1, "Once we thrived in unbroken peace.\nThen they came.");
    lv_obj_set_style_text_color(label1, lv_color_white(), 0);
    lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label1, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_set_style_opa(label1, LV_OPA_COVER, 0);

    label2 = lv_label_create(cg_layer);
    lv_obj_set_width(label2, 850); 
    lv_label_set_long_mode(label2, LV_LABEL_LONG_WRAP); 
    lv_label_set_text_static(label2, "Though the path be broken and uncertain,\n" "claim your place as the King of PlaneWar, and rebuild what we have lost.");
    lv_obj_set_style_text_color(label2, lv_color_white(), 0);
    lv_obj_set_style_text_align(label2, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label2, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_set_style_opa(label2, LV_OPA_TRANSP, 0); 

    //CONSOLE("[DEBUG] ui_cg_init() -> LEAVING");
}

/**
 * @brief 播放音乐 + lvgl动画
 */
void ui_cg_run()
{
    //CONSOLE("[DEBUG] ui_cg_run() -> ENTERED");
    if (dp_cg == NULL || cg_layer == NULL) return;
    
    // 加载黑色的底板屏幕
    lv_scr_load(dp_cg);
    lv_anim_t a;

    lv_obj_set_style_opa(img1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_opa(img2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_opa(label2, LV_OPA_TRANSP, 0);

    // ---- 【5秒点】：文字1隐去 (用时300ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, label1);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 300);
    lv_anim_set_delay(&a, 5000);
    lv_anim_set_exec_cb(&a, anim_fade_out_cb); 
    lv_anim_set_early_apply(&a, false);
    lv_anim_start(&a);

    // ---- 【5秒点】：图片1显现 (用时800ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, img1);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, 800);
    lv_anim_set_delay(&a, 5000);
    lv_anim_set_exec_cb(&a, anim_fade_in_cb);  
    lv_anim_set_early_apply(&a, false);
    lv_anim_start(&a);

    // ---- 【10秒点】：图片1隐去切回黑屏 (用时400ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, img1);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 400);
    lv_anim_set_delay(&a, 10000);
    lv_anim_set_exec_cb(&a, anim_fade_out_cb); // 使用全局淡出，不会覆盖上面的淡入！
    lv_anim_set_early_apply(&a, false);
    lv_anim_start(&a);

    // ---- 【11秒点】：图片2显现 (用时800ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, img2);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, 800); 
    lv_anim_set_delay(&a, 11000);
    lv_anim_set_exec_cb(&a, anim_fade_in_cb);  
    lv_anim_set_early_apply(&a, false);
    lv_anim_start(&a);
    

    // ---- 【11秒点】：文字2显现 (用时800ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, label2);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, 800);
    lv_anim_set_delay(&a, 11000);
    lv_anim_set_exec_cb(&a, anim_fade_in_cb);  
    lv_anim_set_early_apply(&a, false);
    lv_anim_start(&a);
    

    // ---- 【16秒点】：图片2隐去 (用时500ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, img2);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 500);
    lv_anim_set_delay(&a, 16000);
    lv_anim_set_exec_cb(&a, anim_fade_out_cb); 
    lv_anim_set_early_apply(&a, false);
    lv_anim_start(&a);
    

    // ---- 【16秒点】：文字2隐去 (用时500ms) ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, label2);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 500);
    lv_anim_set_delay(&a, 16000);
    lv_anim_set_exec_cb(&a, anim_fade_out_cb); 
    lv_anim_set_early_apply(&a, false);
    lv_anim_start(&a);
    

    // ---- 【16.5秒点】：全黑背景淡出，露出黑色的常驻屏幕 dp_cg （彻底杜绝白屏闪烁） ----
    lv_anim_init(&a);
    lv_anim_set_var(&a, cg_layer);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 1500); 
    lv_anim_set_delay(&a, 16500);
    lv_anim_set_exec_cb(&a, anim_fade_out_cb);
    lv_anim_set_ready_cb(&a, cg_anim_ready_cb); 
    lv_anim_set_early_apply(&a, false);
    lv_anim_start(&a);

    //CONSOLE("[DEBUG] ui_cg_run() -> LEAVING");
}

/**
 * @brief 跳过动画
 */
void ui_cg_skip()
{
    //CONSOLE("[DEBUG] ui_cg_skip() -> ENTERED");
    //LOG("[DEBUG] ui_cg_skip() -> ENTERED");
    if (cg_layer == NULL) return;

    // 清理可能仍然附着在这些组件上的未完成动画
    if(cg_layer) lv_anim_del(cg_layer, NULL);
    if(img1) lv_anim_del(img1, NULL);
    if(img2) lv_anim_del(img2, NULL);
    if(label1) lv_anim_del(label1, NULL);
    if(label2) lv_anim_del(label2, NULL);

    if(switch_timer) lv_timer_del(switch_timer);

    audio_stop(AUDIO_CHAN_BGM);
    fsm_switch_state(GS_MENU);

    //CONSOLE("[DEBUG] ui_cg_skip() -> LEAVING");
    //LOG("[DEBUG] ui_cg_skip() -> LEAVING");
}

/**
 * @brief 供状态机在安全上下文调用的资源释放接口
 */
void ui_cg_cleanup(void)
{
    //CONSOLE("[DEBUG] ui_cg_cleanup() called synchronously from safe state machine context!");
    cg_clean_up_resources();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void anim_fade_in_cb(void * obj, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)value, 0);
}

static void anim_fade_out_cb(void * obj, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)value, 0);
}

static void cg_anim_ready_cb(lv_anim_t * a)
{
    //CONSOLE("[DEBUG] cg_anim_ready_cb() triggered!");
    //LOG("[DEBUG] cg_anim_ready_cb() triggered!");
    audio_stop(AUDIO_CHAN_BGM); 
    switch_timer = lv_timer_create(switch_timer_cb, 500, NULL);
    lv_timer_set_repeat_count(switch_timer, 1);
}

/**
 * @brief 回收内存
 */
static void cg_clean_up_resources(void)
{
    //CONSOLE("[DEBUG] cg_clean_up_resources() -> ENTERED");
    if (cg_layer == NULL) {
        //CONSOLE("[DEBUG] cg_layer is already NULL, skipping cleanup.");
        return ;
    }

    // 仅仅销毁 cg_layer。因为图片和文本是它的子节点，
    // 销毁父节点会自动递归释放所有子节点，并安全触发绑定的 EVENT_DELETE 事件释放内存！
    // dp_cg 常驻黑屏不需要被销毁，因此 100% 不会发生活动屏幕被删除导致的死机。
    lv_obj_del(cg_layer);

    cg_layer = NULL;
    img1 = NULL;
    img2 = NULL;
    label1 = NULL;
    label2 = NULL;
    //CONSOLE("[DEBUG] All pointers reset to NULL");
}

/**
 * @brief 销毁事件监听
 */
static void cg_layer_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_DELETE) {
        //CONSOLE("[DEBUG] dp_cg_event_cb() Captured LV_EVENT_DELETE!");
#ifdef SIMULATOR
        free_img_dsc(&cg1_struct);
        free_img_dsc(&cg2_struct);
        //CONSOLE("[INFO] Image descriptor memory successfully released!");
#endif
    }
}

/**
 * @brief 切换到主菜单
 */
static void switch_timer_cb(lv_timer_t * t)
{
    fsm_switch_state(GS_MENU);
}
