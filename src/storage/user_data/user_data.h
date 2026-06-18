/**
 * @file user_data.h
 * @brief 用户数据持久化模块 — 存储金币、抽奖次数等游戏进度
 *
 * 与 settings 模块类似, 通过注册→自动遍历读写, 无需硬编码。
 * 存储在独立文件 (user_data.txt) 中, 与设置分离。
 */

#ifndef __USER_DATA_H__
#define __USER_DATA_H__

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>
#include <stdbool.h>

/**********************
 *      MACROS
 **********************/

#define USER_DATA_MAX 8           // 最大存储项数量
#define USER_DATA_NAME_LEN 18     // 项目名称最大长度 (实际 +1)
#define USER_DATA_MODULE_LEN 18   // 模块名最大长度

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    USER_DATA_INT,    // int
    USER_DATA_BOOL,   // bool
} user_data_type_t;

typedef struct {
    const char *module;
    const char *name;
    void *data;
    user_data_type_t type;
} user_data_entry_t;

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void user_data_register(const char *module, const char *name,
                        void *data, user_data_type_t type);
void user_data_load(void);
void user_data_save(void);
void user_data_reset(void);

#endif /* __USER_DATA_H__ */
