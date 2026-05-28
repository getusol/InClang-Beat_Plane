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
#include "comm_status.h"
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
static void print_mcu_log(void);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static char current_port[16] = DEFAULT_COM_PORT;
static uint32_t last_receive_time = 0;

static non_blocking_timer_t heart_beat_timer = {
    .delay_ms = HEART_BEAT_INTERVAL_MS,
    .func = comm_pc_send_heart_beat,
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
    comm_set_status(COMM_STATUS_DISCONNECTED);
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

    // 处理数据
    comm_handle_heartbeat();    // 处理心跳数据，可能会更新状态 即连接
    print_mcu_log();                  // 输出日志数据到控制台

    // 更新心跳定时器
    non_blocking_delay(&heart_beat_timer);

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
    if (comm_get_status() != COMM_STATUS_DISCONNECTED) {
        comm_disconnect();
    }

    // 计算端口
    if (port == NULL) {
        strncpy(current_port,DEFAULT_COM_PORT,sizeof(current_port) - 1);
    } else {
        strncpy(current_port,port,sizeof(current_port) - 1);
    }

    // 开始连接
    comm_set_status(COMM_STATUS_CONNECTING);
    if (uart_init(current_port,DEFAULT_BAUD_RATE)) {
        last_receive_time = lv_tick_get();
        return true;
    }

    comm_set_status(COMM_STATUS_ERROR);
    return false;
}

/**
 * @brief 断开comm连接
 */
void comm_disconnect()
{
    uart_deinit();
    comm_set_status(COMM_STATUS_DISCONNECTED);
    last_receive_time = 0;
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
    
    if (comm_has_heartbeat()) {
            last_receive_time = current_time;
    }

    if (comm_get_status() == COMM_STATUS_CONNECTED || comm_get_status() == COMM_STATUS_CONNECTING) {
        if (current_time - last_receive_time > CONNECTION_TIMEOUT_MS) {
            CONSOLE("[WARNING] Connection timeout!");
            comm_disconnect();
        }
    }
}

/**
 * @brief 从缓冲区读取日志并输出到控制台
 */
static void print_mcu_log(void)
{
#ifdef SIMULATOR
#if DO_PC_PRINT_CONSOLE
    if (comm_has_new_log()) {
        char log_txt[256] = {0};
        comm_get_log(log_txt,sizeof(log_txt));
        printf("[MCU]"); // 添加前缀区分MCU日志
        printf("%s",log_txt); // 完整的log 即包含 (in xx.c line xx)
    }
#endif
#endif
}
