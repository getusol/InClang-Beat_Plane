/**
 * @file shop.h
 * @brief 商店抽奖界面头文件 (适配 LVGL 原生定时器与状态机)
 */

#ifndef SHOP_H
#define SHOP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 * INCLUDES
 *********************/
/* 如果有其他底层依赖可以写在这里 */

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief 商店界面的全布局显式初始化
 * @note 建议在程序刚启动时（例如在 main.c 中与其他 UI 初始化函数一起）调用一次。
 * 也可以不调用，由 ui_shop_run 内部进行第一次进入时的懒加载初始化。
 */
void ui_shop_init(void);

/**
 * @brief 状态机绑定的核心商店运行/切入函数
 * @note 当状态机检测到状态切换（last_game_state != fsm_get_state()）
 * 并且当前状态为 GS_SHOP 时，进入 switch-case 触发此函数。
 */
void ui_shop_run(void);
static void shop_coin_update_timer_cb(lv_timer_t * timer);
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* SHOP_H */