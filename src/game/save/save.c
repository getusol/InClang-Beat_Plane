/**
 * @file save.c
 */

/*********************
 * INCLUDES
 *********************/

#include "save.h"
#include "audio.h"
#include "coin.h"
#include "tools.h"
#include <stdio.h>
#include <string.h>

/**********************
 * MACROS
 **********************/

#ifdef SIMULATOR
#define SAVE_DIR     "./data/"
#define SAVE_PATH    "./data/save.data"
#define BACKUP_PATH  "./data/save.data.backup"
#else
#define SAVE_DIR     "0:/data/"
#define SAVE_PATH    "0:/data/save.data"
#define BACKUP_PATH  "0:/data/save.data.backup"
#endif

#define MAX_LINE_LEN 64

/**********************
 * STATIC PROTOTYPES
 **********************/

static int try_parse_file(const char * path);
static void apply_defaults(void);

/**********************
 * GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 从文件加载存档，失败时回退到 .backup 或默认值
 */
void save_load(void)
{
    // 1. 尝试读取主存档
    if (try_parse_file(SAVE_PATH) == 0) {
        CONSOLE_INFO("Save loaded from %s", SAVE_PATH);
        return;
    }

    // 2. 主存档失败，尝试备份
    CONSOLE_WARNING("Failed to load %s, trying backup...", SAVE_PATH);
    if (try_parse_file(BACKUP_PATH) == 0) {
        CONSOLE_INFO("Save loaded from backup %s", BACKUP_PATH);
        // 恢复主存档
        save_write();
        return;
    }

    // 3. 全部失败，使用默认值
    CONSOLE_WARNING("Failed to load backup, using defaults.");
    apply_defaults();
    save_write();
}

/**
 * @brief 将当前状态写入存档（先备份旧文件）
 */
void save_write(void)
{
    // 备份旧存档（如果存在）
#ifdef SIMULATOR
    // PC: copy old file to backup
    FILE * src = fopen(SAVE_PATH, "rb");
    if (src) {
        FILE * dst = fopen(BACKUP_PATH, "wb");
        if (dst) {
            uint8_t buf[512];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
                fwrite(buf, 1, n, dst);
            }
            fclose(dst);
        }
        fclose(src);
    }
    // 写入新存档
    FILE * fp = fopen(SAVE_PATH, "w");
    if (!fp) {
        CONSOLE_WARNING("Cannot open %s for writing", SAVE_PATH);
        return;
    }
    fprintf(fp, "coin_num=%d\n", coin_get_num());
    fprintf(fp, "vol_bgm=%d\n", audio_get_vol_bgm());
    fprintf(fp, "vol_sfx=%d\n", audio_get_vol_sfx());
    fprintf(fp, "vol_amp=%d\n", audio_get_vol_amp());
    fclose(fp);
#else
    // MCU: FatFS
    FIL src, dst;
    if (f_open(&src, SAVE_PATH, FA_READ) == FR_OK) {
        if (f_open(&dst, BACKUP_PATH, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
            uint8_t buf[512];
            UINT n;
            while (f_read(&src, buf, sizeof(buf), &n) == FR_OK && n > 0) {
                f_write(&dst, buf, n, &n);
            }
            f_close(&dst);
        }
        f_close(&src);
    }
    // 写入新存档
    FIL fp;
    if (f_open(&fp, SAVE_PATH, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
        CONSOLE_WARNING("Cannot open %s for writing", SAVE_PATH);
        return;
    }
    char line[MAX_LINE_LEN];
    snprintf(line, sizeof(line), "coin_num=%d\n", coin_get_num()); f_puts(line, &fp);
    snprintf(line, sizeof(line), "vol_bgm=%d\n", audio_get_vol_bgm()); f_puts(line, &fp);
    snprintf(line, sizeof(line), "vol_sfx=%d\n", audio_get_vol_sfx()); f_puts(line, &fp);
    snprintf(line, sizeof(line), "vol_amp=%d\n", audio_get_vol_amp()); f_puts(line, &fp);
    f_close(&fp);
#endif
    CONSOLE_INFO("Save written to %s", SAVE_PATH);
}

/**********************
 * STATIC FUNCTIONS
 **********************/

/**
 * @brief 解析 key=value 文件并应用设置
 * @return 0 成功, -1 失败
 */
static int try_parse_file(const char * path)
{
#ifdef SIMULATOR
    FILE * fp = fopen(path, "r");
    if (!fp) return -1;

    char line[MAX_LINE_LEN];
    int coin_num = -1, vol_bgm = -1, vol_sfx = -1, vol_amp = -1;

    while (fgets(line, sizeof(line), fp)) {
        char key[32];
        int val;
        if (sscanf(line, "%31[^=]=%d", key, &val) == 2) {
            if (strcmp(key, "coin_num") == 0) coin_num = val;
            else if (strcmp(key, "vol_bgm") == 0) vol_bgm = val;
            else if (strcmp(key, "vol_sfx") == 0) vol_sfx = val;
            else if (strcmp(key, "vol_amp") == 0) vol_amp = val;
        }
    }
    fclose(fp);

    if (coin_num < 0 || vol_bgm < 0 || vol_sfx < 0 || vol_amp < 0) return -1;

    coin_set_num(coin_num);
    audio_set_vol_bgm((uint8_t)vol_bgm);
    audio_set_vol_sfx((uint8_t)vol_sfx);
    audio_set_vol_amp((uint8_t)vol_amp);
    return 0;
#else
    FIL fp;
    if (f_open(&fp, path, FA_READ) != FR_OK) return -1;

    char buf[MAX_LINE_LEN * 4 + 4]; // 4 lines max
    UINT bytes_read;
    f_read(&fp, buf, sizeof(buf) - 1, &bytes_read);
    buf[bytes_read] = '\0';
    f_close(&fp);

    int coin_num = -1, vol_bgm = -1, vol_sfx = -1, vol_amp = -1;
    char * line = strtok(buf, "\n");
    while (line) {
        char key[32];
        int val;
        if (sscanf(line, "%31[^=]=%d", key, &val) == 2) {
            if (strcmp(key, "coin_num") == 0) coin_num = val;
            else if (strcmp(key, "vol_bgm") == 0) vol_bgm = val;
            else if (strcmp(key, "vol_sfx") == 0) vol_sfx = val;
            else if (strcmp(key, "vol_amp") == 0) vol_amp = val;
        }
        line = strtok(NULL, "\n");
    }

    if (coin_num < 0 || vol_bgm < 0 || vol_sfx < 0 || vol_amp < 0) return -1;

    coin_set_num(coin_num);
    audio_set_vol_bgm((uint8_t)vol_bgm);
    audio_set_vol_sfx((uint8_t)vol_sfx);
    audio_set_vol_amp((uint8_t)vol_amp);
    return 0;
#endif
}

static void apply_defaults(void)
{
    coin_set_num(0);
    audio_set_vol_bgm(255);
    audio_set_vol_sfx(255);
    audio_set_vol_amp(1);
}
