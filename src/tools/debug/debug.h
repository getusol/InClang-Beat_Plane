/**
 * @file debug.h
 * @brief 调试工具，日志输出
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>
#include <string.h>
#include <stdio.h>

/**********************
 *      MACROS
 **********************/

#define HALT_LOG_CNT 10

// 辅助宏：提取文件名（去掉路径）
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : \
                     (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

/**
 * @brief 控制台输出宏，格式: [级别] 函数名: 消息 (in 文件名 line 行号)
 *        参数 fmt 应以 [级别] 开头，例如 CONSOLE("[INFO] Hello");
 *        有换行
 */
#define CONSOLE(fmt, ...) do {                                                      \
    char __msg_buf[256];                                                            \
    char __out_buf[512];                                                            \
    snprintf(__msg_buf, sizeof(__msg_buf), fmt, ##__VA_ARGS__);                     \
    char *__space = strchr(__msg_buf, ' ');                                         \
    if (__space) {                                                                  \
        *__space = '\0';                                                            \
        snprintf(__out_buf, sizeof(__out_buf), "%s %s: %s (in %s line %d)",         \
                 __msg_buf, __func__, __space + 1, __FILENAME__, __LINE__);         \
        *__space = ' '; /* restore */                                               \
    } else {                                                                        \
        snprintf(__out_buf, sizeof(__out_buf), "%s %s: (in %s line %d)",            \
                 __msg_buf, __func__, __FILENAME__, __LINE__);                      \
    }                                                                               \
    console_out("%s\n", __out_buf);                                                   \
} while(0)

/**
 * @brief 日志输出宏（写入文件），格式同 CONSOLE 有换行
 */
#define LOG(fmt, ...) do {                                                          \
    char __msg_buf[256];                                                            \
    char __out_buf[512];                                                            \
    snprintf(__msg_buf, sizeof(__msg_buf), fmt, ##__VA_ARGS__);                     \
    char *__space = strchr(__msg_buf, ' ');                                         \
    if (__space) {                                                                  \
        *__space = '\0';                                                            \
        snprintf(__out_buf, sizeof(__out_buf), "%s %s: %s (in %s line %d)",         \
                 __msg_buf, __func__, __space + 1, __FILENAME__, __LINE__);         \
        *__space = ' ';                                                             \
    } else {                                                                        \
        snprintf(__out_buf, sizeof(__out_buf), "%s %s: (in %s line %d)",            \
                 __msg_buf, __func__, __FILENAME__, __LINE__);                      \
    }                                                                               \
    log_out("%s", __out_buf);                                                       \
} while(0)

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void console_out(const char * fmt,...);
void log_out(const char * fmt,...);
void debug_init(void);
void debug_reset(void);
void sys_halt(void);
const char* debug_get_halt_log(uint8_t idx);

#endif // #ifndef __DEBUG_H__
