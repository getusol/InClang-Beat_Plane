/**
 * @file comm_status.h
 */

#ifndef __COMM_STATUS_H__
#define __COMM_STATUS_H__

/*********************
 *      INCLUDES
 *********************/

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
    COMM_STATUS_ERROR = 3,        // 错误状态

    COMM_STATUS_MAX, // 状态数量
} comm_status_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

comm_status_t comm_get_status();
void comm_set_status(comm_status_t status);

#endif
