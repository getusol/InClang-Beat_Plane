/**
 * @file flame_wall.c
 * @brief 火墙游戏对象实现 - Ember Y技能，向前发射火墙清除子弹并伤害敌人
 */

/*********************
 *      INCLUDES
 *********************/

#include "flame_wall.h"

#include "config.h"
#include "pool.h"
#include "tools.h"
#include "lvgl_utils.h"
#include "fsm.h"
#include "game.h"
#include "apr.h"

/**********************
 *      MACROS
 **********************/

#define FLAME_WALL_SPEED -8   // 火墙默认向上速度
#define FLAME_WALL_DAMAGE 20  // 火墙对敌人伤害

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    game_obj_t base;
    uint16_t pool_index;
    int16_t damage;           // 碰撞伤害
} flame_wall_t;

 /**********************
  *  STATIC PROTOTYPES
  **********************/

static void flame_wall_update(game_obj_t *g);
static void flame_wall_show(game_obj_t *g);
static void flame_wall_hide(game_obj_t *g);
static void flame_wall_move(game_obj_t *g);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static flame_wall_t flame_walls[MAX_FLAME_WALL_COUNT];
static pool_t fw_pool;
static uint16_t fw_free_indices[MAX_FLAME_WALL_COUNT];

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 火墙系统初始化
 */
void flame_wall_init(lv_obj_t *parent)
{
    memset(flame_walls, 0, sizeof(flame_walls));
    pool_init(&fw_pool, fw_free_indices, MAX_FLAME_WALL_COUNT);

    for (int i = 0; i < MAX_FLAME_WALL_COUNT; i++) {
        flame_walls[i].base.active = false;
        flame_walls[i].base.type = GAME_OBJ_TYPE_FLAME_WALL;
        flame_walls[i].base.x = 0;
        flame_walls[i].base.y = 0;
        flame_walls[i].base.vx = 0;
        flame_walls[i].base.vy = 0;
        flame_walls[i].base.behave = NULL_BEHAVE;
        flame_walls[i].pool_index = POOL_INVALID_ID;
        flame_walls[i].damage = FLAME_WALL_DAMAGE;
        flame_walls[i].base.update = flame_wall_update;
        flame_walls[i].base.show = flame_wall_show;
        flame_walls[i].base.hide = flame_wall_hide;
        flame_walls[i].base.obj = lv_img_create(parent);
        apr_apply(&flame_walls[i].base, APR_FLAME_WALL);

        lv_obj_set_align(flame_walls[i].base.obj, LV_ALIGN_TOP_LEFT);

        flame_walls[i].base.hide(&flame_walls[i].base);

        game_register_obj(&flame_walls[i].base);
    }

    CONSOLE_INFO("Flame wall system initialized with max count: %d", MAX_FLAME_WALL_COUNT);
    LOG_INFO("Flame wall system initialized with max count: %d", MAX_FLAME_WALL_COUNT);
}

/**
 * @brief 创建火墙对象
 * @param x 初始X坐标
 * @param y 初始Y坐标
 * @param vy Y轴速度（负值为向上）
 * @return 创建的火墙对象指针，失败返回NULL
 */
game_obj_t * flame_wall_create(lv_coord_t x, lv_coord_t y, int16_t vy)
{
    if (fsm_get_state() != GS_PLAY) return NULL;

    uint16_t id = pool_alloc(&fw_pool);
    if (id == POOL_INVALID_ID) {
        CONSOLE_WARNING("No available flame wall slots!");
        LOG_WARNING("No available flame wall slots!");
        return NULL;
    }

    flame_wall_t *fw = &flame_walls[id];
    fw->pool_index = id;
    fw->base.x = x;
    fw->base.y = y;
    fw->base.vx = 0;
    fw->base.vy = (vy != 0) ? vy : FLAME_WALL_SPEED;

    lv_obj_set_pos(fw->base.obj, x, y);
    fw->base.show(&fw->base);
    return &fw->base;
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 火墙更新函数
 */
static void flame_wall_update(game_obj_t *g)
{
    game_state_t gs = fsm_get_state();
    if (gs != GS_PLAY && gs != GS_PAUSE && gs != GS_SETTING) {
        g->hide(g);
        return;
    }
    if (gs == GS_PAUSE || gs == GS_SETTING) return;
    if (!g->active) return;
    flame_wall_move(g);
}

/**
 * @brief 火墙显示
 */
static void flame_wall_show(game_obj_t *g)
{
    flame_wall_t *fw = (flame_wall_t *)g;
    if (fw->pool_index == POOL_INVALID_ID) {
        CONSOLE_WARNING("Flame wall show failed: not allocated");
        LOG_WARNING("Flame wall show failed: not allocated");
        return;
    }
    g->active = true;
    lv_obj_clear_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief 火墙隐藏+回收
 */
static void flame_wall_hide(game_obj_t *g)
{
    g->active = false;
    lv_obj_add_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
    g->timered = false;

    flame_wall_t *fw = (flame_wall_t *)g;
    if (fw->pool_index != POOL_INVALID_ID) {
        pool_free(&fw_pool, fw->pool_index);
        fw->pool_index = POOL_INVALID_ID;
    }
}

/**
 * @brief 火墙移动
 */
static void flame_wall_move(game_obj_t *g)
{
    if (g == NULL || !g->active) return;

    g->x += g->vx;
    g->y += g->vy;
    lv_obj_set_pos(g->obj, g->x, g->y);

    // 超出屏幕边界销毁
    if (g->y < -20 || g->y > 620) {
        g->hide(g);
    }
}
