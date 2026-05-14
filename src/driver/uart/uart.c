/**
 * @file uart.h
 */

/*********************
 *      INCLUDES
 *********************/
#include "uart.h"
#include "tools.h"
#ifndef SIMULATOR
#include "drivers.h"
#endif      //#ifndef SIMULATOR
/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 单片机串口初始化函数，PC端无作用
 * @param baudrate 比特率 一般设置为 115200
 * @note 在PC端，此为空函数
 */
void uart_debug_init(uint32_t baudrate)
{
    #ifdef SIMULATOR
    (void) baudrate;    //Unused
    return ;
    #else
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART0);

    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_9);

    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_10);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_10);

    usart_deinit(USART0);
    usart_baudrate_set(USART0, baudrate);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);
    #endif      //#ifdef SIMULATOR
    console_out("[uart] Debug uart initialized.\n");
}
/**
 * @brief 某个单片机库文件需要这个函数，否则报错，PC端无用
 */
void __aeabi_assert(const char *expr, const char *file, int line)
{
    #ifdef SIMULATOR
    console_out("[uart] Assert failed: %s, file: %s, line: %d\n", expr, file, line);
    return ;
    #else
    console_out("[uart] Assert failed: %s, file: %s, line: %d\n", expr, file, line);
    while(1);
    #endif      //#ifdef SIMULATOR
}
