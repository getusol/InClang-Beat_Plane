/**
 * @file uart.h
 */

/*********************
 *      INCLUDES
 *********************/
#include "uart.h"
#include "debug.h"
#include "ring_buffer.h"
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
static ring_buffer_t * uart_rx_buffer = NULL;

static bool uart_initialized = false;

#ifdef SIMULATOR
// window 串口句柄
static HANDLE hSerial = INVALID_HANDLE_VALUE;
static HANDLE hReadThread = NULL;
static volatile BOOL readThreadRunning = FALSE;
#else

#endif

/**********************
 *  STATIC PROTOTYPES
 **********************/

#ifdef SIMULATOR
static DWORD WINAPI SerialReadThread(LPVOID lpParam);
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
    uart_rx_buffer = NULL;
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
    //dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
    //dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;

    dcbSerialParams.fOutxCtsFlow = FALSE;    // 禁用 CTS 输出流控
    dcbSerialParams.fOutxDsrFlow = FALSE;    // 禁用 DSR 输出流控

    dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;   // 或者 DTR_CONTROL_ENABLE，但不影响流控
    dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;   // 禁用 RTS 流控
    dcbSerialParams.fOutX = FALSE;           // 禁用软件流控 (XON/XOFF)
    dcbSerialParams.fInX = FALSE;

    if(!SetCommState(temp_hSerial, &dcbSerialParams)) {
        CONSOLE("[WARNING] Failed to set serial params.");
        LOG("[WARNING] Failed to set serial params.");
        CloseHandle(temp_hSerial);
        return false;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    if(!SetCommTimeouts(temp_hSerial, &timeouts)) {
        CONSOLE("[WARNING] Failed to set serial timeouts.");
        LOG("[WARNING] Failed to set serial timeouts.");
        CloseHandle(temp_hSerial);
        return false;
    }

    uart_rx_buffer = ring_buffer_create();
    if (!uart_rx_buffer) {
        CONSOLE("[WARNING] Failed to create RX buffer.");
        LOG("[WARNING] Failed to create RX buffer.");
        CloseHandle(temp_hSerial);
        return false;
    }

    readThreadRunning = TRUE;
    hReadThread = CreateThread(NULL, 0, SerialReadThread, NULL, 0, NULL);
    if (hReadThread == NULL) {
        DWORD error = GetLastError();
        CONSOLE("[WARNING] Failed to create read thread, Error code: %lu.", error);
        LOG("[WARNING] Failed to create read thread, Error code: %lu.", error);
        CloseHandle(temp_hSerial);
        readThreadRunning = FALSE;
        return false;
    }

    //成功 保存句柄
    hSerial = temp_hSerial;

    PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);  // 同时清空读写缓冲区

    uart_initialized = true;
    CONSOLE("[INFO] Serial port %s initialized at %lu baud.", port_name, baud_rate);
    #else
    ring_buffer_clear(uart_rx_buffer); // 清空缓冲区，避免旧数据干扰
    CONSOLE("[INFO] UART initialized.");
    #endif      //#ifdef SIMULATOR
    return true;
}

/**
 * @brief 启用串口通信 主要MCU
 *         使能接收中断 并配置中断优先级
 */
void uart_enable(void)
{
#ifdef SIMULATOR

#else
    uart_initialized = true;
    // 1. 使能接收中断（RBNE）
    usart_interrupt_enable(UART7, USART_INT_RBNE);
    
    // 2. 配置NVIC中断优先级
    nvic_irq_enable(UART7_IRQn, 1, 0);   // 抢占优先级1，子优先级0
    
    // 3. 使能USART
    usart_enable(UART7);
    
    // 4. 初始化环形缓冲区
    uart_rx_buffer = ring_buffer_create();
    if (!uart_rx_buffer) {
        CONSOLE("[WARNING] Failed to create RX buffer.");
        LOG("[WARNING] Failed to create RX buffer.");
        return;
    }
#endif
}

/**
 * @brief 断开串口通信 主要PC
 */
void uart_deinit(void)
{
#ifdef SIMULATOR
    readThreadRunning = FALSE;
    if (hReadThread) {
        WaitForSingleObject(hReadThread, INFINITE);
        CloseHandle(hReadThread);
        hReadThread = NULL;
    }
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        uart_initialized = false;
    }
#else
    ring_buffer_clear(uart_rx_buffer); // 清空缓冲区，避免旧数据干扰
    CONSOLE("[INFO] UART deinitialized.");
#endif
    ring_buffer_destroy(uart_rx_buffer);
    uart_rx_buffer = NULL;
    CONSOLE("[INFO] Serial port closed.");
}

/**
 * @brief 发送单个字节
 * @param byte 发送的数据
 */
void uart_send_byte(uint8_t byte)
{
    #ifdef SIMULATOR
    if (uart_initialized && hSerial != INVALID_HANDLE_VALUE) {
        //CONSOLE("[DEBUG] uart_send_byte: About to send 0x%02X", byte);
        DWORD bytesWritten;
        BOOL success = WriteFile(hSerial, &byte, 1, &bytesWritten, NULL);
        if (!success || bytesWritten != 1) {
            DWORD error = GetLastError();
            CONSOLE("[WARNING] Failed to send byte 0x%02X, error: %lu", byte, error);
        }
        //CONSOLE("[DEBUG] uart_send_byte: Sent 0x%02X successfully", byte);
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
    uint8_t byte = 0;
    bool success = ring_buffer_read(uart_rx_buffer, &byte);
    if (success) return byte;
    //CONSOLE("[WARNING] No data available to read.");
    //LOG("[WARNING] No data available to read.");
    return 0xFF; // 或者其他错误指示值
    #else
    uint8_t byte;
    if (uart_initialized) {
        bool success = ring_buffer_read(uart_rx_buffer, &byte);
        if (success) {
            return byte;
        } else {
            CONSOLE("[WARNING] No data available to read.");
            LOG("[WARNING] No data available to read.");
            return 0xFF; // 或者其他错误指示值
        }
    } else {
        CONSOLE("[WARNING] UART not initialized, cannot receive byte.");
        LOG("[WARNING] UART not initialized, cannot receive byte.");
        return 0xFF; // 或者其他错误指示值
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
    return !ring_buffer_is_empty(uart_rx_buffer);
    #else
    if (!uart_initialized) return false;
    return !ring_buffer_is_empty(uart_rx_buffer);
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

/**
 * @brief UART7中断服务程序 主要MCU
 *         处理接收中断，读取数据并存入环形缓冲区
 */
#ifndef SIMULATOR
void UART7_IRQHandler(void) {
    // 处理接收数据寄存器非空标志（RBNE）
    if (usart_flag_get(UART7, USART_FLAG_RBNE) != RESET) {
        // 读取数据（硬件自动清除RBNE标志）
        uint8_t received = usart_data_receive(UART7);
        ring_buffer_write(uart_rx_buffer, received);
    }
    
    // 处理溢出错误（ORE）—— 避免发送卡死
    if (usart_flag_get(UART7, USART_FLAG_ORERR) != RESET) {
			uint8_t dummy __attribute__((unused)) = usart_data_receive(UART7);
    }
    
    // 如需更健壮，可类似处理 USART_FLAG_FE, USART_FLAG_NE
}
#endif

/**********************
 *   STATIC FUNCTIONS
 **********************/

#ifdef SIMULATOR
/**
 * @brief 接收线程，不断读取串口数据并存入环形缓冲区
 */
static DWORD WINAPI SerialReadThread(LPVOID lpParam)
{
    uint8_t buf[256];
    DWORD bytes_read;
    while (readThreadRunning) {
        // 阻塞读取，串口超时已在打开时设置
        if (ReadFile(hSerial, buf, sizeof(buf), &bytes_read, NULL)) {
            for (DWORD i = 0; i < bytes_read; i++) {
                ring_buffer_write(uart_rx_buffer, buf[i]);
            }
        } else {
            // 发生错误，稍作延时避免疯狂循环
            Sleep(10);
        }
    }
    return 0;
}
#endif
