/**
 * @file joystick.h
 * @note joystick -> js
 */

/*********************
 *      INCLUDES
 *********************/
#include "joystick.h"
#include "tools.h"
#include <stdbool.h>
#include "config.h"
#include "comm_rx.h"
#include "comm_status.h"
#ifdef SIMULATOR
#include "SDL.h"
#else
#include "drivers.h"
#include "gd32h7xx_adc.h"
#endif
/**********************
 *      MACROS
 **********************/
#ifdef SIMULATOR

#else
#define JOY_ADC           ADC2
#define JOY_ADC_RCU       RCU_ADC2
#define JOY_GPIO_RCU      RCU_GPIOC
#define JOY_PORT          GPIOC
#define JOY_PIN_Y         GPIO_PIN_2    // PC2 -> ADC2_IN0
#define JOY_PIN_X         GPIO_PIN_3    // PC3 -> ADC2_IN1
#define JOY_CH_Y          ADC_CHANNEL_0
#define JOY_CH_X          ADC_CHANNEL_1
//摇杆消抖参数:
#define JOY_CENTER          2048        // ADC 中心值（12-bit ADC: 4096/2）
#endif // #ifdef SIMULATOR

/**********************
 *      TYPEDEFS
 **********************/
/**
 * @brief 摇杆处理后的 x y 值(-JOY_MAX_VALUE ~ JOY_MAX_VALUE)
 */
typedef struct {
    int16_t x;
    int16_t y;
} joystick_state_t;

#ifdef SIMULATOR
/**
 * @brief 摇杆X,Y，增强可读性
 */
enum {
    JS_POS_X = 0,
    JS_NEG_X,
    JS_POS_Y,
    JS_NEG_Y,
};

/**
 * @brief 摇杆对应的方向的键盘输入
 */
typedef SDL_Scancode js_dir_scancodes[JS_DIR_KEY_COUNT];

/**
 * @brief joystick统一处理结构
 * @param dir_keys 处理对应按键绑定
 */
typedef struct {
    joystick_state_t state;
    js_dir_scancodes * dir_keys;
} joystick_t;

/**
 * @brief pc上的摇杆读取函数
 * @param sdl_key sdl键盘状态数组
 * @param scancodes 摇杆方向绑定的按键
 */
typedef bool (*js_read_func)(const uint8_t * sdl_key,const js_dir_scancodes scancodes);
#else
/**
 * @brief MCU摇杆处理结构
 * @param x_raw x轴原始值
 * @param y_raw y轴原始值
 */
typedef struct {
    joystick_state_t state;
    uint16_t x_raw;
    uint16_t y_raw;
} joystick_t;
/**
 * @brief MCU上的摇杆读取函数 读取原始值
 * @param joystick 指针，修改原始值
 */
typedef void (*js_read_func)(joystick_t * joystick);
#endif

 /**********************
  *  STATIC PROTOTYPES
  **********************/
 static void joystick_process(joystick_t * joystick,js_read_func jrf,const uint8_t * ptr);
 #ifdef SIMULATOR
 static void pc_js_init();
 static bool pc_js_read(const uint8_t * sdl_key,const js_dir_scancodes scancodes);
 #else
 static void mcu_js_adc_init();
 static void hw_js_read(joystick_t * joystick);
 static uint16_t get_joystick_value(uint8_t channel);
 #endif

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/
#ifdef SIMULATOR
/**
 * @brief 摇杆方向对应的按键绑定
 */
static js_dir_scancodes js_dir_keys[] = {
    [JS_POS_X] = {SDL_SCANCODE_D,SDL_SCANCODE_RIGHT},       //X+方向，D和右方向键
    [JS_NEG_X] = {SDL_SCANCODE_A,SDL_SCANCODE_LEFT},        //X-方向, A和左方向键
    [JS_POS_Y] = {SDL_SCANCODE_S,SDL_SCANCODE_DOWN},        //Y+方向，S和下方向键
    [JS_NEG_Y] = {SDL_SCANCODE_W,SDL_SCANCODE_UP},          //Y-方向，W和下上方向键
};

static joystick_t joystick  = {
    .state = {0},
    .dir_keys = js_dir_keys,
};
#else
static joystick_t joystick = {
    .state = {0,0},
    .x_raw = 0,
    .y_raw = 0,
};
#endif

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief joystick初始化函数
 * @note 需要在main函数中调用一次
 */
void joystick_init()
{
    #ifdef SIMULATOR
    pc_js_init();
    #else
    mcu_js_adc_init();
    #endif
    CONSOLE_INFO("Initialization done!");
}
/**
 * @brief 定期扫描处理joystick
 * @param ptr 传递sdl键盘状态数组 ，单片机 NULL
 * @note 需要计时器 默认速率参考 宏 SCAN_RATE_MS
 */
void joystick_scan(const uint8_t * ptr)
{
    #ifdef SIMULATOR
    joystick_process(&joystick,pc_js_read,ptr);
    #else
    joystick_process(&joystick,hw_js_read,ptr);
    #endif
}
/**
 * @brief 获取摇杆 x 处理值
 * @note form -128 to 127
 */
int16_t joystick_get_x()
{
    return joystick.state.x;
}
/**
 * @brief 获取摇杆 y 处理值
 * @note form -128 to 127
 */
int16_t joystick_get_y()
{
    return joystick.state.y;
}

int16_t rjoystick_get_x(void)
{
    if (comm_get_status() != COMM_STATUS_CONNECTED) return 0;
    return comm_get_joystick_x();
}

int16_t rjoystick_get_y(void)
{
    if (comm_get_status() != COMM_STATUS_CONNECTED) return 0;
    return comm_get_joystick_y();
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

 #ifdef SIMULATOR
 /**
  * @brief PC模拟摇杆初始化，目前没有什么功能
  * @note 虚位以待
  */
static void pc_js_init()
{
    CONSOLE_INFO("PC Joystick Init (using keyboard keys)");
    // 以后加入自定义绑定按键之后这个地方要根据文件读取修改
    CONSOLE_INFO("Key bindings:");
    CONSOLE_INFO("  X+ : D or Right Arrow");
    CONSOLE_INFO("  X- : A or Left Arrow");
    CONSOLE_INFO("  Y+ : S or Down Arrow");
    CONSOLE_INFO("  Y- : W or Up Arrow");
    CONSOLE_INFO("params:");
    CONSOLE_INFO("  ACCEL: %.2f", JS_ACCEL);
    CONSOLE_INFO("  DECAY: %.2f", JS_DECAY);
    CONSOLE_INFO("  Joystick values range from -%d to %d", JOY_MAX_VALUE, JOY_MAX_VALUE);
    return ;
}
/**
 * @brief 读取摇杆方向对应键盘按键状态
 */
 static bool pc_js_read(const uint8_t * sdl_key,const js_dir_scancodes scancodes)
 {
    bool res = false;
    for (int i = 0;i < JS_DIR_KEY_COUNT;i++)
        res = res || sdl_key[scancodes[i]];
    return res;
 };
/**
 * @brief 摇杆处理函数
 * @param jrf 摇杆读取函数
 * @param ptr sdl_key数列存储键盘状态
 */
static void joystick_process(joystick_t * joystick,js_read_func jrf,const uint8_t * ptr)
{
    static int16_t joy_x_target_value;
    static int16_t joy_y_target_value;
    int8_t joy_x_dir = jrf(ptr,joystick->dir_keys[JS_POS_X])-jrf(ptr,joystick->dir_keys[JS_NEG_X]);
    int8_t joy_y_dir = jrf(ptr,joystick->dir_keys[JS_POS_Y])-jrf(ptr,joystick->dir_keys[JS_NEG_Y]);
    joy_x_target_value = joy_x_dir * JOY_MAX_VALUE;
    joy_y_target_value = joy_y_dir * JOY_MAX_VALUE;
    if (joy_x_target_value != 0) joystick->state.x += (joy_x_target_value - joystick->state.x) * JS_ACCEL;
    else joystick->state.x *= JS_DECAY;
    if (joy_y_target_value != 0) joystick->state.y += (joy_y_target_value - joystick->state.y) * JS_ACCEL;
    else joystick->state.y *= JS_DECAY;
    if (joystick->state.x > JOY_MAX_VALUE) joystick->state.x = JOY_MAX_VALUE;
    if (joystick->state.x < -JOY_MAX_VALUE) joystick->state.x = -JOY_MAX_VALUE;
    if (joystick->state.y > JOY_MAX_VALUE) joystick->state.y = JOY_MAX_VALUE;
    if (joystick->state.y < -JOY_MAX_VALUE) joystick->state.y = -JOY_MAX_VALUE;
}
 #else
 /**
  * @brief MCU摇杆初始化 主要是引脚初始化
  */
static void mcu_js_adc_init()
{
    CONSOLE_INFO("Initializing ADC...");
    rcu_periph_clock_enable(JOY_GPIO_RCU);
    rcu_periph_clock_enable(JOY_ADC_RCU);
    CONSOLE_INFO("Clocks enabled (GPIOC, ADC2)");

    gpio_mode_set(JOY_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, JOY_PIN_X | JOY_PIN_Y);
    CONSOLE_INFO("GPIO configured (PC2, PC3 as ANALOG)");

    adc_deinit(JOY_ADC);

    /* HCLK=300MHz, DIV8=37.5MHz */
    adc_clock_config(JOY_ADC, ADC_CLK_SYNC_HCLK_DIV8);
    CONSOLE_INFO("ADC clock configured (37.5MHz)");

    adc_resolution_config(JOY_ADC, ADC_RESOLUTION_12B);
    adc_data_alignment_config(JOY_ADC, ADC_DATAALIGN_RIGHT);
    adc_special_function_config(JOY_ADC, ADC_SCAN_MODE, DISABLE);
    adc_channel_length_config(JOY_ADC, ADC_REGULAR_CHANNEL, 1);

    adc_regular_channel_config(JOY_ADC, 0, ADC_CHANNEL_0, SQX_SMP(2));

    adc_enable(JOY_ADC);
    delay_us(20);
    CONSOLE_INFO("ADC enabled and stabilized");


    CONSOLE_INFO("Starting calibration...");
    adc_calibration_mode_config(JOY_ADC, ADC_CALIBRATION_OFFSET);
    adc_calibration_number(JOY_ADC, ADC_CALIBRATION_NUM16);

    adc_calibration_enable(JOY_ADC);

    uint32_t cal_timeout = 0x3FFFFF;
    while((ADC_CTL1(JOY_ADC) & ADC_CTL1_CLB) && (--cal_timeout));

    if(cal_timeout == 0) {
        CONSOLE_WARNING("Calibration timeout, skipping...");
        LOG_WARNING("Calibration timeout, skipping...");
    } else {
        CONSOLE_INFO("Calibration done");
    }

    adc_regular_channel_config(JOY_ADC, 0, JOY_CH_X, SQX_SMP(2));
    adc_regular_channel_config(JOY_ADC, 0, JOY_CH_Y, SQX_SMP(2));

    CONSOLE_INFO("ADC Initialized!");
    CONSOLE_INFO("Center: %d, Deadzone: %d, Max: %d",
        JOY_CENTER, JOY_DEADZONE, JOY_MAX_VALUE);
}
/**
 * @brief joystick_adc读取函数
 * @param channel joystick对应轴的通道 JOY_CH_*
 */
static uint16_t get_joystick_value(uint8_t channel)
{
    adc_regular_channel_config(JOY_ADC, 0, channel, SQX_SMP(2));

    adc_flag_clear(JOY_ADC, ADC_FLAG_EOC);

    ADC_CTL1(JOY_ADC) |= ADC_CTL1_SWRCST;

    uint32_t start_timeout = 0xFFFF;
    while((ADC_CTL1(JOY_ADC) & ADC_CTL1_SWRCST) && (--start_timeout));

    if(start_timeout == 0) {
        CONSOLE_ERROR("SWRCST stuck! CTL1=0x%08X", ADC_CTL1(JOY_ADC));
        LOG_ERROR("SWRCST stuck! CTL1=0x%08X", ADC_CTL1(JOY_ADC));
        sys_halt();
        return 0;
    }

    uint32_t conv_timeout = 0xFFFFF;
    while((adc_flag_get(JOY_ADC, ADC_FLAG_EOC) == RESET) && (--conv_timeout));

    if(conv_timeout == 0) {
        CONSOLE_ERROR("Conv Timeout CH%d | STAT=0x%X CTL1=0x%X",
               channel, ADC_STAT(JOY_ADC), ADC_CTL1(JOY_ADC));
        LOG_ERROR("Conv Timeout CH%d | STAT=0x%X CTL1=0x%X",
               channel, ADC_STAT(JOY_ADC), ADC_CTL1(JOY_ADC));
        sys_halt();
        return 0;
    }

    adc_flag_clear(JOY_ADC, ADC_FLAG_EOC);

    uint32_t data = adc_regular_data_read(JOY_ADC);

    return (uint16_t)data;
}
/**
 * @brief 获取原始值
 * @param joystick 指向joystick结构体的指针
 */
static void hw_js_read(joystick_t * joystick)
{
    joystick->y_raw = get_joystick_value(JOY_CH_Y);
    joystick->x_raw = get_joystick_value(JOY_CH_X);
}
/**
 * @brief 原始值处理函数，将原始值映射到 -JOY_MAX_VALUE 到 JOY_MAX_VALUE
 * @param jrf 读取原始数值的函数
 * @note 映射假设硬件摇杆原始值最大为 4095
 */
static void joystick_process(joystick_t * joystick,js_read_func jrf,const uint8_t * ptr)
{
    (void) ptr;     //Unused
    //读取原始数值
    jrf(joystick);
    //静态变量区
    static int16_t filtered_x = 0;
    static int16_t filtered_y = 0;
    static bool joy_init_flag = false;
    // 1. 一阶低通滤波（定点数实现，避免浮点运算）
    // filtered = (filtered * (N-1) + raw) / N
    if (!joy_init_flag) {
        // 首次初始化
        filtered_x = (int16_t)joystick->x_raw;
        filtered_y = (int16_t)joystick->y_raw;
        joy_init_flag = 1;
    } else {
        // 指数平滑滤波
        filtered_x = (filtered_x * (FILTER_FACTOR - 1) + (int16_t)joystick->x_raw) / FILTER_FACTOR;
        filtered_y = (filtered_y * (FILTER_FACTOR - 1) + (int16_t)joystick->y_raw) / FILTER_FACTOR;
    }

    // 2. 减去中心值，转为有符号数
    int16_t offset_x = filtered_x - JOY_CENTER;
    int16_t offset_y = filtered_y - JOY_CENTER;
    // 3. 死区处理（消除微小跳动）
    if (offset_x > -JOY_DEADZONE && offset_x < JOY_DEADZONE) {
        offset_x = 0;
    }
    if (offset_y > -JOY_DEADZONE && offset_y < JOY_DEADZONE) {
        offset_y = 0;
    }

    // 4. 映射到 -JOY_MAX_VALUE ~ +JOY_MAX_VALUE
    // ADC 范围：0-4095，中心 2048，最大偏移 2047
    // 公式：output = offset * JOY_MAX_VALUE / 2047
    joystick->state.x = ((int32_t)offset_x * JOY_MAX_VALUE) / (JOY_CENTER - 1);
    joystick->state.y = ((int32_t)offset_y * JOY_MAX_VALUE) / (JOY_CENTER - 1);

    // 5. 限幅（防止溢出）
    if (joystick->state.x > JOY_MAX_VALUE) joystick->state.x = JOY_MAX_VALUE;
    if (joystick->state.x < -JOY_MAX_VALUE) joystick->state.x = -JOY_MAX_VALUE;
    if (joystick->state.y > JOY_MAX_VALUE) joystick->state.y = JOY_MAX_VALUE;
    if (joystick->state.y < -JOY_MAX_VALUE) joystick->state.y = -JOY_MAX_VALUE;
}
 #endif
