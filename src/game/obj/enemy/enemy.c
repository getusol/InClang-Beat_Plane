/**
 * @file enemy.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "enemy.h"
#include "pool.h"
#include "config.h"
#include "lvgl_utils.h"
#include "game.h"
#include "tools.h"
#include "fsm.h"
#include "event.h"
#include "bullet.h"

/**********************
 *      MACROS
 **********************/

#define ENEMY_IMG_NAME "enemy.bin"

#define ENEMY_MAX_X 980                 // 敌人最大X坐标
#define ENEMY_MIN_X 0                    // 敌人最小X坐标
#define ENEMY_MAX_Y 600                  // 敌人最大Y坐标
#define ENEMY_MIN_Y -64                    // 敌人最小Y坐标

#define ENEMY_WIDTH 19
#define ENEMY_HIGHT 64

/**********************
 *      TYPEDEFS
 **********************/

typedef struct
{
    game_obj_t base;
    int16_t hp; // 生命值
    int16_t hp_max;
    uint16_t pool_index; // 对象池索引
    lv_obj_t * health_bar;
} enemy_t;

 /**********************
  *  STATIC PROTOTYPES
  **********************/

static void enemy_update(game_obj_t * g);
static void enemy_hide(game_obj_t * g);
static void enemy_show(game_obj_t * g);
static lv_point_t enemy_move(game_obj_t * g,lv_coord_t dx,lv_coord_t dy);
static int16_t enemy_modify_hp(game_obj_t * g,int16_t delta);

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

// enemy img
static uint8_t * enemy_img_buf = NULL;
static lv_img_dsc_t enemy_img_struct;


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

  char enemy_img_path[64];
  for (int i = 0; i < MAX_ENEMY_COUNT; i++) {

    // base init
    enemies[i].base.active = false;
    enemies[i].base.w = ENEMY_WIDTH;
    enemies[i].base.h = ENEMY_HIGHT;
    enemies[i].base.type = GAME_OBJ_TYPE_ENEMY;
    enemies[i].base.x = 0;
    enemies[i].base.y = 0;
    enemies[i].base.speed = 7.0f;
    enemies[i].base.hitbox_x = 0;
    enemies[i].base.hitbox_y = 0;
    enemies[i].base.hitbox_w = 19;
    enemies[i].base.hitbox_h = 64;

    // special init
    enemies[i].hp = 100;
    enemies[i].hp_max = 100;
    enemies[i].pool_index = POOL_INVALID_ID;

    // img & func ptr
    enemies[i].base.update = enemy_update;
    enemies[i].base.show = enemy_show;
    enemies[i].base.hide = enemy_hide;
    enemies[i].base.obj = img_create_from_dsc(parent,img_path(ENEMY_IMG_NAME,enemy_img_path,64),enemies[i].base.w,enemies[i].base.h,enemy_img_buf,&enemy_img_struct,false);
    

    // health bar
    enemies[i].health_bar = lv_bar_create(parent);
    lv_obj_set_pos(enemies[i].health_bar,0,0);
    lv_obj_set_size(enemies[i].health_bar,enemies[i].base.w + 20,5);
    lv_bar_set_range(enemies[i].health_bar,0,enemies[i].hp_max);
    lv_bar_set_value(enemies[i].health_bar,enemies[i].hp,LV_ANIM_OFF);
    // 在创建 health_bar 之后
    lv_obj_set_style_bg_color(enemies[i].health_bar, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(enemies[i].health_bar, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);

    enemies[i].base.hide(&enemies[i].base);

    // register obj
    game_register_obj(&enemies[i].base);
    CONSOLE("[INFO] Enemy object %d initialized with img: %s.",i,enemy_img_path);
  }

  // event_cb
  event_register(EVENT_BULLET_HIT_ENEMY,enemy_event_hit_by_bullet_cb);
  event_register(EVENT_PLAYER_HIT_ENEMY,enemy_event_hit_player_cb);

  CONSOLE("[INFO] Enemy system initalized with max enemy count: %d.",MAX_ENEMY_COUNT);
  return ;
}

/**
 * @brief 敌人生成
 * @param x 生成的x坐标
 * @param y 生成的y坐标
 * @return game_obj_t* 生成的敌人
 */
game_obj_t * enemy_spawn(lv_coord_t x, lv_coord_t y)
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
  e->base.active = true;
  e->base.x = x;
  e->base.y = y;
  e->hp = e->hp_max;
  lv_obj_set_pos(e->base.obj,x,y);
  lv_obj_set_pos(e->health_bar,x - 10,y - 10);
  lv_bar_set_value(e->health_bar,e->hp,LV_ANIM_OFF);
  e->base.show(&e->base);
  // CONSOLE("[INFO] Enemy %d spawned at x: %d, y: %d.",e->pool_index,x,y);
  return &e->base;
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
  // 不显示
  if (game_state != GS_PLAY && game_state != GS_PAUSE)
  {
    g->hide(g);
    return ;
  }
  // 不更新
  if (game_state == GS_PAUSE) return ;
  // 不活跃不更新
  if (g->active == false) return ;
  enemy_move(g,0,g->speed);
}

/**
 * @brief 敌人隐藏
 * @param g 敌人对象
 */
static void enemy_hide(game_obj_t * g)
{
  g->active = false;
  lv_obj_add_flag(g->obj,LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(((enemy_t *)g)->health_bar,LV_OBJ_FLAG_HIDDEN);

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
 * @param g 敌人对象
 */
static void enemy_show(game_obj_t * g)
{
  enemy_t * e = (enemy_t *)g;
  if (e->pool_index == POOL_INVALID_ID)
  {
    CONSOLE("[WARNING] Enemy object is not initialized. Cannot show.");
    LOG("[WARNING] Enemy object is not initialized. Cannot show.");
    return ;
  }
  g->active = true;
  lv_obj_clear_flag(g->obj,LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(((enemy_t *)g)->health_bar,LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief 敌人移动
 * @param e 敌人对象指针
 * @param dx x轴移动距离
 * @param dy y轴移动距离
 * @return lv_point_t 移动后的坐标
 */
static lv_point_t enemy_move(game_obj_t * g,lv_coord_t dx,lv_coord_t dy)
{
  if (g == NULL) return (lv_point_t){0,0};

  if (g->active == false) return (lv_point_t){0,0};
  if (dx == 0 && dy == 0) return (lv_point_t){g->x,g->y};

  g->x += dx;
  g->y += dy;

  lv_obj_set_pos(g->obj,g->x,g->y);
  enemy_t * e = (enemy_t *)g;
  lv_obj_set_pos(e->health_bar,g->x - 10,g->y - 10);
  
  // 出界检查
  if (g->x < ENEMY_MIN_X || g->x > ENEMY_MAX_X || g->y < ENEMY_MIN_Y || g->y > ENEMY_MAX_Y)
  {
    g->hide(g);
  }

  return (lv_point_t){g->x,g->y};
}

/**
 * @brief 敌人血量修改
 * @param g 敌人对象指针
 * @param delta 血量变化量
 * @return int16_t 修改后的血量
 */
static int16_t enemy_modify_hp(game_obj_t * g,int16_t delta)
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
    e->base.hide(g);
    CONSOLE("[INFO] Enemy %d has been killed.",e->pool_index);
    return 0;
  }

  lv_bar_set_value(e->health_bar, e->hp, LV_ANIM_OFF);

  return e->hp;
}

/**
 * @brief 子弹击中敌人事件回调 敌人扣血
 * @param scr 子弹对象指针
 * @param trg 敌人对象指针
 */
static void enemy_event_hit_by_bullet_cb(game_obj_t * scr,game_obj_t * trg)
{
  uint8_t damage = bullet_get_damage(scr);
  enemy_modify_hp(trg,-damage);
}

/**
 * @brief 玩家撞击敌人 事件回调 敌人消失
 * @param src 玩家对象指针
 * @param trg 敌人对象指针
 */
static void enemy_event_hit_player_cb(game_obj_t * src,game_obj_t * trg)
{
  trg->hide(trg);
}
