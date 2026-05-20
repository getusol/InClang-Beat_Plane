/**
 * @file player.h
 */

/*********************
 *      INCLUDES
 *********************/
#include "player.h"

#include "tools.h"
#include "lvgl_utils.h"
#include "fsm.h"
#include "input_sw.h"
#include "input_hw.h"
#include "config.h"
#include "game.h"
#include "event.h"

#include "bullet.h"
#include "bullet_behaviors.h"

/**********************
 *      MACROS
 **********************/
//player img params
#define PLAYER_IMG_NAME "player.bin"

#define PLAYER_MAX_X 960                 // 玩家最大X坐标
#define PLAYER_MIN_X 0                    // 玩家最小X坐标
#define PLAYER_MAX_Y 540                  // 玩家最大Y坐标
#define PLAYER_MIN_Y 0                    // 玩家最小Y坐标

#define PLAYER_WIDTH 64
#define PLAYER_HIGHT 64

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief 玩家结构体，继承自游戏对象，包含玩家特有的属性和方法
 */
typedef struct {
    game_obj_t base; // 继承自游戏对象

    // 可以在这里添加玩家特有的属性，例如生命值、分数等
    
    int16_t hp;
    int16_t hp_max;

    uint16_t shoot_cd;

    lv_obj_t * hp_bar; // 生命值显示的lvgl对象指针
} player_t;

 /**********************
  *  STATIC PROTOTYPES
  **********************/

static void player_update(game_obj_t * g);
static void player_show(game_obj_t * g);
static void player_hide(game_obj_t * g);
static void player_move(game_obj_t * g);
static lv_obj_t * player_hp_bar_create(game_obj_t * g,lv_obj_t * parent);
static lv_obj_t * player_obj_create(game_obj_t * g,lv_obj_t * parent);

// 按键回调

static void player_x_pressed_handler();
static void player_fire();

// 事件回调

static void player_event_game_start_cb(game_obj_t * src,game_obj_t * trg);
static void player_event_player_die_cb(game_obj_t * src,game_obj_t * trg);
static void player_event_hit_by_enemy_cb(game_obj_t * src,game_obj_t * trg);

// 发射子弹，子弹行为函数

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static player_t * player_p = NULL;

static lv_img_dsc_t player_img_struct;

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 玩家对象初始化函数，创建玩家对象并设置初始属性
 * @param parent 玩家对象的父lvgl对象指针，游戏主界面
 * @return none
 */
void player_init(lv_obj_t * parent)
{
  player_p = ram_malloc(sizeof(player_t));
  memset(player_p,0,sizeof(player_t));
  if (player_p == NULL)
  {
    CONSOLE("[Error] Failed to allocate memory for player object.");
    LOG("[Error] Failed to allocate memory for player object.");
    sys_halt();
    return;
  }
  
  // 初始化玩家属性
  player_p->base.x = 512;
  player_p->base.y = 500;
  player_p->base.w = PLAYER_WIDTH;
  player_p->base.h = PLAYER_HIGHT;
  player_p->base.vx = 0;
  player_p->base.vy = 0;
  player_p->base.active = true;
  player_p->base.type = GAME_OBJ_TYPE_PLAYER;
  player_p->base.hitbox_x = 2;
  player_p->base.hitbox_y = 22;
  player_p->base.hitbox_w = 60;
  player_p->base.hitbox_h = 20;
  player_p->base.behave = NULL_BEHAVE;

  player_p->base.update = player_update;
  player_p->base.show = player_show;
  player_p->base.hide = player_hide;

  player_p->hp_max = 100;
  player_p->hp = player_p->hp_max;
  player_p->shoot_cd = 200; // 200ms射击冷却

  player_p->hp_bar = player_hp_bar_create((game_obj_t *)player_p,parent);
  player_p->base.obj = player_obj_create((game_obj_t *)player_p,parent);

  lv_obj_set_pos(player_p->base.obj,player_p->base.x,player_p->base.y);

  game_register_obj((game_obj_t *)player_p);

  // 按键行为
  //X 超级子弹
  // input_sw_register_press_callback(KEY_EVENT_X, player_x_pressed_handler);
  input_sw_register_long_press_callback(KEY_EVENT_X,player_x_pressed_handler,5000);
  //A 射击
  // input_sw_register_press_callback(KEY_EVENT_A, player_fire);
  input_sw_register_long_press_callback(KEY_EVENT_A, player_fire, player_p->shoot_cd);

  // 事件注册
  event_register(EVENT_GAME_START,player_event_game_start_cb);
  event_register(EVENT_PLAYER_DIE,player_event_player_die_cb);
  event_register(EVENT_PLAYER_HIT_ENEMY,player_event_hit_by_enemy_cb);
  
  
  CONSOLE("[INFO] Player initialization complete.\n");
  console_out("[PLAYER] player properties:\n");
  console_out("    width: %d\n",player_p->base.w);
  console_out("    height: %d\n",player_p->base.h);
  console_out("    speed: %f\n",player_p->base.speed);
  console_out("    HP_max: %d\n",player_p->hp_max);
  console_out("    shoot_cd: %dms\n",player_p->shoot_cd);
  console_out("\n");

  return;
}

/**
 * @brief 获取玩家基类指针，方便在其他模块中调用玩家对象的update、show、hide等方法
 * @return game_obj_t* 玩家对象的基类指针
 */
game_obj_t * player_get_base()
{
  return (game_obj_t *)player_p;
}

/**
 * @brief 玩家HP修改函数，修改玩家HP并更新HP显示
 * @param delta HP变化量，正数表示增加HP，负数表示减少HP
 * @return int16_t 修改后的HP值
 */
int16_t player_hp_modify(int16_t delta)
{
  if (player_p == NULL) {
    CONSOLE("[WARNING] Player object is not initialized. Cannot modify HP.");
    LOG("[WARNING] Player object is not initialized. Cannot modify HP.");
    return 0;
  }
  if (!player_p->base.active) {
    CONSOLE("[WARNING] Player is not active. Cannot modify HP.");
    LOG("[WARNING] Player is not active. Cannot modify HP.");
    return player_p->hp;
  }
  player_p->hp += delta;
  if (player_p->hp > player_p->hp_max) {
    player_p->hp = player_p->hp_max;
    CONSOLE("[INFO] Player HP modified. HP is at max: %d", player_p->hp_max);
  }
  if (player_p->hp <= 0) {
    player_p->hp = 0;
    CONSOLE("[INFO] Player HP modified. HP has dropped to 0.");
    event_dispatch(EVENT_PLAYER_DIE,NULL,NULL);
  }
  lv_bar_set_value(player_p->hp_bar, player_p->hp, LV_ANIM_OFF);
  CONSOLE("[INFO] Player HP modified by %d. Current HP: %d", delta, player_p->hp);
  return player_p->hp;
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 玩家更新函数 负责管理玩家的显示状态
 */
static void player_update(game_obj_t * g)
{
  if (fsm_get_state() < GS_PLAY || fsm_get_state() > GS_PAUSE) {
    g->hide(g);
    return ;
  }
  g->show(g);
  if (fsm_get_state() != GS_PLAY || !player_p->base.active) {
    return ;
  }
  // 玩家移动逻辑
  g->vx = joystick_get_x() / 127.0f * 7;
  g->vy = joystick_get_y() / 127.0f * 7;
  player_move(g);
}

/**
 * @brief 玩家显示函数 显示玩家对象和生命值
 */
static void player_show(game_obj_t * g)
{
  lv_obj_clear_flag(g->obj,LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(((player_t *)g)->hp_bar,LV_OBJ_FLAG_HIDDEN);
  g->active = true;
}

/**
 * @brief 玩家隐藏函数 隐藏玩家对象和生命值
 */
static void player_hide(game_obj_t * g)
{
  lv_obj_add_flag(g->obj,LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(((player_t *)g)->hp_bar,LV_OBJ_FLAG_HIDDEN);
  g->active = false;
}

/**
 * @brief 玩家移动函数，根据输入的dx和dy计算玩家的新位置，并更新玩家对象的位置属性
 * @param dx 水平移动距离，正数表示向右移动，负数表示向左移动
 * @param dy 垂直移动距离，正数表示向下移动，负数表示向上移动
 * @return lv_point_t 玩家移动后的新位置坐标
 */
static void player_move(game_obj_t * g)
{
  if (g == NULL) {
    console_out("[Warning][player_move] Player object is not initialized. Cannot move player.\n");
    log_out("[Warning][player_move] Player object is not initialized. Cannot move player.");
    return ;
  }

  if (g->active == false) {
    return ;
  }

  if (g->vx == 0 && g->vy == 0) {
    return ;
  }
  
  player_p->base.x += g->vx;
  player_p->base.y += g->vy;

  // 边界检查，确保玩家不会移动出游戏区域
  if (player_p->base.x < PLAYER_MIN_X) {
    player_p->base.x = PLAYER_MIN_X;
  }
  if (player_p->base.x > PLAYER_MAX_X) {
    player_p->base.x = PLAYER_MAX_X;
  }
  if (player_p->base.y < PLAYER_MIN_Y) {
    player_p->base.y = PLAYER_MIN_Y;
  }
  if (player_p->base.y > PLAYER_MAX_Y) {
    player_p->base.y = PLAYER_MAX_Y;
  }

  lv_obj_set_pos(player_p->base.obj,player_p->base.x,player_p->base.y);

  // console_out("[player_move] Player moved by dx: %d, dy: %d. New position - x: %d, y: %d\n", dx, dy, player_p->base.x, player_p->base.y);

  return ;
}

/**
 * @brief 创建玩家对象的生命值显示对象
 */
static lv_obj_t * player_hp_bar_create(game_obj_t * g,lv_obj_t * parent)
{
  lv_obj_t * hp_bar = lv_bar_create(parent);
  lv_obj_set_size(hp_bar, 155, 21);
  lv_obj_set_align(hp_bar, LV_ALIGN_TOP_LEFT);
  lv_obj_set_pos(hp_bar, 45, 13);
  lv_bar_set_range(hp_bar, 0, ((player_t *)g)->hp_max);
  lv_bar_set_value(hp_bar, ((player_t *)g)->hp, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(hp_bar,lv_palette_main(LV_PALETTE_RED),LV_PART_INDICATOR);

  console_out("[player_hp_bar_create] Player HP bar created, max HP: %d\n", ((player_t *)g)->hp_max);

  return hp_bar;
}

/**
 * @brief 创建玩家对象的lvgl显示对象
 */
static lv_obj_t * player_obj_create(game_obj_t * g,lv_obj_t * parent)
{
  char img_path_buf[64];
  lv_obj_t * img  = img_create_from_dsc(parent,img_path(PLAYER_IMG_NAME,img_path_buf,64),g->w,g->h,NULL,&player_img_struct,true);
  lv_obj_set_align(img,LV_ALIGN_TOP_LEFT);

  console_out("[player_obj_create] Player object created with image: %s\n", img_path_buf);

  return img;
}

/**
 * @brief X键短按对玩家的作用
 */
static void player_x_pressed_handler()
{
    if (!player_p->base.active || fsm_get_state() != GS_PLAY) {
        return ;
    }
    behave_t behave = {
      .f = bullet_behave_sine,
      .usr_data = NULL,
    };
    bullet_create((game_obj_t *)player_p,player_p->base.x + player_p->base.w / 2 - 6, player_p->base.y - 16, 0,-6,66,behave);
    return ;
}

/**
 * @brief 玩家开火函数
 */
static void player_fire()
{
  game_obj_t * g = (game_obj_t *)player_p;
  if (fsm_get_state() != GS_PLAY || !game_obj_is_active(g)) {
      return ;
  }
  bullet_create(g,g->x + g->w / 2 - 6, g->y - 16, 0,-20,34,NULL_BEHAVE); // 伤害34 向上速度20
}

/**
 * @brief 玩家游戏开始重置函数
 */
static void player_event_game_start_cb(game_obj_t * src,game_obj_t * trg)
{
  player_p->base.x = 512;
  player_p->base.y = 500;
  player_p->hp = player_p->hp_max;
  lv_bar_set_value(player_p->hp_bar, player_p->hp, LV_ANIM_OFF);
  player_p->base.show((game_obj_t *) player_p);
  lv_obj_set_pos(player_p->base.obj,player_p->base.x,player_p->base.y);
  CONSOLE("[INFO] Player has been revived. HP reset to max: %d", player_p->hp_max);
}

/**
 * @brief 玩家死亡玩家模块事件回调
 */
static void player_event_player_die_cb(game_obj_t * src,game_obj_t * trg)
{
  player_p->base.hide(&player_p->base);     // 玩家HP为0时隐藏玩家对象
  fsm_switch_state(GS_OVER);                // 切换到游戏结束状态 bug:这个时候强制切回GS_PLAY 玩家会0血存活，等到下次受伤又会死亡
}

/**
 * @brief 玩家被敌人击中事件回调 玩家扣血
 * @param src 玩家对象指针
 * @param trg 敌人对象指针
 */
static void player_event_hit_by_enemy_cb(game_obj_t * src,game_obj_t * trg)
{
  player_hp_modify(-20);  //目前固定扣20血
}
