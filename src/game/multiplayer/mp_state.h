/**
 * @file mp_state.h
 * @brief 多人联机状态机状态枚举
 */

#ifndef __MP_STATE_H__
#define __MP_STATE_H__

/*********************
 *      INCLUDES
 *********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief 多人联机状态枚举
 */
typedef enum {
    MP_STATE_IDLE,           /**< 空闲状态，无多人联机活动 */
    MP_STATE_INVITING,       /**< 已发送邀请，等待对方响应 */
    MP_STATE_WAITING,        /**< 收到邀请，等待本地用户响应 */
    MP_STATE_CONNECTED,      /**< 双方已建立联机连接 */
    MP_STATE_GAME_PLAY,      /**< 联机游戏进行中 */
    MP_STATE_DISCONNECTED,   /**< 连接已断开 */

    MP_STATE_MAX,            /**< 最大状态值，用于数组索引和状态机状态_STATE_MAX */
} mp_state_t;

 /* 状态转移图:
 *   IDLE ──(发送邀请)──→ INVITING ──(收到接受)──→ CONNECTED
 *     │                      │                        │
 *     │                      ├──(收到拒绝)──→ IDLE    │
 *     │                      ├──(超时)──────→ IDLE    │
 *     │                      └──(取消)──────→ IDLE    │
 *     │                                               │
 *     ├──(收到邀请)──→ WAITING ──(接受)──→ CONNECTED  │
 *     │                   │                           │
 *     │                   └──(拒绝)──→ IDLE           │
 *     │                                               │
 *     └──←──←──←──←──←──←──←──←──←──←──←──←──←──    │
 *                                                   │
 *   CONNECTED ──(开始游戏)──→ GAME_STARTED           │
 *       │                        │                   │
 *       └──(连接断开)────────────┴──→ DISCONNECTED   │
 *                                         │
 *   DISCONNECTED ──(清理完成)──→ IDLE
 */

#endif // #ifndef __MP_STATE_H__
