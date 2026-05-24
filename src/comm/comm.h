/**
 * @file comm.h
 */

#ifndef __COMM_H__
#define __COMM_H__

/*********************
 *      INCLUDES
 *********************/

#include <stdbool.h>
#include <stdint.h>

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

// 串口通信连接状态
typedef enum
{
    COMM_STATUS_DISCONNECTED = 0, // 未连接
    COMM_STATUS_CONNECTING = 1,   // 连接中
    COMM_STATUS_CONNECTED = 2,    // 已连接
    COMM_STATUS_ERROR = 3         // 错误状态
} comm_status_t;

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void comm_init();
void comm_update();
bool comm_connect(const char *port, uint32_t baud_rate);
void comm_disconnect();
comm_status_t comm_get_status();

#endif // #ifndef __COMM_H__
