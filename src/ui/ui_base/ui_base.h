/**
 * @file ui_base.h
 */

#ifndef __UI_BASE_H__
#define __UI_BASE_H__

/*********************
 *      INCLUDES
 *********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/** 飞机 ID 枚举 —— 用于查询解锁状态、选择飞机等 */
typedef enum {
    PLANE_ID_DEFAULT = 0,
    PLANE_ID_EMBER   = 1,
    PLANE_ID_STREAM  = 2,
    PLANE_ID_VERDANT = 3,

    PLANE_ID_MAX,
} plane_id_t;

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void ui_base_init(void);
void ui_base_run(void);
plane_id_t ui_base_get_selected_plane_id(void);
void ui_base_set_selected_plane_id(plane_id_t id);
plane_id_t ui_base_get_p1_selected_plane_id(void);
plane_id_t ui_base_get_p2_selected_plane_id(void);
void ui_base_set_p1_selected_plane_id(plane_id_t id);
void ui_base_set_p2_selected_plane_id(plane_id_t id);
bool ui_base_plane_is_unlocked(plane_id_t plane_id);
void ui_base_plane_unlock(plane_id_t plane_id);
int  ui_base_get_unlocked_mask(void);
void ui_base_set_unlocked_mask(int mask);

#endif // #ifndef __UI_BASE_H__
