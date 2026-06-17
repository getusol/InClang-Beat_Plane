/**
 * @file settings.h
 */

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

/*********************
 *      INCLUDES
 *********************/

#include <stdbool.h>

/**********************
 *      MACROS
 **********************/

// 不建议太大 否则UI需要重新设置布局
#define MAX_MODULE_NAME_LEN 18  // (实际数组大小为这个宏+1)
#define MAX_SETTING_NAME_LEN 18 // (实际数组大小为这个宏+1)

/**********************
 *      TYPEDEFS
 **********************/

typedef enum
{
    ST_BOOL = 0,
    ST_INT = 1,

    ST_TYPE_MAX,
} setting_type_t; // 设置类型

typedef struct
{
    char module[MAX_MODULE_NAME_LEN + 1]; // 模块名
    char name[MAX_SETTING_NAME_LEN + 1];  // 配置项名称
    setting_type_t type;                  // 配置项类型
    void *data;                           // 配置项数据指针，根据类型不同指向不同的数据
    union
    {
        struct
        {
            bool def; // 默认值
        } bool_data;
        struct
        {
            int def; // 默认值
            int min; // 最小值
            int max; // 最大值
        } int_data;
    }; // 配置项默认值和范围限制
} setting_t; // 配置项结构体

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

// 后续需要根据登录玩家ID来确认写入哪个文件
// void settings_init();

void settings_save();
void settings_load();
void settings_reset();
void settings_register(setting_t *setting);

const setting_t *settings_get_setting(int index);
int settings_get_setting_count();

bool settings_set(const char *module, const char *name, int value);

#endif
