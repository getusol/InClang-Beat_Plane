/**
 * @file comm_tx.h
 */

#ifndef __COMM_TX_H__
#define __COMM_TX_H__

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

void comm_send_key_state(uint8_t key_mask);
void comm_send_joystick(int16_t x, int16_t y);
void comm_mcu_send_log(const char *log_txt);
void comm_pc_send_heart_beat();
void comm_mcu_send_heart_beat_ack();
void comm_send_invite();
void comm_send_invite_ack(bool accept);
void comm_send_invite_cancel();
void comm_send_disconnect();
void comm_send_coin_sync(int32_t coin_num);

#endif // #ifndef __COMM_TX_H__
