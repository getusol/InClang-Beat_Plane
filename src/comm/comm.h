/**
 * @file comm.h
 */

#ifndef __COMM_H__
#define __COMM_H__

/*********************
 *      INCLUDES
 *********************/

#include <stdbool.h>
#include <stdint.h>

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void comm_init();
void comm_update();
bool comm_connect(const char *port);
void comm_disconnect();

#endif // #ifndef __COMM_H__
