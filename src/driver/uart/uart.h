/**
 * @file uart.h
 * @note 单片机专用串口打印文件
 */
#ifndef __UART_H__
#define __UART_H__
/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void uart_debug_init(uint32_t baudrate);
void __aeabi_assert(const char *expr, const char *file, int line);

#endif // #ifndef __UART_H__
