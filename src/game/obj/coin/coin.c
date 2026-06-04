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
#include <string.h>

/**********************
 * MACROS
 **********************/

/**********************
 * TYPEDEFS
 **********************/
typedef struct {
    game_obj_t base;     // 继承自游戏基类
    uint16_t pool_index; // 对象池索引
} coin_t;

/**********************
 * STATIC PROTOTYPES
 **********************/
static void coin_update(game_obj_t * g);
static void coin_hide(game_obj_t * g);
static void coin_show(game_obj_t * g);
static void coin_event_hit_player_cb(game_obj_t * src, game_obj_t * trg);

/**********************
 * STATIC VARIABLES
 **********************/
static pool_t coin_pool;
static uint16_t coin_free_indices[MAX_COIN_COUNT];
static coin_t coins[MAX_COIN_COUNT];

extern int coin_num;

/**********************
 * GLOBAL FUNCTIONS
 **********************/

void coin_init(lv_obj_t * parent)
{
    memset(coins, 0, sizeof(coins));
    pool_init(&coin_pool, coin_free_indices, MAX_COIN_COUNT);

    apr_t *coin_apr = apr_get(APR_COIN_DEFAULT);

    for (int i = 0; i < MAX_COIN_COUNT; i++) {
        // 基础属性初始化
        coins[i].base.active = false;
        coins[i].base.apr = coin_apr;
        coins[i].base.x = 0;
        coins[i].base.y = 0;

        // 设置为标准金币枚举类型
        coins[i].base.type = GAME_OBJ_TYPE_COIN;

        coins[i].pool_index = POOL_INVALID_ID;

        // 函数指针绑定
        coins[i].base.update = coin_update;
        coins[i].base.show = coin_show;
        coins[i].base.hide = coin_hide;

        // 使用 APR 创建 LVGL 图像
#ifdef SIMULATOR
        coins[i].base.obj = lv_img_create(parent);
        lv_img_set_src(coins[i].base.obj, &coin_apr->img_dsc);
#else
        char path[128];
        coins[i].base.obj = lv_img_create(parent);
        lv_img_set_src(coins[i].base.obj, img_path(coin_apr->img_name, path, 128));
#endif

        // 默认隐藏并注册
        coins[i].base.hide(&coins[i].base);
        game_register_obj(&coins[i].base);
    }

    event_register(EVENT_PLAYER_HIT_COIN, coin_event_hit_player_cb);

    CONSOLE("[INFO] Coin system initialized with max count: %d.", MAX_COIN_COUNT);
}

game_obj_t * coin_spawn(lv_coord_t x, lv_coord_t y)
{
    if (fsm_get_state() != GS_PLAY) return NULL;

    uint16_t id = pool_alloc(&coin_pool);
    if (id == POOL_INVALID_ID) {
        CONSOLE("[WARNING] No available coin slots! Max coin count reached.");
        return NULL;
    }

    coin_t * c = &coins[id];
    c->pool_index = id;
    c->base.active = true;
    c->base.x = x;
    c->base.y = y;

    lv_obj_set_pos(c->base.obj, x, y);
    c->base.show(&c->base);

    return &c->base;
}

/**********************
 * STATIC FUNCTIONS
 **********************/

static void coin_update(game_obj_t * g)
{
    game_state_t game_state = fsm_get_state();
    if (game_state != GS_PLAY && game_state != GS_PAUSE) {
        g->hide(g);
        return;
    }
    if (game_state == GS_PAUSE) return;
    if (g->active == false) return;
}

static void coin_event_hit_player_cb(game_obj_t * src, game_obj_t * trg)
{
    if (trg == NULL) return;
    if (trg->active == false) return;

    coin_num += 10;
    CONSOLE("[INFO] Coin collected! coin_num: %d", coin_num);

    trg->hide(trg);
}

static void coin_hide(game_obj_t * g)
{
    g->active = false;
    lv_obj_add_flag(g->obj, LV_OBJ_FLAG_HIDDEN);

    coin_t * c = (coin_t *)g;
    if (c->pool_index != POOL_INVALID_ID) {
        pool_free(&coin_pool, c->pool_index);
        c->pool_index = POOL_INVALID_ID;
    }
}

static void coin_show(game_obj_t * g)
{
    coin_t * c = (coin_t *)g;
    if (c->pool_index == POOL_INVALID_ID) return;
    g->active = true;
    lv_obj_clear_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
}
