/**
 * @file enemy.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "enemy.h"
#include "enemy_behaviors.h"
#include "pool.h"
#include "config.h"
#include "lvgl_utils.h"
#include "game.h"
#include "tools.h"
#include "fsm.h"
#include "event.h"
#include "bullet.h"
#include "coin.h"
#include "player.h"
#include "timer.h"
#include "flame_wall.h"
#include "audio.h"

/**********************
 *      MACROS
 **********************/

#define ENEMY_MAX_X 980 // 敌人最大X坐标
#define ENEMY_MIN_X 0   // 敌人最小X坐标
#define ENEMY_MAX_Y 600 // 敌人最大Y坐标
#define ENEMY_MIN_Y -64 // 敌人最小Y坐标

/**********************
 *      TYPEDEFS
 **********************/

typedef struct
{
  game_obj_t base;
  int16_t hp; // 生命值
  int16_t hp_max;
  int16_t damage;
  uint16_t pool_index; // 对象池索引
  lv_obj_t *health_bar;
  bool is_boss; // 是否为 Boss
  // 状态效果
  bool burning;             // 是否处于灼烧状态
  int8_t burn_ticks_left;   // 剩余灼烧次数
  bool frozen;              // 是否处于冻结状态
  lv_obj_t *freeze_overlay; // 冻结蓝色遮罩
} enemy_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void enemy_update(game_obj_t *g);
static void enemy_hide(game_obj_t *g);
static void enemy_show(game_obj_t *g);
static void enemy_move(game_obj_t *g);
static int16_t enemy_modify_hp(game_obj_t *g, int16_t delta);
static uint16_t enemy_modify_hp_max(game_obj_t *g, uint16_t trg);

static void enemy_event_hit_by_bullet_cb(game_obj_t *scr, game_obj_t *trg);
static void enemy_event_hit_player_cb(game_obj_t *src, game_obj_t *trg);
static void enemy_event_hit_by_flame_wall_cb(game_obj_t *src, game_obj_t *trg);
static void enemy_on_destroyed_cb(game_obj_t *src, game_obj_t *trg);

static void enemy_burn_timer_cb(game_obj_t *owner, void *usr_data);
static void enemy_freeze_end_cb(game_obj_t *owner, void *usr_data);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

// enemy pool
static pool_t enemy_pool;
static uint16_t enemy_free_indices[MAX_ENEMY_COUNT];
static enemy_t enemies[MAX_ENEMY_COUNT];

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief enemy初始化 包括对象池
 * @param parent 父对象容器
 */
void enemy_init(lv_obj_t *parent)
{
  memset(enemies, 0, sizeof(enemies)); // 初始化置零 重要！
  pool_init(&enemy_pool, enemy_free_indices, MAX_ENEMY_COUNT);

  for (int i = 0; i < MAX_ENEMY_COUNT; i++)
  {

    // base init
    enemies[i].base.active = false;
    enemies[i].base.type = GAME_OBJ_TYPE_ENEMY;
    enemies[i].base.x = 0;
    enemies[i].base.y = 0;
    enemies[i].base.speed = 7.0f;
    enemies[i].base.vx = 0;
    enemies[i].base.vy = 0;
    enemies[i].base.behave = NULL_BEHAVE;

    // special init
    enemies[i].hp = 100;
    enemies[i].hp_max = 100;
    enemies[i].pool_index = POOL_INVALID_ID;
    enemies[i].damage = 0;

    // img & func ptr
    enemies[i].base.update = enemy_update;
    enemies[i].base.show = enemy_show;
    enemies[i].base.hide = enemy_hide;

    enemies[i].base.obj = lv_img_create(parent);
    apr_apply(&enemies[i].base, APR_ENEMY_DEFAULT);

    // health bar
    enemies[i].health_bar = lv_bar_create(parent);
    lv_obj_set_pos(enemies[i].health_bar, 0, 0);
    lv_obj_set_size(enemies[i].health_bar, enemies[i].base.apr->w + 20, 5);
    lv_bar_set_range(enemies[i].health_bar, 0, enemies[i].hp_max);
    lv_bar_set_value(enemies[i].health_bar, enemies[i].hp, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(enemies[i].health_bar, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(enemies[i].health_bar, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);

    // 冻结遮罩 - 蓝色半透明
    enemies[i].freeze_overlay = lv_obj_create(parent);
    lv_obj_set_size(enemies[i].freeze_overlay, enemies[i].base.apr->w, 20);
    lv_obj_set_style_bg_color(enemies[i].freeze_overlay, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_bg_opa(enemies[i].freeze_overlay, LV_OPA_50, 0);
    lv_obj_set_style_border_width(enemies[i].freeze_overlay, 0, 0);
    lv_obj_add_flag(enemies[i].freeze_overlay, LV_OBJ_FLAG_HIDDEN);

    // 状态效果初始化
    enemies[i].burning = false;
    enemies[i].burn_ticks_left = 0;
    enemies[i].frozen = false;

    enemies[i].base.hide(&enemies[i].base);

    // register obj
    game_register_obj(&enemies[i].base);
    // CONSOLE_INFO("Enemy object %d initialized with img: %s.",i,default_apr->img_name);
  }

  // event_cb
  event_register(EVENT_BULLET_HIT_ENEMY, enemy_event_hit_by_bullet_cb);
  event_register(EVENT_PLAYER_HIT_ENEMY, enemy_event_hit_player_cb);
  event_register(EVENT_ENEMY_DESTROYED, enemy_on_destroyed_cb);
  event_register(EVENT_FLAME_WALL_HIT_ENEMY, enemy_event_hit_by_flame_wall_cb);

  CONSOLE_INFO("Enemy system initalized with max enemy count: %d.", MAX_ENEMY_COUNT);
  return;
}

/**
 * @brief 敌人生成
 */
game_obj_t *enemy_spawn(lv_coord_t x, lv_coord_t y,
                        int16_t vx, int16_t vy,
                        uint16_t health, int16_t hit_damage,
                        behave_t behave,
                        apr_id_t apr_id)
{
  if (fsm_get_state() != GS_PLAY)
    return NULL;
  uint16_t id = pool_alloc(&enemy_pool);
  if (id == POOL_INVALID_ID)
  {
    CONSOLE_WARNING("No available enemy slots! Max enemy count: %d", MAX_ENEMY_COUNT);
    LOG_WARNING("No available enemy slots! Max enemy count: %d", MAX_ENEMY_COUNT);
    return NULL;
  }
  enemy_t *e = &enemies[id];
  e->pool_index = id;
  e->base.x = x;
  e->base.y = y;
  e->base.vx = vx;
  e->base.vy = vy;
  e->base.behave = behave;
  e->damage = hit_damage;

  apr_apply(&e->base, apr_id);

  enemy_modify_hp_max(&e->base, health);
  e->hp = e->hp_max;
  lv_obj_set_pos(e->base.obj, x, y);
  lv_obj_set_size(e->health_bar, e->base.apr->w + 20, 5);
  lv_obj_set_pos(e->health_bar, x - 10, y - 10);
  lv_bar_set_value(e->health_bar, e->hp, LV_ANIM_OFF);
  e->base.show(&e->base);
  // CONSOLE_INFO("Enemy %d spawned at x: %d, y: %d.",e->pool_index,x,y);
  return &e->base;
}

int16_t enemy_get_damage(game_obj_t *g)
{
  if (g == NULL)
    return 0;
  enemy_t *e = (enemy_t *)g;
  return e->damage;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 敌人更新
 */
static void enemy_update(game_obj_t *g)
{
  game_state_t game_state = fsm_get_state();
  // 不显示
  if (game_state != GS_PLAY && game_state != GS_PAUSE && game_state != GS_SETTING)
  {
    g->hide(g);
    return;
  }
  // 不更新
  if (game_state == GS_PAUSE || game_state == GS_SETTING)
    return;
  // 不活跃不更新
  if (g->active == false)
    return;

  // 冻结遮罩跟随敌人
  enemy_t *e = (enemy_t *)g;
  if (e->frozen)
  {
    lv_obj_set_pos(e->freeze_overlay, g->x, g->y);
    lv_obj_clear_flag(e->freeze_overlay, LV_OBJ_FLAG_HIDDEN);
  }
  else
  {
    lv_obj_add_flag(e->freeze_overlay, LV_OBJ_FLAG_HIDDEN);
  }

  enemy_move(g);
}

/**
 * @brief 敌人隐藏+销毁
 */
static void enemy_hide(game_obj_t *g)
{
  g->active = false;
  lv_obj_add_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(((enemy_t *)g)->health_bar, LV_OBJ_FLAG_HIDDEN);

  g->timered = false;

  enemy_t *e = (enemy_t *)g;
  e->burning = false;
  e->burn_ticks_left = 0;
  e->frozen = false;
  lv_obj_add_flag(e->freeze_overlay, LV_OBJ_FLAG_HIDDEN);

  if (e->pool_index != POOL_INVALID_ID)
  {
    pool_free(&enemy_pool, e->pool_index);
    e->pool_index = POOL_INVALID_ID;
  }
  return;
}

/**
 * @brief 敌人显示
 */
static void enemy_show(game_obj_t *g)
{
  enemy_t *e = (enemy_t *)g;
  if (e->pool_index == POOL_INVALID_ID)
  {
    CONSOLE_WARNING("Enemy object is not initialized. Cannot show.");
    LOG_WARNING("Enemy object is not initialized. Cannot show.");
    return;
  }
  g->active = true;
  lv_obj_clear_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(((enemy_t *)g)->health_bar, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief 敌人移动
 */
static void enemy_move(game_obj_t *g)
{
  if (g == NULL)
    return;

  if (g->active == false)
    return;

  // 被冻结的敌人不能移动
  if (((enemy_t *)g)->frozen)
    return;

  int16_t dx = g->vx;
  int16_t dy = g->vy;

  if (dx == 0 && dy == 0)
    return;

  g->x += dx;
  g->y += dy;

  lv_obj_set_pos(g->obj, g->x, g->y);
  enemy_t *e = (enemy_t *)g;
  lv_obj_set_pos(e->health_bar, g->x - 10, g->y - 10);

  // 出界检查
  if (g->x < ENEMY_MIN_X || g->x > ENEMY_MAX_X || g->y < ENEMY_MIN_Y || g->y > ENEMY_MAX_Y)
  {
    g->hide(g);
  }

  return;
}

/**
 * @brief 敌人血量修改
 */
static int16_t enemy_modify_hp(game_obj_t *g, int16_t delta)
{
  if (g == NULL)
    return -1;
  if (g->active == false)
    return -1;

  enemy_t *e = (enemy_t *)g;
  e->hp += delta;

  if (e->hp > e->hp_max)
  {
    e->hp = e->hp_max;
    return e->hp;
  }
  if (e->hp <= 0)
  {
    e->hp = 0;
    // CONSOLE_INFO("Enemy %d has been killed.", e->pool_index);

    event_dispatch(EVENT_ENEMY_DESTROYED, g, NULL);

    e->base.hide(g);
    return 0;
  }

  lv_bar_set_value(e->health_bar, e->hp, LV_ANIM_OFF);

  return e->hp;
}

/**
 * @brief 敌人最大血量修改
 */
static uint16_t enemy_modify_hp_max(game_obj_t *g, uint16_t trg)
{
  if (g == NULL)
    return 0;
  enemy_t *e = (enemy_t *)g;
  e->hp_max = trg;
  lv_bar_set_range(e->health_bar, 0, e->hp_max);
  lv_bar_set_value(e->health_bar, e->hp, LV_ANIM_OFF);
  return e->hp_max;
}

/**
 * @brief 子弹击中敌人事件回调
 */
static void enemy_event_hit_by_bullet_cb(game_obj_t *scr, game_obj_t *trg)
{
  uint8_t damage = bullet_get_damage(scr);
  enemy_modify_hp(trg, -damage);

  // 播放音效
  audio_load(AUDIO_ENEMYHIT, AUDIO_CHAN_AUTO, false);

  // 应用子弹特殊效果
  uint8_t flags = bullet_get_flags(scr);
  if (flags & BULLET_FLAG_BURN)
  {
    enemy_apply_burn(trg);
  }
  if (flags & BULLET_FLAG_FREEZE)
  {
    enemy_apply_freeze(trg);
  }
}

/**
 * @brief 玩家撞击敌人事件回调 —— 敌人触碰即销毁（Boss 的高伤害由 player 回调处理）
 */
static void enemy_event_hit_player_cb(game_obj_t *src, game_obj_t *trg)
{
  trg->hide(trg);
}

/**
 * @brief EVENT_ENEMY_DESTROYED 回调 —— 批量处理所有死亡敌人
 *        遍历敌人池，对每个 HP<=0 的活跃敌人：
 *        1. 用 BEHAVE_ON_DEATH 标记调用 behave.f → 执行死亡掉落
 *        2. hide 回收对象池槽位
 */
static void enemy_on_destroyed_cb(game_obj_t *src, game_obj_t *trg)
{
  (void)src;
  (void)trg;

  for (int i = 0; i < MAX_ENEMY_COUNT; i++)
  {
    if (!enemies[i].base.active)
      continue;
    if (enemies[i].hp > 0)
      continue;

    if (enemies[i].base.behave.f)
    {
      enemies[i].base.behave.f(&enemies[i].base, BEHAVE_ON_DEATH);
    }

    enemies[i].base.hide(&enemies[i].base);
  }
}

/**
 * @brief 火焰墙撞击敌人事件回调
 */
static void enemy_event_hit_by_flame_wall_cb(game_obj_t *src, game_obj_t *trg)
{
  int damage = flame_wall_get_damage(src);
  enemy_apply_damage(trg, damage);
}

/**
 * @brief 灼烧定时器回调 — 每1秒造成100伤害
 */
static void enemy_burn_timer_cb(game_obj_t *owner, void *usr_data)
{
  (void)usr_data;
  if (owner == NULL || !owner->active)
    return;
  enemy_t *e = (enemy_t *)owner;
  if (!e->burning)
    return;
  enemy_modify_hp(owner, -100);
  e->burn_ticks_left--;
  if (e->burn_ticks_left <= 0)
  {
    e->burning = false;
  }
}

/**
 * @brief 对敌人施加灼烧效果（2秒，每秒100伤害）
 */
void enemy_apply_burn(game_obj_t *g)
{
  if (g == NULL || !g->active)
    return;
  enemy_t *e = (enemy_t *)g;
  if (e->burning)
    return; // 不叠加
  e->burning = true;
  e->burn_ticks_left = 2;
  timer_create(g, 1000, TIMER_MODE_REPEAT, enemy_burn_timer_cb, NULL);
  CONSOLE_INFO("[ENEMY] Burn applied to enemy %d", e->pool_index);
}

/**
 * @brief 冻结结束回调 — 清除冻结状态
 */
static void enemy_freeze_end_cb(game_obj_t *owner, void *usr_data)
{
  (void)usr_data;
  if (owner == NULL || !owner->active)
    return;
  enemy_t *e = (enemy_t *)owner;
  e->frozen = false;
  CONSOLE_INFO("[ENEMY] Freeze ended for enemy %d", e->pool_index);
}

/**
 * @brief 对敌人施加冻结效果（2秒）
 */
void enemy_apply_freeze(game_obj_t *g)
{
  if (g == NULL || !g->active)
    return;
  enemy_t *e = (enemy_t *)g;
  if (e->frozen)
    return; // 不叠加
  e->frozen = true;
  timer_create(g, 2000, TIMER_MODE_ONCE, enemy_freeze_end_cb, NULL);
  CONSOLE_INFO("[ENEMY] Freeze applied to enemy %d", e->pool_index);
}

/**
 * @brief 查询敌人是否被冻结
 */
bool enemy_is_frozen(game_obj_t *g)
{
  if (g == NULL)
    return false;
  return ((enemy_t *)g)->frozen;
}

/**
 * @brief 外部对敌人造成伤害（火墙等非子弹来源）
 */
void enemy_apply_damage(game_obj_t *g, int16_t damage)
{
  if (g == NULL || !g->active)
    return;
  enemy_modify_hp(g, -damage);
}
