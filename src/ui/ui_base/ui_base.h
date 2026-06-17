/**
 * @file ui_base.h
 * @brief 基地/机库 — 飞机选择 & 解锁状态查询
 */

#ifndef __UI_BASE_H__
#define __UI_BASE_H__

/*********************
 *      INCLUDES
 *********************/

#include "character.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void ui_base_init_stage1(void);
void ui_base_init_stage2(void);
void ui_base_run(void);

character_id_t ui_base_get_selected_character_id(void);
void           ui_base_set_selected_character_id(character_id_t id);
bool           ui_base_character_is_unlocked(character_id_t id);
void           ui_base_character_unlock(character_id_t id);

#endif /* __UI_BASE_H__ */
