/**
 * @file bullet.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "bullet.h"

#include "config.h"
#include "pool.h"
#include "tools.h"
#include "lvgl_utils.h"
#include "fsm.h"
#include "game.h"
#include "event.h"

/**********************
 *      MACROS
 **********************/

#define BULLET_MAX_X SCREEN_WIDTH  // 子弹最大X坐标
#define BULLET_MIN_X 0             // 子弹最小X坐标
#define BULLET_MAX_Y SCREEN_HEIGHT // 子弹最大Y坐标
#define BULLET_MIN_Y 0             // 子弹最小Y坐标

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief 子弹结构体
 */
typedef struct bullet_t
{
    game_obj_t base;     // 基对象
    int16_t damage;      // 子弹伤害
    game_obj_t *source;  // 子弹发射源
    uint16_t pool_index; // 对象池索引
    uint8_t flags;       // 特殊效果标志 BULLET_FLAG_*
} bullet_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void bullet_update(game_obj_t *g);
static void bullet_hide(game_obj_t *g);
static void bullet_show(game_obj_t *g);
static void bullet_move(game_obj_t *g);

static void bullet_event_hit_enemy_cb(game_obj_t *scr, game_obj_t *trg);
static void bullet_event_hit_player_cb(game_obj_t *scr, game_obj_t *trg);
static void bullet_event_hit_flame_wall_cb(game_obj_t *scr, game_obj_t *trg);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static bullet_t bullets[MAX_BULLET_COUNT];             // 子弹对象池
static pool_t bullet_pool;                             // 子弹对象池管理器
static uint16_t bullet_free_indices[MAX_BULLET_COUNT]; // 子弹对象池空闲索引栈
static bool enemy_slow_active = false;                 // 敌方子弹减速开关

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化子弹系统
 */
void bullet_init(lv_obj_t *parent)
{
    memset(bullets, 0, sizeof(bullets));
    // 初始化子弹对象池
    pool_init(&bullet_pool, bullet_free_indices, MAX_BULLET_COUNT);

    // 获取默认子弹外观
    apr_t *default_apr = apr_get(APR_BULLET_DEFAULT);

    for (uint16_t i = 0; i < MAX_BULLET_COUNT; i++)
    {
        bullets[i].base.active = false;
        bullets[i].base.apr = default_apr;
        bullets[i].base.speed = 0;
        bullets[i].base.x = 0;
        bullets[i].base.y = 0;
        bullets[i].base.vx = 0;
        bullets[i].base.vy = 0;
        bullets[i].base.type = GAME_OBJ_TYPE_BULLET;
        bullets[i].damage = 0;
        bullets[i].pool_index = POOL_INVALID_ID;
        bullets[i].base.update = bullet_update;
        bullets[i].base.show = bullet_show;
        bullets[i].base.hide = bullet_hide;
        bullets[i].source = NULL;
        bullets[i].base.behave = NULL_BEHAVE;

        // 使用 APR 创建 LVGL 图像对象
#ifdef SIMULATOR
        bullets[i].base.obj = lv_img_create(parent);
        lv_img_set_src(bullets[i].base.obj, &default_apr->img_dsc);
#else
        char path[128];
        bullets[i].base.obj = lv_img_create(parent);
        lv_img_set_src(bullets[i].base.obj, img_path(default_apr->img_name, path, 128));
#endif
        lv_obj_set_align(bullets[i].base.obj, LV_ALIGN_TOP_LEFT);

        bullets[i].base.hide((game_obj_t *)&bullets[i]);

        // obj register

        game_register_obj((game_obj_t *)&bullets[i]);
    }

    // event register
    event_register(EVENT_BULLET_HIT_ENEMY, bullet_event_hit_enemy_cb);
    event_register(EVENT_BULLET_HIT_PLAYER, bullet_event_hit_player_cb);
    event_register(EVENT_FLAME_WALL_HIT_BULLET, bullet_event_hit_flame_wall_cb);

    CONSOLE_INFO("Bullet system initialized with max bullet count: %d", MAX_BULLET_COUNT);
    return;
}

/**
 * @brief 创建子弹
 * @param source 子弹发射源
 * @param x 子弹初始x坐标
 * @param y 子弹初始y坐标
 * @param vx 子弹x轴速度
 * @param vy 子弹y轴速度
 * @param damage 子弹伤害
 * @param behave 子弹行为结构体
 * @param bullet_apr 子弹外观模板ID
 * @return 创建的子弹指针
 */
game_obj_t *bullet_create(game_obj_t *source,
                          lv_coord_t x, lv_coord_t y,
                          int16_t vx, int16_t vy,
                          int16_t damage,
                          behave_t behave,
                          apr_id_t bullet_apr)
{
    uint16_t index = pool_alloc(&bullet_pool);
    if (index == POOL_INVALID_ID)
    {
        CONSOLE_WARNING("No available bullet slots! Max bullet count: %d", MAX_BULLET_COUNT);
        LOG_WARNING("No available bullet slots! Max bullet count: %d", MAX_BULLET_COUNT);
        return NULL;
    }
    bullets[index].pool_index = index;
    bullets[index].source = source;
    bullets[index].damage = damage;
    bullets[index].flags = 0;
    bullets[index].base.vx = vx;
    bullets[index].base.vy = vy;
    bullets[index].base.x = x;
    bullets[index].base.y = y;
    bullets[index].base.behave = behave;

    // 应用子弹外观
    apr_apply(&bullets[index].base, bullet_apr);

    lv_obj_set_pos(bullets[index].base.obj, x, y);

    // 重置透明度（防止上次使用时残留的冰冻效果）
    lv_obj_set_style_opa(bullets[index].base.obj, LV_OPA_COVER, 0);

    bullets[index].base.show((game_obj_t *)&bullets[index]);

    // 如果减速激活中且来源是敌人，应用冰冻视觉效果
    if (enemy_slow_active && source != NULL && source->type == GAME_OBJ_TYPE_ENEMY)
    {
        lv_obj_set_style_opa(bullets[index].base.obj, LV_OPA_60, 0);
    }

    // CONSOLE_INFO("Bullet created at index: %d, position: (%d, %d), speed: %.2f, damage: %d", index, x, y, speed, damage);
    return (game_obj_t *)&bullets[index];
}

/**
 * @brief 获取子弹伤害
 */
int16_t bullet_get_damage(game_obj_t *bullet)
{
    return ((bullet_t *)bullet)->damage;
}

/**
 * @brief 获取子弹发射源
 */
game_obj_t *bullet_get_source(game_obj_t *g)
{
    if (g == NULL)
        return NULL;
    bullet_t *b = (bullet_t *)g;
    return (b->source);
}

/**
 * @brief 获取敌方子弹减速开关状态
 */
bool bullet_get_enemy_slow(void)
{
    return enemy_slow_active;
}

/**
 * @brief 设置子弹特殊效果标志
 */
void bullet_set_flags(game_obj_t *bullet, uint8_t flags)
{
    if (bullet == NULL)
        return;
    ((bullet_t *)bullet)->flags = flags;
}

/**
 * @brief 获取子弹特殊效果标志
 */
uint8_t bullet_get_flags(game_obj_t *bullet)
{
    if (bullet == NULL)
        return 0;
    return ((bullet_t *)bullet)->flags;
}

/**
 * @brief 设置子弹发射源（反射弹用）
 */
void bullet_set_source(game_obj_t *bullet, game_obj_t *source)
{
    if (bullet == NULL)
        return;
    ((bullet_t *)bullet)->source = source;
}

/**
 * @brief 设置敌方子弹减速开关
 */
void bullet_set_enemy_slow(bool enabled)
{
    if (enemy_slow_active == enabled)
        return;
    enemy_slow_active = enabled;

    // 批量应用/移除冰冻视觉效果到所有活跃敌方子弹
    for (int i = 0; i < MAX_BULLET_COUNT; i++)
    {
        bullet_apply_slow_visual(&bullets[i], enabled);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 单个子弹更新
 */
static void bullet_update(game_obj_t *g)
{
    if (fsm_get_state() != GS_PLAY && fsm_get_state() != GS_PAUSE && fsm_get_state() != GS_SETTING)
    {
        g->hide(g);
        return;
    }
    if (fsm_get_state() == GS_PAUSE || fsm_get_state() == GS_SETTING)
    {
        return;
    }
    if (!g->active)
    {
        return;
    }
    bullet_move(g);
}

/**
 * @brief 子弹隐藏 停止渲染+标记不活跃
 */
static void bullet_hide(game_obj_t *g)
{
    g->active = false;
    lv_obj_add_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
    g->timered = false;
    // 重置旋转（防止追踪弹角度残留到下次复用）
    lv_img_set_angle(g->obj, 0);
    lv_img_set_pivot(g->obj, 0, 0);
    // 归还内存
    bullet_t *b = (bullet_t *)g;
    if (b->pool_index == POOL_INVALID_ID)
    {
        return;
    }
    pool_free(&bullet_pool, b->pool_index);
    b->pool_index = POOL_INVALID_ID;
}

/**
 * @brief 子弹显示 激活对象+渲染 不分配内存
 */
static void bullet_show(game_obj_t *g)
{
    if (((bullet_t *)g)->pool_index == POOL_INVALID_ID)
    {
        CONSOLE_ERROR("Attempting to show a bullet that is not allocated! This should not happen.");
        LOG_ERROR("Attempting to show a bullet that is not allocated! This should not happen.");
        return;
    }
    g->active = true;
    lv_obj_clear_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief 子弹移动函数 包括超出边界的处理
 */
static void bullet_move(game_obj_t *g)
{
    if (g == NULL)
        return;
    if (g->active == false)
        return;

    int16_t effective_vx = g->vx;
    int16_t effective_vy = g->vy;

    // 敌方子弹减速: 减速时敌方子弹速度减半
    if (enemy_slow_active)
    {
        game_obj_t *src = bullet_get_source(g);
        if (src != NULL && src->type == GAME_OBJ_TYPE_ENEMY)
        {
            effective_vx /= 2;
            effective_vy /= 2;
        }
    }

    if (effective_vx == 0 && effective_vy == 0)
        return;

    g->x += effective_vx;
    g->y += effective_vy;

    lv_obj_set_pos(g->obj, g->x, g->y);

    // 检查是否超出边界
    if (g->x < BULLET_MIN_X || g->x > BULLET_MAX_X || g->y < BULLET_MIN_Y || g->y > BULLET_MAX_Y)
    {
        g->hide(g);
    }
}

/**
 * @brief 子弹击中敌人 子弹回调 子弹销毁
 * @param scr 子弹对象指针
 * @param trg 敌人对象指针
 */
static void bullet_event_hit_enemy_cb(game_obj_t *scr, game_obj_t *trg)
{
    scr->hide(scr);
}

/**
 * @brief 子弹击中玩家 子弹回调 子弹销毁
 * @param scr 子弹对象指针
 * @param trg 玩家对象指针
 */
static void bullet_event_hit_player_cb(game_obj_t *scr, game_obj_t *trg)
{
    if (!player_shield_is_active(trg))
    {
        scr->hide(scr);
    }
    // 反射弹
    scr->vx = -scr->vx;
    scr->vy = -scr->vy;
    ((bullet_t *)scr)->source = trg;
    scr->behave = NULL_BEHAVE;
}

/**
 * @brief 子弹击中火焰墙 子弹回调 子弹销毁
 */
static void bullet_event_hit_flame_wall_cb(game_obj_t *scr, game_obj_t *trg)
{
    trg->hide(trg);
}

/**
 * @brief 对单个子弹应用/移除减速冰冻视觉效果
 * @param b 子弹指针
 * @param apply true=应用冰冻效果, false=移除
 */
static void bullet_apply_slow_visual(bullet_t *b, bool apply)
{
    if (!b->base.active)
        return;
    game_obj_t *src = b->source;
    if (src == NULL || src->type != GAME_OBJ_TYPE_ENEMY)
        return;

    if (apply)
    {
        lv_obj_set_style_opa(b->base.obj, LV_OPA_60, 0);
    }
    else
    {
        lv_obj_set_style_opa(b->base.obj, LV_OPA_COVER, 0);
    }
}
