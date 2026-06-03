#ifndef MUSIC_H
#define MUSIC_H

#include "stdint.h"

/**
 * @brief ??? I2S ????
 */
void i2s_config(void);

/**
 * @brief 
 * @param path 
 */
void music_bgm_load(void);
// 动态加载并播放指定的 PCM 音频文件
void audio_switch_track(const char * filename, uint32_t size);
int read_file_to_array(const char* filename, uint8_t* buffer, uint32_t max_size);
#endif