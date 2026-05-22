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
#include "event.h"  // 💡 新增：引入事件系统头文件
#include <string.h>

/**********************
 * MACROS
 **********************/

#define COIN_LEAVE_IMG_NAME "coin_leave.bin"

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
static void coin_event_hit_player_cb(game_obj_t * src, game_obj_t * trg); // 💡 新增：事件回调函数声明

/**********************
 * STATIC VARIABLES
 **********************/
static pool_t coin_pool;
static uint16_t coin_free_indices[MAX_COIN_COUNT];
static coin_t coins[MAX_COIN_COUNT];

// 模仿 ui_play.c，只在仿真器模式下定义图片描述符结构体
#ifdef SIMULATOR
static lv_img_dsc_t coin_leave_img_struct;
#endif

// 引入 ui_play.c 中定义的全局金币变量
extern int coin_num;

/**********************
 * GLOBAL FUNCTIONS
 **********************/

void coin_init(lv_obj_t * parent)
{
    memset(coins, 0, sizeof(coins));
    pool_init(&coin_pool, coin_free_indices, MAX_COIN_COUNT);

    char coin_img_path[64];
    for (int i = 0; i < MAX_COIN_COUNT; i++) {
        // 基础属性初始化
        coins[i].base.active = false;
        coins[i].base.w = 18;
        coins[i].base.h = 18;
        coins[i].base.x = 0;
        coins[i].base.y = 0;
        
        // 碰撞箱初始化 (18*18 全覆盖)
        coins[i].base.hitbox_x = 0;
        coins[i].base.hitbox_y = 0;
        coins[i].base.hitbox_w = 20;
        coins[i].base.hitbox_h = 20;

        // 设置为标准金币枚举类型，以便底层主循环能识别并遍历它
        coins[i].base.type = GAME_OBJ_TYPE_COIN; 

        coins[i].pool_index = POOL_INVALID_ID;

        // 函数指针绑定
        coins[i].base.update = coin_update;
        coins[i].base.show = coin_show;
        coins[i].base.hide = coin_hide;

        // 模仿 ui_play.c 的图片加载逻辑
        #ifdef SIMULATOR
        // 创建 LVGL 图像对象：尺寸 18*18，最后一个参数设为 true
        coins[i].base.obj = img_create_from_dsc(parent, 
                                                img_path(COIN_LEAVE_IMG_NAME, coin_img_path, 64), 
                                                coins[i].base.w, 
                                                coins[i].base.h, 
                                                NULL, 
                                                &coin_leave_img_struct, 
                                                true);
        #else
        // 非仿真器环境下的 LVGL 原生图片创建与路径赋予
        coins[i].base.obj = lv_img_create(parent);
        lv_img_set_src(coins[i].base.obj, img_path(COIN_LEAVE_IMG_NAME, coin_img_path, 64));
        #endif

        // 默认隐藏并注册到游戏全局对象管理中
        coins[i].base.hide(&coins[i].base);
        game_register_obj(&coins[i].base);
    }

    // 💡 新增：向系统注册“玩家撞击金币”的事件回调函数
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

/**
 * @brief 金币每帧状态更新
 */
static void coin_update(game_obj_t * g)
{
    game_state_t game_state = fsm_get_state();
    if (game_state != GS_PLAY && game_state != GS_PAUSE) {
        g->hide(g);
        return;
    }
    if (game_state == GS_PAUSE) return;
    if (g->active == false) return;

    // 💡 优化：移除了原本混乱、手写的 AABB 碰撞检测。
    // 碰撞检测现在完全交给底层游戏主循环统一分发处理。
    // 如果以后金币需要添加“向下掉落”或“磁铁吸附”等移动效果，直接在此处编写位移逻辑即可。
}

/**
 * @brief 💡 新增：玩家撞击金币事件回调，用于触发加分和回收金币
 * @param src 触发源对象指针（通常为玩家）
 * @param trg 目标对象指针（即被吃掉的金币）
 */
static void coin_event_hit_player_cb(game_obj_t * src, game_obj_t * trg)
{
    if (trg == NULL) return;
    if (trg->active == false) return;

    // 发生接触：全局金币数值增加，金币隐藏并回收至对象池
    coin_num += 10;
    CONSOLE("[INFO] Coin collected via Event System! coin_num updated to: %d", coin_num);
    
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