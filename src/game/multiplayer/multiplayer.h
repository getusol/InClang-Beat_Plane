/**
 * @file multiplayer.h
 */

#ifndef __MULTIPLAYER_H__
#define __MULTIPLAYER_H__

/*********************
 *      INCLUDES
 *********************/

#include "mp_state.h"
#include "mp_event.h"
#include <stdbool.h>

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void mp_init(void);
void mp_update(void);
mp_state_t mp_get_state(void);
bool mp_send_invite(void);
bool mp_accept_invite(void);
bool mp_reject_invite(void);
void mp_cancel_invite(void);
void mp_disconnect(void);
void mp_exit_game(void);
bool mp_start_game(void);
bool mp_event_register(mp_event_t event, mp_event_cb_t callback);
bool mp_event_unregister(mp_event_t event, mp_event_cb_t callback);

#endif // #ifndef __MULTIPLAYER_H__
