/**
 * @file comm.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "comm.h"
#include "comm_rx.h"
#include "comm_tx.h"
#include "input_hw.h"
#include "protocol.h"
#include "tools.h"
#include "uart.h"
#include <string.h>
#include "config.h"
#include "lvgl.h" // for lvgl tick get

/**********************
 *      MACROS
 **********************/

#define CONNECTION_TIMEOUT_MS 5000
#define HEART_BEAT_INTERVAL_MS 2000 // 不能大于 CONNECTION_TIMEOUT_MS

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void update_connection_status();
static bool check_if_new_data();
static void heart_beat_f();

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static comm_status_t comm_status = COMM_STATUS_DISCONNECTED;
static char current_port[16] = DEFAULT_COM_PORT;
static uint32_t last_receive_time = 0;

static non_blocking_timer_t heart_beat_timer = {
    .delay_ms = HEART_BEAT_INTERVAL_MS,
    .func = heart_beat_f,
    .tick_get = lv_tick_get,
    .last_tick = 0,
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化comm
 */
void comm_init()
{
    comm_status = COMM_STATUS_DISCONNECTED;
    last_receive_time = 0;
    strncpy(current_port,DEFAULT_COM_PORT,sizeof(current_port) - 1);
    comm_rx_init();
}

/**
 * @brief 更新comm
 */
void comm_update()
{
    // 获取数据
    comm_rx_update();

    // 更新连接状态
    update_connection_status();

    // 更新心跳定时器
    if (comm_status == COMM_STATUS_CONNECTED || comm_status == COMM_STATUS_CONNECTING) {
        non_blocking_delay(&heart_beat_timer);
    }

    // 处理数据 now mainly logs
#ifdef SIMULATOR
#if DO_PC_PRINT_CONSOLE
    if (comm_has_new_log()) {
        char log_txt[256] = {0};
        comm_get_log(log_txt,sizeof(log_txt));
        printf("%s",log_txt); // 完整的log 即包含 (in xx.c line xx)
    }
#endif
#else

    if (comm_has_new_heartbeat_data()) {
        if (comm_status != COMM_STATUS_CONNECTED) {
            CONSOLE("[DEBUG] Received heartbeat data");
            uart_init(NULL,DEFAULT_BAUD_RATE);
        }
        comm_mcu_send_heart_beat_ack();
    }

#endif

    // 发送数据由其它模块负责调用相关函数
}

/**
 * @brief 连接comm
 * @param port 串口端口号 NULL则为默认端口 参见 config.h DEFAULT_COM_PORT
 * @param baud_rate 波特率
 * @return true 成功
 * @return false 失败
 * @note 连接成功后再调用会断开连接
 */
bool comm_connect(const char *port)
{
    // 断开连接 重新连接 包括错误 ？
    if (comm_status != COMM_STATUS_DISCONNECTED) {
        comm_disconnect();
    }

    // 计算端口
    if (port == NULL) {
        strncpy(current_port,DEFAULT_COM_PORT,sizeof(current_port) - 1);
    } else {
        strncpy(current_port,port,sizeof(current_port) - 1);
    }

    // 开始连接
    comm_status = COMM_STATUS_CONNECTING;
    if (uart_init(current_port,DEFAULT_BAUD_RATE)) {
        last_receive_time = lv_tick_get();
        return true;
    }

    comm_status = COMM_STATUS_ERROR;
    return false;
}

/**
 * @brief 断开comm连接
 */
void comm_disconnect()
{
    uart_deinit();
    comm_status = COMM_STATUS_DISCONNECTED;
    last_receive_time = 0;
}

/**
 * @brief 获取当前comm状态
 */
comm_status_t comm_get_status()
{
    return comm_status;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 更新连接状态
 */
static void update_connection_status(void)
{
    uint32_t current_time = lv_tick_get();
    
    if (check_if_new_data()) {
        comm_status = COMM_STATUS_CONNECTED;
        last_receive_time = current_time;
        //CONSOLE("[INFO] MCU connected!\n");
    }

    if (comm_status == COMM_STATUS_CONNECTED || comm_status == COMM_STATUS_CONNECTING) {
        if (current_time - last_receive_time > CONNECTION_TIMEOUT_MS) {
            CONSOLE("[WARNING] Connection timeout!");
            comm_disconnect();
        }
    }
}

/**
 * @brief 检查是否有新消息
 */
static bool check_if_new_data()
{
#ifdef SIMULATOR
    bool res = false;
    res = res || comm_has_new_key_data();
    res = res || comm_has_new_joystick_data();
    res = res || comm_has_new_log();
    res = res || comm_has_new_heartbeat_ack_data();
    return res;
#else
    bool res = false;
    res = res || comm_has_new_heartbeat_data();
    return res;
#endif
}

/**
 * @brief 心跳函数 负责定时发送心跳帧
 */
void heart_beat_f()
{
#ifdef SIMULATOR
    comm_pc_send_heart_beat();
#else

#endif
}
