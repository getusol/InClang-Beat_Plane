/**
 * @file debug.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "debug.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "fsm.h"

#ifdef SIMULATOR
#include <stdlib.h>
#else
#include "ff.h"
#endif

/**********************
 *      MACROS
 **********************/
#ifdef SIMULATOR
#define LOG_PATH "./logs/run.log"
#define BOOT_COUNT_PATH "./logs/boot_count.txt"
#else
#define LOG_PATH "0:/logs/run.log"
#define BOOT_COUNT_PATH "0:/logs/boot_count.txt"
#endif //#ifdef SIMULATOR

/**********************
 *      TYPEDEFS
 **********************/

 /**********************
  *  STATIC PROTOTYPES
  **********************/

static void update_boot_count(void);
static void push_halt_log(const char *str);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static int g_boot_count = 0;
static char halt_log_buf[HALT_LOG_CNT][64] = {0};
static uint8_t halt_log_idx = 0;

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 控制台输出，将信息打印到控制台,无自动换行
 */
void console_out(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printf("%s", buf);
}
/**
 * @brief 日志输出，输出到特定目录的日志下
 * @note 自动换行
 */
void log_out(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    push_halt_log(buf);

#ifdef SIMULATOR
    FILE* fp = fopen(LOG_PATH, "a+");
    if(fp){
        fputs(buf, fp);
        fputs("\n", fp);
        fclose(fp);
    }
#else
    FIL file;
    if(f_open(&file, LOG_PATH, FA_OPEN_ALWAYS | FA_WRITE) == FR_OK){
        f_lseek(&file, f_size(&file));
        f_puts(buf, &file);
        f_puts("\r\n", &file);
        f_close(&file);
    }
    else {
        console_out("[Error][debug] Failed opening %s\n",LOG_PATH);
    }
#endif
}
/**
 * @brief debug初始化函数
 */
void debug_init()
{
  update_boot_count();
  log_out("\n=====The %dth boot,log system loaded successfully!=====",g_boot_count);
}
/**
 * @brief debug重置函数 清空日志并且计数归0
 */
void debug_reset(void)
{
  console_out("[debug] Running debug_reset()...\n");
#ifdef SIMULATOR
    // 1. 清空日志文件
    FILE* fp_log = fopen(LOG_PATH, "w");
    if (fp_log) {
        fclose(fp_log);
        console_out("[debug] Log cleared!Path: %s\n",LOG_PATH);
    }
    else {
        console_out("[Error][debug] Failed opening %s\n",LOG_PATH);
    }
    // 2. 计数归零
    FILE* fp_boot = fopen(BOOT_COUNT_PATH, "w");
    if (fp_boot) {
        fprintf(fp_boot, "0");
        fclose(fp_boot);
        console_out("[debug] Boot count cleared!Path: %s\n",BOOT_COUNT_PATH);
    }
    else {
        console_out("[Error][debug] Failed opening %s\n",BOOT_COUNT_PATH);
    }
#else
    // 1. 清空日志文件
    FIL file_log;
    f_open(&file_log, LOG_PATH, FA_CREATE_ALWAYS | FA_WRITE);
    f_close(&file_log);
    console_out("[debug] Log cleared!Path: %s\n",LOG_PATH);

    // 2. 计数归零
    FIL file_boot;
    if (f_open(&file_boot, BOOT_COUNT_PATH, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
        f_puts("0", &file_boot);
        f_close(&file_boot);
        console_out("[debug] Boot count cleared!Path: %s\n",BOOT_COUNT_PATH);
    }
    else {
        console_out("[Error][debug] Failed opening %s\n",BOOT_COUNT_PATH);
    }
#endif
}
/**
 * @brief 系统停机函数 跳转到停机页面
 */
void sys_halt(void)
{
    fsm_switch_state(SYS_HALT);
}
/**
 * @brief 读取缓存的日志（近10条）
 * @param idx 日志缓存序号 0 ~ 9
 */
const char* debug_get_halt_log(uint8_t idx)
{
    if(idx >= HALT_LOG_CNT) return "";
    return halt_log_buf[idx];
}
 /**********************
 *   STATIC FUNCTIONS
 **********************/
/**
 * @brief 更新启动次数计数，便于日志系统纪录第几次启动
 */
static void update_boot_count()
{
#ifdef SIMULATOR
    // PC端读取计数
    FILE *fp = fopen(BOOT_COUNT_PATH, "r");
    if (fp) {
        fscanf(fp, "%d", &g_boot_count);
        fclose(fp);
    }
    else {
        console_out("[Error][debug] Cannot open file: %s\n",BOOT_COUNT_PATH);
    }
    g_boot_count++;

    fp = fopen(BOOT_COUNT_PATH, "w");
    if (fp) {
        fprintf(fp, "%d", g_boot_count);
        fclose(fp);
    }
    else {
        console_out("[Error][debug] Cannot open file: %s\n",BOOT_COUNT_PATH);
    }
#else
    // 单片机端读取计数
    FIL file;
    UINT br;
    char buf[8];

    if (f_open(&file, BOOT_COUNT_PATH, FA_READ) == FR_OK) {
        f_read(&file, buf, 7, &br);
        g_boot_count = atoi(buf);
        f_close(&file);
    }
    else {
        console_out("[Error][debug] Cannot open file: %s\n",BOOT_COUNT_PATH);
    }
    g_boot_count++;

    if (f_open(&file, BOOT_COUNT_PATH, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
        sprintf(buf, "%d", g_boot_count);
        f_puts(buf, &file);
        f_close(&file);
    }
    else {
        console_out("[Error][debug] Cannot open file: %s\n",BOOT_COUNT_PATH);
    }
#endif
}
/**
 * @brief 将日志推送到halt_log_buf中待使用
 */
static void push_halt_log(const char *str)
{
    memset(halt_log_buf[halt_log_idx], 0, 64);
    strncpy(halt_log_buf[halt_log_idx], str, 63);
    halt_log_idx = (halt_log_idx + 1) % HALT_LOG_CNT;
}
