/**
 * @file ui_setting.c
 */

/*********************
 * INCLUDES
 *********************/

#include "ui_setting.h"
#include "lvgl.h"
#include "lvgl_utils.h"
#include "ui_templates.h"
#include "tools.h"
#include "fsm.h"
#include "event.h"
#include "audio.h"
#include "ui_key.h"
#include "config.h"
#include "save.h"
#include "apr.h"
#include "game_object.h"
#include <stdio.h>

/**********************
 * MACROS
 **********************/

#define BACK_ARROW_IMG_NAME "back_arrow.bin"

/**********************
 * STATIC PROTOTYPES
 **********************/

static void back_btn_event_cb(lv_event_t * e);
static void bgm_slider_event_cb(lv_event_t * e);
static void bgm_preview_cb(lv_event_t * e);
static void sfx_preview_cb(lv_event_t * e);
static void sfx_slider_event_cb(lv_event_t * e);
static void amp_slider_event_cb(lv_event_t * e);
static void amp_preview_cb(lv_event_t * e);

static void update_volume_labels(void);
static void hitbox_switch_event_cb(lv_event_t * e);
static void update_hitbox_preview(void);
static void hurt_overlay_switch_event_cb(lv_event_t * e);
static void clear_save_btn_event_cb(lv_event_t * e);
static void clear_confirm_btn_cb(lv_event_t * e);
static void clear_cancel_btn_cb(lv_event_t * e);

/**********************
 * STATIC VARIABLES
 **********************/

static lv_obj_t * dp_setting = NULL;
static lv_obj_t * container = NULL;
static lv_group_t * setting_group = NULL;
static lv_obj_t * bgm_slider = NULL;
static lv_obj_t * sfx_slider = NULL;
static lv_obj_t * bgm_label = NULL;
static lv_obj_t * sfx_label = NULL;
static lv_obj_t * amp_label = NULL;
static lv_obj_t * amp_slider = NULL;
static lv_obj_t * hitbox_label = NULL;
static lv_obj_t * hitbox_switch = NULL;
static lv_obj_t * hitbox_preview_img = NULL;
static lv_obj_t * hitbox_preview_box = NULL;
static lv_obj_t * clear_save_label = NULL;
static lv_obj_t * clear_save_btn = NULL;
static lv_obj_t * clear_confirm_popup = NULL;
static lv_obj_t * hurt_overlay_label = NULL;
static lv_obj_t * hurt_overlay_switch = NULL;

#ifdef SIMULATOR
static lv_img_dsc_t back_arrow_img_struct;
#endif

// 记录进入设置前所在的状态，用于返回
static game_state_t setting_prev_state = GS_MENU;

/**********************
 * GLOBAL FUNCTIONS
 **********************/

void ui_setting_init(void)
{
    if (dp_setting != NULL) return;

    dp_setting = lv_obj_create(NULL);
    lv_obj_clear_flag(dp_setting, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(dp_setting, lv_color_hex(DP_PLAY_FILL_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dp_setting, LV_OPA_COVER, LV_PART_MAIN);

    setting_group = lv_group_create();

    // 标题
    lv_obj_t * title = lv_label_create(dp_setting);
    lv_label_set_text(title, "SETTINGS");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_44, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    // popup 容器
    container = popup_create(dp_setting);
    lv_obj_set_size(container,600,500);
    lv_obj_set_align(container,LV_ALIGN_CENTER);
    lv_obj_set_pos(container,0,40);
    lv_obj_add_flag(container,LV_OBJ_FLAG_SCROLLABLE);

    // ---- 总体 音量 放大器 ---
    amp_label = lv_label_create(container);
    lv_obj_set_style_text_font(amp_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(amp_label, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_pos(amp_label, 60, 20);

    amp_slider = lv_slider_create(container);
    lv_obj_set_size(amp_slider, 400, 20);
    lv_obj_set_pos(amp_slider, 60, 60);
    lv_slider_set_range(amp_slider, 0, 3);
    lv_slider_set_value(amp_slider, audio_get_vol_amp(), LV_ANIM_OFF);
    lv_obj_add_event_cb(amp_slider, amp_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(amp_slider,amp_preview_cb,LV_EVENT_RELEASED,NULL);
    lv_group_add_obj(setting_group, amp_slider);

    // ---- BGM 音量 ----
    bgm_label = lv_label_create(container);
    lv_obj_set_style_text_font(bgm_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(bgm_label, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_pos(bgm_label, 60, 100);

    bgm_slider = lv_slider_create(container);
    lv_obj_set_size(bgm_slider, 400, 20);
    lv_obj_set_pos(bgm_slider, 60, 140);
    lv_slider_set_range(bgm_slider, 0, 100);
    lv_slider_set_value(bgm_slider, (audio_get_vol_bgm() * 100) >> 8, LV_ANIM_OFF);
    lv_obj_add_event_cb(bgm_slider, bgm_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(bgm_slider,bgm_preview_cb,LV_EVENT_RELEASED,NULL);
    lv_group_add_obj(setting_group, bgm_slider);

    // ---- SFX 音量 ----
    sfx_label = lv_label_create(container);
    lv_obj_set_style_text_font(sfx_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(sfx_label, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_pos(sfx_label, 60, 180);

    sfx_slider = lv_slider_create(container);
    lv_obj_set_size(sfx_slider, 400, 20);
    lv_obj_set_pos(sfx_slider, 60, 220);
    lv_slider_set_range(sfx_slider, 0, 100);
    lv_slider_set_value(sfx_slider, (audio_get_vol_sfx() * 100) >> 8, LV_ANIM_OFF);
    lv_obj_add_event_cb(sfx_slider, sfx_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(sfx_slider,sfx_preview_cb,LV_EVENT_RELEASED,NULL);
    lv_group_add_obj(setting_group, sfx_slider);

    // ---- Show Hitbox 开关 ----
    hitbox_label = lv_label_create(container);
    lv_obj_set_style_text_font(hitbox_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(hitbox_label, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_pos(hitbox_label, 60, 260);

    hitbox_switch = lv_switch_create(container);
    lv_obj_set_pos(hitbox_switch, 60, 300);
    lv_obj_add_event_cb(hitbox_switch, hitbox_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_group_add_obj(setting_group, hitbox_switch);

    // 玩家预览图像（右侧，使用默认飞机外观）
    apr_t *preview_apr = apr_get(APR_PLAYER_DEFAULT);
#ifdef SIMULATOR
    hitbox_preview_img = lv_img_create(container);
    lv_img_set_src(hitbox_preview_img, &preview_apr->img_dsc);
#else
    char preview_path_buf[64];
    hitbox_preview_img = lv_img_create(container);
    lv_img_set_src(hitbox_preview_img, img_path(preview_apr->img_name, preview_path_buf, 64));
#endif
    lv_obj_set_pos(hitbox_preview_img, 410, 270);
    lv_obj_set_size(hitbox_preview_img, preview_apr->w, preview_apr->h);

    // 预览碰撞框（红色线框，叠在预览图上）
    hitbox_preview_box = lv_obj_create(hitbox_preview_img);
    lv_obj_remove_style_all(hitbox_preview_box);
    lv_obj_set_style_border_width(hitbox_preview_box, 1, 0);
    lv_obj_set_style_border_color(hitbox_preview_box, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_border_opa(hitbox_preview_box, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(hitbox_preview_box, LV_OPA_0, 0);
    lv_obj_set_style_radius(hitbox_preview_box, 0, 0);
    lv_obj_clear_flag(hitbox_preview_box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(hitbox_preview_box, preview_apr->hitbox_x, preview_apr->hitbox_y);
    lv_obj_set_size(hitbox_preview_box, preview_apr->hitbox_w, preview_apr->hitbox_h);

    // ---- Show Hurt Overlay 开关 ----
    hurt_overlay_label = lv_label_create(container);
    lv_obj_set_style_text_font(hurt_overlay_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);
    lv_label_set_text_fmt(hurt_overlay_label, "Show Hurt Overlay: %s", game_obj_get_show_hurt_overlay() ? "ON" : "OFF");
    lv_obj_set_style_text_color(hurt_overlay_label, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_pos(hurt_overlay_label, 60, 340);

    hurt_overlay_switch = lv_switch_create(container);
    lv_obj_set_pos(hurt_overlay_switch, 60, 380);
    lv_obj_add_event_cb(hurt_overlay_switch, hurt_overlay_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_group_add_obj(setting_group, hurt_overlay_switch);

    // ---- 清空存档 ----
    clear_save_label = lv_label_create(container);
    lv_obj_set_style_text_font(clear_save_label, &lv_font_montserrat_22, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(clear_save_label, lv_color_make(255, 0, 0), LV_STATE_DEFAULT);
    lv_label_set_text(clear_save_label, "CLEAR SAVE DATA");
    lv_obj_set_pos(clear_save_label, 60, 450);

    clear_save_btn = lv_btn_create(container);
    lv_obj_set_size(clear_save_btn, 200, 50);
    lv_obj_set_pos(clear_save_btn, 60, 490);
    lv_obj_set_style_bg_color(clear_save_btn, lv_color_hex(0xD32F2F), LV_STATE_DEFAULT);
    lv_obj_add_event_cb(clear_save_btn, clear_save_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(setting_group, clear_save_btn);

    lv_obj_t * clear_btn_lbl = lv_label_create(clear_save_btn);
    lv_obj_center(clear_btn_lbl);
    lv_label_set_text(clear_btn_lbl, "Clear");
    lv_obj_set_style_text_font(clear_btn_lbl, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    // 清空确认弹窗
    clear_confirm_popup = popup_create(dp_setting);
    lv_obj_add_flag(clear_confirm_popup, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(clear_confirm_popup,0,40);
    lv_obj_set_size(clear_confirm_popup, 400,450);

    lv_obj_t * clear_confirm_label = lv_label_create(clear_confirm_popup);
    lv_obj_set_pos(clear_confirm_label, 30, 50);
    lv_label_set_text(clear_confirm_label, "CLEAR\nALL SAVE DATA?\n\nThis cannot be undone!");
    lv_obj_set_style_text_font(clear_confirm_label, &lv_font_montserrat_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(clear_confirm_label, lv_color_make(255, 80, 80), LV_STATE_DEFAULT);

    lv_obj_t * confirm_btn = lv_btn_create(clear_confirm_popup);
    lv_obj_set_size(confirm_btn, 260, 55);
    lv_obj_set_pos(confirm_btn, 50, 240);
    lv_obj_set_style_bg_color(confirm_btn, lv_color_hex(0xD32F2F), LV_STATE_DEFAULT);
    lv_obj_add_event_cb(confirm_btn, clear_confirm_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * confirm_lbl = lv_label_create(confirm_btn);
    lv_obj_center(confirm_lbl);
    lv_label_set_text(confirm_lbl, "Confirm Clear");
    lv_obj_set_style_text_font(confirm_lbl, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    lv_obj_t * cancel_btn = lv_btn_create(clear_confirm_popup);
    lv_obj_set_size(cancel_btn, 260, 55);
    lv_obj_set_pos(cancel_btn, 50, 310);
    lv_obj_add_event_cb(cancel_btn, clear_cancel_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * cancel_lbl = lv_label_create(cancel_btn);
    lv_obj_center(cancel_lbl);
    lv_label_set_text(cancel_lbl, "Cancel");
    lv_obj_set_style_text_font(cancel_lbl, &lv_font_montserrat_22, LV_STATE_DEFAULT);

    // 右上角返回按钮 (64x64 透明 + 箭头图标)
    lv_obj_t * back_btn = lv_btn_create(dp_setting);
    lv_obj_set_size(back_btn, 64, 64);
    lv_obj_align(back_btn, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(back_btn, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(back_btn, 0, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);

    char img_path_buf[64];
#ifdef SIMULATOR
    lv_obj_t * back_img = img_create_from_dsc(back_btn,
        img_path(BACK_ARROW_IMG_NAME, img_path_buf, sizeof(img_path_buf)),
        64, 64, NULL, &back_arrow_img_struct, true);
#else
    lv_obj_t * back_img = lv_img_create(back_btn);
    lv_img_set_src(back_img, img_path(BACK_ARROW_IMG_NAME, img_path_buf, sizeof(img_path_buf)));
#endif
    lv_obj_center(back_img);

    CONSOLE_INFO("Settings screen initialized.");
}

void ui_setting_run(void)
{
    if (dp_setting == NULL) return;

    lv_scr_load(dp_setting);
    set_group(setting_group);

    // 刷新滑块到当前音量 (内部 0-255 → 显示 0-100)
    if (bgm_slider) lv_slider_set_value(bgm_slider, (audio_get_vol_bgm() * 100) >> 8, LV_ANIM_OFF);
    if (sfx_slider) lv_slider_set_value(sfx_slider, (audio_get_vol_sfx() * 100) >> 8, LV_ANIM_OFF);

    // 刷新 hitbox 开关状态
    if (hitbox_switch) {
        if (game_obj_get_show_hitbox()) {
            lv_obj_add_state(hitbox_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(hitbox_switch, LV_STATE_CHECKED);
        }
        update_hitbox_preview();
    }

    // 刷新 hurt overlay 开关状态
    if (hurt_overlay_switch) {
        if (game_obj_get_show_hurt_overlay()) {
            lv_obj_add_state(hurt_overlay_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(hurt_overlay_switch, LV_STATE_CHECKED);
        }
    }

    update_volume_labels();
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

static void update_volume_labels(void)
{
    if (bgm_label) {
        lv_label_set_text_fmt(bgm_label, "BGM Volume: %d%%", ((audio_get_vol_bgm() * 100) >> 8) == 99 ? 100 : ((audio_get_vol_bgm() * 100) >> 8));
    }
    if (sfx_label) {
        lv_label_set_text_fmt(sfx_label, "SFX Volume: %d%%", ((audio_get_vol_sfx() * 100) >> 8) == 99 ? 100 : ((audio_get_vol_sfx() * 100) >> 8));
    }
    if (amp_label) {
        switch (audio_get_vol_amp()) {
            case 0:
                lv_label_set_text_fmt(amp_label,"Amplifier: 50%%");
                break;
            case 1:
                lv_label_set_text_fmt(amp_label,"Amplifier: 100%%");
                break;
            case 2:
                lv_label_set_text_fmt(amp_label,"Amplifier: 200%%");
                break;
            case 3:
                lv_label_set_text_fmt(amp_label,"Amplifier: 400%%");
                break;
            default:
                lv_label_set_text_fmt(amp_label,"Amplifier: 00%%");
                break; 
        }
    }
}

static void update_hitbox_preview(void)
{
    if (hitbox_label == NULL || hitbox_preview_box == NULL) return;

    bool on = lv_obj_has_state(hitbox_switch, LV_STATE_CHECKED);
    lv_label_set_text_fmt(hitbox_label, "Show hitbox: %s", on ? "ON" : "OFF");

    if (on) {
        lv_obj_clear_flag(hitbox_preview_box, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(hitbox_preview_box, LV_OBJ_FLAG_HIDDEN);
    }
}

static void hitbox_switch_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    bool on = lv_obj_has_state(hitbox_switch, LV_STATE_CHECKED);
    game_obj_set_show_hitbox(on);
    update_hitbox_preview();
}

static void hurt_overlay_switch_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    bool on = lv_obj_has_state(hurt_overlay_switch, LV_STATE_CHECKED);
    lv_label_set_text_fmt(hurt_overlay_label, "Show Hurt Overlay: %s", on ? "ON" : "OFF");
    game_obj_set_show_hurt_overlay(on);
}

static void back_btn_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    save_write();
    audio_stop_all();
    audio_load(AUDIO_MOUSECLOSE,AUDIO_CHAN_AUTO,false);
    fsm_switch_state(setting_prev_state);
}

static void bgm_slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    uint8_t vol = (uint8_t)((lv_slider_get_value(slider) * 255) / 100);
    audio_set_vol_bgm(vol);
    update_volume_labels();
}

static void sfx_slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    uint8_t vol = (uint8_t)((lv_slider_get_value(slider) * 255) / 100);
    audio_set_vol_sfx(vol);
    update_volume_labels();
}

static void bgm_preview_cb(lv_event_t * e)
{
    audio_load(AUDIO_TROPICAL,AUDIO_CHAN_BGM,false);
}

static void sfx_preview_cb(lv_event_t * e)
{
    audio_load(AUDIO_FAH,AUDIO_CHAN_SFX1,false);
}

static void amp_slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    uint8_t vol = (uint8_t)lv_slider_get_value(slider);
    audio_set_vol_amp(vol);
    update_volume_labels();
}

static void amp_preview_cb(lv_event_t * e)
{
    audio_load(AUDIO_BASKETBALLMUSIC,AUDIO_CHAN_BGM,false);
}

static void clear_save_btn_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    popup_show(clear_confirm_popup);
    popup_hide(container);
}

static void clear_confirm_btn_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    popup_hide(clear_confirm_popup);
    popup_show(container);
    save_clear();
    save_load();
    CONSOLE_INFO("Save data cleared from settings.");
}

static void clear_cancel_btn_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    popup_hide(clear_confirm_popup);
    popup_show(container);
}
