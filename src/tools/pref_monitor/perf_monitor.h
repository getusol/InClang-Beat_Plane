/**
 * @file pref_monitor.h
 */

#ifndef __PREF_MONITOR_H__
#define __PREF_MONITOR_H__

/*********************
 *      INCLUDES
 *********************/

#include "lvgl.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void perf_monitor_init(lv_obj_t * parent);
void perf_monitor_set_mspt(uint32_t mspt);
void perf_monitor_set_mspf(uint32_t mspf);
void perf_monitor_update(void);



#endif // #ifndef __PREF_MONITOR_H__
