/**
 * @file pool.h
 * @brief 通用对象池，通过空闲索引栈管理预分配的静态槽位
 * @note  pool 不关心槽位中存储的数据类型，只管理索引分配与回收
 */

#ifndef __POOL_H__
#define __POOL_H__

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stdbool.h>

/**********************
 *      MACROS
 **********************/
/** 无效的池子索引，表示池满 */
#define POOL_INVALID_ID 0xFFFF

/**********************
 *      TYPEDEFS
 **********************/

typedef struct
{
    uint16_t *free_stack; /* 外部提供的空闲索引数组 */
    uint16_t stack_top;   /* 栈顶指针（=当前可用的空闲索引数） */
    uint16_t capacity;    /* 池总容量 */
} pool_t;

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

void pool_init(pool_t *pool, uint16_t *free_stack, uint16_t capacity);
uint16_t pool_alloc(pool_t *pool);
void pool_free(pool_t *pool, uint16_t id);
uint16_t pool_free_count(pool_t *pool);

#endif // __POOL_H__
