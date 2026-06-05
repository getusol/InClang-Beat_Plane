/**
 * @file mp_event.h
 * @brief 多人联机事件系统 —— 事件枚举与回调类型定义
 *
 * 其它模块（如 ui_base）通过注册回调来响应多人联机事件，
 * 从而实现 ui_base 与 comm 的解耦。
 */

#ifndef __MP_EVENT_H__
#define __MP_EVENT_H__

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
 * @brief 多人联机事件枚举
 */
typedef enum {
    MP_EVENT_INVITE_RECEIVED,   /**< 收到对方邀请（弹出 Yes/No 对话框） */
    MP_EVENT_INVITE_ACCEPTED,   /**< 己方邀请被对方接受 */
    MP_EVENT_INVITE_REJECTED,   /**< 己方邀请被对方拒绝 */
    MP_EVENT_INVITE_TIMEOUT,    /**< 己方邀请超时未收到响应 */
    MP_EVENT_CONNECTED,         /**< 联机连接成功建立 */
    MP_EVENT_DISCONNECTED,        /**< 联机连接断开 */
    MP_EVENT_WAITING_TIMEOUT,     /**< 等待响应超时（己方作为接受方时邀请方超时/取消） */

    MP_EVENT_MAX,              /**< 最大事件值，用于数组索引和事件机状态_EVENT_MAX */
} mp_event_t;

/**
 * @brief 多人联机事件回调函数类型
 * @param event  触发的事件类型
 * @param data   事件附加数据（当前统一为 NULL，预留扩展）
 */
typedef void (*mp_event_cb_t)(mp_event_t event, void *data);

#endif // #ifndef __MP_EVENT_H__
