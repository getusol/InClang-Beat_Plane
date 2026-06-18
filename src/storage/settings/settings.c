/**
 * @file settings.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "settings.h"
#include "config.h"
#include <stddef.h>
#include <string.h>
#include "debug.h"

#ifdef SIMULATOR
#include <stdio.h>
#else
#include "ff.h"
#endif

/**********************
 *      MACROS
 **********************/

#define SETTINGS_FILE_NAME "settings.dat"

// 工具宏
#define SETTING_FMT "%18[^.].%18[^:]:%d" // 与config.h中一致
// eg: "%15[^.].%15[^:]:%d"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static int find_by_key(const char *module, const char *name);
static const char *get_file_path();

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static setting_t *settings[SETTINGS_MAX] = {NULL};
static int settings_count = 0;
static char file_path[64] = {0};
static bool settings_changed = false; // 是否有配置项被修改 决定是否需要保存

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 注册一个配置项 让模块统一管理
 * @param setting 配置项指针
 * @note 内容有效性由调用方负责 并且设置一定要严格为 bool 或 int !
 */
void settings_register(setting_t *setting)
{
    if (setting == NULL)
        return;
    if (settings_count >= SETTINGS_MAX)
    {
        CONSOLE_WARNING("Max settings count reached.");
        LOG_WARNING("Max settings count reached.");
        return;
    }

    // 数据检查
    if (setting->type >= ST_TYPE_MAX)
    {
        CONSOLE_WARNING("Unknown setting type %d.Failed to register setting %s.%s.", setting->type, setting->module, setting->name);
        LOG_WARNING("Unknown setting type %d.Failed to register setting %s.%s.", setting->type, setting->module, setting->name);
        return;
    }

    // data指针非NULL检验(防止解引用空指针)
    if (setting->data == NULL)
    {
        CONSOLE_WARNING("Setting data pointer is NULL.Failed to register setting %s.%s.", setting->module, setting->name);
        LOG_WARNING("Setting data pointer is NULL.Failed to register setting %s.%s.", setting->module, setting->name);
        return;
    }

    // 检查是否已注册相同模块和名称的配置项
    int index = find_by_key(setting->module, setting->name);
    if (index != -1)
    {
        CONSOLE_WARNING("Setting %s.%s already registered as type:%d.Failed to register setting %s.%s,type:%d.", setting->module, setting->name, settings[index]->type, setting->module, setting->name, setting->type);
        LOG_WARNING("Setting %s.%s already registered as type:%d.Failed to register setting %s.%s,type:%d.", setting->module, setting->name, settings[index]->type, setting->module, setting->name, setting->type);
        return;
    }

    settings[settings_count++] = setting;
}

// void settings_init();

/**
 * @brief 保存所有配置项到文件
 * @note 更新所有配置项的值到文件
 */
void settings_save()
{
    if (settings_count == 0)
        return;
    if (!settings_changed)
        return;

    // 保存配置项到文件
    CONSOLE_INFO("Settings saved to %s", get_file_path()); // 此时file_path已初始化 且不会改变

#ifdef SIMULATOR
    FILE *fp = fopen(file_path, "w");
    if (!fp)
    {
        CONSOLE_WARNING("Failed to open file %s for writing.", file_path);
        LOG_WARNING("Failed to open file %s for writing.", file_path);
        return;
    }

    for (int i = 0; i < settings_count; i++)
    {
        switch (settings[i]->type)
        {
        case ST_BOOL:
            fprintf(fp, "%s.%s:%d\n", settings[i]->module, settings[i]->name, *(bool *)settings[i]->data);
            break;
        case ST_INT:
            fprintf(fp, "%s.%s:%d\n", settings[i]->module, settings[i]->name, *(int *)settings[i]->data);
            break;
        default:
            CONSOLE_WARNING("Unknown setting type %d.", settings[i]->type);
            LOG_WARNING("Unknown setting type %d.", settings[i]->type);
            break;
        }
    }
    fclose(fp);
#else
    FIL fs_file;
    FRESULT fr = f_open(&fs_file, file_path, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK)
    {
        CONSOLE_WARNING("Failed to open file %s for writing.Error code: %d", file_path, fr);
        LOG_WARNING("Failed to open file %s for writing.Error code: %d", file_path, fr);
        return;
    }
    char line[MAX_MODULE_NAME_LEN + MAX_SETTING_NAME_LEN + 32] = {0};
    for (int i = 0; i < settings_count; i++)
    {
        switch (settings[i]->type)
        {
        case ST_BOOL:
            snprintf(line, sizeof(line), "%s.%s:%d\n", settings[i]->module, settings[i]->name, *(bool *)settings[i]->data);
            f_puts(line, &fs_file);
            break;
        case ST_INT:
            snprintf(line, sizeof(line), "%s.%s:%d\n", settings[i]->module, settings[i]->name, *(int *)settings[i]->data);
            f_puts(line, &fs_file);
            break;
        default:
            CONSOLE_WARNING("Unknown setting type %d.", settings[i]->type);
            LOG_WARNING("Unknown setting type %d.", settings[i]->type);
            break;
        }
    }
    f_close(&fs_file);
#endif
    settings_changed = false;
    CONSOLE_INFO("Settings saved successfully.");
}

/**
 * @brief 从文件加载所有配置项
 * @note 从文件读取配置项值并更新到内存中,配置项必须已注册
 */
void settings_load()
{
    if (settings_count == 0)
        return;

    // 先应用默认值
    for (int i = 0; i < settings_count; i++)
    {
        switch (settings[i]->type)
        {
        case ST_BOOL:
            *(bool *)settings[i]->data = settings[i]->bool_data.def;
            break;
        case ST_INT:
            *(int *)settings[i]->data = settings[i]->int_data.def;
            break;
        default:
            CONSOLE_WARNING("Unknown setting type %d. Failed to load default value for setting:%s.%s.", settings[i]->type, settings[i]->module, settings[i]->name);
            LOG_WARNING("Unknown setting type %d. Failed to load default value for setting:%s.%s.", settings[i]->type, settings[i]->module, settings[i]->name);
            break;
        }
    }

    // 文件读入内存
    char line[MAX_MODULE_NAME_LEN + MAX_SETTING_NAME_LEN + 32] = {0};
    char module[MAX_MODULE_NAME_LEN + 1] = {0};
    char name[MAX_SETTING_NAME_LEN + 1] = {0};
    int value = 0;
    bool success = false;

    get_file_path(); // 设置了file_path

#ifdef SIMULATOR
    FILE *fp = fopen(file_path, "r");
    if (!fp)
    {
        CONSOLE_WARNING("Failed to open file %s for reading.", file_path);
        LOG_WARNING("Failed to open file %s for reading.", file_path);
        return;
    }

    while (fgets(line, sizeof(line), fp))
    {
        line[strcspn(line, "\r\n")] = '\0'; // 去掉换行符
        if (sscanf(line, SETTING_FMT, module, name, &value) != 3)
        {
            CONSOLE_WARNING("Invalid setting format: %s", line);
            LOG_WARNING("Invalid setting format: %s", line);
            continue;
        }
        success = settings_set(module, name, value);
        if (!success)
        {
            CONSOLE_WARNING("Failed to set setting:%s.%s. Value:%d", module, name, value);
            LOG_WARNING("Failed to set setting:%s.%s. Value:%d", module, name, value);
            continue;
        }
    }
    fclose(fp);
#else
    FIL fs_file;
    FRESULT fr = f_open(&fs_file, file_path, FA_READ);
    if (fr != FR_OK)
    {
        CONSOLE_WARNING("Failed to open file %s for reading.Error code: %d", file_path, fr);
        LOG_WARNING("Failed to open file %s for reading.Error code: %d", file_path, fr);
        return;
    }

    while (f_gets(line, sizeof(line), &fs_file))
    {
        line[strcspn(line, "\r\n")] = '\0'; // 去掉换行符
        if (sscanf(line, SETTING_FMT, module, name, &value) != 3)
        {
            CONSOLE_WARNING("Invalid setting format: %s", line);
            LOG_WARNING("Invalid setting format: %s", line);
            continue;
        }
        success = settings_set(module, name, value);
        if (!success)
        {
            CONSOLE_WARNING("Failed to set setting:%s.%s. Value:%d", module, name, value);
            LOG_WARNING("Failed to set setting:%s.%s. Value:%d", module, name, value);
            continue;
        }
    }

    f_close(&fs_file);

#endif
    CONSOLE_INFO("Settings loaded successfully.");
}

void settings_reset()
{
    if (settings_count == 0)
        return;
    // 重置所有配置项为默认值
    for (int i = 0; i < settings_count; i++)
    {
        switch (settings[i]->type)
        {
        case ST_BOOL:
            *(bool *)settings[i]->data = settings[i]->bool_data.def;
            break;
        case ST_INT:
            *(int *)settings[i]->data = settings[i]->int_data.def;
            break;
        default:
            CONSOLE_WARNING("Unknown setting type %d. Failed to reset default value for setting:%s.%s.", settings[i]->type, settings[i]->module, settings[i]->name);
            LOG_WARNING("Unknown setting type %d. Failed to reset default value for setting:%s.%s.", settings[i]->type, settings[i]->module, settings[i]->name);
            break;
        }
    }
    settings_changed = true;
    settings_save();
    CONSOLE_INFO("Settings reset successfully.");
}

/**
 * @brief 根据索引获取配置项指针
 * @param index 配置项索引
 * @return const setting_t* 配置项指针 NULL: 未注册
 * @note 仅用于UI生成，不建议直接调用
 */
const setting_t *settings_get_setting(int index)
{
    if (index < 0 || index >= settings_count)
        return NULL;
    return settings[index];
}

/**
 * @brief 获取已注册的配置项数量
 * @return int 配置项数量
 */
int settings_get_setting_count()
{
    return settings_count;
}

/**
 * @brief 设置配置项值
 * @param module 模块名
 * @param name 配置项名
 * @param value 值 可以为整型、布尔型
 * @return true 成功
 * @return false 失败
 * @note 仅支持布尔型、整型配置
 */
bool settings_set(const char *module, const char *name, int value)
{
    int index = find_by_key(module, name);
    if (index < 0)
        return false;
    switch (settings[index]->type)
    {
    case ST_BOOL:
    {
        bool *b_ptr = (bool *)settings[index]->data;
        if (b_ptr == NULL)
            return false;
        *b_ptr = (value != 0);
        break;
    }
    case ST_INT:
    {
        int *i_ptr = (int *)settings[index]->data;
        if (i_ptr == NULL)
            return false;

        if (value < settings[index]->int_data.min)
            value = settings[index]->int_data.min;
        if (value > settings[index]->int_data.max)
            value = settings[index]->int_data.max;

        *i_ptr = value;
        break;
    }
    default:
    {
        CONSOLE_WARNING("Unknown setting type %d.", settings[index]->type);
        LOG_WARNING("Unknown setting type %d.", settings[index]->type);
        return false;
        break;
    }
    }
    settings_changed = true;
    return true;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 通过键值查找配置项索引
 * @return int 配置项索引 -1: 未找到
 */
static int find_by_key(const char *module, const char *name)
{
    static int last_index = 0;
    last_index = last_index >= settings_count ? 0 : last_index;
    for (int i = last_index; i < settings_count; i++)
    {
        if (strcmp(settings[i]->module, module) == 0 && strcmp(settings[i]->name, name) == 0)
        {
            last_index = i + 1; // 找到后将last_index指向下一个配置项 下一次查找时直接从last_index开始查找
            return i;
        }
    }
    for (int i = 0; i < last_index; i++)
    {
        if (strcmp(settings[i]->module, module) == 0 && strcmp(settings[i]->name, name) == 0)
        {
            last_index = i + 1; // 找到后将last_index指向下一个配置项 下一次查找时直接从last_index开始查找
            return i;
        }
    }
    return -1;
}

/**
 * @brief 获取配置文件路径
 * @return const char* 配置文件路径
 * @note 之后可以根据注册用户不同而返回不同的路径
 */
static const char *get_file_path()
{
#ifdef SIMULATOR
    snprintf(file_path, sizeof(file_path), "./data/%s", SETTINGS_FILE_NAME);
    return file_path;
#else
    snprintf(file_path, sizeof(file_path), "0:/data/%s", SETTINGS_FILE_NAME);
    return file_path;
#endif
}
