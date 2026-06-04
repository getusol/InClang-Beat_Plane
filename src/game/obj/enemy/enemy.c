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

/**********************
 *      MACROS
 **********************/

#define ENEMY_MAX_X 980                 // 敌人最大X坐标
#define ENEMY_MIN_X 0                    // 敌人最小X坐标
#define ENEMY_MAX_Y 600                  // 敌人最大Y坐标
#define ENEMY_MIN_Y -64                  // 敌人最小Y坐标

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
    lv_obj_t * health_bar;
    bool is_boss;       // 是否为 Boss
} enemy_t;

 /**********************
  *  STATIC PROTOTYPES
  **********************/

static void enemy_update(game_obj_t * g);
static void enemy_hide(game_obj_t * g);
static void enemy_show(game_obj_t * g);
static void enemy_move(game_obj_t * g);
static int16_t enemy_modify_hp(game_obj_t * g,int16_t delta);
static uint16_t enemy_modify_hp_max(game_obj_t * g,uint16_t trg);

static void enemy_event_hit_by_bullet_cb(game_obj_t * scr,game_obj_t * trg);
static void enemy_event_hit_player_cb(game_obj_t * src,game_obj_t * trg);

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
void enemy_init(lv_obj_t * parent)
{
  memset(enemies,0,sizeof(enemies)); // 初始化置零 重要！
  pool_init(&enemy_pool, enemy_free_indices, MAX_ENEMY_COUNT);

  apr_t *default_apr = apr_get(APR_ENEMY_DEFAULT);

  for (int i = 0; i < MAX_ENEMY_COUNT; i++) {

    // base init
    enemies[i].base.active = false;
    enemies[i].base.apr = default_apr;
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

    // 使用 APR 创建 LVGL 图像
#ifdef SIMULATOR
    enemies[i].base.obj = lv_img_create(parent);
    lv_img_set_src(enemies[i].base.obj, &default_apr->img_dsc);
#else
    char path[128];
    enemies[i].base.obj = lv_img_create(parent);
    lv_img_set_src(enemies[i].base.obj, img_path(default_apr->img_name, path, 128));
#endif


    // health bar
    enemies[i].health_bar = lv_bar_create(parent);
    lv_obj_set_pos(enemies[i].health_bar,0,0);
    lv_obj_set_size(enemies[i].health_bar, default_apr->w + 20, 5);
    lv_bar_set_range(enemies[i].health_bar, 0, enemies[i].hp_max);
    lv_bar_set_value(enemies[i].health_bar, enemies[i].hp, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(enemies[i].health_bar, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(enemies[i].health_bar, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);

    enemies[i].base.hide(&enemies[i].base);

    // register obj
    game_register_obj(&enemies[i].base);
    CONSOLE("[INFO] Enemy object %d initialized.", i);
  }

  // event_cb
  event_register(EVENT_BULLET_HIT_ENEMY, enemy_event_hit_by_bullet_cb);
  event_register(EVENT_PLAYER_HIT_ENEMY, enemy_event_hit_player_cb);

  CONSOLE("[INFO] Enemy system initialized with max enemy count: %d.", MAX_ENEMY_COUNT);
  return ;
}

/**
 * @brief 敌人生成
 */
game_obj_t * enemy_spawn(lv_coord_t x, lv_coord_t y,
                         int16_t vx, int16_t vy,
                         uint16_t health, int16_t hit_damage,
                         behave_t behave)
{
  if (fsm_get_state() != GS_PLAY) return NULL;
  uint16_t id = pool_alloc(&enemy_pool);
  if (id == POOL_INVALID_ID)
  {
    CONSOLE("[WARNING] No available enemy slots! Max enemy count: %d", MAX_ENEMY_COUNT);
    LOG("[WARNING] No available enemy slots! Max enemy count: %d", MAX_ENEMY_COUNT);
    return NULL;
  }
  enemy_t * e = &enemies[id];
  e->pool_index = id;
  e->base.x = x;
  e->base.y = y;
  e->base.vx = vx;
  e->base.vy = vy;
  e->base.behave = behave;
  e->damage = hit_damage;

  enemy_modify_hp_max(&e->base, health);
  e->hp = e->hp_max;
  lv_obj_set_pos(e->base.obj, x, y);
  lv_obj_set_pos(e->health_bar, x - 10, y - 10);
  lv_bar_set_value(e->health_bar, e->hp, LV_ANIM_OFF);
  e->base.show(&e->base);
  return &e->base;
}

/**
 * @brief Boss 生成 —— 自动应用 Boss 外观和行为
 */
game_obj_t * enemy_spawn_boss(lv_coord_t x, lv_coord_t y,
                               uint16_t health, int16_t hit_damage)
{
    behave_t boss_behave = { .f = enemy_behave_boss, .usr_data = NULL };
    // 用 vy=0 防止首帧越界被 hide；behave 里会重新定位
    game_obj_t *g = enemy_spawn(x, y, 0, 0, health, hit_damage, boss_behave);
    if (g != NULL) {
        ((enemy_t *)g)->is_boss = true;
        // 立即应用 Boss 外观，不等 behave（behave 在 update 之后才执行）
        apr_apply(g, APR_ENEMY_BOSS);
        // 定位到屏幕顶部中央
        g->x = SCREEN_WIDTH / 2 - g->apr->w / 2;
        g->y = 50;
        g->vx = 0;
        g->vy = 0;
        lv_obj_set_pos(g->obj, g->x, g->y);
        CONSOLE("[BOSS] Boss spawned at (%d, %d), HP=%d", g->x, g->y, health);
    }
    return g;
}

int16_t enemy_get_damage(game_obj_t * g)
{
  if (g == NULL) return 0;
  enemy_t * e = (enemy_t *)g;
  return e->damage;
}

bool enemy_is_boss(game_obj_t * g)
{
    if (g == NULL) return false;
    enemy_t * e = (enemy_t *)g;
    return e->is_boss;
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 敌人更新
 */
static void enemy_update(game_obj_t * g)
{
  game_state_t game_state = fsm_get_state();
  if (game_state != GS_PLAY && game_state != GS_PAUSE)
  {
    g->hide(g);
    return ;
  }
  if (game_state == GS_PAUSE) return ;
  if (g->active == false) return ;
  enemy_move(g);
}

/**
 * @brief 敌人隐藏+销毁
 */
static void enemy_hide(game_obj_t * g)
{
  g->active = false;
  lv_obj_add_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(((enemy_t *)g)->health_bar, LV_OBJ_FLAG_HIDDEN);

  g->timered = false;

  enemy_t * e = (enemy_t *)g;
  if (e->pool_index != POOL_INVALID_ID)
  {
    pool_free(&enemy_pool, e->pool_index);
    e->pool_index = POOL_INVALID_ID;
  }
  return ;
}

/**
 * @brief 敌人显示
 */
static void enemy_show(game_obj_t * g)
{
  enemy_t * e = (enemy_t *)g;
  if (e->pool_index == POOL_INVALID_ID)
  {
    CONSOLE("[WARNING] Enemy object is not initialized. Cannot show.");
    return ;
  }
  g->active = true;
  lv_obj_clear_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(((enemy_t *)g)->health_bar, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief 敌人移动
 */
static void enemy_move(game_obj_t * g)
{
  if (g == NULL) return ;

  if (g->active == false) return ;

  int16_t dx = g->vx;
  int16_t dy = g->vy;

  if (dx == 0 && dy == 0) return ;

  g->x += dx;
  g->y += dy;

  lv_obj_set_pos(g->obj, g->x, g->y);
  enemy_t * e = (enemy_t *)g;
  lv_obj_set_pos(e->health_bar, g->x - 10, g->y - 10);

  // 出界检查
  if (g->x < ENEMY_MIN_X || g->x > ENEMY_MAX_X || g->y < ENEMY_MIN_Y || g->y > ENEMY_MAX_Y)
  {
    g->hide(g);
  }

  return ;
}

/**
 * @brief 敌人血量修改
 */
static int16_t enemy_modify_hp(game_obj_t * g, int16_t delta)
{
  if (g == NULL) return -1;
  if (g->active == false) return -1;

  enemy_t * e = (enemy_t *)g;
  e->hp += delta;

  if (e->hp > e->hp_max) {
    e->hp = e->hp_max;
    return e->hp;
  }
  if (e->hp <= 0) {
    e->hp = 0;
    CONSOLE("[INFO] Enemy %d has been killed.", e->pool_index);

    lv_coord_t coin_x = g->x + (g->apr->w - 18) / 2;
    lv_coord_t coin_y = g->y + (g->apr->h - 18) / 2;

    if (e->is_boss) {
      // Boss 死亡掉落 8 个金币
      for (int c = 0; c < 8; c++) {
        lv_coord_t cx = coin_x + lv_rand(-30, 30);
        lv_coord_t cy = coin_y + lv_rand(-30, 30);
        coin_spawn(cx, cy);
      }
      CONSOLE("[BOSS] Boss defeated! 8 coins dropped.");
    } else {
      coin_spawn(coin_x, coin_y);
    }

    e->base.hide(g);
    return 0;
  }

  lv_bar_set_value(e->health_bar, e->hp, LV_ANIM_OFF);

  return e->hp;
}

/**
 * @brief 敌人最大血量修改
 */
static uint16_t enemy_modify_hp_max(game_obj_t * g, uint16_t trg)
{
  if (g == NULL) return 0;
  enemy_t * e = (enemy_t *)g;
  e->hp_max = trg;
  lv_bar_set_range(e->health_bar, 0, e->hp_max);
  lv_bar_set_value(e->health_bar, e->hp, LV_ANIM_OFF);
  return e->hp_max;
}

/**
 * @brief 子弹击中敌人事件回调
 */
static void enemy_event_hit_by_bullet_cb(game_obj_t * scr, game_obj_t * trg)
{
  uint8_t damage = bullet_get_damage(scr);
  enemy_modify_hp(trg, -damage);
}

/**
 * @brief 玩家撞击敌人事件回调 —— Boss 秒杀玩家
 */
static void enemy_event_hit_player_cb(game_obj_t * src, game_obj_t * trg)
{
  enemy_t * e = (enemy_t *)trg;
  if (e->is_boss) {
    // Boss 碰触直接秒杀玩家
    event_dispatch(EVENT_PLAYER_DIE, NULL, NULL);
    CONSOLE("[BOSS] Player crushed by boss!");
  } else {
    trg->hide(trg);
  }
}
