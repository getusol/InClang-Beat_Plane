/**
 * @file comm_status.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "comm_status.h"
#include "debug.h"

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

static comm_status_t comm_status = COMM_STATUS_DISCONNECTED;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 获取当前comm状态
 */
comm_status_t comm_get_status()
{
    return comm_status;
}

/**
 * @brief 设置当前comm状态
 */
void comm_set_status(comm_status_t status)
{
    if (status >= COMM_STATUS_MAX) {
        CONSOLE("[WARNING] Attempted to set invalid communication status: %d", status);
        LOG("[WARNING] Attempted to set invalid communication status: %d", status);
        return; // 无效状态，忽略
    }
    comm_status = status;
    return ;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
