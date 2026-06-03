/**
 * @file audio.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "audio.h"
#include <stdint.h>
#include "lvgl_utils.h" // read_file_to_array
#include "tools.h"
#include <string.h>
#include "config.h"

#ifdef SIMULATOR
    #include "SDL2/SDL.h"
    #include <stdlib.h>
#else
    #include "drivers.h" 
    #include "gd32h7xx_adc.h" 
    #include "lv_port_disp_template.h"
    #include "lv_port_indev_template.h"
#endif

/**********************
 *      MACROS
 **********************/


// 处理音频路径 使用宏替换
#ifdef SIMULATOR
    #define AUDIO_ROOT_DIR "./assets/audios/"
#else
    #define AUDIO_ROOT_DIR "0:/assets/audios/"
#endif

#define AUDIO_PATH(filename) AUDIO_ROOT_DIR filename

// 音频读取宏函数，快速读取16位数据
// too dangerous dont use it.instead use func read_sample()
#define READ_SAMPLE(ptr, idx) (int16_t)((ptr)[idx] | ((ptr)[idx+1] << 8))

// SFX数量
#define SFX_CNT (AUDIO_CHAN_MAX - AUDIO_CHAN_SFX1)

// 音量分配策略 此时：2 ^ AUDIO_BUDGET = AUDIO_ALLOC_BGM + SFX_CNT * AUDIO_ALLOC_SFX
#define AUDIO_BUDGET 9 // 总和的移位 (bgm + sfx) = 2 ^ .. 9->512
#define AUDIO_ALLOC_BGM 200 // bgm所得
#define AUDIO_ALLOC_SFX 104 // sfx所得

/**********************
 *      TYPEDEFS
 **********************/

// 音频资源结构体，包含路径和大小
typedef struct {
    const char * path;
    uint32_t size;
} audio_asset_t;

// 音频通道结构体，未来可以扩展为支持多通道同时播放
typedef struct {
    uint8_t * data;
    uint32_t size;
    uint32_t play_index;
    bool do_repeat;
    bool is_active;
} audio_channel_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void i2s_config(void);
static int16_t mix_wavg(int16_t bgm, int16_t* sfx,uint8_t sfx_cnt);
static int16_t read_sample(audio_channel_id_t channel);
static void audio_play_on_channel(uint8_t channel_id, const char * path, uint32_t size, bool do_repeat);
static int find_idle_sfx_channel();
#ifdef SIMULATOR
static void sdl_audio_callback(void *userdata, Uint8 *stream, int len);
#else

#endif

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

// 音频资源列表，方便管理和扩展
static const audio_asset_t audio_assets[AUDIO_MAX] = {
    [AUDIO_CG] = { .path = AUDIO_PATH("cg.pcm"), .size = 1798144 },
    [AUDIO_BGM] = { .path = AUDIO_PATH("bgm.pcm"), .size = 12996608 },

};

// 音频频道实例
static audio_channel_t audio_channels[AUDIO_CHAN_MAX] = {0};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 音频系统初始化
 *        配置 I2S 接口，准备好音频播放环境
 *        在 MCU 环境下启用 SPI 中断，在 PC 模拟器环境下初始化 SDL 音频子系统
 */
void audio_init()
{
    i2s_config();
#ifndef SIMULATOR
    // 只有在真实硬件（非模拟器）环境下，才开启 SPI 中断
    nvic_irq_enable(SPI1_IRQn, 0, 0);
#endif
}

/**
 * @brief 播放指定 ID 的音频资源
 *        根据传入的 ID 从资源列表中获取路径和大小，加载并播放对应的 PCM 音频文件
 * @param id 音频资源 ID
 * @param channel_id 音频通道 ID，-1 表示自动选择空闲通道
 * @param do_repeat 是否循环播放
 */
void audio_load(audio_id_t id,audio_channel_id_t channel_id,bool do_repeat)
{
    if (id < 0 || id >= AUDIO_MAX) {
        CONSOLE("[WARNING] Invalid audio ID: %d", id);
        LOG("[WARNING] Invalid audio ID: %d", id);
        return;
    }
    int target_id = channel_id;
    if (target_id < -1 || target_id >= AUDIO_CHAN_MAX) {
        CONSOLE("[WARNING] Invalid channel ID: %d", target_id);
        LOG("[WARNING] Invalid channel ID: %d", target_id);
        return;
    }
    if (target_id == AUDIO_CHAN_AUTO) {
        target_id = find_idle_sfx_channel();
        if (target_id == -1) {
            CONSOLE("[WARNING] No idle SFX channel found, discarding request for audio ID %d", id);
            LOG("[WARNING] No idle SFX channel found, discarding request for audio ID %d", id);
            return;
        }
    }
    audio_play_on_channel(target_id, audio_assets[id].path, audio_assets[id].size, do_repeat);
}

/**
 * @brief 停止某个频道的音频
 * @param channel_id 频道id
 */
void audio_stop(audio_channel_id_t channel_id)
{
    if (channel_id < 0 || channel_id >= AUDIO_CHAN_MAX) return;

    audio_channel_t *chan = &audio_channels[channel_id];
    chan->is_active = false;
    chan->size = 0;
    chan->play_index = 0;

    if (chan->data != NULL) {
        ram_free(chan->data);
        chan->data = NULL;
    }

}

/**
 * @brief 停止所有的音频通道
 */
void audio_stop_all(void)
{
    for (int i = AUDIO_CHAN_BGM; i < AUDIO_CHAN_MAX; i++) {
        audio_stop_channel((audio_channel_id_t)i);
    }
}


#ifndef SIMULATOR
void SPI1_IRQHandler(void)
{
    if(SET != spi_i2s_interrupt_flag_get(SPI1, SPI_I2S_INT_FLAG_TP)) return ;
    // 两个通道混音输出
    int16_t bgm_sample = read_sample(AUDIO_CHAN_BGM);
    int16_t sfx_sample[SFX_CNT] = {0};
    for (int i = 0;i < SFX_CNT;i++) {
        sfx_sample[i] = read_sample(AUDIO_CHAN_SFX1 + i);
    }
    int16_t mixed_sample = mix_wavg(bgm_sample, sfx_sample, SFX_CNT);
    spi_i2s_data_transmit(SPI1, mixed_sample);
}
#endif


/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 在指定的音频通道上播放音频文件
 *        先安全地停止当前播放的音频，然后根据传入的文件名和大小加载新的 PCM 音频数据，并重置播放指针开始播放
 *        在 PC 模拟器环境下使用 malloc 和 SDL 加载音频数据，在 MCU 环境下使用 SDRAM 分配和读取文件到内存
 */
static void audio_play_on_channel(uint8_t channel_id, const char * path, uint32_t size, bool do_repeat)
{
    if (channel_id < 0 || channel_id >= AUDIO_CHAN_MAX) return;

    audio_channel_t *chan = &audio_channels[channel_id];

    // 1. 安全保护：先关闭该通道，防止后台线程继续访问正在释放的内存
    chan->is_active = false; 
    chan->size = 0;

    // 2. 释放该通道旧的内存
    if (chan->data != NULL) {
        ram_free(chan->data);
        chan->data = NULL;
    }

    // 3. 申请新内存并加载文件
    chan->data = (uint8_t *)ram_malloc(size);
    if (chan->data == NULL) { // 如果失败
        CONSOLE("[WARNING] ram_malloc failed for channel %d: %s", channel_id, path);
        LOG("[WARNING] ram_malloc failed for channel %d: %s", channel_id, path);
        return;
    }

    read_file_to_array(path, chan->data, size);


    // 4. 重置通道状态并激活播放
    chan->play_index = 0;
    chan->size = size;
    chan->do_repeat = do_repeat;
    chan->is_active = true; // 激活后，驱动层会自动开始读取
}

/**
 * @brief 软件混音方案 目前适配2个通道 之后拓展再说
 *        out = (sample1 * weight1 + sample2 * weight2) / (weight1 + weight2)
 * @param sample1 通道1的音频样本值 bgm
 * @param sample2 通道2的音频样本值 sfx
 * @return 混音后的音频样本值
 */
static int16_t mix_wavg(int16_t bgm, int16_t* sfx,uint8_t sfx_cnt)
{
    int32_t sum = 0;
    sum += VOL_BGM * AUDIO_ALLOC_BGM * bgm;
    for (int i = 0; i < sfx_cnt; i++) {
        sum += VOL_SFX * AUDIO_ALLOC_SFX * sfx[i];
    }
    sum >> (AUDIO_BUDGET + VOL_MAX); //Here 8 is come from the maximum volume 255
    return (int16_t)(sum);
}

/**
 * @brief 读取某个通道上的采样值
 * @param channel 音频通道 ID
 * @return 该通道当前播放位置的采样值，单位为 16
 */
static int16_t read_sample(audio_channel_id_t channel)
{
    if (!audio_channels[channel].is_active) return 0; // 如果通道不活跃，返回静音
    // 安全检查:
    if (audio_channels[channel].data == NULL || audio_channels[channel].size == 0) {
        audio_channels[channel].is_active = false; // 停止播放
        return 0; // 返回静音
    }
    if (audio_channels[channel].play_index + 1 >= audio_channels[channel].size) { // 说明即将越界
        if (audio_channels[channel].do_repeat) {
            audio_channels[channel].play_index = 0; // 循环播放
        } else {
            audio_channels[channel].is_active = false; // 停止播放
            return 0; // 返回静音
        }
    }
    // 读取当前采样值，并更新播放指针
    int16_t sample = READ_SAMPLE(audio_channels[channel].data, audio_channels[channel].play_index);
    audio_channels[channel].play_index += 2; // 16 位采样占 2 字节
    if (audio_channels[channel].play_index >= audio_channels[channel].size) {
        if (audio_channels[channel].do_repeat) {
            audio_channels[channel].play_index = 0; // 循环播放
        } else {
            audio_channels[channel].is_active = false; // 停止播放
        }
    }
    return sample;
}

/**
 * @brief 查找一个空闲的音效通道
 * @return 音频通道 ID，若无 则丢弃请求
 */
static int find_idle_sfx_channel()
{
    for (int i = AUDIO_CHAN_SFX1; i < AUDIO_CHAN_MAX; i++) {
        if (!audio_channels[i].is_active) return i;
    }
    return -1; // 如果所有通道都活跃，返回 -1 (丢弃)
}

/**
 * @brief I2S 配置函数
 *        在 MCU 环境下配置 I2S 硬件接口，在 PC 模拟器环境下初始化 SDL 音频子系统
 */
static void i2s_config(void)
{
#ifdef SIMULATOR
    if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        CONSOLE("[WARNING] SDL_Init Audio Failed: %s\n", SDL_GetError());
        LOG("[WARNING] SDL_Init Audio Failed: %s", SDL_GetError());
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
#else
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
#endif
}

#ifdef SIMULATOR
/**
 * @brief SDL2 音频回调函数
 */
static void sdl_audio_callback(void *userdata, Uint8 *stream, int len)
{
    int sample_count = len / sizeof(int16_t); // 16 位采样占 2 字节
    int16_t * out = (int16_t *)stream;
    
    for (int i = 0; i < sample_count; i++) {
        int16_t bgm_sample = read_sample(AUDIO_CHAN_BGM);
        int16_t sfx_sample[SFX_CNT] = {0};
        for (int j = 0; j < SFX_CNT; j++) {
            sfx_sample[j] = read_sample(AUDIO_CHAN_SFX1 + j);
        }
        out[i] = mix_wavg(bgm_sample, sfx_sample, SFX_CNT);
    }
}
#endif
