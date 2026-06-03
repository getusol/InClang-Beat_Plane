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

#ifdef SIMULATOR
#define RBUF_SIZE 4096
#else
#define RBUF_SIZE 512 // bigger better
#endif

/**********************
 *      TYPEDEFS
 **********************/

typedef struct ring_buffer_t ring_buffer_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

ring_buffer_t *ring_buffer_create();
void ring_buffer_destroy(ring_buffer_t *rbuf);
void ring_buffer_clear(ring_buffer_t *rbuf);
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
