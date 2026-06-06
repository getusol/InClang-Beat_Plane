/**
 * @file audio.c
 * @note 非重复音频（音效）播放一次后内存不会释放 但是会被后来的音效覆盖
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
#define READ_SAMPLE(ptr, idx) (int16_t)((ptr)[idx] | ((ptr)[idx+1] << 8))

// SFX数量
#define SFX_CNT (AUDIO_CHAN_MAX - AUDIO_CHAN_SFX1)

// 音量分配策略 此时：2 ^ AUDIO_BUDGET = AUDIO_ALLOC_BGM + SFX_CNT * AUDIO_ALLOC_SFX
#define AUDIO_BUDGET 9 // 总和的移位 (bgm + sfx) = 2 ^ .. 9->512
                       // 此为基准值 可以通过 vol_amp 修改
#define AUDIO_ALLOC_BGM 200 // bgm所得
#define AUDIO_ALLOC_SFX 104 // sfx所得

#define VOL_MAX 8   // 2 ^ VOL_MAX = 256，用于 mix_wavg 移位归一化 此为基准值

/**********************
 *      TYPEDEFS
 **********************/

// 音频资源结构体，包含路径和大小
typedef struct {
    const char * path;
    uint32_t size;
} audio_asset_t;

// 音频通道结构体
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
static int16_t mix_wavg(int16_t bgm, int16_t* sfx, uint8_t sfx_cnt);
static int16_t read_sample(audio_channel_id_t channel);
static void audio_play_on_channel(uint8_t channel_id, const char * path, uint32_t size, bool do_repeat);
static int find_idle_sfx_channel();

// 新增：音频控制锁，防止主线程和硬件中断/SDL播放线程同时读写通道内存导致崩溃
static void audio_lock(void);
static void audio_unlock(void);

#ifdef SIMULATOR
static void sdl_audio_callback(void *userdata, Uint8 *stream, int len);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

// 音频资源列表
static const audio_asset_t audio_assets[AUDIO_MAX] = {
    [AUDIO_CG] = { .path = AUDIO_PATH("cg.pcm"), .size = 1798144 },
    [AUDIO_BGM] = { .path = AUDIO_PATH("bgm.pcm"), .size = 12996608 },
    [AUDIO_FAH] = {.path = AUDIO_PATH("johnnybacon156fah.pcm"), .size = 154368},
    [AUDIO_TROPICAL] = {.path = AUDIO_PATH("jonasblakewoodtropical.pcm"), .size = 1108224},
    [AUDIO_BASKETBALLMUSIC] = {.path = AUDIO_PATH("basketballmusic.pcm"),.size = 3121920},
    [AUDIO_SHOPMUSIC] = {.path = AUDIO_PATH("PixelGameShopMusic.pcm"), .size = 2651904},
    [AUDIO_BASEMUSIC] = {.path = AUDIO_PATH("SoftBaseAmbient.pcm"), .size = 32693762 },
    [AUDIO_ENEMYHIT] = {.path = AUDIO_PATH("enemy_hurted.pcm"), .size = 10342},
    [AUDIO_BOSSDIE] = {.path = AUDIO_PATH("enemy_die.pcm"), .size = 88200},
    [AUDIO_ENEMYDIE] = {.path = AUDIO_PATH("boss_killed.pcm"), .size = 83968},
    [AUDIO_MOUSEOPEN] = {.path = AUDIO_PATH("Menu_Open.pcm"), .size = 13762},
    [AUDIO_MOUSECLOSE] = {.path = AUDIO_PATH("Menu_Close.pcm"), .size = 12042},
    [AUDIO_BOSSATTACK] = {.path = AUDIO_PATH("boss_attack.pcm"), .size = 39136},
    [AUDIO_PLAYERFIRE] = {.path = AUDIO_PATH("player_normal_fire.pcm"), .size = 20228},
    [AUDIO_ENEMYATTACK] = {.path = AUDIO_PATH("enemy_attack.pcm"), .size = 18752 },
    [AUDIO_PLAYERFIREP2] = {.path = AUDIO_PATH("player_fire_p2.pcm"), .size = 50666},
    [AUDIO_COINPICKED] = {.path = AUDIO_PATH("coin_picked.pcm"), .size = 67712},
    [AUDIO_SKILLSHIELD] = {.path = AUDIO_PATH("Skill_shield.pcm"), .size = 132300},
};

// 音频频道实例
static audio_channel_t audio_channels[AUDIO_CHAN_MAX] = {0};

// 运行时音量 (0-255, 默认最大)
static uint8_t vol_bgm = 255;
static uint8_t vol_sfx = 255;

static uint8_t vol_amp = 1; // 0 +1(0.5) 1 +0(1.0) 2 -1(2.0) 3 -2(4.0)

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 音频系统初始化
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
 */
void audio_load(audio_id_t id, audio_channel_id_t channel_id, bool do_repeat)
{
    if (id >= AUDIO_MAX) {
        CONSOLE_WARNING("Invalid audio ID: %d", id);
        LOG_WARNING("Invalid audio ID: %d", id);
        return;
    }
    int target_id = channel_id;
    if (target_id < -1 || target_id >= AUDIO_CHAN_MAX) {
        CONSOLE_WARNING("Invalid channel ID: %d", target_id);
        LOG_WARNING("Invalid channel ID: %d", target_id);
        return;
    }
    if (target_id == AUDIO_CHAN_AUTO) {
        target_id = find_idle_sfx_channel();
        if (target_id == -1) {
            CONSOLE_WARNING("No idle SFX channel found, discarding request for audio ID %d", id);
            LOG_WARNING("No idle SFX channel found, discarding request for audio ID %d", id);
            return;
        }
    }
    audio_play_on_channel(target_id, audio_assets[id].path, audio_assets[id].size, do_repeat);
}

/**
 * @brief 停止某个通道的音频
 */
void audio_stop(audio_channel_id_t channel_id)
{
    if (channel_id >= AUDIO_CHAN_MAX) return;

    // 1. 进入临界区，快速切断通道并清空指针
    audio_lock();
    audio_channel_t *chan = &audio_channels[channel_id];
    chan->is_active = false;
    chan->size = 0;
    chan->play_index = 0;

    uint8_t * old_data = chan->data;
    chan->data = NULL;
    audio_unlock();

    // 2. 在临界区外安全释放内存
    if (old_data != NULL) {
        ram_free(old_data);
    }
}

/**
 * @brief 停止所有的音频通道
 */
void audio_stop_all(void)
{
    for (int i = AUDIO_CHAN_BGM; i < AUDIO_CHAN_MAX; i++) {
        audio_stop((audio_channel_id_t)i);
    }
}

/**
 * @brief 暂停一个音频通道
 */
void audio_pause(audio_channel_id_t channel_id)
{
    if (channel_id >= AUDIO_CHAN_MAX) return;
    audio_lock();
    audio_channels[channel_id].is_active = false;
    audio_unlock();
}

/**
 * @brief 恢复一个暂停的音频通道
 */
void audio_resume(audio_channel_id_t channel_id)
{
    if (channel_id >= AUDIO_CHAN_MAX) return;
    audio_lock();
    if (audio_channels[channel_id].data != NULL && audio_channels[channel_id].size > 0) {
        audio_channels[channel_id].is_active = true;
    }
    audio_unlock();
}

/**
 * @brief 暂停全部音频
 */
void audio_pause_all()
{
    for (int i = AUDIO_CHAN_BGM; i < AUDIO_CHAN_MAX; i++) {
        audio_pause((audio_channel_id_t)i);
    }
}

/**
 * @brief 恢复全部音频
 */
void audio_resume_all()
{
    for (int i = AUDIO_CHAN_BGM; i < AUDIO_CHAN_MAX; i++) {
        audio_resume((audio_channel_id_t)i);
    }
}

/**
 * @brief 设置bgm音量
 */
void audio_set_vol_bgm(uint8_t vol)
{
    if (vol > 255) {
        vol_bgm = 255;
        return ;
    }
    vol_bgm = vol;
}

/**
 * @brief 获取bgm音量
 */
uint8_t audio_get_vol_bgm(void)
{
    return vol_bgm;
}

/**
 * @brief 设置音效音量
 */
void audio_set_vol_sfx(uint8_t vol)
{
    if (vol > 255) {
        vol_sfx = 255;
        return ;
    }
    vol_sfx = vol;
}

/**
 * @brief 获取音效音量
 */
uint8_t audio_get_vol_sfx(void)
{
    return vol_sfx;
}

/**
 * @brief 设置音量放大系数
 */
void audio_set_vol_amp(uint8_t vol)
{
    if (vol > 3) vol_amp = 3;
    else vol_amp = vol;
    return ;
}

/**
 * @brief 获取音量放大系数
 */
uint8_t audio_get_vol_amp(void)
{
    return vol_amp;
}

#ifndef SIMULATOR
void SPI1_IRQHandler(void)
{
    static int is_right = 0;
    static int16_t last_sample = 0;
    if(SET != spi_i2s_interrupt_flag_get(SPI1, SPI_I2S_INT_FLAG_TP)) return ;
    is_right = 1 - is_right;
    if (is_right == 0) {
        spi_i2s_data_transmit(SPI1, last_sample);
        return ;
    }
    // 多个通道安全读取并混音输出
    int16_t bgm_sample = read_sample(AUDIO_CHAN_BGM);
    int16_t sfx_sample[SFX_CNT] = {0};
    for (int i = 0; i < SFX_CNT; i++) {
        sfx_sample[i] = read_sample(AUDIO_CHAN_SFX1 + i);
    }
    int16_t mixed_sample = mix_wavg(bgm_sample, sfx_sample, SFX_CNT);
    spi_i2s_data_transmit(SPI1, mixed_sample);
    last_sample = mixed_sample;
}
#endif

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 音频互斥锁定（进入临界区）
 */
static void audio_lock(void)
{
#ifdef SIMULATOR
    SDL_LockAudio();
#else
    // 硬件环境：关闭 SPI1 中断，防止 ISR 在主线程释放/分配内存时强行访问
    spi_i2s_interrupt_disable(SPI1, SPI_I2S_INT_TP);
#endif
}

/**
 * @brief 音频互斥解锁（退出临界区）
 */
static void audio_unlock(void)
{
#ifdef SIMULATOR
    SDL_UnlockAudio();
#else
    // 硬件环境：重新开启 SPI1 中断
    spi_i2s_interrupt_enable(SPI1, SPI_I2S_INT_TP);
#endif
}

/**
 * @brief 在指定的音频通道上播放音频文件
 *        （使用了快速交换设计：耗时的 IO 与 Malloc 在锁外执行，锁内仅交换指针，极致降低延迟且彻底解决崩溃问题）
 */
static void audio_play_on_channel(uint8_t channel_id, const char * path, uint32_t size, bool do_repeat)
{
    if (channel_id >= AUDIO_CHAN_MAX) return;

    // 1. 锁外申请新内存并加载文件（I/O 耗时，绝不能放在锁内，否则会导致音频播放产生卡顿）
    uint8_t * new_data = (uint8_t *)ram_malloc(size);
    if (new_data == NULL) {
        CONSOLE_WARNING("ram_malloc failed for channel %d: %s", channel_id, path);
        LOG_WARNING("ram_malloc failed for channel %d: %s", channel_id, path);
        return;
    }
    read_file_to_array(path, new_data, size);

    // 2. 锁定音频（进入临界区），仅做极快的通道数据与参数交换（耗时 < 1微秒）
    audio_lock();

    audio_channel_t *chan = &audio_channels[channel_id];
    uint8_t * old_data = chan->data; // 暂存旧缓冲区指针

    chan->is_active = false;
    chan->data = new_data;
    chan->play_index = 0;
    chan->size = size;
    chan->do_repeat = do_repeat;
    chan->is_active = true; // 激活

    audio_unlock();

    // 3. 在临界区外部安全释放旧通道数据内存
    if (old_data != NULL) {
        ram_free(old_data);
    }
}

/**
 * @brief 软件混音方案
 *        修复：将移位结果正确存回 sum 变量。并引入饱和斩波保护（Clipping Protection）
 */
static int16_t mix_wavg(int16_t bgm, int16_t* sfx, uint8_t sfx_cnt)
{
    int32_t sum = 0;
    sum += (int32_t)vol_bgm * AUDIO_ALLOC_BGM * bgm;
    for (int i = 0; i < sfx_cnt; i++) {
        sum += (int32_t)vol_sfx * AUDIO_ALLOC_SFX * sfx[i];
    }

    // 修复 1：必须将移位运算结果赋值回 sum！
    sum >>= (AUDIO_BUDGET - vol_amp + 1 + VOL_MAX);

    // 修复 2：增加饱和截断保护（防止多通道重叠时产生的数值溢出导致啸叫与噪声）
    if (sum > 32767)  sum = 32767;
    if (sum < -32768) sum = -32768;

    return (int16_t)(sum);
}

/**
 * @brief 读取某个通道上的采样值
 */
static int16_t read_sample(audio_channel_id_t channel)
{
    if (!audio_channels[channel].is_active) return 0;

    if (audio_channels[channel].data == NULL || audio_channels[channel].size == 0) {
        audio_channels[channel].is_active = false;
        return 0;
    }

    if (audio_channels[channel].play_index + 1 >= audio_channels[channel].size) {
        if (audio_channels[channel].do_repeat) {
            audio_channels[channel].play_index = 0;
        } else {
            audio_channels[channel].is_active = false;
            return 0;
        }
    }

    int16_t sample = READ_SAMPLE(audio_channels[channel].data, audio_channels[channel].play_index);
    audio_channels[channel].play_index += 2;

    if (audio_channels[channel].play_index >= audio_channels[channel].size) {
        if (audio_channels[channel].do_repeat) {
            audio_channels[channel].play_index = 0;
        } else {
            audio_channels[channel].is_active = false;
        }
    }
    return sample;
}

/**
 * @brief 查找一个空闲的音效通道
 */
static int find_idle_sfx_channel()
{
    for (int i = AUDIO_CHAN_SFX1; i < AUDIO_CHAN_MAX; i++) {
        if (!audio_channels[i].is_active) return i;
    }
    return -1;
}

/**
 * @brief I2S 配置函数
 */
static void i2s_config(void)
{
#ifdef SIMULATOR
    if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        CONSOLE_WARNING("SDL_Init Audio Failed: %s", SDL_GetError());
        LOG_WARNING("SDL_Init Audio Failed: %s", SDL_GetError());
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
    int sample_count = len / sizeof(int16_t);
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
