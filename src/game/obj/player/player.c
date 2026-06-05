/**
 * @file player.c
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
#include "timer.h"

#include "bullet.h"
#include "bullet_behaviors.h"
#include "enemy.h"
#include "ui_base.h" // for ui_base_get_selected_plane_id()
#include "flame_wall.h"
#include "ui_play.h"
#include "multiplayer.h"
#include "audio.h"

/**********************
 *      MACROS
 **********************/

#define PLAYER_MAX_X 960                 // 玩家最大X坐标
#define PLAYER_MIN_X 0                    // 玩家最小X坐标
#define PLAYER_MAX_Y 540                  // 玩家最大Y坐标
#define PLAYER_MIN_Y 0                    // 玩家最小Y坐标

#define TOTAL_PLANES 4

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief 飞机配置表结构体（游戏性层面，与外观解耦）
 */
typedef struct {
    int id;
    const char *name;
    apr_id_t apr_id;                // 关联的外观模板
    int16_t hp_max;
    uint16_t shoot_cd;
    // 普通子弹
    int16_t bullet_damage;
    int16_t bullet_vx, bullet_vy;
    apr_id_t bullet_apr;
    // X键主动技能
    uint16_t skill_x_cd;
    void (*skill_x_active)(void);
    // Y键主动技能
    uint16_t skill_y_cd;
    void (*skill_y_active)(void);
    const char *skill_desc;
} plane_config_t;

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

    int current_plane_id;       // 当前飞机ID

    // 普通子弹参数
    int16_t bullet_damage;
    int16_t bullet_vx, bullet_vy;
    apr_id_t bullet_apr;
    // X键主动技能
    uint16_t skill_x_cd;
    void (*skill_x_active)(void);
    // Y键主动技能
    uint16_t skill_y_cd;
    void (*skill_y_active)(void);

    // 护盾
    lv_obj_t * shield_overlay;
    bool shield_active;

    // 速度加成 (Verdant)
    bool speed_boost_active;

    // 技能CD可视化用 — 上次使用时刻(play_tick)
    uint32_t skill_x_last_use;
    uint32_t skill_y_last_use;
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
static void player_skill_y_fire();

// 事件回调
static void player_event_game_start_cb(game_obj_t * src,game_obj_t * trg);
static void player_event_player_die_cb(game_obj_t * src,game_obj_t * trg);
static void player_event_hit_by_enemy_cb(game_obj_t * src,game_obj_t * trg);
static void player_event_hit_by_bullet_cb(game_obj_t * src,game_obj_t * trg);

// X键技能函数
static void player_skill_triple_shot(void);
static void player_skill_burn_bullet(void);
static void player_skill_freeze_bullet(void);

// Y键技能函数
static void player_skill_shield(void);
static void player_skill_flame_wall(void);
static void player_skill_bullet_slow(void);
static void player_skill_hp_reclaim(void);

// 技能辅助回调
static void player_shield_end_cb(game_obj_t *owner, void *usr_data);
static void player_slow_end_cb(game_obj_t *owner, void *usr_data);

#ifdef SIMULATOR
static void p2_player_init(lv_obj_t *parent);
static void p2_player_update(game_obj_t *g);
static void p2_player_show(game_obj_t *g);
static void p2_player_hide(game_obj_t *g);
static void p2_player_fire(void);
static void p2_player_x_handler(void);
static void p2_player_skill_y_fire(void);
static void p2_player_die_cb(game_obj_t *src, game_obj_t *trg);
static void p2_player_hit_enemy_cb(game_obj_t *src, game_obj_t *trg);
static void p2_player_hit_bullet_cb(game_obj_t *src, game_obj_t *trg);
static int16_t player_hp_modify_p2(int16_t delta);
#endif

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static player_t * player_p = NULL;

/**
 * @brief 4架飞机的配置表
 */
static const plane_config_t plane_configs[TOTAL_PLANES] = {
    // Player - 基础飞机: 护盾(Y) + 三向散射(X)
    {
        .id = 0, .name = "Player",
        .apr_id = APR_PLAYER_DEFAULT,
        .hp_max = 200, .shoot_cd = 200,
        .bullet_damage = 34, .bullet_vx = 0, .bullet_vy = -20,
        .bullet_apr = APR_BULLET_DEFAULT,
        .skill_x_cd = 3000,
        .skill_x_active = player_skill_triple_shot,
        .skill_y_cd = 5000,
        .skill_y_active = player_skill_shield,
        .skill_desc = "Triple: 3-way burst / Shield: 1s invincible",
    },
    // Ember - 火焰飞机: 火墙(Y) + 灼烧弹(X)
    {
        .id = 1, .name = "Ember",
        .apr_id = APR_PLAYER_EMBER,
        .hp_max = 200, .shoot_cd = 150,
        .bullet_damage = 80, .bullet_vx = 0, .bullet_vy = -25,
        .bullet_apr = APR_BULLET_EMBER,
        .skill_x_cd = 3000,
        .skill_x_active = player_skill_burn_bullet,
        .skill_y_cd = 5000,
        .skill_y_active = player_skill_flame_wall,
        .skill_desc = "Burn: DOT bullet / Flame Wall: clearing wave",
    },
    // Stream - 水流飞机: 子弹减速(Y) + 冻结弹(X)
    {
        .id = 2, .name = "Stream",
        .apr_id = APR_PLAYER_STREAM,
        .hp_max = 200, .shoot_cd = 250,
        .bullet_damage = 10, .bullet_vx = 0, .bullet_vy = -20,
        .bullet_apr = APR_BULLET_STREAM,
        .skill_x_cd = 3000,
        .skill_x_active = player_skill_freeze_bullet,
        .skill_y_cd = 6000,
        .skill_y_active = player_skill_bullet_slow,
        .skill_desc = "Freeze: immobilize / Slow: enemy bullets",
    },
    // Verdant - 自然飞机: 回血(Y) + 速度加成(X长按)
    {
        .id = 3, .name = "Verdant",
        .apr_id = APR_PLAYER_VERDANT,
        .hp_max = 250, .shoot_cd = 220,
        .bullet_damage = 15, .bullet_vx = 0, .bullet_vy = -20,
        .bullet_apr = APR_BULLET_VERDANT,
        .skill_x_cd = 0,          // Verdant X 用长按，不用CD
        .skill_x_active = NULL,   // 在 player_update 中直接处理
        .skill_y_cd = 6000,
        .skill_y_active = player_skill_hp_reclaim,
        .skill_desc = "Speed: double speed / Heal: +50 HP",
    },
};

#ifdef SIMULATOR
static player_t *player_p2 = NULL;
static uint32_t p2_last_skill_x_tick = 0;
static uint32_t p2_last_skill_y_tick = 0;
static player_t *active_skill_player = NULL;
#define SKP (active_skill_player ? active_skill_player : player_p)
#else
#define SKP player_p
#endif

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
  memset(player_p, 0, sizeof(player_t));
  if (player_p == NULL)
  {
    CONSOLE_ERROR("Failed to allocate memory for player object.");
    LOG_ERROR("Failed to allocate memory for player object.");
    sys_halt();
    return;
  }

  // 先获取默认飞机外观和配置
  apr_t *default_apr = apr_get(APR_PLAYER_DEFAULT);
  const plane_config_t *cfg = &plane_configs[0]; // 默认飞机0

  // 初始化玩家属性
  player_p->base.x = 512;
  player_p->base.y = 500;
  player_p->base.apr = default_apr;
  player_p->base.vx = 0;
  player_p->base.vy = 0;
  player_p->base.active = true;
  player_p->base.type = GAME_OBJ_TYPE_PLAYER;
  player_p->base.behave = NULL_BEHAVE;

  player_p->base.update = player_update;
  player_p->base.show = player_show;
  player_p->base.hide = player_hide;

  // 飞机游戏性参数（默认飞机0）
  player_p->current_plane_id = 0;
  player_p->hp_max = cfg->hp_max;
  player_p->hp = player_p->hp_max;
  player_p->shoot_cd = cfg->shoot_cd;
  player_p->bullet_damage = cfg->bullet_damage;
  player_p->bullet_vx = cfg->bullet_vx;
  player_p->bullet_vy = cfg->bullet_vy;
  player_p->bullet_apr = cfg->bullet_apr;
  player_p->skill_x_cd = cfg->skill_x_cd;
  player_p->skill_x_active = cfg->skill_x_active;
  player_p->skill_y_cd = cfg->skill_y_cd;
  player_p->skill_y_active = cfg->skill_y_active;
  player_p->shield_active = false;
  player_p->speed_boost_active = false;
  player_p->skill_x_last_use = 0;
  player_p->skill_y_last_use = 0;

  player_p->hp_bar = player_hp_bar_create((game_obj_t *)player_p, parent);
  player_p->base.obj = player_obj_create((game_obj_t *)player_p, parent);

  // 护盾遮罩 - 灰白色半透明矩形
  player_p->shield_overlay = lv_obj_create(parent);
  lv_obj_set_size(player_p->shield_overlay, 64, 64);
  lv_obj_set_style_bg_color(player_p->shield_overlay, lv_color_make(200, 200, 200), 0);
  lv_obj_set_style_bg_opa(player_p->shield_overlay, LV_OPA_50, 0);
  lv_obj_set_style_border_width(player_p->shield_overlay, 0, 0);
  lv_obj_add_flag(player_p->shield_overlay, LV_OBJ_FLAG_HIDDEN);

  lv_obj_set_pos(player_p->base.obj, player_p->base.x, player_p->base.y);

  game_register_obj((game_obj_t *)player_p);

  // 按键行为
  // X 主动技能
  input_sw_register_key_down_callback(KEY_EVENT_X, player_x_pressed_handler, player_p->skill_x_cd);
  CONSOLE_DEBUG("X key registered: cd=%d handler=%p active_func=%p",
          player_p->skill_x_cd, (void *)player_x_pressed_handler, (void *)player_p->skill_x_active);
  // A 射击
  input_sw_register_key_down_callback(KEY_EVENT_A, player_fire, player_p->shoot_cd);
  // Y 主动技能
  input_sw_register_key_down_callback(KEY_EVENT_Y, player_skill_y_fire, player_p->skill_y_cd);

  // 事件注册
  event_register(EVENT_GAME_START,player_event_game_start_cb);
  event_register(EVENT_PLAYER_DIE,player_event_player_die_cb);
  event_register(EVENT_PLAYER_HIT_ENEMY,player_event_hit_by_enemy_cb);
  event_register(EVENT_BULLET_HIT_PLAYER,player_event_hit_by_bullet_cb);


  CONSOLE_INFO("Player initialization complete.");
  CONSOLE_INFO("player properties:");
  CONSOLE_INFO("    width: %d",player_p->base.apr->w);
  CONSOLE_INFO("    height: %d",player_p->base.apr->h);
  CONSOLE_INFO("    speed: %d",player_p->base.speed);
  CONSOLE_INFO("    HP_max: %d",player_p->hp_max);
  CONSOLE_INFO("    shoot_cd: %dms",player_p->shoot_cd);
  CONSOLE_INFO("    bullet_damage: %d", player_p->bullet_damage);
  CONSOLE_INFO("    skill_x_cd: %dms", player_p->skill_x_cd);
  CONSOLE_INFO("    skill_y_cd: %dms", player_p->skill_y_cd);
  CONSOLE_INFO("");

#ifdef SIMULATOR
  /* 预分配 P2 玩家（隐藏，联机时显示） */
  p2_player_init(parent);
#endif

  return;
}

/**
 * @brief 获取玩家基类指针，方便在其他模块中调用
 */
game_obj_t * player_get_base()
{
  return (game_obj_t *)player_p;
}

/**
 * @brief 玩家HP修改函数
 */
int16_t player_hp_modify(int16_t delta)
{
  if (player_p == NULL) {
    CONSOLE_WARNING("Player object is not initialized. Cannot modify HP.");
    LOG_WARNING("Player object is not initialized. Cannot modify HP.");
    return 0;
  }
  if (!player_p->base.active) {
    CONSOLE_WARNING("Player is not active. Cannot modify HP.");
    LOG_WARNING("Player is not active. Cannot modify HP.");
    return player_p->hp;
  }
  player_p->hp += delta;
  if (player_p->hp > player_p->hp_max) {
    player_p->hp = player_p->hp_max;
    CONSOLE_INFO("Player HP modified. HP is at max: %d", player_p->hp_max);
  }
  if (player_p->hp <= 0) {
    player_p->hp = 0;
    CONSOLE_INFO("Player HP modified. HP has dropped to 0.");
    event_dispatch(EVENT_PLAYER_DIE,NULL,NULL);
  }
  lv_bar_set_value(player_p->hp_bar, player_p->hp, LV_ANIM_OFF);
  CONSOLE_INFO("Player HP modified by %d. Current HP: %d", delta, player_p->hp);
  return player_p->hp;
}

/**
 * @brief 应用飞机配置到玩家
 * @param plane_id 飞机ID (0-3)
 */
void player_apply_config(int plane_id)
{
    if (plane_id < 0 || plane_id >= TOTAL_PLANES) {
        CONSOLE_WARNING("Invalid plane id: %d", plane_id);
        LOG_WARNING("Invalid plane id: %d", plane_id);
        return;
    }
    if (player_p == NULL) {
        CONSOLE_WARNING("Player not initialized, cannot apply config.");
        LOG_WARNING("Player not initialized, cannot apply config.");
        return;
    }

    const plane_config_t *cfg = &plane_configs[plane_id];

    // 切换外观
    apr_apply(&player_p->base, cfg->apr_id);

    // 取消旧的按键回调
    input_sw_unregister_key_down_callback(KEY_EVENT_X, player_x_pressed_handler);
    input_sw_unregister_key_down_callback(KEY_EVENT_A, player_fire);
    input_sw_unregister_key_down_callback(KEY_EVENT_Y, player_skill_y_fire);

    // 重置护盾
    player_p->shield_active = false;
    lv_obj_add_flag(player_p->shield_overlay, LV_OBJ_FLAG_HIDDEN);
    player_p->speed_boost_active = false;
    player_p->skill_x_last_use = 0;
    player_p->skill_y_last_use = 0;

    // 更新游戏性参数
    player_p->current_plane_id = plane_id;
    player_p->hp_max = cfg->hp_max;
    player_p->hp = player_p->hp_max;
    player_p->shoot_cd = cfg->shoot_cd;
    player_p->bullet_damage = cfg->bullet_damage;
    player_p->bullet_vx = cfg->bullet_vx;
    player_p->bullet_vy = cfg->bullet_vy;
    player_p->bullet_apr = cfg->bullet_apr;
    player_p->skill_x_cd = cfg->skill_x_cd;
    player_p->skill_x_active = cfg->skill_x_active;
    player_p->skill_y_cd = cfg->skill_y_cd;
    player_p->skill_y_active = cfg->skill_y_active;

    // 重新注册按键回调 (Verdant X 用长按/状态检测，不注册 key_down)
    if (cfg->skill_x_cd > 0 && cfg->skill_x_active != NULL) {
        input_sw_register_key_down_callback(KEY_EVENT_X, player_x_pressed_handler, cfg->skill_x_cd);
        CONSOLE_DEBUG("X key re-registered: cd=%d handler=%p active=%p",
                cfg->skill_x_cd, (void *)player_x_pressed_handler, (void *)cfg->skill_x_active);
    } else {
        CONSOLE_DEBUG("X key NOT registered: cd=%d active_func=%p",
                cfg->skill_x_cd, (void *)cfg->skill_x_active);
    }
    input_sw_register_key_down_callback(KEY_EVENT_A, player_fire, player_p->shoot_cd);
    input_sw_register_key_down_callback(KEY_EVENT_Y, player_skill_y_fire, cfg->skill_y_cd);

    // 更新HP显示
    lv_bar_set_range(player_p->hp_bar, 0, player_p->hp_max);
    lv_bar_set_value(player_p->hp_bar, player_p->hp, LV_ANIM_OFF);

    CONSOLE_INFO(" Plane changed to: %s (HP=%d, bullet_dmg=%d, x_cd=%d, y_cd=%d)",cfg->name, cfg->hp_max, cfg->bullet_damage, cfg->skill_x_cd, cfg->skill_y_cd);
}

/**
 * @brief 获取当前飞机ID
 */
int player_get_current_plane(void)
{
    return player_p ? player_p->current_plane_id : 0;
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

/* ======================== P1 STATIC FUNCTIONS ======================== */

/**
 * @brief 玩家更新函数
 */
static void player_update(game_obj_t * g)
{
  if (fsm_get_state() != GS_PLAY && fsm_get_state() != GS_PAUSE && fsm_get_state() != GS_SETTING) {
    g->hide(g);
    return ;
  }
  g->show(g);
  if (fsm_get_state() != GS_PLAY || !player_p->base.active) {
    return ;
  }

  // Verdant 速度加成: 长按X键
  if (player_p->current_plane_id == 3) {
    player_p->speed_boost_active = input_sw_is_key_down(KEY_EVENT_X);
  }

  g->vx = joystick_get_x() / 127.0f * 7;
  g->vy = joystick_get_y() / 127.0f * 7;

  if (player_p->speed_boost_active) {
    g->vx *= 2;
    g->vy *= 2;
  }

  player_move(g);
}

/**
 * @brief 玩家显示函数
 */
static void player_show(game_obj_t * g)
{
  lv_obj_clear_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(((player_t *)g)->hp_bar, LV_OBJ_FLAG_HIDDEN);
  g->active = true;
}

/**
 * @brief 玩家隐藏函数
 */
static void player_hide(game_obj_t * g)
{
  lv_obj_add_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(((player_t *)g)->hp_bar, LV_OBJ_FLAG_HIDDEN);
  g->timered = false;
  g->active = false;
}

/**
 * @brief 玩家移动函数
 */
static void player_move(game_obj_t * g)
{
  if (g == NULL) {
    CONSOLE_WARNING("Player object is not initialized. Cannot move player.");
    LOG_WARNING("Player object is not initialized. Cannot move player.");
    return ;
  }

  if (g->active == false) {
    return ;
  }

  if (g->vx == 0 && g->vy == 0) {
    return ;
  }

  g->x += g->vx;
  g->y += g->vy;

  // 边界检查
  if (g->x < PLAYER_MIN_X) {
    g->x = PLAYER_MIN_X;
  }
  if (g->x > PLAYER_MAX_X) {
    g->x = PLAYER_MAX_X;
  }
  if (g->y < PLAYER_MIN_Y) {
    g->y = PLAYER_MIN_Y;
  }
  if (g->y > PLAYER_MAX_Y) {
    g->y = PLAYER_MAX_Y;
  }

  lv_obj_set_pos(g->obj,g->x,g->y);

  // CONSOLE_INFO("Player moved by dx: %d, dy: %d. New position - x: %d, y: %d", dx, dy, g->x, g->y);

  player_t * p = (player_t *)g;
  // 护盾遮罩跟随
  if (p->shield_active) {
    lv_obj_set_pos(p->shield_overlay, g->x,g->y);
  }

  return ;
}

/**
 * @brief 创建玩家HP条
 */
static lv_obj_t * player_hp_bar_create(game_obj_t * g, lv_obj_t * parent)
{
  lv_obj_t * hp_bar = lv_bar_create(parent);
  lv_obj_set_size(hp_bar, 162, 18);
  lv_obj_set_align(hp_bar, LV_ALIGN_TOP_LEFT);
  lv_obj_set_pos(hp_bar, 21, 18);
  lv_bar_set_range(hp_bar, 0, ((player_t *)g)->hp_max);
  lv_bar_set_value(hp_bar, ((player_t *)g)->hp, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(hp_bar,lv_palette_main(LV_PALETTE_RED),LV_PART_INDICATOR);

  CONSOLE_INFO("Player HP bar created, max HP: %d", ((player_t *)g)->hp_max);

  return hp_bar;
}

/**
 * @brief 创建玩家LVGL图像对象
 */
static lv_obj_t * player_obj_create(game_obj_t * g, lv_obj_t * parent)
{
  apr_t *apr = (apr_t *)g->apr; // 去除const
  lv_obj_t * img;
#ifdef SIMULATOR
  img = lv_img_create(parent);
  lv_img_set_src(img, &apr->img_dsc);
#else
  char path[128];
  img = lv_img_create(parent);
  lv_img_set_src(img, img_path(apr->img_name, path, 128));
#endif
  lv_obj_set_align(img, LV_ALIGN_TOP_LEFT);

  return img;
}

/**
 * @brief X键 — 发射特殊子弹
 */
static void player_x_pressed_handler()
{
    CONSOLE_DEBUG("X handler called, active=%d state=%d skill_x_active=%p",
            player_p->base.active, fsm_get_state(), (void *)player_p->skill_x_active);

    if (!player_p->base.active || fsm_get_state() != GS_PLAY) {
        CONSOLE_DEBUG("X handler blocked: active=%d state=%d",
                player_p->base.active, fsm_get_state());
        return ;
    }
    // 记录X技能使用时刻（用于CD可视化）
    player_p->skill_x_last_use = play_tick_get();

    // 派遣到各飞机的X技能函数
    if (player_p->skill_x_active) {
        CONSOLE_DEBUG("Calling skill_x_active (plane=%d)", player_p->current_plane_id);
        player_p->skill_x_active();
    } else {
        CONSOLE_DEBUG("skill_x_active is NULL!");
    }
    return ;
}

/**
 * @brief A键 — 普通射击
 */
static void player_fire()
{
  game_obj_t * g = (game_obj_t *)player_p;
  if (fsm_get_state() != GS_PLAY || !game_obj_is_active(g)) {
      return ;
  }
  audio_load(AUDIO_PLAYERFIRE,AUDIO_CHAN_AUTO,false);
  bullet_create(g,
                g->x + g->apr->w / 2 - g->apr->w / 16,
                g->y - g->apr->h / 4,
                player_p->bullet_vx, player_p->bullet_vy,
                player_p->bullet_damage,
                NULL_BEHAVE,
                player_p->bullet_apr);
}

/**
 * @brief Y键 — 主动技能
 */
static void player_skill_y_fire()
{
    if (!player_p->base.active || fsm_get_state() != GS_PLAY) {
        return ;
    }
    // 记录Y技能使用时刻（用于CD可视化）
    player_p->skill_y_last_use = play_tick_get();
    if (player_p->skill_y_active) {
        player_p->skill_y_active();
    }
}

// ==================== 技能实现 ====================

// ---------- X键技能 ----------

/**
 * @brief 三向散射 (Player X) — 发射3颗子弹
 */
static void player_skill_triple_shot(void)
{
    game_obj_t *g = (game_obj_t *)SKP;
    lv_coord_t cx = g->x + g->apr->w / 2 - g->apr->w / 16;
    lv_coord_t cy = g->y - g->apr->h / 4;

    bullet_create(g, cx, cy, 0, SKP->bullet_vy,
                  SKP->bullet_damage, NULL_BEHAVE, SKP->bullet_apr);
    bullet_create(g, cx, cy, -3, SKP->bullet_vy,
                  SKP->bullet_damage, NULL_BEHAVE, SKP->bullet_apr);
    bullet_create(g, cx, cy, 3, SKP->bullet_vy,
                  SKP->bullet_damage, NULL_BEHAVE, SKP->bullet_apr);
}

/**
 * @brief 灼烧弹 (Ember X) — 发射带灼烧标志的子弹
 */
static void player_skill_burn_bullet(void)
{
    game_obj_t *g = (game_obj_t *)SKP;
    lv_coord_t cx = g->x + g->apr->w / 2 - g->apr->w / 16;
    lv_coord_t cy = g->y - g->apr->h / 4;

    game_obj_t *bullet = bullet_create(g, cx, cy, 0, -10,
                                        40, NULL_BEHAVE, APR_BULLET_EMBER);
    if (bullet) {
        bullet_set_flags(bullet, BULLET_FLAG_BURN);
    }
}

/**
 * @brief 冻结弹 (Stream X) — 发射带冻结标志的子弹
 */
static void player_skill_freeze_bullet(void)
{
    game_obj_t *g = (game_obj_t *)SKP;
    lv_coord_t cx = g->x + g->apr->w / 2 - g->apr->w / 16;
    lv_coord_t cy = g->y - g->apr->h / 4;

    game_obj_t *bullet = bullet_create(g, cx, cy, 0, -10,
                                        30, NULL_BEHAVE, APR_BULLET_STREAM);
    if (bullet) bullet_set_flags(bullet, BULLET_FLAG_FREEZE);
}

static void player_skill_shield(void)
{
    if (SKP->shield_active) return;
    SKP->shield_active = true;
    lv_obj_clear_flag(SKP->shield_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(SKP->shield_overlay, SKP->base.x, SKP->base.y);
    timer_create((game_obj_t *)SKP, 1000, TIMER_MODE_ONCE, player_shield_end_cb, NULL);
}

static void player_shield_end_cb(game_obj_t *owner, void *usr_data)
{
    (void)owner; (void)usr_data;
    if (SKP == NULL) return;
    SKP->shield_active = false;
    lv_obj_add_flag(SKP->shield_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void player_skill_flame_wall(void)
{
    game_obj_t *g = (game_obj_t *)SKP;
    lv_coord_t cx = g->x + g->apr->w / 2 - 32;
    lv_coord_t cy = g->y - g->apr->h / 2;
    flame_wall_create(cx, cy, -8);
}

static void player_skill_bullet_slow(void)
{
    bullet_set_enemy_slow(true);
    ui_play_set_freeze_overlay(true);
    timer_create((game_obj_t *)SKP, 2000, TIMER_MODE_ONCE, player_slow_end_cb, NULL);
}

static void player_slow_end_cb(game_obj_t *owner, void *usr_data)
{
    (void)owner; (void)usr_data;
    bullet_set_enemy_slow(false);
    ui_play_set_freeze_overlay(false);
}

static void player_skill_hp_reclaim(void)
{
    player_hp_modify(50);
    CONSOLE_INFO("HP Reclaimed: +50 HP. Current HP: %d", SKP->hp);
}

// ==================== 事件回调 ====================

/**
 * @brief 游戏开始重置玩家
 */
static void player_event_game_start_cb(game_obj_t * src, game_obj_t * trg)
{
#ifdef SIMULATOR
  if (mp_get_state() == MP_STATE_CONNECTED) {
    /* 联机模式：两架飞机 */
    player_apply_config(ui_base_get_p1_selected_plane_id());
    player_p->base.x = 400; player_p->base.y = 500;
    player_p->hp = player_p->hp_max;
    lv_bar_set_value(player_p->hp_bar, player_p->hp, LV_ANIM_OFF);
    player_p->base.show((game_obj_t *)player_p);
    lv_obj_set_pos(player_p->base.obj, player_p->base.x, player_p->base.y);

    player_apply_p2_config(ui_base_get_p2_selected_plane_id());
    player_p2->base.x = 624; player_p2->base.y = 500;
    player_p2->hp = player_p2->hp_max;
    lv_bar_set_value(player_p2->hp_bar, player_p2->hp, LV_ANIM_OFF);
    player_p2->base.show((game_obj_t *)player_p2);
    lv_obj_set_pos(player_p2->base.obj, player_p2->base.x, player_p2->base.y);

    mp_start_game();
    CONSOLE_INFO("Dual-player game started. P1=%d P2=%d",
                 player_p->current_plane_id, player_p2->current_plane_id);
    return;
  }
#endif
  // 单人模式（不变）
  player_apply_config(ui_base_get_selected_plane_id());
  player_p->base.x = 512;
  player_p->base.y = 500;
  player_p->hp = player_p->hp_max;
  lv_bar_set_value(player_p->hp_bar, player_p->hp, LV_ANIM_OFF);
  player_p->base.show((game_obj_t *) player_p);
  lv_obj_set_pos(player_p->base.obj,player_p->base.x,player_p->base.y);
  CONSOLE_INFO("Player has been revived. HP reset to max: %d", player_p->hp_max);
}

/**
 * @brief 玩家死亡回调
 */
static void player_event_player_die_cb(game_obj_t * src, game_obj_t * trg)
{
  player_p->base.hide(&player_p->base);
  fsm_switch_state(GS_OVER);
}

/**
 * @brief 被敌人撞击
 */
static void player_event_hit_by_enemy_cb(game_obj_t * src, game_obj_t * trg)
{
  if (trg != (game_obj_t *)player_p) return;
  int16_t damage = enemy_get_damage(trg);
  player_hp_modify(-damage);
}

static void player_event_hit_by_bullet_cb(game_obj_t * src, game_obj_t * trg)
{
  if (trg != (game_obj_t *)player_p) return;
  int16_t damage = bullet_get_damage(src);
  player_hp_modify(-damage);
}

#ifdef SIMULATOR
/**
 * @brief P2 玩家初始化（远程操作）
 */
static void p2_player_init(lv_obj_t * parent)
{
    player_p2 = ram_malloc(sizeof(player_t));
    memset(player_p2, 0, sizeof(player_t));
    if (player_p2 == NULL) { CONSOLE_ERROR("P2 malloc failed"); return; }

    game_obj_t *g = &player_p2->base;
    g->x = 600; g->y = 500;
    g->type = GAME_OBJ_TYPE_PLAYER;
    g->active = false;
    g->behave.f = NULL;
    g->update = p2_player_update;
    g->show = p2_player_show;
    g->hide = p2_player_hide;

    apr_t *apr = apr_get(APR_PLAYER_DEFAULT);
    g->apr = apr;

    /* 使用与 P1 相同的默认配置 */
    const plane_config_t *cfg = &plane_configs[0];
    player_p2->hp_max = cfg->hp_max;
    player_p2->hp = cfg->hp_max;
    player_p2->shoot_cd = cfg->shoot_cd;
    player_p2->bullet_damage = cfg->bullet_damage;
    player_p2->bullet_vx = cfg->bullet_vx;
    player_p2->bullet_vy = cfg->bullet_vy;
    player_p2->bullet_apr = cfg->bullet_apr;
    player_p2->skill_x_cd = cfg->skill_x_cd;
    player_p2->skill_x_active = cfg->skill_x_active;
    player_p2->skill_y_cd = cfg->skill_y_cd;
    player_p2->skill_y_active = cfg->skill_y_active;
    player_p2->current_plane_id = 0;
    player_p2->shield_active = false;
    player_p2->speed_boost_active = false;

    /* LVGL 对象 */
    player_p2->hp_bar = player_hp_bar_create(g, parent);
    lv_obj_set_align(player_p2->hp_bar, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(player_p2->hp_bar, 21, 96);
    lv_obj_add_flag(player_p2->hp_bar, LV_OBJ_FLAG_HIDDEN);
    player_p2->shield_overlay = lv_obj_create(parent);
    lv_obj_set_size(player_p2->shield_overlay, 64, 64);
    lv_obj_set_style_bg_color(player_p2->shield_overlay, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_bg_opa(player_p2->shield_overlay, LV_OPA_30, 0);
    lv_obj_set_style_border_width(player_p2->shield_overlay, 0, 0);
    lv_obj_add_flag(player_p2->shield_overlay, LV_OBJ_FLAG_HIDDEN);

    g->obj = lv_img_create(parent);
#ifdef SIMULATOR
    lv_img_set_src(g->obj, &apr->img_dsc);
#else
    lv_img_set_src(g->obj, apr->img_name);
#endif
    lv_obj_set_pos(g->obj, g->x, g->y);
    lv_obj_add_flag(g->obj, LV_OBJ_FLAG_HIDDEN);

    /* 注册到游戏对象列表 */
    game_register_obj(g);

    /* 注册远程按键回调 */
    input_sw_register_key_down_callback(KEY_EVENT_RKEY_A, p2_player_fire, player_p2->shoot_cd);
    if (player_p2->skill_x_cd > 0 && player_p2->skill_x_active != NULL)
        input_sw_register_key_down_callback(KEY_EVENT_RKEY_X, p2_player_x_handler, player_p2->skill_x_cd);
    input_sw_register_key_down_callback(KEY_EVENT_RKEY_Y, p2_player_skill_y_fire, player_p2->skill_y_cd);

    /* 注册事件处理器（与 P1 共用 EVENT_GAME_START） */
    event_register(EVENT_PLAYER_DIE, p2_player_die_cb);
    event_register(EVENT_PLAYER_HIT_ENEMY, p2_player_hit_enemy_cb);
    event_register(EVENT_BULLET_HIT_PLAYER, p2_player_hit_bullet_cb);

    CONSOLE_INFO("P2 player initialized at (%d,%d)", g->x, g->y);
}
#endif

/**
 * @brief 查询护盾是否激活（供碰撞检测使用）
 */
bool player_is_shield_active(void)
{
    return player_p ? player_p->shield_active : false;
}

bool player_is_shield_active_for(game_obj_t *obj)
{
#ifdef SIMULATOR
    if (obj == (game_obj_t *)player_p2) return player_p2 && player_p2->shield_active;
#endif
    return player_is_shield_active();
}

uint32_t player_get_skill_x_cd(void)
{
    return player_p ? player_p->skill_x_cd : 0;
}

uint32_t player_get_skill_y_cd(void)
{
    return player_p ? player_p->skill_y_cd : 0;
}

uint32_t player_get_skill_x_elapsed(void)
{
    if (!player_p) return 0;
    if (player_p->skill_x_last_use == 0) return 0xFFFFFFFF; // 从未使用，始终就绪
    return play_tick_get() - player_p->skill_x_last_use;
}

uint32_t player_get_skill_y_elapsed(void)
{
    if (!player_p) return 0;
    if (player_p->skill_y_last_use == 0) return 0xFFFFFFFF;
    return play_tick_get() - player_p->skill_y_last_use;
}

/* ======================== P2 公共 API (全局函数) ======================== */
#ifdef SIMULATOR

game_obj_t * player_get_p2_base(void) { return (game_obj_t *)player_p2; }

void player_apply_p2_config(int plane_id)
{
    if (!player_p2 || plane_id < 0 || plane_id >= TOTAL_PLANES) return;
    const plane_config_t *cfg = &plane_configs[plane_id];

    /* 切换外观 */
    apr_apply(&player_p2->base, cfg->apr_id);

    /* 注销旧按键 */
    input_sw_unregister_key_down_callback(KEY_EVENT_RKEY_X, p2_player_x_handler);
    input_sw_unregister_key_down_callback(KEY_EVENT_RKEY_A, p2_player_fire);
    input_sw_unregister_key_down_callback(KEY_EVENT_RKEY_Y, p2_player_skill_y_fire);

    /* 重置状态 */
    player_p2->shield_active = false;
    lv_obj_add_flag(player_p2->shield_overlay, LV_OBJ_FLAG_HIDDEN);
    player_p2->speed_boost_active = false;

    /* 应用参数 */
    player_p2->current_plane_id = cfg->id;
    player_p2->hp_max = cfg->hp_max;
    player_p2->hp = cfg->hp_max;
    player_p2->shoot_cd = cfg->shoot_cd;
    player_p2->bullet_damage = cfg->bullet_damage;
    player_p2->bullet_vx = cfg->bullet_vx;
    player_p2->bullet_vy = cfg->bullet_vy;
    player_p2->bullet_apr = cfg->bullet_apr;
    player_p2->skill_x_cd = cfg->skill_x_cd;
    player_p2->skill_x_active = cfg->skill_x_active;
    player_p2->skill_y_cd = cfg->skill_y_cd;
    player_p2->skill_y_active = cfg->skill_y_active;
    player_p2->skill_x_last_use = 0;
    player_p2->skill_y_last_use = 0;

    /* 注册远程按键 */
    input_sw_register_key_down_callback(KEY_EVENT_RKEY_A, p2_player_fire, player_p2->shoot_cd);
    if (player_p2->skill_x_cd > 0 && player_p2->skill_x_active != NULL)
        input_sw_register_key_down_callback(KEY_EVENT_RKEY_X, p2_player_x_handler, player_p2->skill_x_cd);
    input_sw_register_key_down_callback(KEY_EVENT_RKEY_Y, p2_player_skill_y_fire, player_p2->skill_y_cd);

    lv_bar_set_range(player_p2->hp_bar, 0, player_p2->hp_max);
    lv_bar_set_value(player_p2->hp_bar, player_p2->hp, LV_ANIM_OFF);
}

uint32_t player_get_p2_skill_x_cd(void)  { return player_p2 ? player_p2->skill_x_cd : 0; }
uint32_t player_get_p2_skill_y_cd(void)  { return player_p2 ? player_p2->skill_y_cd : 0; }
uint32_t player_get_p2_skill_x_elapsed(void) {
    if (!player_p2) return 0;
    if (player_p2->skill_x_last_use == 0) return 0xFFFFFFFF;
    return play_tick_get() - player_p2->skill_x_last_use;
}
uint32_t player_get_p2_skill_y_elapsed(void) {
    if (!player_p2) return 0;
    if (player_p2->skill_y_last_use == 0) return 0xFFFFFFFF;
    return play_tick_get() - player_p2->skill_y_last_use;
}

/* ======================== P2 STATIC FUNCTIONS ======================== */

static void p2_player_update(game_obj_t *g)
{
    if (!player_p2) return;
    game_state_t gs = fsm_get_state();
    if (gs != GS_PLAY && gs != GS_PAUSE && gs != GS_SETTING) { g->hide(g); return; }
    g->show(g);
    if (gs != GS_PLAY || !player_p2->base.active) return;
    if (mp_get_state() != MP_STATE_GAME_PLAY) { g->hide(g); return; }

    g->vx = rjoystick_get_x() / 127.0f * 7;
    g->vy = rjoystick_get_y() / 127.0f * 7;

    if (player_p2->speed_boost_active) { g->vx *= 2; g->vy *= 2; }
    player_move(g);
}

static void p2_player_show(game_obj_t *g) {
    player_p2->base.active = true;
    lv_obj_clear_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(player_p2->hp_bar, LV_OBJ_FLAG_HIDDEN);
}

static void p2_player_hide(game_obj_t *g) {
    player_p2->base.active = false;
    lv_obj_add_flag(g->obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(player_p2->hp_bar, LV_OBJ_FLAG_HIDDEN);
}

static void p2_player_fire(void)
{
    if (!player_p2 || fsm_get_state() != GS_PLAY || !player_p2->base.active) return;
    game_obj_t *g = &player_p2->base;
    bullet_create(g,
        g->x + g->apr->w / 2 - g->apr->w / 16,
        g->y - g->apr->h / 4,
        player_p2->bullet_vx, player_p2->bullet_vy,
        player_p2->bullet_damage, NULL_BEHAVE, player_p2->bullet_apr);
}

static void p2_player_x_handler(void)
{
    if (!player_p2 || fsm_get_state() != GS_PLAY || !player_p2->base.active) return;
    if (player_p2->skill_x_active) {
        player_p2->skill_x_last_use = play_tick_get();
        active_skill_player = player_p2;
        player_p2->skill_x_active();
        active_skill_player = NULL;
    }
}

static void p2_player_skill_y_fire(void)
{
    if (!player_p2 || fsm_get_state() != GS_PLAY || !player_p2->base.active) return;
    if (player_p2->skill_y_active) {
        player_p2->skill_y_last_use = play_tick_get();
        active_skill_player = player_p2;
        player_p2->skill_y_active();
        active_skill_player = NULL;
    }
}

static void p2_player_die_cb(game_obj_t *src, game_obj_t *trg)
{
    if (!player_p2) return;
    player_p2->base.hide(&player_p2->base);
}

static void p2_player_hit_enemy_cb(game_obj_t *src, game_obj_t *trg)
{
    if (!player_p2 || !trg) return;
    if (trg != (game_obj_t *)player_p2) return;
    int dmg = enemy_get_damage(trg);
    player_hp_modify_p2(-dmg);
    if (player_p2->hp <= 0)
        event_dispatch(EVENT_PLAYER_DIE, &player_p2->base, NULL);
}

static void p2_player_hit_bullet_cb(game_obj_t *src, game_obj_t *trg)
{
    if (!player_p2 || !src) return;
    if (trg != (game_obj_t *)player_p2) return;
    int dmg = bullet_get_damage(src);
    player_hp_modify_p2(-dmg);
    if (player_p2->hp <= 0)
        event_dispatch(EVENT_PLAYER_DIE, &player_p2->base, NULL);
}

static int16_t player_hp_modify_p2(int16_t delta)
{
    if (!player_p2) return 0;
    if (!player_p2->base.active) return player_p2->hp;
    player_p2->hp += delta;
    if (player_p2->hp > player_p2->hp_max) player_p2->hp = player_p2->hp_max;
    if (player_p2->hp <= 0) {
        player_p2->hp = 0;
        event_dispatch(EVENT_PLAYER_DIE, &player_p2->base, NULL);
    }
    lv_bar_set_value(player_p2->hp_bar, player_p2->hp, LV_ANIM_OFF);
    return player_p2->hp;
}
#endif
