/**
 * @file player.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "lvgl.h"
#include "player.h"
#include "character.h"
#include "game_object.h"
#include "config.h"
#include "tools.h"
#include "pool.h"
#include "game.h"
#include "fsm.h"
#include "event.h"
#include "ui_play.h" // for hud layer
#include "bullet.h"  // for bullet damage calculation
#include "enemy.h"   // for enemy damage calculation
#include "coin.h"    // for coin value calculation

/**********************
 *      MACROS
 **********************/

#define PLAYER_MAX_X 960 // 玩家最大X坐标
#define PLAYER_MIN_X 0   // 玩家最小X坐标
#define PLAYER_MAX_Y 540 // 玩家最大Y坐标
#define PLAYER_MIN_Y 0   // 玩家最小Y坐标

// 工具宏
#define AS_PLAYER(player) ((player_t *)player)

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief 玩家结构体，继承自游戏对象，包含玩家特有的属性和方法
 */
typedef struct
{
    game_obj_t base; // 继承自游戏对象
    uint16_t pool_index;
    int16_t hp;
    lv_obj_t *hp_bar;                    // 生命值显示的lvgl对象指针
    int coin_count;                      // 玩家当前携带的金币数量
    const character_config_t *character; // 角色属性 包括最大生命值、射速、子弹参数、技能CD、技能函数、技能描述等
    // 护盾
    bool shield_active;
    lv_obj_t *shield_overlay;
    // 技能CD可视化用 — 上次使用时刻(play_tick)
    uint32_t skill_x_last_use;
    uint32_t skill_y_last_use;
    // 普攻CD追踪
    uint32_t fire_last_tick;
    // 是否在本局游戏开始时就已生成 (用于分数显示判断)
    bool was_active;
} player_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

// 行为函数
static void player_update(game_obj_t *g);
static void player_show(game_obj_t *g);
static void player_hide(game_obj_t *g);
static void player_move(game_obj_t *g);

// 事件函数
static void player_on_start(game_obj_t *g, game_obj_t *target);
static void player_on_death(game_obj_t *g, game_obj_t *target);
static void player_bullet_hit_player_cb(game_obj_t *src, game_obj_t *trg);
static void player_player_hit_enemy_cb(game_obj_t *src, game_obj_t *trg);
static void player_player_hit_coin_cb(game_obj_t *src, game_obj_t *trg);

// lvgl组件创建函数

static lv_obj_t *player_hp_bar_create(game_obj_t *g, lv_obj_t *parent, int index);
static lv_obj_t *player_shield_overlay_create(game_obj_t *g, lv_obj_t *parent);

/**********************
 *  STATIC VARIABLES
 **********************/

// 玩家池管理玩家
static pool_t player_pool;
static uint16_t player_free_indices[MAX_PLAYER_COUNT];
static player_t players[MAX_PLAYER_COUNT];

// 纪录当前存活玩家数量 需要在开始游戏和结束游戏、退出游戏时重置
static int active_player_count = 0; // 在player_create和player_on_death中使用，记录当前存活玩家数量

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化玩家 使用对象池管理玩家
 */
void player_init(lv_obj_t *parent)
{
    memset(players, 0, sizeof(players));
    pool_init(&player_pool, player_free_indices, MAX_PLAYER_COUNT);

    // 单个玩家初始化
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        players[i].base.behave = NULL_BEHAVE;
        players[i].base.type = GAME_OBJ_TYPE_PLAYER;
        players[i].base.obj = lv_img_create(parent);
        CONSOLE_DEBUG("Created player obj %d", i);
        players[i].hp_bar = player_hp_bar_create(&(players[i].base), ui_play_get_hud_layer(), i);
        CONSOLE_DEBUG("Created player hp bar %d", i);
        players[i].shield_overlay = player_shield_overlay_create(&(players[i].base), parent);
        CONSOLE_DEBUG("Created player shield overlay %d", i);
        player_character_set(&(players[i].base), PLAYER);
        CONSOLE_DEBUG("Set player character %d", i);
        players[i].base.hide = player_hide;
        players[i].base.show = player_show;
        players[i].base.update = player_update;

        players[i].base.hide(&(players[i].base));
        game_register_obj(&(players[i].base));
    }

    // 事件注册
    event_register(EVENT_PLAYER_DIE, player_on_death);
    event_register(EVENT_GAME_START, player_on_start);

    event_register(EVENT_BULLET_HIT_PLAYER, player_bullet_hit_player_cb);
    event_register(EVENT_PLAYER_HIT_ENEMY, player_player_hit_enemy_cb);
    event_register(EVENT_PLAYER_HIT_COIN, player_player_hit_coin_cb);

    CONSOLE_DEBUG("Player init done");
}

/**
 * @brief 生成玩家
 * @param x 玩家X坐标
 * @param y 玩家Y坐标
 * @param character_id 角色id
 * @param behave 行为函数指针 用来决定玩家的被动或者接入的设备
 * @note  behave.usr_data 须指向有效的 input_device_t (如 LOCAL / REMOTE)
 * @return game_obj_t* 玩家对象指针
 */
game_obj_t *player_spawn(lv_coord_t x, lv_coord_t y,
                         character_id_t character_id, behave_t behave)
{
    if (fsm_get_state() != GS_PLAY)
        return NULL;
    uint16_t id = pool_alloc(&player_pool);
    if (id == POOL_INVALID_ID)
    {
        CONSOLE_WARNING("No available player slots,max player count is %d", MAX_PLAYER_COUNT);
        LOG_WARNING("No available player slots,max player count is %d", MAX_PLAYER_COUNT);
        return NULL;
    }
    player_t *p = &players[id];
    p->pool_index = id;
    p->base.x = x;
    p->base.y = y;
    p->base.vx = 0;
    p->base.vy = 0;
    lv_obj_set_pos(p->base.obj, x, y);
    p->base.behave = behave;
    p->base.speed = 14;
    p->coin_count = 0;
    player_character_set(&(p->base), character_id);
    p->shield_active = false;
    p->skill_x_last_use = 0;
    p->skill_y_last_use = 0;
    p->fire_last_tick = 0;
    p->was_active = true;
    active_player_count++;
    p->base.show(&(p->base));
    return &(p->base);
}

/**
 * @brief 获取玩家基类指针根据对象池索引
 * @param pool_index 玩家对象池索引
 * @return game_obj_t* 玩家对象指针
 */
game_obj_t *player_get(uint16_t pool_index)
{
    if (pool_index >= MAX_PLAYER_COUNT)
        return NULL;
    return &players[pool_index].base;
}

/**
 * @brief 玩家角色指针设置
 * @param player 玩家对象
 * @param config 角色配置指针
 */
void player_character_set(game_obj_t *player, character_id_t id)
{
    if (player == NULL || id >= CHARACTER_ID_MAX)
        return;
    AS_PLAYER(player)->character = character_get_config(id);
    apr_apply(player, AS_PLAYER(player)->character->apr_id);
    player_hp_set(player, AS_PLAYER(player)->character->hp_max);
    lv_bar_set_range(AS_PLAYER(player)->hp_bar, 0, AS_PLAYER(player)->character->hp_max);
}

/**
 * @brief 获取玩家角色指针
 * @param player 玩家对象
 * @return const character_config_t* 角色配置指针
 */
const character_config_t *player_character_get(game_obj_t *player)
{
    if (player == NULL)
        return NULL;
    return AS_PLAYER(player)->character;
}

/**
 * @brief 获取玩家金币数量
 * @param player 玩家对象
 * @return int 金币数量
 */
int player_coin_count_get(game_obj_t *player)
{
    if (player == NULL)
    {
        CONSOLE_WARNING("Player is NULL,coin count is 0");
        return 0;
    }
    return AS_PLAYER(player)->coin_count;
}

/**
 * @brief 设置玩家生命值
 * @param player 玩家对象
 * @param hp 生命值
 */
void player_hp_set(game_obj_t *player, int hp)
{
    if (player == NULL)
        return;
    int16_t target_hp = (int16_t)hp;
    if (target_hp < 0)
        target_hp = 0;
    if (target_hp > AS_PLAYER(player)->character->hp_max)
        target_hp = AS_PLAYER(player)->character->hp_max;
    AS_PLAYER(player)->hp = target_hp;
    if (AS_PLAYER(player)->hp_bar != NULL)
        lv_bar_set_value(AS_PLAYER(player)->hp_bar, target_hp, LV_ANIM_OFF);
}

/**
 * @brief 修改玩家生命值
 * @param player 玩家对象
 * @param delta 生命值变化量
 */
void player_hp_modify(game_obj_t *player, int delta)
{
    if (player == NULL || delta == 0)
        return;
    int cur_hp = AS_PLAYER(player)->hp;
    player_hp_set(player, cur_hp + delta);
}

/**
 * @brief 检查玩家是否激活护盾
 * @param player 玩家对象
 * @return true 激活护盾
 * @return false 未激活护盾
 */
bool player_shield_is_active(game_obj_t *player)
{
    if (player == NULL)
        return false;
    return AS_PLAYER(player)->shield_active;
}

/**
 * @brief 设置玩家护盾激活状态
 * @param player 玩家对象
 * @param active 是否激活
 */
void player_shield_set_active(game_obj_t *player, bool active)
{
    if (player == NULL)
        return;
    AS_PLAYER(player)->shield_active = active;
}

/**
 * @brief 获取普攻上次发射时刻
 */
uint32_t player_fire_last_tick_get(game_obj_t *player)
{
    if (player == NULL)
        return 0;
    return AS_PLAYER(player)->fire_last_tick;
}

/**
 * @brief 设置普攻上次发射时刻
 */
void player_fire_last_tick_set(game_obj_t *player, uint32_t tick)
{
    if (player == NULL)
        return;
    AS_PLAYER(player)->fire_last_tick = tick;
}

/**
 * @brief 获取技能X上次使用时刻
 */
uint32_t player_skill_x_last_use_get(game_obj_t *player)
{
    if (player == NULL)
        return 0;
    return AS_PLAYER(player)->skill_x_last_use;
}

/**
 * @brief 设置技能X上次使用时刻
 */
void player_skill_x_last_use_set(game_obj_t *player, uint32_t tick)
{
    if (player == NULL)
        return;
    AS_PLAYER(player)->skill_x_last_use = tick;
}

/**
 * @brief 获取技能Y上次使用时刻
 */
uint32_t player_skill_y_last_use_get(game_obj_t *player)
{
    if (player == NULL)
        return 0;
    return AS_PLAYER(player)->skill_y_last_use;
}

/**
 * @brief 设置技能Y上次使用时刻
 */
void player_skill_y_last_use_set(game_obj_t *player, uint32_t tick)
{
    if (player == NULL)
        return;
    AS_PLAYER(player)->skill_y_last_use = tick;
}

bool player_was_active_get(game_obj_t *player)
{
    if (player == NULL) return false;
    return AS_PLAYER(player)->was_active;
}

void player_was_active_set(game_obj_t *player, bool val)
{
    if (player == NULL) return;
    AS_PLAYER(player)->was_active = val;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 更新玩家状态
 * @param g 玩家对象
 */
static void player_update(game_obj_t *g)
{
    game_state_t state = fsm_get_state();
    if (state != GS_PLAY && state != GS_PAUSE && state != GS_SETTING)
    {
        g->hide(g);
        return;
    }
    if (state != GS_PLAY)
        return;
    if (!(g->active))
    {
        g->hide(g);
        return;
    }

    // 显示由创建负责管理(即show)

    if (AS_PLAYER(g)->hp <= 0 && g->active)
        event_dispatch(EVENT_PLAYER_DIE, g, NULL);
    player_move(g);
}

/**
 * @brief 显示玩家
 * @param g 玩家对象
 */
static void player_show(game_obj_t *g)
{
    if (g == NULL)
        return;
    g->active = true;
    lv_obj_clear_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(AS_PLAYER(g)->hp_bar, LV_OBJ_FLAG_HIDDEN);
    // 血条装饰由游戏开始显示
    // 护盾由技能维护显示隐藏状态
}

/**
 * @brief 隐藏玩家
 * @param g 玩家对象
 */
static void player_hide(game_obj_t *g)
{
    if (g == NULL)
        return;
    g->active = false;
    lv_obj_add_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(AS_PLAYER(g)->hp_bar, LV_OBJ_FLAG_HIDDEN);
    // 血条装饰框不隐藏 由游戏结束统一隐藏
    g->timered = false;
}

/**
 * @brief 根据速度移动玩家
 * @param g 玩家对象
 */
static void player_move(game_obj_t *g)
{
    if (g == NULL)
        return;
    if (g->active == false)
        return;
    if (g->vx == 0 && g->vy == 0)
        return;

    g->x += g->vx;
    g->y += g->vy;

    // 边界检查
    if (g->x < PLAYER_MIN_X)
    {
        g->x = PLAYER_MIN_X;
    }
    if (g->x > PLAYER_MAX_X)
    {
        g->x = PLAYER_MAX_X;
    }
    if (g->y < PLAYER_MIN_Y)
    {
        g->y = PLAYER_MIN_Y;
    }
    if (g->y > PLAYER_MAX_Y)
    {
        g->y = PLAYER_MAX_Y;
    }

    lv_obj_set_pos(g->obj, g->x, g->y);
    // 护盾遮罩跟随
    if (AS_PLAYER(g)->shield_active)
    {
        lv_obj_set_pos(AS_PLAYER(g)->shield_overlay, g->x, g->y);
    }
}

/**
 * @brief 玩家死亡事件处理
 * @param g 玩家对象
 * @param target 事件目标对象
 */
static void player_on_death(game_obj_t *g, game_obj_t *target)
{
    LV_UNUSED(target);
    g->hide(g);
    if (active_player_count > 0)
        active_player_count--;
    if (active_player_count == 0)
    {
        fsm_switch_state(GS_OVER);
        event_dispatch(EVENT_GAME_OVER, NULL, NULL);
        CONSOLE_DEBUG("Game over by player death.");
    }
}

/**
 * @brief 游戏开始事件处理 统计活跃玩家数量
 * @param g 玩家对象
 * @param target 事件目标对象
 */
static void player_on_start(game_obj_t *g, game_obj_t *target)
{
    LV_UNUSED(target);
    LV_UNUSED(g);
    int cnt = 0;
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        if (players[i].base.active)
            cnt++;
    }
    active_player_count = cnt;
}

/**
 * @brief 玩家被子弹击中事件处理
 * @param src 子弹对象
 * @param trg 玩家对象
 */
static void player_bullet_hit_player_cb(game_obj_t *src, game_obj_t *trg)
{
    if (src == NULL || trg == NULL)
        return;
    int16_t damage = bullet_get_damage(src);
    player_hp_modify(trg, -damage);
}

/**
 * @brief 玩家被敌人击中事件处理
 * @param src 玩家对象
 * @param trg 敌人对象
 */
/**
 * @brief 玩家被敌人碰撞事件回调
 * @param src 玩家对象 (EVENT_PLAYER_HIT_ENEMY: src=玩家, trg=敌人)
 * @param trg 敌人对象
 */
static void player_player_hit_enemy_cb(game_obj_t *src, game_obj_t *trg)
{
    if (src == NULL || trg == NULL)
        return;
    int16_t damage = enemy_get_damage(trg);
    player_hp_modify(src, -damage);
}

/**
 * @brief 玩家被金币击中事件处理
 * @param src 玩家对象
 * @param trg 金币对象
 */
/**
 * @brief 玩家拾取金币事件回调
 * @param src 玩家对象 (EVENT_PLAYER_HIT_COIN: src=玩家, trg=金币)
 * @param trg 金币对象
 */
static void player_player_hit_coin_cb(game_obj_t *src, game_obj_t *trg)
{
    if (src == NULL || trg == NULL)
        return;
    int value = coin_get_value(trg);
    AS_PLAYER(src)->coin_count += value;
    CONSOLE_DEBUG("Player %p hit coin, add %d coins, now: %d",
                  src, value, AS_PLAYER(src)->coin_count);
}

/**
 * @brief 创建玩家HP条
 * @param g 玩家对象
 * @param parent 父对象
 * @param index 索引,决定条排列位置 间距 78px
 * @return lv_obj_t* HP条对象指针
 */
static lv_obj_t *player_hp_bar_create(game_obj_t *g, lv_obj_t *parent, int index)
{
    CONSOLE_DEBUG("Created player hp bar %d,parent: %p,game_obj:%p", index, parent, g);
    lv_obj_t *hp_bar = lv_bar_create(parent);
    lv_obj_set_size(hp_bar, 162, 18);
    lv_obj_set_align(hp_bar, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(hp_bar, 21, 18 + 78 * index);
    // 由character_set负责设置
    // lv_bar_set_range(hp_bar, 0, AS_PLAYER(g)->character->hp_max);
    // lv_bar_set_value(hp_bar, AS_PLAYER(g)->hp, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(hp_bar, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
    lv_obj_add_flag(hp_bar, LV_OBJ_FLAG_HIDDEN);
    return hp_bar;
}

/**
 * @brief 创建玩家护盾覆盖层
 * @param g 玩家对象
 * @param parent 父对象
 * @return lv_obj_t* 护盾覆盖层对象指针
 */
static lv_obj_t *player_shield_overlay_create(game_obj_t *g, lv_obj_t *parent)
{
    lv_obj_t *shield_overlay = lv_obj_create(parent);
    lv_obj_set_size(shield_overlay, 64, 64);
    lv_obj_set_style_bg_color(shield_overlay, lv_color_make(200, 200, 200), 0);
    lv_obj_set_style_bg_opa(shield_overlay, LV_OPA_50, 0);
    lv_obj_set_style_border_width(shield_overlay, 0, 0);
    lv_obj_add_flag(shield_overlay, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_pos(shield_overlay, g->x, g->y);
    return shield_overlay;
}
