/**
 * @file event.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "event.h"
#include "tools.h"
#include <string.h> // memset

/**********************
 *      MACROS
 **********************/

#define MAX_CALLBACKS_PRE_EVENT 4   //每个事件最多4个回调

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

static event_callback_t callbacks[EVENT_COUNT][MAX_CALLBACKS_PRE_EVENT];

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
* @brief 事件初始化
*/
void event_init()
{
  // 清空所有回调
  memset(callbacks,0,sizeof(callbacks));
  CONSOLE("[INFO] Event initialization complete.");
}

/**
 * @brief 事件分发,依次调用该事件的所有注册回调
 * @param code 事件类型
 * @param source 事件源对象
 * @param target 事件目标对象
 */
void event_dispatch(event_code_t code,game_obj_t * source,game_obj_t * target)
{
  if (code <= EVENT_NONE || code >= EVENT_COUNT) {
    CONSOLE("[WARNING] Event dispatch failed,invalid event type: %d",code);
    LOG("[WARNING] Event dispatch failed,invalid event type: %d",code);
    return ;
  }

  for (int i = 0; i < MAX_CALLBACKS_PRE_EVENT; i++) {
    if (callbacks[code][i] != NULL) {
      callbacks[code][i](source,target);
    }
  }
}

/**
 * @brief 注册事件回调
 * @param code 事件类型
 * @param callback 回调函数
 * @note 单个事件最多支持 ` MAX_CALLBACKS_PRE_EVENT ` 个回调
 */
void event_register(event_code_t code,event_callback_t callback)
{
  if (code <= EVENT_NONE || code >= EVENT_COUNT) {
    CONSOLE("[WARNING] Event register failed,invalid event type: %d",code);
    LOG("[WARNING] Event register failed,invalid event type: %d",code);
    return ;
  }
  if (callback == NULL) {
    CONSOLE("[WARNING] Event register failed,callback is NULL");
    LOG("[WARNING] Event register failed,callback is NULL");
    return ;
  }

  // 重复则跳过 视为成功
  for (int i = 0;i < MAX_CALLBACKS_PRE_EVENT;i++) {
    if (callbacks[code][i] == callback) {
      return ;
    }
  }

  for (int i = 0; i < MAX_CALLBACKS_PRE_EVENT; i++) {
    if (callbacks[code][i] == NULL) {
      callbacks[code][i] = callback;
      CONSOLE("[INFO] Event callback registered for event %d at index %d", code, i);
      return ;
    }
  }
  
  // 没有空位
  CONSOLE("[WARNING] No available slot to register callback for event: %d", code);
  LOG("[WARNING] No available slot to register callback for event: %d", code);
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/
