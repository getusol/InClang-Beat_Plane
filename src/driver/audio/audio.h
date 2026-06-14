/**
 * @file audio.h
 */

#ifndef __AUDIO_H__
#define __AUDIO_H__

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>
#include <stdbool.h>

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

// 枚举量 记录的是音频文件的ID
typedef enum audio_id_t
{
    AUDIO_CG,
    AUDIO_BGM,
    AUDIO_FAH,
    AUDIO_TROPICAL,
    AUDIO_BASKETBALLMUSIC,
    AUDIO_SHOPMUSIC,
    AUDIO_BASEMUSIC,
    AUDIO_ENEMYHIT,
    AUDIO_ENEMYDIE,
    AUDIO_BOSSDIE,
    AUDIO_MOUSEOPEN,
    AUDIO_MOUSECLOSE,
    AUDIO_BOSSATTACK,
    AUDIO_PLAYERFIRE,
    AUDIO_ENEMYATTACK,
    AUDIO_PLAYERFIREP2,
    AUDIO_COINPICKED,
    AUDIO_SKILLSHIELD,

    AUDIO_MAX,
} audio_id_t;

typedef enum
{
    AUDIO_CHAN_AUTO = -1, // 自动寻找空闲通道（适用于大多数瞬时音效）
    AUDIO_CHAN_BGM = 0,   // 背景音乐专属通道
    AUDIO_CHAN_SFX1 = 1,  // 显式控制音效通道 1
    AUDIO_CHAN_SFX2 = 2,  // 显式控制音效通道 2
    AUDIO_CHAN_SFX3 = 3,  // 显式控制音效通道 3
    // note: 目前代码写死了3个sfx 要添加需要修改相应代码

    AUDIO_CHAN_MAX = 4 // 最大通道数（可根据 MCU 的 RAM 大小调大或调小）
} audio_channel_id_t;

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void audio_init();
void audio_load(audio_id_t id, audio_channel_id_t channel_id, bool do_repeat);
void audio_stop(audio_channel_id_t channel_id);
void audio_stop_all();
void audio_pause(audio_channel_id_t channel_id);
void audio_resume(audio_channel_id_t channel_id);
void audio_pause_all();
void audio_resume_all();

#endif // #ifndef __AUDIO_H__
