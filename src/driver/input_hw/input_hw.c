/**
 * @file input_hw.h
 */

/*********************
 *      INCLUDES
 *********************/
#include "input_hw.h"
#include "tools.h"
#ifdef SIMULATOR
#include "SDL2/SDL.h"
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

 /**********************
  *  STATIC PROTOTYPES
  **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

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
  
  console_out("[input] Initialization done!\n");
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
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

