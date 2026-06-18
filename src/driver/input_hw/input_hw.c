/**
 * @file input_hw.h
 */

/*********************
 *      INCLUDES
 *********************/
#include "input_hw.h"
#include "tools.h"
#include "lvgl.h" // for lvgl tick get
#ifdef SIMULATOR
#include "SDL.h"
#else
#include "protocol.h"
#include "comm_tx.h"
#include "comm_status.h"
#endif

/**********************
 *      MACROS
 **********************/

#define COMM_SEND_RATE_MS 5

/**********************
 *      TYPEDEFS
 **********************/

 /**********************
  *  STATIC PROTOTYPES
  **********************/

static void input_remote_send();

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static non_blocking_timer_t comm_send_timer;

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 按键硬件初始化入口
 * @note 需要调用一次
 */
void input_hw_init()
{
  key_init();
  joystick_init();

  comm_send_timer = (non_blocking_timer_t){
    .func = input_remote_send,
    .tick_get = lv_tick_get,
    .delay_ms = COMM_SEND_RATE_MS,
    .last_tick = 0,
  };

  CONSOLE_INFO("Initialization done!");
}

/**
 * @brief 获取按键状态
 */
void input_hw_scan(void)
{
  static const uint8_t * sdl_key = (void*)0;
  #ifdef SIMULATOR
  if (!sdl_key) sdl_key = SDL_GetKeyboardState(NULL);
  #endif
  key_scan(sdl_key);
  joystick_scan(sdl_key);
  non_blocking_delay(&comm_send_timer); // COMM_SEND_RATE_MS ms 一次
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

/**
* @brief 输入远程发送函数，负责将输入状态通过通信模块发送到其他设备
*/
static void input_remote_send()
{
#ifdef SIMULATOR
  return ; //Do nothing in simulator mode
#else
  if (comm_get_status() != COMM_STATUS_CONNECTED) return ;
  // keys
  uint8_t key_mask = 0x00;
  for (int i = 1; i < KEY_MAX; i++) {
    if (key_down(i)) {
      switch (i) {
        case KEY_A:
          key_mask |= COMM_KEY_A_MASK;
          break;
        case KEY_B:
          key_mask |= COMM_KEY_B_MASK;
          break;
        case KEY_X:
          key_mask |= COMM_KEY_X_MASK;
          break;
        case KEY_Y:
          key_mask |= COMM_KEY_Y_MASK;
          break;
        default:
          CONSOLE_WARNING("Unhandled key code: %d\n", i);
          LOG_WARNING("Unhandled key code: %d", i);
          break;
      }
    }
  }
  comm_mcu_send_key_state(key_mask);

  // joystick
  comm_mcu_send_joystick(joystick_get_x(), joystick_get_y());

  return ;
#endif
}
