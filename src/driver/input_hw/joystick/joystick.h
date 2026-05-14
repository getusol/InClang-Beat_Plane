/**
 * @file joystick.h
 * @note joystick -> js
 */

#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>

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

void joystick_init();
void joystick_scan(const uint8_t * ptr);

int16_t joystick_get_x();
int16_t joystick_get_y();

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
