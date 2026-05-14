/**
 * @file ui_key.h
 */

/*********************
 *      INCLUDES
 *********************/

#include "ui_key.h"
#include "input_hw.h"
#include "fsm.h"
#include "config.h"

#include "lvgl.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

 /**********************
  *  STATIC PROTOTYPES
  **********************/

static void ui_key_read_cb(lv_indev_drv_t * drv,lv_indev_data_t * data);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

//lvgl 输入驱动
static lv_indev_drv_t ui_indev_drv;
static uint8_t is_init = 0;
static lv_indev_t * indev;

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief keypad input device for lvgl groups
 *        实现硬件按钮对UI的交互
 */
lv_indev_t *key_get_indev(void)
{

  if (!is_init)
  {
    lv_indev_drv_init(&ui_indev_drv);
    ui_indev_drv.type = LV_INDEV_TYPE_KEYPAD;  // 按键类型
    ui_indev_drv.read_cb = ui_key_read_cb;     // 绑定读取
    indev = lv_indev_drv_register(&ui_indev_drv);
    is_init = 1;
  }
  return indev;
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief lvgl输入设备配置函数，指明lvgl互动上，下，确认和谁绑定
 *        lvgl会自动调用
 */
static void ui_key_read_cb(lv_indev_drv_t * drv,lv_indev_data_t * data)
{
  //游戏状态 不参与UI交互
  if (fsm_get_state() == GS_PLAY) 
  {
    data->state = LV_INDEV_STATE_REL;
    return ;
  }

  data->state = LV_INDEV_STATE_REL;

  if (joystick_get_y() > JS_THRESHOLD)    //上
  {
    data->key = LV_KEY_PREV;
    data->state = LV_INDEV_STATE_PR;
  }
  if (joystick_get_y() < - JS_THRESHOLD)  //下
  {
    data->key = LV_KEY_NEXT;
    data->state = LV_INDEV_STATE_PR;
  }
  if (joystick_get_x() > JS_THRESHOLD)  //右
  {
    data->key = LV_KEY_NEXT;
    data->state = LV_INDEV_STATE_PR;
  }
  if (joystick_get_x() < - JS_THRESHOLD)  //左
  {
    data->key = LV_KEY_PREV;
    data->state = LV_INDEV_STATE_PR;
  }
  if (key_down(KEY_A))                    //Enter
  {
    data->key = LV_KEY_ENTER;
    data->state = LV_INDEV_STATE_PR;
  }
}
