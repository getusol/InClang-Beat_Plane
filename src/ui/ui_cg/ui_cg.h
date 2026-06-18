/**
 * @file ui_cg.h
 */

#ifndef __UI_CG_H__
#define __UI_CG_H__

/*********************
 *      INCLUDES
 *********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void ui_cg_init_stage1(void);
void ui_cg_init_stage2(void);
void ui_cg_run(void);
void ui_cg_skip(void);
void ui_cg_cleanup(void); // 新增：供 ui_run 调用的安全释放接口

#endif // #ifndef __UI_CG_H__
