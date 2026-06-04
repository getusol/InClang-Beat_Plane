/**
 * @file ring_buffer.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "ring_buffer.h"
#include <stdlib.h>
#include "debug.h"
#ifdef SIMULATOR
#include "windows.h"
#else
#include "drivers.h"
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

struct ring_buffer_t
{
    uint8_t data[RBUF_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
#ifdef SIMULATOR
    CRITICAL_SECTION lock; // Windows下的临界区对象
#endif
};

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

/**
 * @brief 创建一个新的环形缓冲区实例 使用malloc分配内存，需要调用者负责释放
 * @return 新创建的环形缓冲区实例指针，失败返回NULL
 */
ring_buffer_t *ring_buffer_create()
{
    ring_buffer_t *rbuf = (ring_buffer_t *)malloc(sizeof(ring_buffer_t));
    if (!rbuf) {
        CONSOLE("[WARNING] Failed to create ring buffer: Out of memory.");
        LOG("[WARNING] Failed to create ring buffer: Out of memory.");
        return NULL;
    }
    memset(rbuf, 0, sizeof(ring_buffer_t));
#ifdef SIMULATOR
    InitializeCriticalSection(&rbuf->lock);
#endif
    return rbuf;
}

/**
 * @brief 关闭临界区对象
 * @param rbuf 环形缓冲区指针
 */
void ring_buffer_destroy(ring_buffer_t *rbuf)
{
    if (!rbuf) return;
#ifdef SIMULATOR
    DeleteCriticalSection(&rbuf->lock);
#endif
    free(rbuf);
}

/**
 * @brief 清空环形缓冲区
 * @param rbuf 环形缓冲区指针
 */
void ring_buffer_clear(ring_buffer_t *rbuf)
{
    if (!rbuf) return;
#ifdef SIMULATOR
    EnterCriticalSection(&rbuf->lock);
#else
    //__disable_irq(); // 禁止中断，确保操作原子性
#endif
    // memset(rbuf->data, 0, RBUF_SIZE); // 可选：根据需要是否清零数据区
    rbuf->head = 0;
    rbuf->tail = 0;
#ifdef SIMULATOR
    LeaveCriticalSection(&rbuf->lock);
#else
    //__enable_irq(); // 恢复中断
#endif
}

/**
 * @brief 向环形缓冲区写入数据
 * @param rbuf 环形缓冲区指针
 * @param data 要写入的数据
 * @return true = 成功，false = 失败
 */
bool ring_buffer_write(ring_buffer_t *rbuf, uint8_t data)
{
#ifdef SIMULATOR
    EnterCriticalSection(&rbuf->lock);
#endif
    uint32_t next_head = (rbuf->head + 1) % RBUF_SIZE;
    if (next_head == rbuf->tail) {
        // 缓冲区满，无法写入 丢弃数据
        return false;
    }
    rbuf->data[rbuf->head] = data;
    rbuf->head = next_head;
#ifdef SIMULATOR
    LeaveCriticalSection(&rbuf->lock);
#endif
    return true;
}

/**
 * @brief 从环形缓冲区读取数据
 * @param[in] rbuf 环形缓冲区指针
 * @param[out] data 读取到的数据指针
 * @return true = 成功，false = 失败
 */
bool ring_buffer_read(ring_buffer_t *rbuf, uint8_t *data)
{
#ifdef SIMULATOR
    EnterCriticalSection(&rbuf->lock);
#endif
    if (rbuf->head == rbuf->tail) {
        // 缓冲区空，无法读取
        return false;
    }
    *data = rbuf->data[rbuf->tail];
    rbuf->tail = (rbuf->tail + 1) % RBUF_SIZE;
#ifdef SIMULATOR
    LeaveCriticalSection(&rbuf->lock);
#endif
    return true;
}

/**
 * @brief 检查环形缓冲区是否为空
 * @param rbuf 环形缓冲区指针
 * @return true = 空，false = 非空
 */
bool ring_buffer_is_empty(ring_buffer_t *rbuf)
{
#ifdef SIMULATOR
    EnterCriticalSection(&rbuf->lock);
#endif
    bool empty = (rbuf->head == rbuf->tail);
#ifdef SIMULATOR
    LeaveCriticalSection(&rbuf->lock);
#endif
    return empty;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
