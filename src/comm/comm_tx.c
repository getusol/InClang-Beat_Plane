/**
 * @file comm_tx.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "comm_tx.h"
#include "protocol.h"
#include "uart.h"
#include <string.h>
#include <stdbool.h>
#include "config.h"
#include "comm_status.h"
#include "debug.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static uint8_t calculate_checksum(const uint8_t *data,uint16_t len);
static void send_escaped_byte(uint8_t byte);

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
 * @brief 发送按键状态
 * @param key_mask 按键位掩码
 *                 0000 0001 -> A
 *                 0000 0010 -> B
 *                 0000 0100 -> X
 *                 0000 1000 -> Y
 */
void comm_mcu_send_key_state(uint8_t key_mask)
{
#ifndef SIMULATOR
#if DO_MCU_SEND_INPUT
    uart_send_byte(COMM_SOF);
    send_escaped_byte(COMM_FRAME_KEY_STATE);
    send_escaped_byte(0x00); //长度高字节
    send_escaped_byte(0x01); //长度低字节（1字节）
    send_escaped_byte(key_mask); // 发送掩码
    uint8_t checksum = calculate_checksum(&key_mask, 1);
    send_escaped_byte(checksum);
    uart_send_byte(COMM_EOF);
#endif
#endif
}

/**
 * @brief 发送摇杆状态
 * @param x 摇杆X轴位置（-JOY_MAX_VALUE-JOY_MAX_VALUE）
 * @param y 摇杆Y轴位置（-JOY_MAX_VALUE-JOY_MAX_VALUE）
 */
void comm_mcu_send_joystick(int16_t x, int16_t y)
{
#ifndef SIMULATOR
#if DO_MCU_SEND_INPUT
    uint8_t data[4];
    data[0] = x & 0xFF;           // X 低字节
    data[1] = (x >> 8) & 0xFF;    // X 高字节
    data[2] = y & 0xFF;           // Y 低字节
    data[3] = (y >> 8) & 0xFF;    // Y 高字节

    uart_send_byte(COMM_SOF);
    send_escaped_byte(COMM_FRAME_JOYSTICK);
    send_escaped_byte(0x00); //长度高字节
    send_escaped_byte(0x04); //长度低字节（4字节）

    for (int i = 0; i < 4; i++) {
        send_escaped_byte(data[i]);
    }

    uint8_t checksum = calculate_checksum(data, 4);
    send_escaped_byte(checksum);
    uart_send_byte(COMM_EOF);
#endif
#endif
}

/**
 * @brief 发送日志
 * @param log_txt 日志字符串
 * @note WARNING:千万不要在里面调用 console_out 或 CONSOLE，否则会死循环，因为它们会调用这个函数来发送日志
 */
void comm_mcu_send_log(const char *log_txt)
{
#ifndef SIMULATOR
    if (!log_txt) return ;
#if DO_MCU_SEND_CONSOLE
    uint16_t len = strlen(log_txt);
    if (len > 255) len = 255; // 最大长度 255

    uart_send_byte(COMM_SOF);
    send_escaped_byte(COMM_FRAME_LOG);
    send_escaped_byte((len >> 8) & 0xFF); //长度高字节
    send_escaped_byte(len & 0xFF); //长度低字节
    for (uint16_t i = 0;i < len;i++) {
        send_escaped_byte(log_txt[i]);
    }
    uint8_t checksum = calculate_checksum((uint8_t *)log_txt, len);
    send_escaped_byte(checksum);
    uart_send_byte(COMM_EOF);
#endif
#endif
}

/**
 * @brief 发送邀请帧
 */
void comm_send_invite()
{
    // 发送帧头
    uart_send_byte(COMM_SOF);
    
    // 发送帧类型
    send_escaped_byte(COMM_FRAME_INVITE);
    
    // 发送长度（无数据）
    send_escaped_byte(0x00);  // 高字节
    send_escaped_byte(0x00);  // 低字节
    
    // 发送校验和（无数据时为0）
    send_escaped_byte(0x00);
    
    // 发送帧尾
    uart_send_byte(COMM_EOF);
}

/**
 * @brief 发送邀请确认帧
 * @param accept 是否接受邀请
 * @note 仅在收到邀请帧后调用
 */
void comm_send_invite_ack(bool accept)
{
    // 发送帧头
    uart_send_byte(COMM_SOF);
    send_escaped_byte(COMM_FRAME_INVITE_ACK);

    // 发送长度（1字节）
    send_escaped_byte(0x00);  // 高字节
    send_escaped_byte(0x01);  // 低字节
    
    uint8_t accept_byte = accept ? 0x01 : 0x00;
    // 发送是否接受邀请
    send_escaped_byte(accept_byte);
    
    // 发送校验和（1字节）
    uint8_t checksum = calculate_checksum((uint8_t *)&accept_byte, 1);
    send_escaped_byte(checksum);
    
    // 发送帧尾
    uart_send_byte(COMM_EOF);
}

/**
 * @brief 发送邀请取消帧
 * @note 当邀请方超时或主动取消时发送，通知接受方停止等待
 */
void comm_send_invite_cancel()
{
    uart_send_byte(COMM_SOF);
    send_escaped_byte(COMM_FRAME_INVITE_CANCEL);
    send_escaped_byte(0x00);  // 长度高字节
    send_escaped_byte(0x00);  // 长度低字节
    send_escaped_byte(0x00);  // 校验和（无数据时为0）
    uart_send_byte(COMM_EOF);
}

/**
 * @brief 发送断开联机帧
 * @note 当一方主动断开联机会话时发送，通知对方也退出联机状态
 */
void comm_send_disconnect()
{
    uart_send_byte(COMM_SOF);
    send_escaped_byte(COMM_FRAME_DISCONNECT);
    send_escaped_byte(0x00);  // 长度高字节
    send_escaped_byte(0x00);  // 长度低字节
    send_escaped_byte(0x00);  // 校验和（无数据时为0）
    uart_send_byte(COMM_EOF);
}

/**
 * @brief PC→MCU 发送 P2 金币同步帧
 * @param coin_num 当前 P2 金币数量
 */
void comm_send_coin_sync(int32_t coin_num)
{
    uint8_t data[4];
    data[0] = (uint8_t)(coin_num & 0xFF);
    data[1] = (uint8_t)((coin_num >> 8) & 0xFF);
    data[2] = (uint8_t)((coin_num >> 16) & 0xFF);
    data[3] = (uint8_t)((coin_num >> 24) & 0xFF);

    uart_send_byte(COMM_SOF);
    send_escaped_byte(COMM_FRAME_COIN_SYNC);
    send_escaped_byte(0x00);
    send_escaped_byte(0x04);
    for (int i = 0; i < 4; i++) send_escaped_byte(data[i]);
    send_escaped_byte(calculate_checksum(data, 4));
    uart_send_byte(COMM_EOF);
}

/**
 * @brief PC->MCU 发送心跳请求
 */
void comm_pc_send_heart_beat()
{
#ifdef SIMULATOR

    if (comm_get_status() != COMM_STATUS_CONNECTED && comm_get_status() != COMM_STATUS_CONNECTING) {
        return ;
    }

    //CONSOLE("[DEBUG] Sending heartbeat to MCU...");
    
    // 发送帧头
    uart_send_byte(COMM_SOF);
    
    // 发送帧类型
    send_escaped_byte(COMM_FRAME_HEART_BEAT);
    
    // 发送长度（无数据）
    send_escaped_byte(0x00);  // 高字节
    send_escaped_byte(0x00);  // 低字节
    
    // 发送校验和（无数据时为0）
    send_escaped_byte(0x00);
    
    // 发送帧尾
    uart_send_byte(COMM_EOF);

    //CONSOLE("[DEBUG] Heartbeat frame sent: 0x7B 0x%02X 0x00 0x00 0x00 0x7D", COMM_FRAME_HEART_BEAT);
#else
    return ;
#endif
}

/**
 * @brief MCU->PC 发送心跳确认帧
 */
void comm_mcu_send_heart_beat_ack()
{
#ifdef SIMULATOR
    return ;
#else
    // 发送帧头
    uart_send_byte(COMM_SOF);
    
    // 发送帧类型
    send_escaped_byte(COMM_FRAME_HEART_BEAT_ACK);
    
    // 发送长度（无数据）
    send_escaped_byte(0x00);  // 高字节
    send_escaped_byte(0x00);  // 低字节
    
    // 发送校验和（无数据时为0）
    send_escaped_byte(0x00);
    
    // 发送帧尾
    uart_send_byte(COMM_EOF);
    
    //CONSOLE("[DEBUG] Heartbeat ACK sent to PC");
#endif
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 计算校验和
 * @param data 数据指针
 * @param len 数据长度
 * @return 校验和
 * @note 校验和计算公式：
 *       校验和 = XOR(0, len-1, data[i])
 */
static uint8_t calculate_checksum(const uint8_t *data,uint16_t len)
{
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < len; i++)
    {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * @brief 发送转义字节
 */
static void send_escaped_byte(uint8_t byte)
{
    if (byte == COMM_ESC || byte == COMM_SOF || byte == COMM_EOF) {
        uart_send_byte(COMM_ESC);
        uart_send_byte(byte ^ COMM_ESC_XOR);
    } else {
        uart_send_byte(byte);
    }
}
