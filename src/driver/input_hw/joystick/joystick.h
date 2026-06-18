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

int16_t joystick_get_x(void);
int16_t joystick_get_y(void);

int16_t rjoystick_get_x(void);
int16_t rjoystick_get_y(void);

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
