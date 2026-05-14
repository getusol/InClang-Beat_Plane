/**
 * @file key.h
 * @note 搜索:添加新硬件按钮 以查看需要进行哪些操作 搜索:添加新虚拟按钮 以查看需要进行的操作(包括.h)
 */

/*********************
 *      INCLUDES
 *********************/
#include "key.h"
#include "config.h"
#ifdef SIMULATOR
#include "SDL2/SDL.h"
#else
#include "drivers.h"
#endif

/**********************
 *      MACROS
 **********************/

#define KEYS_COUNT (sizeof(keys)/sizeof(keys[0]))

#ifdef SIMULATOR
#else
#define HW_KEYS_COUNT (sizeof(HW_KEYS)/sizeof(HW_KEYS[0]))
#define DEBOUNCE_MS 20
#endif

/**********************
 *      TYPEDEFS
 **********************/
/**
 * @brief 按钮状态结构体，纪录按钮是按下还是长按状态
 */
typedef struct {
    bool pressed;
    bool released;
    bool long_press;
    bool stable;
    bool last;
} key_state_t;


//PC独有
 #ifdef SIMULATOR
 /**
  * @brief 每个虚拟按键绑定的键盘按键数组，内容为SDL的ScanCode
  */
 typedef SDL_Scancode bind_scancodes[MAX_BINDING_KEYS_COUNT];
/**
 * @brief PC模拟 按钮结构体 和 长按时间 以及对应的键盘按钮映射绑定 
 */ 
typedef struct {
    key_state_t state;
    bind_scancodes bind_codes; 
    uint16_t press_tick;
    uint8_t pressed_reset_cnt;
    uint8_t released_reset_cnt;
} key_t;
/**
 * @brief key_read_func 执行获取按键状态的函数
 * @param sdl_key sdl键盘状态记录数组
 * @param bind_codes 虚拟按键绑定的键盘按键数组
 */
typedef bool (*key_read_func)(const uint8_t * sdl_key,const bind_scancodes bind_codes);
//MCU独有
 #else
/**
 * @brief gpio按钮枚举值
 * @note 添加新硬件按钮:如果要加入新的硬件按钮，此处需要注册
 */
enum {
    S3 = 0,
    S4,
    S5,
    S2
};
/**
 * @brief gpio硬件按钮注册结构体
 */
typedef struct {
    uint32_t port;
    uint32_t pin;
    uint32_t rcu_clock;
} gpio_key_info_t;
/**
 * @brief MCU 按钮结构体 包括原始信号 和 消抖时间 以及对应的硬件按钮 
 */ 
typedef struct {
    key_state_t state;
    const gpio_key_info_t * binding_key;
    bool raw;
    uint16_t press_tick;
    uint16_t debounce_cnt;
    uint8_t pressed_reset_cnt;
    uint8_t released_reset_cnt;
} key_t;

/**
 * @brief key_read_func 执行获取按键状态的函数
 * @param port gpio_port
 * @param pin gpio_pin
 */
typedef bool (*key_read_func)(uint32_t port,uint32_t pin);
#endif
 /**********************
  *  STATIC PROTOTYPES
  **********************/

 static void key_process(key_t * key,key_read_func krf,const uint8_t * ptr);

 #ifdef SIMULATOR
 static bool kb_key_read(const uint8_t * sdl_key,const bind_scancodes bind_codes);
 static void pc_key_init();
 #else
 static void hw_gpio_init();
 static bool hw_key_read(uint32_t port,uint32_t pin);
 #endif

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/
#ifdef SIMULATOR
 /**
  * @brief PC 用到的 A B X Y键的key_t结构初始化 所有虚拟按钮添加在此处 按照 key_code_t的顺序排列
  * @note 添加新虚拟按钮:在此处注册，并指明其绑定的键盘按钮,顺序与key_code_t枚举一致
  *       由于KEY_NONE的存在每个KEY_ANY 都要相应减一
  */
static key_t keys[] = {
    [KEY_A - 1] = {
        .state = {0},
        .bind_codes = {SDL_SCANCODE_SPACE,SDL_SCANCODE_RETURN},
        .press_tick = 0,
        .pressed_reset_cnt = 0,
        .released_reset_cnt = 0,
    },                                  //BUTTON_KEY_A 空格和回车
    [KEY_B - 1] = {
        .state = {0},
        .bind_codes = {SDL_SCANCODE_Q,SDL_SCANCODE_ESCAPE},
        .press_tick = 0,
        .pressed_reset_cnt = 0,
    },                                  //BUTTON_KEY_B Q和ESCAPE
    [KEY_X - 1] = {
        .state = {0},
        .bind_codes = {SDL_SCANCODE_LSHIFT,SDL_SCANCODE_UNKNOWN},
        .press_tick = 0,
        .released_reset_cnt = 0,
        .pressed_reset_cnt = 0,
    },                                  //BUTTON_KEY_X LSHIFT
    [KEY_Y - 1] = {
        .state = {0},
        .bind_codes = {SDL_SCANCODE_F,SDL_SCANCODE_UNKNOWN},
        .press_tick = 0,
        .pressed_reset_cnt = 0,
        .released_reset_cnt = 0,
    },                                  //BUTTON_KEY_Y F
};
#else
 /**
  * @brief 硬件按钮信息 包括 port，pin,rcu_clock 所有硬件按钮在此次注册
  * @note 添加新硬件按钮:需要在此处注册，绑定对应端口引脚，确保顺序和gpio按钮枚举一致
  */
 static const gpio_key_info_t HW_KEYS[] = {
    [S3] = {
        .port = GPIOD,
        .pin = GPIO_PIN_3,
        .rcu_clock = RCU_GPIOD
    },                                  //S3
    [S4] = {
        .port = GPIOD,
        .pin = GPIO_PIN_4,
        .rcu_clock = RCU_GPIOD
    },                                  //S4
    [S5] = {
        .port = GPIOD,
        .pin = GPIO_PIN_5,
        .rcu_clock = RCU_GPIOD
    },                                  //S5
    [S2] = {
        .port = GPIOC,
        .pin = GPIO_PIN_13,
        .rcu_clock = RCU_GPIOC
    },                                  //S2
 };

 /**
  * @brief MCU 用到的 A B X Y键的key_t结构初始化 所有虚拟按钮添加在此处 按照 button_key_t的顺序排列
  * @note 添加新虚拟按钮:在此处注册，并指明其绑定的硬件按钮,顺序与button_key_t枚举一致
  *       由于BUTTON_KEY_NONE的存在每个BUTTON_KEY_ANY 都要相应减一
  */
 static key_t keys[] = {
    [KEY_A - 1] = {
        .state = {0},
        .binding_key = &HW_KEYS[S3],
        .debounce_cnt = 0,
        .press_tick = 0,
        .raw = false,
        .pressed_reset_cnt = 0,
        .released_reset_cnt = 0,
    },                                  //BUTTON_KEY_A S3
    [KEY_B - 1] = {
        .state = {0},
        .binding_key = &HW_KEYS[S4],
        .debounce_cnt = 0,
        .press_tick = 0,
        .raw = false,
        .pressed_reset_cnt = 0,
        .released_reset_cnt = 0,
    },                                  //BUTTON_KEY_B S4
    [KEY_X - 1] = {
        .state = {0},
        .binding_key = &HW_KEYS[S5],
        .debounce_cnt = 0,
        .press_tick = 0,
        .raw = false,
        .pressed_reset_cnt = 0,
        .released_reset_cnt = 0,
    },                                  //BUTTON_KEY_X S5
    [KEY_Y - 1] = {
        .state = {0},
        .binding_key = &HW_KEYS[S2],
        .debounce_cnt = 0,
        .press_tick = 0,
        .raw = false,
        .pressed_reset_cnt = 0,
        .released_reset_cnt = 0,
    }                                   //BUTTON_KEY_Y S2
 };
 #endif

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化按钮读取，包括gpio的port,pin,clock设置，以及PC上的一些设置
 * @note 这个函数需要在main.c调用一次
 */
void key_init()
{
    #ifdef SIMULATOR
    pc_key_init();
    #else
    hw_gpio_init();
    #endif
}
/**
 * @brief key_scan 负责每隔一段时间扫描处理所有按键，基本上只是一堆key_process的入口 间隔 SCAN_RATE_MS
 * @note 这个函数需要被添加一个lvgl计时器 间隔 SCAN_RATE_MS
 * @param ptr 这个参数传递sdl_key键盘状态数组，若为单片机 则无用填NULL
 */
void key_scan(const uint8_t * ptr)
{
    for (int i = 0;i < KEYS_COUNT;i++)
    {
        #ifdef SIMULATOR
        key_process(keys + i,kb_key_read,ptr);
        #else
        key_process(keys + i,hw_key_read,ptr);
        #endif
    }
}
/**
 * @brief 检测某按键是否按下（单次触发）
 * @param key 虚拟按键枚举编号
 */
bool key_pressed(key_code_t key)
{
    if (!key) return false;
    if (!keys[key - 1].state.pressed) return false;
    keys[key - 1].state.pressed = false; //单次触发，读取后重置
    return true;
}
/**
 * @brief 检测某按键是否松开（单次触发）
 * @param key 虚拟按键枚举编号
 */
bool key_released(key_code_t key)
{
    if (!key) return false;
    if (!keys[key - 1].state.released) return false;
    keys[key - 1].state.released = false; //单次触发，读取后重置
    return true;
}
/**
 * @brief 检测某按键是否按下（处于按下状态）
 * @param key 虚拟按键枚举编号
 */
bool key_down(key_code_t key)
{
    if (!key) return false;
    return keys[key - 1].state.stable;
}
/**
 * @brief 检测某按键是否长按（处于长按状态）
 * @param key 虚拟按键枚举编号
 */
bool key_long_press(key_code_t key)
{
    if (!key) return false;
    return keys[key - 1].state.long_press;
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

#ifdef SIMULATOR
/**
 * @brief PC端函数，读取虚拟键盘绑定键盘按键并返回其按下状态
 * @param sdl_key sdl键盘状态记录数组
 * @param bind_codes 虚拟按键绑定的2个SDL_Scancodes
 * @note 采取或形式 即两个按钮按下任意都算
 */
static bool kb_key_read(const uint8_t * sdl_key,const bind_scancodes bind_codes)
{
    bool res = false;
    for (int i = 0;i < MAX_BINDING_KEYS_COUNT;i++)
        res = res || sdl_key[bind_codes[i]];
    return res;
}
/**
 * @brief PC输入初始化函数
 * @note 目前没有可以做的，虚位以待
 */
static void pc_key_init()
{
    return ;
}
/**
 * @brief 按钮处理，从键盘按钮状态获取信息并最终写入 key_state_t
 * @param ptr 用来传递sdl_key数组
 */
static void key_process(key_t * key,key_read_func krf,const uint8_t * ptr)
{
    //1.key stable状态获取
    key->state.stable = krf(ptr,key->bind_codes);
    //2.单击/释放边沿检测
    bool raw_pressed = !key->state.last && key->state.stable;
    bool raw_released = key->state.last && !key->state.stable;
    if (raw_pressed) {
        key->state.pressed = true;
        key->pressed_reset_cnt = 0;
    }
    if (raw_released) {
        key->state.released = true;
        key->released_reset_cnt = 0;
    }

    if (key->state.pressed) {
        key->pressed_reset_cnt += SCAN_RATE_MS;
        if (key->pressed_reset_cnt >= PRESSED_TICKS_THRESHOLD) {
            key->state.pressed = false;
        }
    }
    if (key->state.released) {
        key->released_reset_cnt += SCAN_RATE_MS;
        if (key->released_reset_cnt >= RELEASED_TICKS_THRESHOLD) {
            key->state.released = false;
        }
    }
    key->state.last = key->state.stable;
    //3.长按检测
    if (key->state.stable)
    {
        key->press_tick += SCAN_RATE_MS;
        if (key->press_tick >= LONG_PRESS_MS)
            key->state.long_press = true;
    } 
    else 
    {
        key->press_tick = 0;
        key->state.long_press = false;
    }
}
 #else
/**
 * @brief 初始化硬件按钮port,pin,clock的函数
 */
static void hw_gpio_init()
{
    //1.开启时钟
    for (int i = 0;i < HW_KEYS_COUNT;i++)
        rcu_periph_clock_enable(HW_KEYS[i].rcu_clock);
    //2.按键上拉模式配置
    for (int i = 0;i < HW_KEYS_COUNT;i++)
     gpio_mode_set(
        HW_KEYS[i].port,
        GPIO_MODE_INPUT,       // 输入模式
        GPIO_PUPD_PULLUP,      // 上拉
        HW_KEYS[i].pin
    );
    //3.其它特殊按键单独配置

}
/**
 * @brief 读取对应硬件按钮的状态，返回true即为按下
 */
static bool hw_key_read(uint32_t port,uint32_t pin)
{
    return (RESET == gpio_input_bit_get(port,pin));
}
/**
 * @brief 按钮处理，从硬件按钮状态获取信息并最终写入 key_state_t
 * @param ptr 用来传递sdl_key数组 MCU中没有用，填NULL就行
 */
static void key_process(key_t * key,key_read_func krf,const uint8_t * ptr)
{
    (void) ptr;                                  //Unused
    //1.读取原始信息
    key->raw = krf(key->binding_key->port,key->binding_key->pin);
    //2.消抖
    if (key->raw == key->state.stable)          //消抖完毕
    {
        key->debounce_cnt = 0;
    }
    else                                        //消抖过程
    {
        key->debounce_cnt += SCAN_RATE_MS;
        if (key->debounce_cnt >= DEBOUNCE_MS)   //不相等维持了20ms,达到消抖阈值,说明stable确实应该变了
        {
            key->state.stable = key->raw;
            key->debounce_cnt = 0;
        }
    }
    //3.单击/释放边沿检测
    bool raw_pressed = !key->state.last && key->state.stable;
    bool raw_released = key->state.last && !key->state.stable;
    if (raw_pressed) {
        key->state.pressed = true;
        key->pressed_reset_cnt = 0;
    }
    if (raw_released) {
        key->state.released = true;
        key->released_reset_cnt = 0;
    }

    if (key->state.pressed) {
        key->pressed_reset_cnt += SCAN_RATE_MS;
        if (key->pressed_reset_cnt >= PRESSED_TICKS_THRESHOLD) {
            key->state.pressed = false;
        }
    }
    if (key->state.released) {
        key->released_reset_cnt += SCAN_RATE_MS;
        if (key->released_reset_cnt >= RELEASED_TICKS_THRESHOLD) {
            key->state.released = false;
        }
    }
    key->state.last = key->state.stable;                    //同步
    //4.长按检测
    if (key->state.stable)
    {
        key->press_tick += SCAN_RATE_MS;
        if (key->press_tick >= LONG_PRESS_MS)
            key->state.long_press = true;
    }
    else
    {
        key->press_tick = 0;
        key->state.long_press = false;
    }
}
#endif
