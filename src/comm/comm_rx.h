/**
 * @file comm_rx.h
 */

#ifndef __COMM_RX_H__
#define __COMM_RX_H__

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stdbool.h>

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

void comm_rx_init(void);
void comm_rx_update(void);

// getters get received and stored data

bool comm_has_new_key_data(void);
uint8_t comm_get_key_mask(void);
bool comm_has_new_joystick_data(void);
int16_t comm_get_joystick_x(void);
int16_t comm_get_joystick_y(void);
#ifdef SIMULATOR // PC
bool comm_has_new_log(void);
uint16_t comm_get_log(char *buffer, uint16_t buf_size);
#endif

bool comm_has_heartbeat(void);
void comm_handle_heartbeat(void);

bool comm_has_invite(void);
bool comm_has_invite_ack(void);
bool comm_get_invite_ack(void);
bool comm_has_invite_cancel(void);
bool comm_has_disconnect(void);
bool comm_has_coin_sync(void);
int32_t comm_get_coin_sync(void);

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif // #ifndef __COMM_RX_H__
