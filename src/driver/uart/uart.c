/**
 * @file uart.h
 */

/*********************
 *      INCLUDES
 *********************/
#include "uart.h"
#include "debug.h"
#ifdef SIMULATOR
#include <stdio.h>
#include <windows.h>
#else
#include "drivers.h"
#endif      //#ifndef SIMULATOR

/**********************
 *  STATIC VARIABLES
 **********************/

// ring_buffer


#ifdef SIMULATOR
static bool uart_initialized = false;
#else
static bool uart_initialized = true; // 在MCU上默认初始化成功
#endif

#ifdef SIMULATOR
// window 串口句柄
static HANDLE hSerial = INVALID_HANDLE_VALUE;
#else

#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 串口初始化函数
 * @param baudrate 比特率 一般设置为 115200
 * @param port_name 串口名称 例："COM3"
 * @return true = 成功，false = 失败
 */
bool uart_init(const char * port_name,uint32_t baud_rate)
{
    #ifdef SIMULATOR
    (void) baud_rate;    //Unused
    char full_port_name[64];
    HANDLE temp_hSerial;
    CONSOLE("[INFO] Attempting to open serial port: %s.",port_name);
     temp_hSerial = CreateFile(port_name,
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             0,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             0);

    // 如果失败且端口号大于9，则尝试扩展格式
    if(temp_hSerial == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if(error == ERROR_FILE_NOT_FOUND && strstr(port_name, "COM")) {
            // 检查是否是 COM10 或更大的端口
            int port_num = 0;
            if(sscanf(port_name, "COM%d", &port_num) == 1 && port_num >= 10) {
                // 尝试使用扩展格式
                snprintf(full_port_name, sizeof(full_port_name), "\\\\.\\%s", port_name);
                CONSOLE("[INFO] Trying extended format: %s", full_port_name);
                
                temp_hSerial = CreateFile(full_port_name,
                                         GENERIC_READ | GENERIC_WRITE,
                                         0,
                                         0,
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL,
                                         0);
            }
        }
    }

    if(temp_hSerial == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        CONSOLE("[WARNING] Failed to open serial port: %s, Error code: %lu.", port_name, error);
        LOG("[WARNING] Failed to open serial port: %s, Error code: %lu.", port_name, error);
        return false;
    }

    // 配置串口参数
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if(!GetCommState(temp_hSerial, &dcbSerialParams)) {
        CONSOLE("[WARNING] Failed to get serial params.");
        LOG("[WARNING] Failed to get serial params.");
        CloseHandle(temp_hSerial);
        return false;
    }

    dcbSerialParams.BaudRate = baud_rate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if(!SetCommState(temp_hSerial, &dcbSerialParams)) {
        CONSOLE("[WARNING] Failed to set serial params.");
        LOG("[WARNING] Failed to set serial params.");
        CloseHandle(temp_hSerial);
        return false;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if(!SetCommTimeouts(temp_hSerial, &timeouts)) {
        CONSOLE("[WARNING] Failed to set serial timeouts.");
        LOG("[WARNING] Failed to set serial timeouts.");
        CloseHandle(temp_hSerial);
        return false;
    }

    //成功 保存句柄
    hSerial = temp_hSerial;

    uart_initialized = true;
    CONSOLE("[INFO] Serial port %s initialized at %lu baud.", port_name, baud_rate);
    #else
    uart_initialized = true;
    usart_receive_config(UART7, USART_RECEIVE_ENABLE);
    CONSOLE("[INFO] UART initialized.\n");
    #endif      //#ifdef SIMULATOR
    return true;
}

/**
 * @brief 断开串口通信 主要PC
 */
void uart_deinit(void)
{
#ifdef SIMULATOR
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        uart_initialized = false;
    }
#else
    if (uart_initialized) {
        uart_initialized = false;
        usart_receive_config(UART7, USART_RECEIVE_DISABLE);
    }
    CONSOLE("[INFO] UART deinitialized.\n");
#endif
    CONSOLE("[INFO] Serial port closed.");
}

/**
 * @brief 发送单个字节
 * @param byte 发送的数据
 * @note mainly for mcu, not for pc
 */
void uart_send_byte(uint8_t byte)
{
    #ifdef SIMULATOR
    if (uart_initialized && hSerial != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        BOOL success = WriteFile(hSerial, &byte, 1, &bytesWritten, NULL);
        if (!success || bytesWritten != 1) {
            DWORD error = GetLastError();
            CONSOLE("[WARNING] Failed to send byte 0x%02X, error: %lu", byte, error);
        }
        //CONSOLE("[DEBUG] Data sent on PC: 0x%02X", byte);
    }
    #else
    if (uart_initialized) {
        while(usart_flag_get(UART7,USART_FLAG_TC) == RESET);
        usart_data_transmit(UART7,byte);
        //CONSOLE("[DEBUG] Data sended on mcu!");
    } else {
        CONSOLE("[WARNING] UART not initialized, cannot send byte.");
    }
    #endif
}

/**
 * @brief 接收单个字节(blocking)
 * @return 接收到的字节 失败一般返回 0 
 * @note 主要用于单片机端，PC端无作用
 */
uint8_t uart_receive_byte(void)
{
    #ifdef SIMULATOR
    if (!uart_initialized) return 0;

    uint8_t byte = 0;
    DWORD bytesRead = 0;
    
    if(ReadFile(hSerial, &byte, 1, &bytesRead, NULL) && bytesRead == 1) {
        return byte;
    }
    return 0;
    #else
    if (uart_initialized) {
        while(usart_flag_get(UART7,USART_FLAG_RBNE) == RESET);
        return usart_data_receive(UART7);
    } else {
        CONSOLE("[WARNING] UART not initialized, cannot receive byte.");
        return 0;
    }
    #endif
}

/**
 * @brief 检查是否有数据可以接收
 * @return true = 有 ，false = 没有
 */
bool uart_receive_available(void)
{
    #ifdef SIMULATOR
    if (!uart_initialized) {
        //CONSOLE("[DEBUG] Serial not initialized.");
        return false;
    }

    if (hSerial == INVALID_HANDLE_VALUE) {
        CONSOLE("[DEBUG] Invalid serial handle.");
        return false;
    }

    DWORD dwErrors;
    COMSTAT comstat;

    if (ClearCommError(hSerial,&dwErrors,&comstat)) {
        bool result = comstat.cbInQue > 0;
        if (result) {
            //CONSOLE("[DEBUG] %lu bytes in queue.", comstat.cbInQue);
        }
        return result;
    }
    
    DWORD error = GetLastError();
    CONSOLE("[DEBUG] ClearCommError failed,error:%lu", error);
    return false;
    #else
    if (!uart_initialized) return false;
    return usart_flag_get(UART7,USART_FLAG_RBNE) != RESET;
    #endif
}

/**
 * @brief 某个单片机库文件需要这个函数，否则报错，PC端无用
 */
void __aeabi_assert(const char *expr, const char *file, int line)
{
    #ifdef SIMULATOR
    CONSOLE("[ERROR] Assert failed: %s, file: %s, line: %d", expr, file, line);
    LOG("[ERROR] Assert failed: %s, file: %s, line: %d", expr, file, line);
    return ;
    #else
    CONSOLE("[ERROR] Assert failed: %s, file: %s, line: %d\n", expr, file, line);
    LOG("[ERROR] Assert failed: %s, file: %s, line: %d", expr, file, line);
    while(1);
    #endif      //#ifdef SIMULATOR
}
