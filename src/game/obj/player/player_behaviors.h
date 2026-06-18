/**
 * @file player_behaviors.h
 * @brief 玩家行为模块 — 由 input_device 驱动的玩家控制
 */

#ifndef __PLAYER_BEHAVIORS_H__
#define __PLAYER_BEHAVIORS_H__

/*********************
 *      INCLUDES
 *********************/

#include "game_object.h"

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**
 * @brief 玩家受控行为 — 从 input_device 读取输入驱动玩家
 * @param g 玩家 game_obj
 * @param v input_device_t* 设备指针
 *
 * 作为 behave_t.f 使用, behave.usr_data 须指向有效的 input_device_t。
 *
 * - 摇杆 → vx/vy (速度 = g->speed)
 * - KEY_A 按住 → 连射 (CD = cfg->fire_cd)
 * - KEY_X 按下 → 技能 X (CD = cfg->skill_x_cd)
 * - KEY_Y 按下 → 技能 Y (CD = cfg->skill_y_cd)
 */
void player_control(game_obj_t *g, void *v);

#endif /* __PLAYER_BEHAVIORS_H__ */
