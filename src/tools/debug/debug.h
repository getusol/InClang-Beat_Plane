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
#include "config.h"

/**********************
 *      MACROS
 **********************/

#define HALT_LOG_CNT 10

// 辅助宏：提取文件名（去掉路径）
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : \
                     (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

/**
 * @brief 内部控制台输出辅助宏
 *        输出格式: [LEVEL] function_name: message (in filename line N)
 */
#define _CONSOLE_OUT(level, fmt, ...) do {                                          \
    char __msg_buf[256];                                                            \
    char __out_buf[512];                                                            \
    snprintf(__msg_buf, sizeof(__msg_buf), fmt, ##__VA_ARGS__);                     \
    snprintf(__out_buf, sizeof(__out_buf), "[%s] %s: %s (in %s line %d)",           \
             level, __func__, __msg_buf, __FILENAME__, __LINE__);                   \
    console_out("%s\n", __out_buf);                                                   \
} while(0)

/**
 * @brief 内部日志输出辅助宏（写入文件）
 *        输出格式同 _CONSOLE_OUT
 */
#define _LOG_OUT(level, fmt, ...) do {                                              \
    char __msg_buf[256];                                                            \
    char __out_buf[512];                                                            \
    snprintf(__msg_buf, sizeof(__msg_buf), fmt, ##__VA_ARGS__);                     \
    snprintf(__out_buf, sizeof(__out_buf), "[%s] %s: %s (in %s line %d)",           \
             level, __func__, __msg_buf, __FILENAME__, __LINE__);                   \
    log_out("%s", __out_buf);                                                         \
} while(0)

/*===========================================================================
 *  Per-level CONSOLE macros (compile-time gated by config.h)
 *===========================================================================*/
#if CONSOLE_ENABLE

    #if CONSOLE_DEBUG_ENABLE
        #define CONSOLE_DEBUG(fmt, ...)    _CONSOLE_OUT("DEBUG", fmt, ##__VA_ARGS__)
    #else
        #define CONSOLE_DEBUG(fmt, ...)    do {} while(0)
    #endif

    #if CONSOLE_INFO_ENABLE
        #define CONSOLE_INFO(fmt, ...)     _CONSOLE_OUT("INFO", fmt, ##__VA_ARGS__)
    #else
        #define CONSOLE_INFO(fmt, ...)     do {} while(0)
    #endif

    #if CONSOLE_WARNING_ENABLE
        #define CONSOLE_WARNING(fmt, ...)  _CONSOLE_OUT("WARNING", fmt, ##__VA_ARGS__)
    #else
        #define CONSOLE_WARNING(fmt, ...)  do {} while(0)
    #endif

    #if CONSOLE_ERROR_ENABLE
        #define CONSOLE_ERROR(fmt, ...)    _CONSOLE_OUT("ERROR", fmt, ##__VA_ARGS__)
    #else
        #define CONSOLE_ERROR(fmt, ...)    do {} while(0)
    #endif

#else /* CONSOLE_ENABLE == 0 */

    #define CONSOLE_DEBUG(fmt, ...)    do {} while(0)
    #define CONSOLE_INFO(fmt, ...)     do {} while(0)
    #define CONSOLE_WARNING(fmt, ...)  do {} while(0)
    #define CONSOLE_ERROR(fmt, ...)    do {} while(0)

#endif /* CONSOLE_ENABLE */

/*===========================================================================
 *  Per-level LOG macros (compile-time gated by config.h)
 *===========================================================================*/
#if LOG_ENABLE

    #define LOG_DEBUG(fmt, ...)      _LOG_OUT("DEBUG", fmt, ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...)       _LOG_OUT("INFO", fmt, ##__VA_ARGS__)
    #define LOG_WARNING(fmt, ...)    _LOG_OUT("WARNING", fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...)      _LOG_OUT("ERROR", fmt, ##__VA_ARGS__)

#else /* LOG_ENABLE == 0 */

    #define LOG_DEBUG(fmt, ...)      do {} while(0)
    #define LOG_INFO(fmt, ...)       do {} while(0)
    #define LOG_WARNING(fmt, ...)    do {} while(0)
    #define LOG_ERROR(fmt, ...)      do {} while(0)

#endif /* LOG_ENABLE */

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
