/**
 * @file ring_buffer.h
 */

#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

/*********************
 *      INCLUDES
 *********************/

#include <stdbool.h>
#include <stdint.h>

/**********************
 *      MACROS
 **********************/

#define RBUF_SIZE 256

/**********************
 *      TYPEDEFS
 **********************/

typedef struct
{
    uint8_t data[RBUF_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
} ring_buffer_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void ring_buffer_init(ring_buffer_t *rbuf);
bool ring_buffer_write(ring_buffer_t *rbuf, uint8_t data);
bool ring_buffer_read(ring_buffer_t *rbuf, uint8_t *data);
bool ring_buffer_is_empty(ring_buffer_t *rbuf);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif
