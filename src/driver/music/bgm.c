#include "bgm.h"

/* =========================================
 * 头文件引用隔离
 * 根据当前平台（PC / 单片机）引用不同的库
 * ========================================= */
#ifdef SIMULATOR
    // PC 模拟器所需头文件
    #include "SDL2/SDL.h"
    #include <stdlib.h>
    #include <stdio.h>
#else
    // MCU 开发板所需硬件头文件
    #include "drivers.h" 
    #include "gd32h7xx_adc.h" 
    #include "lv_port_disp_template.h"
    #include "lv_port_indev_template.h"
#endif

// 共用的头文件
#include <stdio.h>
#include <string.h>
#include "lvgl.h"

/* =========================================
 * 静态全局变量
 * ========================================= */
static uint8_t * bgm_data = NULL;
static uint32_t bgm_size = 12996608;
static uint32_t play_index = 0;

/* =========================================
 * 【新增】通用切歌功能：完美兼容你的 PC 和 MCU 架构
 * ========================================= */
void audio_switch_track(const char * filename, uint32_t size)
{
    // 1. 安全保护：先将大小清零，让正在运行的音频中断/线程输出静音，防止切歌瞬间指针越界
    bgm_size = 0; 
    
#ifdef SIMULATOR
    // PC 模拟器端逻辑
    if (bgm_data != NULL) {
        free(bgm_data);
        bgm_data = NULL;
    }
    bgm_data = (uint8_t *)malloc(size);
    if(bgm_data != NULL) {
        char path_buf[64];
        sprintf(path_buf, "assets/music/%s", filename);
        FILE *fp = fopen(path_buf, "rb");
        if (fp != NULL) {
            fread(bgm_data, 1, size, fp);
            fclose(fp);
            printf("[Audio Info] Loaded track on PC: %s\n", path_buf);
        } else {
            printf("[Audio Error] Could not open %s on PC!\n", path_buf);
        }
    }
#else
    // MCU 单片机端逻辑
    // 如果你的驱动或内存管理支持释放 SDRAM，请在此处调用释放函数，例如：
    // if (bgm_data != NULL) { sdram_free(bgm_data); }
    
    bgm_data = (uint8_t *)sdram_malloc(size);
    if(bgm_data != NULL) {
        char path_buf[64];
        sprintf(path_buf, "0:/assets/music/%s", filename);
        read_file_to_array(path_buf, bgm_data, size);
    }
#endif

    // 2. 重置播放指针，重新注入大小，音频会立刻开始播放
    play_index = 0;
    bgm_size = size;
}


/* =========================================
 * PC 模拟器具体实现 (SIMULATOR)
 * ========================================= */
#ifdef SIMULATOR

/**
 * @brief SDL2 音频回调函数
 */
static void sdl_audio_callback(void *userdata, Uint8 *stream, int len)
{
    if(bgm_data != NULL && bgm_size > 0) {
        for(int i = 0; i < len; i++) {
            if(play_index >= bgm_size) {
                play_index = 0;
            }
            stream[i] = bgm_data[play_index++];
        }
    } else {
        memset(stream, 0, len);
    }
}

void i2s_config(void)
{
    if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        printf("SDL_Init Audio Failed: %s\n", SDL_GetError());
        return;
    }

    SDL_AudioSpec wanted_spec;
    wanted_spec.freq = 44100;           
    wanted_spec.format = AUDIO_S16SYS;  
    wanted_spec.channels = 1;           
    wanted_spec.silence = 0;
    wanted_spec.samples = 1024;         
    wanted_spec.callback = sdl_audio_callback;
    wanted_spec.userdata = NULL;

    if (SDL_OpenAudio(&wanted_spec, NULL) < 0) {
        printf("SDL_OpenAudio Failed: %s\n", SDL_GetError());
        return;
    }

    SDL_PauseAudio(0);
}

void music_bgm_load(void)
{
    // 复用通用切歌函数加载主BGM
    audio_switch_track("bgm.pcm", 12996608);
}


/* =========================================
 * MCU 开发板具体实现
 * ========================================= */
#else

void SPI1_IRQHandler(void)
{
    if(SET == spi_i2s_interrupt_flag_get(SPI1, SPI_I2S_INT_FLAG_TP)) {
        if(bgm_data != NULL && bgm_size > 0) {
            
            // 【注意】这里保留了你原版的 bgm_size*2 逻辑。
            // 提示：如果切歌后播放 CG.pcm 声音速度不对或者卡死，请把下面的 `bgm_size * 2` 改为 `bgm_size`
            if(play_index >= bgm_size * 2) {
                play_index = 0;
            }

            uint16_t sample = bgm_data[play_index] | (bgm_data[play_index + 1] << 8);
            spi_i2s_data_transmit(SPI1, sample);
            play_index += 2;
        } else {
            spi_i2s_data_transmit(SPI1, 0); 
        }
    }
}

void i2s_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_SPI1);
    rcu_spi_clock_config(IDX_SPI1, RCU_SPISRC_PLL0Q);

    gpio_af_set(GPIOB, GPIO_AF_5, GPIO_PIN_12 | GPIO_PIN_13);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_12 | GPIO_PIN_13);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_12 | GPIO_PIN_13);
    gpio_af_set(GPIOC, GPIO_AF_5, GPIO_PIN_1);
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_1);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_1);

    spi_i2s_deinit(SPI1);
    i2s_psc_config(SPI1, I2S_AUDIOSAMPLE_44K, I2S_FRAMEFORMAT_DT16B_CH16B, I2S_MCKOUT_DISABLE);
    i2s_init(SPI1, I2S_MODE_MASTERTX, I2S_STD_PHILIPS, I2S_CKPL_LOW);
    
    i2s_enable(SPI1);
    spi_master_transfer_start(SPI1, SPI_TRANS_START);
    spi_i2s_interrupt_enable(SPI1, SPI_I2S_INT_TP);
}

void music_bgm_load(void)
{
    // 复用通用切歌函数加载主BGM
    audio_switch_track("bgm.pcm", 12996608);
}

#endif