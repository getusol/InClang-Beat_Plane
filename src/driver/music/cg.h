#ifndef CG_H
#define CG_H

#include "lvgl.h"

/**
 * @brief 播放开场 CG 动画
 * @param parent 挂载动画效果的父对象（通常是当前屏幕对象）
 */
void cg_play(lv_obj_t * parent);

#endif /* CG_H */