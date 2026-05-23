/**
 * @file timer.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "timer.h"
#include "config.h" // MAX_TIMER_COUNT 最大定时器数量
#include "pool.h"
#include "fsm.h"
#include "tools.h"
#include "game_object.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

struct timer_t
{
    bool active;               // 是否活跃
    uint32_t start_tick;       // 开始计时的时间戳
    uint32_t interval_ms;      // 间隔时间（毫秒）
    timer_mode_t mode;         // 计时模式
    timer_callback_t callback; // 回调函数
    void *usr_data;            // 用户数据
    game_obj_t * owner;        // 所属游戏对象 便于释放
    uint16_t pool_index;       // 在对象池中的索引
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

static timer_t timers[MAX_TIMER_COUNT] = {{0}}; // 定时器数组
static pool_t timer_pool = {0}; // 定时器对象池
static uint16_t timer_free_indices[MAX_TIMER_COUNT] = {0}; // 空闲定时器索引数组


 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化定时器对象池
 */
void timer_init(void)
{
    for (int i = 0;i < MAX_TIMER_COUNT;i++) {
        timers[i].pool_index = POOL_INVALID_ID;
        timers[i].active = false;
    }
    // 初始化定时器对象池
    pool_init(&timer_pool,timer_free_indices,MAX_TIMER_COUNT);
}

/**
 * @brief 创建定时器并开始（从创建的那一刻）
 * @param interval_ms 定时器间隔时间，单位毫秒
 * @param mode 定时器模式，单次或重复
 * @param callback 定时器回调函数
 * @param usr_data 定时器用户数据
 * @return timer_t* 定时器指针 失败返回 NULL
 */
timer_t * timer_create(game_obj_t * owner,uint32_t interval_ms,timer_mode_t mode,
                        timer_callback_t callback,void * usr_data)
{
    // 参数检查
    if (callback == NULL) return NULL;

    uint16_t id = pool_alloc(&timer_pool);
    if (id == POOL_INVALID_ID) {
        CONSOLE("[WARNING] Failed to create timer, no free index available.");
        LOG("[WARNING] Failed to create timer, no free index available.");
        return NULL;
    }

    // 初始化定时器
    timers[id].pool_index = id;
    timers[id].active = true;
    timers[id].interval_ms = interval_ms;
    timers[id].mode = mode;
    timers[id].callback = callback;
    timers[id].usr_data = usr_data;
    timers[id].start_tick = play_tick_get();
    timers[id].owner = owner;

    return &timers[id];
}

/**
 * @brief 更新定时器状态
 */
void timer_update()
{
    if (fsm_get_state() != GS_PLAY) return;

    uint32_t current_tick = play_tick_get();
    uint32_t elapsed_tick = 0;
    timer_t * t = NULL;

    for (int i = 0;i < MAX_TIMER_COUNT;i++) {

        t = &timers[i];
        if (!t->active) continue;

        // 检查所属游戏对象是否存在
        if (t->owner == NULL) {
            t->active = false;
            pool_free(&timer_pool,t->pool_index);
            continue;
        }
        // 检查所属游戏对象是否活跃
        if (!game_obj_is_active(t->owner)) {
            t->active = false;
            pool_free(&timer_pool,t->pool_index);
            continue;
        }

        elapsed_tick = current_tick - t->start_tick;

        if (elapsed_tick >= t->interval_ms) {
            // 触发回调
            if (t->callback) t->callback(t->owner,t->usr_data);
            // 单次触发 回收
            if (t->mode == TIMER_MODE_ONCE) {
                t->active = false;
                pool_free(&timer_pool,t->pool_index);
            // 重复触发 重置时间
            } else {
                t->start_tick = current_tick;
            }
        }
    }
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/
