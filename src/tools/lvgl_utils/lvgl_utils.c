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

#ifdef EMBED_ASSETS
#include "embedded_assets.h"
#endif

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
        CONSOLE_WARNING("output_size is too less for img:%s.", name);
        LOG_WARNING("output_size is too less for img:%s.", name);
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
        CONSOLE_WARNING("Invalid arguments:filepath=%p, buffer=%p, max_size=%lu", (void*)filepath, (void*)buffer, (unsigned long)max_size);
        LOG_WARNING("Invalid arguments:filepath=%p, buffer=%p, max_size=%lu", (void*)filepath, (void*)buffer, (unsigned long)max_size);
        memset(buffer, 0, max_size);
        return -1;
    }
#ifdef EMBED_ASSETS
    // 嵌入资源优先：如果文件已编译进exe则直接拷贝，否则回退到磁盘读取
    {
        uint32_t emb_size = 0;
        const uint8_t *emb_data = embedded_asset_find(filepath, &emb_size);
        if (emb_data != NULL) {
            if (emb_size > max_size) {
                CONSOLE_WARNING("Embedded too large: %s", filepath);
                LOG_WARNING("Embedded too large: %s", filepath);
                memset(buffer, 0, max_size);
                return -1;
            }
            memcpy(buffer, emb_data, emb_size);
            return (int)emb_size;
        }
    }
#endif
#ifdef SIMULATOR
   FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        CONSOLE_WARNING("Cant open: %s", filepath);
        LOG_WARNING("Cant open: %s", filepath);
        memset(buffer, 0, max_size);
        return -1;
    }

    // 1. 检查 fseek 是否成功移动到了文件末尾
    if (fseek(file, 0, SEEK_END) != 0) {
        CONSOLE_WARNING("fseek to SEEK_END failed for path: %s", filepath);
        LOG_WARNING("fseek to SEEK_END failed for path: %s", filepath);
        fclose(file);
        memset(buffer, 0, max_size);
        return -1;
    }

    // 2. 检查 ftell 是否出错 (出错时返回 -1L)
    long raw_file_size = ftell(file);
    if (raw_file_size == -1L) {
        CONSOLE_WARNING("ftell failed (returned -1) for path: %s", filepath);
        LOG_WARNING("ftell failed (returned -1) for path: %s", filepath);
        fclose(file);
        memset(buffer, 0, max_size);
        return -1;
    }

    // 打印原始获取到的文件大小，排查是否在这里就已经是 0 了
    //CONSOLE_DEBUG("Raw file size from ftell: %ld bytes", raw_file_size);

    uint32_t file_size = (uint32_t)raw_file_size;

    // 3. 放弃 rewind，改用 fseek 回到头部并检查返回值
    if (fseek(file, 0, SEEK_SET) != 0) {
        CONSOLE_WARNING("fseek back to SEEK_SET failed for path: %s", filepath);
        LOG_WARNING("fseek back to SEEK_SET failed for path: %s", filepath);
        fclose(file);
        memset(buffer, 0, max_size);
        return -1;
    }

    // 4. 单独拦截 0 字节文件，防止后续 fread 产生不可预知的行为
    if (file_size == 0) {
        CONSOLE_INFO("File is legitimately empty (0 bytes): %s", filepath);
        fclose(file);
        return 0; // 空文件读取 0 字节，属于正常业务逻辑，不需要清空 buffer 返回 -1
    }

    if (file_size > max_size) {
        fclose(file);
        // 注意：file_size 已经是 uint32_t，格式化字符应使用 %u
        CONSOLE_WARNING("File too large, need:%u, given:%u", file_size, max_size);
        LOG_WARNING("File too large, need:%u, given:%u", file_size, max_size);
        memset(buffer, 0, max_size);
        return -1;
    }

    // 5. 核心：诊断 fread 失败的确切原因
    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        int err_num = ferror(file); // 检查是否发生 I/O 错误
        int is_eof = feof(file);    // 检查是否意外提前触碰到了文件末尾 EOF
        
        fclose(file);
        CONSOLE_WARNING("bytes_read dont suit file_size!,bytes_read:%ld,file_size:%ld", bytes_read, file_size);
        LOG_WARNING("bytes_read dont suit file_size!,bytes_read:%ld,file_size:%ld", bytes_read, file_size);
        memset(buffer, 0, max_size);
        return -1;
    }

    fclose(file);
    //CONSOLE_INFO("File read successfully, size:%ld ,path:%s", bytes_read, filepath);
    return bytes_read;

#else
    FIL file;
    FRESULT res;
    UINT bytes_read;

    res = f_open(&file, filepath, FA_READ);
    if (res != FR_OK) {
        CONSOLE_WARNING("Cant open: %s", filepath);
        LOG_WARNING("Cant open: %s", filepath);
        memset(buffer, 0, max_size);
        return -1;
    }

    FSIZE_t file_size = f_size(&file);
    if (file_size > max_size) {
        f_close(&file);
        CONSOLE_WARNING("File too large, need:%lu, given:%lu", (unsigned long)file_size, (unsigned long)max_size);
        LOG_WARNING("File too large, need:%lu, given:%lu", (unsigned long)file_size, (unsigned long)max_size);
        memset(buffer, 0, max_size);
        return -1;
    }

    res = f_read(&file, buffer, file_size, &bytes_read);
    if (res != FR_OK || bytes_read != file_size) {
        f_close(&file);
        CONSOLE_WARNING("Error reading file: %s", filepath);
        LOG_WARNING("Error reading file: %s", filepath);
        memset(buffer, 0, max_size);
        return -1;
    }

    f_close(&file);
    //CONSOLE_INFO("File read successfully, size:%ld ,path:%s", bytes_read, filepath);
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
        CONSOLE_WARNING("img_struct for %s is NULL.",path);
        LOG_WARNING("img_struct for %s is NULL.",path);
        return img;
    }

    CONSOLE_INFO("Start creating img from dsc.Path: %s, size: %d * %d, alpha: %d.",path,w,h,is_alpha);

    if (img_buf != NULL) {
        if (is_alpha) {
            if (read_file_to_array(path, img_buf, w * h * 3 + 4) < 0) {
                CONSOLE_WARNING("Failed to read image file: %s", path);
                LOG_WARNING("Failed to read image file: %s", path);
                return img;
            }

            img_struct->data = img_buf + 4;
            img_struct->header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
        } else {
            if (read_file_to_array(path, img_buf, w * h * 3) < 0) {
                CONSOLE_WARNING("Failed to read image file: %s", path);
                LOG_WARNING("Failed to read image file: %s", path);
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
            CONSOLE_WARNING("Failed to load img %s to dsc.",path);
            LOG_WARNING("Failed to load img %s to dsc.",path);
            return img;
        }
    }

    lv_img_set_src(img,img_struct);
    CONSOLE_INFO("Img obj created: %s (%d * %d ,alpha = %d).",path,w,h,is_alpha);
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
        CONSOLE_WARNING("load_img_dsc failed,img_dsc is NULL, or path is NULL,path : %s",path ? path : "(null)");
        LOG_WARNING("load_img_dsc failed,img_dsc is NULL or path is NULL,path : %s",path ? path : "(null)");
        return NULL;
    }

    uint8_t * img_buf = NULL;
    size_t data_bytes = 0;

    CONSOLE_INFO("Start loading img: %s into dsc with w:%d,h:%d is_alpha:%d",path,w,h,is_alpha);

    if (is_alpha) {

        data_bytes = w * h * 3 + 4;
        img_buf = ram_malloc(data_bytes);

        if (!img_buf) {
            CONSOLE_WARNING("load_img_dsc failed,ram_malloc failed,path : %s",path);
            LOG_WARNING("load_img_dsc failed,ram_malloc failed,path : %s",path);
            return NULL;
        }

        if (read_file_to_array(path,img_buf,data_bytes) < 0) {
            CONSOLE_WARNING("load_img_dsc failed,read_file_to_array failed,path : %s",path);
            LOG_WARNING("load_img_dsc failed,read_file_to_array failed,path : %s",path);
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
            CONSOLE_WARNING("load_img_dsc failed,ram_malloc failed,path : %s",path);
            LOG_WARNING("load_img_dsc failed,ram_malloc failed,path : %s",path);
            return NULL;
        }
        if (read_file_to_array(path,img_buf,data_bytes) < 0) {
            CONSOLE_WARNING("load_img_dsc failed,read_file_to_array failed,path : %s",path);
            LOG_WARNING("load_img_dsc failed,read_file_to_array failed,path : %s",path);
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

    CONSOLE_INFO("Img descriptor loaded: %s (%d * %d ,alpha = %d).",path,w,h,is_alpha);

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
        CONSOLE_INFO("Free img dsc with alpha channel.");
    } else {
        ram_free((void *)dsc->data);
        CONSOLE_INFO("Free img dsc without alpha channel.");
    }
    dsc->data = NULL;
    dsc->data_size = 0;
}
