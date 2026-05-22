/**
 * @file coin.h
 */

#ifndef COIN_H
#define COIN_H

/*********************
 * INCLUDES
 *********************/
#include "lvgl.h"   // 提供 lv_obj_t 和 lv_coord_t 类型定义
#include "game.h"   // 提供 game_obj_t 基类定义

/**********************
 * MACROS
 **********************/
// 注意：MAX_COIN_COUNT 已经在您的 "config.h" 中定义了，因此这里无需重复定义。

/**********************
 * GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化金币系统
 * 清空金币数组、初始化对象池、绑定函数指针、创建 LVGL 图像部件并默认隐藏。
 * 同时向事件系统注册全局的“玩家撞击金币”事件回调。
 * * @param parent 金币显示的父容器指针（通常是游戏的主播放画面容器）
 */
void coin_init(lv_obj_t * parent);

/**
 * @brief 在指定坐标位置生成（激活）一枚金币
 * 内部会从 pool 对象池中申请空闲槽位，设置坐标并展现。
 * * @param x 金币生成的初始 X 坐标
 * @param y 金币生成的初始 Y 坐标
 * @return game_obj_t* 返回金币的游戏对象基类指针；若对象池已满，则返回 NULL
 */
game_obj_t * coin_spawn(lv_coord_t x, lv_coord_t y);

#endif /* COIN_H */