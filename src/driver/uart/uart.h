/**
 * @file uart.h
 * @note UART 硬件驱动
 */

#ifndef __UART_H__
#define __UART_H__

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stdbool.h>
#ifdef SIMULATOR

#endif

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

bool uart_init(const char * port_name,uint32_t baud_rate);
void uart_send_byte(uint8_t byte);
uint8_t uart_receive_byte(void);
bool uart_receive_available(void);
void __aeabi_assert(const char *expr, const char *file, int line);

#endif // #ifndef __UART_H__
