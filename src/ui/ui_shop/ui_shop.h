/**
 * @file shop.h
 * @brief 商店抽奖界面头文件 (适配 LVGL 原生定时器与状态机)
 */

#ifndef __SHOP_H__
#define __SHOP_H__

/*********************
 * INCLUDES
 *********************/
/* 如果有其他底层依赖可以写在这里 */

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void ui_shop_init_stage1(void);
void ui_shop_init_stage2(void);
void ui_shop_run(void);
void ui_shop_esc_behave(void);
int  ui_shop_get_draw_cnt(void);
void ui_shop_set_draw_cnt(int cnt);
int  ui_shop_coin_get(void);
void ui_shop_coin_add(int amount);

#endif // #ifndef __SHOP_H__
