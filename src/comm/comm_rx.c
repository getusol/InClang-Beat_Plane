/**
 * @file comm_rx.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "comm_rx.h"
#include "protocol.h"
#include "comm_status.h"
#include "uart.h"
#include "debug.h"
#include <string.h>
#include "config.h"
#include <stdbool.h>

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

// 解析器状态
typedef enum {
    PARSER_WAITING_SOF = 0,    // 等待帧开始标志
    PARSER_WAITING_TYPE = 1,    // 等待帧类型
    PARSER_WAITING_LEN_HI = 2,  // 等待长度高字节
    PARSER_WAITING_LEN_LO = 3,  // 等待长度低字节
    PARSER_WAITING_DATA = 4,    // 等待数据
    PARSER_WAITING_CHECKSUM = 5, // 等待校验和
    PARSER_WAITING_EOF = 6,   // 等待帧结束标志（如果需要）
} parser_state_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static uint8_t calculate_checksum(const uint8_t *data,uint16_t len);
static bool data_process();
static bool read_escaped_byte(uint8_t * byte);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

// 解析器内部状态
static parser_state_t parser_state = PARSER_WAITING_SOF;
static uint8_t frame_type = 0;
static uint16_t frame_length = 0;
static uint8_t frame_data[256] = {0};
static uint8_t current_data_index = 0;
static uint8_t expected_checksum = 0;

// 解析结果存储

static bool new_heartbeat = false;

#ifdef SIMULATOR // PC
static uint8_t stored_key_mask = 0;
static int16_t stored_joystick_x = 0;
static int16_t stored_joystick_y = 0;
static char stored_log[256] = {0};
static uint16_t stored_log_len = 0;
// 新数据标志
static bool new_key_data = false;
static bool new_joystick_data = false;
static bool new_log_data = false;
#else           // MCU

#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化接收解释器
 */
void comm_rx_init()
{
    parser_state = PARSER_WAITING_SOF;
    memset(frame_data, 0, sizeof(frame_data));

#ifdef SIMULATOR
    memset(stored_log, 0, sizeof(stored_log));
    stored_key_mask = 0;
    stored_joystick_x = 0;
    stored_joystick_y = 0;
    stored_log_len = 0;
    new_key_data = false;
    new_joystick_data = false;
    new_log_data = false;
    
#else

#endif

    CONSOLE("[INFO] Communication receiver initialized.\n");
}

/**
 * @brief 更新通信状态（需要定期调用，如每5ms）
 *        读取串口数据并解析，更新内部存储的数据
 */
void comm_rx_update(void)
{
    uint8_t byte = 0x00;
    // 持续读取串口数据并解析
    while (uart_receive_available()) {

        bool success = read_escaped_byte(&byte);
        //CONSOLE("[DEBUG] Read byte:0x%02X (%d), parser_state:%d.", byte, byte, parser_state);
        
        if (!success) {
            CONSOLE("[WARNING] Failed to read escaped byte, resetting parser.");
            LOG("[WARNING] Failed to read escaped byte, resetting parser.");
            parser_state = PARSER_WAITING_SOF;
            continue;
        }
        
        switch (parser_state) {
            case PARSER_WAITING_SOF:
                if (byte == COMM_SOF) {
                    parser_state = PARSER_WAITING_TYPE;
                    //CONSOLE("[DEBUG] SOF 0x%02x read.", byte);
                }
                break;
                
            case PARSER_WAITING_TYPE:
                frame_type = byte;
                parser_state = PARSER_WAITING_LEN_HI;
                //CONSOLE("[DEBUG] Type 0x%02x read.",frame_type);
                break;
                
            case PARSER_WAITING_LEN_HI:
                frame_length = byte << 8;
                parser_state = PARSER_WAITING_LEN_LO;
                break;
                
            case PARSER_WAITING_LEN_LO:
                frame_length |= byte;
                current_data_index = 0;
                //CONSOLE("[DEBUG] Length read:%d.",frame_length);
                
                if (frame_length == 0) {
                    // 无数据帧，直接跳到校验和
                    parser_state = PARSER_WAITING_CHECKSUM;
                } else if (frame_length > sizeof(frame_data)) {
                    // 数据长度超出限制，重置解析器
                    CONSOLE("[WARNING] Frame length %d exceeds buffer size, resetting parser.", frame_length);
                    LOG("[WARNING] Frame length %d exceeds buffer size, resetting parser.", frame_length);
                    parser_state = PARSER_WAITING_SOF;
                } else {
                    parser_state = PARSER_WAITING_DATA;
                }
                break;
                
            case PARSER_WAITING_DATA:
                if (current_data_index < sizeof(frame_data)) {
                    frame_data[current_data_index++] = byte;
                    
                    if (current_data_index >= frame_length) {
                        parser_state = PARSER_WAITING_CHECKSUM;
                        //CONSOLE("[DEBUG] Data read: %s.",frame_data);
                    }
                }
                break;
                
            case PARSER_WAITING_CHECKSUM:
                expected_checksum = byte;
                
                // 验证校验和
                uint8_t calculated_checksum = calculate_checksum(frame_data, frame_length);
                if (calculated_checksum == expected_checksum) {
                    // 校验成功
                    //CONSOLE("[DEBUG] Checksum correct.");
                    parser_state = PARSER_WAITING_EOF; // 如果协议需要等待结束标志，可以设置为 PARSER_WAITING_EOF，否则直接处理数据
                } else {
                    CONSOLE("[WARNING] Checksum mismatch: expected 0x%02X, got 0x%02X", 
                           expected_checksum, calculated_checksum);
                    LOG("[WARNING] Checksum mismatch: expected 0x%02X, got 0x%02X", 
                           expected_checksum, calculated_checksum);
                    // 校验失败，重置解析器
                    parser_state = PARSER_WAITING_SOF;
                }
                
                break;

            case PARSER_WAITING_EOF:
                if (byte == COMM_EOF) {
                    // 成功接收完整帧，处理数据
                    //CONSOLE("[DEBUG] EOF 0x%02x received.",byte);
                    if (!data_process()) {
                        CONSOLE("[WARNING] Data processing failed for frame type 0x%02X", frame_type);
                        LOG("[WARNING] Data processing failed for frame type 0x%02X", frame_type);
                    }
                } else {
                    CONSOLE("[WARNING] Expected EOF but got 0x%02X, resetting parser.", byte);
                    LOG("[WARNING] Expected EOF but got 0x%02X, resetting parser.", byte);
                }
                // 无论成功与否，重置解析器准备接收下一帧
                parser_state = PARSER_WAITING_SOF;
                break;
                
            default:
                parser_state = PARSER_WAITING_SOF;
                break;
        }
    }
}

/**
 * @brief 检查是否有心跳数据
 * @return true=有心跳数据，false=无心跳数据
 */
bool comm_has_heartbeat(void)
{
    return new_heartbeat;
}

/**
 * @brief 处理心跳数据，调用后会清除心跳标志
 */
void comm_handle_heartbeat(void)
{
    if (!new_heartbeat) return;
    new_heartbeat = false;
    //CONSOLE("[DEBUG] Heartbeat processed!");
#ifdef SIMULATOR
    if (comm_get_status() == COMM_STATUS_CONNECTING) {
        comm_set_status(COMM_STATUS_CONNECTED);
        CONSOLE("[INFO] Connection established with MCU.");
        LOG("[INFO] Connection established with MCU.");
    }
#else
    if (comm_get_status() != COMM_STATUS_CONNECTED) {
        uart_init(NULL,DEFAULT_BAUD_RATE);
    }
    comm_mcu_send_heart_beat_ack();
    if (comm_get_status() == COMM_STATUS_DISCONNECTED || comm_get_status() == COMM_STATUS_CONNECTING) {
        comm_set_status(COMM_STATUS_CONNECTED);
    }
#endif
}

// getters
#ifdef SIMULATOR
/**
 * @brief 检查是否有新的按键数据
 * @return true=有新数据，false=无新数据
 */
bool comm_has_new_key_data(void)
{
    return new_key_data;
}

/**
 * @brief 获取最新的按键掩码
 * @return 按键掩码
 */
uint8_t comm_get_key_mask(void)
{
    new_key_data = false;  // 读取后清除标志
    return stored_key_mask;
}

/**
 * @brief 检查是否有新的摇杆数据
 * @return true=有新数据，false=无新数据
 */
bool comm_has_new_joystick_data(void)
{
    return new_joystick_data;
}

/**
 * @brief 获取最新的摇杆数据x轴
 * @return 摇杆数据x轴值
 */
int16_t comm_get_joystick_x(void)
{
    new_joystick_data = false;  // 读取后清除标志
    return stored_joystick_x;
}

/**
 * @brief 获取最新的摇杆数据y轴
 * @return 摇杆数据y轴值
 */
int16_t comm_get_joystick_y(void)
{
    new_joystick_data = false;  // 读取后清除标志
    return stored_joystick_y;
}

/**
 * @brief 检查是否有新的日志数据
 * @return true=有新数据，false=无新数据
 */
bool comm_has_new_log(void)
{
    return new_log_data;
}

/**
 * @brief 获取最新的日志数据
 * @param[out] buffer 存储日志的缓冲区
 * @param[in] buf_size 缓冲区大小
 * @return 实际复制的日志长度
 */
uint16_t comm_get_log(char *buffer, uint16_t buf_size)
{
    if (!buffer || buf_size == 0) return 0;
    
    uint16_t copy_len = (stored_log_len < buf_size - 1) ? stored_log_len : buf_size - 1;
    memcpy(buffer, stored_log, copy_len);
    buffer[copy_len] = '\0';
    
    new_log_data = false;  // 读取后清除标志
    return copy_len;
}

#else // MCU

#endif

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
 * @brief 对读取到的数据根据帧类型进行处理
 * @return true 如果数据处理成功，否则返回 false
 * @note 对所有帧状态处理 但有些帧状态只有某一个平台可以接收到
 */
static bool data_process()
{
    switch (frame_type) {
        case COMM_FRAME_KEY_STATE:
#ifdef SIMULATOR

            if (frame_length >= 1) {
                stored_key_mask = frame_data[0];
                new_key_data = true;
                //CONSOLE("[INFO] Success getting MCU key state: 0x%02X", stored_key_mask);
            }

#else
            CONSOLE("[WARNING] Key state frame received in non-simulator mode!");
            LOG("[WARNING] Key state frame received in non-simulator mode!");
            return false;
#endif
            break;
            
        case COMM_FRAME_JOYSTICK:
#ifdef SIMULATOR
            if (frame_length >= 4) {
                stored_joystick_x = (frame_data[1] << 8) | frame_data[0];
                stored_joystick_y = (frame_data[3] << 8) | frame_data[2];
                new_joystick_data = true;
                //CONSOLE("[INFO] Success getting MCU joystick: (%d, %d)", stored_joystick_x, stored_joystick_y);
            }
#else
            CONSOLE("[WARNING] Joystick frame received in non-simulator mode!");
            LOG("[WARNING] Joystick frame received in non-simulator mode!");
            return false;
#endif
            break;
            
        case COMM_FRAME_LOG:
#ifdef SIMULATOR
            if (frame_length <= sizeof(stored_log) - 1) {
                memcpy(stored_log, frame_data, frame_length);
                stored_log[frame_length] = '\0';
                stored_log_len = frame_length;
                new_log_data = true;
                //CONSOLE("[INFO] Success getting MCU log: %s", stored_log);
            }
#else
            CONSOLE("[WARNING] Log frame received in non-simulator mode!");
            LOG("[WARNING] Log frame received in non-simulator mode!");
            return false;
#endif
            break;
        
        case COMM_FRAME_HEART_BEAT:
#ifdef SIMULATOR
            CONSOLE("[WARNING] Heartbeat frame received in simulator mode!");
            LOG("[WARNING] Heartbeat frame received in simulator mode!");
            return false;
#else
            new_heartbeat = true;
            //CONSOLE("[DEBUG] Heartbeat received!");
#endif
            break;

        case COMM_FRAME_HEART_BEAT_ACK:
#ifdef SIMULATOR
            //CONSOLE("[DEBUG] Received heartbeat ACK frame from MCU.");
            new_heartbeat = true;
#else
            CONSOLE("[WARNING] Heartbeat ACK frame received in non-simulator mode!");
            LOG("[WARNING] Heartbeat ACK frame received in non-simulator mode!");
            return false;
#endif
            break;
            
        default:
            CONSOLE("[WARNING] Unknown frame type: 0x%02X", frame_type);
            LOG("[WARNING] Unknown frame type: 0x%02X", frame_type);
            return false;
    }
    return true;
}

/**
 * @brief 读取一个字节并处理转义序列
 * @param[out] byte 读取到的字节
 * @return 成功与否
 * @note 发送端保证转义序列完整性，所以 COMM_ESC 后一定有字节
 */
static bool read_escaped_byte(uint8_t * byte)
{
    uint8_t raw_byte = uart_receive_byte();
    if (raw_byte == COMM_ESC) {
        if (!uart_receive_available()) {
            CONSOLE("[WARNING] Incomplete escape sequence!");
            LOG("[WARNING] Incomplete escape sequence!");
            return false; // 转义序列不完整，读取失败 不修改 byte
        }

        raw_byte = uart_receive_byte() ^ COMM_ESC_XOR;
    }

    *byte = raw_byte;
    return true; // 成功读取普通字节
}
