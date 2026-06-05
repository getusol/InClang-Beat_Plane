/**
 * @file multiplayer.c
 * @brief 多人联机模块 —— 状态机与事件分发实现
 *
 * 本模块是 ui_base 与 comm 之间的桥梁：
 * - 从 comm 读取邀请/确认数据，驱动状态机
 * - 向 UI 等关注方分发事件
 * - 将 UI 的用户动作转换为 comm 的帧发送
 */

/*********************
 *      INCLUDES
 *********************/

#include "multiplayer.h"
#include "mp_state.h"
#include "mp_event.h"
#include "comm_rx.h"
#include "comm_tx.h"
#include "comm_status.h"
#include "tools.h"
#include "lvgl.h"
#include "fsm.h"
#include <string.h>

/**********************
 *      MACROS
 **********************/

#define MP_MAX_CALLBACKS        3     /**< 每个事件最多注册的回调数量 */
#define MP_INVITE_TIMEOUT_MS    10000 /**< 邀请超时时间（毫秒），超时后触发 MP_EVENT_INVITE_TIMEOUT */
#define MP_WAITING_TIMEOUT_MS   15000 /**< 等待响应超时（毫秒），比 INVITING 超时更长以防竞争 */

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void dispatch_event(mp_event_t event, void *data);
static bool can_send_invite(void);
static bool can_accept_invite(void);
static void change_state(mp_state_t new_state);
static void handle_state_idle(void);
static void handle_state_inviting(void);
static void handle_state_waiting(void);
static void handle_state_connected(void);
static void handle_state_game_started(void);
static void handle_state_disconnected(void);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static mp_state_t current_state = MP_STATE_IDLE;

/* 事件回调注册表：每个事件最多 MP_MAX_CALLBACKS 个回调 */
static mp_event_cb_t callbacks[MP_EVENT_MAX][MP_MAX_CALLBACKS]; /* 7 = mp_event_t 枚举成员数 */

/* 邀请超时计时器（仅在 MP_STATE_INVITING 期间有效） */
static non_blocking_timer_t invite_timer = {
    .func = NULL, /* 不需要回调，在 mp_update 中主动检查 */
    .tick_get = lv_tick_get,
    .delay_ms = MP_INVITE_TIMEOUT_MS,
    .last_tick = 0,
};

/* 等待超时计时器（仅在 MP_STATE_WAITING 期间有效） */
static non_blocking_timer_t waiting_timer = {
    .func = NULL,
    .tick_get = lv_tick_get,
    .delay_ms = MP_WAITING_TIMEOUT_MS,
    .last_tick = 0,
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化多人联机模块
 * @note 必须在 comm_init() 之后调用
 */
void mp_init(void)
{
    current_state = MP_STATE_IDLE;
    memset(callbacks, 0, sizeof(callbacks));
    CONSOLE_INFO("Multiplayer module initialized.");
}

/**
 * @brief 定期更新多人联机状态机
 * @note 应在主循环中定期调用（推荐间隔 100ms）
 *       检查 comm 收到的邀请/确认帧、超时状态、连接状态等
 */
void mp_update(void)
{
    switch (current_state) {
    case MP_STATE_IDLE:         handle_state_idle();         break;
    case MP_STATE_INVITING:     handle_state_inviting();     break;
    case MP_STATE_WAITING:      handle_state_waiting();      break;
    case MP_STATE_CONNECTED:    handle_state_connected();    break;
    case MP_STATE_GAME_PLAY: handle_state_game_started(); break;
    case MP_STATE_DISCONNECTED: handle_state_disconnected(); break;
    default:
        CONSOLE_WARNING("mp_update: unknown state %d, resetting to IDLE.", current_state);
        change_state(MP_STATE_IDLE);
        break;
    }
}

/**
 * @brief 获取当前多人联机状态
 * @return 当前状态值
 */
mp_state_t mp_get_state(void)
{
    return current_state;
}

/**
 * @brief 发送联机邀请
 * @return true=发送成功, false=当前状态不允许发送
 * @note 仅在 MP_STATE_IDLE 且 comm 已连接时有效
 *       成功后状态切换为 MP_STATE_INVITING
 */
bool mp_send_invite(void)
{
    if (!can_send_invite()) {
        CONSOLE_WARNING("mp_send_invite: cannot send in state %d", current_state);
        return false;
    }

    /* 防御：丢弃上一次会话残留的 INVITE_ACK，防止被误读 */
    while (comm_has_invite_ack()) {
        comm_get_invite_ack();
    }

    comm_send_invite();
    change_state(MP_STATE_INVITING);
    invite_timer.last_tick = lv_tick_get();
    CONSOLE_INFO("Invite sent, waiting for response...");
    return true;
}

/**
 * @brief 接受收到的联机邀请
 * @return true=操作成功
 * @note 仅在 MP_STATE_WAITING 时有效
 *       成功后状态切换为 MP_STATE_CONNECTED
 */
bool mp_accept_invite(void)
{
    if (current_state != MP_STATE_WAITING) {
        CONSOLE_WARNING("mp_accept_invite: not in WAITING state (current=%d)", current_state);
        return false;
    }

    comm_send_invite_ack(true);
    change_state(MP_STATE_CONNECTED);
    dispatch_event(MP_EVENT_CONNECTED, NULL);
    CONSOLE_INFO("Invite accepted, connected.");
    return true;
}


/**
 * @brief 拒绝收到的联机邀请
 * @return true=操作成功
 * @note 仅在 MP_STATE_WAITING 时有效
 *       成功后状态切换为 MP_STATE_IDLE
 */
bool mp_reject_invite(void)
{
    if (current_state != MP_STATE_WAITING) {
        CONSOLE_WARNING("mp_reject_invite: not in WAITING state (current=%d)", current_state);
        return false;
    }

    comm_send_invite_ack(false);
    change_state(MP_STATE_IDLE);
    CONSOLE_INFO("Invite rejected.");
    return true;
}

/**
 * @brief 取消已发出但未收到响应的邀请
 * @note 仅在 MP_STATE_INVITING 时有效
 *       成功后状态切换为 MP_STATE_IDLE
 */
void mp_cancel_invite(void)
{
    if (current_state != MP_STATE_INVITING) {
        CONSOLE_WARNING("mp_cancel_invite: not in INVITING state (current=%d)", current_state);
        return;
    }

    comm_send_invite_cancel();
    change_state(MP_STATE_IDLE);
    CONSOLE_INFO("Invite cancelled.");
}

/**
 * @brief 主动断开联机会话
 * @note 在 MP_STATE_CONNECTED 或 MP_STATE_GAME_PLAY 时有效
 *       成功后状态切换为 MP_STATE_DISCONNECTED
 */
void mp_disconnect(void)
{
    if (current_state != MP_STATE_CONNECTED && current_state != MP_STATE_GAME_PLAY) {
        CONSOLE_WARNING("mp_disconnect: not connected (current=%d)", current_state);
        return;
    }

    comm_send_disconnect();
    change_state(MP_STATE_DISCONNECTED);
    dispatch_event(MP_EVENT_DISCONNECTED, NULL);
    CONSOLE_INFO("Multiplayer session disconnected.");
}

/**
 * @brief 标记联机游戏开始
 * @return true=操作成功
 * @note 仅在 MP_STATE_CONNECTED 时有效
 *       成功后状态切换为 MP_STATE_GAME_PLAY
 */
bool mp_start_game(void)
{
    if (current_state != MP_STATE_CONNECTED) {
        CONSOLE_WARNING("mp_start_game: not connected (current=%d)", current_state);
        return false;
    }

    change_state(MP_STATE_GAME_PLAY);
    CONSOLE_INFO("Multiplayer game started.");
    return true;
}


/**
 * @brief 注册多人联机事件回调
 * @param event    要监听的事件类型
 * @param callback 回调函数指针
 * @return true=注册成功, false=槽位已满或参数无效
 * @note 每个事件最多支持 MP_MAX_CALLBACKS 个回调
 */
bool mp_event_register(mp_event_t event, mp_event_cb_t callback)
{
    if (event >= MP_EVENT_MAX) {
        CONSOLE_WARNING("mp_event_register: invalid event %d", event);
        return false;
    }
    if (callback == NULL) {
        CONSOLE_WARNING("mp_event_register: callback is NULL");
        return false;
    }

    /* 去重：已注册则视为成功 */
    for (int i = 0; i < MP_MAX_CALLBACKS; i++) {
        if (callbacks[event][i] == callback) {
            return true;
        }
    }

    /* 寻找空槽位 */
    for (int i = 0; i < MP_MAX_CALLBACKS; i++) {
        if (callbacks[event][i] == NULL) {
            callbacks[event][i] = callback;
            CONSOLE_INFO("mp_event: callback registered for event %d at slot %d", event, i);
            return true;
        }
    }

    CONSOLE_WARNING("mp_event_register: no free slot for event %d (max %d)", event, MP_MAX_CALLBACKS);
    return false;
}

/**
 * @brief 注销多人联机事件回调
 * @param event    事件类型
 * @param callback 要移除的回调函数指针
 * @return true=注销成功, false=未找到该回调
 */
bool mp_event_unregister(mp_event_t event, mp_event_cb_t callback)
{
    if (event >= MP_EVENT_MAX) {
        CONSOLE_WARNING("mp_event_unregister: invalid event %d", event);
        return false;
    }
    if (callback == NULL) {
        return false;
    }

    for (int i = 0; i < MP_MAX_CALLBACKS; i++) {
        if (callbacks[event][i] == callback) {
            callbacks[event][i] = NULL;
            CONSOLE_INFO("mp_event: callback unregistered for event %d at slot %d", event, i);
            return true;
        }
    }

    return false;
}

/**
 * @brief 退出游戏 将GAME_PLAY变为CONNECTED
 */
void mp_exit_game(void)
{
    if (current_state == MP_STATE_GAME_PLAY) {
        change_state(MP_STATE_CONNECTED);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 分发事件到所有注册的回调
 */
static void dispatch_event(mp_event_t event, void *data)
{
    if (event >= MP_EVENT_MAX) return;

    for (int i = 0; i < MP_MAX_CALLBACKS; i++) {
        if (callbacks[event][i] != NULL) {
            callbacks[event][i](event, data);
        }
    }
}

/**
 * @brief 检查是否允许发送邀请
 * @return true 仅当处于 IDLE 状态且 comm 已连接
 */
static bool can_send_invite(void)
{
    if (current_state != MP_STATE_IDLE) return false;
    return (comm_get_status() == COMM_STATUS_CONNECTED);
}

/**
 * @brief 检查是否允许接受邀请
 * @return true 仅当不处于游戏状态
 */
static bool can_accept_invite(void)
{
    game_state_t gs = fsm_get_state();
    return gs != GS_PLAY;
}

/**
 * @brief 状态转移（含日志）
 */
static void change_state(mp_state_t new_state)
{
    static const char *state_names[] = {
        "IDLE", "INVITING", "WAITING", "CONNECTED", "GAME_PLAY", "DISCONNECTED"
    };
    CONSOLE_INFO("mp_state: %s -> %s", state_names[current_state], state_names[new_state]);
    current_state = new_state;
}

/**
 * @brief IDLE 状态：检查是否收到对方邀请，同时清理残留数据
 */
static void handle_state_idle(void)
{
    /* 防御性清理：丢弃上一次会话残留的数据 */
    if (comm_has_invite_ack()) {
        comm_get_invite_ack();       /* 丢弃旧 ACK */
    }
    comm_has_invite_cancel();        /* 丢弃旧 CANCEL（read-and-clear） */
    comm_has_disconnect();           /* 丢弃旧 DISCONNECT（read-and-clear） */

    if (comm_has_invite()) {
        change_state(MP_STATE_WAITING);
        waiting_timer.last_tick = lv_tick_get();  /* 启动等待超时计时器 */
        if (can_accept_invite()) {
            dispatch_event(MP_EVENT_INVITE_RECEIVED, NULL);
        } else {
            mp_reject_invite(); /* 游戏进行中则自动拒绝 */
        }
    }
}

/**
 * @brief INVITING 状态：等待对方确认或超时
 */
static void handle_state_inviting(void)
{
    /* 检查是否收到邀请确认 */
    if (comm_has_invite_ack()) {
        bool accepted = comm_get_invite_ack();
        if (accepted) {
            change_state(MP_STATE_CONNECTED);
            dispatch_event(MP_EVENT_INVITE_ACCEPTED, NULL);
            dispatch_event(MP_EVENT_CONNECTED, NULL);
        } else {
            change_state(MP_STATE_IDLE);
            dispatch_event(MP_EVENT_INVITE_REJECTED, NULL);
        }
        return;
    }

    /* 检查邀请超时 */
    uint32_t now = lv_tick_get();
    if (now - invite_timer.last_tick >= MP_INVITE_TIMEOUT_MS) {
        comm_send_invite_cancel();  /* 通知对方取消 */
        change_state(MP_STATE_IDLE);
        dispatch_event(MP_EVENT_INVITE_TIMEOUT, NULL);
    }
}

/**
 * @brief WAITING 状态：等待本地用户通过 UI 做出选择，或检测对方取消/超时
 */
static void handle_state_waiting(void)
{
    /*
     * 用户将通过 UI 按钮调用 mp_accept_invite() 或 mp_reject_invite()，
     * 状态转移发生在那些函数中。
     */

    /* 1. 收到 INVITE_CANCEL → 对方取消或超时了 */
    if (comm_has_invite_cancel()) {
        change_state(MP_STATE_IDLE);
        dispatch_event(MP_EVENT_WAITING_TIMEOUT, NULL);
        return;
    }

    /* 2. 本地等待超时（对方 INVITE_CANCEL 丢包时的兜底） */
    uint32_t now = lv_tick_get();
    if (now - waiting_timer.last_tick >= MP_WAITING_TIMEOUT_MS) {
        change_state(MP_STATE_IDLE);
        dispatch_event(MP_EVENT_WAITING_TIMEOUT, NULL);
        return;
    }

    /* 3. 收到新的邀请帧 → 重置计时器，重新通知 UI */
    if (comm_has_invite()) {
        waiting_timer.last_tick = lv_tick_get();
        dispatch_event(MP_EVENT_INVITE_RECEIVED, NULL);
    }
}

/**
 * @brief CONNECTED 状态：监控连接，等待游戏开始或断开
 */
static void handle_state_connected(void)
{
    /* 收到对方 DISCONNECT 帧 → 同步断开 */
    if (comm_has_disconnect()) {
        change_state(MP_STATE_DISCONNECTED);
        dispatch_event(MP_EVENT_DISCONNECTED, NULL);
        return;
    }

    /* 防御：丢弃残留/意外的邀请和确认数据 */
    if (comm_has_invite()) { /* discard */ }
    if (comm_has_invite_ack()) { comm_get_invite_ack(); }

    /* 底层连接断开检测（心跳超时兜底） */
    comm_status_t st = comm_get_status();
    if (st == COMM_STATUS_DISCONNECTED || st == COMM_STATUS_ERROR) {
        change_state(MP_STATE_DISCONNECTED);
        dispatch_event(MP_EVENT_DISCONNECTED, NULL);
    }
}

/**
 * @brief GAME_STARTED 状态：游戏进行中，监控连接
 */
static void handle_state_game_started(void)
{
    /* 收到对方 DISCONNECT 帧 → 同步断开 */
    if (comm_has_disconnect()) {
        change_state(MP_STATE_DISCONNECTED);
        dispatch_event(MP_EVENT_DISCONNECTED, NULL);
        return;
    }

    /* 防御：丢弃残留/意外的邀请和确认数据 */
    if (comm_has_invite()) { /* discard */ }
    if (comm_has_invite_ack()) { comm_get_invite_ack(); }

    /* 底层连接断开检测（心跳超时兜底） */
    comm_status_t st = comm_get_status();
    if (st == COMM_STATUS_DISCONNECTED || st == COMM_STATUS_ERROR) {
        change_state(MP_STATE_DISCONNECTED);
        dispatch_event(MP_EVENT_DISCONNECTED, NULL);
    }
}

/**
 * @brief DISCONNECTED 状态：清理并回到 IDLE
 */
static void handle_state_disconnected(void)
{
    change_state(MP_STATE_IDLE);
}
