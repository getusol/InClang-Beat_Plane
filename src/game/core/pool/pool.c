/**
 * @file pool.c
 * @brief 通用对象池实现
 *        通过空闲索引栈管理预分配的静态槽位
 *        pool 不关心槽位中存储的数据类型，只管理索引分配与回收
 */

/*********************
 *      INCLUDES
 *********************/
#include "pool.h"
#include "tools.h"

/*********************
 *   GLOBAL FUNCTIONS
 *********************/

/**
 * @brief 初始化池管理器
 * @param pool       池结构体指针
 * @param free_stack 外部静态分配的 uint16_t 数组，容量 = capacity
 * @param capacity   池的总槽位数
 */
void pool_init(pool_t *pool, uint16_t *free_stack, uint16_t capacity)
{
    if (pool == NULL || free_stack == NULL || capacity == 0) {
        console_out("[Warning][pool] init failed: invalid params\n");
        log_out("[Warning][pool] init failed: invalid params");
        return;
    }

    pool->free_stack = free_stack;
    pool->capacity   = capacity;
    pool->stack_top  = capacity;          /* 初始时全部槽位都空闲 */

    /* 将索引 0, 1, 2, ..., capacity-1 依次压入栈 */
    for (uint16_t i = 0; i < capacity; i++) {
        free_stack[i] = i;
    }

    console_out("[pool] init ok, capacity=%d\n", capacity);
}

/**
 * @brief 从池中申请一个空闲槽位的索引
 * @param  pool 池结构体指针
 * @return 空闲槽位索引，若池满返回 POOL_INVALID_ID
 */
uint16_t pool_alloc(pool_t *pool)
{
    if (pool == NULL) {
        console_out("[Warning][pool] alloc failed: invalid params\n");
        log_out("[Warning][pool] alloc failed: invalid params");
        return POOL_INVALID_ID;
    }

    /* 栈空 → 池满，无可用槽位 */
    if (pool->stack_top == 0) {
        console_out("[Warning][pool] full, cannot alloc\n");
        log_out("[Warning][pool] full, cannot alloc");
        return POOL_INVALID_ID;
    }

    /* 弹出栈顶索引，返回给调用者 */
    return pool->free_stack[--pool->stack_top];
}

/**
 * @brief 归还槽位索引，使之重新变为空闲
 * @param pool 池结构体指针
 * @param id   要归还的槽位索引
 */
void pool_free(pool_t *pool, uint16_t id)
{
    if (pool == NULL || id >= pool->capacity) {
        console_out("[Warning][pool] free failed: invalid params\n");
        log_out("[Warning][pool] free failed: invalid params");
        return;
    }

    /* 防御：检查该 id 是否已经在空闲栈中（防止重复归还） */
    for (uint16_t i = 0; i < pool->stack_top; i++) {
        if (pool->free_stack[i] == id) {
            console_out("[pool] id %d already free, skip\n", id);
            return;
        }
    }

    /* 将 id 压回栈 */
    pool->free_stack[pool->stack_top++] = id;
}

/**
 * @brief 查询池中剩余空闲槽位数
 * @param  pool 池结构体指针
 * @return 剩余空闲槽位数
 */
uint16_t pool_free_count(pool_t *pool)
{
    if (pool == NULL) {
        return 0;
    }
    return pool->stack_top;
}
