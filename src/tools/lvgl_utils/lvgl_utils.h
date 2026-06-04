/**
 * @file lvgl_utils.h
 * @brief LVGL 辅助工具函数：图像加载、弹窗、组操作等
 */

#ifndef __LVGL_UTILS_H__
#define __LVGL_UTILS_H__

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void popup_show(lv_obj_t *popup);
void popup_hide(lv_obj_t *popup);
void set_group(lv_group_t *g);

const char *img_path(const char *name, char *output_path, size_t max_length);
int read_file_to_array(const char *filepath, uint8_t *buffer, uint32_t max_size);

lv_obj_t *img_create_from_dsc(lv_obj_t *parent, const char *path,
                                lv_coord_t w, lv_coord_t h,
                                uint8_t *img_buf, lv_img_dsc_t *img_struct,
                                bool is_alpha);

lv_img_dsc_t * load_img_dsc(const char * path,lv_img_dsc_t * dsc,
                                lv_coord_t w,lv_coord_t h,
                                bool is_alpha);

void free_img_dsc(lv_img_dsc_t * dsc);

#endif // #ifndef __LVGL_UTILS_H__
