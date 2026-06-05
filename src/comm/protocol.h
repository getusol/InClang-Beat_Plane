/**
 * @file protocol.h
 * @brief 通信协议定义 - PPP 协议
 */

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

/*********************
 *      INCLUDES
 *********************/

/**********************
 *      MACROS
 **********************/

// PPP 协议标准标记
#define COMM_SOF 0x7B    // 帧开始标志
#define COMM_EOF 0x7D      // 帧结束标志
#define COMM_ESC 0x7E     // 转义字符
#define COMM_ESC_XOR 0x20 // 转义字符 XOR 值

// 按键位掩码 A B X Y
#define COMM_KEY_A_MASK (1 << 0) // 0000 0001
#define COMM_KEY_B_MASK (1 << 1) // 0000 0010
#define COMM_KEY_X_MASK (1 << 2) // 0000 0100
#define COMM_KEY_Y_MASK (1 << 3) // 0000 1000

/**********************
 *      TYPEDEFS
 **********************/

// 帧通信类型
typedef enum
{
    COMM_FRAME_LOG = 0x01,           // 日志帧
    COMM_FRAME_KEY_STATE = 0x02,     // 按键状态帧
    COMM_FRAME_JOYSTICK = 0x03,      // 手柄数据帧
    COMM_FRAME_HEART_BEAT = 0x04,    // 心跳帧 PC->MCU
    COMM_FRAME_HEART_BEAT_ACK = 0x05, // 心跳确认帧 MCU->PC
    COMM_FRAME_INVITE = 0x06,        // 邀请帧 PC<->MCU
    COMM_FRAME_INVITE_ACK = 0x07,     // 邀请确认帧 MCU<->PC
    COMM_FRAME_INVITE_CANCEL = 0x08,  // 邀请取消帧（双向）
    COMM_FRAME_DISCONNECT = 0x09,     // 断开联机帧（双向）
} comm_frame_type_t;

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

#endif // #ifndef __PROTOCOL_H__
