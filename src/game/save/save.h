/**
 * @file save.h
 * @brief 存档系统：持久化 coin_num 与音频设置到 data/save.data
 */

#ifndef __SAVE_H__
#define __SAVE_H__

/*********************
 *      INCLUDES
 *********************/

/**********************
 *      MACROS
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void save_load(void);
void save_write(void);
void save_clear(void);

#endif // #ifndef __SAVE_H__
