/**
 * @file lv_port.h
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_port.h"
#include "lvgl.h"
#include "config.h"
#ifdef SIMULATOR
#include <stdint.h>
#include <stdbool.h>
#include "sdl/sdl.h"
#else
#include "drivers.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"
#endif

/**********************
 *      MACROS
 **********************/
#ifdef SIMULATOR
#define DISP_BUF_SIZE (1024 * 100)
#endif

/**********************
 *      TYPEDEFS
 **********************/

 /**********************
  *  STATIC PROTOTYPES
  **********************/
#ifdef SIMULATOR
static void lv_port_disp_init();
static void lv_port_indev_init();
static void lv_port_mouse_init();
static void lv_port_keyboard_init();
#else
static void pin_init();
#endif

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/
#ifdef SIMULATOR
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[DISP_BUF_SIZE];
#else

#endif
 
 /**********************
 *   GLOBAL FUNCTIONS
 **********************/
#ifdef SIMULATOR
/**
 * @brief 初始化lvgl,sdl,显示屏和输入设备
 */
void lv_port_init()
{
    lv_init();
    sdl_init();
    lv_port_disp_init();
    lv_port_indev_init();
}
#else
/**
 * @brief 初始化单片机sys,pin,lvgl,显示屏和输入设备
 */
void lv_port_init()
{
    sys_init();
    pin_init();
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
}
#endif

 /**********************
 *   STATIC FUNCTIONS
 **********************/

#ifdef SIMULATOR
/**
 * @brief LVGL显示屏初始化
 */
static void lv_port_disp_init()
{
    /*LVGL 显示缓冲区*/
    lv_disp_draw_buf_init(&draw_buf,buf,NULL,DISP_BUF_SIZE);
    /*LVGL 显示屏初始化、配置*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}
/**
 * @brief LVGL 输入设备初始化
 */
static void lv_port_indev_init()
{
    lv_port_mouse_init();
    lv_port_keyboard_init();
}
/**
 * @brief LVGL_SDL鼠标初始化
 */
static void lv_port_mouse_init()
{
    static lv_indev_drv_t indev_drv_mouse;
    lv_indev_drv_init(&indev_drv_mouse);
    indev_drv_mouse.type = LV_INDEV_TYPE_POINTER;
    indev_drv_mouse.read_cb = sdl_mouse_read;
    lv_indev_drv_register(&indev_drv_mouse);
}
/**
 * @brief LVGL_SDL键盘初始化
 */
static void lv_port_keyboard_init()
{
    static lv_indev_drv_t indev_drv_kb;
    lv_indev_drv_init(&indev_drv_kb);
    indev_drv_kb.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv_kb.read_cb = sdl_keyboard_read;
    lv_indev_drv_register(&indev_drv_kb);
}
#else
/**
 * @brief 初始化单片机LED的引脚
 */
static void pin_init()
{
    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    gpio_bit_reset(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
}
#endif
