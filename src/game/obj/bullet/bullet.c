/**
 * @file 
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

// bullet image name
#define BULLET_IMG_NAME "bullet.bin"

#define BULLET_MAX_X SCREEN_WIDTH         // 子弹最大X坐标
#define BULLET_MIN_X 0                    // 子弹最小X坐标
#define BULLET_MAX_Y SCREEN_HEIGHT        // 子弹最大Y坐标
#define BULLET_MIN_Y 0                    // 子弹最小Y坐标

//子弹大小

#define BULLET_WIDTH 6
#define BULLET_HIGHT 16

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief 子弹结构体
 */
typedef struct bullet_t
{
    game_obj_t base;    // 基对象
    uint8_t damage;     // 子弹伤害
    game_obj_t *source; // 子弹发射源
    uint16_t pool_index; // 对象池索引
} bullet_t;

 /**********************
  *  STATIC PROTOTYPES
  **********************/

static void bullet_update(game_obj_t * g);
static void bullet_hide(game_obj_t * g);
static void bullet_show(game_obj_t * g);
static void bullet_move(bullet_t * b,lv_coord_t dx,lv_coord_t dy);

static void bullet_event_hit_enemy_cb(game_obj_t * scr,game_obj_t * trg);

/***********************
 *   GLOBAL PROTOTYPES
 ***********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static bullet_t bullets[MAX_BULLET_COUNT];                      // 子弹对象池
static pool_t bullet_pool;                                      // 子弹对象池管理器
static uint16_t bullet_free_indices[MAX_BULLET_COUNT];          // 子弹对象池空闲索引栈

static lv_img_dsc_t bullet_img_struct;                          // 子弹图像描述结构体
static uint8_t * bullet_img_buf = NULL;                         // 子弹图像缓冲区

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化子弹系统
 */
void bullet_init(lv_obj_t * parent)
{
    memset(bullets,0,sizeof(bullets));
    //初始化子弹对象池
    pool_init(&bullet_pool, bullet_free_indices, MAX_BULLET_COUNT);
    //初始化子弹属性

    char bullet_img_path[64];
    for (uint16_t i = 0;i < MAX_BULLET_COUNT;i++)
    {
        bullets[i].base.active = false;
        bullets[i].base.w = BULLET_WIDTH;
        bullets[i].base.h = BULLET_HIGHT;
        bullets[i].base.hitbox_x = 0;
        bullets[i].base.hitbox_y = 0;
        bullets[i].base.hitbox_h = 16;
        bullets[i].base.hitbox_w = 6;
        bullets[i].base.speed = 0.0f;
        bullets[i].base.x = 0;
        bullets[i].base.y = 0;
        bullets[i].base.type = GAME_OBJ_TYPE_BULLET;
        bullets[i].damage = 0;
        bullets[i].pool_index = POOL_INVALID_ID; //注意这里没有给他们分配内存 只是初始化了索引值，真正分配内存是在create函数中
        bullets[i].base.update = bullet_update;
        bullets[i].base.show = bullet_show;
        bullets[i].base.hide = bullet_hide;
        bullets[i].source = NULL;

        bullets[i].base.obj = img_create_from_dsc(parent,img_path(BULLET_IMG_NAME,bullet_img_path,64),bullets[i].base.w,bullets[i].base.h,bullet_img_buf,&bullet_img_struct,false);
        lv_obj_set_align(bullets[i].base.obj,LV_ALIGN_TOP_LEFT);
        
        console_out("[bullet_init] Bullet %d initialized with image: %s\n", i, bullet_img_path);
        
        bullets[i].base.hide((game_obj_t *)&bullets[i]);

        // obj register
        
        game_register_obj((game_obj_t *)&bullets[i]);
    }

    // event register
    event_register(EVENT_BULLET_HIT_ENEMY,bullet_event_hit_enemy_cb);

    console_out("[bullet_init] Bullet system initialized with max bullet count: %d\n", MAX_BULLET_COUNT);
    return ;
}

/**
 * @brief 创建子弹
 * @param source 子弹发射源
 * @param damage 子弹伤害
 * @param speed 子弹速度
 * @param x 子弹初始x坐标
 * @param y 子弹初始y坐标
 * @return 分配内存索引
 */
uint16_t bullet_create(game_obj_t *source, float speed,lv_coord_t x, lv_coord_t y, uint8_t damage)
{
    uint16_t index = pool_alloc(&bullet_pool);
    if (index == POOL_INVALID_ID)
    {
        CONSOLE("[WARNING] No available bullet slots! Max bullet count: %d", MAX_BULLET_COUNT);
        LOG("[WARNING] No available bullet slots! Max bullet count: %d", MAX_BULLET_COUNT);
        return POOL_INVALID_ID;
    }
    bullets[index].pool_index = index;
    bullets[index].source = source;
    bullets[index].damage = damage;
    bullets[index].base.speed = speed;
    bullets[index].base.x = x;
    bullets[index].base.y = y;
    lv_obj_set_pos(bullets[index].base.obj,x,y);
    bullets[index].base.show((game_obj_t *)&bullets[index]);
    // console_out("[bullet_create] Bullet created at index: %d, position: (%d, %d), speed: %.2f, damage: %d\n", index, x, y, speed, damage);
    return index;
}

/**
 * @brief 获取子弹伤害
 */
uint8_t bullet_get_damage(game_obj_t * bullet)
{
    return ((bullet_t *)bullet)->damage;
}

 /**********************
 *   STATIC FUNCTIONS
 **********************/

 /**
  * @brief 单个子弹更新
  */
static void bullet_update(game_obj_t * g)
{
    if (fsm_get_state() != GS_PLAY && fsm_get_state() != GS_PAUSE)
    {
        g->hide(g);
        return ;
    }
    if (fsm_get_state() == GS_PAUSE)
    {
        return ;
    }
    if (!g->active)
    {
        return ;
    }
    bullet_move((bullet_t *)g, 0, -g->speed);  // 固定向上移动
}

/**
 * @brief 子弹隐藏 停止渲染+标记不活跃
 */
static void bullet_hide(game_obj_t * g)
{
    g->active = false;
    lv_obj_add_flag(g->obj,LV_OBJ_FLAG_HIDDEN);
    //归还内存
    bullet_t * b = (bullet_t *)g;
    if (b->pool_index == POOL_INVALID_ID)
    {
        return ;
    }
    pool_free(&bullet_pool, b->pool_index);
    b->pool_index = POOL_INVALID_ID;
}

/**
 * @brief 子弹显示 激活对象+渲染 不分配内存
 */
static void bullet_show(game_obj_t * g)
{
    if (((bullet_t *)g)->pool_index == POOL_INVALID_ID)
    {
        console_out("[Error][bullet_show] Attempting to show a bullet that is not allocated! This should not happen.\n");
        log_out("[Error][bullet_show] Attempting to show a bullet that is not allocated! This should not happen.");
        return ;
    }
    g->active = true;
    lv_obj_clear_flag(g->obj,LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief 子弹移动函数 包括超出边界的处理
 */
static void bullet_move(bullet_t * b, lv_coord_t dx, lv_coord_t dy)
{
    if (b == NULL) return ;
    if (b->base.active == false) return ;
    if (dx == 0 && dy == 0) return ;
    
    b->base.x += dx;
    b->base.y += dy;

    lv_obj_set_pos(b->base.obj, b->base.x, b->base.y);

    // 检查是否超出边界
    if (b->base.x < BULLET_MIN_X || b->base.x > BULLET_MAX_X || b->base.y < BULLET_MIN_Y || b->base.y > BULLET_MAX_Y)
    {
        b->base.hide((game_obj_t *)b);
    }
}

/**
 * @brief 子弹击中敌人 子弹回调 子弹销毁
 * @param scr 子弹对象指针
 * @param trg 敌人对象指针
 */
static void bullet_event_hit_enemy_cb(game_obj_t * scr,game_obj_t * trg)
{
    scr->hide(scr);
}
