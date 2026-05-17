/**
 * @file lvgl_utils.c
 * @brief LVGL 辅助工具实现
 */

/*********************
 *      INCLUDES
 *********************/
#include "lvgl_utils.h"
#include "tools.h"       // 需要 ram_malloc, delay_ms, console_out, log_out
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ui_key.h"

#ifdef SIMULATOR
#include <SDL.h>
#else
#include "ff.h"
#include "drivers.h"
#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 显示弹窗
 */
void popup_show(lv_obj_t *popup)
{
    lv_obj_clear_flag(popup, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(popup);
}

/**
 * @brief 隐藏弹窗
 */
void popup_hide(lv_obj_t *popup)
{
    lv_obj_add_flag(popup, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief 设置输入设备的目标组，并聚焦到组内对象
 */
void set_group(lv_group_t *g)
{
    lv_indev_set_group(key_get_indev(), g);

    lv_indev_t *indev = key_get_indev();
    if (indev != NULL) {
        lv_indev_reset(indev, NULL);
        lv_indev_wait_release(indev);
    }

    if (lv_group_get_obj_count(g) == 0) {
        return;
    }
    lv_group_focus_prev(g);
    lv_group_focus_next(g);
}

/**
 * @brief 获取图像文件路径
 * @param name 图像文件名
 * @param output_path 输出路径缓冲区
 * @param max_length 输出路径缓冲区大小
 * @return 输出路径字符串指针，失败返回NULL
 */
const char *img_path(const char *name, char *output_path, size_t max_length)
{
    if (max_length < 41 + strlen(name)) {
        console_out("[Warning][img_path] output_size is too less for img:%s.\n", name);
        log_out("[Warning][img_path] output_size is too less for img:%s.", name);
        return NULL;
    }
#ifdef SIMULATOR
    strcpy(output_path, "./assets/pics/");
#else
    strcpy(output_path, "0:/assets/pics/");
#endif
    strcat(output_path, name);
    return output_path;
}

/**
 * @brief 从文件读取数据到数组
 * @param filepath 文件路径
 * @param buffer 数据缓冲区
 * @param max_size 数据缓冲区最大大小
 * @return 读取的字节数，失败返回-1
 */
int read_file_to_array(const char *filepath, uint8_t *buffer, uint32_t max_size)
{
    if (filepath == NULL || buffer == NULL || max_size == 0) {
        console_out("[Warning][read_file_to_array] Invalid arguments:filepath=%p, buffer=%p, max_size=%lu\n", (void*)filepath, (void*)buffer, (unsigned long)max_size);
        log_out("[Warning][read_file_to_array] Invalid arguments:filepath=%p, buffer=%p, max_size=%lu", (void*)filepath, (void*)buffer, (unsigned long)max_size);
        memset(buffer, 0, max_size);
        return -1;
    }
#ifdef SIMULATOR
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        console_out("[Warning][read_file_to_array] Cant open: %s\n", filepath);
        log_out("[Warning][read_file_to_array] Cant open: %s", filepath);
        memset(buffer, 0, max_size);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    uint32_t file_size = ftell(file);
    rewind(file);

    if (file_size > max_size) {
        fclose(file);
        console_out("[Warning][read_file_to_array] File too large, need:%ld, given:%ld\n", file_size, max_size);
        log_out("[Warning][read_file_to_array] File too large, need:%ld, given:%ld", file_size, max_size);
        memset(buffer, 0, max_size);
        return -1;
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        fclose(file);
        console_out("[Warning][read_file_to_array] bytes_read dont suit file_size!,bytes_read:%ld,file_size:%ld\n", bytes_read, file_size);
        log_out("[Warning][read_file_to_array] bytes_read dont suit file_size!,bytes_read:%ld,file_size:%ld", bytes_read, file_size);
        memset(buffer, 0, max_size);
        return -1;
    }

    fclose(file);
    console_out("[read_file_to_array] File read successfully, size:%ld ,path:%s\n", bytes_read, filepath);
    return bytes_read;

#else
    FIL file;
    FRESULT res;
    UINT bytes_read;

    res = f_open(&file, filepath, FA_READ);
    if (res != FR_OK) {
        console_out("[Warning][read_file_to_array] Cant open: %s\n", filepath);
        log_out("[Warning][read_file_to_array] Cant open: %s", filepath);
        memset(buffer, 0, max_size);
        return -1;
    }

    FSIZE_t file_size = f_size(&file);
    if (file_size > max_size) {
        f_close(&file);
        console_out("[Warning][read_file_to_array] File too large, need:%ld, given:%ld\n", file_size, max_size);
        log_out("[Warning][read_file_to_array] File too large, need:%ld, given:%ld", file_size, max_size);
        memset(buffer, 0, max_size);
        return -1;
    }

    res = f_read(&file, buffer, file_size, &bytes_read);
    if (res != FR_OK || bytes_read != file_size) {
        f_close(&file);
        console_out("[Warning][read_file_to_array] Error reading file: %s\n", filepath);
        log_out("[Warning][read_file_to_array] Error reading file: %s", filepath);
        memset(buffer, 0, max_size);
        return -1;
    }

    f_close(&file);
    console_out("[read_file_to_array] File read successfully, size:%ld ,path:%s\n", bytes_read, filepath);
    return bytes_read;
#endif
}

/**
 * @brief 从描述符创建图像对象
 * @param parent 父对象
 * @param path 图像文件路径
 * @param w 图像宽度
 * @param h 图像高度
 * @param img_buf 图像数据缓冲区，如果为NULL则函数内部会分配内存
 * @param img_struct 图像描述结构体指针，如果为NULL则函数内部不会设置图像源
 * @param is_alpha 图像是否包含alpha通道
 * @return 创建的图像对象指针，失败返回无图片源的图像对象指针
 */
lv_obj_t *img_create_from_dsc(lv_obj_t *parent, const char *path,
                                lv_coord_t w, lv_coord_t h,
                                uint8_t *img_buf, lv_img_dsc_t *img_struct,
                                bool is_alpha)
{
    lv_obj_t * img = lv_img_create(parent);
    if (!img_struct) {
        CONSOLE("[WARNING] img_struct for %s is NULL.",path);
        LOG("[WARNING] img_struct for %s is NULL.",path);
        return img;
    }

    CONSOLE("[INFO] Start creating img from dsc.Path: %s, size: %d * %d, alpha: %d.",path,w,h,is_alpha);

    if (img_buf != NULL) {
        if (is_alpha) {
            if (read_file_to_array(path, img_buf, w * h * 3 + 4) < 0) {
                CONSOLE("[WARNING] Failed to read image file: %s", path);
                LOG("[WARNING] Failed to read image file: %s", path);
                return img;
            }

            img_struct->data = img_buf + 4;
            img_struct->header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
        } else {
            if (read_file_to_array(path, img_buf, w * h * 3) < 0) {
                CONSOLE("[WARNING] Failed to read image file: %s", path);
                LOG("[WARNING] Failed to read image file: %s", path);
                return img;
            }

            img_struct->data = img_buf;
            img_struct->header.cf = LV_IMG_CF_TRUE_COLOR;
        }

        img_struct->header.always_zero = 0;
        img_struct->header.w = w;
        img_struct->header.h = h;
        img_struct->header.reserved = 0;
        img_struct->data_size = w * h * 3;
    } else {
        if (load_img_dsc(path,img_struct,w,h,is_alpha) == NULL) {
            CONSOLE("[WARNING] Failed to load img %s to dsc.",path);
            LOG("[WARNING] Failed to load img %s to dsc.",path);
            return img;
        }
    }

    lv_img_set_src(img,img_struct);
    CONSOLE("[INFO] Img obj created: %s (%d * %d ,alpha = %d).",path,w,h,is_alpha);
    return img;
}

/**
 * @brief 从文件加载图像描述结构体
 * @param path 图像文件路径
 * @param dsc 图像描述结构体指针 由外部提供内存
 * @param w 图像宽度
 * @param h 图像高度
 * @param is_alpha 图像是否包含alpha通道
 * @return 图像描述结构体指针 失败返回 NULL
 */
lv_img_dsc_t * load_img_dsc(const char * path,lv_img_dsc_t * dsc,
                            lv_coord_t w,lv_coord_t h,
                            bool is_alpha)
{
    if (!dsc || !path) {
        CONSOLE("[WARNING] load_img_dsc failed,img_dsc is NULL, or path is NULL,path : %s",path ? path : "(null)");
        LOG("[WARNING] load_img_dsc failed,img_dsc is NULL or path is NULL,path : %s",path ? path : "(null)");
        return NULL;
    }

    uint8_t * img_buf = NULL;
    size_t data_bytes = 0;

    CONSOLE("[INFO] Start loading img: %s into dsc with w:%d,h:%d is_alpha:%d",path,w,h,is_alpha);

    if (is_alpha) {

        data_bytes = w * h * 3 + 4;
        img_buf = ram_malloc(data_bytes);
        
        if (!img_buf) {
            CONSOLE("[WARNING] load_img_dsc failed,ram_malloc failed,path : %s",path);
            LOG("[WARNING] load_img_dsc failed,ram_malloc failed,path : %s",path);
            return NULL;
        }

        if (read_file_to_array(path,img_buf,data_bytes) < 0) {
            CONSOLE("[WARNING] load_img_dsc failed,read_file_to_array failed,path : %s",path);
            LOG("[WARNING] load_img_dsc failed,read_file_to_array failed,path : %s",path);
            ram_free(img_buf);
            return NULL;
        }
        dsc->header.always_zero = 0;
        dsc->header.reserved    = 0;
        dsc->header.cf          = LV_IMG_CF_TRUE_COLOR_ALPHA;
        dsc->header.w           = w;
        dsc->header.h           = h;
        dsc->data_size          = w * h * 3;           /* 像素部分 */
        dsc->data               = img_buf + 4;
    } else {
        data_bytes = w * h * 3;
        img_buf = ram_malloc(data_bytes);
        if (!img_buf) {
            CONSOLE("[WARNING] load_img_dsc failed,ram_malloc failed,path : %s",path);
            LOG("[WARNING] load_img_dsc failed,ram_malloc failed,path : %s",path);
            return NULL;
        }
        if (read_file_to_array(path,img_buf,data_bytes) < 0) {
            CONSOLE("[WARNING] load_img_dsc failed,read_file_to_array failed,path : %s",path);
            LOG("[WARNING] load_img_dsc failed,read_file_to_array failed,path : %s",path);
            ram_free(img_buf);
            return NULL;
        }
        dsc->header.always_zero = 0;
        dsc->header.reserved    = 0;
        dsc->header.cf          = LV_IMG_CF_TRUE_COLOR;
        dsc->header.w           = w;
        dsc->header.h           = h;
        dsc->data_size          = data_bytes;
        dsc->data               = img_buf;                 /* 直接指向像素 */
    }

    CONSOLE("[INFO] Img descriptor loaded: %s (%d * %d ,alpha = %d).",path,w,h,is_alpha);

    return dsc;
}

/**
 * @brief 释放图像描述符
 * @param dsc 图像描述符
 */
void free_img_dsc(lv_img_dsc_t * dsc)
{
    if (!dsc || !dsc->data) return ;
    if (dsc->header.cf == LV_IMG_CF_TRUE_COLOR_ALPHA) {
        uint8_t * data = (uint8_t *)dsc->data - 4;
        ram_free(data);
        CONSOLE("[INFO] Free img dsc with alpha channel.");
    } else {
        ram_free((void *)dsc->data);
        CONSOLE("[INFO] Free img dsc without alpha channel.");
    }
    dsc->data = NULL;
    dsc->data_size = 0;
}
