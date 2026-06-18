/**
 * @file user_data.c
 * @brief 用户数据持久化实现 (PC + MCU)
 *
 * 文件格式: module.name:value (每行一项)
 * 文件路径: <exe_dir>/user_data.txt
 */

/*********************
 *      INCLUDES
 *********************/
#include "user_data.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#ifdef SIMULATOR
#else
#include "ff.h" /* FatFS */
#endif

/**********************
 *      MACROS
 **********************/

#define FMT_ENTRY "%18[^.].%18[^:]:%d"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static const char *get_file_path(void);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static user_data_entry_t *entries[USER_DATA_MAX];
static int entry_count = 0;
static char s_file_path[64] = {0};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void user_data_register(const char *module, const char *name,
                        void *data, user_data_type_t type)
{
    if (entry_count >= USER_DATA_MAX || module == NULL || name == NULL || data == NULL)
    {
        CONSOLE_WARNING("user_data_register: invalid params or full");
        return;
    }
    static user_data_entry_t pool[USER_DATA_MAX];
    pool[entry_count].module = module;
    pool[entry_count].name = name;
    pool[entry_count].data = data;
    pool[entry_count].type = type;
    entries[entry_count] = &pool[entry_count];
    entry_count++;
}

void user_data_load(void)
{
    if (entry_count == 0)
        return;

    const char *path = get_file_path();
    char line[USER_DATA_MODULE_LEN + USER_DATA_NAME_LEN + 32] = {0};
    char module[USER_DATA_MODULE_LEN + 1] = {0};
    char name[USER_DATA_NAME_LEN + 1] = {0};
    int value = 0;

#ifdef SIMULATOR
    FILE *fp = fopen(path, "r");
    if (!fp)
        return;

    while (fgets(line, sizeof(line), fp))
    {
        line[strcspn(line, "\r\n")] = '\0';
        int n = sscanf(line, FMT_ENTRY, module, name, &value);
        if (n != 3)
        {
            CONSOLE_WARNING("user_data: invalid (n=%d fmt=%s): [%s]", n, FMT_ENTRY, line);
            continue;
        }
        for (int i = 0; i < entry_count; i++)
        {
            if (strcmp(entries[i]->module, module) == 0 &&
                strcmp(entries[i]->name, name) == 0)
            {
                switch (entries[i]->type)
                {
                case USER_DATA_INT:
                    *(int *)entries[i]->data = value;
                    break;
                case USER_DATA_BOOL:
                    *(bool *)entries[i]->data = (value != 0);
                    break;
                }
                break;
            }
        }
    }
    fclose(fp);
#else
    FIL fs_file;
    if (f_open(&fs_file, path, FA_READ) != FR_OK)
        return;

    while (f_gets(line, sizeof(line), &fs_file))
    {
        line[strcspn(line, "\r\n")] = '\0';
        int n = sscanf(line, FMT_ENTRY, module, name, &value);
        if (n != 3)
        {
            CONSOLE_WARNING("user_data: invalid (n=%d fmt=%s): [%s]", n, FMT_ENTRY, line);
            continue;
        }
        for (int i = 0; i < entry_count; i++)
        {
            if (strcmp(entries[i]->module, module) == 0 &&
                strcmp(entries[i]->name, name) == 0)
            {
                switch (entries[i]->type)
                {
                case USER_DATA_INT:
                    *(int *)entries[i]->data = value;
                    break;
                case USER_DATA_BOOL:
                    *(bool *)entries[i]->data = (value != 0);
                    break;
                }
                break;
            }
        }
    }
    f_close(&fs_file);
#endif
}

void user_data_save(void)
{
    if (entry_count == 0)
        return;

    const char *path = get_file_path();
    char line[USER_DATA_MODULE_LEN + USER_DATA_NAME_LEN + 32];

#ifdef SIMULATOR
    FILE *fp = fopen(path, "w");
    if (!fp)
    {
        CONSOLE_WARNING("user_data_save: cannot open %s", path);
        LOG_WARNING("user_data_save: cannot open %s", path);
        return;
    }

    for (int i = 0; i < entry_count; i++)
    {
        int value = 0;
        switch (entries[i]->type)
        {
        case USER_DATA_INT:
            value = *(int *)entries[i]->data;
            break;
        case USER_DATA_BOOL:
            value = *(bool *)entries[i]->data ? 1 : 0;
            break;
        }
        fprintf(fp, "%s.%s:%d\n", entries[i]->module, entries[i]->name, value);
    }
    fclose(fp);
#else
    FIL fs_file;
    if (f_open(&fs_file, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        CONSOLE_WARNING("user_data_save: cannot open %s", path);
        LOG_WARNING("user_data_save: cannot open %s", path);
        return;
    }
    for (int i = 0; i < entry_count; i++)
    {
        int value = 0;
        switch (entries[i]->type)
        {
        case USER_DATA_INT:
            value = *(int *)entries[i]->data;
            break;
        case USER_DATA_BOOL:
            value = *(bool *)entries[i]->data ? 1 : 0;
            break;
        }
        snprintf(line, sizeof(line), "%s.%s:%d\n", entries[i]->module, entries[i]->name, value);
        f_puts(line, &fs_file);
    }
    f_close(&fs_file);
#endif
}

void user_data_reset(void)
{
    for (int i = 0; i < entry_count; i++)
    {
        switch (entries[i]->type)
        {
        case USER_DATA_INT:
            *(int *)entries[i]->data = 0;
            break;
        case USER_DATA_BOOL:
            *(bool *)entries[i]->data = false;
            break;
        }
    }
    user_data_save();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static const char *get_file_path(void)
{
    if (s_file_path[0] == '\0')
    {
#ifdef SIMULATOR
        snprintf(s_file_path, sizeof(s_file_path), "./data/user_data.txt");
#else
        snprintf(s_file_path, sizeof(s_file_path), "0:/data/user_data.txt");
#endif
    }
    return s_file_path;
}
