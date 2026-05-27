/**
 * @file ring_buffer.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "ring_buffer.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

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
 * @brief 初始化环形缓冲区
 * @param rbuf 环形缓冲区指针
 */
void ring_buffer_init(ring_buffer_t *rbuf)
{
    rbuf->head = 0;
    rbuf->tail = 0;
}

/**
 * @brief 向环形缓冲区写入数据
 * @param rbuf 环形缓冲区指针
 * @param data 要写入的数据
 * @return true = 成功，false = 失败
 */
bool ring_buffer_write(ring_buffer_t *rbuf, uint8_t data)
{
    uint32_t next_head = (rbuf->head + 1) % RBUF_SIZE;
    if (next_head == rbuf->tail) {
        // 缓冲区满，无法写入 丢弃数据
        return false;
    }
    rbuf->data[rbuf->head] = data;
    rbuf->head = next_head;
    return true;
}
bool ring_buffer_read(ring_buffer_t *rbuf, uint8_t *data);
bool ring_buffer_is_empty(ring_buffer_t *rbuf);

/**********************
 *   STATIC FUNCTIONS
 **********************/
