/**
 * @file coin.c
 */

/*********************
 * INCLUDES
 *********************/

#include "coin.h"
#include "pool.h"
#include "config.h"
#include "lvgl_utils.h"
#include "game.h"
#include "tools.h"
#include "fsm.h"
#include "player.h"
#include "event.h"
#include "apr.h"
#include "timer.h"
#include <string.h>

/**********************
 * MACROS
 **********************/

#define COIN_IMG_NAME "coin.bin"

/**********************
 * TYPEDEFS
 **********************/

typedef struct {
    game_obj_t base;       // 继承自游戏基类
    uint16_t value;        // 该金币携带的金币值
    uint16_t pool_index;   // 对象池索引
} coin_t;

/**********************
 * STATIC PROTOTYPES
 **********************/

static void coin_update(game_obj_t * g);
static void coin_hide(game_obj_t * g);
static void coin_show(game_obj_t * g);
static void coin_event_hit_player_cb(game_obj_t * src, game_obj_t * trg);
static void coin_disappear_timer_cb(game_obj_t * owner, void * usr_data);

// 金币消失闪烁动画

static void coin_anim_opa_cb(void * var, int32_t value);
static void coin_anim_ready_cb(lv_anim_t * a);

/**********************
 * STATIC VARIABLES
 **********************/

// coin pool
static pool_t coin_pool;
static uint16_t coin_free_indices[MAX_COIN_COUNT];
static coin_t coins[MAX_COIN_COUNT];

// 金币总数（模块内部维护）
static int coin_num = 0;

/**********************
 * GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 金币初始化
 */
void coin_init(lv_obj_t * parent)
{
    memset(coins, 0, sizeof(coins));
    pool_init(&coin_pool, coin_free_indices, MAX_COIN_COUNT);

    apr_t *coin_apr = apr_get(APR_COIN_DEFAULT);

    for (int i = 0; i < MAX_COIN_COUNT; i++) {
        // base init
        coins[i].base.active = false;
        coins[i].base.type = GAME_OBJ_TYPE_COIN;
        coins[i].base.x = 0;
        coins[i].base.y = 0;
        coins[i].base.speed = 0;
        coins[i].base.vx = 0;
        coins[i].base.vy = 0;
        coins[i].base.behave = NULL_BEHAVE;
        coins[i].base.timered = false;

        coins[i].base.obj = lv_img_create(parent);

        apr_apply(&(coins[i].base),APR_COIN_DEFAULT);

        // special init
        coins[i].value = 0;
        coins[i].pool_index = POOL_INVALID_ID;

        // img & func ptr
        coins[i].base.update = coin_update;
        coins[i].base.show = coin_show;
        coins[i].base.hide = coin_hide;

        // 默认隐藏并注册
        coins[i].base.hide(&coins[i].base);
        game_register_obj(&coins[i].base);
    }

    // 注册事件回调
    event_register(EVENT_PLAYER_HIT_COIN, coin_event_hit_player_cb);

    CONSOLE_INFO("Coin system initialized with max count: %d.", MAX_COIN_COUNT);
}

/**
 * @brief 在指定坐标位置生成一枚金币
 * @param x                 金币生成的 X 坐标
 * @param y                 金币生成的 Y 坐标
 * @param value             该金币携带的金币值（被拾取时加到总数）
 * @param disappear_time_s  自动消失时间(s)，0 表示永不自动消失（最大 255s）
 * @return game_obj_t*      返回金币的游戏对象基类指针；若对象池已满则返回 NULL
 * @note 消失事件到后 还有一个持续时间1s的闪烁动画 之后才会消失
 */
game_obj_t * coin_spawn(lv_coord_t x, lv_coord_t y,
                        uint16_t value, uint8_t disappear_time_s,
                        apr_id_t apr_id)
{
    if (fsm_get_state() != GS_PLAY) return NULL;

    uint16_t id = pool_alloc(&coin_pool);
    if (id == POOL_INVALID_ID) {
        CONSOLE_WARNING("No available coin slots! Max coin count reached.");
        return NULL;
    }

    coin_t * c = &coins[id];
    c->pool_index = id;
    c->value = value;
    c->base.x = x;
    c->base.y = y;
    c->base.vx = 0;
    c->base.vy = 0;
    c->base.behave = NULL_BEHAVE;
    c->base.timered = false;

    apr_apply(&c->base, apr_id);

    lv_obj_set_pos(c->base.obj, x, y);
    c->base.show(&c->base);
    // 避免动画导致的不透明金币
    lv_obj_set_style_opa(c->base.obj, LV_OPA_COVER, 0);

    // 自动消失定时器 (0 表示永不消失)
    if (disappear_time_s > 0) {
        uint32_t interval_ms = (uint32_t)disappear_time_s * 1000;
        if (timer_create(&c->base, interval_ms, TIMER_MODE_ONCE,
                         coin_disappear_timer_cb, NULL) != NULL) {
            c->base.timered = true;
        }
    }

    return &c->base;
}

/**
 * @brief 获取当前总金币数
 */
int coin_get_num(void)
{
    return coin_num;
}

/**
 * @brief 相对增减金币数（正=拾取，负=花费），下限为 0(目前如此)
 * @param delta 变化量
 */
void coin_add_num(int delta)
{
    coin_num += delta;
    if (coin_num < 0) coin_num = 0;
}

/**
 * @brief 设置金币数为绝对值（若为负则设为 0）
 * @param value 目标值
 */
void coin_set_num(int value)
{
    coin_num = (value < 0) ? 0 : value;
}

/**********************
 * STATIC FUNCTIONS
 **********************/

static void coin_update(game_obj_t * g)
{
    game_state_t game_state = fsm_get_state();
    if (game_state != GS_PLAY && game_state != GS_PAUSE && game_state != GS_SETTING) {
        g->hide(g);
        return;
    }
    if (game_state == GS_PAUSE || game_state == GS_SETTING) return;
    if (g->active == false) return;

    // 碰撞检测已交给底层游戏主循环统一分发处理
    // 若未来需要金币移动效果（如下落、磁铁吸附），在此处添加位移逻辑
}

/**
 * @brief 玩家撞击金币事件回调，触发加分并回收金币
 * @param src 触发源对象指针（通常为玩家）
 * @param trg 目标对象指针（即被吃掉的金币）
 */
static void coin_event_hit_player_cb(game_obj_t * src, game_obj_t * trg)
{
    if (trg == NULL) return;
    if (trg->active == false) return;

    coin_t * c = (coin_t *)trg;
    coin_add_num(c->value);
    CONSOLE_INFO("Coin collected! value=%d total=%d", c->value, coin_num);

    trg->hide(trg);
}

/**
 * @brief 金币自动消失定时器回调 动画持续1s
 */
static void coin_disappear_timer_cb(game_obj_t * owner, void * usr_data)
{
    (void)usr_data;
    if (owner == NULL || !owner->active || owner->obj == NULL) return;

    // 创建金币闪烁并消失的动画
    // 持续 1s (1000ms)，闪烁 4 次。
    // 每次闪烁周期（淡出 + 淡入）占 250ms （125ms 渐隐 + 125ms 渐显）
    lv_anim_t a;
    lv_anim_init(&a);

    // 将整个 owner 结构体指针作为动画变量传入，便于在各回调函数中安全获取
    lv_anim_set_var(&a, owner);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP); // 从不透明到全透明

    lv_anim_set_time(&a, 125);               // 单向（淡出）耗时 125ms
    lv_anim_set_playback_time(&a, 125);      // 开启往返播放（淡入）耗时 125ms，组合为一个 250ms 周期
    lv_anim_set_repeat_count(&a, 4);         // 循环 4 次，总共刚好 1000ms (1秒)

    lv_anim_set_exec_cb(&a, coin_anim_opa_cb);
    lv_anim_set_ready_cb(&a, coin_anim_ready_cb);

    lv_anim_start(&a);
}

/**
 * @brief 金币消失动画透明度修改
 */
static void coin_anim_opa_cb(void * var, int32_t value)
{
    game_obj_t * owner = (game_obj_t *)var;
    // 增加防御性防空检查，防止动画执行期间对象已被意外销毁
    if (owner != NULL && owner->obj != NULL) {
        lv_obj_set_style_opa(owner->obj, (lv_opa_t)value, 0);
    }
}

/**
 * @brief 金币闪烁动画结束回调 触发隐藏
 */
static void coin_anim_ready_cb(lv_anim_t * a)
{
    game_obj_t * owner = (game_obj_t *)a->var;
    if (owner != NULL && owner->active) {
        // 闪烁 4 次结束后，真正让金币从地图上消失并回收
        owner->hide(owner);
    }
}

/**
 * @brief 金币消失逻辑
 */
static void coin_hide(game_obj_t * g)
{
    g->active = false;
    g->timered = false;
    lv_obj_add_flag(g->obj, LV_OBJ_FLAG_HIDDEN);

    coin_t * c = (coin_t *)g;
    if (c->pool_index != POOL_INVALID_ID) {
        pool_free(&coin_pool, c->pool_index);
        c->pool_index = POOL_INVALID_ID;
    }

    // 停止可能的lvgl动画
    lv_anim_del(g,NULL);
}

/**
 * @brief 金币显示
 */
static void coin_show(game_obj_t * g)
{
    coin_t * c = (coin_t *)g;
    if (c->pool_index == POOL_INVALID_ID) return;
    g->active = true;
    lv_obj_clear_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
}
